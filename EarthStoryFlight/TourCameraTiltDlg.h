#pragma once

#include "TourCameraTiltSettings.h"

class CTourCameraTiltDlg : public CDialogEx
{
public:
	explicit CTourCameraTiltDlg(CWnd* pParent = nullptr);

	int GetSelectedTiltDeg() const { return m_selectedTiltDeg; }
	bool IsKmlRecommendedSelected() const { return m_kmlRecommendedSelected; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual BOOL OnInitDialog() override;

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEnChangeEditTourTilt();
	afx_msg void OnBnClickedDefault();
	afx_msg void OnBnClickedApply();

	DECLARE_MESSAGE_MAP()

private:
	void SyncControlsFromSlider();
	void SyncControlsFromEdit();
	void UpdateCurrentValueLabel();
	int ReadEditTiltDeg(bool& outValid) const;

	CSliderCtrl m_slider;
	CEdit m_editTilt;
	CStatic m_staticCurrent;
	int m_selectedTiltDeg = TourCameraTiltSettings::kDefaultUserTiltDeg;
	bool m_kmlRecommendedSelected = false;
	bool m_updatingControls = false;
};
