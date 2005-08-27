#pragma once
#include "afxwin.h"
#include "resource.h"


// CDlgSetTraceWidths dialog

class CDlgSetTraceWidths : public CDialog
{
	DECLARE_DYNAMIC(CDlgSetTraceWidths)

public:
	CDlgSetTraceWidths(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgSetTraceWidths();

// Dialog Data
	enum { IDD = IDD_SET_TRACE_WIDTHS };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_width;
	int m_via_width;
	int m_hole_width;
	BOOL m_apply;
	CArray<int> *m_w;
	CArray<int> *m_v_w;	
	CArray<int> *m_v_h_w;
private:
	CComboBox m_combo_width;
	CButton m_radio_default;
	bool m_radio_set_via;
	CEdit m_edit_via_pad;
	CEdit m_edit_via_hole;
	afx_msg void OnBnClickedRadioDef();
	afx_msg void OnBnClickedRadioSet();
public:
	afx_msg void OnCbnSelchangeComboWidth();
	afx_msg void OnCbnEditchangeComboWidth();
	CButton m_check_apply;
};
