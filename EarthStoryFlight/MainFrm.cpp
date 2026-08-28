#include "pch.h"
#include "framework.h"
#include "MainFrm.h"
#include "EarthStoryFlightView.h"
#include "GoogleEarthPoC.h"
#include "TourCameraRangeDlg.h"
#include "TourCameraRangeSettings.h"
#include "TourCameraTiltDlg.h"
#include "TourCameraTiltSettings.h"
#include "EsfDebugLog.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_WM_CREATE()
	ON_COMMAND(ID_MAP_OPEN, &CMainFrame::OnMapOpen)
	ON_COMMAND(ID_TOUR_CAMERA_RANGE_KML, &CMainFrame::OnTourCameraRangeKml)
	ON_COMMAND(ID_TOUR_CAMERA_RANGE_1000, &CMainFrame::OnTourCameraRange1000)
	ON_COMMAND(ID_TOUR_CAMERA_RANGE_3000, &CMainFrame::OnTourCameraRange3000)
	ON_COMMAND(ID_TOUR_CAMERA_RANGE_5000, &CMainFrame::OnTourCameraRange5000)
	ON_COMMAND(ID_TOUR_CAMERA_RANGE_10000, &CMainFrame::OnTourCameraRange10000)
	ON_COMMAND(ID_TOUR_CAMERA_RANGE_20000, &CMainFrame::OnTourCameraRange20000)
	ON_COMMAND(ID_TOUR_CAMERA_RANGE_30000, &CMainFrame::OnTourCameraRange30000)
	ON_COMMAND(ID_TOUR_CAMERA_RANGE_CUSTOM, &CMainFrame::OnTourCameraRangeCustom)
	ON_COMMAND(ID_TOUR_CAMERA_TILT_KML, &CMainFrame::OnTourCameraTiltKml)
	ON_COMMAND(ID_TOUR_CAMERA_TILT_30, &CMainFrame::OnTourCameraTilt30)
	ON_COMMAND(ID_TOUR_CAMERA_TILT_45, &CMainFrame::OnTourCameraTilt45)
	ON_COMMAND(ID_TOUR_CAMERA_TILT_60, &CMainFrame::OnTourCameraTilt60)
	ON_COMMAND(ID_TOUR_CAMERA_TILT_70, &CMainFrame::OnTourCameraTilt70)
	ON_COMMAND(ID_TOUR_CAMERA_TILT_75, &CMainFrame::OnTourCameraTilt75)
	ON_COMMAND(ID_TOUR_CAMERA_TILT_CUSTOM, &CMainFrame::OnTourCameraTiltCustom)
	ON_COMMAND(ID_TOUR_GEUMSAN_TEST, &CMainFrame::OnTourGeumsanTest)
	ON_COMMAND(ID_TOUR_GEUMSAN_8SCENERY, &CMainFrame::OnTourGeumsan8Scenery)
	ON_COMMAND(ID_TOUR_GEUMSAN_10SCENIC, &CMainFrame::OnTourGeumsan10Scenic)
	ON_COMMAND(ID_TOUR_EXODUS_MEMPHIS_JERICHO, &CMainFrame::OnTourExodusMemphisToJericho)
END_MESSAGE_MAP()

CMainFrame::CMainFrame() noexcept
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (!CFrameWnd::PreCreateWindow(cs))
		return FALSE;

	cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
		| WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU;

	return TRUE;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (!m_wndStatusBar.Create(this))
		return -1;

	static UINT indicators[] = { ID_SEPARATOR };
	m_wndStatusBar.SetIndicators(indicators, _countof(indicators));

	ApplyKoreanMenuText();
	RefreshTourCameraUi();
	return 0;
}

void CMainFrame::ApplyKoreanMenuText()
{
	CMenu* menu = GetMenu();
	if (!menu)
		return;

	constexpr int kMapMenuIndex = 1;
	constexpr int kTourMenuIndex = 2;

	CMenu* mapMenu = menu->GetSubMenu(kMapMenuIndex);
	if (mapMenu)
	{
		mapMenu->ModifyMenuW(ID_MAP_OPEN, MF_BYCOMMAND | MF_STRING, ID_MAP_OPEN, L"\uC9C0\uB3C4 \uC5F4\uAE30");
	}

	MENUITEMINFOW tourPopupInfo{};
	tourPopupInfo.cbSize = sizeof(tourPopupInfo);
	tourPopupInfo.fMask = MIIM_STRING;
	tourPopupInfo.dwTypeData = const_cast<LPWSTR>(L"\uD22C\uC5B4");
	menu->SetMenuItemInfo(kTourMenuIndex, &tourPopupInfo, TRUE);

	CMenu* tourMenu = menu->GetSubMenu(kTourMenuIndex);
	if (tourMenu)
	{
		MENUITEMINFOW cameraRangeInfo{};
		cameraRangeInfo.cbSize = sizeof(cameraRangeInfo);
		cameraRangeInfo.fMask = MIIM_STRING;
		cameraRangeInfo.dwTypeData = const_cast<LPWSTR>(L"\uCE74\uBA54\uB77C \uAC70\uB9AC");
		tourMenu->SetMenuItemInfo(0, &cameraRangeInfo, TRUE);

		CMenu* rangeMenu = tourMenu->GetSubMenu(0);
		if (rangeMenu)
		{
			rangeMenu->ModifyMenuW(
				ID_TOUR_CAMERA_RANGE_KML,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_RANGE_KML,
				L"KML \uAD8C\uC7A5\uAC12");
			rangeMenu->ModifyMenuW(
				ID_TOUR_CAMERA_RANGE_1000,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_RANGE_1000,
				L"1,000 m");
			rangeMenu->ModifyMenuW(
				ID_TOUR_CAMERA_RANGE_3000,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_RANGE_3000,
				L"3,000 m");
			rangeMenu->ModifyMenuW(
				ID_TOUR_CAMERA_RANGE_5000,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_RANGE_5000,
				L"5,000 m");
			rangeMenu->ModifyMenuW(
				ID_TOUR_CAMERA_RANGE_10000,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_RANGE_10000,
				L"10,000 m");
			rangeMenu->ModifyMenuW(
				ID_TOUR_CAMERA_RANGE_20000,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_RANGE_20000,
				L"20,000 m");
			rangeMenu->ModifyMenuW(
				ID_TOUR_CAMERA_RANGE_30000,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_RANGE_30000,
				L"30,000 m");
			rangeMenu->ModifyMenuW(
				ID_TOUR_CAMERA_RANGE_CUSTOM,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_RANGE_CUSTOM,
				L"\uC0AC\uC6A9\uC790 \uC9C0\uC815...");
		}

		MENUITEMINFOW cameraTiltInfo{};
		cameraTiltInfo.cbSize = sizeof(cameraTiltInfo);
		cameraTiltInfo.fMask = MIIM_STRING;
		cameraTiltInfo.dwTypeData = const_cast<LPWSTR>(L"\uCE74\uBA54\uB77C \uAE30\uC6B8\uAE30");
		tourMenu->SetMenuItemInfo(1, &cameraTiltInfo, TRUE);

		CMenu* tiltMenu = tourMenu->GetSubMenu(1);
		if (tiltMenu)
		{
			tiltMenu->ModifyMenuW(
				ID_TOUR_CAMERA_TILT_KML,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_TILT_KML,
				L"KML \uAD8C\uC7A5\uAC12");
			tiltMenu->ModifyMenuW(
				ID_TOUR_CAMERA_TILT_30,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_TILT_30,
				L"30\u00B0");
			tiltMenu->ModifyMenuW(
				ID_TOUR_CAMERA_TILT_45,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_TILT_45,
				L"45\u00B0");
			tiltMenu->ModifyMenuW(
				ID_TOUR_CAMERA_TILT_60,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_TILT_60,
				L"60\u00B0");
			tiltMenu->ModifyMenuW(
				ID_TOUR_CAMERA_TILT_70,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_TILT_70,
				L"70\u00B0");
			tiltMenu->ModifyMenuW(
				ID_TOUR_CAMERA_TILT_75,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_TILT_75,
				L"75\u00B0");
			tiltMenu->ModifyMenuW(
				ID_TOUR_CAMERA_TILT_CUSTOM,
				MF_BYCOMMAND | MF_STRING,
				ID_TOUR_CAMERA_TILT_CUSTOM,
				L"\uC0AC\uC6A9\uC790 \uC9C0\uC815...");
		}

		tourMenu->ModifyMenuW(
			ID_TOUR_GEUMSAN_TEST,
			MF_BYCOMMAND | MF_STRING,
			ID_TOUR_GEUMSAN_TEST,
			L"\uAE30\uBCF8");
		tourMenu->ModifyMenuW(
			ID_TOUR_GEUMSAN_8SCENERY,
			MF_BYCOMMAND | MF_STRING,
			ID_TOUR_GEUMSAN_8SCENERY,
			L"\uAE08\uC0B0\uD314\uACBD \uC0C1\uACF5 \uC120\uD68C \uBE44\uD589");
		tourMenu->ModifyMenuW(
			ID_TOUR_GEUMSAN_10SCENIC,
			MF_BYCOMMAND | MF_STRING,
			ID_TOUR_GEUMSAN_10SCENIC,
			L"\uAE08\uC0B0 10\uACBD");
		tourMenu->ModifyMenuW(
			ID_TOUR_EXODUS_MEMPHIS_JERICHO,
			MF_BYCOMMAND | MF_STRING,
			ID_TOUR_EXODUS_MEMPHIS_JERICHO,
			L"\uCD9C\uC560\uAD70 \uACBD\uB85C");
	}

	DrawMenuBar();
	UpdateCameraRangeMenuChecks();
	UpdateCameraTiltMenuChecks();
}

void CMainFrame::UpdateCameraRangeMenuChecks()
{
	CMenu* menu = GetMenu();
	if (!menu)
		return;

	CMenu* tourMenu = menu->GetSubMenu(2);
	if (!tourMenu)
		return;

	CMenu* rangeMenu = tourMenu->GetSubMenu(0);
	if (!rangeMenu)
		return;

	const TourCameraRangeSettings& settings = TourCameraRangeSettings::Instance();
	const bool kmlRecommended = settings.GetMode() == TourCameraRangeMode::KmlRecommended;
	const int presetRange = settings.GetActivePresetRangeM();
	const bool customSelected =
		settings.GetMode() == TourCameraRangeMode::UserSpecified && presetRange == 0;

	const struct
	{
		UINT commandId;
		bool checked;
	} items[] = {
		{ ID_TOUR_CAMERA_RANGE_KML, kmlRecommended },
		{ ID_TOUR_CAMERA_RANGE_1000, presetRange == 1000 },
		{ ID_TOUR_CAMERA_RANGE_3000, presetRange == 3000 },
		{ ID_TOUR_CAMERA_RANGE_5000, presetRange == 5000 },
		{ ID_TOUR_CAMERA_RANGE_10000, presetRange == 10000 },
		{ ID_TOUR_CAMERA_RANGE_20000, presetRange == 20000 },
		{ ID_TOUR_CAMERA_RANGE_30000, presetRange == 30000 },
		{ ID_TOUR_CAMERA_RANGE_CUSTOM, customSelected },
	};

	for (const auto& item : items)
	{
		rangeMenu->CheckMenuItem(
			item.commandId,
			MF_BYCOMMAND | (item.checked ? MF_CHECKED : MF_UNCHECKED));
	}
}

void CMainFrame::UpdateCameraTiltMenuChecks()
{
	CMenu* menu = GetMenu();
	if (!menu)
		return;

	CMenu* tourMenu = menu->GetSubMenu(2);
	if (!tourMenu)
		return;

	CMenu* tiltMenu = tourMenu->GetSubMenu(1);
	if (!tiltMenu)
		return;

	const TourCameraTiltSettings& settings = TourCameraTiltSettings::Instance();
	const bool kmlRecommended = settings.GetMode() == TourCameraTiltMode::KmlRecommended;
	const int presetTilt = settings.GetActivePresetTiltDeg();
	const bool customSelected =
		settings.GetMode() == TourCameraTiltMode::UserSpecified && presetTilt == 0;

	const struct
	{
		UINT commandId;
		bool checked;
	} items[] = {
		{ ID_TOUR_CAMERA_TILT_KML, kmlRecommended },
		{ ID_TOUR_CAMERA_TILT_30, presetTilt == 30 },
		{ ID_TOUR_CAMERA_TILT_45, presetTilt == 45 },
		{ ID_TOUR_CAMERA_TILT_60, presetTilt == 60 },
		{ ID_TOUR_CAMERA_TILT_70, presetTilt == 70 },
		{ ID_TOUR_CAMERA_TILT_75, presetTilt == 75 },
		{ ID_TOUR_CAMERA_TILT_CUSTOM, customSelected },
	};

	for (const auto& item : items)
	{
		tiltMenu->CheckMenuItem(
			item.commandId,
			MF_BYCOMMAND | (item.checked ? MF_CHECKED : MF_UNCHECKED));
	}
}

void CMainFrame::UpdateTourCameraStatus()
{
	const CString statusText =
		TourCameraRangeSettings::Instance().FormatStatusText() +
		L" | " +
		TourCameraTiltSettings::Instance().FormatStatusText();
	m_wndStatusBar.SetPaneText(0, statusText);
	EsfDebugLog::Log(statusText);
}

void CMainFrame::RefreshTourCameraUi()
{
	UpdateCameraRangeMenuChecks();
	UpdateCameraTiltMenuChecks();
	UpdateTourCameraStatus();
}

void CMainFrame::SetCameraRangePreset(int rangeM)
{
	TourCameraRangeSettings::Instance().SetUserRangeM(rangeM);
	RefreshTourCameraUi();
}

void CMainFrame::SetCameraTiltPreset(int tiltDeg)
{
	TourCameraTiltSettings::Instance().SetUserTiltDeg(tiltDeg);
	RefreshTourCameraUi();
}

CEarthStoryFlightView* CMainFrame::GetActiveEarthView() const
{
	return DYNAMIC_DOWNCAST(CEarthStoryFlightView, GetActiveView());
}

void CMainFrame::OnMapOpen()
{
	if (CEarthStoryFlightView* view = GetActiveEarthView())
		view->OpenMap();
}

void CMainFrame::OnTourCameraRangeKml()
{
	TourCameraRangeSettings::Instance().SetKmlRecommended();
	RefreshTourCameraUi();
}

void CMainFrame::OnTourCameraRange1000()
{
	SetCameraRangePreset(1000);
}

void CMainFrame::OnTourCameraRange3000()
{
	SetCameraRangePreset(3000);
}

void CMainFrame::OnTourCameraRange5000()
{
	SetCameraRangePreset(5000);
}

void CMainFrame::OnTourCameraRange10000()
{
	SetCameraRangePreset(10000);
}

void CMainFrame::OnTourCameraRange20000()
{
	SetCameraRangePreset(20000);
}

void CMainFrame::OnTourCameraRange30000()
{
	SetCameraRangePreset(30000);
}

void CMainFrame::OnTourCameraRangeCustom()
{
	CTourCameraRangeDlg dlg(this);
	if (dlg.DoModal() != IDOK)
		return;

	if (dlg.IsKmlRecommendedSelected())
		TourCameraRangeSettings::Instance().SetKmlRecommended();
	else
		TourCameraRangeSettings::Instance().SetUserRangeM(dlg.GetSelectedRangeM());

	RefreshTourCameraUi();
}

void CMainFrame::OnTourCameraTiltKml()
{
	TourCameraTiltSettings::Instance().SetKmlRecommended();
	RefreshTourCameraUi();
}

void CMainFrame::OnTourCameraTilt30()
{
	SetCameraTiltPreset(30);
}

void CMainFrame::OnTourCameraTilt45()
{
	SetCameraTiltPreset(45);
}

void CMainFrame::OnTourCameraTilt60()
{
	SetCameraTiltPreset(60);
}

void CMainFrame::OnTourCameraTilt70()
{
	SetCameraTiltPreset(70);
}

void CMainFrame::OnTourCameraTilt75()
{
	SetCameraTiltPreset(75);
}

void CMainFrame::OnTourCameraTiltCustom()
{
	CTourCameraTiltDlg dlg(this);
	if (dlg.DoModal() != IDOK)
		return;

	if (dlg.IsKmlRecommendedSelected())
		TourCameraTiltSettings::Instance().SetKmlRecommended();
	else
		TourCameraTiltSettings::Instance().SetUserTiltDeg(dlg.GetSelectedTiltDeg());

	RefreshTourCameraUi();
}

void CMainFrame::OnTourGeumsanTest()
{
	if (!GoogleEarthPoC::Run())
		AfxMessageBox(L"Failed to launch Google Earth tour.", MB_ICONERROR);
}

void CMainFrame::OnTourGeumsan8Scenery()
{
	CString errorMessage;
	if (!GoogleEarthPoC::RunGeumsan8SceneryTour(errorMessage))
		AfxMessageBox(errorMessage, MB_ICONERROR);
}

void CMainFrame::OnTourGeumsan10Scenic()
{
	CString errorMessage;
	if (!GoogleEarthPoC::RunGeumsan10ScenicTour(errorMessage))
		AfxMessageBox(errorMessage, MB_ICONERROR);
}

void CMainFrame::OnTourExodusMemphisToJericho()
{
	CString errorMessage;
	if (!GoogleEarthPoC::RunExodusMemphisToJerichoTour(errorMessage))
		AfxMessageBox(errorMessage, MB_ICONERROR);
}
