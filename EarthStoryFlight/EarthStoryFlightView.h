#pragma once

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

protected:
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG
inline CEarthStoryFlightDoc* CEarthStoryFlightView::GetDocument() const
   { return reinterpret_cast<CEarthStoryFlightDoc*>(m_pDocument); }
#endif
