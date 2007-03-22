// DlgExportDsn.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgExportDsn.h"

// globals to save options
BOOL g_bVerbose = FALSE;
BOOL g_bInfo = FALSE;

// CDlgExportDsn dialog

IMPLEMENT_DYNAMIC(CDlgExportDsn, CDialog)
CDlgExportDsn::CDlgExportDsn(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgExportDsn::IDD, pParent)
	, m_bVerbose(FALSE)
	, m_bInfo(FALSE)
	, m_bounds_poly(0)
	, m_signals_poly(0)
	, m_dsn_filepath(_T(""))
{
}

CDlgExportDsn::~CDlgExportDsn()
{
}

void CDlgExportDsn::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Check(pDX, IDC_CHECK_VERBOSE, m_bVerbose);
	DDX_Check(pDX, IDC_CHECK_INFO, m_bInfo);
	DDX_CBIndex(pDX, IDC_COMBO_BOUNDS, m_bounds_poly);
	DDX_CBIndex(pDX, IDC_COMBO_SIGNALS, m_signals_poly);
	DDX_Control(pDX, IDC_COMBO_BOUNDS, m_combo_bounds);
	DDX_Control(pDX, IDC_COMBO_SIGNALS, m_combo_signals);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		if( m_num_polys < 1 )
			m_num_polys = 1;
		for( int i=1; i<=m_num_polys; i++ )
		{
			CString str;
			str.Format( "%d", i );
			m_combo_bounds.InsertString( i-1, str );
			m_combo_signals.InsertString( i-1, str );
		}
		m_combo_bounds.SetCurSel(m_bounds_poly);
		m_combo_signals.SetCurSel(m_signals_poly);
	}
	else 
	{
		// outgoing 
		g_bVerbose = m_bVerbose;
		g_bInfo = m_bInfo;
	}
	DDX_Text(pDX, IDC_EDIT_FILE, m_dsn_filepath);
}


BEGIN_MESSAGE_MAP(CDlgExportDsn, CDialog)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
END_MESSAGE_MAP()


void CDlgExportDsn::Initialize( CString * dsn_filepath,
							    int num_board_outline_polys,
								int bounds_poly, int signals_poly )
{
	m_dsn_filepath = *dsn_filepath;
	m_bounds_poly = bounds_poly;
	m_signals_poly = signals_poly;
	m_num_polys = num_board_outline_polys;
	m_bVerbose = g_bVerbose;
	m_bInfo = g_bInfo;
}
			
// CDlgExportDsn message handlers

void CDlgExportDsn::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here
	OnOK();
}


