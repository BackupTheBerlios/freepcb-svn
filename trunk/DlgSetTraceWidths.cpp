// DlgSetTraceWidths.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgSetTraceWidths.h"


// CDlgSetTraceWidths dialog

IMPLEMENT_DYNAMIC(CDlgSetTraceWidths, CDialog)
CDlgSetTraceWidths::CDlgSetTraceWidths(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgSetTraceWidths::IDD, pParent)
	, m_radio_set_via(false)
{
}

CDlgSetTraceWidths::~CDlgSetTraceWidths()
{
}

void CDlgSetTraceWidths::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_WIDTH, m_combo_width);
	DDX_Control(pDX, IDC_RADIO_DEF, m_radio_default);
	DDX_Control(pDX, IDC_EDIT_VIA_W, m_edit_via_pad);
	DDX_Control(pDX, IDC_EDIT_HOLE_W, m_edit_via_hole);
	DDX_Control(pDX, IDC_CHECK1, m_check_apply);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString str;
		for( int i=0; i<m_w->GetSize(); i++ )
		{
			str.Format( "%d", (*m_w)[i]/NM_PER_MIL );
			m_combo_width.InsertString( i, str );
		}
		m_radio_default.SetCheck( 1 );
		m_edit_via_pad.EnableWindow( 0 );
		m_edit_via_hole.EnableWindow( 0 );
		m_check_apply.SetCheck(1);
	}
	else
	{
		// outgoing
		DDX_Text( pDX, IDC_COMBO_WIDTH, m_width );
		DDX_Text( pDX, IDC_EDIT_VIA_W, m_via_width );
		DDX_Text( pDX, IDC_EDIT_HOLE_W, m_hole_width );
		m_width *= NM_PER_MIL;
		m_via_width *= NM_PER_MIL;
		m_hole_width *= NM_PER_MIL;
		m_apply = m_check_apply.GetCheck();
	}
}


BEGIN_MESSAGE_MAP(CDlgSetTraceWidths, CDialog)
	ON_BN_CLICKED(IDC_RADIO_DEF, OnBnClickedRadioDef)
	ON_BN_CLICKED(IDC_RADIO_SET, OnBnClickedRadioSet)
	ON_CBN_SELCHANGE(IDC_COMBO_WIDTH, OnCbnSelchangeComboWidth)
	ON_CBN_EDITCHANGE(IDC_COMBO_WIDTH, OnCbnEditchangeComboWidth)
END_MESSAGE_MAP()


// CDlgSetTraceWidths message handlers

void CDlgSetTraceWidths::OnBnClickedRadioDef()
{
	m_edit_via_pad.EnableWindow( 0 );
	m_edit_via_hole.EnableWindow( 0 );
}

void CDlgSetTraceWidths::OnBnClickedRadioSet()
{
	m_edit_via_pad.EnableWindow( 1 );
	m_edit_via_hole.EnableWindow( 1 );
}

void CDlgSetTraceWidths::OnCbnSelchangeComboWidth()
{
	CString test;
	int i = m_combo_width.GetCurSel();
	m_combo_width.GetLBText( i, test );
	int n = m_w->GetSize();
	if( m_radio_default.GetCheck() )
	{
		int new_w = atoi( (LPCSTR)test )*NM_PER_MIL;
		int new_v_w = 0;
		int new_v_h_w = 0;
		if( new_w >= 0 )
		{
			if( new_w == 0 )
			{
				new_v_w = 0;
				new_v_h_w = 0;
			}
			else if( new_w <= (*m_w)[0] )
			{
				new_v_w = (*m_v_w)[0];
				new_v_h_w = (*m_v_h_w)[0];
			}
			else if( new_w >= (*m_w)[n-1] )
			{
				new_v_w = (*m_v_w)[n-1];
				new_v_h_w = (*m_v_h_w)[n-1];
			}
			else
			{
				for( int i=1; i<n; i++ )
				{
					if( new_w > (*m_w)[i-1] && new_w <= (*m_w)[i] ) 
					{
						new_v_w = (*m_v_w)[i];
						new_v_h_w = (*m_v_h_w)[i];
						break;
					}
				}
			}
			test.Format( "%d", new_v_w/NM_PER_MIL );
			m_edit_via_pad.SetWindowText( test );
			test.Format( "%d", new_v_h_w/NM_PER_MIL );
			m_edit_via_hole.SetWindowText( test );
		}
	}
}

void CDlgSetTraceWidths::OnCbnEditchangeComboWidth()
{
	CString test;
	int n = m_w->GetSize();
	if( m_radio_default.GetCheck() )
	{
		m_combo_width.GetWindowText( test );
		int new_w = atoi( (LPCSTR)test )*NM_PER_MIL;
		int new_v_w = 0;
		int new_v_h_w = 0;
		if( new_w >= 0 )
		{
			if( new_w == 0 )
			{
				new_v_w = 0;
				new_v_h_w = 0;
			}
			else if( new_w <= (*m_w)[0] )
			{
				new_v_w = (*m_v_w)[0];
				new_v_h_w = (*m_v_h_w)[0];
			}
			else if( new_w >= (*m_w)[n-1] )
			{
				new_v_w = (*m_v_w)[n-1];
				new_v_h_w = (*m_v_h_w)[n-1];
			}
			else
			{
				for( int i=1; i<n; i++ )
				{
					if( new_w > (*m_w)[i-1] && new_w <= (*m_w)[i] ) 
					{
						new_v_w = (*m_v_w)[i];
						new_v_h_w = (*m_v_h_w)[i];
						break;
					}
				}
			}
			test.Format( "%d", new_v_w/NM_PER_MIL );
			m_edit_via_pad.SetWindowText( test );
			test.Format( "%d", new_v_h_w/NM_PER_MIL );
			m_edit_via_hole.SetWindowText( test );
		}
	}
}
