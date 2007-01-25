// DlgReassignLayers.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgReassignLayers.h"


// CDlgReassignLayers dialog

IMPLEMENT_DYNAMIC(CDlgReassignLayers, CDialog)
CDlgReassignLayers::CDlgReassignLayers(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgReassignLayers::IDD, pParent)
{
}

CDlgReassignLayers::~CDlgReassignLayers()
{
}

void CDlgReassignLayers::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO1, m_combo[0]);
	DDX_Control(pDX, IDC_COMBO2, m_combo[1]);
	DDX_Control(pDX, IDC_COMBO3, m_combo[2]);
	DDX_Control(pDX, IDC_COMBO4, m_combo[3]);
	DDX_Control(pDX, IDC_COMBO5, m_combo[4]);
	DDX_Control(pDX, IDC_COMBO6, m_combo[5]);
	DDX_Control(pDX, IDC_COMBO7, m_combo[6]);
	DDX_Control(pDX, IDC_COMBO8, m_combo[7]);
	if( !pDX->m_bSaveAndValidate )
	{
		// incoming
		// set up combo boxes, loop through all possible copper layers
		for( int il=0; il<8; il++ ) 
		{
			if( il < m_old_layers )
			{
				// old layer, set combo box to pick from new layers or "delete"
				m_combo[il].EnableWindow( TRUE );
				for( int inew=0; inew<m_new_layers; inew++ )
					m_combo[il].InsertString( inew, layer_str[LAY_TOP_COPPER+inew] );
				m_combo[il].InsertString( m_new_layers, "delete" );
				// initialize combo box to old layer, or delete if old layer > new layers
				if( il < m_new_layers )
					m_combo[il].SetCurSel(il);
				else
					m_combo[il].SetCurSel( m_new_layers );
			}
			else
				// this isn't an old layer, disable combo box
				m_combo[il].EnableWindow( FALSE );
		}
	}
	else
	{
		// outgoing
	}
}

void CDlgReassignLayers::Initialize( int num_old_layers, int num_new_layers )
{
	m_old_layers = num_old_layers;
	m_new_layers = num_new_layers;
}


BEGIN_MESSAGE_MAP(CDlgReassignLayers, CDialog)
END_MESSAGE_MAP()


// CDlgReassignLayers message handlers
