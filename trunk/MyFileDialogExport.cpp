// MyFileDialog.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "MyFileDialogExport.h"


// CMyFileDialogExport

IMPLEMENT_DYNAMIC(CMyFileDialogExport, CFileDialog)
CMyFileDialogExport::CMyFileDialogExport(BOOL bOpenFileDialog, LPCTSTR lpszDefExt, LPCTSTR lpszFileName,
		DWORD dwFlags, LPCTSTR lpszFilter, CWnd* pParentWnd, DWORD dsize) :
		CFileDialog(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, pParentWnd, dsize)
{
}

CMyFileDialogExport::~CMyFileDialogExport()
{
}

void CMyFileDialogExport::DoDataExchange(CDataExchange* pDX)
{
	// note: for some reason, this is not called on exit, use OnFileNameOK() instead
	CFileDialog::DoDataExchange(pDX);
	DDX_Control( pDX, IDC_RADIO_PARTS, m_radio_parts );
	DDX_Control( pDX, IDC_RADIO_NETS, m_radio_nets );
	DDX_Control( pDX, IDC_RADIO_PARTSANDNETS, m_radio_parts_and_nets );
	DDX_Control( pDX, IDC_RADIO_PADSPCB, m_radio_padspcb );
	DDX_Control( pDX, IDC_RADIO_FREEPCB, m_radio_freepcb );
	if( !pDX->m_bSaveAndValidate )
	{
		// on entry
		m_radio_parts_and_nets.SetCheck( TRUE );
		m_radio_padspcb.SetCheck( TRUE );
	}
}

BOOL CMyFileDialogExport::OnInitDialog()
{
	CFileDialog::OnInitDialog(); //Call base class method first
	return TRUE;
}

BOOL CMyFileDialogExport::OnFileNameOK()
{
	// on exit
	if( m_radio_parts.GetCheck() )
		m_select = PARTS_ONLY;
	else if( m_radio_nets.GetCheck() )
		m_select = NETS_ONLY;
	else
		m_select = PARTS_AND_NETS;

	if( m_radio_padspcb.GetCheck() )
		m_format = PADSPCB;

	return FALSE;
}

BEGIN_MESSAGE_MAP(CMyFileDialogExport, CFileDialog)
END_MESSAGE_MAP()

// CMyFileDialogExport message handlers

