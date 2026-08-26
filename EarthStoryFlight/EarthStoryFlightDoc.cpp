#include "pch.h"
#include "framework.h"
#include "EarthStoryFlightDoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

IMPLEMENT_DYNCREATE(CEarthStoryFlightDoc, CDocument)

BEGIN_MESSAGE_MAP(CEarthStoryFlightDoc, CDocument)
END_MESSAGE_MAP()

CEarthStoryFlightDoc::CEarthStoryFlightDoc() noexcept
{
}

BOOL CEarthStoryFlightDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	SetTitle(_T("\uC774\uC57C\uAE30\uB97C \uB530\uB77C \uB6F0\uB098\uB294 \uC9C0\uAD6C\uC5EC\uD589"));
	return TRUE;
}

void CEarthStoryFlightDoc::Serialize(CArchive& ar)
{
	if (ar.IsStoring())
	{
	}
	else
	{
	}
}

#ifdef _DEBUG
void CEarthStoryFlightDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CEarthStoryFlightDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif
