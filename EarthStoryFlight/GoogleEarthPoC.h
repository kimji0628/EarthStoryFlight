#pragma once

class GoogleEarthPoC
{
public:
	static bool Run();

private:
	static CString GetSolutionRoot();
	static bool ResolveGoogleEarthPath(CString& outPath);
	static bool EnsureDirectory(const CString& path);
	static bool WritePoCKml(const CString& filePath);
	static bool LaunchGoogleEarthWithKml(const CString& googleEarthPath, const CString& kmlPath);
};
