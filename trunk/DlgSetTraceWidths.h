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
	BOOL bTraces;     
	BOOL bRevertTraces; 
	BOOL bVias;
	BOOL bDefaultVias;
	BOOL bRevertVias;
private:
	CComboBox m_combo_width;
	CButton m_radio_default;
	CEdit m_edit_via_pad;
	CEdit m_edit_via_hole;
	afx_msg void OnBnClickedRadioDef();
	afx_msg void OnBnClickedRadioSet();
public:
	afx_msg void OnCbnSelchangeComboWidth();
	afx_msg void OnCbnEditchangeComboWidth();
	CButton m_check_apply;
	CButton m_check_trace;
	CButton m_check_vias;
	afx_msg void OnBnClickedSetTrace();
	afx_msg void OnBnClickedSetVias();
	void SetFields();
	CButton m_radio_set;
	CButton m_radio_revert_traces;
	afx_msg void OnBnClickedRadioRevertTraces();
	CButton m_radio_revert_vias;
	afx_msg void OnBnClickedRadioRevertVias();
	afx_msg void OnBnClickedRadioSetTraceWidth();
	CButton m_radio_set_trace_width;
};
