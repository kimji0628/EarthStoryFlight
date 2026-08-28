#pragma once

class CEarthStoryFlightDoc;

#include "MapWebViewHost.h"
#include "MapRegionService.h"
#include "SavedMapTypes.h"
#include <atlimage.h>

class CEarthStoryFlightView : public CView
{
protected:
	CEarthStoryFlightView() noexcept;
	DECLARE_DYNCREATE(CEarthStoryFlightView)

public:
	CEarthStoryFlightDoc* GetDocument() const;

public:
	virtual void OnDraw(CDC* pDC);
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

public:
	void OpenMap();

	const SavedMapEntry& GetSelectedSavedMap() const { return m_selectedSavedMap; }
	bool HasSelectedSavedMap() const { return m_hasSelectedSavedMap; }

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnDestroy();
	afx_msg void OnMapEditorSelect();
	afx_msg void OnMapEditorSave();
	afx_msg void OnMapEditorBack();
	DECLARE_MESSAGE_MAP()

private:
	static constexpr int kMapToolbarHeight = 36;

	MapWebViewHost m_mapHost;
	MapRegionBounds m_bounds;
	CString m_apiKey;
	CString m_currentMapName = L"Geumsan";
	CString m_currentMapDirectory;
	MapStorageType m_currentMapType = MapStorageType::GeoBounds;
	SavedMapEntry m_selectedSavedMap;
	CImage m_workMapImage;
	CButton m_btnMapSelect;
	CButton m_btnMapSave;
	CButton m_btnMapBack;
	bool m_hasSelectedSavedMap = false;
	bool m_mapOpened = false;
	bool m_newMapEditorMode = false;

	bool EnsureApiKey(CString& outMessage);
	bool PromptMapName(CString& outMapName);
	void ApplySavedMapSelection(const SavedMapEntry& entry);
	void HandleMapMessage(const CString& message);
	void ShowMapLibrary();
	void BeginNewMapEditor();
	void EndNewMapEditor();
	bool OpenGoogleMapInternal();
	bool CaptureAndSaveRegion();
	void ReloadSavedRegion();
	void LoadSelectedMapIntoWorkView();
	void ClearWorkMapImage();
	void CreateMapEditorToolbar();
	void DestroyMapEditorToolbar();
	void LayoutMapEditorToolbar();
	void UpdateMapEditorToolbar();
	CRect GetMapHostRect() const;
	CString MakeUniqueFolderName(const CString& baseName) const;
	static double ParseJsonNumber(const CString& json, const CString& key);
};

#ifndef _DEBUG
inline CEarthStoryFlightDoc* CEarthStoryFlightView::GetDocument() const
   { return reinterpret_cast<CEarthStoryFlightDoc*>(m_pDocument); }
#endif
