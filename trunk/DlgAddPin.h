#pragma once
#include "afxwin.h"


// CDlgAddPin dialog

class CDlgAddPin : public CDialog
{
	DECLARE_DYNAMIC(CDlgAddPin)

public:
	enum { ADD, EDIT };		// modes
	CDlgAddPin(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlgAddPin();
	void InitDialog( CEditShape * fp, int mode, 
		int pin_num, int units );
	void EnableFields();

	CEditShape * m_fp;

// Dialog Data
	enum { IDD = IDD_ADD_PIN };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	void SetFields();
	void GetFields();

	DECLARE_MESSAGE_MAP()
public:
	int m_mode;
	int m_units;			// MIL or MM
	int m_pin_num;
	CString m_pin_name;
	int m_num_pins;
	int m_same_as_pin_flag;
	int m_padstack_type;	// 0=SMT, 1=TH, 2=SMT(bottom)
	int m_same_as_pin_num;		
	int m_hole_diam;		// for TH
	int m_pad_orient;
	int m_top_pad_shape;
	int m_top_pad_width;
	int m_top_pad_length;
	int m_top_pad_radius;
	int m_inner_pad_shape;
	int m_inner_pad_width;
	int m_inner_pad_length;
	int m_inner_pad_radius;
	int m_bottom_pad_shape;
	int m_bottom_pad_width;
	int m_bottom_pad_length;
	int m_bottom_pad_radius;
	int m_x;
	int m_y;
	int m_row_orient;
	int m_row_spacing;
	BOOL m_drag_flag;	// TRUE to drag pad after exiting

	CButton m_radio_add_pin;
	CButton m_radio_add_row;
	afx_msg void OnBnClickedRadioAddPin();
	afx_msg void OnBnClickedRadioAddRow();
	CComboBox m_combo_units;
	CButton m_check_same_as_pin;
	CButton m_radio_smt;
	CButton m_radio_smt_bottom;
	CButton m_radio_th;
	afx_msg void OnBnClickedCheckSameAs();
	afx_msg void OnBnClickedRadioSmt();
	afx_msg void OnBnClickedRadioSmtBottom();
	afx_msg void OnBnClickedRadioTh();
	CComboBox m_combo_same_as_pin;
	CEdit m_edit_pin_name;
	CEdit m_edit_num_pins;
	CEdit m_edit_hole_diam;
	CButton m_radio_drag;
	afx_msg void OnBnClickedRadioDragPin();
	afx_msg void OnBnClickedRadioSetPinPos();
	CEdit m_edit_pin_x;
	CEdit m_edit_pin_y;
	CComboBox m_combo_top_shape;
	CEdit m_edit_top_width;
	CEdit m_edit_top_length;
	CEdit m_edit_top_radius;
	CButton m_check_inner_same_as;
	afx_msg void OnBnClickedCheckInnerSameAs();
	CComboBox m_combo_inner_shape;
	CEdit m_edit_inner_width;
	CEdit m_edit_inner_length;
	CEdit m_edit_inner_radius;
	CButton m_check_bottom_same_as;
	afx_msg void OnBnClickedCheckBottomSameAs();
	CComboBox m_combo_bottom_shape;
	CEdit m_edit_bottom_width;
	CEdit m_edit_bottom_length;
	CEdit m_edit_bottom_radius;
	CComboBox m_combo_row_orient;
	CEdit m_edit_row_spacing;
	afx_msg void OnCbnSelchangeComboPinUnits();
	afx_msg void OnCbnSelchangeComboRowOrient();
	afx_msg void OnCbnSelchangeComboTopPadShape();
	afx_msg void OnCbnSelchangeComboInnerPadShape();
	afx_msg void OnCbnSelchangeComboBottomPadShape();
	afx_msg void OnCbnSelchangeComboSameAsPin();
	CComboBox m_combo_pad_orient;
	afx_msg void OnCbnSelchangeComboPadOrient();
	CButton m_radio_set_pos;
	afx_msg void OnEnChangeEditTopPadW();
	afx_msg void OnEnChangeEditTopPadL();
	afx_msg void OnEnChangeEditTopPadRadius();
};
