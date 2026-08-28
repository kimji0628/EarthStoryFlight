#include "pch.h"
#include "TourCameraRangeDlg.h"
#include "TourCameraRangeSettings.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CTourCameraRangeDlg::CTourCameraRangeDlg(CWnd* pParent)
	: CDialogEx(IDD_TOUR_CAMERA_RANGE, pParent)
{
}

void CTourCameraRangeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_TOUR_RANGE, m_slider);
	DDX_Control(pDX, IDC_EDIT_TOUR_RANGE, m_editRange);
	DDX_Control(pDX, IDC_STATIC_TOUR_RANGE_CURRENT, m_staticCurrent);
}

BEGIN_MESSAGE_MAP(CTourCameraRangeDlg, CDialogEx)
	ON_WM_HSCROLL()
	ON_EN_CHANGE(IDC_EDIT_TOUR_RANGE, &CTourCameraRangeDlg::OnEnChangeEditTourRange)
	ON_BN_CLICKED(IDC_BTN_TOUR_RANGE_DEFAULT, &CTourCameraRangeDlg::OnBnClickedDefault)
	ON_BN_CLICKED(IDC_BTN_TOUR_RANGE_APPLY, &CTourCameraRangeDlg::OnBnClickedApply)
END_MESSAGE_MAP()

BOOL CTourCameraRangeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowTextW(L"\uCE74\uBA54\uB77C \uAC70\uB9AC \uC124\uC815");
	SetDlgItemTextW(IDC_STATIC_TOUR_RANGE_MIN, L"1,000 m");
	SetDlgItemTextW(IDC_STATIC_TOUR_RANGE_MAX, L"30,000 m");
	SetDlgItemTextW(IDC_STATIC_TOUR_RANGE_LABEL, L"\uCE74\uBA54\uB77C \uAC70\uB9AC:");
	SetDlgItemTextW(IDC_STATIC_TOUR_RANGE_UNIT, L"m");
	SetDlgItemTextW(IDC_BTN_TOUR_RANGE_DEFAULT, L"\uAE30\uBCF8\uAC12");
	SetDlgItemTextW(IDC_BTN_TOUR_RANGE_APPLY, L"\uC801\uC6A9");
	SetDlgItemTextW(IDCANCEL, L"\uCDE8\uC18C");

	const TourCameraRangeSettings& settings = TourCameraRangeSettings::Instance();
	m_selectedRangeM = (settings.GetMode() == TourCameraRangeMode::UserSpecified)
		? settings.GetUserRangeM()
		: TourCameraRangeSettings::kDefaultUserRangeM;
	m_kmlRecommendedSelected = false;

	m_slider.SetRange(1, 30, TRUE);
	m_slider.SetPos(RangeMToSliderPos(m_selectedRangeM));
	m_slider.SetTicFreq(1);

	CString editText;
	editText.Format(L"%d", m_selectedRangeM);
	m_editRange.SetWindowTextW(editText);
	UpdateCurrentValueLabel();

	return TRUE;
}

void CTourCameraRangeDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == reinterpret_cast<CScrollBar*>(&m_slider))
		SyncControlsFromSlider();

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CTourCameraRangeDlg::OnEnChangeEditTourRange()
{
	if (m_updatingControls)
		return;

	SyncControlsFromEdit();
}

void CTourCameraRangeDlg::OnBnClickedDefault()
{
	m_kmlRecommendedSelected = true;
	EndDialog(IDOK);
}

void CTourCameraRangeDlg::OnBnClickedApply()
{
	bool valid = false;
	const int rangeM = ReadEditRangeM(valid);
	if (!valid || rangeM < TourCameraRangeSettings::kMinRangeM || rangeM > TourCameraRangeSettings::kMaxRangeM)
	{
		AfxMessageBox(
			L"\uCE74\uBA54\uB77C \uAC70\uB9AC\uB294 1,000m\uC5D0\uC11C 30,000m \uC0AC\uC774\uB85C \uC785\uB825\uD574 \uC8FC\uC138\uC694.",
			MB_ICONWARNING);
		return;
	}

	m_selectedRangeM = rangeM;
	m_kmlRecommendedSelected = false;
	EndDialog(IDOK);
}

void CTourCameraRangeDlg::SyncControlsFromSlider()
{
	m_updatingControls = true;
	m_selectedRangeM = SliderPosToRangeM(m_slider.GetPos());

	CString editText;
	editText.Format(L"%d", m_selectedRangeM);
	m_editRange.SetWindowTextW(editText);
	UpdateCurrentValueLabel();
	m_updatingControls = false;
}

void CTourCameraRangeDlg::SyncControlsFromEdit()
{
	bool valid = false;
	const int rangeM = ReadEditRangeM(valid);
	if (!valid)
	{
		UpdateCurrentValueLabel();
		return;
	}

	m_selectedRangeM = rangeM;
	m_updatingControls = true;
	m_slider.SetPos(RangeMToSliderPos(rangeM));
	UpdateCurrentValueLabel();
	m_updatingControls = false;
}

void CTourCameraRangeDlg::UpdateCurrentValueLabel()
{
	CString label;
	label.Format(
		L"\uD604\uC7AC \uAC12: %s",
		TourCameraRangeSettings::Instance().FormatRangeM(m_selectedRangeM).GetString());
	m_staticCurrent.SetWindowTextW(label);
}

int CTourCameraRangeDlg::ReadEditRangeM(bool& outValid) const
{
	outValid = false;

	CString text;
	m_editRange.GetWindowTextW(text);
	text.Trim();

	if (text.IsEmpty())
		return m_selectedRangeM;

	for (int index = 0; index < text.GetLength(); ++index)
	{
		const TCHAR ch = text[index];
		if (ch < L'0' || ch > L'9')
			return m_selectedRangeM;
	}

	outValid = true;
	return _ttoi(text);
}

int CTourCameraRangeDlg::SliderPosToRangeM(int pos) const
{
	if (pos < 1)
		pos = 1;
	if (pos > 30)
		pos = 30;
	return pos * 1000;
}

int CTourCameraRangeDlg::RangeMToSliderPos(int rangeM) const
{
	if (rangeM < TourCameraRangeSettings::kMinRangeM)
		rangeM = TourCameraRangeSettings::kMinRangeM;
	if (rangeM > TourCameraRangeSettings::kMaxRangeM)
		rangeM = TourCameraRangeSettings::kMaxRangeM;

	const int pos = rangeM / 1000;
	return (pos < 1) ? 1 : pos;
}
