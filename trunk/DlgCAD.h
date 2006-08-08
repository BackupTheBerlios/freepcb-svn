#pragma once
#include "afxwin.h"
#include "DlgLog.h"


// CDlgCAD dialog

class CDlgCAD : public CDialog
{
	DECLARE_DYNAMIC(CDlgCAD)

public:
	CDlgCAD(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgCAD();
	void Initialize( double version, CString * folder, int num_copper_layers, int units, 
						 int fill_clearance, int mask_clearance, int thermal_width,
						 int pilot_diameter, int min_silkscreen_wid,
						 int outline_width, int hole_clearance,
						 int annular_ring_pins,  int annular_ring_vias,
						 int flags, int layers, int drill_file,
						 CArray<CPolyLine> * bd, CArray<CPolyLine> * sm, 
						 BOOL * bShowMessageForClearance,
						 CPartList * pl, CNetList * nl, CTextList * tl, CDisplayList * dl );
	void SetFields();
	void GetFields();
// Dialog Data
	enum { IDD = IDD_CAD };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	BOOL m_bShowMessageForClearance;
	double m_version;
	double m_file_version;
	CEdit m_edit_folder;
	CEdit m_edit_fill;
	CEdit m_edit_mask;
	CButton m_check_drill;
	CButton m_check_top_silk;
	CButton m_check_bottom_silk;
	CButton m_check_top_solder;
	CButton m_check_bottom_solder;
	CButton m_check_top_copper;
	CButton m_check_bottom_copper;
	CButton m_check_inner1;
	CButton m_check_inner2;
	CButton m_check_inner3;
	CButton m_check_inner4;
	CButton m_check_inner5;
	CButton m_check_inner6;
	CButton m_check_outline;
	CButton m_check_moires;
	CButton m_check_layer_text;
	int m_num_copper_layers;
	int m_fill_clearance;
	int m_hole_clearance;
	int m_mask_clearance;
	int m_thermal_width;
	int m_pilot_diameter;
	int m_min_silkscreen_width;
	int m_outline_width;
	int m_annular_ring_pins;
	int m_annular_ring_vias;
	int m_units;
	int m_flags;
	int m_layers;
	int m_drill_file;
	CArray<CPolyLine> * m_bd;
	CArray<CPolyLine> * m_sm;
	CPartList * m_pl; 
	CNetList * m_nl; 
	CTextList * m_tl; 
	CDisplayList * m_dl;
	CDlgLog * m_dlg_log;
	CString m_folder;
	afx_msg void OnBnClickedGo();
	CComboBox m_combo_units;
	afx_msg void OnCbnSelchangeComboCadUnits();
	CEdit m_edit_pilot_diam;
	CButton m_check_pilot;
	afx_msg void OnBnClickedCheckCadPilot();
	afx_msg void OnBnClickedCancel();
	CEdit m_edit_min_ss_w;
	CEdit m_edit_thermal_width;
	CEdit m_edit_outline_width;
	CEdit m_edit_hole_clearance;
	CEdit m_edit_ann_pins;
	CEdit m_edit_ann_vias;
	CButton m_check_thermal_pins;
	CButton m_check_thermal_vias;
	CButton m_check_mask_vias;
};
