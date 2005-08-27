// imlementation of classes to manage footprint libraries
//
#include "stdafx.h"
#include "FootprintLib.h"
#include "DlgLog.h"
#include "DlgAddPart.h"

// index one library file
// file_name is just the file name, not the complete path
//
void CFootLibFolder::IndexLib( CString * file_name, CDlgLog * dlog )
{
	// see if file_name already exists in index
	int n_libs = m_footlib.GetSize();
	CString full_path = m_full_path_to_folder + "\\" + *file_name;
	int nlib = n_libs;
	for( int ilib=0; ilib<n_libs; ilib++ )
	{
		if( m_footlib[ilib].m_full_path == full_path )
		{
			// found it
			nlib = ilib;
			break;
		}
	}
	if( nlib < n_libs )
	{
		// file has been indexed previously, we are re-indexing it
		// remove all previous entries
		for( int ih=0; ih<GetNumHeadings(nlib); ih++ )
			for( int i=0; i<GetNumFootprints( nlib, ih ); i++ )
				m_lib_map.RemoveKey( *GetFootprintName( nlib, ih, i ) );
		m_footlib[nlib].m_heading.SetSize(0);
	}
	else
	{
		// new file to be added to index
		m_footlib.SetSize( nlib+1 );	// add one to array
		m_footlib[nlib].m_full_path = full_path;
		m_footlib[nlib].m_heading.SetSize(0);
	}
	// now index the file
	int n_footprints = 0;

	// headings are deprecated
	m_footlib[nlib].m_heading.SetSize(1);
	m_footlib[nlib].m_heading[0].m_name = "unclassified";

	CStdioFile file;
	int err = file.Open( m_footlib[nlib].m_full_path, CFile::modeRead );
	if( !err )
		ASSERT(0);

	CString instr;
	int pos = 0;
	int last_ih = 0;
	int last_if = -1;
	while( file.ReadString( instr ) )
	{
		if( instr.Left(5) == "name:" )
		{
			// found a footprint
			// if there was a previous footprint, save offset to next one
			if( last_if != -1 )
				m_footlib[nlib].m_heading[0].m_foot[last_if].m_next_offset = file.GetPosition();
			CString shape_name = instr.Right( instr.GetLength()-5 );
			shape_name.Trim();
			if( shape_name.Right(1) == "\"" )
				shape_name = shape_name.Left( shape_name.GetLength() -1 );
			if( shape_name.Left(1) == "\"" )
				shape_name = shape_name.Right( shape_name.GetLength() -1 );
			shape_name.Trim();
			if( n_footprints >= (m_footlib[nlib].m_heading[0].m_foot.GetSize()-1) )
				m_footlib[nlib].m_heading[0].m_foot.SetSize(n_footprints + 100 );
			m_footlib[nlib].m_heading[0].m_foot[n_footprints].m_name = shape_name;
			m_footlib[nlib].m_heading[0].m_foot[n_footprints].m_offset = pos;
			unsigned int i;
			i = (nlib<<24) + pos;
			m_lib_map.SetAt( shape_name, (void*)i );
			// save indices to this footprint
			last_if = n_footprints;
			// next
			n_footprints++;
		}
		pos = file.GetPosition();
	}
	// set next_offset of last footprint to -1
	if( last_if != -1 )
		m_footlib[nlib].m_heading[0].m_foot[last_if].m_next_offset = -1;
	// set array sizes
	m_footlib[nlib].m_heading[0].m_foot.SetSize( n_footprints );
	SetExpanded( nlib, FALSE );
	file.Close();
	m_footlib[nlib].m_indexed = TRUE;
}

// index all of the library files in a folder
//
void CFootLibFolder::IndexAllLibs( CString * full_path )
{
	Clear();
	CDlgAddPart dlg;
	m_full_path_to_folder = *full_path;

	// start looking for library files
	CFileFind finder;
	if( chdir( m_full_path_to_folder ) != 0 )
	{
		CString mess;
		mess.Format( "Unable to open library folder \"%s\"", m_full_path_to_folder );
		AfxMessageBox( mess );
	}
	else
	{
		// pop up log dialog
		CDlgLog * dlg_log = new CDlgLog;
		dlg_log->Create( IDD_LOG );
		dlg_log->CenterWindow();
		dlg_log->Move( 100, 50 );
		dlg_log->ShowWindow( SW_SHOW );
		dlg_log->UpdateWindow();
		dlg_log->BringWindowToTop();
		dlg_log->Clear();
		dlg_log->EnableOK( FALSE );

		BOOL bWorking = finder.FindFile( "*.fpl" );
		while (bWorking)
		{
			bWorking = finder.FindNextFile();
			CString fn = finder.GetFileName();
			if( !finder.IsDirectory() )
			{
				// found a library file, index it
				CString log_message;
				log_message.Format( "Indexing library: \"%s\"\r\n", fn );
				dlg_log->AddLine( &log_message );
				IndexLib( &fn );
			}
		}
		// close log dialog
		dlg_log->DestroyWindow();
		delete dlg_log;
	}
	finder.Close();
}

// clear all data
//
void CFootLibFolder::Clear()
{
	m_footlib.RemoveAll();
	m_lib_map.RemoveAll();
	m_full_path_to_folder = "";
}

// returns the number of libraries
//
int CFootLibFolder::GetNumLibs()
{ 
	return m_footlib.GetSize(); 
}

// returns the full path to the folder
//
CString * CFootLibFolder::GetFullPath()
{ 
	return &m_full_path_to_folder; 
}

// get info about a footprint
// enter with:
//	name = pointer to CString containing footprint name
//	ilib = pointer to variable to receive library index (or NULL)
//	iheading = pointer to variable to receive heading index (or NULL)
//	file_name = pointer to CString to receive lib file name (or NULL)
//	offset = pointer to variable to receive position of footprint in lib file (or NULL)
// returns FALSE if fails, TRUE if succeeds with:
//	*ilib = index into array of libraries
//	*iheading = index into array of headings
//	*file_name = name of library file
//	*offset = offset into library file
//
BOOL CFootLibFolder::GetFootprintInfo( CString * name, int * ilib, 
		int * iheading, int * ifootprint, CString * file_name, int * offset, int * next_offset ) 
{
	void * ptr;
	int m_ilib;
	int m_ih = -1;
	int m_if = -1;
	CString m_file_name;
	int m_offset;

	BOOL exists = m_lib_map.Lookup( *name, ptr );
	if( exists )
	{
		UINT32 pos = (UINT32)ptr;
		m_ilib = pos>>24;
		m_offset = pos & 0xffffff;
		m_file_name = m_footlib[m_ilib].m_full_path;
		if( ilib )
			*ilib = m_ilib;

		// search arrays for heading
		for( int ih=0; ih<m_footlib[m_ilib].m_heading.GetSize(); ih++ )
		{
			for( int i=0; i<m_footlib[m_ilib].m_heading[ih].m_foot.GetSize(); i++ )
			{
				if( m_footlib[m_ilib].m_heading[ih].m_foot[i].m_name == *name )
				{
					m_ih = ih;
					m_if = i;
					break;
				}
			}
		}
		if( m_ih == -1 )
			ASSERT(0);
		if( m_if == -1 )
			ASSERT(0);
		// OK
		if( iheading )
			*iheading = m_ih;
		if( ifootprint )
			*ifootprint = m_if;
		if( file_name )
			*file_name = m_file_name;
		if( offset )
			*offset = m_offset;
		if( next_offset )
			*next_offset = m_footlib[m_ilib].m_heading[m_ih].m_foot[m_if].m_next_offset;
		return TRUE;
	}
	else
		return FALSE;
}

// search for a file name in library
// returns index to file, or -1 if not found
//
int CFootLibFolder::SearchFileName( CString * fn )
{
	for( int i=0; i<m_footlib.GetSize(); i++ )
	{
		if( m_footlib[i].m_full_path == *fn )
			return i;
	}
	return -1;	// if file not found
}

// get the number of headings in library file
int CFootLibFolder::GetNumHeadings( int ilib )
{ 
	return m_footlib[ilib].m_heading.GetSize(); 
}

// get heading
CString * CFootLibFolder::GetHeading( int ilib, int iheading )
{ 
	return &m_footlib[ilib].m_heading[iheading].m_name; 
}

// get library file name
CString * CFootLibFolder::GetLibraryFilename( int ilib )
{ 
	return &m_footlib[ilib].m_full_path; 
}

// get number of footprints under a heading in library file
int CFootLibFolder::GetNumFootprints( int ilib, int iheading )
{ 
	return m_footlib[ilib].m_heading[iheading].m_foot.GetSize(); 
}

// get footprint name
CString * CFootLibFolder::GetFootprintName( int ilib, int iheading, int ifoot )
{ 
	return &m_footlib[ilib].m_heading[iheading].m_foot[ifoot].m_name; 
}

// get footprint offset
int CFootLibFolder::GetFootprintOffset( int ilib, int iheading, int ifoot )
{ 
	return m_footlib[ilib].m_heading[iheading].m_foot[ifoot].m_offset; 
}

//********************* class CFootLibFolderMap *************************
//

// constructor
//
CFootLibFolderMap::CFootLibFolderMap()
{
}

// destructor, also destroys all CFootLibFolders in the map
//
CFootLibFolderMap::~CFootLibFolderMap()
{
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_folder_map.GetStartPosition(); pos != NULL; )
	{
		m_folder_map.GetNextAssoc( pos, name, ptr );
		CFootLibFolder * folder = (CFootLibFolder*)ptr;
		if( folder )
		{
			folder->Clear();
			delete folder;
		}
	}
}

// add CFootLibFolder to map
// if folder == NULL, the folder will be created on the first call to GetFolder
//
void CFootLibFolderMap::AddFolder( CString * full_path, CFootLibFolder * folder  )
{
	CString str = full_path->MakeLower();
	void * ptr;
	if( !m_folder_map.Lookup( str, ptr ) )
		m_folder_map.SetAt( str, folder );
}

CFootLibFolder * CFootLibFolderMap::GetFolder( CString * full_path )
{
	void * ptr;
	CFootLibFolder * folder;
	CString str = full_path->MakeLower();
	if( !m_folder_map.Lookup( str, ptr ) || ptr == NULL )
	{
		folder = new CFootLibFolder();
		folder->IndexAllLibs( &str );
		m_folder_map.SetAt( str, folder );
	}
	else
		folder = (CFootLibFolder*)ptr;
	return folder;
}

void CFootLibFolderMap::SetDefaultFolder( CString * def_full_path )
{
	m_default_folder = *def_full_path;
}

void CFootLibFolderMap::SetLastFolder( CString * last_full_path )
{
	m_last_folder = *last_full_path;
}

CString * CFootLibFolderMap::GetDefaultFolder()
{
	return( &m_default_folder );
}

CString * CFootLibFolderMap::GetLastFolder()
{
	if( m_last_folder != "" )
		return( &m_last_folder );
	else
		return( &m_default_folder );
}

