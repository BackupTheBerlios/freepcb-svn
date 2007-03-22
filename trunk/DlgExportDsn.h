#pragma once
#include "afxwin.h"


// CDlgExportDsn dialog

class CDlgExportDsn : public CDialog
{
	DECLARE_DYNAMIC(CDlgExportDsn)

public:
	CDlgExportDsn(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgExportDsn();
	void Initialize( CString * dsn_filepath,
					 int num_board_outline_polys,
					 int bounds_poly, int signals_poly );


// Dialog Data
	enum { IDD = IDD_EXPORT_DSN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	BOOL m_bVerbose;
	BOOL m_bInfo;
	int m_num_polys;
	int m_bounds_poly;
	int m_signals_poly;
	CComboBox m_combo_bounds;
	CComboBox m_combo_signals;
	CString m_dsn_filepath;
};
