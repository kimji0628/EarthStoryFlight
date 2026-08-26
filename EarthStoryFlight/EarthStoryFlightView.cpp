#include "pch.h"
#include "framework.h"
#include "EarthStoryFlightDoc.h"
#include "EarthStoryFlightView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CEarthStoryFlightView, CView)

BEGIN_MESSAGE_MAP(CEarthStoryFlightView, CView)
END_MESSAGE_MAP()

CEarthStoryFlightView::CEarthStoryFlightView() noexcept
{
}

BOOL CEarthStoryFlightView::PreCreateWindow(CREATESTRUCT& cs)
{
	return CView::PreCreateWindow(cs);
}

void CEarthStoryFlightView::OnDraw(CDC* pDC)
{
	CEarthStoryFlightDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	CRect rect;
	GetClientRect(&rect);
	pDC->FillSolidRect(rect, RGB(255, 255, 255));
}

#ifdef _DEBUG
CEarthStoryFlightDoc* CEarthStoryFlightView::GetDocument() const
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CEarthStoryFlightDoc)));
	return (CEarthStoryFlightDoc*)m_pDocument;
}
#endif
