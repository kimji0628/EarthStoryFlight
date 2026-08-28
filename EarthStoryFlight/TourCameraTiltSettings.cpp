#include "pch.h"
#include "TourCameraTiltSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	constexpr LPCTSTR kRegistrySection = _T("TourCameraTilt");
	constexpr LPCTSTR kRegistryModeKey = _T("Mode");
	constexpr LPCTSTR kRegistryUserTiltKey = _T("UserTiltDeg");
	constexpr int kMissingProfileValue = -1;
}

TourCameraTiltSettings& TourCameraTiltSettings::Instance()
{
	static TourCameraTiltSettings settings;
	return settings;
}

void TourCameraTiltSettings::Load()
{
	CWinApp* app = AfxGetApp();
	if (!app)
		return;

	const int modeValue = app->GetProfileInt(kRegistrySection, kRegistryModeKey, kMissingProfileValue);
	if (modeValue == kMissingProfileValue)
	{
		m_mode = TourCameraTiltMode::UserSpecified;
		m_userTiltDeg = kDefaultUserTiltDeg;
		Normalize();
		return;
	}

	m_mode = (modeValue == static_cast<int>(TourCameraTiltMode::UserSpecified))
		? TourCameraTiltMode::UserSpecified
		: TourCameraTiltMode::KmlRecommended;
	m_userTiltDeg = app->GetProfileInt(kRegistrySection, kRegistryUserTiltKey, kDefaultUserTiltDeg);
	Normalize();
}

void TourCameraTiltSettings::Save() const
{
	CWinApp* app = AfxGetApp();
	if (!app)
		return;

	app->WriteProfileInt(kRegistrySection, kRegistryModeKey, static_cast<int>(m_mode));
	app->WriteProfileInt(kRegistrySection, kRegistryUserTiltKey, m_userTiltDeg);
}

void TourCameraTiltSettings::SetKmlRecommended()
{
	m_mode = TourCameraTiltMode::KmlRecommended;
	Save();
}

void TourCameraTiltSettings::SetUserTiltDeg(int tiltDeg)
{
	m_userTiltDeg = tiltDeg;
	m_mode = TourCameraTiltMode::UserSpecified;
	Normalize();
	Save();
}

bool TourCameraTiltSettings::IsPresetTilt(int tiltDeg) const
{
	switch (tiltDeg)
	{
	case 30:
	case 45:
	case 60:
	case 70:
	case 75:
		return true;
	default:
		return false;
	}
}

int TourCameraTiltSettings::GetActivePresetTiltDeg() const
{
	if (m_mode != TourCameraTiltMode::UserSpecified)
		return 0;

	return IsPresetTilt(m_userTiltDeg) ? m_userTiltDeg : 0;
}

CString TourCameraTiltSettings::FormatTiltDeg(int tiltDeg) const
{
	CString text;
	text.Format(L"%d\u00B0", tiltDeg);
	return text;
}

CString TourCameraTiltSettings::FormatStatusText() const
{
	if (m_mode == TourCameraTiltMode::KmlRecommended)
		return L"\uCE74\uBA54\uB77C \uAE30\uC6B8\uAE30: KML \uAD8C\uC7A5\uAC12";

	CString text;
	text.Format(
		L"\uCE74\uBA54\uB77C \uAE30\uC6B8\uAE30: %s",
		FormatTiltDeg(m_userTiltDeg).GetString());
	return text;
}

void TourCameraTiltSettings::Normalize()
{
	if (m_userTiltDeg < kMinTiltDeg)
		m_userTiltDeg = kMinTiltDeg;
	if (m_userTiltDeg > kMaxTiltDeg)
		m_userTiltDeg = kMaxTiltDeg;
}
