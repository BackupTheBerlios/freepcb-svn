#pragma once
#include "afxwin.h"


// CDlgImportSes dialog

class CDlgImportSes : public CDialog
{
	DECLARE_DYNAMIC(CDlgImportSes)

public:
	CDlgImportSes(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgImportSes();
	void Initialize( CString * ses_filepath, 
					 CString * pcb_filepath );

// Dialog Data
	enum { IDD = IDD_IMPORT_SES };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CEdit m_edit_ses_filepath;
	CEdit m_edit_out_filepath;
	afx_msg void OnBnClickedButtonDef();
	afx_msg void OnBnClickedButtonBrowse();
	BOOL m_bLoad;
	CString m_ses_filepath;
	CString m_default_ses_filepath;
	CString m_pcb_filepath;
	CString m_routed_pcb_filepath;
	BOOL m_bVerbose;
};
