#include "pch.h"
#include "framework.h"
#include "EarthStoryFlight.h"
#include "MainFrm.h"
#include "EarthStoryFlightDoc.h"
#include "EarthStoryFlightView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

CEarthStoryFlightApp theApp;

BEGIN_MESSAGE_MAP(CEarthStoryFlightApp, CWinApp)
END_MESSAGE_MAP()

CEarthStoryFlightApp::CEarthStoryFlightApp() noexcept
{
}

BOOL CEarthStoryFlightApp::InitInstance()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	EnableTaskbarInteraction(FALSE);

	SetRegistryKey(_T("EarthStoryFlight"));

	LoadStdProfileSettings(4);

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CEarthStoryFlightDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(CEarthStoryFlightView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);

	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();
	return TRUE;
}

int CEarthStoryFlightApp::ExitInstance()
{
	return CWinApp::ExitInstance();
}
