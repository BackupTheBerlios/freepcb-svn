// DlgCAD.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgCAD.h"
#include "Gerber.h"
#include "DlgLog.h"
#include "DlgMyMessageBox2.h"

// CDlgCAD dialog

IMPLEMENT_DYNAMIC(CDlgCAD, CDialog)
CDlgCAD::CDlgCAD(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgCAD::IDD, pParent)
{
	m_dlg_log = NULL;
	m_folder = "";
}

CDlgCAD::~CDlgCAD()
{
	if( m_dlg_log )
	{
		m_dlg_log->DestroyWindow();
		delete m_dlg_log;
	}
}
 
void CDlgCAD::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_FOLDER, m_edit_folder);
	DDX_Control(pDX, IDC_EDIT_FILL, m_edit_fill);
	DDX_Control(pDX, IDC_EDIT_MASK, m_edit_mask);
	DDX_Control(pDX, IDC_CHECK_DRILL, m_check_drill);
	DDX_Control(pDX, IDC_CHECK_TOP_SILK, m_check_top_silk);
	DDX_Control(pDX, IDC_CHECK_BOTTOM_SILK, m_check_bottom_silk);
	DDX_Control(pDX, IDC_CHECK_TOP_SOLDER, m_check_top_solder);
	DDX_Control(pDX, IDC_CHECKBOTTOM_SOLDER_MASK, m_check_bottom_solder);
	DDX_Control(pDX, IDC_CHECK_TOP_COPPER, m_check_top_copper);
	DDX_Control(pDX, IDC_CHECK_BOTTOM_COPPER, m_check_bottom_copper);
	DDX_Control(pDX, IDC_CHECK_INNER1, m_check_inner1);
	DDX_Control(pDX, IDC_CHECK_INNER2, m_check_inner2);
	DDX_Control(pDX, IDC_CHECK_INNER3, m_check_inner3);
	DDX_Control(pDX, IDC_CHECK_INNER4, m_check_inner4);
	DDX_Control(pDX, IDC_CHECK_INNER5, m_check_inner5);
	DDX_Control(pDX, IDC_CHECK_INNER_6, m_check_inner6);
	DDX_Control(pDX, IDC_BOARD_OUTLINE, m_check_outline);
	DDX_Control(pDX, IDC_MOIRES, m_check_moires);
	DDX_Control(pDX, IDC_LAYER_DESC, m_check_layer_text);
	DDX_Control(pDX, IDC_COMBO_CAD_UNITS, m_combo_units);
	DDX_Control(pDX, IDC_EDIT_CAD_PILOT_DIAM, m_edit_pilot_diam);
	DDX_Control(pDX, IDC_CHECK_CAD_PILOT, m_check_pilot);
	DDX_Control(pDX, IDC_EDIT_CAD_MIN_SSW, m_edit_min_ss_w);
	DDX_Control(pDX, IDC_EDIT_CAD_THERMALWID, m_edit_thermal_width);
	DDX_Text( pDX, IDC_EDIT_FOLDER, m_folder );
	DDX_Control(pDX, IDC_EDIT_CAD_OUTLINE_WID, m_edit_outline_width);
	DDX_Control(pDX, IDC_EDIT_CAD_HOLE_CLEARANCE, m_edit_hole_clearance);
	DDX_Control(pDX, IDC_EDIT_CAD_ANN_PINS, m_edit_ann_pins);
	DDX_Control(pDX, IDC_EDIT_CAD_ANN_VIAS, m_edit_ann_vias);
	DDX_Control(pDX, IDC_CHECK1, m_check_thermal_pins);
	DDX_Control(pDX, IDC_CHECK2, m_check_thermal_vias);
	DDX_Control(pDX, IDC_CHECK3, m_check_mask_vias);
	if( !pDX->m_bSaveAndValidate )
	{
		// entry
		m_combo_units.InsertString( 0, "MIL" );
		m_combo_units.InsertString( 1, "MM" );
		if( m_units == MIL )
			m_combo_units.SetCurSel( 0 );
		else
			m_combo_units.SetCurSel( 1 );
		SetFields();
		// enable checkboxes for valid gerber layers
		if( m_num_copper_layers < 2 )
			m_check_bottom_copper.EnableWindow( FALSE );
		if( m_num_copper_layers < 3 )
			m_check_inner1.EnableWindow( FALSE );
		if( m_num_copper_layers < 4 )
			m_check_inner2.EnableWindow( FALSE );
		if( m_num_copper_layers < 5 )
			m_check_inner3.EnableWindow( FALSE );
		if( m_num_copper_layers < 6 )
			m_check_inner4.EnableWindow( FALSE );
		if( m_num_copper_layers < 7 )
			m_check_inner5.EnableWindow( FALSE );
		if( m_num_copper_layers < 8 )
			m_check_inner6.EnableWindow( FALSE );

		// load saved settings
		SetFields();

		for( int i=0; i<(4+m_num_copper_layers); i++ )
		{
			switch(i)
			{
			case 0: m_check_top_silk.SetCheck(m_layers & (1<<i)); break;
			case 1: m_check_bottom_silk.SetCheck(m_layers & (1<<i)); break;
			case 2: m_check_top_solder.SetCheck(m_layers & (1<<i)); break;
			case 3: m_check_bottom_solder.SetCheck(m_layers & (1<<i)); break;
			case 4: m_check_top_copper.SetCheck(m_layers & (1<<i)); break;
			case 5: m_check_bottom_copper.SetCheck(m_layers & (1<<i)); break;
			case 6: m_check_inner1.SetCheck(m_layers & (1<<i)); break;
			case 7: m_check_inner2.SetCheck(m_layers & (1<<i)); break; 
			case 8: m_check_inner3.SetCheck(m_layers & (1<<i)); break;
			case 9: m_check_inner4.SetCheck(m_layers & (1<<i)); break;
			case 10: m_check_inner5.SetCheck(m_layers & (1<<i)); break;
			case 11: m_check_inner6.SetCheck(m_layers & (1<<i)); break;
			default: break;
			}
		}
		m_check_drill.SetCheck( m_drill_file );
		m_check_outline.SetCheck( m_flags & GERBER_BOARD_OUTLINE );
		m_check_moires.SetCheck( m_flags & GERBER_AUTO_MOIRES );
		m_check_layer_text.SetCheck( m_flags & GERBER_LAYER_TEXT );
		m_check_thermal_pins.SetCheck( !(m_flags & GERBER_NO_PIN_THERMALS) );
		m_check_thermal_vias.SetCheck( !(m_flags & GERBER_NO_VIA_THERMALS) );
		m_check_mask_vias.SetCheck( !(m_flags & GERBER_MASK_VIAS) );
		if( m_flags & GERBER_PILOT_HOLES )
		{
			m_check_pilot.SetCheck( 1 );
			m_edit_pilot_diam.EnableWindow( 1 );
		}
		else
		{
			m_check_pilot.SetCheck( 0 );
			m_edit_pilot_diam.EnableWindow( 0 );
		}
	}
}


BEGIN_MESSAGE_MAP(CDlgCAD, CDialog)
	ON_BN_CLICKED(ID_GO, OnBnClickedGo)
	ON_CBN_SELCHANGE(IDC_COMBO_CAD_UNITS, OnCbnSelchangeComboCadUnits)
	ON_BN_CLICKED(IDC_CHECK_CAD_PILOT, OnBnClickedCheckCadPilot)
	ON_BN_CLICKED(IDCANCEL, OnBnClickedCancel)
END_MESSAGE_MAP()

void CDlgCAD::Initialize( double version, CString * folder, int num_copper_layers, int units, 
						 int fill_clearance, int mask_clearance, int thermal_width,
						 int pilot_diameter, int min_silkscreen_wid,
						 int outline_width, int hole_clearance,
						 int annular_ring_pins, int annular_ring_vias,
						 int flags, int layers, int drill_file,
						 CPolyLine * bd, CArray<CPolyLine> * sm, 
						 BOOL * bShowMessageForClearance,
						 CPartList * pl, CNetList * nl, CTextList * tl, CDisplayList * dl )
{
	m_bShowMessageForClearance = *bShowMessageForClearance;
	m_version = version;
	m_folder = *folder;
	m_units = units;
	m_fill_clearance = fill_clearance;
	m_mask_clearance = mask_clearance;
	m_thermal_width = thermal_width;
	m_pilot_diameter = pilot_diameter;
	m_min_silkscreen_width = min_silkscreen_wid;
	m_num_copper_layers = num_copper_layers;
	m_flags = flags;
	m_layers = layers;
	m_drill_file = drill_file;
	m_bd = bd;
	m_sm = sm;
	m_pl = pl;
	m_nl = nl;
	m_tl = tl;
	m_dl = dl;
	m_hole_clearance = hole_clearance;
	m_outline_width = outline_width;
	m_annular_ring_pins = annular_ring_pins;
	m_annular_ring_vias = annular_ring_vias;
}

// CDlgCAD message handlers

void CDlgCAD::OnBnClickedGo()
{
	// get CAM folder and create it if necessary
	struct _stat buf;
	m_edit_folder.GetWindowText( m_folder );
	int err = _stat( m_folder, &buf );
	if( err )
	{
		CString str;
		str.Format( "Folder \"%s\" doesn't exist, create it ?", m_folder );
		int ret = AfxMessageBox( str, MB_YESNO );
		if( ret == IDYES )
		{
			err = _mkdir( m_folder );
			if( err )
			{
				str.Format( "Unable to create folder \"%s\"", m_folder );
				AfxMessageBox( str, MB_OK );
				return;
			}
		}
		else
			return;
	}
	GetFields();

	// warn about copper-copper clearance
	if( m_fill_clearance == 0 && m_bShowMessageForClearance )     
	{
		CDlgMyMessageBox2 dlg;
		CString mess = "WARNING: You have set the copper to copper-fill clearance to 0.";
		mess += "\nThis will disable automatic generation of clearances for pads and vias in copper areas.";
		mess += "\nAre you SURE that you don't need these clearances ?";
		dlg.Initialize( &mess );
		int ret = dlg.DoModal();
		if( ret == IDCANCEL )
			return;
		else
			m_bShowMessageForClearance = !dlg.bDontShowBoxState;
	}

	if( m_hole_clearance < m_fill_clearance )
	{
		m_hole_clearance = m_fill_clearance;
		SetFields();
	}

	// get flags for Gerbers
	m_flags = 0;
	if( m_check_outline.GetCheck() )
		m_flags |= GERBER_BOARD_OUTLINE;
	if( m_check_moires.GetCheck() )
		m_flags |= GERBER_AUTO_MOIRES;
	if( m_check_layer_text.GetCheck() )
		m_flags |= GERBER_LAYER_TEXT;
	if( m_check_pilot.GetCheck() )
		m_flags |= GERBER_PILOT_HOLES;
	if( !m_check_thermal_pins.GetCheck() )
		m_flags |= GERBER_NO_PIN_THERMALS;
	if( !m_check_thermal_vias.GetCheck() )
		m_flags |= GERBER_NO_VIA_THERMALS;
	if( !m_check_mask_vias.GetCheck() )
		m_flags |= GERBER_MASK_VIAS;

	// open log
	if( !m_dlg_log )
	{
		m_dlg_log = new CDlgLog;
		m_dlg_log->Create( IDD_LOG );
		CRect wRect;
		m_dlg_log->Move( 100, 100 );
	}
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	m_dlg_log->EnableOK( FALSE );

	BOOL errors = FALSE;	// if errors occur

	// create drill file, if requested
	if( m_check_drill.GetCheck() )
	{
		CStdioFile f;
		CString f_name = m_folder + "\\drill_file.drl";
		int ok = f.Open( f_name, CFile::modeCreate | CFile::modeWrite ); 
		if( !ok )
		{
			CString log_message;
			log_message.Format( "ERROR: Unable to open file \"%s\"\r\n", f_name );
			m_dlg_log->AddLine( &log_message );
			errors = TRUE;
		}
		else
		{
			CString log_message;
			log_message.Format( "Writing file: \"%s\"\r\n", f_name );
			m_dlg_log->AddLine( &log_message );
			::WriteDrillFile( &f, m_pl, m_nl );
			f.Close();
		}
		m_drill_file = 1;
	}
	else
		m_drill_file = 0;

	// create Gerber files for selected layers
	m_layers = 0x0;
	for( int i=0; i<(4+m_num_copper_layers); i++ )
	{
		CString gerber_name = "";
		int layer = 0;
		switch(i)
		{
		case 0: if( m_check_top_silk.GetCheck() )
				{ gerber_name = "top_silk.grb"; layer = LAY_SILK_TOP; m_layers |= 1<<i; } 
				break;
		case 1: if( m_check_bottom_silk.GetCheck() )
				{ gerber_name = "bottom_silk.grb"; layer = LAY_SILK_BOTTOM; m_layers |= 1<<i; } 
				break;
		case 2: if( m_check_top_solder.GetCheck() )
				{ gerber_name = "top_mask.grb"; layer = LAY_MASK_TOP; m_layers |= 1<<i; } 
				break;
		case 3: if( m_check_bottom_solder.GetCheck() )
				{ gerber_name = "bottom_mask.grb"; layer = LAY_MASK_BOTTOM; m_layers |= 1<<i; } 
				break;
		case 4: if( m_check_top_copper.GetCheck() )
				{ gerber_name = "top_copper.grb"; layer = LAY_TOP_COPPER; m_layers |= 1<<i; } 
				break;
		case 5: if( m_check_bottom_copper.GetCheck() )
				{ gerber_name = "bottom_copper.grb"; layer = LAY_BOTTOM_COPPER; m_layers |= 1<<i; } 
				break;
		case 6: if( m_check_inner1.GetCheck() )
				{ gerber_name = "inner_copper_1.grb"; layer = LAY_BOTTOM_COPPER+1; m_layers |= 1<<i; } 
				break;
		case 7: if( m_check_inner2.GetCheck() )
				{ gerber_name = "inner_copper_2.grb"; layer = LAY_BOTTOM_COPPER+2; m_layers |= 1<<i; } 
				break;
		case 8: if( m_check_inner3.GetCheck() )
				{ gerber_name = "inner_copper_3.grb"; layer = LAY_BOTTOM_COPPER+3; m_layers |= 1<<i; } 
				break;
		case 9: if( m_check_inner4.GetCheck() )
				{ gerber_name = "inner_copper_4.grb"; layer = LAY_BOTTOM_COPPER+4; m_layers |= 1<<i; } 
				break;
		case 10: if( m_check_inner5.GetCheck() )
				{ gerber_name = "inner_copper_5.grb"; layer = LAY_BOTTOM_COPPER+5; m_layers |= 1<<i; } 
				break;
		case 11: if( m_check_inner6.GetCheck() )
				{ gerber_name = "inner_copper_6.grb"; layer = LAY_BOTTOM_COPPER+6; m_layers |= 1<<i; } 
				break;
		case 12: if( m_check_inner6.GetCheck() )
				{ gerber_name = "inner_copper_7.grb"; layer = LAY_BOTTOM_COPPER+7; m_layers |= 1<<i; } 
				break;
		case 13: if( m_check_inner6.GetCheck() )
				{ gerber_name = "inner_copper_8.grb"; layer = LAY_BOTTOM_COPPER+8; m_layers |= 1<<i; } 
				break;
		case 14: if( m_check_inner6.GetCheck() )
				{ gerber_name = "inner_copper_9.grb"; layer = LAY_BOTTOM_COPPER+9; m_layers |= 1<<i; } 
				break;
		case 15: if( m_check_inner6.GetCheck() )
				{ gerber_name = "inner_copper_10.grb"; layer = LAY_BOTTOM_COPPER+10; m_layers |= 1<<i; } 
				break;
		case 16: if( m_check_inner6.GetCheck() )
				{ gerber_name = "inner_copper_11.grb"; layer = LAY_BOTTOM_COPPER+11; m_layers |= 1<<i; } 
				break;
		case 17: if( m_check_inner6.GetCheck() )
				{ gerber_name = "inner_copper_12.grb"; layer = LAY_BOTTOM_COPPER+12; m_layers |= 1<<i; } 
				break;
		case 18: if( m_check_inner6.GetCheck() )
				{ gerber_name = "inner_copper_13.grb"; layer = LAY_BOTTOM_COPPER+13; m_layers |= 1<<i; } 
				break;
		case 19: if( m_check_inner6.GetCheck() )
				{ gerber_name = "inner_copper_14.grb"; layer = LAY_BOTTOM_COPPER+14; m_layers |= 1<<i; } 
				break;
		default: ASSERT(0); 
				break;
		}
		if( layer )
		{
			// write the gerber file
			CString f_str = m_folder + "\\" + gerber_name;
			CStdioFile f;
			int ok = f.Open( f_str, CFile::modeCreate | CFile::modeWrite );
			if( !ok )
			{
				CString log_message;
				log_message.Format( "ERROR: Unable to open file \"%s\"\r\n", f_str );
				m_dlg_log->AddLine( &log_message );
				errors = TRUE;
			}
			else
			{
				CString log_message;
				log_message.Format( "Writing file: \"%s\"\r\n", f_str );
				m_dlg_log->AddLine( &log_message );
				CString line;
				line.Format( "G04 FreePCB version %5.3f*\n", m_version );
				f.WriteString( line );
				line.Format( "G04 %s*\n", f_str );
				f.WriteString( line );
				::WriteGerberFile( &f, m_flags, layer, 
					m_dlg_log,
					m_fill_clearance, m_mask_clearance, m_pilot_diameter,
					m_min_silkscreen_width, m_thermal_width,
					m_outline_width, m_hole_clearance,
					m_annular_ring_pins, m_annular_ring_vias,
					m_bd, m_sm, m_pl, m_nl, m_tl, m_dl );
				f.WriteString( "M02*\n" );	// end of job
				f.Close();
			}
		}
	}
	if( errors )
		m_dlg_log->AddLine( &CString( "********* ERRORS OCCURRED **********\r\n" ) );
	else
		m_dlg_log->AddLine( &CString( "************* SUCCESS **************\r\n" ) );
	m_dlg_log->EnableOK( TRUE );
}

void CDlgCAD::GetFields()
{
	CString str;
	if( m_units == MIL )
	{
		m_edit_fill.GetWindowText( str );
		m_fill_clearance = atof( str ) * NM_PER_MIL;
		m_edit_mask.GetWindowText( str );
		m_mask_clearance = atof( str ) * NM_PER_MIL;
		m_edit_pilot_diam.GetWindowText( str );
		m_pilot_diameter = atof( str ) * NM_PER_MIL;
		m_edit_min_ss_w.GetWindowText( str );
		m_min_silkscreen_width = atof( str ) * NM_PER_MIL;
		m_edit_thermal_width.GetWindowText( str );
		m_thermal_width = atof( str ) * NM_PER_MIL;
		m_edit_outline_width.GetWindowText( str );
		m_outline_width = atof( str ) * NM_PER_MIL;
		m_edit_hole_clearance.GetWindowText( str );
		m_hole_clearance = atof( str ) * NM_PER_MIL;
		m_edit_ann_pins.GetWindowText( str );
		m_annular_ring_pins = atof( str ) * NM_PER_MIL;
		m_edit_ann_vias.GetWindowText( str );
		m_annular_ring_vias = atof( str ) * NM_PER_MIL;
	}
	else
	{
		m_edit_fill.GetWindowText( str );
		m_fill_clearance = atof( str ) * 1000000.0;
		m_edit_mask.GetWindowText( str );
		m_mask_clearance = atof( str ) * 1000000.0;
		m_edit_pilot_diam.GetWindowText( str );
		m_pilot_diameter = atof( str ) * 1000000.0;
		m_edit_min_ss_w.GetWindowText( str );
		m_min_silkscreen_width = atof( str ) * 1000000.0;
		m_edit_thermal_width.GetWindowText( str );
		m_thermal_width = atof( str ) * 1000000.0;
		m_edit_outline_width.GetWindowText( str );
		m_outline_width = atof( str ) * 1000000.0;
		m_edit_hole_clearance.GetWindowText( str );
		m_hole_clearance = atof( str ) * 1000000.0;
		m_edit_ann_pins.GetWindowText( str );
		m_annular_ring_pins = atof( str ) * 1000000.0;
		m_edit_ann_vias.GetWindowText( str );
		m_annular_ring_vias = atof( str ) * 1000000.0;
	}
}

void CDlgCAD::SetFields()
{
	CString str;
	if( m_units == MIL )
	{
		MakeCStringFromDouble( &str, m_fill_clearance/NM_PER_MIL );
		m_edit_fill.SetWindowText( str );
		MakeCStringFromDouble( &str, m_mask_clearance/NM_PER_MIL );
		m_edit_mask.SetWindowText( str );
		MakeCStringFromDouble( &str, m_pilot_diameter/NM_PER_MIL );
		m_edit_pilot_diam.SetWindowText( str );
		MakeCStringFromDouble( &str, m_min_silkscreen_width/NM_PER_MIL );
		m_edit_min_ss_w.SetWindowText( str );
		MakeCStringFromDouble( &str, m_thermal_width/NM_PER_MIL );
		m_edit_thermal_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_outline_width/NM_PER_MIL );
		m_edit_outline_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_hole_clearance/NM_PER_MIL );
		m_edit_hole_clearance.SetWindowText( str );
		MakeCStringFromDouble( &str, m_annular_ring_pins/NM_PER_MIL );
		m_edit_ann_pins.SetWindowText( str );
		MakeCStringFromDouble( &str, m_annular_ring_vias/NM_PER_MIL );
		m_edit_ann_vias.SetWindowText( str );
	}
	else
	{
		MakeCStringFromDouble( &str, m_fill_clearance/1000000.0 );
		m_edit_fill.SetWindowText( str );
		MakeCStringFromDouble( &str, m_mask_clearance/1000000.0 );
		m_edit_mask.SetWindowText( str );
		MakeCStringFromDouble( &str, m_pilot_diameter/1000000.0 );
		m_edit_pilot_diam.SetWindowText( str );
		MakeCStringFromDouble( &str, m_min_silkscreen_width/1000000.0 );
		m_edit_min_ss_w.SetWindowText( str );
		MakeCStringFromDouble( &str, m_thermal_width/1000000.0 );
		m_edit_thermal_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_outline_width/1000000.0 );
		m_edit_outline_width.SetWindowText( str );
		MakeCStringFromDouble( &str, m_hole_clearance/1000000.0 );
		m_edit_hole_clearance.SetWindowText( str );
		MakeCStringFromDouble( &str, m_annular_ring_pins/1000000.0 );
		m_edit_ann_pins.SetWindowText( str );
		MakeCStringFromDouble( &str, m_annular_ring_vias/1000000.0 );
		m_edit_ann_vias.SetWindowText( str );
	}
}
void CDlgCAD::OnCbnSelchangeComboCadUnits()
{
	GetFields();
	if( m_combo_units.GetCurSel() == 0 )
		m_units = MIL;
	else
		m_units = MM;
	SetFields();
}

void CDlgCAD::OnBnClickedCheckCadPilot()
{
	if( m_check_pilot.GetCheck() )
		m_edit_pilot_diam.EnableWindow( 1 );
	else
		m_edit_pilot_diam.EnableWindow( 0 );
}

void CDlgCAD::OnBnClickedCancel()
{
	OnCancel();
}
