// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINFRM_H__A67CF07C_BB64_4AAC_A6B3_A53183E1282F__INCLUDED_)
#define AFX_MAINFRM_H__A67CF07C_BB64_4AAC_A6B3_A53183E1282F__INCLUDED_

#pragma once
#include "MyToolBar.h"
#include "FreePcbDoc.h"
#include "FreePcbView.h"

#define TIMER_PERIOD 10		// seconds between timer events

class CMainFrame : public CFrameWnd
{
	
protected: // create from serialization only
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

// Attributes
public:
 
// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	CMyToolBar  m_wndMyToolBar;

public:
	int DrawStatus( int pane, CString * str );
	afx_msg LONG OnChangeVisibleGrid( UINT wp, LONG lp );
	afx_msg LONG OnChangePlacementGrid( UINT wp, LONG lp );
	afx_msg LONG OnChangeRoutingGrid( UINT wp, LONG lp );
	afx_msg LONG OnChangeSnapAngle( UINT wp, LONG lp );
	afx_msg LONG OnChangeUnits( UINT wp, LONG lp );
protected:  // control bar embedded members
	CStatusBar  m_wndStatusBar;
	CView * m_view;
//	CFreePcbDoc * m_doc;
//	CStatic m_ctlStaticVisibleGrid;
//	CComboBox m_ctlComboVisibleGrid;
//	CStatic m_ctlStaticPlacementGrid;
//	CComboBox m_ctlComboPlacementGrid;
//	CStatic m_ctlStaticRoutingGrid;
//	CComboBox m_ctlComboRoutingGrid;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnEditUndo();
	UINT_PTR m_timer;
	afx_msg void OnTimer(UINT nIDEvent);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_MAINFRM_H__A67CF07C_BB64_4AAC_A6B3_A53183E1282F__INCLUDED_)
