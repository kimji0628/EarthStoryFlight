#pragma once

enum class TourCameraRangeMode
{
	KmlRecommended = 0,
	UserSpecified = 1,
};

class TourCameraRangeSettings
{
public:
	static TourCameraRangeSettings& Instance();

	static constexpr int kMinRangeM = 1000;
	static constexpr int kMaxRangeM = 30000;
	static constexpr int kDefaultUserRangeM = 10000;

	void Load();
	void Save() const;

	TourCameraRangeMode GetMode() const { return m_mode; }
	int GetUserRangeM() const { return m_userRangeM; }

	void SetKmlRecommended();
	void SetUserRangeM(int rangeM);

	bool IsPresetRange(int rangeM) const;
	int GetActivePresetRangeM() const;

	CString FormatStatusText() const;
	CString FormatRangeM(int rangeM) const;

private:
	TourCameraRangeSettings() = default;

	void Normalize();

	TourCameraRangeMode m_mode = TourCameraRangeMode::UserSpecified;
	int m_userRangeM = kDefaultUserRangeM;
};
