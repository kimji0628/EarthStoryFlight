#include "pch.h"
#include "TourVideoRecorder.h"
#include "EsfDebugLog.h"

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	constexpr UINT32 kWidth = 1920;
	constexpr UINT32 kHeight = 1080;
	constexpr UINT32 kFps = 30;
	constexpr UINT32 kBitrate = 8000000;
	constexpr LONGLONG kFrameDuration = 10000000 / kFps;
	constexpr UINT32 kStride = kWidth * 4;

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif
	constexpr UINT kPrintFlags = PW_CLIENTONLY | PW_RENDERFULLCONTENT;

	void SafeRelease(IUnknown** pp)
	{
		if (pp && *pp)
		{
			(*pp)->Release();
			*pp = nullptr;
		}
	}

	CString HresultText(HRESULT hr)
	{
		CString text;
		text.Format(L"HRESULT 0x%08X", hr);
		return text;
	}

	bool EndsWithGoogleEarth(const CString& imagePath)
	{
		CString lower(imagePath);
		lower.MakeLower();
		return lower.Find(L"\\googleearth.exe") >= 0;
	}

	struct FindGeContext
	{
		HWND best = nullptr;
		LONG bestArea = 0;
	};

	BOOL CALLBACK EnumGoogleEarthWindows(HWND hwnd, LPARAM lParam)
	{
		if (!::IsWindowVisible(hwnd) || ::IsIconic(hwnd))
			return TRUE;

		DWORD processId = 0;
		::GetWindowThreadProcessId(hwnd, &processId);
		if (processId == 0)
			return TRUE;

		HANDLE process = ::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
		if (!process)
			return TRUE;

		wchar_t imagePath[MAX_PATH]{};
		DWORD imageSize = MAX_PATH;
		const BOOL queried = ::QueryFullProcessImageNameW(process, 0, imagePath, &imageSize);
		::CloseHandle(process);
		if (!queried || !EndsWithGoogleEarth(imagePath))
			return TRUE;

		RECT rect{};
		if (!::GetWindowRect(hwnd, &rect))
			return TRUE;

		const LONG area = (rect.right - rect.left) * (rect.bottom - rect.top);
		if (area < 200 * 200)
			return TRUE;

		auto* context = reinterpret_cast<FindGeContext*>(lParam);
		if (area > context->bestArea)
		{
			context->bestArea = area;
			context->best = hwnd;
		}
		return TRUE;
	}

	struct FindRenderContext
	{
		HWND named = nullptr;
		HWND largest = nullptr;
		LONG largestArea = 0;
	};

	bool IsChromeTitle(const wchar_t* title)
	{
		if (!title || !title[0])
			return false;
		return wcsstr(title, L"menubar") != nullptr
			|| wcsstr(title, L"LeftPanel") != nullptr
			|| wcsstr(title, L"toolbar") != nullptr
			|| wcsstr(title, L"splithandle") != nullptr
			|| wcsstr(title, L"touredit") != nullptr
			|| wcsstr(title, L"browser_page") != nullptr
			|| wcsstr(title, L"rubberband") != nullptr;
	}

	bool IsRenderTitle(const wchar_t* title)
	{
		if (!title || !title[0])
			return false;
		return wcsstr(title, L"RenderWidget") != nullptr
			|| wcsstr(title, L"Render Window") != nullptr
			|| wcsstr(title, L"RenderFrame") != nullptr;
	}

	BOOL CALLBACK EnumGoogleEarthRenderWindows(HWND hwnd, LPARAM lParam)
	{
		if (!::IsWindowVisible(hwnd))
			return TRUE;

		wchar_t title[256]{};
		::GetWindowTextW(hwnd, title, 256);

		RECT client{};
		if (!::GetClientRect(hwnd, &client))
			return TRUE;

		const LONG area = (client.right - client.left) * (client.bottom - client.top);
		if (area < 200 * 200)
			return TRUE;

		auto* context = reinterpret_cast<FindRenderContext*>(lParam);
		if (IsRenderTitle(title))
			context->named = hwnd;

		if (!IsChromeTitle(title) && area > context->largestArea)
		{
			context->largestArea = area;
			context->largest = hwnd;
		}
		return TRUE;
	}

	bool IsMostlyBlack(const BYTE* rgb, int width, int height, int stride)
	{
		if (!rgb || width < 2 || height < 2)
			return true;

		int samples = 0;
		int black = 0;
		for (int y = 0; y < height; y += 24)
		{
			const BYTE* row = rgb + static_cast<size_t>(y) * stride;
			for (int x = 0; x < width; x += 24)
			{
				const BYTE* p = row + x * 4;
				++samples;
				if (p[0] < 8 && p[1] < 8 && p[2] < 8)
					++black;
			}
		}
		return samples > 0 && black * 10 >= samples * 9;
	}

	HBITMAP CreateBgraDib(HDC dc, int width, int height, void** bits)
	{
		BITMAPINFO info{};
		info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		info.bmiHeader.biWidth = width;
		info.bmiHeader.biHeight = -height;
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		return ::CreateDIBSection(dc, &info, DIB_RGB_COLORS, bits, nullptr, 0);
	}

	struct CaptureScratch
	{
		HDC srcDc = nullptr;
		HBITMAP srcDib = nullptr;
		void* srcBits = nullptr;
		int srcWidth = 0;
		int srcHeight = 0;
		HGDIOBJ srcOld = nullptr;

		HDC destDc = nullptr;
		HBITMAP destDib = nullptr;
		void* destBits = nullptr;
		HGDIOBJ destOld = nullptr;

		bool preferChild = true;
		bool childValidated = false;

		void ResetSrc()
		{
			if (srcDc && srcOld)
				::SelectObject(srcDc, srcOld);
			srcOld = nullptr;
			if (srcDib)
				::DeleteObject(srcDib);
			srcDib = nullptr;
			srcBits = nullptr;
			if (srcDc)
				::DeleteDC(srcDc);
			srcDc = nullptr;
			srcWidth = 0;
			srcHeight = 0;
		}

		void ResetDest()
		{
			if (destDc && destOld)
				::SelectObject(destDc, destOld);
			destOld = nullptr;
			if (destDib)
				::DeleteObject(destDib);
			destDib = nullptr;
			destBits = nullptr;
			if (destDc)
				::DeleteDC(destDc);
			destDc = nullptr;
		}

		void Reset()
		{
			ResetSrc();
			ResetDest();
			preferChild = true;
			childValidated = false;
		}

		bool Ensure(int width, int height)
		{
			if (srcDc && srcDib && destDc && destDib && srcWidth == width && srcHeight == height)
				return true;

			ResetSrc();
			if (!destDc)
			{
				destDc = ::CreateCompatibleDC(nullptr);
				if (!destDc)
					return false;
				destDib = CreateBgraDib(destDc, static_cast<int>(kWidth), static_cast<int>(kHeight), &destBits);
				if (!destDib || !destBits)
				{
					ResetDest();
					return false;
				}
				destOld = ::SelectObject(destDc, destDib);
				::SetStretchBltMode(destDc, COLORONCOLOR);
			}

			srcDc = ::CreateCompatibleDC(destDc);
			if (!srcDc)
				return false;
			srcDib = CreateBgraDib(srcDc, width, height, &srcBits);
			if (!srcDib || !srcBits)
			{
				ResetSrc();
				return false;
			}
			srcOld = ::SelectObject(srcDc, srcDib);
			srcWidth = width;
			srcHeight = height;
			return true;
		}

		bool Stretch(int srcX, int srcY, int width, int height, BYTE* rgb32_1920x1080)
		{
			if (!destDc || !destBits)
				return false;
			const BOOL ok = ::StretchBlt(
				destDc, 0, 0, static_cast<int>(kWidth), static_cast<int>(kHeight),
				srcDc, srcX, srcY, width, height,
				SRCCOPY);
			if (ok)
				memcpy(rgb32_1920x1080, destBits, kStride * kHeight);
			return ok == TRUE;
		}
	};

	CaptureScratch g_scratch;

	bool CaptureHwndClient(HWND hwnd, BYTE* rgb32_1920x1080)
	{
		RECT client{};
		if (!::GetClientRect(hwnd, &client))
			return false;

		const int srcWidth = client.right - client.left;
		const int srcHeight = client.bottom - client.top;
		if (srcWidth < 2 || srcHeight < 2)
			return false;
		if (!g_scratch.Ensure(srcWidth, srcHeight))
			return false;

		const BOOL printed = ::PrintWindow(hwnd, g_scratch.srcDc, kPrintFlags);
		if (!printed || !g_scratch.Stretch(0, 0, srcWidth, srcHeight, rgb32_1920x1080))
			return false;

		if (!g_scratch.childValidated)
		{
			if (IsMostlyBlack(rgb32_1920x1080, static_cast<int>(kWidth), static_cast<int>(kHeight), static_cast<int>(kStride)))
				return false;
			g_scratch.childValidated = true;
		}
		return true;
	}

	bool CaptureMainCroppedToRender(HWND mainWindow, HWND renderWindow, BYTE* rgb32_1920x1080)
	{
		RECT renderClient{};
		if (!::GetClientRect(renderWindow, &renderClient))
			return false;

		POINT renderInMain{ 0, 0 };
		::MapWindowPoints(renderWindow, mainWindow, &renderInMain, 1);

		RECT mainClient{};
		if (!::GetClientRect(mainWindow, &mainClient))
			return false;

		const int mainWidth = mainClient.right - mainClient.left;
		const int mainHeight = mainClient.bottom - mainClient.top;
		const int cropX = renderInMain.x;
		const int cropY = renderInMain.y;
		const int cropW = renderClient.right - renderClient.left;
		const int cropH = renderClient.bottom - renderClient.top;
		if (mainWidth < 2 || mainHeight < 2 || cropW < 2 || cropH < 2)
			return false;
		if (cropX < 0 || cropY < 0 || cropX + cropW > mainWidth || cropY + cropH > mainHeight)
			return false;
		if (!g_scratch.Ensure(mainWidth, mainHeight))
			return false;

		const BOOL printed = ::PrintWindow(mainWindow, g_scratch.srcDc, kPrintFlags);
		return printed && g_scratch.Stretch(cropX, cropY, cropW, cropH, rgb32_1920x1080);
	}

	HRESULT ConfigureSinkWriter(IMFSinkWriter* writer, DWORD* streamIndex)
	{
		IMFMediaType* typeOut = nullptr;
		HRESULT hr = MFCreateMediaType(&typeOut);
		if (FAILED(hr))
			return hr;

		hr = typeOut->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		if (SUCCEEDED(hr))
			hr = typeOut->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
		if (SUCCEEDED(hr))
			hr = typeOut->SetUINT32(MF_MT_AVG_BITRATE, kBitrate);
		if (SUCCEEDED(hr))
			hr = typeOut->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
		if (SUCCEEDED(hr))
			hr = MFSetAttributeSize(typeOut, MF_MT_FRAME_SIZE, kWidth, kHeight);
		if (SUCCEEDED(hr))
			hr = MFSetAttributeRatio(typeOut, MF_MT_FRAME_RATE, kFps, 1);
		if (SUCCEEDED(hr))
			hr = MFSetAttributeRatio(typeOut, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
		if (SUCCEEDED(hr))
			hr = writer->AddStream(typeOut, streamIndex);
		typeOut->Release();
		if (FAILED(hr))
			return hr;

		IMFMediaType* typeIn = nullptr;
		hr = MFCreateMediaType(&typeIn);
		if (FAILED(hr))
			return hr;
		hr = typeIn->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
		if (SUCCEEDED(hr))
			hr = typeIn->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
		if (SUCCEEDED(hr))
			hr = typeIn->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
		if (SUCCEEDED(hr))
			hr = MFSetAttributeSize(typeIn, MF_MT_FRAME_SIZE, kWidth, kHeight);
		if (SUCCEEDED(hr))
			hr = MFSetAttributeRatio(typeIn, MF_MT_FRAME_RATE, kFps, 1);
		if (SUCCEEDED(hr))
			hr = MFSetAttributeRatio(typeIn, MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
		if (SUCCEEDED(hr))
			hr = typeIn->SetUINT32(MF_MT_DEFAULT_STRIDE, static_cast<UINT32>(kStride));
		if (SUCCEEDED(hr))
			hr = writer->SetInputMediaType(*streamIndex, typeIn, nullptr);
		typeIn->Release();
		return hr;
	}

	HRESULT WriteRgbFrame(IMFSinkWriter* writer, DWORD streamIndex, const BYTE* rgb, LONGLONG timestamp)
	{
		IMFMediaBuffer* buffer = nullptr;
		HRESULT hr = MFCreateMemoryBuffer(kStride * kHeight, &buffer);
		if (FAILED(hr))
			return hr;

		BYTE* dest = nullptr;
		hr = buffer->Lock(&dest, nullptr, nullptr);
		if (SUCCEEDED(hr))
		{
			memcpy(dest, rgb, kStride * kHeight);
			buffer->Unlock();
			hr = buffer->SetCurrentLength(kStride * kHeight);
		}

		IMFSample* sample = nullptr;
		if (SUCCEEDED(hr))
			hr = MFCreateSample(&sample);
		if (SUCCEEDED(hr))
			hr = sample->AddBuffer(buffer);
		if (SUCCEEDED(hr))
			hr = sample->SetSampleTime(timestamp);
		if (SUCCEEDED(hr))
			hr = sample->SetSampleDuration(kFrameDuration);
		if (SUCCEEDED(hr))
			hr = writer->WriteSample(streamIndex, sample);

		if (sample)
			sample->Release();
		buffer->Release();
		return hr;
	}
}

TourVideoRecorder& TourVideoRecorder::Instance()
{
	static TourVideoRecorder instance;
	return instance;
}

bool TourVideoRecorder::IsRecording() const
{
	return ::InterlockedCompareExchange(const_cast<volatile LONG*>(&m_running), 0, 0) != 0;
}

CString TourVideoRecorder::GetOutputPath() const
{
	CSingleLock lock(&m_lock, TRUE);
	return m_outputPath;
}

HWND TourVideoRecorder::FindGoogleEarthWindow()
{
	FindGeContext context;
	::EnumWindows(EnumGoogleEarthWindows, reinterpret_cast<LPARAM>(&context));
	return context.best;
}

HWND TourVideoRecorder::FindGoogleEarthRenderWindow(HWND geWindow)
{
	if (!geWindow || !::IsWindow(geWindow))
		return nullptr;

	FindRenderContext context;
	::EnumChildWindows(geWindow, EnumGoogleEarthRenderWindows, reinterpret_cast<LPARAM>(&context));
	if (context.named)
		return context.named;
	return context.largest;
}

bool TourVideoRecorder::CaptureGoogleEarthFrame(HWND geWindow, BYTE* rgb32_1920x1080)
{
	if (!geWindow || !::IsWindow(geWindow) || !rgb32_1920x1080)
		return false;

	const HWND renderWindow = FindGoogleEarthRenderWindow(geWindow);
	if (!renderWindow || !::IsWindow(renderWindow))
		return false;

	if (g_scratch.preferChild && CaptureHwndClient(renderWindow, rgb32_1920x1080))
		return true;

	g_scratch.preferChild = false;
	return CaptureMainCroppedToRender(geWindow, renderWindow, rgb32_1920x1080);
}

UINT __cdecl TourRecordThreadProc(LPVOID param)
{
	reinterpret_cast<TourVideoRecorder*>(param)->CaptureLoop();
	return 0;
}

bool TourVideoRecorder::Start(const CString& outputPath, CString& errorMessage)
{
	errorMessage.Empty();
	CSingleLock lock(&m_lock, TRUE);

	if (IsRecording())
	{
		errorMessage = L"이미 녹화 중입니다.";
		return false;
	}

	if (outputPath.IsEmpty())
	{
		errorMessage = L"저장 경로가 비어 있습니다.";
		return false;
	}

	const ULONGLONG now = ::GetTickCount64();
	if (m_tourEndTick != 0 && now >= m_tourEndTick)
		m_tourEndTick = 0;

	m_outputPath = outputPath;
	m_stopRequested = 0;
	m_loggedCaptureError = 0;
	::InterlockedExchange(&m_running, 1);

	m_thread = ::AfxBeginThread(
		TourRecordThreadProc,
		this,
		THREAD_PRIORITY_NORMAL,
		0,
		CREATE_SUSPENDED);
	if (!m_thread)
	{
		::InterlockedExchange(&m_running, 0);
		errorMessage = L"녹화 스레드를 시작할 수 없습니다.";
		return false;
	}

	m_thread->m_bAutoDelete = FALSE;
	m_thread->ResumeThread();
	EsfDebugLog::Log(L"Tour video recording started: " + outputPath);
	return true;
}

void TourVideoRecorder::Stop()
{
	CWinThread* thread = nullptr;
	{
		CSingleLock lock(&m_lock, TRUE);
		if (!IsRecording() && m_thread == nullptr)
			return;
		::InterlockedExchange(&m_stopRequested, 1);
		thread = m_thread;
		m_thread = nullptr;
	}

	if (thread)
	{
		::WaitForSingleObject(thread->m_hThread, 15000);
		delete thread;
	}

	CSingleLock lock(&m_lock, TRUE);
	::InterlockedExchange(&m_running, 0);
	EsfDebugLog::Log(L"Tour video recording stopped.");
}

void TourVideoRecorder::NotifyTourStarted(int durationMs)
{
	if (durationMs <= 0)
		return;

	CSingleLock lock(&m_lock, TRUE);
	m_tourEndTick = ::GetTickCount64() + static_cast<ULONGLONG>(durationMs);
}

bool TourVideoRecorder::ShouldAutoStop() const
{
	CSingleLock lock(&m_lock, TRUE);
	if (!IsRecording() || m_tourEndTick == 0)
		return false;
	return ::GetTickCount64() >= m_tourEndTick;
}

void TourVideoRecorder::CaptureLoop()
{
	HRESULT hrCo = ::CoInitializeEx(nullptr, COINIT_MULTITHREADED);
	HRESULT hrMf = MFStartup(MF_VERSION);

	CString outputPath;
	{
		CSingleLock lock(&m_lock, TRUE);
		outputPath = m_outputPath;
	}

	IMFSinkWriter* writer = nullptr;
	DWORD streamIndex = 0;
	HRESULT hr = S_OK;
	if (SUCCEEDED(hrMf))
		hr = MFCreateSinkWriterFromURL(outputPath, nullptr, nullptr, &writer);
	if (SUCCEEDED(hr) && writer)
		hr = ConfigureSinkWriter(writer, &streamIndex);
	if (SUCCEEDED(hr) && writer)
		hr = writer->BeginWriting();

	if (FAILED(hr) || !writer)
	{
		EsfDebugLog::Log(L"Tour video encoder failed: " + HresultText(hr));
		SafeRelease(reinterpret_cast<IUnknown**>(&writer));
		if (SUCCEEDED(hrMf))
			MFShutdown();
		if (SUCCEEDED(hrCo))
			::CoUninitialize();
		::InterlockedExchange(&m_running, 0);
		return;
	}

	std::vector<BYTE> frame(kStride * kHeight);
	LONGLONG timestamp = 0;
	UINT frameCount = 0;
	g_scratch.Reset();

	while (::InterlockedCompareExchange(&m_stopRequested, 0, 0) == 0)
	{
		const DWORD startTick = ::GetTickCount();
		const HWND geWindow = FindGoogleEarthWindow();
		if (geWindow && CaptureGoogleEarthFrame(geWindow, frame.data()))
		{
			const HRESULT writeHr = WriteRgbFrame(writer, streamIndex, frame.data(), timestamp);
			if (SUCCEEDED(writeHr))
			{
				timestamp += kFrameDuration;
				++frameCount;
			}
			else if (::InterlockedCompareExchange(&m_loggedCaptureError, 1, 0) == 0)
			{
				EsfDebugLog::Log(L"Tour video frame write failed: " + HresultText(writeHr));
			}
		}

		const DWORD elapsed = ::GetTickCount() - startTick;
		const DWORD frameMs = 1000 / kFps;
		if (elapsed < frameMs)
			::Sleep(frameMs - elapsed);
	}

	const HRESULT finalizeHr = writer->Finalize();
	SafeRelease(reinterpret_cast<IUnknown**>(&writer));
	MFShutdown();
	if (SUCCEEDED(hrCo))
		::CoUninitialize();

	g_scratch.Reset();
	if (FAILED(finalizeHr))
		EsfDebugLog::Log(L"Tour video finalize failed: " + HresultText(finalizeHr));
	else if (frameCount == 0)
		EsfDebugLog::Log(L"Tour video capture failed: no frames written.");
	::InterlockedExchange(&m_running, 0);
}
