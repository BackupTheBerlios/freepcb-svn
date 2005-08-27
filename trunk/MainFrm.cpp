// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "mainfrm.h"
#include ".\mainfrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFreePcbApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNCREATE(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
	ON_MESSAGE( WM_USER_VISIBLE_GRID, OnChangeVisibleGrid )
	ON_MESSAGE( WM_USER_PLACEMENT_GRID, OnChangePlacementGrid )
	ON_MESSAGE( WM_USER_ROUTING_GRID, OnChangeRoutingGrid )
	ON_MESSAGE( WM_USER_SNAP_ANGLE, OnChangeSnapAngle )
	ON_MESSAGE( WM_USER_UNITS, OnChangeUnits )
	ON_WM_SYSCOMMAND()
	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_WM_TIMER()
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	0,
	0,
	0,
	0,
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_bAutoMenuEnable = FALSE;
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndMyToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) )
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	// status bar stuff
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators, sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	// initialize pane 0 of status bar
	UINT uID, uStyle;
	int nWidth;
	m_wndStatusBar.GetPaneInfo( 0, uID, uStyle, nWidth );
	m_wndStatusBar.SetPaneInfo( 0, uID, uStyle | SBPS_STRETCH, 100 );

	// initialize pane 1 of status bar
	m_wndStatusBar.GetPaneInfo( 1, uID, uStyle, nWidth );
	CDC * pDC = m_wndStatusBar.GetDC();
	pDC->SelectObject( m_wndStatusBar.GetFont() );
	CRect rectArea;
	pDC->DrawText( _T("X: -9999999"), 	-1, rectArea, DT_SINGLELINE | DT_CALCRECT );
	m_wndStatusBar.ReleaseDC( pDC );
	int pane_width = rectArea.Width();
	m_wndStatusBar.SetPaneInfo( 1, uID, uStyle, pane_width );
	CString test;
	test.Format( "X: 0" );
	m_wndStatusBar.SetPaneText( 1, test );

	// initialize pane 2 of status bar
	m_wndStatusBar.GetPaneInfo( 2, uID, uStyle, nWidth );
	pDC = m_wndStatusBar.GetDC();
	pDC->SelectObject( m_wndStatusBar.GetFont() );
	pDC->DrawText( _T("Y: -9999999"), -1, rectArea, DT_SINGLELINE | DT_CALCRECT );
	m_wndStatusBar.ReleaseDC( pDC );
	pane_width = rectArea.Width();
	m_wndStatusBar.SetPaneInfo( 2, uID, uStyle, pane_width );
	test.Format( "Y: 0" );
	m_wndStatusBar.SetPaneText( 2, test );

	// initialize pane 3 of status bar
	m_wndStatusBar.GetPaneInfo( 3, uID, uStyle, nWidth );
	pDC = m_wndStatusBar.GetDC();
	pDC->SelectObject( m_wndStatusBar.GetFont() );
	pDC->DrawText( _T("AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), 
						-1, rectArea, DT_SINGLELINE | DT_CALCRECT );
	m_wndStatusBar.ReleaseDC( pDC );
	pane_width = rectArea.Width();
	m_wndStatusBar.SetPaneInfo( 3, uID, uStyle, pane_width );
	test.Format( "hello" );
	m_wndStatusBar.SetPaneText( 3, test );

	// initialize pane 4 of status bar
	m_wndStatusBar.GetPaneInfo( 4, uID, uStyle, nWidth );
	pDC = m_wndStatusBar.GetDC();
	pDC->SelectObject( m_wndStatusBar.GetFont() );
	pDC->DrawText( _T("BottomXXX"), -1, rectArea, DT_SINGLELINE | DT_CALCRECT );
	m_wndStatusBar.ReleaseDC( pDC );
	pane_width = rectArea.Width();
	m_wndStatusBar.SetPaneInfo( 4, uID, uStyle, pane_width );
	test.Format( "hello" );
	m_wndStatusBar.SetPaneText( 4, test );

	// menu stuff
	CMenu* pMenu = GetMenu();
	pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
	CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
	submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
	submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );

	// create timer event every TIMER_PERIOD seconds
	m_timer = SetTimer( 1, TIMER_PERIOD*1000, 0 );

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

int CMainFrame::DrawStatus( int pane, CString * str )
{
	m_wndStatusBar.SetPaneText( pane, *str );
	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers

LONG CMainFrame::OnChangeVisibleGrid( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangeVisibleGrid( wp, lp );
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnChangeVisibleGrid( wp, lp );
	return 0;
}

LONG CMainFrame::OnChangePlacementGrid( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangePlacementGrid( wp, lp );
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnChangePlacementGrid( wp, lp );
	return 0;
}

LONG CMainFrame::OnChangeRoutingGrid( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangeRoutingGrid( wp, lp );
	return 0;
}

LONG CMainFrame::OnChangeSnapAngle( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangeSnapAngle( wp, lp );
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnChangeSnapAngle( wp, lp );
	return 0;
}

LONG CMainFrame::OnChangeUnits( UINT wp, LONG lp )
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		((CFreePcbView*)m_view)->OnChangeUnits( wp, lp );
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnChangeUnits( wp, lp );
	return 0;
}


void CMainFrame::OnSysCommand(UINT nID, LPARAM lParam)
{
	m_view = (CView*)GetActiveView();
	if( nID == SC_CLOSE )
	{
		CFreePcbDoc * doc = (CFreePcbDoc*)GetActiveDocument();
		if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		{
			if( doc )
			{
				if( doc->FileClose() == IDCANCEL )
					return;
			}
		}
		else
		{
			((CFootprintView*)m_view)->OnFootprintFileClose();
			return;
		}
	}
	CFrameWnd::OnSysCommand(nID, lParam);
}

void CMainFrame::OnEditUndo()
{
	m_view = (CView*)GetActiveView();
	CFreePcbDoc * doc = (CFreePcbDoc*)GetActiveDocument();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
		doc->OnEditUndo();
	else if( m_view->IsKindOf( RUNTIME_CLASS(CFootprintView) ) )
		((CFootprintView*)m_view)->OnEditUndo();
}

void CMainFrame::OnTimer(UINT nIDEvent)
{
	m_view = (CView*)GetActiveView();
	if( m_view->IsKindOf( RUNTIME_CLASS(CFreePcbView) ) )
	{
		CFreePcbDoc * doc = (CFreePcbDoc*)GetActiveDocument();
		if( doc->m_project_open )
			doc->OnTimer();
	}

	CFrameWnd::OnTimer(nIDEvent);
}
