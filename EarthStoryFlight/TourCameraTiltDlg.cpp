#include "pch.h"
#include "TourCameraTiltDlg.h"
#include "TourCameraTiltSettings.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CTourCameraTiltDlg::CTourCameraTiltDlg(CWnd* pParent)
	: CDialogEx(IDD_TOUR_CAMERA_TILT, pParent)
{
}

void CTourCameraTiltDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_SLIDER_TOUR_TILT, m_slider);
	DDX_Control(pDX, IDC_EDIT_TOUR_TILT, m_editTilt);
	DDX_Control(pDX, IDC_STATIC_TOUR_TILT_CURRENT, m_staticCurrent);
}

BEGIN_MESSAGE_MAP(CTourCameraTiltDlg, CDialogEx)
	ON_WM_HSCROLL()
	ON_EN_CHANGE(IDC_EDIT_TOUR_TILT, &CTourCameraTiltDlg::OnEnChangeEditTourTilt)
	ON_BN_CLICKED(IDC_BTN_TOUR_TILT_DEFAULT, &CTourCameraTiltDlg::OnBnClickedDefault)
	ON_BN_CLICKED(IDC_BTN_TOUR_TILT_APPLY, &CTourCameraTiltDlg::OnBnClickedApply)
END_MESSAGE_MAP()

BOOL CTourCameraTiltDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetWindowTextW(L"\uCE74\uBA54\uB77C \uAE30\uC6B8\uAE30 \uC124\uC815");
	SetDlgItemTextW(IDC_STATIC_TOUR_TILT_MIN, L"0\u00B0");
	SetDlgItemTextW(IDC_STATIC_TOUR_TILT_MAX, L"80\u00B0");
	SetDlgItemTextW(IDC_STATIC_TOUR_TILT_LABEL, L"\uCE74\uBA54\uB77C \uAE30\uC6B8\uAE30:");
	SetDlgItemTextW(IDC_STATIC_TOUR_TILT_UNIT, L"\u00B0");
	SetDlgItemTextW(IDC_BTN_TOUR_TILT_DEFAULT, L"\uAE30\uBCF8\uAC12");
	SetDlgItemTextW(IDC_BTN_TOUR_TILT_APPLY, L"\uC801\uC6A9");
	SetDlgItemTextW(IDCANCEL, L"\uCDE8\uC18C");

	const TourCameraTiltSettings& settings = TourCameraTiltSettings::Instance();
	m_selectedTiltDeg = (settings.GetMode() == TourCameraTiltMode::UserSpecified)
		? settings.GetUserTiltDeg()
		: TourCameraTiltSettings::kDefaultUserTiltDeg;
	m_kmlRecommendedSelected = false;

	m_slider.SetRange(TourCameraTiltSettings::kMinTiltDeg, TourCameraTiltSettings::kMaxTiltDeg, TRUE);
	m_slider.SetPos(m_selectedTiltDeg);
	m_slider.SetTicFreq(10);

	CString editText;
	editText.Format(L"%d", m_selectedTiltDeg);
	m_editTilt.SetWindowTextW(editText);
	UpdateCurrentValueLabel();

	return TRUE;
}

void CTourCameraTiltDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	if (pScrollBar == reinterpret_cast<CScrollBar*>(&m_slider))
		SyncControlsFromSlider();

	CDialogEx::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CTourCameraTiltDlg::OnEnChangeEditTourTilt()
{
	if (m_updatingControls)
		return;

	SyncControlsFromEdit();
}

void CTourCameraTiltDlg::OnBnClickedDefault()
{
	m_kmlRecommendedSelected = true;
	EndDialog(IDOK);
}

void CTourCameraTiltDlg::OnBnClickedApply()
{
	bool valid = false;
	const int tiltDeg = ReadEditTiltDeg(valid);
	if (!valid || tiltDeg < TourCameraTiltSettings::kMinTiltDeg || tiltDeg > TourCameraTiltSettings::kMaxTiltDeg)
	{
		AfxMessageBox(
			L"\uCE74\uBA54\uB77C \uAE30\uC6B8\uAE30\uB294 0\u00B0\uC5D0\uC11C 80\u00B0 \uC0AC\uC774\uB85C \uC785\uB825\uD574 \uC8FC\uC138\uC694.",
			MB_ICONWARNING);
		return;
	}

	m_selectedTiltDeg = tiltDeg;
	m_kmlRecommendedSelected = false;
	EndDialog(IDOK);
}

void CTourCameraTiltDlg::SyncControlsFromSlider()
{
	m_updatingControls = true;
	m_selectedTiltDeg = m_slider.GetPos();

	CString editText;
	editText.Format(L"%d", m_selectedTiltDeg);
	m_editTilt.SetWindowTextW(editText);
	UpdateCurrentValueLabel();
	m_updatingControls = false;
}

void CTourCameraTiltDlg::SyncControlsFromEdit()
{
	bool valid = false;
	const int tiltDeg = ReadEditTiltDeg(valid);
	if (!valid)
	{
		UpdateCurrentValueLabel();
		return;
	}

	m_selectedTiltDeg = tiltDeg;
	m_updatingControls = true;
	m_slider.SetPos(tiltDeg);
	UpdateCurrentValueLabel();
	m_updatingControls = false;
}

void CTourCameraTiltDlg::UpdateCurrentValueLabel()
{
	CString label;
	label.Format(
		L"\uD604\uC7AC \uAC12: %s",
		TourCameraTiltSettings::Instance().FormatTiltDeg(m_selectedTiltDeg).GetString());
	m_staticCurrent.SetWindowTextW(label);
}

int CTourCameraTiltDlg::ReadEditTiltDeg(bool& outValid) const
{
	outValid = false;

	CString text;
	m_editTilt.GetWindowTextW(text);
	text.Trim();

	if (text.IsEmpty())
		return m_selectedTiltDeg;

	for (int index = 0; index < text.GetLength(); ++index)
	{
		const TCHAR ch = text[index];
		if (ch < L'0' || ch > L'9')
			return m_selectedTiltDeg;
	}

	outValid = true;
	return _ttoi(text);
}
