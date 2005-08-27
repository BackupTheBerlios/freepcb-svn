#pragma once
#include "afxwin.h"
#include "PartList.h"
#include "resource.h"

// CDlgRefText dialog

class CDlgRefText : public CDialog
{
	DECLARE_DYNAMIC(CDlgRefText)

public:
	CDlgRefText(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgRefText();
	void Initialize( CPartList * plist, cpart * part, CMapStringToPtr * shape_cache_map );
	void GetFields();
	void SetFields();

// Dialog Data
	enum { IDD = IDD_REF_TEXT };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	int m_units;
	CPartList * m_plist;
	cpart * m_part;
	CMapStringToPtr * m_footprint_cache_map;
	CEdit m_edit_ref_des;
	CEdit m_edit_footprint;
	CEdit m_edit_height;
	int m_height;
	CButton m_radio_set;
	CButton m_radio_def;
	CEdit m_edit_width;
	int m_width;
	int m_def_width;
	CEdit m_edit_def_width;
	afx_msg void OnBnClickedRadioSet();
	afx_msg void OnEnChangeEditCharHeight();
	afx_msg void OnBnClickedRadioDef();
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeComboRefTextUnits();
};
