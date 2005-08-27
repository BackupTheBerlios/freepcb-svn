// DlgDRC.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgDRC.h"
#include ".\dlgdrc.h"

#define nm_per_mil 25400.0

// DlgDRC dialog

IMPLEMENT_DYNAMIC(DlgDRC, CDialog)
DlgDRC::DlgDRC(CWnd* pParent /*=NULL*/)
	: CDialog(DlgDRC::IDD, pParent)
{
	m_dlg_log = NULL;
}

DlgDRC::~DlgDRC()
{
	if( m_dlg_log )
	{
		m_dlg_log->DestroyWindow();
		delete m_dlg_log;
	}
}

void DlgDRC::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo_units);
	DDX_Control(pDX, IDC_EDIT1, m_edit_pad_pad);
	DDX_Control(pDX, IDC_EDIT7, m_edit_pad_trace);
	DDX_Control(pDX, IDC_EDIT6, m_edit_trace_trace);
	DDX_Control(pDX, IDC_EDIT5, m_edit_hole_copper);
	DDX_Control(pDX, IDC_EDIT4, m_edit_annular_ring_pins);
	DDX_Control(pDX, IDC_EDIT2, m_edit_board_edge_copper);
	DDX_Control(pDX, IDC_EDIT8, m_edit_hole_hole);
	DDX_Control(pDX, IDC_EDIT9, m_edit_annular_ring_vias);
	DDX_Control(pDX, IDC_EDIT10, m_edit_copper_copper);
	DDX_Control(pDX, IDC_EDIT14, m_edit_trace_width);
	DDX_Control(pDX, IDC_EDIT11, m_edit_board_edge_hole);
	DDX_Control(pDX, IDC_CHECK_SHOW_UNROUTED, m_check_show_unrouted);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		SetFields();
	}
}


BEGIN_MESSAGE_MAP(DlgDRC, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnChangeUnits)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
	ON_BN_CLICKED(IDOK, OnBnClickedOk)
	ON_BN_CLICKED(IDC_BUTTON1, OnBnClickedCheck)
END_MESSAGE_MAP()


// Initialize
//
void DlgDRC::Initialize( int units, 
						DesignRules * dr, 
						CPartList * pl, 
						CNetList * nl, 
						DRErrorList * drelist, 
						int copper_layers, 
						CPolyLine * board_outline )
{
	m_units = units; 
	m_dr = dr;
	m_pl = pl;
	m_nl = nl;
	m_copper_layers = copper_layers;
	m_board_outline = board_outline;
	m_drelist = drelist;
}

void DlgDRC::GetFields()
{
	CString str;
	m_dr->bCheckUnrouted = m_check_show_unrouted.GetCheck();
	if( m_units == MIL )
	{
		m_edit_trace_width.GetWindowText( str );
		m_dr->trace_width = atof( str ) * nm_per_mil;
		m_edit_pad_pad.GetWindowText( str );
		m_dr->pad_pad = atof( str ) * nm_per_mil;
		m_edit_pad_trace.GetWindowText( str );
		m_dr->pad_trace = atof( str ) * nm_per_mil;
		m_edit_trace_trace.GetWindowText( str );
		m_dr->trace_trace = atof( str ) * nm_per_mil;
		m_edit_hole_copper.GetWindowText( str );
		m_dr->hole_copper = atof( str ) * nm_per_mil;
		m_edit_annular_ring_pins.GetWindowText( str );
		m_dr->annular_ring_pins = atof( str ) * nm_per_mil;
		m_edit_annular_ring_vias.GetWindowText( str );
		m_dr->annular_ring_vias = atof( str ) * nm_per_mil;
		m_edit_board_edge_copper.GetWindowText( str );
		m_dr->board_edge_copper = atof( str ) * nm_per_mil;
		m_edit_board_edge_hole.GetWindowText( str );
		m_dr->board_edge_hole = atof( str ) * nm_per_mil;
		m_edit_hole_hole.GetWindowText( str );
		m_dr->hole_hole = atof( str ) * nm_per_mil;
		m_edit_copper_copper.GetWindowText( str );
		m_dr->copper_copper = atof( str ) * nm_per_mil;
	}
	else
	{
		m_edit_trace_width.GetWindowText( str );
		m_dr->trace_width = atof( str ) * 1000000.0;
		m_edit_pad_pad.GetWindowText( str );
		m_dr->pad_pad = atof( str ) * 1000000.0;
		m_edit_pad_trace.GetWindowText( str );
		m_dr->pad_trace = atof( str ) * 1000000.0;
		m_edit_trace_trace.GetWindowText( str );
		m_dr->trace_trace = atof( str ) * 1000000.0;
		m_edit_hole_copper.GetWindowText( str );
		m_dr->hole_copper = atof( str ) * 1000000.0;
		m_edit_annular_ring_pins.GetWindowText( str );
		m_dr->annular_ring_pins = atof( str ) * 1000000.0;
		m_edit_annular_ring_vias.GetWindowText( str );
		m_dr->annular_ring_vias = atof( str ) * 1000000.0;
		m_edit_board_edge_copper.GetWindowText( str );
		m_dr->board_edge_copper = atof( str ) * 1000000.0;
		m_edit_board_edge_hole.GetWindowText( str );
		m_dr->board_edge_hole = atof( str ) * 1000000.0;
		m_edit_hole_hole.GetWindowText( str );
		m_dr->hole_hole = atof( str ) * 1000000.0;
		m_edit_copper_copper.GetWindowText( str );
		m_dr->copper_copper = atof( str ) * 1000000.0;
	}
}

void DlgDRC::SetFields()
{
	CString str;
	m_check_show_unrouted.SetCheck( m_dr->bCheckUnrouted );
	if( m_units == MIL )
	{
		MakeCStringFromDouble( &str, m_dr->trace_width/nm_per_mil );
		m_edit_trace_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->pad_pad/nm_per_mil );
		m_edit_pad_pad.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->pad_trace/nm_per_mil );
		m_edit_pad_trace.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->trace_trace/nm_per_mil );
		m_edit_trace_trace.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->hole_copper/nm_per_mil );
		m_edit_hole_copper.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->annular_ring_pins/nm_per_mil );
		m_edit_annular_ring_pins.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->annular_ring_vias/nm_per_mil );
		m_edit_annular_ring_vias.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->board_edge_copper/nm_per_mil );
		m_edit_board_edge_copper.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->board_edge_hole/nm_per_mil );
		m_edit_board_edge_hole.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->hole_hole/nm_per_mil );
		m_edit_hole_hole.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->copper_copper/nm_per_mil );
		m_edit_copper_copper.SetWindowText( str );
	}
	else
	{
		MakeCStringFromDouble( &str, m_dr->trace_width/1000000.0 );
		m_edit_trace_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->pad_pad/1000000.0 );
		m_edit_pad_pad.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->pad_trace/1000000.0 );
		m_edit_pad_trace.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->trace_trace/1000000.0 );
		m_edit_trace_trace.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->hole_copper/1000000.0 );
		m_edit_hole_copper.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->annular_ring_pins/1000000.0 );
		m_edit_annular_ring_pins.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->annular_ring_vias/1000000.0 );
		m_edit_annular_ring_vias.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->board_edge_copper/1000000.0 );
		m_edit_board_edge_copper.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->board_edge_hole/1000000.0 );
		m_edit_board_edge_hole.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->hole_hole/1000000.0 );
		m_edit_hole_hole.SetWindowText( str );
		MakeCStringFromDouble( &str, m_dr->copper_copper/1000000.0 );
		m_edit_copper_copper.SetWindowText( str );
	}
}

// DlgDRC message handlers

void DlgDRC::OnCbnChangeUnits()
{
	CString str;
	m_combo_units.GetWindowText( str );
	GetFields();
	if( str == "MIL" )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

// the actual checker
//
void DlgDRC::CheckDesign()
{
	CString str;

	GetFields();
	m_drelist->Clear();
	m_pl->DRC( m_dlg_log, m_copper_layers, 
		m_units, m_check_show_unrouted.GetCheck(),
		m_board_outline,
		m_dr, m_drelist );
}

void DlgDRC::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	OnCancel();
}

void DlgDRC::OnBnClickedOk()
{
}

void DlgDRC::OnBnClickedCheck()
{
	if( !m_dlg_log )
	{
		m_dlg_log = new CDlgLog;
		m_dlg_log->Create( IDD_LOG );
	}
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	m_dlg_log->EnableOK( FALSE );
	CheckDesign();
	m_dlg_log->EnableOK( TRUE );
}
