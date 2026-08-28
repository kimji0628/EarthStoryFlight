#pragma once

#include "TourCameraRangeSettings.h"
#include "TourCameraTiltSettings.h"

namespace EsfDebugLog
{
	void Log(LPCTSTR message);
	void LogTourCameraRangeMode(TourCameraRangeMode mode);
	void LogTourCameraRangeApplied(int rangeM, int replacedCount);
	void LogTourCameraRangePreserved();
	void LogTourCameraRangeNoTags();
	void LogTourCameraTiltMode(TourCameraTiltMode mode);
	void LogTourCameraTiltApplied(int tiltDeg, int replacedCount);
	void LogTourCameraTiltPreserved();
	void LogTourCameraTiltNoTags();
}
