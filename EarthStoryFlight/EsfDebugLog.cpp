#include "pch.h"
#include "EsfDebugLog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace EsfDebugLog
{
	void Log(LPCTSTR message)
	{
		if (!message)
			return;

		CString line(message);
		line += L"\r\n";
		::OutputDebugStringW(line);
	}

	void LogTourCameraRangeMode(TourCameraRangeMode mode)
	{
		if (mode == TourCameraRangeMode::KmlRecommended)
			Log(L"[Tour] Camera Range Mode = KmlRecommended");
		else
			Log(L"[Tour] Camera Range Mode = UserSpecified");
	}

	void LogTourCameraRangeApplied(int rangeM, int replacedCount)
	{
		CString message;
		message.Format(L"[Tour] Camera Range = %d m", rangeM);
		Log(message);

		message.Format(L"[Tour] Replaced LookAt range count = %d", replacedCount);
		Log(message);
	}

	void LogTourCameraRangePreserved()
	{
		Log(L"[Tour] Original KML range values preserved");
	}

	void LogTourCameraRangeNoTags()
	{
		Log(L"[Tour] No LookAt range tags found; original tour behavior preserved");
	}

	void LogTourCameraTiltMode(TourCameraTiltMode mode)
	{
		if (mode == TourCameraTiltMode::KmlRecommended)
			Log(L"[Tour] Camera Tilt Mode = KmlRecommended");
		else
			Log(L"[Tour] Camera Tilt Mode = UserSpecified");
	}

	void LogTourCameraTiltApplied(int tiltDeg, int replacedCount)
	{
		CString message;
		message.Format(L"[Tour] Camera Tilt = %d deg", tiltDeg);
		Log(message);

		message.Format(L"[Tour] Replaced LookAt tilt count = %d", replacedCount);
		Log(message);
	}

	void LogTourCameraTiltPreserved()
	{
		Log(L"[Tour] Original KML tilt values preserved");
	}

	void LogTourCameraTiltNoTags()
	{
		Log(L"[Tour] No LookAt tilt tags found; original tour behavior preserved");
	}
}
