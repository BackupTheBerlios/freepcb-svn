// DlgPartlist.cpp : implementation file
//

#include "stdafx.h"
#include "FreePcb.h"
#include "DlgPartlist.h"
#include "DlgAddPart.h"
#include ".\dlgpartlist.h"

//global so that sorting callbacks will work
partlist_info pl;

// columns for list
enum {
	COL_NAME = 0,
	COL_PACKAGE,
	COL_FOOTPRINT
};

// sort types
enum {
	SORT_UP_NAME = 0,
	SORT_DOWN_NAME,
	SORT_UP_PACKAGE,
	SORT_DOWN_PACKAGE,
	SORT_UP_FOOTPRINT,
	SORT_DOWN_FOOTPRINT
};

// global callback function for sorting
// lp1, lp2 are indexes to global arrays above
//		
int CALLBACK ComparePartlist( LPARAM lp1, LPARAM lp2, LPARAM type )
{
	int ret = 0;
	switch( type )
	{
		case SORT_UP_NAME:
		case SORT_DOWN_NAME:
			ret = (strcmp( ::pl[lp1].ref_des, ::pl[lp2].ref_des ));
			break;

		case SORT_UP_PACKAGE:
		case SORT_DOWN_PACKAGE:
			ret = (strcmp( ::pl[lp1].package, ::pl[lp2].package ));
			break;

		case SORT_UP_FOOTPRINT:
		case SORT_DOWN_FOOTPRINT:
			if( ::pl[lp1].shape && ::pl[lp2].shape )
				ret = (strcmp( ::pl[lp1].shape->m_name, ::pl[lp2].shape->m_name ));
			else
				ret = 0;
			break;

	}
	switch( type )
	{
		case SORT_DOWN_NAME:
		case SORT_DOWN_PACKAGE:
		case SORT_DOWN_FOOTPRINT:
			ret = -ret;
			break;
	}
	return ret;
}

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
	ON_NOTIFY(LVN_COLUMNCLICK, IDC_LIST1, OnLvnColumnClickList1)
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
	m_list_ctrl.SortItems( ::ComparePartlist, SORT_UP_NAME ); 
	return TRUE;
}

void CDlgPartlist::Initialize( CPartList * plist,
			CMapStringToPtr * shape_cache_map,
			CFootLibFolderMap * footlibfoldermap,
			int units, CDlgLog * log )
{
	m_units = units;
	m_plist = plist;
	m_footprint_cache_map = shape_cache_map;
	m_footlibfoldermap = footlibfoldermap;
	m_sort_type = 0;
	m_dlg_log = log;
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
		m_footprint_cache_map, m_footlibfoldermap, m_units, m_dlg_log );
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
				::pl[ip].ref_size = ::pl[i].ref_size;
				::pl[ip].ref_width = ::pl[i].ref_width;
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
		m_footprint_cache_map, m_footlibfoldermap, m_units, m_dlg_log );
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

void CDlgPartlist::OnLvnColumnClickList1(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	int column = pNMLV->iSubItem;
	if( column == COL_NAME )
	{
		if( m_sort_type == SORT_UP_NAME )
			m_sort_type = SORT_DOWN_NAME;
		else
			m_sort_type = SORT_UP_NAME;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	else if( column == COL_PACKAGE )
	{
		if( m_sort_type == SORT_UP_PACKAGE )
			m_sort_type = SORT_DOWN_PACKAGE;
		else
			m_sort_type = SORT_UP_PACKAGE;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	else if( column == COL_FOOTPRINT )
	{
		if( m_sort_type == SORT_UP_FOOTPRINT )
			m_sort_type = SORT_DOWN_FOOTPRINT;
		else
			m_sort_type = SORT_UP_FOOTPRINT;
		m_list_ctrl.SortItems( ::ComparePartlist, m_sort_type );
	}
	*pResult = 0;
}
