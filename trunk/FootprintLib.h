// classes to manage footprint libraries
//
#pragma once
#include "DlgLog.h"

// this class represents a single footprint in a file
class CFootLibEntry
{
public:
	long m_offset;		// position in file
	CString m_name;		// footprint name
	long m_next_offset;	// offset to next heading or footprint in file
};

// this class represents a heading in a footprint library,
// containing multiple footprints
class CFootLibHeading
{
public:
	CString m_name;	// heading
	CArray<CFootLibEntry> m_foot;	// footprints under heading
};

// this class represents a footprint library
class CFootLib
{
public:
	CFootLib(){ m_indexed = m_expanded = FALSE; };
	CString m_full_path;	// path
	BOOL m_expanded;		// save expanded state in tree control
	BOOL m_indexed;			// TRUE if library has been indexed
	CArray<CFootLibHeading> m_heading; // headings in library
};

// this class represents a footprint library folder,
// containing multiple footprint library files
class CFootLibFolder
{
public:
	~CFootLibFolder(){ Clear(); };
	void Clear();
	void IndexAllLibs( CString * full_path );
	int GetNumLibs();
	void IndexLib( CString * file_name, CDlgLog * dlog = NULL );
	CString * GetFullPath();
	int SearchFileName( CString * fn );
	BOOL GetFootprintInfo( CString * name, int * ilib, int * iheading, 
		int * ifootprint, CString * file_name, int * offset, int * next_offset = NULL );
	CString * GetLibraryFilename( int ilib );
	int GetNumHeadings( int ilib );
	CString * GetHeading( int ilib, int iheading );
	int GetNumFootprints( int ilib, int iheading );
	CString * GetFootprintName( int ilib, int iheading, int ifoot );
	int GetFootprintOffset( int ilib, int iheading, int ifoot );
	void SetExpanded( int ilib, BOOL state ){ m_footlib[ilib].m_expanded = state; };
	BOOL GetExpanded( int ilib ){ return m_footlib[ilib].m_expanded; };
private:
	CString m_full_path_to_folder; 
	CArray<CFootLib> m_footlib;	// array of libraries
	CMapStringToPtr m_lib_map;	// map of all footprint names
};

// this class keeps track of multiple footprint library folders
// basically just maps CFootLibFolders to path names
class CFootLibFolderMap
{
public:
	CFootLibFolderMap();
	~CFootLibFolderMap();
	void AddFolder( CString * full_path, CFootLibFolder * folder );
	CFootLibFolder * GetFolder( CString * full_path );
	void SetDefaultFolder( CString * def_full_path );
	void SetLastFolder( CString * last_full_path );
	CString * GetDefaultFolder();
	CString * GetLastFolder();
private:
	CMapStringToPtr m_folder_map;
	CString m_default_folder;		// path to default folder
	CString m_last_folder;			// path to last folder used
};

