// DlgPartlist.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgPartlist.h"
#include "DlgAddPart.h"

//global so that sorting callbacks will work
partlist_info pl;

// CDlgPartlist dialog

IMPLEMENT_DYNAMIC(CDlgPartlist, CDialog)
CDlgPartlist::CDlgPartlist(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPartlist::IDD, pParent)
{
}

CDlgPartlist::~CDlgPartlist()
{
}

void CDlgPartlist::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST1, m_list_ctrl);
	DDX_Control(pDX, IDC_BUTTON_ADD, m_button_add);
	DDX_Control(pDX, IDC_BUTTON_EDIT, m_button_edit);
	DDX_Control(pDX, IDC_BUTTON_DELETE, m_button_delete);
	if( pDX->m_bSaveAndValidate )
		m_plist->ImportPartListInfo( &::pl, 0 );
}


BEGIN_MESSAGE_MAP(CDlgPartlist, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ADD, OnBnClickedButtonAdd)
	ON_BN_CLICKED(IDC_BUTTON_EDIT, OnBnClickedButtonEdit)
	ON_BN_CLICKED(IDC_BUTTON_DELETE, OnBnClickedButtonDelete)
END_MESSAGE_MAP()

BOOL CDlgPartlist::OnInitDialog()
{
	CDialog::OnInitDialog();
	m_plist->ExportPartListInfo( &::pl, NULL );
	::pl.GetSize();

	// now set up listview control
	int nItem;
	LVITEM lvitem;
	CString str;
	DWORD old_style = m_list_ctrl.GetExtendedStyle();
	m_list_ctrl.SetExtendedStyle( LVS_EX_FULLROWSELECT | LVS_EX_FLATSB | old_style );
	m_list_ctrl.InsertColumn( 0, "Reference", LVCFMT_LEFT, 70 );
	m_list_ctrl.InsertColumn( 1, "Package", LVCFMT_LEFT, 150 );
	m_list_ctrl.InsertColumn( 2, "Footprint", LVCFMT_LEFT, 150 );
	for( int i=0; i<::pl.GetSize(); i++ )
	{
		lvitem.mask = LVIF_TEXT | LVIF_PARAM;
		lvitem.pszText = "";
		lvitem.lParam = i;
		nItem = m_list_ctrl.InsertItem( i, "" );
		m_list_ctrl.SetItemData( i, (LPARAM)i );
		m_list_ctrl.SetItem( i, 0, LVIF_TEXT, ::pl[i].ref_des, 0, 0, 0, 0 );
		if( ::pl[i].package != "" )
			m_list_ctrl.SetItem( i, 1, LVIF_TEXT, ::pl[i].package, 0, 0, 0, 0 );
		else
			m_list_ctrl.SetItem( i, 1, LVIF_TEXT, "??????", 0, 0, 0, 0 );
		if( ::pl[i].shape )
			m_list_ctrl.SetItem( i, 2, LVIF_TEXT, ::pl[i].shape->m_name, 0, 0, 0, 0 );
		else
			m_list_ctrl.SetItem( i, 2, LVIF_TEXT, "??????", 0, 0, 0, 0 );
	}
	return TRUE;
}

void CDlgPartlist::Initialize( CPartList * plist,
			CMapStringToPtr * shape_cache_map,
			CFootLibFolderMap * footlibfoldermap,
			int units )
{
	m_units = units;
	m_plist = plist;
	m_footprint_cache_map = shape_cache_map;
	m_footlibfoldermap = footlibfoldermap;
}

// CDlgPartlist message handlers

void CDlgPartlist::OnBnClickedButtonEdit()
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
		AfxMessageBox( "You have no part selected" );
	BOOL bMultiple = FALSE;
	if( n_sel > 1 )
		bMultiple = TRUE;

	POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition(); 
	if (pos == NULL)
		ASSERT(0);
	int iItem = m_list_ctrl.GetNextSelectedItem(pos);
	int i = m_list_ctrl.GetItemData( iItem );
	CDlgAddPart dlg;
	dlg.Initialize( &::pl, i, FALSE, FALSE, bMultiple,
		m_footprint_cache_map, m_footlibfoldermap, m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString str;
		if( bMultiple )
		{
			// update all selected parts with new package and footprint
			CString new_package = ::pl[i].package;
			CString new_footprint = ::pl[i].shape->m_name;
			POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
			while( pos )
			{
				int iItem = m_list_ctrl.GetNextSelectedItem(pos);
				int ip = m_list_ctrl.GetItemData( iItem );
				::pl[ip].shape = ::pl[i].shape;
			}
		}
		for( int ip=0; ip<m_list_ctrl.GetItemCount(); ip++ ) 
		{
			int i = m_list_ctrl.GetItemData( ip );
			m_list_ctrl.SetItem( ip, 0, LVIF_TEXT, ::pl[i].ref_des, 0, 0, 0, 0 );
			if( ::pl[i].package != "" )
				m_list_ctrl.SetItem( ip, 1, LVIF_TEXT, ::pl[i].package, 0, 0, 0, 0 );
			else
				m_list_ctrl.SetItem( ip, 1, LVIF_TEXT, "??????", 0, 0, 0, 0 );
			if( ::pl[i].shape )
				str.Format( "%s", ::pl[i].shape->m_name );
			else
				str = "??????";
			m_list_ctrl.SetItem( ip, 2, LVIF_TEXT, str, 0, 0, 0, 0 );
		}
	}
}

void CDlgPartlist::OnBnClickedButtonAdd()
{
	CDlgAddPart dlg;
	dlg.Initialize( &::pl, -1, FALSE, TRUE, FALSE,
		m_footprint_cache_map, m_footlibfoldermap, m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// last item in partlist_info is the new part
		int ip = ::pl.GetSize() - 1;
		int il = m_list_ctrl.GetItemCount();
		m_list_ctrl.InsertItem( il, "" );
		m_list_ctrl.SetItemData( il, (LPARAM)ip );
		m_list_ctrl.SetItem( il, 0, LVIF_TEXT, ::pl[ip].ref_des, 0, 0, 0, 0 );
		m_list_ctrl.SetItem( il, 1, LVIF_TEXT, ::pl[ip].shape->m_name, 0, 0, 0, 0 );
		m_list_ctrl.SetItem( il, 2, LVIF_TEXT, ::pl[ip].shape->m_name, 0, 0, 0, 0 );
	}
}

void CDlgPartlist::OnBnClickedButtonDelete()
{
	int n_sel = m_list_ctrl.GetSelectedCount();
	if( n_sel == 0 )
		AfxMessageBox( "You have no part selected" );
	else
	{
		while( m_list_ctrl.GetSelectedCount() )
		{
			POSITION pos = m_list_ctrl.GetFirstSelectedItemPosition();
			if (pos == NULL)
				ASSERT(0);
			int iItem = m_list_ctrl.GetNextSelectedItem(pos);
			int ip = m_list_ctrl.GetItemData( iItem );
			::pl[ip].deleted = TRUE;
			m_list_ctrl.DeleteItem( iItem );
		}
	}
}
