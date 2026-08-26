#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"

class CEarthStoryFlightApp : public CWinApp
{
public:
	CEarthStoryFlightApp() noexcept;

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	DECLARE_MESSAGE_MAP()
};

extern CEarthStoryFlightApp theApp;
