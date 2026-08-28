#pragma once

#include "TourCameraRangeSettings.h"

class CTourCameraRangeDlg : public CDialogEx
{
public:
	explicit CTourCameraRangeDlg(CWnd* pParent = nullptr);

	int GetSelectedRangeM() const { return m_selectedRangeM; }
	bool IsKmlRecommendedSelected() const { return m_kmlRecommendedSelected; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX) override;
	virtual BOOL OnInitDialog() override;

	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnEnChangeEditTourRange();
	afx_msg void OnBnClickedDefault();
	afx_msg void OnBnClickedApply();

	DECLARE_MESSAGE_MAP()

private:
	void SyncControlsFromSlider();
	void SyncControlsFromEdit();
	void UpdateCurrentValueLabel();
	int ReadEditRangeM(bool& outValid) const;
	int SliderPosToRangeM(int pos) const;
	int RangeMToSliderPos(int rangeM) const;

	CSliderCtrl m_slider;
	CEdit m_editRange;
	CStatic m_staticCurrent;
	int m_selectedRangeM = TourCameraRangeSettings::kDefaultUserRangeM;
	bool m_kmlRecommendedSelected = false;
	bool m_updatingControls = false;
};
