// DlgImportOptions.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgImportOptions.h"


// CDlgImportOptions dialog

IMPLEMENT_DYNAMIC(CDlgImportOptions, CDialog)
CDlgImportOptions::CDlgImportOptions(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgImportOptions::IDD, pParent)
{
}

CDlgImportOptions::~CDlgImportOptions()
{
}

void CDlgImportOptions::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_REMOVE_PARTS, m_radio_remove_parts);
	DDX_Control(pDX, IDC_RADIO_KEEP_PARTS_NO_CONNECTIONS, m_radio_keep_parts_no_connections);
	DDX_Control(pDX, IDC_RADIO_KEEP_PARTS_AND_CONNECTIONS, m_radio_keep_parts_and_connections);
	DDX_Control(pDX, IDC_RADIO4, m_radio_change_fp);
	DDX_Control(pDX, IDC_RADIO3, m_radio_keep_fp);
	DDX_Control(pDX, IDC_RADIO6, m_radio_remove_nets);
	DDX_Control(pDX, IDC_RADIO5, m_radio_keep_nets);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_radio_remove_parts.SetCheck( TRUE );
		m_radio_change_fp.SetCheck( TRUE );
		m_radio_remove_nets.SetCheck( TRUE );
		if( m_pn_flags & IMPORT_PARTS )
		{
			m_radio_remove_parts.EnableWindow( 1 );
			m_radio_keep_parts_no_connections.EnableWindow( 1 );
			m_radio_keep_parts_and_connections.EnableWindow( 1 );
			m_radio_keep_fp.EnableWindow( 1 );
			m_radio_change_fp.EnableWindow( 1 );
		}
		else
		{
			m_radio_remove_parts.EnableWindow( 0 );
			m_radio_keep_parts_no_connections.EnableWindow( 0 );
			m_radio_keep_parts_and_connections.EnableWindow( 0 );
			m_radio_keep_fp.EnableWindow( 0 );
			m_radio_change_fp.EnableWindow( 0 );
		}
		if( m_pn_flags & IMPORT_NETS )
		{
			m_radio_keep_nets.EnableWindow( 1 );
			m_radio_remove_nets.EnableWindow( 1 );
		}
		else
		{
			m_radio_keep_nets.EnableWindow( 0 );
			m_radio_remove_nets.EnableWindow( 0 );
		}
	}
	else
	{
		// outgoing
		m_flags = 0;
		if( m_radio_keep_parts_no_connections.GetCheck() )
			m_flags |= KEEP_PARTS_NO_CON;
		else if( m_radio_keep_parts_and_connections.GetCheck() )
			m_flags |= KEEP_PARTS_AND_CON;
		if( m_radio_keep_fp.GetCheck() )
			m_flags |= KEEP_FP;
		if( m_radio_keep_nets.GetCheck() )
			m_flags |= KEEP_NETS;
	}
}


BEGIN_MESSAGE_MAP(CDlgImportOptions, CDialog)
END_MESSAGE_MAP()

void CDlgImportOptions::Initialize( int pn_flags, int flags )
{
	m_flags = flags;
	m_pn_flags = pn_flags;
}

// CDlgImportOptions message handlers
