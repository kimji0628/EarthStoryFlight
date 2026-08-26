#pragma once

class CEarthStoryFlightDoc : public CDocument
{
protected:
	CEarthStoryFlightDoc() noexcept;
	DECLARE_DYNCREATE(CEarthStoryFlightDoc)

public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	DECLARE_MESSAGE_MAP()
};
