#pragma once

class CEarthStoryFlightView;

class CMainFrame : public CFrameWnd
{
protected:
	CMainFrame() noexcept;
	DECLARE_DYNCREATE(CMainFrame)

public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnMapOpen();
	afx_msg void OnTourCameraRangeKml();
	afx_msg void OnTourCameraRange1000();
	afx_msg void OnTourCameraRange3000();
	afx_msg void OnTourCameraRange5000();
	afx_msg void OnTourCameraRange10000();
	afx_msg void OnTourCameraRange20000();
	afx_msg void OnTourCameraRange30000();
	afx_msg void OnTourCameraRangeCustom();
	afx_msg void OnTourCameraTiltKml();
	afx_msg void OnTourCameraTilt30();
	afx_msg void OnTourCameraTilt45();
	afx_msg void OnTourCameraTilt60();
	afx_msg void OnTourCameraTilt70();
	afx_msg void OnTourCameraTilt75();
	afx_msg void OnTourCameraTiltCustom();
	afx_msg void OnTourGeumsanTest();
	afx_msg void OnTourGeumsan8Scenery();
	afx_msg void OnTourGeumsan10Scenic();
	afx_msg void OnTourExodusMemphisToJericho();
	DECLARE_MESSAGE_MAP()

private:
	CEarthStoryFlightView* GetActiveEarthView() const;
	void ApplyKoreanMenuText();
	void UpdateCameraRangeMenuChecks();
	void UpdateCameraTiltMenuChecks();
	void UpdateTourCameraStatus();
	void RefreshTourCameraUi();
	void SetCameraRangePreset(int rangeM);
	void SetCameraTiltPreset(int tiltDeg);

	CStatusBar m_wndStatusBar;
};
