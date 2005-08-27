#pragma once
#include "afxwin.h"


// CDlgImportOptions dialog

class CDlgImportOptions : public CDialog
{
	DECLARE_DYNAMIC(CDlgImportOptions)

public:
	CDlgImportOptions(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgImportOptions();
	void Initialize( int pn_flags, int flags );

// Dialog Data
	enum { IDD = IDD_IMPORT_OPTIONS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_pn_flags;		// shows whether to import parts or nets
	int m_flags;
	CButton m_radio_remove_parts;
	CButton m_radio_keep_parts_no_connections;
	CButton m_radio_keep_parts_and_connections;
	CButton m_radio_change_fp;
	CButton m_radio_keep_fp;
	CButton m_radio_remove_nets;
	CButton m_radio_keep_nets;
};
