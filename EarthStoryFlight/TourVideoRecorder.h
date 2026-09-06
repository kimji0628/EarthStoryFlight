#pragma once

#include <afxmt.h>

class TourVideoRecorder
{
public:
	static TourVideoRecorder& Instance();

	bool IsRecording() const;
	CString GetOutputPath() const;

	bool Start(const CString& outputPath, CString& errorMessage);
	void Stop();
	void NotifyTourStarted(int durationMs);
	bool ShouldAutoStop() const;

private:
	TourVideoRecorder() = default;
	~TourVideoRecorder() = default;
	TourVideoRecorder(const TourVideoRecorder&) = delete;
	TourVideoRecorder& operator=(const TourVideoRecorder&) = delete;

	void CaptureLoop();
	static HWND FindGoogleEarthWindow();
	static HWND FindGoogleEarthRenderWindow(HWND geWindow);
	static bool CaptureGoogleEarthFrame(HWND geWindow, BYTE* rgb32_1920x1080);

	friend UINT __cdecl TourRecordThreadProc(LPVOID param);

	mutable CCriticalSection m_lock;
	CWinThread* m_thread = nullptr;
	CString m_outputPath;
	ULONGLONG m_tourEndTick = 0;
	volatile LONG m_running = 0;
	volatile LONG m_stopRequested = 0;
	volatile LONG m_loggedCaptureError = 0;
};
