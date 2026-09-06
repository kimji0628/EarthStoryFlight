#include "pch.h"
#include "framework.h"
#include "EarthStoryFlightDoc.h"
#include "EarthStoryFlightView.h"
#include "MapConfig.h"
#include "MapLibraryDlg.h"
#include "MapLibraryService.h"
#include "MainFrm.h"
#include "EsfDebugLog.h"
#include "EsfPaths.h"
#include "GoogleEarthPoC.h"
#include "resource.h"
#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
	void AppendWpEventLog(const CString& line)
	{
		const CString dir = EsfPaths::GetOutputRoot() + L"\\Export";
		if (!EsfPaths::EnsureDirectoryExists(dir))
			return;

		const CString path = dir + L"\\wp_event_debug.txt";
		CFile file;
		if (!file.Open(path, CFile::modeCreate | CFile::modeNoTruncate | CFile::modeWrite | CFile::shareDenyWrite))
			return;

		file.SeekToEnd();
		CString stamped = line + L"\r\n";
		const int utf8Length = ::WideCharToMultiByte(
			CP_UTF8, 0, stamped, stamped.GetLength(), nullptr, 0, nullptr, nullptr);
		if (utf8Length <= 0)
		{
			file.Close();
			return;
		}

		CStringA utf8;
		LPSTR buffer = utf8.GetBuffer(utf8Length);
		::WideCharToMultiByte(
			CP_UTF8, 0, stamped, stamped.GetLength(), buffer, utf8Length, nullptr, nullptr);
		utf8.ReleaseBuffer(utf8Length);
		file.Write(utf8.GetString(), static_cast<UINT>(utf8.GetLength()));
		file.Close();
	}
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CEarthStoryFlightView, CView)

BEGIN_MESSAGE_MAP(CEarthStoryFlightView, CView)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DESTROY()
	ON_BN_CLICKED(IDC_BTN_MAP_SELECT, &CEarthStoryFlightView::OnMapEditorSelect)
	ON_BN_CLICKED(IDC_BTN_MAP_SAVE, &CEarthStoryFlightView::OnMapEditorSave)
	ON_BN_CLICKED(IDC_BTN_MAP_BACK, &CEarthStoryFlightView::OnMapEditorBack)
END_MESSAGE_MAP()

CEarthStoryFlightView::CEarthStoryFlightView() noexcept
{
}

BOOL CEarthStoryFlightView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

int CEarthStoryFlightView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rect;
	GetClientRect(&rect);
	m_mapHost.SetMessageCallback([this](const CString& message) { HandleMapMessage(message); });
	m_mapHost.Create(m_hWnd, rect);
	m_mapHost.Show(SW_HIDE);
	return 0;
}

CRect CEarthStoryFlightView::GetMapHostRect() const
{
	CRect rect;
	GetClientRect(&rect);
	if (m_newMapEditorMode)
		rect.top += kMapToolbarHeight;
	return rect;
}

void CEarthStoryFlightView::OnSize(UINT nType, int cx, int cy)
{
	CView::OnSize(nType, cx, cy);
	LayoutMapEditorToolbar();
	m_mapHost.Resize(GetMapHostRect());
}

void CEarthStoryFlightView::OnDestroy()
{
	DestroyMapEditorToolbar();
	m_mapHost.Show(SW_HIDE);
	CView::OnDestroy();
}

void CEarthStoryFlightView::OnDraw(CDC* pDC)
{
	CEarthStoryFlightDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	if (m_mapOpened || m_newMapEditorMode)
		return;

	CRect rect;
	GetClientRect(&rect);
	pDC->FillSolidRect(rect, RGB(255, 255, 255));

	if (!m_hasSelectedSavedMap || m_workMapImage.IsNull())
		return;

	const int imageWidth = m_workMapImage.GetWidth();
	const int imageHeight = m_workMapImage.GetHeight();
	if (imageWidth <= 0 || imageHeight <= 0)
		return;

	const double scale = (std::min)(
		static_cast<double>(rect.Width()) / static_cast<double>(imageWidth),
		static_cast<double>(rect.Height()) / static_cast<double>(imageHeight));
	const int drawWidth = static_cast<int>(std::lround(imageWidth * scale));
	const int drawHeight = static_cast<int>(std::lround(imageHeight * scale));
	const int offsetX = rect.left + ((rect.Width() - drawWidth) / 2);
	const int offsetY = rect.top + ((rect.Height() - drawHeight) / 2);

	m_workMapImage.StretchBlt(
		pDC->GetSafeHdc(),
		offsetX,
		offsetY,
		drawWidth,
		drawHeight,
		SRCCOPY);
}

double CEarthStoryFlightView::ParseJsonNumber(const CString& json, const CString& key)
{
	const CString token = L"\"" + key + L"\"";
	const int keyPos = json.Find(token);
	if (keyPos < 0)
		return 0.0;

	const int colonPos = json.Find(L':', keyPos + token.GetLength());
	if (colonPos < 0)
		return 0.0;

	int endPos = colonPos + 1;
	while (endPos < json.GetLength() && (json[endPos] == L' ' || json[endPos] == L'\t'))
		++endPos;

	int scanPos = endPos;
	while (scanPos < json.GetLength())
	{
		const wchar_t ch = json[scanPos];
		if ((ch >= L'0' && ch <= L'9') || ch == L'.' || ch == L'-' || ch == L'+')
		{
			++scanPos;
			continue;
		}
		break;
	}

	return _wtof(json.Mid(endPos, scanPos - endPos));
}

CString CEarthStoryFlightView::ParseJsonString(const CString& json, const CString& key)
{
	const CString token = L"\"" + key + L"\"";
	const int keyPos = json.Find(token);
	if (keyPos < 0)
		return CString();

	const int colonPos = json.Find(L':', keyPos + token.GetLength());
	if (colonPos < 0)
		return CString();

	int valuePos = colonPos + 1;
	while (valuePos < json.GetLength())
	{
		const wchar_t ch = json[valuePos];
		if (ch == L' ' || ch == L'\t' || ch == L'\r' || ch == L'\n')
		{
			++valuePos;
			continue;
		}
		break;
	}

	if (valuePos >= json.GetLength() || json[valuePos] != L'"')
		return CString();

	const int firstQuote = valuePos;
	const int secondQuote = json.Find(L'"', firstQuote + 1);
	if (secondQuote < 0)
		return CString();

	return json.Mid(firstQuote + 1, secondQuote - firstQuote - 1);
}

void CEarthStoryFlightView::UpdateMapCursorStatus()
{
	auto* frame = DYNAMIC_DOWNCAST(CMainFrame, GetParentFrame());
	if (!frame)
		return;

	if (m_hasCursor)
	{
		CString text;
		text.Format(L"\uC704\uB3C4 %.6f\u00B0 / \uACBD\uB3C4 %.6f\u00B0", m_cursorLat, m_cursorLng);
		frame->SetMapCursorStatus(text);
		return;
	}

	if (m_bounds.valid)
	{
		CString text;
		text.Format(
			L"\uBD81 %.6f\u00B0 / \uB0A8 %.6f\u00B0 / \uC11C %.6f\u00B0 / \uB3D9 %.6f\u00B0",
			m_bounds.north,
			m_bounds.south,
			m_bounds.west,
			m_bounds.east);
		frame->SetMapCursorStatus(text);
		return;
	}

	if (m_mapOpened || m_newMapEditorMode)
		frame->SetMapCursorStatus(L"\uC88C\uD45C \uC5C6\uC74C");
	else
		frame->SetMapCursorStatus(CString());
}

bool CEarthStoryFlightView::EnsureApiKey(CString& outMessage)
{
	if (!m_apiKey.IsEmpty())
		return true;

	if (!MapConfig::LoadGoogleMapsApiKey(m_apiKey))
	{
		const CString configPath = MapConfig::GetConfigFilePath();
		if (::GetFileAttributes(configPath) == INVALID_FILE_ATTRIBUTES)
		{
			outMessage.Format(
				L"Config file not found:\r\n%s\r\nCopy maps.json.example from the same Config folder and set your API key.",
				configPath.GetString());
		}
		else
		{
			outMessage.Format(
				L"googleMapsApiKey is missing or invalid in:\r\n%s",
				configPath.GetString());
		}
		return false;
	}

	return true;
}

bool CEarthStoryFlightView::PromptMapName(CString& outMapName)
{
	class CMapNameDlg : public CDialogEx
	{
	public:
		CString m_mapName;

		CMapNameDlg(CWnd* pParent, const CString& initialName)
			: CDialogEx(IDD_MAPNAME, pParent)
			, m_mapName(initialName)
		{
		}

	protected:
		BOOL OnInitDialog() override
		{
			CDialogEx::OnInitDialog();
			SetWindowTextW(L"\uC9C0\uB3C4 \uC774\uB984");
			SetDlgItemTextW(IDC_STATIC, L"\uC9C0\uB3C4 \uC774\uB984 \uC785\uB825:");
			SetDlgItemTextW(IDOK, L"\uD655\uC778");
			SetDlgItemTextW(IDCANCEL, L"\uCDE8\uC18C");
			SetDlgItemTextW(IDC_EDIT_MAPNAME, m_mapName);
			return TRUE;
		}

		void OnOK() override
		{
			GetDlgItemTextW(IDC_EDIT_MAPNAME, m_mapName);
			m_mapName.Trim();
			CDialogEx::OnOK();
		}
	};

	CMapNameDlg dlg(this, outMapName);
	const INT_PTR nameResult = dlg.DoModal();
	m_mapHost.PostCommandJson(L"{\"cmd\":\"logMapType\",\"event\":\"save:afterDialog\"}");
	if (nameResult != IDOK)
		return false;

	outMapName = dlg.m_mapName;
	outMapName.Trim();
	return true;
}

bool CEarthStoryFlightView::PromptRouteName(CString& outRouteName)
{
	class CRouteNameDlg : public CDialogEx
	{
	public:
		CString m_routeName;

		CRouteNameDlg(CWnd* pParent, const CString& initialName)
			: CDialogEx(IDD_MAPNAME, pParent)
			, m_routeName(initialName)
		{
		}

	protected:
		BOOL OnInitDialog() override
		{
			CDialogEx::OnInitDialog();
			SetWindowTextW(L"\uACBD\uB85C \uC774\uB984");
			SetDlgItemTextW(IDC_STATIC, L"\uACBD\uB85C \uC774\uB984 \uC785\uB825:");
			SetDlgItemTextW(IDOK, L"\uD655\uC778");
			SetDlgItemTextW(IDCANCEL, L"\uCDE8\uC18C");
			SetDlgItemTextW(IDC_EDIT_MAPNAME, m_routeName);
			return TRUE;
		}

		void OnOK() override
		{
			GetDlgItemTextW(IDC_EDIT_MAPNAME, m_routeName);
			m_routeName.Trim();
			CDialogEx::OnOK();
		}
	};

	CRouteNameDlg dlg(this, outRouteName);
	if (dlg.DoModal() != IDOK)
		return false;

	outRouteName = dlg.m_routeName;
	outRouteName.Trim();
	return !outRouteName.IsEmpty();
}

CString CEarthStoryFlightView::MakeUniqueFolderName(const CString& baseName) const
{
	CString folder = baseName;
	if (folder.IsEmpty())
		folder = L"map";

	CString candidate = folder;
	int suffix = 1;
	while (::GetFileAttributes(MapRegionService::GetGeoBoundsMapDirectory(candidate)) != INVALID_FILE_ATTRIBUTES)
	{
		candidate.Format(L"%s_%d", folder.GetString(), suffix);
		++suffix;
	}

	return candidate;
}

void CEarthStoryFlightView::HandleMapMessage(const CString& message)
{
	if (message.Find(L"\"type\":\"bounds\"") >= 0)
	{
		m_bounds.north = ParseJsonNumber(message, L"north");
		m_bounds.south = ParseJsonNumber(message, L"south");
		m_bounds.west = ParseJsonNumber(message, L"west");
		m_bounds.east = ParseJsonNumber(message, L"east");
		m_bounds.valid = m_bounds.north > m_bounds.south && m_bounds.east > m_bounds.west;
		UpdateMapEditorToolbar();
		UpdateMapCursorStatus();
	}
	else if (message.Find(L"\"type\":\"cursorClear\"") >= 0)
	{
		m_hasCursor = false;
		UpdateMapCursorStatus();
	}
	else if (message.Find(L"\"type\":\"cursor\"") >= 0)
	{
		m_cursorLat = ParseJsonNumber(message, L"lat");
		m_cursorLng = ParseJsonNumber(message, L"lng");
		m_hasCursor = true;
		UpdateMapCursorStatus();
	}
	else if (message.Find(L"\"type\":\"mapTypeLog\"") >= 0)
	{
		CString log;
		log.Format(
			L"[MapType] %s before=%s after=%s active=%s",
			ParseJsonString(message, L"event").GetString(),
			ParseJsonString(message, L"before").GetString(),
			ParseJsonString(message, L"after").GetString(),
			ParseJsonString(message, L"active").GetString());
		EsfDebugLog::Log(log);
	}
	else if (message.Find(L"\"type\":\"mapType\"") >= 0)
	{
		m_currentMapTypeId = MapConfig::NormalizeMapTypeId(ParseJsonString(message, L"mapType"));
		MapConfig::SaveMapTypeId(m_currentMapTypeId);
	}
	else if (message.Find(L"\"type\":\"ready\"") >= 0)
	{
		const CString reportedType = ParseJsonString(message, L"mapType");
		if (!reportedType.IsEmpty())
			m_currentMapTypeId = MapConfig::NormalizeMapTypeId(reportedType);
		else if (m_currentMapTypeId.IsEmpty())
			m_currentMapTypeId = MapConfig::LoadMapTypeId();
		UpdateMapCursorStatus();
	}
	else if (message.Find(L"\"type\":\"wpLog\"") >= 0)
	{
		CString log;
		log.Format(
			L"[WP] event=%s mode=%s hasSelected=%d selectedIndex=%d count=%d lat=%.7f lng=%.7f alt=%.2f reason=%s",
			ParseJsonString(message, L"event").GetString(),
			ParseJsonString(message, L"mode").GetString(),
			static_cast<int>(ParseJsonNumber(message, L"hasSelected")),
			static_cast<int>(ParseJsonNumber(message, L"selectedIndex")),
			static_cast<int>(ParseJsonNumber(message, L"count")),
			ParseJsonNumber(message, L"lat"),
			ParseJsonNumber(message, L"lng"),
			ParseJsonNumber(message, L"alt"),
			ParseJsonString(message, L"reason").GetString());
		EsfDebugLog::Log(log);
		AppendWpEventLog(log);
	}
	else if (message.Find(L"\"type\":\"waypointSelected\"") >= 0)
	{
		m_cursorLat = ParseJsonNumber(message, L"lat");
		m_cursorLng = ParseJsonNumber(message, L"lng");
		m_hasCursor = true;
		UpdateMapCursorStatus();
		CString log;
		log.Format(
			L"[WP] selected index=%d lat=%.7f lng=%.7f",
			static_cast<int>(ParseJsonNumber(message, L"index")),
			m_cursorLat,
			m_cursorLng);
		EsfDebugLog::Log(log);
		AppendWpEventLog(log);
	}
	else if (message.Find(L"\"type\":\"waypointsChanged\"") >= 0)
	{
		ApplyWaypointsFromMessage(message);
	}
	else if (message.Find(L"\"type\":\"saveRoute\"") >= 0)
	{
		ApplyWaypointsFromMessage(message);
		SaveCurrentRoute(true);
	}
	else if (message.Find(L"\"type\":\"routeLoad\"") >= 0)
	{
		LoadRouteById(ParseJsonString(message, L"id"));
	}
	else if (message.Find(L"\"type\":\"routeRename\"") >= 0)
	{
		const CString routeId = ParseJsonString(message, L"id");
		if (!routeId.IsEmpty())
			m_selectedRouteId = routeId;
		RenameSelectedRoute();
	}
	else if (message.Find(L"\"type\":\"routeDelete\"") >= 0)
	{
		const CString routeId = ParseJsonString(message, L"id");
		if (!routeId.IsEmpty())
			m_selectedRouteId = routeId;
		DeleteSelectedRoute();
	}
	else if (message.Find(L"\"type\":\"routeFly\"") >= 0)
	{
		const CString routeId = ParseJsonString(message, L"id");
		if (!routeId.IsEmpty())
			m_selectedRouteId = routeId;
		FlySelectedRoute(ParseJsonString(message, L"speedText"));
	}
	else if (message.Find(L"\"type\":\"reset\"") >= 0)
	{
		m_bounds = MapRegionBounds();
		UpdateMapEditorToolbar();
		UpdateMapCursorStatus();
	}
	else if (message.Find(L"\"type\":\"error\"") >= 0)
	{
		if (message.Find(L"\"code\":\"init_failed\"") >= 0 ||
			message.Find(L"\"code\":\"gm_authFailure\"") >= 0)
		{
			AfxMessageBox(L"Map initialization failed.\r\n" + message, MB_ICONERROR);
		}
	}
}

void CEarthStoryFlightView::OpenMap()
{
	ShowMapLibrary();
}

void CEarthStoryFlightView::ShowMapLibrary()
{
	CMapLibraryDlg dlg(this);
	const INT_PTR result = dlg.DoModal();
	if (result == IDCANCEL)
		return;

	if (result == IDOK)
	{
		ApplySavedMapSelection(dlg.GetSelectedMap());
		LoadSelectedMapIntoWorkView();
		return;
	}

	if (result == IDC_BTN_ADD_NEW_MAP)
		BeginNewMapEditor();
}

void CEarthStoryFlightView::CreateMapEditorToolbar()
{
	if (::IsWindow(m_btnMapSelect.GetSafeHwnd()))
		return;

	const DWORD style = WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON;
	m_btnMapSelect.Create(L"\uC601\uC5ED \uC120\uD0DD", style, CRect(0, 0, 0, 0), this, IDC_BTN_MAP_SELECT);
	m_btnMapSave.Create(L"\uC120\uD0DD \uC601\uC5ED \uC800\uC7A5", style, CRect(0, 0, 0, 0), this, IDC_BTN_MAP_SAVE);
	m_btnMapBack.Create(L"\uBAA9\uB85D\uC73C\uB85C \uB3CC\uC544\uAC00\uAE30", style, CRect(0, 0, 0, 0), this, IDC_BTN_MAP_BACK);
	LayoutMapEditorToolbar();
	UpdateMapEditorToolbar();
}

void CEarthStoryFlightView::DestroyMapEditorToolbar()
{
	if (::IsWindow(m_btnMapSelect.GetSafeHwnd()))
		m_btnMapSelect.DestroyWindow();
	if (::IsWindow(m_btnMapSave.GetSafeHwnd()))
		m_btnMapSave.DestroyWindow();
	if (::IsWindow(m_btnMapBack.GetSafeHwnd()))
		m_btnMapBack.DestroyWindow();
}

void CEarthStoryFlightView::LayoutMapEditorToolbar()
{
	if (!m_newMapEditorMode)
		return;

	CRect clientRect;
	GetClientRect(&clientRect);

	CRect selectRect(clientRect.left + 8, clientRect.top + 6, clientRect.left + 98, clientRect.top + 30);
	CRect saveRect(selectRect.right + 8, selectRect.top, selectRect.right + 128, selectRect.bottom);
	CRect backRect(saveRect.right + 8, selectRect.top, saveRect.right + 158, selectRect.bottom);

	if (::IsWindow(m_btnMapSelect.GetSafeHwnd()))
		m_btnMapSelect.MoveWindow(selectRect);
	if (::IsWindow(m_btnMapSave.GetSafeHwnd()))
		m_btnMapSave.MoveWindow(saveRect);
	if (::IsWindow(m_btnMapBack.GetSafeHwnd()))
		m_btnMapBack.MoveWindow(backRect);
}

void CEarthStoryFlightView::UpdateMapEditorToolbar()
{
	if (::IsWindow(m_btnMapSave.GetSafeHwnd()))
		m_btnMapSave.EnableWindow(m_bounds.valid ? TRUE : FALSE);
}

void CEarthStoryFlightView::ClearEditorMapIdentity()
{
	m_currentMapName.Empty();
	m_currentMapDirectory.Empty();
	m_routeName.Empty();
	m_selectedRouteId.Empty();
	m_routeCatalog = MapRouteCatalog();
	m_waypoints.clear();
	m_hasSelectedSavedMap = false;
	m_selectedSavedMap = SavedMapEntry();
}

void CEarthStoryFlightView::BeginNewMapEditor()
{
	CString message;
	if (!EnsureApiKey(message))
	{
		AfxMessageBox(message, MB_ICONWARNING);
		ShowMapLibrary();
		return;
	}

	m_newMapEditorMode = true;
	m_mapOpened = false;
	m_bounds = MapRegionBounds();
	m_hasCursor = false;
	m_currentMapTypeId = MapConfig::LoadMapTypeId();
	ClearWorkMapImage();
	ClearEditorMapIdentity();

	CreateMapEditorToolbar();
	if (!OpenGoogleMapInternal())
	{
		EndNewMapEditor();
		ShowMapLibrary();
		return;
	}

	Invalidate(FALSE);
}

void CEarthStoryFlightView::BeginExistingMapEditor()
{
	CString message;
	if (!EnsureApiKey(message))
	{
		AfxMessageBox(message, MB_ICONWARNING);
		ShowMapLibrary();
		return;
	}

	const SavedMapEntry selected = m_selectedSavedMap;
	m_newMapEditorMode = true;
	m_mapOpened = false;
	m_hasCursor = false;
	m_currentMapTypeId = MapConfig::LoadMapTypeId();
	m_currentMapName = selected.name;
	m_currentMapDirectory = selected.directory;
	m_currentMapType = selected.type;
	m_hasSelectedSavedMap = true;
	if (selected.hasBounds)
		m_bounds = selected.bounds;
	ClearWorkMapImage();

	CreateMapEditorToolbar();
	if (!OpenGoogleMapInternal())
	{
		EndNewMapEditor();
		ShowMapLibrary();
		return;
	}

	if (m_bounds.valid)
		m_mapHost.LoadSavedBounds(m_bounds);
	RestoreRouteForCurrentMap();
	Invalidate(FALSE);
}

void CEarthStoryFlightView::EndNewMapEditor()
{
	m_newMapEditorMode = false;
	m_mapOpened = false;
	m_bounds = MapRegionBounds();
	m_hasCursor = false;
	m_waypoints.clear();
	m_routeName.Empty();
	m_selectedRouteId.Empty();
	m_routeCatalog = MapRouteCatalog();
	m_mapHost.Show(SW_HIDE);
	DestroyMapEditorToolbar();
	UpdateMapCursorStatus();
	Invalidate(FALSE);
}

bool CEarthStoryFlightView::OpenGoogleMapInternal()
{
	m_mapOpened = true;
	m_mapHost.Resize(GetMapHostRect());
	m_mapHost.Show(SW_SHOW);
	if (!m_mapHost.OpenMap(m_apiKey))
	{
		CString errorMessage = m_mapHost.GetLastErrorMessage();
		if (errorMessage.IsEmpty())
			errorMessage = L"Failed to open ESF map page.";
		AfxMessageBox(errorMessage, MB_ICONERROR);
		m_mapOpened = false;
		m_mapHost.Show(SW_HIDE);
		return false;
	}

	return true;
}

void CEarthStoryFlightView::OnMapEditorSelect()
{
	if (!m_newMapEditorMode || !m_mapOpened)
		return;

	m_mapHost.PostCommandJson(L"{\"cmd\":\"select\"}");
}

void CEarthStoryFlightView::OnMapEditorSave()
{
	if (!m_newMapEditorMode)
		return;

	m_mapHost.PostCommandJson(L"{\"cmd\":\"logMapType\",\"event\":\"save:before\"}");
	if (CaptureAndSaveRegion())
	{
		EndNewMapEditor();
		ShowMapLibrary();
	}
}

void CEarthStoryFlightView::OnMapEditorBack()
{
	if (!m_newMapEditorMode)
		return;

	EndNewMapEditor();
	ShowMapLibrary();
}

bool CEarthStoryFlightView::CaptureAndSaveRegion()
{
	CString message;
	if (!EnsureApiKey(message))
	{
		AfxMessageBox(message, MB_ICONWARNING);
		return false;
	}
	if (!m_bounds.valid)
	{
		AfxMessageBox(L"Select a region with two clicks first.", MB_ICONINFORMATION);
		return false;
	}

	CString displayName;
	if (!PromptMapName(displayName))
		return false;

	if (displayName.IsEmpty())
		displayName = L"map";

	const CString folderName = MakeUniqueFolderName(displayName);
	m_currentMapName = folderName;
	m_currentMapType = MapStorageType::GeoBounds;
	m_currentMapDirectory = MapRegionService::GetGeoBoundsMapDirectory(folderName);

	const CString mapTypeId = m_currentMapTypeId.IsEmpty()
		? MapConfig::LoadMapTypeId()
		: MapConfig::NormalizeMapTypeId(m_currentMapTypeId);
	if (!MapRegionService::SaveRegionFiles(m_bounds, m_apiKey, folderName, displayName, mapTypeId, message))
	{
		AfxMessageBox(message, MB_ICONERROR);
		return false;
	}

	if (!m_waypoints.empty())
	{
		m_routeCatalog = MapRouteCatalog();
		m_routeCatalog.mapId = folderName;
		m_selectedRouteId.Empty();
		m_routeName = m_routeName.IsEmpty() ? (folderName + L" \uACBD\uB85C") : m_routeName;
		if (UpdateOrAddSavedRoute(m_routeName))
			message += L"\r\nSaved routes.json";
	}

	AfxMessageBox(message, MB_ICONINFORMATION);
	return true;
}

void CEarthStoryFlightView::ApplyWaypointsFromMessage(const CString& message)
{
	std::vector<MapWaypoint> waypoints;
	if (MapRegionService::ParseWaypointsJson(message, waypoints))
		m_waypoints = std::move(waypoints);
}

void CEarthStoryFlightView::PushRouteStateToMap()
{
	MapRoute route;
	const int index = MapRegionService::FindRouteIndex(m_routeCatalog, m_selectedRouteId);
	if (index >= 0)
		route = m_routeCatalog.routes[static_cast<size_t>(index)];
	else
	{
		route.id = m_selectedRouteId;
		route.name = m_routeName;
		route.mapId = m_currentMapName;
		route.waypoints = m_waypoints;
	}

	m_mapHost.LoadRouteCommand(
		MapRegionService::BuildLoadWaypointsCommand(route, m_routeCatalog, m_selectedRouteId));
}

void CEarthStoryFlightView::RefreshRouteListOnMap()
{
	if (!m_mapOpened)
		return;

	m_mapHost.PostCommandJson(
		MapRegionService::BuildSetRouteListCommand(m_routeCatalog, m_selectedRouteId));
}

void CEarthStoryFlightView::RestoreRouteForCurrentMap()
{
	m_waypoints.clear();
	m_routeName.Empty();
	m_selectedRouteId.Empty();
	m_routeCatalog = MapRouteCatalog();
	if (m_currentMapDirectory.IsEmpty())
		return;

	CString message;
	if (!MapRegionService::LoadOrMigrateRoutes(
		m_currentMapDirectory, m_currentMapName, m_routeCatalog, message))
	{
		return;
	}

	if (m_routeCatalog.routes.empty())
	{
		RefreshRouteListOnMap();
		return;
	}

	m_selectedRouteId = m_routeCatalog.routes[0].id;
	m_routeName = m_routeCatalog.routes[0].name;
	m_waypoints = m_routeCatalog.routes[0].waypoints;
	PushRouteStateToMap();
}

bool CEarthStoryFlightView::UpdateOrAddSavedRoute(const CString& routeName)
{
	if (m_currentMapDirectory.IsEmpty() || m_currentMapName.IsEmpty())
		return false;

	m_routeCatalog.mapId = m_currentMapName;
	const CString now = MapRegionService::NowTimeStamp();
	const int selectedIndex = MapRegionService::FindRouteIndex(m_routeCatalog, m_selectedRouteId);
	const bool updateSelected = selectedIndex >= 0 &&
		m_routeCatalog.routes[static_cast<size_t>(selectedIndex)].name == routeName;

	if (updateSelected)
	{
		MapRoute& route = m_routeCatalog.routes[static_cast<size_t>(selectedIndex)];
		route.name = routeName;
		route.mapId = m_currentMapName;
		route.modifiedAt = now;
		if (route.createdAt.IsEmpty())
			route.createdAt = now;
		route.waypoints = m_waypoints;
		m_routeName = routeName;
	}
	else
	{
		MapRoute route;
		route.id = MapRegionService::NextRouteId(m_routeCatalog);
		route.name = routeName;
		route.mapId = m_currentMapName;
		route.createdAt = now;
		route.modifiedAt = now;
		route.speedMps = MapRegionService::kInitialFlightSpeedMps;
		route.waypoints = m_waypoints;
		m_routeCatalog.routes.push_back(route);
		m_selectedRouteId = route.id;
		m_routeName = routeName;
	}

	CString message;
	if (!MapRegionService::SaveRoutesJson(m_currentMapDirectory, m_routeCatalog, message))
	{
		AfxMessageBox(message, MB_ICONERROR);
		return false;
	}

	RefreshRouteListOnMap();
	return true;
}

bool CEarthStoryFlightView::SaveCurrentRoute(bool confirmOverwrite)
{
	if (m_currentMapDirectory.IsEmpty() || m_currentMapName.IsEmpty())
	{
		AfxMessageBox(L"\uACBD\uB85C\uB97C \uC800\uC7A5\uD558\uB824\uBA74 \uBA3C\uC800 \uC120\uD0DD \uC601\uC5ED\uC744 \uC800\uC7A5\uD558\uC138\uC694.", MB_ICONINFORMATION);
		return false;
	}

	CString routeName = m_routeName.IsEmpty() ? (m_currentMapName + L" \uACBD\uB85C") : m_routeName;
	if (!PromptRouteName(routeName))
		return false;

	const int selectedIndex = MapRegionService::FindRouteIndex(m_routeCatalog, m_selectedRouteId);
	const bool updating = selectedIndex >= 0 &&
		m_routeCatalog.routes[static_cast<size_t>(selectedIndex)].name == routeName;
	if (confirmOverwrite && updating)
	{
		if (AfxMessageBox(
			L"\uC120\uD0DD\uD55C \uACBD\uB85C\uB97C \uB36E\uC5B4\uC4F0\uC2DC\uACA0\uC2B5\uB2C8\uAE4C?",
			MB_YESNO | MB_ICONQUESTION) != IDYES)
		{
			return false;
		}
	}

	if (!UpdateOrAddSavedRoute(routeName))
		return false;

	AfxMessageBox(L"\uACBD\uB85C\uB97C \uC800\uC7A5\uD588\uC2B5\uB2C8\uB2E4.", MB_ICONINFORMATION);
	return true;
}

void CEarthStoryFlightView::LoadRouteById(const CString& routeId)
{
	const int index = MapRegionService::FindRouteIndex(m_routeCatalog, routeId);
	if (index < 0)
	{
		AfxMessageBox(L"\uC120\uD0DD\uD55C \uACBD\uB85C\uB97C \uCC3E\uC744 \uC218 \uC5C6\uC2B5\uB2C8\uB2E4.", MB_ICONINFORMATION);
		return;
	}

	const MapRoute& route = m_routeCatalog.routes[static_cast<size_t>(index)];
	m_selectedRouteId = route.id;
	m_routeName = route.name;
	m_waypoints = route.waypoints;
	PushRouteStateToMap();
}

void CEarthStoryFlightView::RenameSelectedRoute()
{
	const int index = MapRegionService::FindRouteIndex(m_routeCatalog, m_selectedRouteId);
	if (index < 0)
	{
		AfxMessageBox(L"\uC774\uB984\uC744 \uBCC0\uACBD\uD560 \uACBD\uB85C\uB97C \uBA3C\uC800 \uC120\uD0DD\uD558\uC138\uC694.", MB_ICONINFORMATION);
		return;
	}

	CString routeName = m_routeCatalog.routes[static_cast<size_t>(index)].name;
	if (!PromptRouteName(routeName))
		return;

	MapRoute& route = m_routeCatalog.routes[static_cast<size_t>(index)];
	route.name = routeName;
	route.modifiedAt = MapRegionService::NowTimeStamp();
	m_routeName = routeName;

	CString message;
	if (!MapRegionService::SaveRoutesJson(m_currentMapDirectory, m_routeCatalog, message))
	{
		AfxMessageBox(message, MB_ICONERROR);
		return;
	}

	RefreshRouteListOnMap();
}

void CEarthStoryFlightView::DeleteSelectedRoute()
{
	const int index = MapRegionService::FindRouteIndex(m_routeCatalog, m_selectedRouteId);
	if (index < 0)
	{
		AfxMessageBox(L"\uC0AD\uC81C\uD560 \uACBD\uB85C\uB97C \uBA3C\uC800 \uC120\uD0DD\uD558\uC138\uC694.", MB_ICONINFORMATION);
		return;
	}

	CString confirm;
	confirm.Format(
		L"\uACBD\uB85C '%s'\uB97C \uC0AD\uC81C\uD558\uC2DC\uACA0\uC2B5\uB2C8\uAE4C?",
		m_routeCatalog.routes[static_cast<size_t>(index)].name.GetString());
	if (AfxMessageBox(confirm, MB_YESNO | MB_ICONQUESTION) != IDYES)
		return;

	m_routeCatalog.routes.erase(m_routeCatalog.routes.begin() + index);
	if (!m_routeCatalog.routes.empty())
	{
		const MapRoute& route = m_routeCatalog.routes[0];
		m_selectedRouteId = route.id;
		m_routeName = route.name;
		m_waypoints = route.waypoints;
	}
	else
	{
		m_selectedRouteId.Empty();
		m_routeName.Empty();
		m_waypoints.clear();
	}

	CString message;
	if (!MapRegionService::SaveRoutesJson(m_currentMapDirectory, m_routeCatalog, message))
	{
		AfxMessageBox(message, MB_ICONERROR);
		return;
	}

	PushRouteStateToMap();
}

void CEarthStoryFlightView::FlySelectedRoute(const CString& speedText)
{
	const int index = MapRegionService::FindRouteIndex(m_routeCatalog, m_selectedRouteId);
	if (index < 0)
	{
		AfxMessageBox(L"\uBE44\uD589\uD560 \uACBD\uB85C\uB97C \uBA3C\uC800 \uC120\uD0DD\uD558\uC138\uC694.", MB_ICONINFORMATION);
		return;
	}

	double speedMps = 0.0;
	CString speedMessage;
	if (!MapRegionService::ParseFlightSpeedMps(speedText, speedMps, speedMessage))
	{
		AfxMessageBox(speedMessage, MB_ICONINFORMATION);
		return;
	}

	MapRoute& route = m_routeCatalog.routes[static_cast<size_t>(index)];
	route.speedMps = speedMps;
	route.modifiedAt = MapRegionService::NowTimeStamp();

	CString saveMessage;
	if (!MapRegionService::SaveRoutesJson(m_currentMapDirectory, m_routeCatalog, saveMessage))
	{
		AfxMessageBox(saveMessage, MB_ICONERROR);
		return;
	}

	RefreshRouteListOnMap();

	CString flyMessage;
	if (!GoogleEarthPoC::RunSavedRouteTour(route, flyMessage))
	{
		if (!flyMessage.IsEmpty())
			AfxMessageBox(flyMessage, MB_ICONERROR);
	}
}

void CEarthStoryFlightView::ReloadSavedRegion()
{
	MapRegionBounds savedBounds;
	CString message;
	const CString mapDirectory = m_currentMapDirectory.IsEmpty()
		? MapRegionService::GetGeoBoundsMapDirectory(m_currentMapName)
		: m_currentMapDirectory;
	if (MapRegionService::LoadRegionJsonFromDirectory(savedBounds, mapDirectory, message))
	{
		m_bounds = savedBounds;
		if (m_mapOpened)
			m_mapHost.LoadSavedBounds(savedBounds);
	}
}

void CEarthStoryFlightView::ApplySavedMapSelection(const SavedMapEntry& entry)
{
	m_selectedSavedMap = entry;
	m_hasSelectedSavedMap = true;
	m_currentMapName = entry.name;
	m_currentMapType = entry.type;
	m_currentMapDirectory = entry.directory;
	if (entry.hasBounds)
		m_bounds = entry.bounds;
}

void CEarthStoryFlightView::ClearWorkMapImage()
{
	if (!m_workMapImage.IsNull())
		m_workMapImage.Destroy();
}

void CEarthStoryFlightView::LoadSelectedMapIntoWorkView()
{
	if (m_selectedSavedMap.type == MapStorageType::GeoBounds && m_selectedSavedMap.hasBounds)
	{
		BeginExistingMapEditor();
		return;
	}

	EndNewMapEditor();
	m_mapOpened = false;
	m_mapHost.Show(SW_HIDE);

	ClearWorkMapImage();
	if (!m_selectedSavedMap.mapPngPath.IsEmpty())
	{
		if (FAILED(m_workMapImage.Load(m_selectedSavedMap.mapPngPath)))
			AfxMessageBox(L"Failed to load selected map image.", MB_ICONWARNING);
	}

	Invalidate(FALSE);
}

#ifdef _DEBUG
CEarthStoryFlightDoc* CEarthStoryFlightView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CEarthStoryFlightDoc)));
	return (CEarthStoryFlightDoc*)m_pDocument;
}
#endif
