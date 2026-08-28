#include "pch.h"
#include "TourCameraRangeSettings.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

namespace
{
	constexpr LPCTSTR kRegistrySection = _T("TourCameraRange");
	constexpr LPCTSTR kRegistryModeKey = _T("Mode");
	constexpr LPCTSTR kRegistryUserRangeKey = _T("UserRangeM");
	constexpr int kMissingProfileValue = -1;
}

TourCameraRangeSettings& TourCameraRangeSettings::Instance()
{
	static TourCameraRangeSettings settings;
	return settings;
}

void TourCameraRangeSettings::Load()
{
	CWinApp* app = AfxGetApp();
	if (!app)
		return;

	const int modeValue = app->GetProfileInt(kRegistrySection, kRegistryModeKey, kMissingProfileValue);
	if (modeValue == kMissingProfileValue)
	{
		m_mode = TourCameraRangeMode::UserSpecified;
		m_userRangeM = kDefaultUserRangeM;
		Normalize();
		return;
	}

	m_mode = (modeValue == static_cast<int>(TourCameraRangeMode::UserSpecified))
		? TourCameraRangeMode::UserSpecified
		: TourCameraRangeMode::KmlRecommended;
	m_userRangeM = app->GetProfileInt(kRegistrySection, kRegistryUserRangeKey, kDefaultUserRangeM);
	Normalize();
}

void TourCameraRangeSettings::Save() const
{
	CWinApp* app = AfxGetApp();
	if (!app)
		return;

	app->WriteProfileInt(kRegistrySection, kRegistryModeKey, static_cast<int>(m_mode));
	app->WriteProfileInt(kRegistrySection, kRegistryUserRangeKey, m_userRangeM);
}

void TourCameraRangeSettings::SetKmlRecommended()
{
	m_mode = TourCameraRangeMode::KmlRecommended;
	Save();
}

void TourCameraRangeSettings::SetUserRangeM(int rangeM)
{
	m_userRangeM = rangeM;
	m_mode = TourCameraRangeMode::UserSpecified;
	Normalize();
	Save();
}

bool TourCameraRangeSettings::IsPresetRange(int rangeM) const
{
	switch (rangeM)
	{
	case 1000:
	case 3000:
	case 5000:
	case 10000:
	case 20000:
	case 30000:
		return true;
	default:
		return false;
	}
}

int TourCameraRangeSettings::GetActivePresetRangeM() const
{
	if (m_mode != TourCameraRangeMode::UserSpecified)
		return 0;

	return IsPresetRange(m_userRangeM) ? m_userRangeM : 0;
}

CString TourCameraRangeSettings::FormatRangeM(int rangeM) const
{
	CString number;
	number.Format(L"%d", rangeM);

	CString formatted;
	const int length = number.GetLength();
	for (int index = 0; index < length; ++index)
	{
		if (index > 0 && ((length - index) % 3) == 0)
			formatted += L',';
		formatted += number[index];
	}

	formatted += L" m";
	return formatted;
}

CString TourCameraRangeSettings::FormatStatusText() const
{
	if (m_mode == TourCameraRangeMode::KmlRecommended)
		return L"\uCE74\uBA54\uB77C \uAC70\uB9AC: KML \uAD8C\uC7A5\uAC12";

	CString text;
	text.Format(L"\uCE74\uBA54\uB77C \uAC70\uB9AC: %s", FormatRangeM(m_userRangeM).GetString());
	return text;
}

void TourCameraRangeSettings::Normalize()
{
	if (m_userRangeM < kMinRangeM)
		m_userRangeM = kMinRangeM;
	if (m_userRangeM > kMaxRangeM)
		m_userRangeM = kMaxRangeM;
}
