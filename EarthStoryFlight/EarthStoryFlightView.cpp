#include "pch.h"
#include "framework.h"
#include "EarthStoryFlightDoc.h"
#include "EarthStoryFlightView.h"
#include "MapConfig.h"
#include "MapLibraryDlg.h"
#include "MapLibraryService.h"
#include "resource.h"
#include <algorithm>
#include <cmath>

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
			SetDlgItemText(IDC_EDIT_MAPNAME, m_mapName);
			return TRUE;
		}

		void OnOK() override
		{
			GetDlgItemText(IDC_EDIT_MAPNAME, m_mapName);
			m_mapName.Trim();
			CDialogEx::OnOK();
		}
	};

	CMapNameDlg dlg(this, outMapName);
	if (dlg.DoModal() != IDOK)
		return false;

	outMapName = dlg.m_mapName;
	outMapName.Trim();
	return true;
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
	}
	else if (message.Find(L"\"type\":\"reset\"") >= 0)
	{
		m_bounds = MapRegionBounds();
		UpdateMapEditorToolbar();
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
	ClearWorkMapImage();
	m_hasSelectedSavedMap = false;

	CreateMapEditorToolbar();
	if (!OpenGoogleMapInternal())
	{
		EndNewMapEditor();
		ShowMapLibrary();
		return;
	}

	Invalidate(FALSE);
}

void CEarthStoryFlightView::EndNewMapEditor()
{
	m_newMapEditorMode = false;
	m_mapOpened = false;
	m_bounds = MapRegionBounds();
	m_mapHost.Show(SW_HIDE);
	DestroyMapEditorToolbar();
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

	if (!MapRegionService::SaveRegionFiles(m_bounds, m_apiKey, folderName, displayName, message))
	{
		AfxMessageBox(message, MB_ICONERROR);
		return false;
	}

	AfxMessageBox(message, MB_ICONINFORMATION);
	return true;
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
