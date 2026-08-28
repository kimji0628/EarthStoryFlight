#pragma once

enum class TourCameraTiltMode
{
	KmlRecommended = 0,
	UserSpecified = 1,
};

class TourCameraTiltSettings
{
public:
	static TourCameraTiltSettings& Instance();

	static constexpr int kMinTiltDeg = 0;
	static constexpr int kMaxTiltDeg = 80;
	static constexpr int kDefaultUserTiltDeg = 70;

	void Load();
	void Save() const;

	TourCameraTiltMode GetMode() const { return m_mode; }
	int GetUserTiltDeg() const { return m_userTiltDeg; }

	void SetKmlRecommended();
	void SetUserTiltDeg(int tiltDeg);

	bool IsPresetTilt(int tiltDeg) const;
	int GetActivePresetTiltDeg() const;

	CString FormatTiltDeg(int tiltDeg) const;
	CString FormatStatusText() const;

private:
	TourCameraTiltSettings() = default;

	void Normalize();

	TourCameraTiltMode m_mode = TourCameraTiltMode::UserSpecified;
	int m_userTiltDeg = kDefaultUserTiltDeg;
};
