#pragma once
#include "afxwin.h"
#include "afxcmn.h"
#include "FreePcbDoc.h"

// CDlgAddPart dialog

class CDlgAddPart : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddPart)

public:
	CDlgAddPart(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddPart();

// Dialog Data
	enum { IDD = IDD_ADD_PART };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	CString m_footprint_name;
	int m_ilib;
	int m_ihead;
	int m_ifoot;
	BOOL m_in_cache;	// true if new footprint selected from cache
	CShape m_shape;
	BOOL m_drag_flag;
	BOOL m_offboard_flag;
	partlist_info * m_pl;
	int m_ip;
	BOOL m_standalone;
	BOOL m_new_part;
	BOOL m_multiple;
	CComboBox m_combo_shape;
	CMapStringToPtr * m_footprint_cache_map;
	CFootLibFolderMap * m_footlibfoldermap;
	CFootLibFolder * m_folder;
	CTreeCtrl part_tree;
	CEdit m_edit_footprint;
	CButton m_radio_drag;
	CEdit m_edit_x;
	CEdit m_edit_y;
	CListBox m_list_angle;
	CListBox m_list_side;
	CEdit m_edit_ref_des;
	CButton m_radio_set;

public:
	void Initialize( partlist_info * pl, 
		int i, 
		BOOL standalone,
		BOOL new_part,
		BOOL multiple,
		CMapStringToPtr * shape_cache_map,
		CFootLibFolderMap * footlibfoldermap,
		int units,
		CDlgLog * log );
	void InitPartLibTree();
	BOOL GetDragFlag();
	void SetFields();
	void GetFields();

public:
	int m_units;
	int m_x, m_y;
	CEdit m_edit_package;
	CStatic m_preview;
	CComboBox m_combo_units;
	CEdit m_edit_author;
	CEdit m_edit_source;
	CEdit m_edit_desc;
	CButton m_button_browse;
	CEdit m_edit_lib;
	CButton m_radio_offboard;
	CDlgLog * m_dlg_log;

private:
	virtual BOOL OnInitDialog();
	afx_msg void OnLbnSelchangeListSide();
	afx_msg void OnBnClickedRadioDrag();
	afx_msg void OnBnClickedRadioSet();
	afx_msg void OnTvnSelchangedPartLibTree(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnLbnSelchangeListAngle();
	afx_msg void OnBnClickedRadioOffBoard();
	afx_msg void OnBnClickedButtonBrowse();
	afx_msg void OnCbnSelchangeComboAddPartUnits();
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedOk();
public:
	CEdit m_edit_value;
};
