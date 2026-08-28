#pragma once

class GoogleEarthPoC
{
public:
	static bool Run();
	static bool RunGeumsan8SceneryTour(CString& outErrorMessage);
	static bool RunGeumsan10ScenicTour(CString& outErrorMessage);
	static bool RunExodusMemphisToJerichoTour(CString& outErrorMessage);
	static CString GetSolutionRootPath();

private:
	static CString GetSolutionRoot();
	static bool ResolveGoogleEarthPath(CString& outPath);
	static bool EnsureDirectory(const CString& path);
	static bool WriteStoryFlightTourKml(const CString& filePath);
	static bool LaunchGoogleEarthWithKml(const CString& googleEarthPath, const CString& kmlPath);
	static bool ReadTextFileUtf8(const CString& filePath, CStringA& outUtf8);
	static bool ValidateTourKmlContent(const CStringA& utf8Content);
	static bool RunTourKmlFile(
		const CString& kmlPath,
		const CString& kmlFileName,
		const CString& tourNotFoundLabel,
		CString& outErrorMessage);
	static bool ExtractGxTourInnerContent(const CStringA& utf8Content, CStringA& outInnerContent, CString& outErrorMessage);
	static bool WriteExodusTourPlayKml(const CStringA& tourInnerContent, const CString& outputPath, CString& outErrorMessage);
	static bool ApplyCameraRangeToTourContent(CStringA& tourInnerContent, int& outReplacedCount);
	static bool ApplyCameraTiltToTourContent(CStringA& tourInnerContent, int& outReplacedCount);
	static bool RunExodusTourWithDualLaunch(
		const CString& sourceFileName,
		const CString& playFileName,
		CString& outErrorMessage);
	static bool RunTourWithDualLaunch(
		const CString& sourcePath,
		const CString& playPath,
		CString& outErrorMessage);
};

