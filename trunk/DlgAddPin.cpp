// DlgAddPin.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddPin.h"
#include ".\dlgaddpin.h"

double GetNameValue( CString * name )
{
	double value = 0.0;
	for( int ic=0; ic<name->GetLength(); ic++ )
	{
		char c = name->GetAt( ic );
		int v = 0;
		if( c >= '0' && c <= '9' )
			v = c - '0';
		else if( c >= 'a' && c <= 'z' )
			v = c - 'a' + 10;
		else if( c >= 'A' && c <= 'Z' )
			v = c - 'A' + 10;
		else v = 39;
		value = value*40.0 + v;
	}
	return value;
}

void SortByName( CArray<CString> * names )
{
	// bubble sort
	int n = names->GetSize();
	for( int is=0; is<n-1; is++ )
	{
		// swap name[is] with lowest value following in array
		double vmin = GetNameValue( &(*names)[is] );
		int imin = is;
		for( int it=is+1; it<n; it++ )
		{
			double vtest = GetNameValue( &(*names)[it] );
			if( vtest < vmin )
			{
				imin = it;
				vmin = vtest;
			}
		}
		if( imin != is )
		{
			// swap name[is] with name[imin]
			CString temp = (*names)[is];
			(*names)[is] = (*names)[imin];
			(*names)[imin] = temp;
		}
	}
}

// CDlgAddPin dialog

IMPLEMENT_DYNAMIC(CDlgAddPin, CDialog)
CDlgAddPin::CDlgAddPin(CWnd* pParent /*=NULL*/)
: CDialog(CDlgAddPin::IDD, pParent)
{
	// init params
	m_units = MIL;
	m_padstack_type = 0;
	m_same_as_pin_num = 1;
	m_hole_diam = 0;
	m_top_pad_shape = 0;
	m_top_pad_width = 0;
	m_top_pad_length = 0;
	m_top_pad_radius = 0;
	m_inner_pad_shape = 0;
	m_inner_pad_width = 0;
	m_inner_pad_length = 0;
	m_inner_pad_radius = 0;
	m_bottom_pad_shape = 0;
	m_bottom_pad_width = 0;
	m_bottom_pad_length = 0;
	m_bottom_pad_radius = 0;
	m_x = 0;
	m_y = 0;
	m_row_spacing = 0;

}

CDlgAddPin::~CDlgAddPin()
{
}

void CDlgAddPin::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_RADIO_ADD_PIN, m_radio_add_pin);
	DDX_Control(pDX, IDC_RADIO_ADD_ROW, m_radio_add_row);
	DDX_Control(pDX, IDC_COMBO_PIN_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_CHECK_SAME_AS, m_check_same_as_pin);
	DDX_Control(pDX, IDC_RADIO_SMT, m_radio_smt);
	DDX_Control(pDX, IDC_RADIO_SMT_BOTTOM, m_radio_smt_bottom);
	DDX_Control(pDX, IDC_RADIO_TH, m_radio_th);
	DDX_Control(pDX, IDC_COMBO_SAME_AS_PIN, m_combo_same_as_pin);
	DDX_Control(pDX, IDC_EDIT_PIN_NAME, m_edit_pin_name);
	DDX_Control(pDX, IDC_EDIT_NUM_PINS, m_edit_num_pins);
	DDX_Control(pDX, IDC_EDIT_HOLE_DIAM, m_edit_hole_diam);
	DDX_Control(pDX, IDC_RADIO_DRAG_PIN, m_radio_drag);
	DDX_Control(pDX, IDC_EDIT_PIN_X, m_edit_pin_x);
	DDX_Control(pDX, IDC_EDIT_PIN_Y, m_edit_pin_y);
	DDX_Control(pDX, IDC_COMBO_TOP_PAD_SHAPE, m_combo_top_shape);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_W, m_edit_top_width);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_L, m_edit_top_length);
	DDX_Control(pDX, IDC_EDIT_TOP_PAD_RAD, m_edit_top_radius);
	DDX_Control(pDX, IDC_CHECK_INNER_SAME_AS, m_check_inner_same_as);
	DDX_Control(pDX, IDC_COMBO_INNER_PAD_SHAPE, m_combo_inner_shape);
	DDX_Control(pDX, IDC_EDIT_INNER_PAD_W, m_edit_inner_width);
	DDX_Control(pDX, IDC_EDIT_INNER_PAD_L, m_edit_inner_length);
	DDX_Control(pDX, IDC_EDIT_INNER_PAD_RAD, m_edit_inner_radius);
	DDX_Control(pDX, IDC_CHECK_BOTTOM_SAME_AS, m_check_bottom_same_as);
	DDX_Control(pDX, IDC_COMBO_BOTTOM_PAD_SHAPE, m_combo_bottom_shape);
	DDX_Control(pDX, IDC_EDIT1_BOTTOM_PAD_W, m_edit_bottom_width);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_L, m_edit_bottom_length);
	DDX_Control(pDX, IDC_EDIT_BOTTOM_PAD_RAD, m_edit_bottom_radius);
	DDX_Control(pDX, IDC_COMBO_ROW_ORIENT, m_combo_row_orient);
	DDX_Control(pDX, IDC_EDIT_ROW_SPACING, m_edit_row_spacing);
	DDX_Control(pDX, IDC_COMBO_PAD_ORIENT, m_combo_pad_orient);
	DDX_Control(pDX, IDC_RADIO_SET_PIN_POS, m_radio_set_pos);
	DDX_Control(pDX, IDC_LIST_PINS, m_list_pins);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		CString str;
		m_combo_top_shape.InsertString( PAD_NONE, "none" );
		m_combo_top_shape.InsertString( PAD_ROUND, "round" );
		m_combo_top_shape.InsertString( PAD_SQUARE, "square" );
		m_combo_top_shape.InsertString( PAD_RECT, "rect" );
		m_combo_top_shape.InsertString( PAD_RRECT, "rounded-rect" );
		m_combo_top_shape.InsertString( PAD_OVAL, "oval" );
		m_combo_top_shape.InsertString( PAD_OCTAGON, "octagon" );
		m_combo_top_shape.SetCurSel( 1 );
		m_combo_inner_shape.InsertString( PAD_NONE, "none" );
		m_combo_inner_shape.InsertString( PAD_ROUND, "round" );
		m_combo_inner_shape.InsertString( PAD_SQUARE, "square" );
		m_combo_inner_shape.InsertString( PAD_RECT, "rect" );
		m_combo_inner_shape.InsertString( PAD_RRECT, "rounded-rect" );
		m_combo_inner_shape.InsertString( PAD_OVAL, "oval" );
		m_combo_inner_shape.InsertString( PAD_OCTAGON, "octagon" );
		m_combo_inner_shape.SetCurSel( 0 );
		m_combo_bottom_shape.InsertString( PAD_NONE, "none" );
		m_combo_bottom_shape.InsertString( PAD_ROUND, "round" );
		m_combo_bottom_shape.InsertString( PAD_SQUARE, "square" );
		m_combo_bottom_shape.InsertString( PAD_RECT, "rect" );
		m_combo_bottom_shape.InsertString( PAD_RRECT, "rounded-rect" );
		m_combo_bottom_shape.InsertString( PAD_OVAL, "oval" );
		m_combo_bottom_shape.InsertString( PAD_OCTAGON, "octagon" );
		m_combo_bottom_shape.SetCurSel( 1 );
		m_radio_add_pin.SetCheck( 1 );
		OnBnClickedRadioAddPin();
		m_combo_units.AddString( "mil" );
		m_combo_units.AddString( "mm" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		m_combo_row_orient.InsertString( 0, "horiz" );
		m_combo_row_orient.InsertString( 1, "vert" );
		m_combo_row_orient.SetCurSel( 0 );
		m_combo_pad_orient.InsertString( 0, "horiz" );
		m_combo_pad_orient.InsertString( 1, "vert" );
		m_combo_pad_orient.SetCurSel( 0 );
		if( m_fp->GetNumPins() )
		{
			CArray<CString> pin_name;
			pin_name.SetSize( m_fp->GetNumPins() );
			for( int i=0; i<m_fp->GetNumPins(); i++ )
			{
				pin_name[i] = m_fp->m_padstack[i].name;
			}
			::SortByName( &pin_name );
			int ipp = 0; 
			for( int i=0; i<m_fp->GetNumPins(); i++ )
			{
//				str.Format( "%s", m_fp->m_padstack[i].name );
//				m_combo_same_as_pin.AddString( str );
				m_combo_same_as_pin.InsertString( i, pin_name[i] );
				if( m_mode == ADD || pin_name[i] != m_fp->m_padstack[m_pin_num].name )
					m_list_pins.InsertString( ipp++, pin_name[i] );
			}
		} 
		if( m_mode == ADD && m_fp->GetNumPins() )
		{
			// add pin, pins already defined
			m_pin_num = m_fp->GetNumPins();
			str.Format( "%d", m_fp->GetNumPins()+1 );
			m_edit_pin_name.SetWindowText( str );
			m_combo_same_as_pin.SetCurSel( 0 );
			m_check_same_as_pin.SetCheck(1);
			OnBnClickedCheckSameAs();
			m_radio_drag.SetCheck( 1 );
			OnBnClickedRadioDragPin();
		}
		else if( m_mode == ADD )
		{
			// add pin, no pins already defined
			m_pin_num = 0;
			m_edit_pin_name.SetWindowText( "1" );
			m_check_same_as_pin.EnableWindow( FALSE );
			m_combo_same_as_pin.EnableWindow( FALSE );
			m_radio_smt.SetCheck( 1 );
			OnBnClickedRadioSmt();
			m_radio_drag.SetCheck( 1 );
			OnBnClickedRadioDragPin();
		}
		else if( m_mode == EDIT )
		{
			// edit existing pin
			m_edit_pin_name.SetWindowText( m_fp->GetPinNameByIndex( m_pin_num ) );
			m_check_same_as_pin.SetCheck(1);
			m_combo_same_as_pin.SetCurSel( m_pin_num );
			OnBnClickedCheckSameAs();	// this sets dialog to match pin
			m_check_same_as_pin.SetCheck(0);
			OnBnClickedCheckSameAs();
			m_radio_add_pin.SetCheck( 0 );
			m_radio_add_pin.EnableWindow( FALSE );
			m_radio_add_row.EnableWindow( FALSE );
			m_edit_num_pins.EnableWindow( FALSE );
			m_radio_set_pos.SetCheck( 1 );
			OnBnClickedRadioSetPinPos();
			GetFields();
			m_x = m_fp->m_padstack[m_pin_num].x_rel;
			m_y = m_fp->m_padstack[m_pin_num].y_rel;
			m_hole_diam = m_fp->m_padstack[m_pin_num].hole_size;
			m_top_pad_shape = m_fp->m_padstack[m_pin_num].top.shape;
			m_top_pad_width = m_fp->m_padstack[m_pin_num].top.size_h;
			m_top_pad_length = m_fp->m_padstack[m_pin_num].top.size_l*2;
			m_top_pad_radius = m_fp->m_padstack[m_pin_num].top.radius;
			m_inner_pad_shape = m_fp->m_padstack[m_pin_num].inner.shape;
			m_inner_pad_width = m_fp->m_padstack[m_pin_num].inner.size_h;
			m_inner_pad_length = m_fp->m_padstack[m_pin_num].inner.size_l*2;
			m_inner_pad_radius = m_fp->m_padstack[m_pin_num].inner.radius;
			m_bottom_pad_shape = m_fp->m_padstack[m_pin_num].bottom.shape;
			m_bottom_pad_width = m_fp->m_padstack[m_pin_num].bottom.size_h;
			m_bottom_pad_length = m_fp->m_padstack[m_pin_num].bottom.size_l*2;
			m_bottom_pad_radius = m_fp->m_padstack[m_pin_num].bottom.radius;
			SetFields();
			if( m_hole_diam != 0 )
			{
				m_padstack_type = 1;
				m_radio_smt.SetCheck( 0 );
				m_radio_th.SetCheck( 1 );
			}
			else
			{
				if( m_top_pad_shape != PAD_NONE )
				{
					m_padstack_type = 0;
					m_radio_smt.SetCheck( 1 );
					m_radio_smt_bottom.SetCheck( 0 );
					m_radio_th.SetCheck( 0 );
				}
				else
				{
					m_padstack_type = 2;
					m_radio_smt.SetCheck( 0 );
					m_radio_smt_bottom.SetCheck( 1 );
					m_radio_th.SetCheck( 0 );
				}
			}
			if( m_hole_diam
				&& m_top_pad_shape == m_inner_pad_shape 
				&& m_top_pad_width == m_inner_pad_width
				&& m_top_pad_length == m_inner_pad_length
				&& m_top_pad_radius == m_inner_pad_radius )
			{
				m_check_inner_same_as.SetCheck(1);
				OnBnClickedCheckInnerSameAs();
			}
			if( m_hole_diam
				&& m_top_pad_shape == m_bottom_pad_shape 
				&& m_top_pad_width == m_bottom_pad_width
				&& m_top_pad_length == m_bottom_pad_length
				&& m_top_pad_radius == m_bottom_pad_radius )
			{
				m_check_bottom_same_as.SetCheck(1);
				OnBnClickedCheckBottomSameAs();
			}
			if( m_fp->m_padstack[m_pin_num].angle == 90 || m_fp->m_padstack[m_pin_num].angle == 270 )
			{
				m_combo_pad_orient.SetCurSel( 1 );
				m_pad_orient = 1;
			}
			else
			{
				m_combo_pad_orient.SetCurSel( 0 );
				m_pad_orient = 0;
			}
			EnableFields();
		}
	}
	else
	{
		// outgoing
		CString str;
		CString astr;
		CString nstr;
		int n;
		BOOL conflict = FALSE;
		// force updates if "same as top pad" boxes are checked
		OnBnClickedCheckInnerSameAs();
		OnBnClickedCheckBottomSameAs();
		// update fields
		GetFields();
		// check for legal pin name
		if( !CheckLegalPinName( &m_pin_name, &astr, &nstr, &n ) )
		{
			str = "Pin name must consist of zero or more letters\n";
			str	+= "Followed by zero or more numbers\n";
			str	+= "The characters \" .,;:/!@#$%^&*(){}[]|<>?\\~\'\" are illegal\n";
			str	+= "For example: 1, 23, A12, SOURCE are legal\n";
			str	+= "while 1A, A2B3, A:3 are not\n";
			AfxMessageBox( str );
			pDX->Fail();
		}
		if( m_mode == ADD )
		{
			if( m_num_pins > 1 && m_row_spacing == 0 )
			{
				AfxMessageBox( "illegal row spacing" );
				pDX->Fail();
			}
		}
		if( m_padstack_type == 1 && m_hole_diam <= 0 )
		{
			AfxMessageBox( "For through-hole pin, hole diameter must be > 0" );
			pDX->Fail();
		}
		if(  (m_padstack_type == 0 || m_padstack_type == 1) && m_top_pad_shape != PAD_NONE && m_top_pad_width <= 0 )
		{
			AfxMessageBox( "Illegal top pad width" );
			pDX->Fail();
		}
		if( m_padstack_type == 1 && m_inner_pad_shape != PAD_NONE && m_inner_pad_width <= 0 )
		{
			AfxMessageBox( "Illegal inner pad width" );
			pDX->Fail();
		}
		if( (m_padstack_type == 2 || m_padstack_type == 1) && m_bottom_pad_shape != PAD_NONE && m_bottom_pad_width <= 0 )
		{
			AfxMessageBox( "Illegal bottom pad width" );
			pDX->Fail();
		}
		// now check for conflicts
		if( nstr == "" )
		{
			// pin name is pure alphabetic string
			// can't make row
			if( m_num_pins > 1 )
			{
				AfxMessageBox( "To create a row of pins, the pin name must end in a number" );
				pDX->Fail();
			}
			// check for conflicts
			int npins = m_fp->m_padstack.GetSize();
			for( int i=0; i<npins; i++ )
			{
				if( astr == m_fp->m_padstack[i].name )
				{
					if( (m_mode == EDIT && i != m_pin_num) || m_mode == ADD )
					{
						conflict = TRUE;
						break;
					}
				}
			}
			if( conflict )
			{
				str.Format( "Pin name \"%s\" is already in use", m_pin_name );
				pDX->Fail();
			}
		}
		else
		{
			// pin name ends in a number, allow insertion
			int npins = m_fp->m_padstack.GetSize();
			for( int ip=0; ip<m_num_pins; ip++ )
			{
				CString pin_name;
				pin_name.Format( "%s%d", astr, n+ip );
				for( int i=0; i<npins; i++ )
				{
					if( pin_name == m_fp->m_padstack[i].name )
					{
						if( (m_mode == EDIT && i != (m_pin_num)) || m_mode == ADD )
						{
							conflict = TRUE;
							break;
						}
					}
				}
			}
			if( conflict )
			{
				if( m_num_pins == 1 )
					str.Format( "Pin name \"%s\" conflicts with an existing pin\nShift the existing pin name up?", m_pin_name );
				else
					str.Format( "Pin names \"%s%d\" through \"%s%d\" conflict with existing pins\nShift existing pin names up?", 
					astr, n, astr, n+m_num_pins-1 );
				int ret = AfxMessageBox( str, MB_OKCANCEL );
				if( ret != IDOK )
					pDX->Fail();
			}
		}
		// OK, we are ready to go
		m_fp->Undraw();
		// check if we will be dragging
		m_drag_flag = m_radio_drag.GetCheck();
		// now insert padstacks
		padstack ps;
		ps.hole_size = m_hole_diam;
		if( m_padstack_type == 0 || m_padstack_type == 2 )
			ps.hole_size = 0;
		ps.angle = 0;
		if( m_pad_orient == 1 )
			ps.angle = 90;
		if( m_padstack_type == 0 )
		{
			m_inner_pad_shape = PAD_NONE;
			m_bottom_pad_shape = PAD_NONE;
		}
		if( m_padstack_type == 2 )
		{
			m_top_pad_shape = PAD_NONE;
			m_inner_pad_shape = PAD_NONE;
		}
		ps.top.shape = m_top_pad_shape;
		ps.top.size_h = m_top_pad_width;
		ps.top.size_l = m_top_pad_length/2; 
		ps.top.size_r = m_top_pad_length/2;
		ps.top.radius = m_top_pad_radius;
		ps.inner.shape = m_inner_pad_shape;
		ps.inner.size_h = m_inner_pad_width;
		ps.inner.size_l = m_inner_pad_length/2; 
		ps.inner.size_r = m_inner_pad_length/2;
		ps.inner.radius = m_inner_pad_radius;
		ps.bottom.shape = m_bottom_pad_shape;
		ps.bottom.size_h = m_bottom_pad_width;
		ps.bottom.size_l = m_bottom_pad_length/2; 
		ps.bottom.size_r = m_bottom_pad_length/2;
		ps.bottom.radius = m_bottom_pad_radius;
		ps.x_rel = m_x;
		ps.y_rel = m_y;
		// apply to other pins if requested
		for( int ip=0; ip<m_list_pins.GetCount(); ip++ )
		{
			if( m_list_pins.GetSel( ip ) )
			{
				m_list_pins.GetText( ip, str );
				int i = m_fp->GetPinIndexByName( &str );
				if( i == -1 )
					ASSERT(0);
				else
				{
					m_fp->m_padstack[i].hole_size = ps.hole_size;
					m_fp->m_padstack[i].top = ps.top;
					m_fp->m_padstack[i].inner = ps.inner;
					m_fp->m_padstack[i].bottom = ps.bottom;
				}
			}
		}
		// add or replace selected pin
		if( m_mode == EDIT )
		{
			// remove pin
			m_fp->m_padstack.RemoveAt(m_pin_num);
		}
		// add pins to footprint
		int dx = 0;
		int dy = 0;
		if( m_row_orient == 0 )
			dx = m_row_spacing;
		else
			dy = m_row_spacing;
		for( int ip=0; ip<m_num_pins; ip++ )
		{
			ps.name.Format( "%s%d", astr, n+ip );
			m_fp->ShiftToInsertPadName( &astr, n+ip );
			m_fp->m_padstack.InsertAt(  m_pin_num+ip, ps );
			ps.x_rel += dx;
			ps.y_rel += dy;
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgAddPin, CDialog)
	ON_BN_CLICKED(IDC_RADIO_ADD_PIN, OnBnClickedRadioAddPin)
	ON_BN_CLICKED(IDC_RADIO_ADD_ROW, OnBnClickedRadioAddRow)
	ON_BN_CLICKED(IDC_CHECK_SAME_AS, OnBnClickedCheckSameAs)
	ON_BN_CLICKED(IDC_RADIO_SMT, OnBnClickedRadioSmt)
	ON_BN_CLICKED(IDC_RADIO_SMT_BOTTOM, OnBnClickedRadioSmtBottom)
	ON_BN_CLICKED(IDC_RADIO_TH, OnBnClickedRadioTh)
	ON_BN_CLICKED(IDC_RADIO_DRAG_PIN, OnBnClickedRadioDragPin)
	ON_BN_CLICKED(IDC_RADIO_SET_PIN_POS, OnBnClickedRadioSetPinPos)
	ON_BN_CLICKED(IDC_CHECK_INNER_SAME_AS, OnBnClickedCheckInnerSameAs)
	ON_BN_CLICKED(IDC_CHECK_BOTTOM_SAME_AS, OnBnClickedCheckBottomSameAs)
	ON_CBN_SELCHANGE(IDC_COMBO_PIN_UNITS, OnCbnSelchangeComboPinUnits)
	ON_CBN_SELCHANGE(IDC_COMBO_ROW_ORIENT, OnCbnSelchangeComboRowOrient)
	ON_CBN_SELCHANGE(IDC_COMBO_TOP_PAD_SHAPE, OnCbnSelchangeComboTopPadShape)
	ON_CBN_SELCHANGE(IDC_COMBO_INNER_PAD_SHAPE, OnCbnSelchangeComboInnerPadShape)
	ON_CBN_SELCHANGE(IDC_COMBO_BOTTOM_PAD_SHAPE, OnCbnSelchangeComboBottomPadShape)
	ON_CBN_SELCHANGE(IDC_COMBO_SAME_AS_PIN, OnCbnSelchangeComboSameAsPin)
	ON_CBN_SELCHANGE(IDC_COMBO_PAD_ORIENT, OnCbnSelchangeComboPadOrient)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_W, OnEnChangeEditTopPadW)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_L, OnEnChangeEditTopPadL)
	ON_EN_CHANGE(IDC_EDIT_TOP_PAD_RAD, OnEnChangeEditTopPadRadius)
END_MESSAGE_MAP()


void CDlgAddPin::InitDialog( CEditShape * fp, int mode, int pin_num, int units )
{
	m_units = units;
	m_mode = mode;
	if( m_mode == EDIT)
		m_pin_num = pin_num;
	if( fp )
		m_fp = fp;
	else
		ASSERT(0);
	return;
}

// CDlgAddPin message handlers

void CDlgAddPin::OnBnClickedRadioAddPin()
{
	CString str;
	m_radio_add_pin.SetCheck( 1 );
	m_edit_num_pins.EnableWindow( FALSE );
	m_edit_num_pins.SetWindowText( "1" );
	m_combo_row_orient.EnableWindow( FALSE );
	m_edit_row_spacing.EnableWindow( FALSE );
}

void CDlgAddPin::OnBnClickedRadioAddRow()
{
	CString str;
	m_edit_num_pins.EnableWindow( TRUE );
	m_edit_num_pins.SetWindowText( "1" );
	m_combo_row_orient.EnableWindow( TRUE );
	m_edit_row_spacing.EnableWindow( TRUE );
}

void CDlgAddPin::OnBnClickedCheckSameAs()
{
	EnableFields();
}

void CDlgAddPin::OnBnClickedRadioSmt()
{
	EnableFields();
}


void CDlgAddPin::OnBnClickedRadioSmtBottom()
{
	EnableFields();
}


void CDlgAddPin::OnBnClickedRadioTh()
{
	EnableFields();
}

void CDlgAddPin::OnBnClickedRadioDragPin()
{
	m_edit_pin_x.EnableWindow( FALSE );
	m_edit_pin_y.EnableWindow( FALSE );
}

void CDlgAddPin::OnBnClickedRadioSetPinPos()
{
	m_edit_pin_x.EnableWindow( TRUE );
	m_edit_pin_y.EnableWindow( TRUE );
}

void CDlgAddPin::OnBnClickedCheckInnerSameAs()
{
	if( m_check_inner_same_as.GetCheck() )
	{
		m_top_pad_shape = m_combo_top_shape.GetCurSel();
		m_combo_inner_shape.SetCurSel( m_top_pad_shape );
		m_combo_inner_shape.EnableWindow( FALSE );
		m_edit_inner_width.EnableWindow( FALSE );
		m_edit_inner_length.EnableWindow( FALSE );
		m_edit_inner_radius.EnableWindow( FALSE );
		CString str;
		m_edit_top_width.GetWindowText( str );
		m_edit_inner_width.SetWindowText( str );
		m_edit_top_length.GetWindowText( str );
		m_edit_inner_length.SetWindowText( str );
	}
	else
	{
		m_combo_inner_shape.EnableWindow( TRUE );
		m_edit_inner_width.EnableWindow( TRUE );
		m_edit_inner_length.EnableWindow( TRUE );
		m_edit_inner_radius.EnableWindow( TRUE );
		OnCbnSelchangeComboInnerPadShape();
	}
}

void CDlgAddPin::OnBnClickedCheckBottomSameAs()
{
	if( m_check_bottom_same_as.GetCheck() )
	{
		m_top_pad_shape = m_combo_top_shape.GetCurSel();
		m_combo_bottom_shape.SetCurSel( m_top_pad_shape );
		m_combo_bottom_shape.EnableWindow( FALSE );
		m_edit_bottom_width.EnableWindow( FALSE );
		m_edit_bottom_length.EnableWindow( FALSE );
		m_edit_bottom_radius.EnableWindow( FALSE );
		CString str;
		m_edit_top_width.GetWindowText( str );
		m_edit_bottom_width.SetWindowText( str );
		m_edit_top_length.GetWindowText( str );
		m_edit_bottom_length.SetWindowText( str );
		m_edit_top_radius.GetWindowText( str );
		m_edit_bottom_radius.SetWindowText( str );
	}
	else
	{
		m_combo_bottom_shape.EnableWindow( TRUE );
		m_edit_bottom_width.EnableWindow( TRUE );
		m_edit_bottom_length.EnableWindow( TRUE );
		m_edit_bottom_radius.EnableWindow( TRUE );
		OnCbnSelchangeComboBottomPadShape();
	}
}

void CDlgAddPin::OnCbnSelchangeComboPinUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void CDlgAddPin::OnCbnSelchangeComboRowOrient()
{
	m_row_orient = m_combo_row_orient.GetCurSel();
}

void CDlgAddPin::OnCbnSelchangeComboTopPadShape()
{
	m_top_pad_shape = m_combo_top_shape.GetCurSel();
	if( m_top_pad_shape < PAD_RECT || m_top_pad_shape == PAD_OCTAGON )
		m_edit_top_length.EnableWindow( FALSE );
	else
		m_edit_top_length.EnableWindow( TRUE );
	if( m_top_pad_shape == PAD_NONE )
		m_edit_top_width.EnableWindow( FALSE );
	else
		m_edit_top_width.EnableWindow( TRUE );
	if( m_top_pad_shape == PAD_RRECT )
		m_edit_top_radius.EnableWindow( TRUE );
	else
		m_edit_top_radius.EnableWindow( FALSE );
	if( m_check_inner_same_as.GetCheck() )
	{
		m_inner_pad_shape = m_top_pad_shape;
		m_combo_inner_shape.SetCurSel( m_inner_pad_shape );
	}
	if( m_check_bottom_same_as.GetCheck() )
	{
		m_bottom_pad_shape = m_top_pad_shape;
		m_combo_bottom_shape.SetCurSel( m_bottom_pad_shape );
	}
}

void CDlgAddPin::OnCbnSelchangeComboInnerPadShape()
{
	m_inner_pad_shape = m_combo_inner_shape.GetCurSel();
	if( m_inner_pad_shape < PAD_RECT || m_inner_pad_shape == PAD_OCTAGON )
		m_edit_inner_length.EnableWindow( FALSE );
	else
		m_edit_inner_length.EnableWindow( TRUE );
	if( m_inner_pad_shape == PAD_NONE )
		m_edit_inner_width.EnableWindow( FALSE );
	else
		m_edit_inner_width.EnableWindow( TRUE );
	if( m_inner_pad_shape == PAD_RRECT )
		m_edit_inner_radius.EnableWindow( TRUE );
	else
		m_edit_inner_radius.EnableWindow( FALSE );
}

void CDlgAddPin::OnCbnSelchangeComboBottomPadShape()
{
	m_bottom_pad_shape = m_combo_bottom_shape.GetCurSel();
	if( m_bottom_pad_shape < PAD_RECT  || m_bottom_pad_shape == PAD_OCTAGON)
		m_edit_bottom_length.EnableWindow( FALSE );
	else
		m_edit_bottom_length.EnableWindow( TRUE );
	if( m_bottom_pad_shape == PAD_NONE )
		m_edit_bottom_width.EnableWindow( FALSE );
	else
		m_edit_bottom_width.EnableWindow( TRUE );
	if( m_bottom_pad_shape == PAD_RRECT )
		m_edit_bottom_radius.EnableWindow( TRUE );
	else
		m_edit_bottom_radius.EnableWindow( FALSE );
}

void CDlgAddPin::SetFields()
{
	CString str;

	m_combo_top_shape.SetCurSel( m_top_pad_shape );
	m_combo_inner_shape.SetCurSel( m_inner_pad_shape );
	m_combo_bottom_shape.SetCurSel( m_bottom_pad_shape );
	if( m_units == MIL )
	{
		str.Format( "%d", m_hole_diam/NM_PER_MIL );
		m_edit_hole_diam.SetWindowText( str );

		str.Format( "%d", m_top_pad_width/NM_PER_MIL );
		m_edit_top_width.SetWindowText( str );
		str.Format( "%d", m_top_pad_length/NM_PER_MIL );
		m_edit_top_length.SetWindowText( str );
		str.Format( "%d", m_top_pad_radius/NM_PER_MIL );
		m_edit_top_radius.SetWindowText( str );

		str.Format( "%d", m_inner_pad_width/NM_PER_MIL );
		m_edit_inner_width.SetWindowText( str );
		str.Format( "%d", m_inner_pad_length/NM_PER_MIL );
		m_edit_inner_length.SetWindowText( str );
		str.Format( "%d", m_inner_pad_radius/NM_PER_MIL );
		m_edit_inner_radius.SetWindowText( str );

		str.Format( "%d", m_bottom_pad_width/NM_PER_MIL );
		m_edit_bottom_width.SetWindowText( str );
		str.Format( "%d", m_bottom_pad_length/NM_PER_MIL );
		m_edit_bottom_length.SetWindowText( str );
		str.Format( "%d", m_bottom_pad_radius/NM_PER_MIL );
		m_edit_bottom_radius.SetWindowText( str );

		str.Format( "%d", m_x/NM_PER_MIL );
		m_edit_pin_x.SetWindowText( str );
		str.Format( "%d", m_y/NM_PER_MIL );
		m_edit_pin_y.SetWindowText( str );

		str.Format( "%d", m_row_spacing/NM_PER_MIL );
		m_edit_row_spacing.SetWindowText( str );

	}
	else if( m_units == MM )
	{
		str.Format( "%7.4f", m_hole_diam/1000000.0 );
		m_edit_hole_diam.SetWindowText( str );

		str.Format( "%7.4f", m_top_pad_width/1000000.0 );
		m_edit_top_width.SetWindowText( str );
		str.Format( "%7.4f", m_top_pad_length/1000000.0 );
		m_edit_top_length.SetWindowText( str );
		str.Format( "%7.4f", m_top_pad_radius/1000000.0 );
		m_edit_top_radius.SetWindowText( str );

		str.Format( "%7.4f", m_inner_pad_width/1000000.0 );
		m_edit_inner_width.SetWindowText( str );
		str.Format( "%7.4f", m_inner_pad_length/1000000.0 );
		m_edit_inner_length.SetWindowText( str );
		str.Format( "%7.4f", m_inner_pad_radius/1000000.0 );
		m_edit_inner_radius.SetWindowText( str );

		str.Format( "%7.4f", m_bottom_pad_width/1000000.0 );
		m_edit_bottom_width.SetWindowText( str );
		str.Format( "%7.4f", m_bottom_pad_length/1000000.0 );
		m_edit_bottom_length.SetWindowText( str );
		str.Format( "%7.4f", m_bottom_pad_radius/1000000.0 );
		m_edit_bottom_radius.SetWindowText( str );

		str.Format( "%7.4f", m_x/1000000.0 );
		m_edit_pin_x.SetWindowText( str );
		str.Format( "%7.4f", m_y/1000000.0 );
		m_edit_pin_y.SetWindowText( str );

		str.Format( "%7.4f", m_row_spacing/1000000.0 );
		m_edit_row_spacing.SetWindowText( str );
	}
	else
		ASSERT(0);
}

void CDlgAddPin::GetFields()
{
	CString str;
	int val;

	// get pin number and name
	m_edit_pin_name.GetWindowText( m_pin_name );
	if( m_units == MIL )
	{
		m_edit_hole_diam.GetWindowText( str );
		m_hole_diam = NM_PER_MIL*atoi( str );

		m_edit_top_width.GetWindowText( str );
		m_top_pad_width = NM_PER_MIL*atoi( str );

		m_edit_top_length.GetWindowText( str );
		m_top_pad_length = NM_PER_MIL*atoi( str );

		m_edit_top_radius.GetWindowText( str );
		m_top_pad_radius = NM_PER_MIL*atoi( str );

		m_edit_inner_width.GetWindowText( str );
		m_inner_pad_width = NM_PER_MIL*atoi( str );

		m_edit_inner_length.GetWindowText( str );
		m_inner_pad_length = NM_PER_MIL*atoi( str );

		m_edit_inner_radius.GetWindowText( str );
		m_inner_pad_radius = NM_PER_MIL*atoi( str );

		m_edit_bottom_width.GetWindowText( str );
		m_bottom_pad_width = NM_PER_MIL*atoi( str );

		m_edit_bottom_length.GetWindowText( str );
		m_bottom_pad_length = NM_PER_MIL*atoi( str );

		m_edit_bottom_radius.GetWindowText( str );
		m_bottom_pad_radius = NM_PER_MIL*atoi( str );

		m_edit_pin_x.GetWindowText( str );
		m_x = NM_PER_MIL*atoi( str );

		m_edit_pin_y.GetWindowText( str );
		m_y = NM_PER_MIL*atoi( str );

		m_edit_row_spacing.GetWindowText( str );
		m_row_spacing = NM_PER_MIL*atoi( str );
	}
	else if( m_units == MM )
	{
		m_edit_hole_diam.GetWindowText( str );
		m_hole_diam = 1000000.0*atof( str );

		m_edit_top_width.GetWindowText( str );
		m_top_pad_width = 1000000.0*atof( str );

		m_edit_top_length.GetWindowText( str );
		m_top_pad_length = 1000000.0*atof( str );

		m_edit_top_radius.GetWindowText( str );
		m_top_pad_radius = 1000000.0*atof( str );

		m_edit_inner_width.GetWindowText( str );
		m_inner_pad_width = 1000000.0*atof( str );

		m_edit_inner_length.GetWindowText( str );
		m_inner_pad_length = 1000000.0*atof( str );

		m_edit_inner_radius.GetWindowText( str );
		m_inner_pad_radius = 1000000.0*atof( str );

		m_edit_bottom_width.GetWindowText( str );
		m_bottom_pad_width = 1000000.0*atof( str );

		m_edit_bottom_length.GetWindowText( str );
		m_bottom_pad_length = 1000000.0*atof( str );

		m_edit_bottom_radius.GetWindowText( str );
		m_bottom_pad_radius = 1000000.0*atof( str );

		m_edit_pin_x.GetWindowText( str );
		m_x = 1000000.0*atof( str );

		m_edit_pin_y.GetWindowText( str );
		m_y = 1000000.0*atof( str );

		m_edit_row_spacing.GetWindowText( str );
		m_row_spacing = 1000000.0*atof( str );
	}
	else
		ASSERT(0);
	// get number of pins to add
	if( m_radio_add_pin.GetCheck() )
		m_num_pins = 1;
	else
	{
		m_edit_num_pins.GetWindowText( str );
		m_num_pins = atoi( str );
	}
	// get pad shapes
	m_top_pad_shape = m_combo_top_shape.GetCurSel();
	m_inner_pad_shape = m_combo_inner_shape.GetCurSel();
	m_bottom_pad_shape = m_combo_bottom_shape.GetCurSel();
	// get pad and row orientation
	m_pad_orient = m_combo_pad_orient.GetCurSel();
	m_row_orient = m_combo_row_orient.GetCurSel();
}

void CDlgAddPin::OnCbnSelchangeComboSameAsPin()
{
	OnBnClickedCheckSameAs();
}

void CDlgAddPin::OnCbnSelchangeComboPadOrient()
{
	// TODO: Add your control notification handler code here
}

void CDlgAddPin::OnEnChangeEditTopPadW()
{
	if( m_check_inner_same_as.GetCheck() )
	{
		CString str;
		m_edit_top_width.GetWindowText( str );
		m_edit_inner_width.SetWindowText( str );
	}
	if( m_check_bottom_same_as.GetCheck() )
	{
		CString str;
		m_edit_top_width.GetWindowText( str );
		m_edit_bottom_width.SetWindowText( str );
	}
}

void CDlgAddPin::OnEnChangeEditTopPadL()
{
	if( m_check_inner_same_as.GetCheck() )
	{
		CString str;
		m_edit_top_length.GetWindowText( str );
		m_edit_inner_length.SetWindowText( str );
	}
	if( m_check_bottom_same_as.GetCheck() )
	{
		CString str;
		m_edit_top_length.GetWindowText( str );
		m_edit_bottom_length.SetWindowText( str );
	}
}

void CDlgAddPin::OnEnChangeEditTopPadRadius()
{
	if( m_check_inner_same_as.GetCheck() )
	{
		CString str;
		m_edit_top_radius.GetWindowText( str );
		m_edit_inner_radius.SetWindowText( str );
	}
	if( m_check_bottom_same_as.GetCheck() )
	{
		CString str;
		m_edit_top_radius.GetWindowText( str );
		m_edit_bottom_radius.SetWindowText( str );
	}
}

void CDlgAddPin::EnableFields()
{
	// check "same as pin" box, if set we can disable most other fields
	m_same_as_pin_flag = m_check_same_as_pin.GetCheck(); 
	if( m_same_as_pin_flag )
	{
		// same as another pin, set fields
		m_combo_same_as_pin.EnableWindow( TRUE );
		m_radio_smt.EnableWindow( FALSE );
		m_radio_smt_bottom.EnableWindow( FALSE );
		m_radio_th.EnableWindow( FALSE );
		m_combo_pad_orient.EnableWindow( FALSE );
		m_combo_top_shape.EnableWindow( FALSE );
		m_edit_top_width.EnableWindow( FALSE );
		m_edit_top_length.EnableWindow( FALSE );
		m_edit_top_radius.EnableWindow( FALSE );
		m_check_inner_same_as.EnableWindow( FALSE );
		m_check_inner_same_as.SetCheck( 0 );
		m_combo_inner_shape.EnableWindow( FALSE );
		m_edit_inner_width.EnableWindow( FALSE );
		m_edit_inner_length.EnableWindow( FALSE );
		m_edit_inner_radius.EnableWindow( FALSE );
		m_check_bottom_same_as.EnableWindow( FALSE );
		m_check_bottom_same_as.SetCheck( 0 );
		m_combo_bottom_shape.EnableWindow( FALSE );
		m_edit_bottom_width.EnableWindow( FALSE );
		m_edit_bottom_length.EnableWindow( FALSE );
		m_edit_bottom_radius.EnableWindow( FALSE );
		m_edit_hole_diam.EnableWindow( FALSE );
		m_check_inner_same_as.EnableWindow( FALSE );
		m_check_bottom_same_as.EnableWindow( FALSE );

		// now set padstack params
		GetFields();
		CString pin_name;
		m_combo_same_as_pin.GetWindowText( pin_name );
		int ip = m_fp->GetPinIndexByName( &pin_name );
		m_pad_orient = 0;
		if( m_fp->m_padstack[ip].angle == 90 || m_fp->m_padstack[ip].angle == 270 )
			m_pad_orient = 1;
		m_hole_diam = m_fp->m_padstack[ip].hole_size;
		m_top_pad_shape = m_fp->m_padstack[ip].top.shape;
		m_top_pad_width = m_fp->m_padstack[ip].top.size_h;
		m_top_pad_length = m_fp->m_padstack[ip].top.size_l*2;
		m_top_pad_radius = m_fp->m_padstack[ip].top.radius;
		m_inner_pad_shape = m_fp->m_padstack[ip].inner.shape;
		m_inner_pad_width = m_fp->m_padstack[ip].inner.size_h;
		m_inner_pad_length = m_fp->m_padstack[ip].inner.size_l*2;
		m_inner_pad_radius = m_fp->m_padstack[ip].inner.radius;
		m_bottom_pad_shape = m_fp->m_padstack[ip].bottom.shape;
		m_bottom_pad_width = m_fp->m_padstack[ip].bottom.size_h;
		m_bottom_pad_length = m_fp->m_padstack[ip].bottom.size_l*2;
		m_bottom_pad_radius = m_fp->m_padstack[ip].bottom.radius;
		if( m_hole_diam != 0 )
		{
			m_padstack_type = 1;
			m_radio_smt.SetCheck( 0 );
			m_radio_th.SetCheck( 1 );
		}
		else
		{
			if( m_top_pad_shape != PAD_NONE )
			{
				m_padstack_type = 0;
				m_radio_smt.SetCheck( 1 );
				m_radio_smt_bottom.SetCheck( 0 );
				m_radio_th.SetCheck( 0 );
			}
			else
			{
				m_padstack_type = 2;
				m_radio_smt.SetCheck( 0 );
				m_radio_smt_bottom.SetCheck( 1 );
				m_radio_th.SetCheck( 0 );
			}
		}
		m_combo_pad_orient.SetCurSel( m_pad_orient );
		m_combo_top_shape.SetCurSel( m_top_pad_shape );
		m_combo_inner_shape.SetCurSel( m_inner_pad_shape );
		m_combo_bottom_shape.SetCurSel( m_bottom_pad_shape );
		SetFields();
	}
	else
	{
		// not same as another pin, enable fields
		m_combo_same_as_pin.EnableWindow( FALSE );
		m_radio_smt.EnableWindow( TRUE );
		m_radio_smt_bottom.EnableWindow( TRUE );
		m_radio_th.EnableWindow( TRUE );
		m_combo_pad_orient.EnableWindow( TRUE );
		m_combo_top_shape.EnableWindow( TRUE );
		m_edit_top_width.EnableWindow( TRUE );
		m_edit_top_length.EnableWindow( TRUE );
		m_edit_top_radius.EnableWindow( TRUE );
		if( m_radio_smt.GetCheck() )
		{
			// SMT
			m_combo_top_shape.EnableWindow( TRUE );
			m_edit_top_width.EnableWindow( TRUE );
			m_edit_top_length.EnableWindow( TRUE );
			m_edit_top_radius.EnableWindow( TRUE );

			m_combo_inner_shape.EnableWindow( FALSE );
			m_edit_inner_width.EnableWindow( FALSE );
			m_edit_inner_length.EnableWindow( FALSE );
			m_edit_inner_radius.EnableWindow( FALSE );
			m_check_inner_same_as.EnableWindow( FALSE );

			m_combo_bottom_shape.EnableWindow( FALSE );
			m_edit_bottom_width.EnableWindow( FALSE );
			m_edit_bottom_length.EnableWindow( FALSE );
			m_edit_bottom_radius.EnableWindow( FALSE );
			m_check_bottom_same_as.EnableWindow( FALSE );

			m_edit_hole_diam.EnableWindow( FALSE );
			OnCbnSelchangeComboTopPadShape();
			m_padstack_type = 0;
		}
		else if( m_radio_smt_bottom.GetCheck() )
		{
			// SMT on bottom
			m_combo_top_shape.EnableWindow( FALSE );
			m_edit_top_width.EnableWindow( FALSE );
			m_edit_top_length.EnableWindow( FALSE );
			m_edit_top_radius.EnableWindow( FALSE );

			m_combo_inner_shape.EnableWindow( FALSE );
			m_edit_inner_width.EnableWindow( FALSE );
			m_edit_inner_length.EnableWindow( FALSE );
			m_edit_inner_radius.EnableWindow( FALSE );
			m_check_inner_same_as.EnableWindow( FALSE );

			m_combo_bottom_shape.EnableWindow( TRUE );
			m_edit_bottom_width.EnableWindow( TRUE );
			m_edit_bottom_length.EnableWindow( TRUE );
			m_edit_bottom_radius.EnableWindow( TRUE );
			m_check_bottom_same_as.EnableWindow( FALSE );
			m_check_bottom_same_as.SetCheck(0);

			m_edit_hole_diam.EnableWindow( FALSE );
			OnCbnSelchangeComboBottomPadShape();
			m_padstack_type = 2;
		}
		else
		{
			// Through-hole
			m_combo_top_shape.EnableWindow( TRUE );
			m_edit_top_width.EnableWindow( TRUE );
			m_edit_top_length.EnableWindow( TRUE );
			m_edit_top_radius.EnableWindow( TRUE );

			m_check_inner_same_as.EnableWindow( TRUE );
			m_combo_inner_shape.EnableWindow( TRUE );
			m_edit_inner_width.EnableWindow( TRUE );
			m_edit_inner_length.EnableWindow( TRUE );
			m_edit_inner_radius.EnableWindow( TRUE );
			m_check_inner_same_as.EnableWindow( TRUE );

			m_check_bottom_same_as.EnableWindow( TRUE );
			m_combo_bottom_shape.EnableWindow( TRUE );
			m_edit_bottom_width.EnableWindow( TRUE );
			m_edit_bottom_length.EnableWindow( TRUE );
			m_edit_bottom_radius.EnableWindow( TRUE ); 
			m_check_bottom_same_as.EnableWindow( TRUE );
			OnBnClickedCheckInnerSameAs();
			OnBnClickedCheckBottomSameAs();
			OnCbnSelchangeComboTopPadShape();
			m_edit_hole_diam.EnableWindow( TRUE );
			if( !m_check_inner_same_as.GetCheck() )
				OnCbnSelchangeComboInnerPadShape();
			if( !m_check_bottom_same_as.GetCheck() )
				OnCbnSelchangeComboBottomPadShape();
			m_padstack_type = 1;
		}
	}
}
