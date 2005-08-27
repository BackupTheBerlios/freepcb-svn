// DlgAddArea.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgAddArea.h"
#include "layers.h"


// CDlgAddArea dialog

IMPLEMENT_DYNAMIC(CDlgAddArea, CDialog)
CDlgAddArea::CDlgAddArea(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgAddArea::IDD, pParent)
{
}

CDlgAddArea::~CDlgAddArea()
{
}

void CDlgAddArea::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_NET, m_combo_net);
	DDX_Control(pDX, IDC_LIST_LAYER, m_list_layer);
	DDX_Control(pDX, IDC_RADIO_NONE, m_radio_none);
	DDX_Control(pDX, IDC_RADIO_FULL, m_radio_full);
	DDX_Control(pDX, IDC_RADIO_EDGE, m_radio_edge);
	if( pDX->m_bSaveAndValidate )
	{
		m_layer = m_list_layer.GetCurSel() + LAY_TOP_COPPER;
		m_combo_net.GetWindowText( m_net_name );

		POSITION pos;
		CString name;
		void * ptr;
		m_net = m_nlist->GetNetPtrByName( &m_net_name );
		if( !m_net )
		{
			AfxMessageBox( "Illegal net name" );
			pDX->Fail();
		}
		if( m_radio_none.GetCheck() )
			m_hatch = CPolyLine::NO_HATCH;
		else if( m_radio_full.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_FULL;
		else if( m_radio_edge.GetCheck() )
			m_hatch = CPolyLine::DIAGONAL_EDGE;
		else 
			ASSERT(0);
	}
}


BEGIN_MESSAGE_MAP(CDlgAddArea, CDialog)
END_MESSAGE_MAP()


// CDlgAddArea message handlers
int CDlgAddArea::OnInitDialog()
{
	POSITION pos;
	CString name;
	void * ptr;
	CString str;
	int i = 0;

	CDialog::OnInitDialog();

	// initialize net list
	for( pos = m_nlist->m_map.GetStartPosition(); pos != NULL; )
	{
		m_nlist->m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		m_combo_net.AddString( name );
		i++;
	}
	m_num_nets = i;
	m_num_layers = m_num_layers-LAY_TOP_COPPER;
	for( int il=0; il<m_num_layers; il++ )
	{
		m_list_layer.InsertString( il, &layer_str[il+LAY_TOP_COPPER][0] );
	}
	m_list_layer.SetCurSel( m_layer - LAY_TOP_COPPER );
	m_radio_none.SetCheck( 1 );
	return 0;
}