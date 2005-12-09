// FreePcbDoc.cpp : implementation of the CFreePcbDoc class
//
#pragma once

#include "stdafx.h"
#include <direct.h>
#include <shlwapi.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "PcbFont.h"
#include "DlgAddPart.h"
#include "DlgEditNet.h"
#include "DlgAssignNet.h"
#include "DlgNetlist.h"
#include "DlgProjectOptions.h"
#include "DlgImportOptions.h"
#include "freepcbdoc.h"
#include "DlgLayers.h"
#include "DlgPartlist.h"
#include "MyFileDialog.h"
#include "MyFileDialogExport.h"
#include "DlgIvex.h"
#include "DlgIndexing.h"
#include "UndoBuffer.h"
#include "UndoList.h"
#include "DlgCAD.h"
#include "DlgWizQuad.h"
#include "utility.h"
#include "gerber.h"
#include "dlgdrc.h"
#include ".\freepcbdoc.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CFreePcbApp theApp;

CFreePcbDoc * this_Doc;		// global for callback

// global arrays to map file_layers to actual layers
int m_file_layer_by_layer[MAX_LAYERS];
int m_layer_by_file_layer[MAX_LAYERS];


/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc

IMPLEMENT_DYNCREATE(CFreePcbDoc, CDocument)

BEGIN_MESSAGE_MAP(CFreePcbDoc, CDocument)
	//{{AFX_MSG_MAP(CFreePcbDoc)
	ON_COMMAND(ID_FILE_SAVE_AS, OnFileSaveAs)
	ON_COMMAND(ID_FILE_SAVE, OnFileSave)
	//}}AFX_MSG_MAP
	ON_COMMAND(ID_ADD_PART, OnAddPart)
	ON_COMMAND(ID_NONE_ADDPART, OnAddPart)
	ON_COMMAND(ID_VIEW_NETLIST, OnViewNetlist)
	ON_COMMAND(ID_FILE_OPEN, OnFileOpen)
	ON_COMMAND(ID_FILE_NEW, OnFileNew)
	ON_COMMAND(ID_FILE_CLOSE, OnFileClose)
	ON_COMMAND(ID_VIEW_LAYERS, OnViewLayers)
	ON_COMMAND(ID_VIEW_PARTLIST, OnViewPartlist)
	ON_COMMAND(ID_PART_PROPERTIES, OnPartProperties)
	ON_COMMAND(ID_FILE_IMPORT, OnFileImport)
	ON_COMMAND(ID_APP_EXIT, OnAppExit)
	ON_COMMAND(ID_FILE_CONVERT, OnFileConvert)
//	ON_COMMAND(ID_EDIT_UNDO, OnEditUndo)
	ON_COMMAND(ID_FILE_GENERATECADFILES, OnFileGenerateCadFiles)
	ON_COMMAND(ID_TOOLS_FOOTPRINTWIZARD, OnToolsFootprintwizard)
	ON_COMMAND(ID_PROJECT_OPTIONS, OnProjectOptions)
	ON_COMMAND(ID_FILE_EXPORTNETLIST, OnFileExport)
	ON_COMMAND(ID_TOOLS_CHECK_PARTS_NETS, OnToolsCheckPartsAndNets)
	ON_COMMAND(ID_TOOLS_DRC, OnToolsDrc)
	ON_COMMAND(ID_TOOLS_CLEAR_DRC, OnToolsClearDrc)
	ON_COMMAND(ID_TOOLS_SHOWDRCERRORLIST, OnToolsShowDRCErrorlist)
	ON_COMMAND(ID_TOOLS_CHECK_CONNECTIVITY, OnToolsCheckConnectivity)
	ON_COMMAND(ID_VIEW_LOG, OnViewLog)
	ON_COMMAND(ID_TOOLS_CHECKCOPPERAREAS, OnToolsCheckCopperAreas)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc construction/destruction

CFreePcbDoc::CFreePcbDoc()
{
	// get application directory
	// (there must be a better way to do this!!!)
	int token_start = 0;
	CString delim = " ";
	CString cmdline = GetCommandLine();
	if( cmdline[0] == '\"' )
	{
		delim = "\"";
		token_start = 1;
	}
	CString app_dir = cmdline.Tokenize( delim, token_start );
	int pos = app_dir.ReverseFind( '\\' );
	if( pos == -1 )
		pos = app_dir.ReverseFind( ':' ); 
	if( pos == -1 )
		ASSERT(0);	// failed to find application folder
	app_dir = app_dir.Left( pos );
	m_app_dir = app_dir;
	m_app_dir.Trim();
	int err = _chdir( m_app_dir );	// change to application folder
	if( err )
		ASSERT(0);	// failed to switch to application folder

	m_smfontutil = new SMFontUtil( &m_app_dir );
	m_dlist = new CDisplayList();
	m_dlist_fp = new CDisplayList();
	m_plist = new CPartList( m_dlist, m_smfontutil );
	m_nlist = new CNetList( m_dlist, m_plist );
	m_plist->UseNetList( m_nlist );
	m_plist->SetShapeCacheMap( &m_footprint_cache_map );
	m_tlist = new CTextList( m_dlist, m_smfontutil );
	m_drelist = new DRErrorList;
	m_drelist->SetLists( m_plist, m_nlist, m_dlist );
	m_pcb_filename = "";
	m_pcb_full_path = "";
	m_board_outline = 0;
	m_project_open = FALSE;
	m_project_modified = FALSE;
	m_footprint_modified = FALSE;
	m_footprint_name_changed = FALSE;
	theApp.m_Doc = this;
	m_undo_list = new CUndoList( 10000 );
	this_Doc = this;
	m_auto_interval = 0;
	m_auto_elapsed = 0;
	m_dlg_log = NULL;
	bNoFilesOpened = TRUE;
	m_version = 1.3;
	m_file_version = 1.112;
	m_dlg_log = new CDlgLog;
	m_dlg_log->Create( IDD_LOG );
}

CFreePcbDoc::~CFreePcbDoc()
{
	// delete partlist, netlist, displaylist, etc.
	m_sm_cutout.RemoveAll();
	delete m_drelist;
	delete m_undo_list;
	if( m_board_outline )
		delete m_board_outline;
	delete m_nlist;
	delete m_plist;
	delete m_tlist;
	delete m_dlist;
	delete m_dlist_fp;
	delete m_smfontutil;

	// delete all shapes from local cache
	POSITION pos = m_footprint_cache_map.GetStartPosition();
	while( pos != NULL )
	{
		void * ptr;
		CShape * shape;
		CString string;
		m_footprint_cache_map.GetNextAssoc( pos, string, ptr );
		shape = (CShape*)ptr;
		delete shape;
	}
	m_footprint_cache_map.RemoveAll();
	if( (long)m_dlg_log == 0xcdcdcdcd )
		ASSERT(0);
	else if( m_dlg_log )
	{
		m_dlg_log->DestroyWindow();
		delete m_dlg_log;
	}
}

void CFreePcbDoc::SendInitialUpdate()
{
	CDocument::SendInitialUpdate();
}

// this is only executed once, at beginning of app
//
BOOL CFreePcbDoc::OnNewDocument()
{
	if (!CDocument::OnNewDocument())
		return FALSE;

	m_window_title = "no project open";
	m_parent_folder = "..\\projects\\";
	m_lib_dir = "..\\lib\\" ;
	return TRUE;
}



/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc serialization

void CFreePcbDoc::Serialize(CArchive& ar)
{
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc diagnostics

#ifdef _DEBUG
void CFreePcbDoc::AssertValid() const
{
	CDocument::AssertValid();
}

void CFreePcbDoc::Dump(CDumpContext& dc) const
{
	CDocument::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFreePcbDoc commands


BOOL CFreePcbDoc::OnSaveDocument(LPCTSTR lpszPathName) 
{
	return CDocument::OnSaveDocument(lpszPathName);
}

void CFreePcbDoc::OnFileNew()
{
	if( theApp.m_view_mode == CFreePcbApp::FOOTPRINT )
	{
		theApp.m_View_fp->OnFootprintFileNew();
		return;
	}

	if( FileClose() == IDCANCEL )
		return;

	m_view->CancelSelection();

	// now set default project options
	InitializeNewProject();
	CDlgProjectOptions dlg;
	dlg.Init( TRUE, &m_name, &m_parent_folder, &m_lib_dir,
		m_num_copper_layers, m_trace_w, m_via_w, m_via_hole_w,
		m_auto_interval, &m_w, &m_v_w, &m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// set up project file name and path
		m_name = dlg.GetName();
//**		_chdir( m_app_dir );
		m_pcb_filename = m_name + ".fpc";
		CString fullpath;
		char full[_MAX_PATH];
		fullpath = _fullpath( full, (LPCSTR)dlg.GetPathToFolder(), MAX_PATH );
		m_path_to_folder = (CString)fullpath;

		// Check if project folder exists and create it if necessary
		struct _stat buf;
		int err = _stat( m_path_to_folder, &buf );
		if( err )
		{
			CString str;
			str.Format( "Folder \"%s\" doesn't exist, create it ?", m_path_to_folder );
			int ret = AfxMessageBox( str, MB_YESNO );
			if( ret == IDYES )
			{
				err = _mkdir( m_path_to_folder );
				if( err )
				{
					str.Format( "Unable to create folder \"%s\"", m_path_to_folder );
					AfxMessageBox( str, MB_OK );
				}
			}
		}
		if( err )
			return;

		CString str;
		m_pcb_full_path = (CString)fullpath	+ "\\" + m_pcb_filename;
		m_window_title = "FreePCB - " + m_pcb_filename;
		CWnd* pMain = AfxGetMainWnd();
		pMain->SetWindowText( m_window_title );

		// make path to library folder and index libraries
		m_lib_dir = dlg.GetLibFolder();
		fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
		if( fullpath[fullpath.GetLength()-1] == '\\' )	
			fullpath = fullpath.Left(fullpath.GetLength()-1);
		m_full_lib_dir = fullpath;
		MakeLibraryMaps( &m_full_lib_dir );
		CMenu* pMenu = pMain->GetMenu();
		pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_ENABLED ); 
		pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_ENABLED ); 
		pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_ENABLED ); 
		pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_ENABLED ); 
		pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_ENABLED ); 
		CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
		submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_ENABLED );	
		submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_ENABLED );	
		pMain->DrawMenuBar();

		// set options from dialog
		m_num_copper_layers = dlg.GetNumCopperLayers();
		m_plist->SetNumCopperLayers( m_num_copper_layers );
		m_nlist->SetNumCopperLayers( m_num_copper_layers );
		m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
		m_trace_w = dlg.GetTraceWidth();
		m_via_w = dlg.GetViaWidth();
		m_via_hole_w = dlg.GetViaHoleWidth();
		for( int i=0; i<m_num_layers; i++ )
		{
			m_vis[i] = 1;
			m_dlist->SetLayerRGB( i, m_rgb[i][0], m_rgb[i][1], m_rgb[i][2] );
		}

		// force redraw of left pane
		m_view->InvalidateLeftPane();
		m_view->Invalidate( FALSE );
		m_project_open = TRUE;

		// force redraw of function key text
		m_view->m_cursor_mode = 999;
		m_view->SetCursorMode( CUR_NONE_SELECTED );

		// force redraw of window title
		m_project_modified = FALSE;
		m_auto_elapsed = 0;
		ProjectModified( TRUE );
	}
}

void CFreePcbDoc::OnFileOpen()
{
	if( theApp.m_view_mode == CFreePcbApp::FOOTPRINT )
	{
		theApp.m_View_fp->OnFootprintFileImport();
		return;
	}

	if( FileClose() == IDCANCEL )
		return;

	m_view->CancelSelection();
	InitializeNewProject();		// set defaults

	// get project file name
	// force old-style file dialog by setting size of OPENFILENAME struct (for Win98)
	CFileDialog dlg( 1, "fpc", LPCSTR(*m_pcb_filename), 0, 
		"PCB files (*.fpc)|*.fpc|All Files (*.*)|*.*||", 
		NULL, OPENFILENAME_SIZE_VERSION_400 );
	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_parent_folder;
	// now show dialog
	int err = dlg.DoModal();
	if( err == IDOK )
	{
		// read project file
		CString pathname = dlg.GetPathName();
		CString filename = dlg.GetFileName();
		CStdioFile pcb_file;
		int err = pcb_file.Open( pathname, CFile::modeRead, NULL );
		if( !err )
		{
			// error opening project file
			CString mess;
			mess.Format( "Unable to open file %s", pathname );
			AfxMessageBox( mess );
			return;
		}

		try
		{
			CString key_str;
			CString in_str;
			CArray<CString> p;

			ReadOptions( &pcb_file );
			ReadFootprints( &pcb_file );
			ReadBoardOutline( &pcb_file );
			ReadSolderMaskCutouts( &pcb_file );
			m_plist->ReadParts( &pcb_file );
			m_nlist->ReadNets( &pcb_file );
			m_tlist->ReadTexts( &pcb_file );

			// make path to library folder and index libraries
			if( m_full_lib_dir == "" )
			{
//**				_chdir( m_app_dir );
				CString fullpath;
				char full[MAX_PATH];
				fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
				if( fullpath[fullpath.GetLength()-1] == '\\' )	
					fullpath = fullpath.Left(fullpath.GetLength()-1);
				m_full_lib_dir = fullpath;
			}
			MakeLibraryMaps( &m_full_lib_dir );

			m_pcb_full_path = pathname;
			m_pcb_filename = filename;
			int fnl = m_pcb_filename.GetLength();
			m_path_to_folder = m_pcb_full_path.Left( m_pcb_full_path.GetLength() - fnl - 1 );
			m_window_title = "FreePCB - " + m_pcb_filename;
			CWnd* pMain = AfxGetMainWnd();
			pMain->SetWindowText( m_window_title );
			SetPathName( m_pcb_filename, TRUE );
			if( m_name == "" )
			{
				m_name = filename;
				if( m_name.Right(4) == ".fpc" )
					m_name = m_name.Left( m_name.GetLength() - 4 );
			}
			if (pMain != NULL)
			{
				CMenu* pMenu = pMain->GetMenu();
				pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_ENABLED ); 
				pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_ENABLED ); 
				pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_ENABLED ); 
				pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_ENABLED ); 
				pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_ENABLED ); 
				CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
				submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED );	
				submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_ENABLED );	
				submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED );	
				submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
				submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_ENABLED );	
				submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_ENABLED );	
				pMain->DrawMenuBar();
			}
			m_project_open = TRUE;
			theApp.AddMRUFile( &pathname );
			// now set layer visibility
			for( int i=0; i<m_num_layers; i++ )
			{
				m_dlist->SetLayerRGB( i, m_rgb[i][0], m_rgb[i][1], m_rgb[i][2] );
				m_dlist->SetLayerVisible( i, m_vis[i] );
			}
			// force redraw
			m_view->m_cursor_mode = 999;
			m_view->SetCursorMode( CUR_NONE_SELECTED );
			m_view->InvalidateLeftPane();
			ProjectModified( FALSE );
			m_auto_elapsed = 0;
			CDC * pDC = m_view->GetDC();
			m_view->OnViewAllElements();
			m_view->OnDraw( pDC );
			m_view->ReleaseDC( pDC );
			return;
		}
		catch( CString * err_str )
		{
			// parsing error
			AfxMessageBox( *err_str );
			delete err_str;
			ProjectModified( FALSE );
			OnFileClose();	// TODO: change this
			return;
		}
	}
	else
	{
		// CANCEL or error
		DWORD dwError = ::CommDlgExtendedError();
		if( dwError )
		{
			CString str;
			str.Format( "File Open Dialog error code = %ulx\n", (unsigned long)dwError );
			AfxMessageBox( str );
		}
	}
}

void CFreePcbDoc::OnFileAutoOpen( CString * fn )
{
	if( FileClose() == IDCANCEL )
		return;

	m_view->CancelSelection();
	InitializeNewProject();		// set defaults

	CStdioFile pcb_file;
	int err = pcb_file.Open( *fn, CFile::modeRead, NULL );
	if( !err )
	{
		// error opening project file
		CString mess;
		mess.Format( "Unable to open file %s", fn );
		AfxMessageBox( mess );
		return;
	}

	try
	{
		CString key_str;
		CString in_str;
		CArray<CString> p;

		ReadOptions( &pcb_file );
		ReadFootprints( &pcb_file );
		ReadBoardOutline( &pcb_file );
		ReadSolderMaskCutouts( &pcb_file );
		m_plist->ReadParts( &pcb_file );
		m_nlist->ReadNets( &pcb_file );
		m_tlist->ReadTexts( &pcb_file );

		// make path to library folder and index libraries
		if( m_full_lib_dir == "" )
		{
//**			_chdir( m_app_dir );
			CString fullpath;
			char full[MAX_PATH];
			fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
			if( fullpath[fullpath.GetLength()-1] == '\\' )	
				fullpath = fullpath.Left(fullpath.GetLength()-1);
			m_full_lib_dir = fullpath;
		}
		MakeLibraryMaps( &m_full_lib_dir );

		m_pcb_full_path = *fn;
		int fpl = m_pcb_full_path.GetLength();
		int isep = m_pcb_full_path.ReverseFind( '\\' );
		if( isep == -1 )
			isep = m_pcb_full_path.ReverseFind( ':' );
		if( isep == -1 )
			ASSERT(0);		// unable to parse filename
		m_pcb_filename = m_pcb_full_path.Right( fpl - isep - 1);
		int fnl = m_pcb_filename.GetLength();
		m_path_to_folder = m_pcb_full_path.Left( m_pcb_full_path.GetLength() - fnl - 1 );
		m_window_title = "FreePCB - " + m_pcb_filename;
		CWnd* pMain = AfxGetMainWnd();
		pMain->SetWindowText( m_window_title );
		m_name = m_pcb_filename;
		if( m_name.Right(4) == ".fpc" )
			m_name = m_name.Left( m_name.GetLength() - 4 );
		if (pMain != NULL)
		{
			CMenu* pMenu = pMain->GetMenu();
			pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_ENABLED ); 
			pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_ENABLED ); 
			CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
			submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_ENABLED );	
			submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_ENABLED );	
			pMain->DrawMenuBar();
		}
		m_project_open = TRUE;
		theApp.AddMRUFile( &m_pcb_full_path );
		// now set layer visibility
		for( int i=0; i<m_num_layers; i++ )
		{
			m_dlist->SetLayerRGB( i, m_rgb[i][0], m_rgb[i][1], m_rgb[i][2] );
			m_dlist->SetLayerVisible( i, m_vis[i] );
		}
		// force redraw of function key text
		m_view->m_cursor_mode = 999;
		m_view->SetCursorMode( CUR_NONE_SELECTED );
		m_view->InvalidateLeftPane();
		m_view->Invalidate( FALSE );
		ProjectModified( FALSE );
		m_view->OnViewAllElements();
		m_auto_elapsed = 0;
		CDC * pDC = m_view->GetDC();
		m_view->OnDraw( pDC );
		m_view->ReleaseDC( pDC );
		bNoFilesOpened = FALSE;
		return;
	}
	catch( CString * err_str )
	{
		// parsing error
		AfxMessageBox( *err_str );
		delete err_str;
		CDC * pDC = m_view->GetDC();
		m_view->OnDraw( pDC );
		m_view->ReleaseDC( pDC );
		return;
	}
}

void CFreePcbDoc::OnFileClose()
{
	FileClose();
}

// return IDCANCEL if closing cancelled by user
//
int CFreePcbDoc::FileClose()
{
	if( m_project_open && m_project_modified )
	{
		int ret = AfxMessageBox( "Project modified, save it ? ", MB_YESNOCANCEL );
		if( ret == IDCANCEL )
			return IDCANCEL;
		else if( ret == IDYES )
			OnFileSave();
	}
	m_view->CancelSelection();

	// destroy existing project
	// delete undo list, partlist, netlist, displaylist, etc.
	m_sm_cutout.RemoveAll();
	m_drelist->Clear();
	m_undo_list->Clear();
	if( m_board_outline )
		delete m_board_outline;
	m_board_outline = NULL;
	m_nlist->RemoveAllNets();
	m_plist->RemoveAllParts();
	m_tlist->RemoveAllTexts();
	m_dlist->RemoveAll();

	// delete all shapes from local cache
	POSITION pos = m_footprint_cache_map.GetStartPosition();
	while( pos != NULL )
	{
		void * ptr;
		CShape * shape;
		CString string;
		m_footprint_cache_map.GetNextAssoc( pos, string, ptr );
		shape = (CShape*)ptr;
		delete shape;
	}
	m_footprint_cache_map.RemoveAll();
	CWnd* pMain = AfxGetMainWnd();
	if (pMain != NULL)
	{
		CMenu* pMenu = pMain->GetMenu();
		pMenu->EnableMenuItem( 1, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 2, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 3, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 4, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		pMenu->EnableMenuItem( 5, MF_BYPOSITION | MF_DISABLED | MF_GRAYED ); 
		CMenu* submenu = pMenu->GetSubMenu(0);	// "File" submenu
		submenu->EnableMenuItem( ID_FILE_SAVE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_SAVE_AS, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_IMPORT, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_EXPORTNETLIST, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		submenu->EnableMenuItem( ID_FILE_GENERATECADFILES, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED );	
		pMain->DrawMenuBar();
	}

	CFreePcbView * view = (CFreePcbView*)m_view;
	view->Invalidate( FALSE );
	m_project_open = FALSE;
	ProjectModified( FALSE );
	m_auto_elapsed = 0;
	// force redraw
	m_view->m_cursor_mode = 999;
	m_view->SetCursorMode( CUR_NONE_SELECTED );
	m_window_title = "FreePCB - no project open";
	pMain->SetWindowText( m_window_title );
	CDC * pDC = m_view->GetDC();
	m_view->OnDraw( pDC );
	m_view->ReleaseDC( pDC );
	return IDOK;
}

void CFreePcbDoc::OnFileSave() 
{
	if( theApp.m_view_mode == CFreePcbApp::FOOTPRINT )
	{
		theApp.m_View_fp->OnFootprintFileSaveAs();
		return;
	}

	m_plist->PurgeFootprintCache();
	FileSave();
	m_undo_list->Clear();	// can't undo after saving because cache purged
	bNoFilesOpened = FALSE;
}

void CFreePcbDoc::FileSave() 
{
	// write project file
	CStdioFile pcb_file;
	int err = pcb_file.Open( LPCSTR(m_pcb_full_path), CFile::modeCreate | CFile::modeWrite, NULL );
	if( !err )
	{
		// error opening file
		CString mess;
		mess.Format( "Unable to open file %s", LPCSTR(m_pcb_full_path) );
		AfxMessageBox( mess );
	}
	else
	{
		// write project to file
		try
		{
			WriteOptions( &pcb_file );
			WriteFootprints( &pcb_file );
			WriteBoardOutline( &pcb_file );
			WriteSolderMaskCutouts( &pcb_file );
			m_plist->WriteParts( &pcb_file );
			m_nlist->WriteNets( &pcb_file );
			m_tlist->WriteTexts( &pcb_file );
			pcb_file.WriteString( "[end]\n" );
			pcb_file.Close();
			theApp.AddMRUFile( &m_pcb_full_path );
			ProjectModified( FALSE );
			m_auto_elapsed = 0;
			bNoFilesOpened = FALSE;
		}
		catch( CString * err_str )
		{
			// error
			AfxMessageBox( *err_str );
			delete err_str;
			CDC * pDC = m_view->GetDC();
			m_view->OnDraw( pDC ) ;
			m_view->ReleaseDC( pDC );
			return;
		}
	}	
}

void CFreePcbDoc::OnFileSaveAs() 
{
	// force old-style file dialog by setting size of OPENFILENAME struct
	CFileDialog dlg( 0, "fpc", LPCSTR(*m_pcb_filename), 0, 
		"PCB files (*.fpc)|*.fpc|All Files (*.*)|*.*||",
		NULL, OPENFILENAME_SIZE_VERSION_400 );
	// get folder of most-recent file or project folder
	CString MRFile = theApp.GetMRUFile();
	CString MRFolder;
	if( MRFile != "" )
	{
		MRFolder = MRFile.Left( MRFile.ReverseFind( '\\' ) ) + "\\";
		dlg.m_ofn.lpstrInitialDir = MRFolder;
	}
	else
		dlg.m_ofn.lpstrInitialDir = m_parent_folder;
	int err = dlg.DoModal();
	if( err == IDOK )
	{
		CString pathname = dlg.GetPathName();
		// write project file
		CStdioFile pcb_file;
		int err = pcb_file.Open( pathname, CFile::modeCreate | CFile::modeWrite, NULL );
		if( !err )
		{
			// error opening partlist file
			CString mess;
			mess.Format( "Unable to open file %s", pathname );
			AfxMessageBox( mess );
		}
		else
		{
			// write project to file
			try
			{
				m_plist->PurgeFootprintCache();
				WriteOptions( &pcb_file );
				WriteFootprints( &pcb_file );
				WriteBoardOutline( &pcb_file );
				WriteSolderMaskCutouts( &pcb_file );
				m_plist->WriteParts( &pcb_file );
				m_nlist->WriteNets( &pcb_file );
				m_tlist->WriteTexts( &pcb_file );
				pcb_file.WriteString( "[end]\n" );
				pcb_file.Close();
				ProjectModified( FALSE );
				m_auto_elapsed = 0;
				m_undo_list->Clear();	// can't undo after saving because cache purged
			}
			catch( CString * err_str )
			{
				// error
				AfxMessageBox( *err_str );
				delete err_str;
				CDC * pDC = m_view->GetDC();
				m_view->OnDraw( pDC );
				m_view->ReleaseDC( pDC );
				return;
			}
			m_pcb_filename = dlg.GetFileName();
			m_pcb_full_path = pathname;
			int fnl = m_pcb_filename.GetLength();
			m_path_to_folder = m_pcb_full_path.Left( m_pcb_full_path.GetLength() - fnl - 1 );
			theApp.AddMRUFile( &m_pcb_full_path );
			m_window_title = "FreePCB - " + m_pcb_filename;
			CWnd* pMain = AfxGetMainWnd();
			pMain->SetWindowText( m_window_title );
		}
	}
}

void CFreePcbDoc::OnAddPart()
{
	// invoke dialog
	CDlgAddPart dlg;
	partlist_info pl;
	m_plist->ExportPartListInfo( &pl, NULL );
	dlg.Initialize( &pl, -1, TRUE, TRUE, FALSE, &m_footprint_cache_map, 
		&m_footlibfoldermap, m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// select new part, and start dragging it if requested
		m_plist->ImportPartListInfo( &pl, 0 );
		int n_parts = pl.GetSize();
		cpart * part = m_plist->GetPart( &pl[n_parts-1].ref_des );
		ProjectModified( TRUE );
		m_view->SaveUndoInfoForPart( part, CPartList::UNDO_PART_ADD );
		m_view->SelectPart( part );
		if( dlg.GetDragFlag() )
		{
			m_view->m_dragging_new_item = TRUE;
			m_view->OnPartMove();
		}
	}
}

void CFreePcbDoc::OnViewNetlist()
{
	CFreePcbView * view = (CFreePcbView*)m_view;
	CDlgNetlist dlg;
	dlg.Initialize( m_nlist, m_plist, &m_w, &m_v_w, &m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_undo_list->Clear();
		m_nlist->ImportNetListInfo( dlg.m_nl, 0, NULL, m_trace_w, m_via_w, m_via_hole_w );
		ProjectModified( TRUE );
		view->CancelSelection();
		m_nlist->OptimizeConnections();
		view->Invalidate( FALSE );
	}
}

// write footprint info from local cache to file
//
int CFreePcbDoc::WriteFootprints( CStdioFile * file )
{
	void * ptr;
	CShape * s;
	POSITION pos;
	CString key;

	file->WriteString( "[footprints]\n\n" );
	for( pos = m_footprint_cache_map.GetStartPosition(); pos != NULL; )
	{
		m_footprint_cache_map.GetNextAssoc( pos, key, ptr );
		s = (CShape*)ptr;
		s->WriteFootprint( file );
	}
	return 0;
}

// get shape from cache
// if necessary, make shape from library file and put into cache first
// returns NULL if shape not found
//
CShape * CFreePcbDoc::GetFootprintPtr( CString name )
{
	// lookup shape, first in cache
	void * ptr;
	int err = m_footprint_cache_map.Lookup( name, ptr );
	if( err )
	{
		// found in cache
		return (CShape*)ptr; 
	}
	else
	{
		// not in cache, lookup in library file
		int ilib;
		CString file_name;
		int offset;
		CString * project_lib_folder_str;
		project_lib_folder_str = m_footlibfoldermap.GetDefaultFolder();
		CFootLibFolder * project_footlibfolder = m_footlibfoldermap.GetFolder( project_lib_folder_str );
		BOOL ok = project_footlibfolder->GetFootprintInfo( &name, &ilib, NULL, NULL, &file_name, &offset );
		if( !ok )
		{
			// unable to find shape, return NULL
			return NULL;
		}
		else
		{
			// make shape from library file and put into cache
			CShape * shape = new CShape;
			CString lib_name = *project_footlibfolder->GetFullPath();
			err = shape->MakeFromFile( NULL, name, file_name, offset ); 
			if( err )
			{
				// failed
				CString mess;
				mess.Format( "Unable to make shape %s from file", name );
				AfxMessageBox( mess );
				return NULL;
			}
			else
			{
				// success, put into cache and return pointer
				m_footprint_cache_map.SetAt( name, shape );
				ProjectModified( TRUE );
				return shape;
			}
		}
	}
	return NULL;
}

// read shapes from file
//
void CFreePcbDoc::ReadFootprints( CStdioFile * pcb_file )
{
	// find beginning of shapes section
	ULONGLONG pos;
	int err;
	CString key_str;
	CString in_str;
	CArray<CString> p;

	// delete all shapes from local cache
	POSITION mpos = m_footprint_cache_map.GetStartPosition();
	while( mpos != NULL )
	{
		void * ptr;
		CShape * shape;
		CString string;
		m_footprint_cache_map.GetNextAssoc( mpos, string, ptr );
		shape = (CShape*)ptr;
		delete shape;
	}
	m_footprint_cache_map.RemoveAll();

	// find beginning of shapes section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess;
			mess.Format( "Unable to find [footprints] section in file" );
			AfxMessageBox( mess );
			return;
		}
		in_str.Trim();
	}
	while( in_str != "[shapes]" && in_str != "[footprints]" );

	// get each shape and add it to the cache
	while( 1 )
	{
		pos = pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString( "unexpected EOF in project file" );
			throw err_str;
		}
		in_str.Trim();
		if( in_str[0] == '[' )
		{
			pcb_file->Seek( pos, CFile::begin );
			break;		// next section, exit
		}
		else if( in_str.Left(5) == "name:" )
		{
			CString name = in_str.Right( in_str.GetLength()-5 );
			name.Trim();
			if( name.Right(1) == '\"' )
				name = name.Left( name.GetLength() - 1 );
			if( name.Left(1) == '\"' )
				name = name.Right( name.GetLength() - 1 );
			name = name.Left( CShape::MAX_NAME_SIZE );
			CShape * s = new CShape;
			pcb_file->Seek( pos, CFile::begin );	// back up
			err = s->MakeFromFile( pcb_file, "", "", 0 );
			if( !err )
				m_footprint_cache_map.SetAt( name, s );
			else
				delete s;
		}
	}
}

// write board outline to file
//
// throws CString * exception on error
//
void CFreePcbDoc::WriteBoardOutline( CStdioFile * file )
{
	CString line;

	try
	{
		line.Format( "[board]\n\n" );
		file->WriteString( line );
		if( m_board_outline )
		{
			line.Format( "outline: %d\n", m_board_outline->GetNumCorners() );
			file->WriteString( line );
		}
		else
		{
			file->WriteString( "outline: 0\n\n" );
			return;
		}
		for( int icor=0; icor<m_board_outline->GetNumCorners(); icor++ )
		{
			line.Format( "  corner: %d %d %d %d\n", icor+1,
				m_board_outline->GetX( icor ),
				m_board_outline->GetY( icor ),
				m_board_outline->GetSideStyle( icor )
				);
			file->WriteString( line );
		}
		file->WriteString( "\n" );
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString;
		if( e->m_lOsError == -1 )
			err_str->Format( "File error: %d\n", e->m_cause );
		else
			err_str->Format( "File error: %d %ld (%s)\n", 
				e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteBoardOutline()\n" + *err_str;
		throw err_str;
	}
}
void CFreePcbDoc::WriteSolderMaskCutouts( CStdioFile * file )
{
	CString line;

	try
	{
		line.Format( "[solder_mask_cutouts]\n\n" );
		file->WriteString( line );
		for( int i=0; i<m_sm_cutout.GetSize(); i++ )
		{
			line.Format( "sm_cutout: %d %d %d\n", m_sm_cutout[i].GetNumCorners(),
				m_sm_cutout[i].GetHatch(), m_sm_cutout[i].GetLayer() );
			file->WriteString( line );
			for( int icor=0; icor<m_sm_cutout[i].GetNumCorners(); icor++ )
			{
				line.Format( "  corner: %d %d %d %d\n", icor+1,
					m_sm_cutout[i].GetX( icor ),
					m_sm_cutout[i].GetY( icor ),
					m_sm_cutout[i].GetSideStyle( icor )
					);
				file->WriteString( line );
			}
			file->WriteString( "\n" );
		}
		file->WriteString( "\n" );
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString;
		if( e->m_lOsError == -1 )
			err_str->Format( "File error: %d\n", e->m_cause );
		else
			err_str->Format( "File error: %d %ld (%s)\n", 
				e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteSolderMaskCutouts()\n" + *err_str;
		throw err_str;
	}
}

// read board outline from file
//
// throws CString * exception on error
//
void CFreePcbDoc::ReadBoardOutline( CStdioFile * pcb_file )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;
	int last_side_style = CPolyLine::STRAIGHT;

	try
	{
		// find beginning of [board] section
		do
		{
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				// error reading pcb file
				CString mess;
				mess.Format( "Unable to find [board] section in file" );
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str != "[board]" );

		// get data
		while( 1 )
		{
			pos = pcb_file->GetPosition();
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				CString * err_str = new CString( "unexpected EOF in project file" );
				throw err_str;
			}
			in_str.Trim();
			if( in_str[0] == '[' )
			{
				// normal return
				pcb_file->Seek( pos, CFile::begin );
				return;
			}
			np = ParseKeyString( &in_str, &key_str, &p );
			if( np && key_str == "outline" )
			{
				if( np != 2 )
				{
					CString * err_str = new CString( "error parsing [board] section of project file" );
					throw err_str;
				}
				int ncorners = my_atoi( &p[0] );
				for( int icor=0; icor<ncorners; icor++ )
				{
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString( "unexpected EOF in project file" );
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "corner" || (np != 4 && np != 5) )
					{
						CString * err_str = new CString( "error parsing [board] section of project file" );
						throw err_str;
					}
					int ncor = my_atoi( &p[0] );
					if( (ncor-1) != icor )
					{
						CString * err_str = new CString( "error parsing [board] section of project file" );
						throw err_str;
					}
					int x = my_atoi( &p[1] );
					int y = my_atoi( &p[2] );
					if( icor == 0 )
					{
						// make new board outline 
						if( m_board_outline )
							delete m_board_outline;
						m_board_outline = new CPolyLine( m_dlist );
						id bid( ID_BOARD, ID_BOARD_OUTLINE );
						m_board_outline->Start( LAY_BOARD_OUTLINE, 1, 20*NM_PER_MIL, x, y, 
							0, &bid, NULL );
					}
					else
						m_board_outline->AppendCorner( x, y, last_side_style );
					if( np == 5 )
						last_side_style = my_atoi( &p[3] );
					else
						last_side_style = CPolyLine::STRAIGHT;
					if( icor == (ncorners-1) )
						m_board_outline->Close( last_side_style );
				}
			}
		}
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString;
		if( e->m_lOsError == -1 )
			err_str->Format( "File error: %d\n", e->m_cause );
		else
			err_str->Format( "File error: %d %ld (%s)\n", 
				e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteBoardOutline()\n" + *err_str;
		throw err_str;
	}
}

// read solder mask cutouts from file
//
// throws CString * exception on error
//
void CFreePcbDoc::ReadSolderMaskCutouts( CStdioFile * pcb_file )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;
	int last_side_style = CPolyLine::STRAIGHT;

	try
	{
		// find beginning of [solder_mask_cutouts] section
		int pos = pcb_file->GetPosition();
		do
		{
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				// error reading pcb file
				CString mess;
				mess.Format( "Unable to find [solder_mask_cutouts] section in file" );
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str[0] != '[' );

		if( in_str != "[solder_mask_cutouts]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			return;
		}

		// get data
		while( 1 )
		{
			pos = pcb_file->GetPosition();
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				CString * err_str = new CString( "unexpected EOF in project file" );
				throw err_str;
			}
			in_str.Trim();
			if( in_str[0] == '[' )
			{
				// normal return
				pcb_file->Seek( pos, CFile::begin );
				return;
			}
			np = ParseKeyString( &in_str, &key_str, &p );
			if( np && key_str == "sm_cutout" )
			{
				if( np != 4 ) 
				{
					CString * err_str = new CString( "error parsing [solder_mask_cutouts] section of project file" );
					throw err_str;
				}
				int ncorners = my_atoi( &p[0] );
				int hatch = my_atoi( &p[1] );
				int lay = my_atoi( &p[2] );
				int ic = m_sm_cutout.GetSize();
				m_sm_cutout.SetSize(ic+1);
				for( int icor=0; icor<ncorners; icor++ )
				{
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString( "unexpected EOF in project file" );
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "corner" || (np != 4 && np != 5) )
					{
						CString * err_str = new CString( "error parsing [solder_mask_cutouts] section of project file" );
						throw err_str;
					}
					int ncor = my_atoi( &p[0] );
					if( (ncor-1) != icor )
					{
						CString * err_str = new CString( "error parsing [solder_mask_cutouts] section of project file" );
						throw err_str;
					}
					int x = my_atoi( &p[1] );
					int y = my_atoi( &p[2] );
					id id_sm( ID_SM_CUTOUT, ID_SM_CUTOUT, ic );
					if( icor == 0 )
					{
						// make new cutout 
						m_sm_cutout[ic].Start( lay, 0, 10*NM_PER_MIL, x, y, hatch, &id_sm, NULL );
						m_sm_cutout[ic].SetDisplayList( m_dlist );
					}
					else
						m_sm_cutout[ic].AppendCorner( x, y, last_side_style );
					if( np == 5 )
						last_side_style = my_atoi( &p[3] );
					else
						last_side_style = CPolyLine::STRAIGHT;
					if( icor == (ncorners-1) )
						m_sm_cutout[ic].Close( last_side_style );
				}
			}
		}
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString;
		if( e->m_lOsError == -1 )
			err_str->Format( "File error: %d\n", e->m_cause );
		else
			err_str->Format( "File error: %d %ld (%s)\n", 
				e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::ReadSolderMaskCutouts()\n" + *err_str;
		throw err_str;
	}
}

// read project options from file
//
// throws CString * exception on error
//
void CFreePcbDoc::ReadOptions( CStdioFile * pcb_file )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;

	// initalize
	CFreePcbView * view = (CFreePcbView*)m_view;
	m_visible_grid.SetSize( 0 );
	m_part_grid.SetSize( 0 );
	m_routing_grid.SetSize( 0 );
	m_fp_visible_grid.SetSize( 0 );
	m_fp_part_grid.SetSize( 0 );
	m_name = "";
	m_auto_interval = 0;
	m_dr.bCheckUnrouted = FALSE;
	for( int i=0; i<MAX_LAYERS; i++ )
	{
		m_file_layer_by_layer[i] = i;
		m_layer_by_file_layer[i] = i;
	}

	try
	{
		// find beginning of [options] section
		do
		{
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				// error reading pcb file
				CString mess;
				mess.Format( "Unable to find [options] section in file" );
				AfxMessageBox( mess );
				return;
			}
			in_str.Trim();
		}
		while( in_str != "[options]" );

		// get data
		while( 1 )
		{
			pos = pcb_file->GetPosition();
			err = pcb_file->ReadString( in_str );
			if( !err )
			{
				CString * err_str = new CString( "unexpected EOF in project file" );
				throw err_str;
			}
			in_str.Trim();
			if( in_str[0] == '[' )
			{
				// normal return
				pcb_file->Seek( pos, CFile::begin );
				break;
			}
			np = ParseKeyString( &in_str, &key_str, &p );
			if( np == 2 && key_str == "project_name" )
			{
				m_name = p[0];
			}
			if( np == 2 && key_str == "file_version" )
			{
				double file_version = my_atof( &p[0] );
				if( file_version > m_version )
				{
					CString mess;
					mess.Format( "Warning: the file version is %5.3f\n\nYou are running an earlier FreePCB version %5.3f", 
						file_version, m_version );
					mess += "\n\nErrors may occur\n\nClick on OK to continue reading or CANCEL to cancel";
					int ret = AfxMessageBox( mess, MB_OKCANCEL );
					if( ret == IDCANCEL )
					{
						CString * err_str = new CString( "Reading project file cancelled by user" );
						throw err_str;
					}
				}
			}
			if( np && key_str == "parent_folder" )
			{
				m_parent_folder = p[0];
			}
			if( np && key_str == "library_folder" )
			{
				m_lib_dir = p[0];
			}
			if( np && key_str == "full_library_folder" )
			{
				m_full_lib_dir = p[0];
			}
			else if( np && key_str == "n_copper_layers" )
			{
				m_num_copper_layers = my_atoi( &p[0] );
				m_plist->SetNumCopperLayers( m_num_copper_layers );
				m_nlist->SetNumCopperLayers( m_num_copper_layers );
				m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
			}
			else if( np && key_str == "autosave_interval" )
			{
				m_auto_interval = my_atoi( &p[0] );
			}
			else if( np && key_str == "units" )
			{
				if( p[0] == "MM" )
					m_units = MM;
				else
					m_units = MIL;
			}
			else if( np && key_str == "visible_grid_spacing" )
			{
				m_visual_grid_spacing = my_atof( &p[0] );
			}
			else if( np && key_str == "visible_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_visible_grid.Add( -value );
				else
					m_visible_grid.Add( value );
			}
			else if( np && key_str == "placement_grid_spacing" )
			{
				m_part_grid_spacing = my_atof( &p[0] );
			}
			else if( np && key_str == "placement_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_part_grid.Add( -value );
				else
					m_part_grid.Add( value );
			}
			else if( np && key_str == "routing_grid_spacing" )
			{
				m_routing_grid_spacing = my_atof( &p[0] );
			}
			else if( np && key_str == "routing_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_routing_grid.Add( -value );
				else
					m_routing_grid.Add( value );
			}
			else if( np && key_str == "snap_angle" )
			{
				m_snap_angle = my_atof( &p[0] );
			}
			else if( np && key_str == "fp_visible_grid_spacing" )
			{
				m_fp_visual_grid_spacing = my_atof( &p[0] );
			}
			else if( np && key_str == "fp_visible_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_fp_visible_grid.Add( -value );
				else
					m_fp_visible_grid.Add( value );
			}
			else if( np && key_str == "fp_placement_grid_spacing" )
			{
				m_fp_part_grid_spacing = my_atof( &p[0] );
			}
			else if( np && key_str == "fp_placement_grid_item" )
			{
				CString str;
				double value;
				if( np == 3 )
					str = p[1];
				else
					str = p[0];
				value = my_atof( &str );
				if( str.Right(2) == "MM" || str.Right(2) == "mm" )
					m_fp_part_grid.Add( -value );
				else
					m_fp_part_grid.Add( value );
			}
			else if( np && key_str == "fp_snap_angle" )
			{
				m_fp_snap_angle = my_atof( &p[0] );
			}
			// CAM stuff
			else if( np && key_str == "fill_clearance" )
			{
				m_fill_clearance = my_atoi( &p[0] );
			}
			else if( np && key_str == "mask_clearance" )
			{
				m_mask_clearance = my_atoi( &p[0] );
			}
			else if( np && key_str == "thermal_width" )
			{
				m_thermal_width = my_atoi( &p[0] );
			}
			else if( np && key_str == "min_silkscreen_width" )
			{
				m_min_silkscreen_stroke_wid = my_atoi( &p[0] );
			}
			else if( np && key_str == "pilot_diameter" )
			{
				m_pilot_diameter = my_atoi( &p[0] );
			}
			else if( np && key_str == "board_outline_width" )
			{
				m_outline_width = my_atoi( &p[0] );
			}
			else if( np && key_str == "hole_clearance" )
			{
				m_hole_clearance = my_atoi( &p[0] );
			}
			else if( np && key_str == "annular_ring_for_pins" )
			{
				m_annular_ring_pins = my_atoi( &p[0] );
			}
			else if( np && key_str == "annular_ring_for_vias" )
			{
				m_annular_ring_vias = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_flags" )
			{
				m_cam_flags = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_layers" )
			{
				m_cam_layers = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_drill_file" )
			{
				m_cam_drill_file = my_atoi( &p[0] );
			}
			else if( np && key_str == "cam_units" )
			{
				m_cam_units = my_atoi( &p[0] );
			}
			// DRC stuff
			else if( np && key_str == "drc_check_unrouted" )
			{
				m_dr.bCheckUnrouted = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_trace_width" )
			{
				m_dr.trace_width = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_pad_pad" )
			{
				m_dr.pad_pad = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_pad_trace" )
			{
				m_dr.pad_trace = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_trace_trace" )
			{
				m_dr.trace_trace = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_hole_copper" )
			{
				m_dr.hole_copper = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_annular_ring_pins" )
			{
				m_dr.annular_ring_pins = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_annular_ring_vias" )
			{
				m_dr.annular_ring_vias = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_board_edge_copper" )
			{
				m_dr.board_edge_copper = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_board_edge_hole" )
			{
				m_dr.board_edge_hole = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_hole_hole" )
			{
				m_dr.hole_hole = my_atoi( &p[0] );
			}
			else if( np && key_str == "drc_copper_copper" )
			{
				m_dr.copper_copper = my_atoi( &p[0] );
			}

			else if( np && key_str == "default_trace_width" )
			{
				m_trace_w = my_atoi( &p[0] );
			}
			else if( np && key_str == "default_via_pad_width" )
			{
				m_via_w = my_atoi( &p[0] );
			}
			else if( np && key_str == "default_via_hole_width" )
			{
				m_via_hole_w = my_atoi( &p[0] );
			}
			else if( np && key_str == "n_width_menu" )
			{
				int n = my_atoi( &p[0] );
				m_w.SetSize( n );
				m_v_w.SetSize( n );
				m_v_h_w.SetSize( n );
				for( int i=0; i<n; i++ )
				{
					pos = pcb_file->GetPosition();
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString( "unexpected EOF in project file" );
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( np != 5 || key_str != "width_menu_item" )
					{
						CString * err_str = new CString( "error parsing [options] section of project file" );
						throw err_str;
					}
					int ig = my_atoi( &p[0] ) - 1;
					if( ig != i )
					{
						CString * err_str = new CString( "error parsing [options] section of project file" );
						throw err_str;
					}
					m_w[i] = my_atoi( &p[1] );
					m_v_w[i] = my_atoi( &p[2] );
					m_v_h_w[i] = my_atoi( &p[3] );
				}
			}
			else if( np && key_str == "layer_info" )
			{
				CString file_layer_name = p[0];
				int i = my_atoi( &p[1] );
				int layer = -1;
				for( int il=0; il<MAX_LAYERS; il++ )
				{
					CString layer_string = &layer_str[il][0];
					if( file_layer_name == layer_string )
					{
						SetFileLayerMap( i, il );
						layer = il;
						break;
					}
				}
				if( layer < 0 )
				{
					AfxMessageBox( "Warning: layer \"" + file_layer_name + "\" not supported" );
				}
				else
				{
					m_rgb[layer][0] = my_atoi( &p[2] );
					m_rgb[layer][1] = my_atoi( &p[3] );
					m_rgb[layer][2] = my_atoi( &p[4] );
					m_vis[layer] = my_atoi( &p[5] );
				}
			}
		}
		if( m_fp_visible_grid.GetSize() == 0 )
		{
			m_fp_visual_grid_spacing = m_visual_grid_spacing;
			for( int i=0; i<m_visible_grid.GetSize(); i++ )
				m_fp_visible_grid.Add( m_visible_grid[i] );
		}
		if( m_fp_part_grid.GetSize() == 0 )
		{
			m_fp_part_grid_spacing = m_part_grid_spacing;
			for( int i=0; i<m_part_grid.GetSize(); i++ )
				m_fp_part_grid.Add( m_part_grid[i] );
		}
		if( m_fp_snap_angle != 0 && m_fp_snap_angle != 45 && m_fp_snap_angle != 90 )
			m_fp_snap_angle = m_snap_angle;
		CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
		frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
			m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, m_units );
		m_dlist->SetVisibleGrid( TRUE, m_visual_grid_spacing );
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString;
		if( e->m_lOsError == -1 )
			err_str->Format( "File error: %d\n", e->m_cause );
		else
			err_str->Format( "File error: %d %ld (%s)\n", 
				e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteOptions()\n" + *err_str;
		throw err_str;
	}
}

// write project options to file
//
// throws CString * exception on error
//
void CFreePcbDoc::WriteOptions( CStdioFile * file )
{
	CString line;

	try
	{
		CString str;
		CFreePcbView * view = (CFreePcbView*)m_view;
		line.Format( "[options]\n\n" );
		file->WriteString( line );
		line.Format( "version: %5.3f\n", m_version );
		file->WriteString( line );
		line.Format( "file_version: %5.3f\n", m_file_version );
		file->WriteString( line );
		line.Format( "project_name: \"%s\"\n", m_name );
		file->WriteString( line );
		line.Format( "full_library_folder: \"%s\"\n", m_full_lib_dir );
		file->WriteString( line );
		line.Format( "CAM_folder: \"%s\"\n\n", m_cam_full_path );
		file->WriteString( line );
		line.Format( "autosave_interval: %d\n\n", m_auto_interval );
		file->WriteString( line );
		if( m_units == MIL )
			file->WriteString( "units: MIL\n\n" );
		else
			file->WriteString( "units: MM\n\n" );
		line.Format( "visible_grid_spacing: %f\n", m_visual_grid_spacing );
		file->WriteString( line );
		for( int i=0; i<m_visible_grid.GetSize(); i++ )
		{
			if( m_visible_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_visible_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_visible_grid[i], MM );
			file->WriteString( "  visible_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "placement_grid_spacing: %f\n", m_part_grid_spacing );
		file->WriteString( line );
		for( int i=0; i<m_part_grid.GetSize(); i++ )
		{
			if( m_part_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_part_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_part_grid[i], MM );
			file->WriteString( "  placement_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "routing_grid_spacing: %f\n", m_routing_grid_spacing );
		file->WriteString( line );
		for( int i=0; i<m_routing_grid.GetSize(); i++ )
		{
			if( m_routing_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_routing_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_routing_grid[i], MM );
			file->WriteString( "  routing_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "snap_angle: %d\n", m_snap_angle );
		file->WriteString( line );
		file->WriteString( "\n" );
		line.Format( "fp_visible_grid_spacing: %f\n", m_fp_visual_grid_spacing );
		file->WriteString( line );
		for( int i=0; i<m_fp_visible_grid.GetSize(); i++ )
		{
			if( m_fp_visible_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_fp_visible_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_fp_visible_grid[i], MM );
			file->WriteString( "  fp_visible_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "fp_placement_grid_spacing: %f\n", m_fp_part_grid_spacing );
		file->WriteString( line );
		for( int i=0; i<m_fp_part_grid.GetSize(); i++ )
		{
			if( m_fp_part_grid[i] > 0 )
				::MakeCStringFromDimension( &str, m_fp_part_grid[i], MIL );
			else
				::MakeCStringFromDimension( &str, -m_fp_part_grid[i], MM );
			file->WriteString( "  fp_placement_grid_item: " + str + "\n" );
		}
		file->WriteString( "\n" );
		line.Format( "fp_snap_angle: %d\n", m_fp_snap_angle );
		file->WriteString( line );
		file->WriteString( "\n" );
		line.Format( "fill_clearance: %d\n", m_fill_clearance );
		file->WriteString( line );
		line.Format( "mask_clearance: %d\n", m_mask_clearance );
		file->WriteString( line );
		line.Format( "thermal_width: %d\n", m_thermal_width );
		file->WriteString( line );
		line.Format( "min_silkscreen_width: %d\n", m_min_silkscreen_stroke_wid );
		file->WriteString( line );
		line.Format( "board_outline_width: %d\n", m_outline_width );
		file->WriteString( line );
		line.Format( "hole_clearance: %d\n", m_hole_clearance );
		file->WriteString( line );
		line.Format( "pilot_diameter: %d\n", m_pilot_diameter );
		file->WriteString( line );
		line.Format( "annular_ring_for_pins: %d\n", m_annular_ring_pins );
		file->WriteString( line );
		line.Format( "annular_ring_for_vias: %d\n", m_annular_ring_vias );
		file->WriteString( line );
		line.Format( "cam_flags: %d\n", m_cam_flags );
		file->WriteString( line );
		line.Format( "cam_layers: %d\n", m_cam_layers );
		file->WriteString( line );
		line.Format( "cam_drill_file: %d\n", m_cam_drill_file );
		file->WriteString( line );
		line.Format( "cam_units: %d\n", m_cam_units );
		file->WriteString( line );
		file->WriteString( "\n" );

		line.Format( "drc_check_unrouted: %d\n", m_dr.bCheckUnrouted );
		file->WriteString( line );
		line.Format( "drc_trace_width: %d\n", m_dr.trace_width );
		file->WriteString( line );
		line.Format( "drc_pad_pad: %d\n", m_dr.pad_pad );
		file->WriteString( line );
		line.Format( "drc_pad_trace: %d\n", m_dr.pad_trace );
		file->WriteString( line );
		line.Format( "drc_trace_trace: %d\n", m_dr.trace_trace );
		file->WriteString( line );
		line.Format( "drc_hole_copper: %d\n", m_dr.hole_copper );
		file->WriteString( line );
		line.Format( "drc_annular_ring_pins: %d\n", m_dr.annular_ring_pins );
		file->WriteString( line );
		line.Format( "drc_annular_ring_vias: %d\n", m_dr.annular_ring_vias );
		file->WriteString( line );
		line.Format( "drc_board_edge_copper: %d\n", m_dr.board_edge_copper );
		file->WriteString( line );
		line.Format( "drc_board_edge_hole: %d\n", m_dr.board_edge_hole );
		file->WriteString( line );
		line.Format( "drc_hole_hole: %d\n", m_dr.hole_hole );
		file->WriteString( line );
		line.Format( "drc_copper_copper: %d\n", m_dr.copper_copper );
		file->WriteString( line );
		file->WriteString( "\n" );

		line.Format( "default_trace_width: %d\n", m_trace_w );
		file->WriteString( line );
		line.Format( "default_via_pad_width: %d\n", m_via_w );
		file->WriteString( line );
		line.Format( "default_via_hole_width: %d\n", m_via_hole_w );
		file->WriteString( line );
		line.Format( "n_width_menu: %d\n", m_w.GetSize() );
		file->WriteString( line );
		for( int i=0; i<m_w.GetSize(); i++ )
		{
			line.Format( "  width_menu_item: %d %d %d %d\n", i+1, m_w[i], m_v_w[i], m_v_h_w[i]  );
			file->WriteString( line );
		}
		file->WriteString( "\n" );
		line.Format( "n_copper_layers: %d\n", m_num_copper_layers );
		file->WriteString( line );
		for( int i=0; i<(LAY_TOP_COPPER+m_num_copper_layers); i++ )
		{
			line.Format( "  layer_info: \"%s\" %d %d %d %d %d\n",
				&layer_str[i][0], i,
				m_rgb[i][0], m_rgb[i][1], m_rgb[i][2], m_vis[i] );
			file->WriteString( line );
		}
		file->WriteString( "\n" );
		return;
	}
	catch( CFileException * e )
	{
		CString * err_str = new CString;
		if( e->m_lOsError == -1 )
			err_str->Format( "File error: %d\n", e->m_cause );
		else
			err_str->Format( "File error: %d %ld (%s)\n", 
				e->m_cause, e->m_lOsError, _sys_errlist[e->m_lOsError] );
		*err_str = "CFreePcbDoc::WriteBoardOutline()\n" + *err_str;
		throw err_str;
	}
}

// set defaults for a new project
//
void CFreePcbDoc::InitializeNewProject()
{
	// these are the embedded defaults
	m_name = "";
	m_path_to_folder = "..\\projects\\";
	m_lib_dir = "..\\lib\\" ;
	m_pcb_filename = "";
	m_pcb_full_path = "";
	m_board_outline = NULL;
	m_units = MIL;
	m_num_copper_layers = 4;
	m_plist->SetNumCopperLayers( m_num_copper_layers );
	m_nlist->SetNumCopperLayers( m_num_copper_layers );
	m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
	m_layer_mask = 0x0000007f;
	m_active_layer = LAY_TOP_COPPER;
	m_auto_interval = 0;
	m_sm_cutout.RemoveAll();

	// colors for layers
	for( int i=0; i<MAX_LAYERS; i++ )
	{
		m_vis[i] = 0;
		m_rgb[i][0] = 127; 
		m_rgb[i][1] = 127; 
		m_rgb[i][2] = 127;			// default grey
	}
	m_rgb[LAY_BACKGND][0] = 0; 
	m_rgb[LAY_BACKGND][1] = 0; 
	m_rgb[LAY_BACKGND][2] = 0;			// background BLACK
	m_rgb[LAY_VISIBLE_GRID][0] = 255; 
	m_rgb[LAY_VISIBLE_GRID][1] = 255; 
	m_rgb[LAY_VISIBLE_GRID][2] = 255;	// visible grid WHITE 
	m_rgb[LAY_HILITE][0] = 255; 
	m_rgb[LAY_HILITE][1] = 255; 
	m_rgb[LAY_HILITE][2] = 255;			//highlight WHITE
	m_rgb[LAY_DRC_ERROR][0] = 255; 
	m_rgb[LAY_DRC_ERROR][1] = 128; 
	m_rgb[LAY_DRC_ERROR][2] = 64;		// DRC error ORANGE
	m_rgb[LAY_BOARD_OUTLINE][0] = 0; 
	m_rgb[LAY_BOARD_OUTLINE][1] = 0; 
	m_rgb[LAY_BOARD_OUTLINE][2] = 255;	//board outline BLUE
	m_rgb[LAY_SELECTION][0] = 255; 
	m_rgb[LAY_SELECTION][1] = 255; 
	m_rgb[LAY_SELECTION][2] = 255;		//selection WHITE
	m_rgb[LAY_SILK_TOP][0] = 255; 
	m_rgb[LAY_SILK_TOP][1] = 255; 
	m_rgb[LAY_SILK_TOP][2] =   0;		//top silk YELLOW
	m_rgb[LAY_SILK_BOTTOM][0] = 255; 
	m_rgb[LAY_SILK_BOTTOM][1] = 192; 
	m_rgb[LAY_SILK_BOTTOM][2] = 192;	//bottom silk PINK
	m_rgb[LAY_SM_TOP][0] =   160; 
	m_rgb[LAY_SM_TOP][1] =   160; 
	m_rgb[LAY_SM_TOP][2] =   160;		//top solder mask cutouts LIGHT GREY
	m_rgb[LAY_SM_BOTTOM][0] = 95; 
	m_rgb[LAY_SM_BOTTOM][1] = 95; 
	m_rgb[LAY_SM_BOTTOM][2] = 95;	//bottom solder mask cutouts DARK GREY
	m_rgb[LAY_PAD_THRU][0] =   0; 
	m_rgb[LAY_PAD_THRU][1] =   0; 
	m_rgb[LAY_PAD_THRU][2] = 255;		//thru-hole pads BLUE
	m_rgb[LAY_RAT_LINE][0] = 255; 
	m_rgb[LAY_RAT_LINE][1] = 0; 
	m_rgb[LAY_RAT_LINE][2] = 255;		//ratlines VIOLET
	m_rgb[LAY_TOP_COPPER][0] =   0; 
	m_rgb[LAY_TOP_COPPER][1] = 255; 
	m_rgb[LAY_TOP_COPPER][2] =   0;		//top copper GREEN
	m_rgb[LAY_BOTTOM_COPPER][0] = 255; 
	m_rgb[LAY_BOTTOM_COPPER][1] =   0; 
	m_rgb[LAY_BOTTOM_COPPER][2] =   0;	//bottom copper RED
	m_rgb[LAY_BOTTOM_COPPER+1][0] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+1][1] = 128; 
	m_rgb[LAY_BOTTOM_COPPER+1][2] = 64;	
	m_rgb[LAY_BOTTOM_COPPER+2][0] = 128; // inner 1 
	m_rgb[LAY_BOTTOM_COPPER+2][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+2][2] = 64;	
	m_rgb[LAY_BOTTOM_COPPER+3][0] = 64; // inner 2
	m_rgb[LAY_BOTTOM_COPPER+3][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+3][2] = 128;	
	m_rgb[LAY_BOTTOM_COPPER+4][0] = 64; // inner 3
	m_rgb[LAY_BOTTOM_COPPER+4][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+4][2] = 64;	
	m_rgb[LAY_BOTTOM_COPPER+5][0] = 64; // inner 5
	m_rgb[LAY_BOTTOM_COPPER+5][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+5][2] = 64;	
	m_rgb[LAY_BOTTOM_COPPER+6][0] = 64; // inner 6 
	m_rgb[LAY_BOTTOM_COPPER+6][1] = 64; 
	m_rgb[LAY_BOTTOM_COPPER+6][2] = 64;	

	// now set layer colors and visibility
	for( int i=0; i<m_num_layers; i++ )
	{
		m_vis[i] = 1;
		m_dlist->SetLayerRGB( i, m_rgb[i][0], m_rgb[i][1], m_rgb[i][2] );
		m_dlist->SetLayerVisible( i, m_vis[i] );
	}

	// colors for footprint editor layers
	m_fp_num_layers = LAY_FP_BOTTOM_COPPER + 1;
	m_fp_rgb[LAY_FP_SELECTION][0] = 255; 
	m_fp_rgb[LAY_FP_SELECTION][1] = 255; 
	m_fp_rgb[LAY_FP_SELECTION][2] = 255;		//selection WHITE
	m_fp_rgb[LAY_FP_BACKGND][0] = 0; 
	m_fp_rgb[LAY_FP_BACKGND][1] = 0; 
	m_fp_rgb[LAY_FP_BACKGND][2] = 0;			// background BLACK
	m_fp_rgb[LAY_FP_VISIBLE_GRID][0] = 255; 
	m_fp_rgb[LAY_FP_VISIBLE_GRID][1] = 255; 
	m_fp_rgb[LAY_FP_VISIBLE_GRID][2] = 255;	// visible grid WHITE 
	m_fp_rgb[LAY_FP_HILITE][0] = 255; 
	m_fp_rgb[LAY_FP_HILITE][1] = 255; 
	m_fp_rgb[LAY_FP_HILITE][2] = 255;		//highlight WHITE
	m_fp_rgb[LAY_FP_SILK_TOP][0] = 255; 
	m_fp_rgb[LAY_FP_SILK_TOP][1] = 255; 
	m_fp_rgb[LAY_FP_SILK_TOP][2] =   0;		//top silk YELLOW
	m_fp_rgb[LAY_FP_PAD_THRU][0] =   0; 
	m_fp_rgb[LAY_FP_PAD_THRU][1] =   0; 
	m_fp_rgb[LAY_FP_PAD_THRU][2] = 255;		//thru-hole pads BLUE
	m_fp_rgb[LAY_FP_TOP_COPPER][0] =   0; 
	m_fp_rgb[LAY_FP_TOP_COPPER][1] = 255; 
	m_fp_rgb[LAY_FP_TOP_COPPER][2] =   0;		//top copper GREEN
	m_fp_rgb[LAY_FP_INNER_COPPER][0] =  128; 
	m_fp_rgb[LAY_FP_INNER_COPPER][1] = 128; 
	m_fp_rgb[LAY_FP_INNER_COPPER][2] =  128;		//inner copper GREY
	m_fp_rgb[LAY_FP_BOTTOM_COPPER][0] = 255; 
	m_fp_rgb[LAY_FP_BOTTOM_COPPER][1] = 0; 
	m_fp_rgb[LAY_FP_BOTTOM_COPPER][2] = 0;		//bottom copper RED

	// default visible grid spacing menu values (in NM)
	m_visible_grid.RemoveAll();
	m_visible_grid.Add( 100*NM_PER_MIL );
	m_visible_grid.Add( 125*NM_PER_MIL );
	m_visible_grid.Add( 200*NM_PER_MIL );	// default index = 2
	m_visible_grid.Add( 250*NM_PER_MIL );
	m_visible_grid.Add( 400*NM_PER_MIL );
	m_visible_grid.Add( 500*NM_PER_MIL );
	m_visible_grid.Add( 1000*NM_PER_MIL );
//	int visible_index = 2;
	m_visual_grid_spacing = 200*NM_PER_MIL;
	m_dlist->SetVisibleGrid( TRUE, m_visual_grid_spacing );

	// default placement grid spacing menu values (in NM)
	m_part_grid.RemoveAll();
	m_part_grid.Add( 10*NM_PER_MIL );
	m_part_grid.Add( 20*NM_PER_MIL );
	m_part_grid.Add( 25*NM_PER_MIL );
	m_part_grid.Add( 40*NM_PER_MIL );
	m_part_grid.Add( 50*NM_PER_MIL );		// default
	m_part_grid.Add( 100*NM_PER_MIL );
	m_part_grid.Add( 200*NM_PER_MIL );
	m_part_grid.Add( 250*NM_PER_MIL );
	m_part_grid.Add( 400*NM_PER_MIL );
	m_part_grid.Add( 500*NM_PER_MIL );
	m_part_grid.Add( 1000*NM_PER_MIL );
	m_part_grid_spacing = 50*NM_PER_MIL;

	// default routing grid spacing menu values (in NM)
	m_routing_grid.RemoveAll();
	m_routing_grid.Add( 1*NM_PER_MIL );
	m_routing_grid.Add( 2*NM_PER_MIL );
	m_routing_grid.Add( 2.5*NM_PER_MIL );
	m_routing_grid.Add( 3.333333333333*NM_PER_MIL );
	m_routing_grid.Add( 4*NM_PER_MIL );
	m_routing_grid.Add( 5*NM_PER_MIL );
	m_routing_grid.Add( 8.333333333333*NM_PER_MIL );
	m_routing_grid.Add( 10*NM_PER_MIL );	// default
	m_routing_grid.Add( 16.66666666666*NM_PER_MIL );
	m_routing_grid.Add( 20*NM_PER_MIL );
	m_routing_grid.Add( 25*NM_PER_MIL );
	m_routing_grid.Add( 40*NM_PER_MIL );
	m_routing_grid.Add( 50*NM_PER_MIL );
	m_routing_grid.Add( 100*NM_PER_MIL );
	m_routing_grid_spacing = 10*NM_PER_MIL;

	// footprint editor parameters
	m_fp_units = MIL;

	// default footprint editor visible grid spacing menu values (in NM)
	m_fp_visible_grid.RemoveAll();
	m_fp_visible_grid.Add( 100*NM_PER_MIL );
	m_fp_visible_grid.Add( 125*NM_PER_MIL );
	m_fp_visible_grid.Add( 200*NM_PER_MIL );	
	m_fp_visible_grid.Add( 250*NM_PER_MIL );
	m_fp_visible_grid.Add( 400*NM_PER_MIL );
	m_fp_visual_grid_spacing = 200*NM_PER_MIL;

	// default footprint editor placement grid spacing menu values (in NM)
	m_fp_part_grid.RemoveAll();
	m_fp_part_grid.Add( 10*NM_PER_MIL );
	m_fp_part_grid.Add( 20*NM_PER_MIL );
	m_fp_part_grid.Add( 25*NM_PER_MIL );
	m_fp_part_grid.Add( 40*NM_PER_MIL );
	m_fp_part_grid.Add( 50*NM_PER_MIL );
	m_fp_part_grid.Add( 100*NM_PER_MIL );
	m_fp_part_grid.Add( 200*NM_PER_MIL );
	m_fp_part_grid.Add( 250*NM_PER_MIL );
	m_fp_part_grid.Add( 400*NM_PER_MIL );
	m_fp_part_grid.Add( 500*NM_PER_MIL );
	m_fp_part_grid.Add( 1000*NM_PER_MIL );
	m_fp_part_grid_spacing = 50*NM_PER_MIL;

	CMainFrame * frm = (CMainFrame*)AfxGetMainWnd();
	frm->m_wndMyToolBar.SetLists( &m_visible_grid, &m_part_grid, &m_routing_grid,
		m_visual_grid_spacing, m_part_grid_spacing, m_routing_grid_spacing, m_snap_angle, MIL );

	// default PCB parameters
	m_active_layer = LAY_TOP_COPPER;
	m_trace_w = 10*NM_PER_MIL;
	m_via_w = 28*NM_PER_MIL;
	m_via_hole_w = 14*NM_PER_MIL;

	// default cam parameters
	m_cam_full_path = "";
	m_fill_clearance = 10*NM_PER_MIL;
	m_mask_clearance = 8*NM_PER_MIL;
	m_thermal_width = 10*NM_PER_MIL;
	m_min_silkscreen_stroke_wid = 5*NM_PER_MIL;
	m_pilot_diameter = 10*NM_PER_MIL;
	m_cam_flags = GERBER_BOARD_OUTLINE;
	m_cam_layers = 0xfff;	// all layers
	m_cam_units = MIL;
	m_cam_drill_file = 1;
	m_outline_width = 5*NM_PER_MIL;
	m_hole_clearance = 15*NM_PER_MIL;
	m_annular_ring_pins = 7*NM_PER_MIL;
	m_annular_ring_vias = 5*NM_PER_MIL;

	// default DRC limits
	m_dr.bCheckUnrouted = FALSE;
	m_dr.trace_width = 10*NM_PER_MIL; 
	m_dr.pad_pad = 10*NM_PER_MIL; 
	m_dr.pad_trace = 10*NM_PER_MIL;
	m_dr.trace_trace = 10*NM_PER_MIL; 
	m_dr.hole_copper = 15*NM_PER_MIL; 
	m_dr.annular_ring_pins = 7*NM_PER_MIL;
	m_dr.annular_ring_vias = 5*NM_PER_MIL;
	m_dr.board_edge_copper = 25*NM_PER_MIL;
	m_dr.board_edge_hole = 25*NM_PER_MIL;
	m_dr.hole_hole = 25*NM_PER_MIL;
	m_dr.copper_copper = 10*NM_PER_MIL;

	// default trace widths (must be in ascending order)
	m_w.SetAtGrow( 0, 6*NM_PER_MIL );
	m_w.SetAtGrow( 1, 8*NM_PER_MIL );
	m_w.SetAtGrow( 2, 10*NM_PER_MIL );
	m_w.SetAtGrow( 3, 12*NM_PER_MIL );
	m_w.SetAtGrow( 4, 15*NM_PER_MIL );
	m_w.SetAtGrow( 5, 20*NM_PER_MIL );
	m_w.SetAtGrow( 6, 25*NM_PER_MIL );

	// default via widths
	m_v_w.SetAtGrow( 0, 24*NM_PER_MIL );
	m_v_w.SetAtGrow( 1, 24*NM_PER_MIL );
	m_v_w.SetAtGrow( 2, 24*NM_PER_MIL );
	m_v_w.SetAtGrow( 3, 24*NM_PER_MIL );
	m_v_w.SetAtGrow( 4, 30*NM_PER_MIL );
	m_v_w.SetAtGrow( 5, 30*NM_PER_MIL );
	m_v_w.SetAtGrow( 6, 40*NM_PER_MIL );

	// default via hole widths
	m_v_h_w.SetAtGrow( 0, 15*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 1, 15*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 2, 15*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 3, 15*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 4, 18*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 5, 18*NM_PER_MIL );
	m_v_h_w.SetAtGrow( 6, 20*NM_PER_MIL );

	CFreePcbView * view = (CFreePcbView*)m_view;
	view->InitializeView();

	// now try to find global options file
	CString fn = m_app_dir + "\\" + "default.cfg";
	CStdioFile file;
	if( !file.Open( fn, CFile::modeRead | CFile::typeText ) )
	{
		AfxMessageBox( "Unable to open global configuration file \"default.cfg\"\nUsing application defaults" );
	}
	else
	{
		try
		{
			// read global default file options
			ReadOptions( &file );
			// make path to library folder and index libraries
			char full[_MAX_PATH];
			CString fullpath = _fullpath( full, (LPCSTR)m_lib_dir, MAX_PATH );
			if( fullpath[fullpath.GetLength()-1] == '\\' )	
				fullpath = fullpath.Left(fullpath.GetLength()-1);
			m_full_lib_dir = fullpath;
		}
		catch( CString * err_str )
		{
			*err_str = "CFreePcbDoc::InitializeNewProject()\n" + *err_str;
			throw err_str;
		}
	}
}

void CFreePcbDoc::ProjectModified( BOOL flag )
{
	if( flag && m_project_modified )
		return;
	CWnd* pMain = AfxGetMainWnd();
	if( flag )
	{
		m_project_modified = TRUE;
		m_window_title = m_window_title + "*";
		pMain->SetWindowText( m_window_title );
	}
	else
	{
		m_project_modified = FALSE;
		int len = m_window_title.GetLength();
		if( len > 1 && m_window_title[len-1] == '*' )
		{
			m_window_title = m_window_title.Left(len-1);
			pMain->SetWindowText( m_window_title );
		}
	}
}

void CFreePcbDoc::OnViewLayers()
{
	CDlgLayers dlg;
	CFreePcbView * view = (CFreePcbView*)m_view;
	dlg.Initialize( m_num_layers, m_vis, m_rgb );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		for( int i=0; i<m_num_layers; i++ )
		{
			m_dlist->SetLayerRGB( i, m_rgb[i][0], m_rgb[i][1], m_rgb[i][2] );
			m_dlist->SetLayerVisible( i, m_vis[i] );
		}
		view->m_left_pane_invalid = TRUE;	// force erase of left pane
		view->CancelSelection();
		ProjectModified( TRUE );
		view->Invalidate( FALSE );
	}
}

void CFreePcbDoc::OnViewPartlist()
{
	CDlgPartlist dlg;
	dlg.Initialize( m_plist, &m_footprint_cache_map, &m_footlibfoldermap, m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_undo_list->Clear();
		CFreePcbView * view = (CFreePcbView*)m_view;
		view->CancelSelection();
		m_nlist->OptimizeConnections();
		ProjectModified( TRUE );
		view->Invalidate( FALSE );
	}
}

void CFreePcbDoc::OnPartProperties()
{
	CFreePcbView * view = (CFreePcbView*)m_view;
	partlist_info pl;
	int ip = m_plist->ExportPartListInfo( &pl, view->m_sel_part );
	CDlgAddPart dlg;
	dlg.Initialize( &pl, ip, TRUE, FALSE, FALSE, &m_footprint_cache_map, 
		&m_footlibfoldermap, m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		view->CancelSelection();
		CShape * old_shape = view->m_sel_part->shape;
		//** TODO: undo doesn't work if footprint changed
		m_view->SaveUndoInfoForPartAndNets( 
		m_view->m_sel_part, CPartList::UNDO_PART_MODIFY );
		CString ref_des = view->m_sel_part->ref_des;
		m_plist->ImportPartListInfo( &pl, 0 );
		cpart * part = m_plist->GetPart( &ref_des );
		if( !part )
			ASSERT(0);
		if( old_shape != part->shape )
		{
			// footprint changed, clear undo list
//			m_undo_list->Clear();
		}
		view->SelectPart( part );
		if( dlg.GetDragFlag() )
			ASSERT(0);	// not allowed
		else
		{
			m_nlist->OptimizeConnections();
			view->Invalidate( FALSE );
			ProjectModified( TRUE );
		}
	}
}

void CFreePcbDoc::OnFileExport()
{
	// force old-style file dialog by setting size of OPENFILENAME struct
	CMyFileDialogExport dlg( FALSE, NULL, NULL, OFN_HIDEREADONLY | OFN_EXPLORER, 
		"All Files (*.*)|*.*||", NULL, OPENFILENAME_SIZE_VERSION_400 );
	dlg.SetTemplate( IDD_EXPORT, IDD_EXPORT );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString str = dlg.GetPathName();
		CStdioFile file;
		if( !file.Open( str, CFile::modeWrite | CFile::modeCreate ) )
		{
			AfxMessageBox( "Unable to open file" );
		}
		else
		{
			partlist_info pl;
			m_plist->ExportPartListInfo( &pl, NULL );
			netlist_info nl;
			m_nlist->ExportNetListInfo( &nl );

			int flag = 0;
			switch( dlg.m_select )
			{
			case CMyFileDialog::PARTS_ONLY: flag = IMPORT_PARTS; break;
			case CMyFileDialog::NETS_ONLY: flag = IMPORT_NETS; break;
			case CMyFileDialog::PARTS_AND_NETS: flag = IMPORT_PARTS | IMPORT_NETS; break;
			}
			if( dlg.m_format == CMyFileDialog::PADSPCB )
				ExportPADSPCBNetlist( &file, flag, &pl, &nl );
			else
				ASSERT(0);
			file.Close();
		}
	}
}
void CFreePcbDoc::OnFileImport()
{
	int flags = 0;

	// force old-style file dialog by setting size of OPENFILENAME struct
	CMyFileDialog dlg( TRUE, NULL, NULL, OFN_HIDEREADONLY | OFN_EXPLORER, 
		"All Files (*.*)|*.*||", NULL, OPENFILENAME_SIZE_VERSION_400 );
	dlg.SetTemplate( IDD_IMPORT, IDD_IMPORT );
	dlg.m_ofn.lpstrTitle = "Import netlist file";
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{ 
		CString str = dlg.GetPathName(); 
		CStdioFile file;
		if( !file.Open( str, CFile::modeRead ) )
		{
			AfxMessageBox( "Unable to open file" );
		}
		else
		{
			m_undo_list->Clear();	// clear undo list
			partlist_info pl;
			netlist_info nl;

			// set flag for PARTS, NETS or both
			int flag = 0;
			if( dlg.m_select == CMyFileDialog::PARTS_ONLY 
				|| dlg.m_select == CMyFileDialog::PARTS_AND_NETS )
				flag |= IMPORT_PARTS;
			if( dlg.m_select == CMyFileDialog::NETS_ONLY 
				|| dlg.m_select == CMyFileDialog::PARTS_AND_NETS )
				flag |= IMPORT_NETS;

			if( m_plist->GetFirstPart() != NULL || m_nlist->m_map.GetCount() != 0 )
			{
				// there are parts and/or nets in project 
				CDlgImportOptions dlg_options;
				dlg_options.Initialize( flag, flags );
				int ret = dlg_options.DoModal();
				if( ret == IDCANCEL )
					return;
				else
					flags = dlg_options.m_flags;
			}

			// show log dialog
			m_dlg_log->ShowWindow( SW_SHOW );
			m_dlg_log->UpdateWindow();
			m_dlg_log->SetWindowPos( &CWnd::wndTopMost, 0, 0, 0, 0, 
				SWP_NOMOVE | SWP_NOSIZE );
			m_dlg_log->Clear();
			m_dlg_log->UpdateWindow();
			m_dlg_log->EnableOK( FALSE );

			// import the netlist file
			CString line;
			if( dlg.m_format == CMyFileDialog::PADSPCB )
			{
				line.Format( "Reading netlist file \"%s\":\r\n", str ); 
				m_dlg_log->AddLine( &line );
				ImportPADSPCBNetlist( &file, flag, &pl, &nl, m_dlg_log );
			}
			else
				ASSERT(0);
			if( flag & IMPORT_PARTS )
			{
				line = "\r\nImporting parts into project:\r\n";
				m_dlg_log->AddLine( &line );
				m_plist->ImportPartListInfo( &pl, flags, m_dlg_log );
			}
			if( flag & IMPORT_NETS )
			{
				line = "\r\nImporting nets into project:\r\n";
				m_dlg_log->AddLine( &line );
				m_nlist->ImportNetListInfo( &nl, flags, m_dlg_log, 0, 0, 0 );
			}
			line = "\r\n************** DONE ****************\r\n";
			m_dlg_log->AddLine( &line );

			// allow the user to dismiss the log
			m_dlg_log->EnableOK( TRUE );
			
			// finish up
			m_nlist->OptimizeConnections();
			m_view->OnViewAllElements();
			ProjectModified( TRUE );
			m_view->Invalidate( FALSE );
		}
	}
}

// import netlist 
// enter with file already open
//
int CFreePcbDoc::ImportNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pl, netlist_info * nl )
{
	CString instr;
	int err_flags = 0;
	int line = 0;
	BOOL not_eof;

	// find next section
	not_eof = file->ReadString( instr );
	line++;
	instr.Trim();
	while( 1 )
	{
		if( instr == "[parts]" && pl && (flags & IMPORT_PARTS) )
		{
			// read parts
			int ipart = 0;
			BOOL parts_section = FALSE;
			while( 1 )
			{
				not_eof = file->ReadString( instr );
				if( !not_eof )
				{
					// end of file
					file->Close();
					return err_flags;
				}
				line++;
				instr.Trim();
				if( instr[0] == '[' )
				{
					// next section
					break;
				}
				else if( instr.GetLength() && instr[0] != '/' )
				{
					// get ref prefix, ref number and shape
					pl->SetSize( ipart+1 );
					CString ref_str( mystrtok( (char*)LPCSTR(instr), " \t" ) );
					CString shape_str( mystrtok( NULL, "\n\r" ) );
					shape_str.Trim();
					// find footprint, get from library if necessary
					CShape * s = GetFootprintPtr( shape_str );
					// add part to partlist_info
					(*pl)[ipart].part = NULL;
					(*pl)[ipart].ref_des = ref_str;
					if( s )
					{
						(*pl)[ipart].ref_size = s->m_ref_size;
						(*pl)[ipart].ref_width = s->m_ref_w;
					}
					else
					{
						(*pl)[ipart].ref_size = 0;
						(*pl)[ipart].ref_width = 0;
					}
					(*pl)[ipart].package = shape_str;
					(*pl)[ipart].shape = s;
					(*pl)[ipart].x = 0;
					(*pl)[ipart].y = 0;
					(*pl)[ipart].angle = 0;
					(*pl)[ipart].side = 0;
					if( !s )
						err_flags |= FOOTPRINTS_NOT_FOUND;
					ipart++;
				}
			}
		}
		else if( instr == "[nets]" && nl && (flags & IMPORT_NETS) )
		{
			// read nets
			cnet * net = 0;
			int num_pins = 0;
			while( 1 )
			{
				not_eof = file->ReadString( instr );
				if( !not_eof )
				{
					// end of file
					file->Close();
					return err_flags;
				}
				line++;
				instr.Trim();
				if( instr[0] == '[' )
				{
					// next section
					break;
				}
				else if( instr.GetLength() && instr[0] != '/' )
				{
					int delim_pos;
					if( (delim_pos = instr.Find( ":", 0 )) != -1 )
					{
						// new net, get net name
						int inet = nl->GetSize();
						nl->SetSize( inet+1 );
						CString net_name( mystrtok( (char*)LPCSTR(instr), ":" ) );
						net_name.Trim();
						if( net_name.GetLength() )
						{
							// add new net
							(*nl)[inet].name = net_name;
							(*nl)[inet].net = NULL;
							(*nl)[inet].modified = TRUE;
							(*nl)[inet].deleted = FALSE;
							(*nl)[inet].visible = TRUE;
							(*nl)[inet].w = 0;
							(*nl)[inet].v_w = 0;
							(*nl)[inet].v_h_w = 0;
							instr = instr.Right( instr.GetLength()-delim_pos-1 );
							num_pins = 0;
						}
						// add pins to net
						char * pin = mystrtok( (char*)LPCSTR(instr), " \t\n\r" );
						while( pin )
						{
							CString pin_cstr( pin );
							if( pin_cstr.GetLength() > 3 )
							{
								int dot = pin_cstr.Find( ".", 0 );
								if( dot )
								{
									CString ref_des = pin_cstr.Left( dot );
									CString pin_num_cstr = pin_cstr.Right( pin_cstr.GetLength()-dot-1 );
									(*nl)[inet].ref_des.Add( ref_des );
									(*nl)[inet].pin_name.Add( pin_num_cstr );
#if 0	// TODO: check for illegal pin names
									}
									else
									{
										// illegal pin number for part
										CString mess;
										mess.Format( "Error in line %d of netlist file\nIllegal pin number \"%s\"", 
											line, pin_cstr );
										AfxMessageBox( mess );
										break;
									}
#endif
								}
								else
								{
									// illegal string
									break;
								}
							}
							else if( pin_cstr != "" )
							{
								// illegal pin identifier
								CString mess;
								mess.Format( "Error in line %d of netlist file\nIllegal pin identifier \"%s\"", 
									line, pin_cstr );
								AfxMessageBox( mess );
							}
							pin = mystrtok( NULL, " \t\n\r" );
						} // end while( pin )
					}
				}
			}
		}
		else if( instr == "[end]" )
		{
			// normal return
			file->Close();
			return err_flags;
		}
		else
		{
			not_eof = file->ReadString( instr );
			line++;
			instr.Trim();
			if( !not_eof)
			{
				// end of file
				file->Close();
				return err_flags;
			}
		}
	}
}

// export netlist in PADS-PCB format
// enter with file already open
//
int CFreePcbDoc::ExportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pl, netlist_info * nl )
{
	CString str;

	file->WriteString( "*PADS-PCB*\n" );
	if( flags & IMPORT_PARTS )
	{
		file->WriteString( "*PART*\n" );
		for( int i=0; i<pl->GetSize(); i++ )
		{
			part_info * pi = &(*pl)[i];
			if( pi->shape )
				str.Format( "%s %s\n", pi->ref_des, pi->shape->m_name );
			else
				str.Format( "%s %s\n", pi->ref_des, pi->package );
			file->WriteString( str );
		}
	}

	if( flags & IMPORT_NETS )
	{
		if( flags & IMPORT_PARTS )
			file->WriteString( "\n" );
		file->WriteString( "*NET*\n" );
		for( int i=0; i<nl->GetSize(); i++ )
		{
			net_info * ni = &(*nl)[i];
			str.Format( "*SIGNAL* %s\n", ni->name );
			file->WriteString( str );
			str = "";
			int np = ni->pin_name.GetSize();
			for( int ip=0; ip<np; ip++ )
			{
				CString pin_str;
				pin_str.Format( "%s.%s ", ni->ref_des[ip], ni->pin_name[ip] );
				str += pin_str;
				if( !((ip+1)%8) || ip==(np-1) )
				{
					str += "\n";
					file->WriteString( str );
					str = "";
				}
			}
		}
	}
	file->WriteString( "*END*\n" );
	return 0;
}

// import netlist in PADS-PCB format
// enter with file already open
//
int CFreePcbDoc::ImportPADSPCBNetlist( CStdioFile * file, UINT flags, 
							   partlist_info * pl, netlist_info * nl,
							   CDlgLog * log )
{
	CString instr, net_name, mess;
	CMapStringToPtr part_map, net_map, pin_map;
	void * ptr;
	int npins, inet;
	int err_flags = 0;
	int line = 0;
	BOOL not_eof;
	int ipart;
	if( pl )
		ipart = pl->GetSize();

	// state machine
	enum { IDLE, PARTS, NETS, SIGNAL };
	int state = IDLE;

	while( 1 )
	{
		not_eof = file->ReadString( instr );
		line++;
		instr.Trim();
		if( instr == "*END*" || !not_eof )
		{
			// normal return
			file->Close();
			return err_flags;
		}
		else if( instr.Left(6) == "*PART*" )
			state = PARTS;
		else if( instr.Left(5) == "*NET*" )
			state = NETS;
		else if( state == PARTS && pl && (flags & IMPORT_PARTS) )
		{
			// read parts
			if( instr.GetLength() && instr[0] != '/' )
			{
				// get ref_des and footprint
				CString ref_str( mystrtok( (char*)LPCSTR(instr), " \t" ) );
				if( ref_str.GetLength() > MAX_REF_DES_SIZE )
				{
					CString mess;
					mess.Format( "  line %d: Reference designator \"%s\" too long, truncated\r\n",
						line, ref_str );
					if( log ) 
						log->AddLine( &mess );
					ref_str = ref_str.Left(CShape::MAX_PIN_NAME_SIZE);
				}
				// check for legal ref_designator
				if( ref_str.FindOneOf( ". " ) != -1 )
				{
					mess.Format( "  line %d: Part \"%s\" illegal reference designator, ignored\r\n", 
						line, ref_str );
					if( log ) 
						log->AddLine( &mess );
					continue;
				}
				// check for duplicate part
				if( part_map.Lookup( ref_str, ptr ) )
				{
					mess.Format( "  line %d: Part \"%s\" is duplicate, ignored\r\n", 
						line, ref_str );
					if( log ) 
						log->AddLine( &mess );
					continue;
				}
				// new part
				pl->SetSize( ipart+1 );
				CString shape_str( mystrtok( NULL, "\n\r" ) );
				shape_str.Trim();
				// check for "ssss@ffff" format
				int pos = shape_str.Find( "@" );
				if( pos != -1 )
				{
					shape_str = shape_str.Right( shape_str.GetLength()-pos-1 );
				}
				if( shape_str.GetLength() > CShape::MAX_NAME_SIZE )
				{
					CString mess;
					mess.Format( "  line %d: Package name \"%s\" too long, truncated\r\n",
						line, shape_str );
					if( log ) 
						log->AddLine( &mess );
					shape_str = shape_str.Left(CShape::MAX_PIN_NAME_SIZE);
				}
				// find footprint, get from library if necessary
				CShape * s = GetFootprintPtr( shape_str );
				if( s == NULL )
				{
					mess.Format( "  line %d: Part \"%s\" footprint \"%s\" not found\r\n", 
						line, ref_str, shape_str );
					log->AddLine( &mess );
				}
				// add part to partlist_info
				(*pl)[ipart].part = NULL;
				(*pl)[ipart].ref_des = ref_str;
				part_map.SetAt( ref_str, NULL );
				if( s )
				{
					(*pl)[ipart].ref_size = s->m_ref_size;
					(*pl)[ipart].ref_width = s->m_ref_w;
				}
				else
				{
					(*pl)[ipart].ref_size = 0;
					(*pl)[ipart].ref_width = 0;
				}
				(*pl)[ipart].package = shape_str;
				(*pl)[ipart].bOffBoard = TRUE;
				(*pl)[ipart].shape = s;
				(*pl)[ipart].angle = 0;
				(*pl)[ipart].side = 0;
				(*pl)[ipart].x = 0;
				(*pl)[ipart].y = 0;
				ipart++;
			}
		}
		else if( instr.Left(8) == "*SIGNAL*" && nl && (flags & IMPORT_NETS) )
		{
			state = NETS;
			net_name = instr.Right(instr.GetLength()-8);
			net_name.Trim();
			int pos = net_name.Find( " " );
			if( pos != -1 )
			{
				net_name = net_name.Left( pos );
			}
			if( net_name.GetLength() )
			{
				if( net_name.GetLength() > MAX_NET_NAME_SIZE )
				{
					mess.Format( "  line %d: Net name \"%s\" too long, truncated\r\n                    truncated to \"%s\"\r\n", 
						line, net_name, net_name.Left(MAX_NET_NAME_SIZE) );
					if( log ) 
						log->AddLine( &mess );
					net_name = net_name.Left(MAX_NET_NAME_SIZE);
				}
				if( net_name.FindOneOf( " \"" ) != -1 )
				{
					mess.Format( "  line %d: Net name \"%s\" illegal, ignored\r\n", 
						line, net_name );
					if( log ) 
						log->AddLine( &mess );
				}
				else
				{
					if( net_map.Lookup( net_name, ptr ) )
					{
						mess.Format( "  line %d: Net name \"%s\" is duplicate, ignored\r\n", 
							line, net_name );
						if( log ) 
							log->AddLine( &mess );
					}
					else
					{
						// add new net
						net_map.SetAt( net_name, NULL );
						inet = nl->GetSize();
						nl->SetSize( inet+1 );
						(*nl)[inet].name = net_name;
						(*nl)[inet].net = NULL;
						(*nl)[inet].apply_widths = FALSE;
						(*nl)[inet].modified = TRUE;
						(*nl)[inet].deleted = FALSE;
						(*nl)[inet].visible = TRUE;
						// mark widths as undefined
						(*nl)[inet].w = -1;
						(*nl)[inet].v_w = -1;
						(*nl)[inet].v_h_w = -1;
						npins = 0;
						state = SIGNAL;
					}
				}
			}
		}
		else if( state == SIGNAL  && nl && (flags & IMPORT_NETS) )
		{
			// add pins to net
			char * pin = mystrtok( (char*)LPCSTR(instr), " \t\n\r" );
			while( pin )
			{
				CString pin_cstr( pin );
				if( pin_cstr.GetLength() > 3 )
				{
					int dot = pin_cstr.Find( ".", 0 );
					if( dot )
					{
						if( pin_map.Lookup( pin_cstr, ptr ) )
						{
							mess.Format( "  line %d: Net \"%s\" pin \"%s\" is duplicate, ignored\r\n", 
								line, net_name, pin_cstr );
							if( log ) 
								log->AddLine( &mess );
						}
						else
						{
							pin_map.SetAt( pin_cstr, NULL );
							CString ref_des = pin_cstr.Left( dot );
							CString pin_num_cstr = pin_cstr.Right( pin_cstr.GetLength()-dot-1 );
							(*nl)[inet].ref_des.Add( ref_des );
							if( pin_num_cstr.GetLength() > CShape::MAX_PIN_NAME_SIZE )
							{
								CString mess;
								mess.Format( "  line %d: Pin name \"%s\" too long, truncated\r\n",
									line, pin_num_cstr );
								if( log ) 
									log->AddLine( &mess );
								pin_num_cstr = pin_num_cstr.Left(CShape::MAX_PIN_NAME_SIZE);
							}
							(*nl)[inet].pin_name.Add( pin_num_cstr );
						}
					}
					else
					{
						// illegal pin identifier
						mess.Format( "  line %d: Pin identifier \"%s\" illegal, ignored\r\n", 
							line, pin_cstr );
						if( log ) 
							log->AddLine( &mess );
					}
				}
				else
				{
					// illegal pin identifier
					mess.Format( "  line %d: Pin identifier \"%s\" illegal, ignored\r\n", 
						line, pin_cstr );
					if( log ) 
						log->AddLine( &mess );
				}
				pin = mystrtok( NULL, " \t\n\r" );
			} // end while( pin )
		}
	} // end while
}

void CFreePcbDoc::OnAppExit()
{
	if( FileClose() != IDCANCEL )
		AfxGetMainWnd()->SendMessage( WM_CLOSE, 0, 0 );
}

void CFreePcbDoc::OnFileConvert()
{
	CivexDlg dlg;
	dlg.DoModal();

}

void CFreePcbDoc::OnEditUndo()
{
	// undo last operation unless dragging something
	if( !m_view->CurDragging() )
	{
		m_view->CancelSelection();
		while( m_undo_list->Pop() )
		{
		}
		m_nlist->SetAreaConnections();
		m_view->Invalidate();
	}
}

// create undo record for moving origin
//
undo_move_origin * CFreePcbDoc::CreateMoveOriginUndoRecord( int x_off, int y_off )
{
	// create undo record 
	undo_move_origin * undo = new undo_move_origin;
	undo->x_off = x_off;
	undo->y_off = y_off;
	return undo;
}

// undo operation on move origin
//
void CFreePcbDoc::MoveOriginUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		// restore previous origin
		undo_move_origin * un_mo = (undo_move_origin*)ptr;
		int x_off = un_mo->x_off;
		int y_off = un_mo->y_off;
		this_Doc->m_view->MoveOrigin( -x_off, -y_off );
		this_Doc->m_view->Invalidate( FALSE );
	}
	delete ptr;
}

// create undo record for board outline
//
undo_board_outline * CFreePcbDoc::CreateBoardOutlineUndoRecord( int type )
{
	// create undo record for board outline
	int ncorners = m_board_outline->GetNumCorners();
	undo_board_outline * undo;
	if( type == UNDO_BOARD_ADD )
	{
		undo = (undo_board_outline*)malloc( sizeof(undo_board_outline) );
		undo->ncorners = 0;
	}
	else
	{
		undo = (undo_board_outline*)malloc( sizeof(undo_board_outline)+ncorners*sizeof(undo_corner));
		undo->ncorners = ncorners;
		undo_corner * corner = (undo_corner*)((UINT)undo + sizeof(undo_board_outline));
		for( int ic=0; ic<ncorners; ic++ )
		{
			corner[ic].x = m_board_outline->GetX( ic );
			corner[ic].y = m_board_outline->GetY( ic );
			corner[ic].style = m_board_outline->GetSideStyle( ic );
		}
	}
	return undo;
}

// undo operation on board outline
//
void CFreePcbDoc::BoardOutlineUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		// destroy current outline
		if( this_Doc->m_board_outline )
		{
			delete this_Doc->m_board_outline;
			this_Doc->m_board_outline = NULL;
		}
		if( type != UNDO_BOARD_ADD )
		{
			// restore previous outline from undo record
			undo_board_outline * un_bd = (undo_board_outline*)ptr;
			undo_corner * corner = (undo_corner*)((UINT)un_bd + sizeof(undo_board_outline));
			this_Doc->m_board_outline = new CPolyLine( this_Doc->m_dlist );
			id bid( ID_BOARD, ID_BOARD_OUTLINE );
			this_Doc->m_board_outline->Start( LAY_BOARD_OUTLINE, 1, 20*NM_PER_MIL, 
				corner[0].x, corner[0].y, 0, &bid, NULL );
			for( int ic=1; ic<un_bd->ncorners; ic++ )
				this_Doc->m_board_outline->AppendCorner( 
					corner[ic].x, corner[ic].y, corner[ic-1].style );
			this_Doc->m_board_outline->Close( corner[un_bd->ncorners-1].style );
		}
	}
	delete ptr;
}

// create undo record for SM cutout
// only include closed polys
//
undo_sm_cutout * CFreePcbDoc::CreateSMCutoutUndoRecord( int type, CPolyLine * poly )
{
	// create undo record for sm cutout
	undo_sm_cutout * undo;
	if( type == UNDO_SM_CUTOUT_NONE )
	{
		undo = (undo_sm_cutout*)malloc( sizeof(undo_sm_cutout));
		undo->ncorners = 0;
	}
	else
	{
		int ncorners = poly->GetNumCorners();
		undo = (undo_sm_cutout*)malloc( sizeof(undo_sm_cutout)+ncorners*sizeof(undo_corner));
		undo->layer = poly->GetLayer();
		undo->hatch_style = poly->GetHatch();
		undo->ncorners = poly->GetNumCorners();
		undo_corner * corner = (undo_corner*)((UINT)undo + sizeof(undo_sm_cutout));
		for( int ic=0; ic<ncorners; ic++ )
		{
			corner[ic].x = poly->GetX( ic );
			corner[ic].y = poly->GetY( ic );
			corner[ic].style = poly->GetSideStyle( ic );
		}
	}
	return undo;
}

// undo operation on solder mask cutout
//
void CFreePcbDoc::SMCutoutUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo ) 
	{
		if( type == UNDO_SM_CUTOUT_NONE || type == UNDO_SM_CUTOUT_LAST ) 
		{
			// remove all cutouts
			this_Doc->m_sm_cutout.RemoveAll();
		}
		if( type != UNDO_SM_CUTOUT_NONE )
		{
			// restore cutout from undo record
			undo_sm_cutout * un_sm = (undo_sm_cutout*)ptr;
			undo_corner * corner = (undo_corner*)((UINT)un_sm + sizeof(undo_sm_cutout));
			int i = this_Doc->m_sm_cutout.GetSize();
			this_Doc->m_sm_cutout.SetSize(i+1);
			CPolyLine * poly = &this_Doc->m_sm_cutout[i];
			poly->SetDisplayList( this_Doc->m_dlist );
			id sm_id( ID_SM_CUTOUT, ID_SM_CUTOUT, i );
			poly->Start( un_sm->layer, 1, 10*NM_PER_MIL, 
				corner[0].x, corner[0].y, un_sm->hatch_style, &sm_id, NULL );
			for( int ic=1; ic<un_sm->ncorners; ic++ )
				poly->AppendCorner( corner[ic].x, corner[ic].y, corner[ic-1].style );
			poly->Close( corner[un_sm->ncorners-1].style );
		}
	}
	delete ptr;
}

// call dialog to create Gerber and drill files
void CFreePcbDoc::OnFileGenerateCadFiles()
{
	CDlgCAD dlg;
	if( m_cam_full_path == "" )
		m_cam_full_path = m_path_to_folder + "\\CAM";
	dlg.Initialize( m_version,
		&m_cam_full_path, 
		m_num_copper_layers, 
		m_cam_units,
		m_fill_clearance, 
		m_mask_clearance,
		m_thermal_width,
		m_pilot_diameter,
		m_min_silkscreen_stroke_wid,
		m_outline_width,
		m_hole_clearance,
		m_annular_ring_pins,
		m_annular_ring_vias,
		m_cam_flags,
		m_cam_layers,
		m_cam_drill_file,
		m_board_outline, 
		&m_sm_cutout,
		m_plist, 
		m_nlist, 
		m_tlist, 
		m_dlist );
	dlg.DoModal();
	// update parameters in case changed in dialog
	if( m_cam_full_path != dlg.m_folder
		|| m_cam_units != dlg.m_units
		|| m_fill_clearance != dlg.m_fill_clearance
		|| m_mask_clearance != dlg.m_mask_clearance
		|| m_thermal_width != dlg.m_thermal_width
		|| m_min_silkscreen_stroke_wid != dlg.m_min_silkscreen_width
		|| m_pilot_diameter != dlg.m_pilot_diameter
		|| m_outline_width != dlg.m_outline_width
		|| m_hole_clearance != dlg.m_hole_clearance
		|| m_annular_ring_pins != dlg.m_annular_ring_pins
		|| m_annular_ring_vias != dlg.m_annular_ring_vias
		|| m_cam_flags != dlg.m_flags
		|| m_cam_layers != dlg.m_layers
		|| m_cam_drill_file != dlg.m_drill_file )
	{
		ProjectModified( TRUE );
	}
	m_cam_full_path = dlg.m_folder;
	m_cam_units = dlg.m_units;
	m_fill_clearance = dlg.m_fill_clearance;
	m_mask_clearance = dlg.m_mask_clearance;
	m_thermal_width = dlg.m_thermal_width;
	m_min_silkscreen_stroke_wid = dlg.m_min_silkscreen_width;
	m_pilot_diameter = dlg.m_pilot_diameter;
	m_outline_width = dlg.m_outline_width;
	m_hole_clearance = dlg.m_hole_clearance;
	m_annular_ring_pins = dlg.m_annular_ring_pins;
	m_annular_ring_vias = dlg.m_annular_ring_vias;
	m_cam_flags = dlg.m_flags;
	m_cam_layers = dlg.m_layers;
	m_cam_drill_file = dlg.m_drill_file;
}

void CFreePcbDoc::OnToolsFootprintwizard()
{
	CDlgWizQuad dlg;
	dlg.Initialize( &m_footprint_cache_map, &m_footlibfoldermap );
	dlg.DoModal();
}

void CFreePcbDoc::MakeLibraryMaps( CString * fullpath )
{
	m_footlibfoldermap.SetDefaultFolder( fullpath );
	m_footlibfoldermap.AddFolder( fullpath, NULL );
}

void CFreePcbDoc::OnProjectOptions()
{
	CDlgProjectOptions dlg;
	if( m_name == "" )
	{
		m_name = m_pcb_filename;
		if( m_name.Right(4) == ".fpc" )
			m_name = m_name.Left( m_name.GetLength()-4 );
	}
	dlg.Init( FALSE, &m_name, &m_path_to_folder, &m_full_lib_dir,
		m_num_copper_layers, m_trace_w, m_via_w, m_via_hole_w,
		m_auto_interval, &m_w, &m_v_w, &m_v_h_w );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// set options from dialog
		//m_num_copper_layers = dlg.GetNumCopperLayers();
		//m_num_layers = m_num_copper_layers + LAY_TOP_COPPER;
		m_trace_w = dlg.GetTraceWidth();
		m_via_w = dlg.GetViaWidth();
		m_via_hole_w = dlg.GetViaHoleWidth();
		m_auto_interval = dlg.GetAutoInterval();

		m_view->InvalidateLeftPane();
		m_view->Invalidate( FALSE );
		m_project_open = TRUE;

		// force redraw of function key text
		m_view->m_cursor_mode = 999;
		m_view->SetCursorMode( CUR_NONE_SELECTED );
		ProjectModified( TRUE );
	}
}

// come here from MainFrm on timer event
//
void CFreePcbDoc::OnTimer()
{
	m_auto_elapsed += TIMER_PERIOD;
	if( m_view && m_auto_interval && m_auto_elapsed > m_auto_interval )
	{
		if( !m_view->CurDragging() && m_project_modified )
			FileSave();
	}
}



void CFreePcbDoc::OnToolsCheckPartsAndNets()
{
	// open log
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	m_dlg_log->EnableOK( FALSE );
	CString str;
	int nerrors = m_plist->CheckPartlist( &str );
	str += "\r\n";
	nerrors += m_nlist->CheckNetlist( &str );
	m_dlg_log->AddLine( &str );
	m_dlg_log->EnableOK( TRUE );
}

void CFreePcbDoc::OnToolsDrc()
{
	DlgDRC dlg;
	m_drelist->Clear();
	dlg.Initialize( m_units, 
					&m_dr,
					m_plist,
					m_nlist,
					m_drelist,
					m_num_copper_layers,
					m_board_outline );
	int ret = dlg.DoModal();
	ProjectModified( TRUE );
	m_view->BringWindowToTop();
	m_view->Invalidate( FALSE );
}

void CFreePcbDoc::OnToolsClearDrc()
{
	if( m_view->m_cursor_mode == CUR_DRE_SELECTED )
	{
		m_view->CancelSelection();
		m_view->SetCursorMode( CUR_NONE_SELECTED );
	}
	m_drelist->Clear();
	m_view->Invalidate( FALSE );
}

void CFreePcbDoc::OnToolsShowDRCErrorlist()
{
	// TODO: Add your command handler code here
}

void CFreePcbDoc::SetFileLayerMap( int file_layer, int layer )
{
	m_file_layer_by_layer[layer] = file_layer;
	m_layer_by_file_layer[file_layer] = layer;
}


void CFreePcbDoc::OnToolsCheckConnectivity()
{
	// open log
	m_dlg_log->ShowWindow( SW_SHOW );
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	m_dlg_log->EnableOK( FALSE );
	CString str;
	int nerrors = m_nlist->CheckConnectivity( &str );
	m_dlg_log->AddLine( &str );
	if( !nerrors )
	{
		str.Format( "********* NO UNROUTED CONNECTIONS ************\r\n" );
		m_dlg_log->AddLine( &str );
	}
	m_dlg_log->EnableOK( TRUE );
}

void CFreePcbDoc::OnViewLog()
{
	m_dlg_log->ShowWindow( SW_SHOW ); 
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
//	m_dlg_log->Clear();
//	m_dlg_log->UpdateWindow();
	m_dlg_log->EnableOK( TRUE );
}

void CFreePcbDoc::OnToolsCheckCopperAreas()
{
	CString str;
 
	m_dlg_log->ShowWindow( SW_SHOW );   
	m_dlg_log->UpdateWindow();
	m_dlg_log->BringWindowToTop();
	m_dlg_log->Clear();
	m_dlg_log->UpdateWindow();
	m_view->CancelSelection();
	cnet * net = m_nlist->GetFirstNet(); 
	BOOL new_event = TRUE; 
	while( net )
	{
		if( net->nareas > 0 )
		{
			str.Format( "net \"%s\": %d areas\r\n", net->name, net->nareas ); 
			m_dlg_log->AddLine( &str );
		}
		m_view->SaveUndoInfoForAllAreasInNet( net, new_event ); 
		new_event = FALSE;
		// check all areas in net for intersection
		if( net->nareas > 1 )
		{
			for( int ia1=0; ia1<net->nareas-1; ia1++ ) 
			{
				BOOL mod_ia1 = FALSE;
				for( int ia2=net->nareas-1; ia2 > ia1; ia2-- )
				{
					if( net->area[ia1].poly->GetLayer() == net->area[ia2].poly->GetLayer() )
					{
						// check ia2 against 1a1 
						int n_ext = m_nlist->CheckIntersection( net, ia1, ia2 );
						if( n_ext == 1 )
						{
							str.Format( "      combining areas %d and %d\r\n", ia1+1, ia2+1 );
							m_dlg_log->AddLine( &str );
							mod_ia1 = TRUE;
						}
						else if( n_ext == 2 )
						{
							str.Format( "      areas %d and %d have an intersecting arc, can't combine\r\n", ia1+1, ia2+1 );
							m_dlg_log->AddLine( &str );
						}
					}
				}
				if( mod_ia1 )
					ia1--;		// if modified, we need to check it again
			}
		}
		net = m_nlist->GetNextNet();
	}
	str.Format( "*******  DONE *******\r\n" );
	m_dlg_log->AddLine( &str );
	ProjectModified( TRUE );
	m_view->Invalidate( FALSE );
	m_dlg_log->EnableOK( TRUE );
}
