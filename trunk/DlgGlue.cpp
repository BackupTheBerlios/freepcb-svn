// DlgCentroid.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgCentroid.h"
#include "DlgGlue.h"

// defines
enum {
	POS_DEFAULT,
	POS_DEFINED,
	POS_DRAG
};

// CDlgGlue dialog

IMPLEMENT_DYNAMIC(CDlgGlue, CDialog)
CDlgGlue::CDlgGlue(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgGlue::IDD, pParent)
{
}

CDlgGlue::~CDlgGlue()
{
}

void CDlgGlue::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO3, m_button_default);
	DDX_Control(pDX, IDC_RADIO2, m_button_set);
	DDX_Control(pDX, IDC_RADIO1, m_radio_drag);
	DDX_Control(pDX, IDC_COMBO1, m_combo_units);
	DDX_Control(pDX, IDC_EDIT1, m_edit_x);
	DDX_Control(pDX, IDC_EDIT2, m_edit_y);
	DDX_Control(pDX, IDC_RADIO4, m_radio_default_size);
	DDX_Control(pDX, IDC_RADIO_SET_SIZE, m_radio_set_size);
	DDX_Control(pDX, IDC_EDIT_DIAMETER, m_edit_w);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_size_type == GLUE_SIZE_DEFAULT )
			m_radio_default_size.SetCheck( 1 );
		if( m_pos_type == POS_DEFAULT )
			m_button_default.SetCheck( 1 );
		else if( m_pos_type == POS_DEFINED )
			m_button_default.SetCheck( 1 );
		else
			ASSERT(0);
		SetFields();
	}
	else
	{
		// outgoing
		GetFields();
	}
}


BEGIN_MESSAGE_MAP(CDlgGlue, CDialog)
	ON_CBN_SELCHANGE(IDC_COMBO1, OnCbnSelChangeCombo1)
	ON_BN_CLICKED(IDC_RADIO3, OnBnClickedDefault)
	ON_BN_CLICKED(IDC_RADIO2, OnBnClickedSet)
	ON_BN_CLICKED(IDC_RADIO1, OnBnClickedDrag)
	ON_BN_CLICKED(IDC_RADIO4, OnBnClickedDefaultSize)
	ON_BN_CLICKED(IDC_RADIO_SET_SIZE, OnBnClickedRadioSetSize)
END_MESSAGE_MAP()

void CDlgGlue::Initialize( GLUE_SIZE_TYPE size_type, GLUE_POS_TYPE pos_type, int units, int w, int x, int y )
{
	m_size_type = size_type;
	if( pos_type == GLUE_POS_DEFAULT )
		m_pos_type = POS_DEFAULT;
	else if( pos_type == GLUE_POS_DEFINED )
		m_pos_type = POS_DEFINED;
	m_units = units;
	m_w = w;
	m_x = x;
	m_y = y;
}

void CDlgGlue::SetFields()
{
	CString str;
	if( m_units == MIL ) 
		m_combo_units.SetCurSel( 0 );
	else
		m_combo_units.SetCurSel( 1 );
	if( m_size_type == GLUE_SIZE_DEFAULT )
	{
		m_edit_w.EnableWindow( FALSE );
		m_radio_default_size.SetCheck( 1 );
	}
	else
	{
		m_edit_w.EnableWindow( TRUE );
		m_radio_set_size.SetCheck( 1 );
	}
	if( m_pos_type == POS_DEFAULT )
	{
		m_edit_x.EnableWindow( FALSE );
		m_edit_y.EnableWindow( FALSE );
	}
	else if( m_pos_type == POS_DEFINED )
	{
		m_edit_x.EnableWindow( TRUE );
		m_edit_y.EnableWindow( TRUE );
	}
	else
	{
		m_edit_x.EnableWindow( FALSE );
		m_edit_y.EnableWindow( FALSE );
	}
	::MakeCStringFromDimension( &str, m_w, m_units, FALSE, FALSE, FALSE, 3 );
	m_edit_w.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_x, m_units, FALSE, FALSE, FALSE, 3 );
	m_edit_x.SetWindowText( str );
	::MakeCStringFromDimension( &str, m_y, m_units, FALSE, FALSE, FALSE, 3 );
	m_edit_y.SetWindowText( str );
}

void CDlgGlue::GetFields()
{
	if( m_button_default.GetCheck() )
		m_pos_type = POS_DEFAULT;
	else if( m_button_set.GetCheck() )
		m_pos_type = POS_DEFINED;
	else
		m_pos_type = POS_DRAG;
	if( m_radio_default_size.GetCheck() )
		m_size_type = GLUE_SIZE_DEFAULT;
	else
		m_size_type = GLUE_SIZE_DEFINED;
	CString str;
	m_edit_w.GetWindowText( str );
	m_w = ::GetDimensionFromString( &str, m_units );
	m_edit_x.GetWindowText( str );
	m_x = ::GetDimensionFromString( &str, m_units );
	m_edit_y.GetWindowText( str );
	m_y = ::GetDimensionFromString( &str, m_units );
}


// CDlgGlue message handlers

void CDlgGlue::OnBnClickedDefault()
{
	GetFields();
	SetFields();
}

void CDlgGlue::OnBnClickedSet()
{
	GetFields();
	SetFields();
}

void CDlgGlue::OnCbnSelChangeCombo1()
{
	GetFields();
	int m_sel_units = m_combo_units.GetCurSel();
	if( m_sel_units == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void CDlgGlue::OnBnClickedDrag()
{
	GetFields();
	SetFields();
}

void CDlgGlue::OnBnClickedDefaultSize()
{
	GetFields();
	SetFields();
}


void CDlgGlue::OnBnClickedRadioSetSize()
{
	GetFields();
	SetFields();
}
