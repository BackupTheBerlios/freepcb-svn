// FreePcbView.h : interface of the CFreePcbView class
//
/////////////////////////////////////////////////////////////////////////////


#if !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
#define AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_

#pragma once
#include "stdafx.h"
#include "DisplayList.h"
#include "FreePcbDoc.h"

class CFreePcbView;

// cursor modes
enum {
	CUR_NONE_SELECTED = 0,		// nothing selected
	CUR_SMCUTOUT_CORNER_SELECTED,	// corner of board outline sel. 
	CUR_SMCUTOUT_SIDE_SELECTED,	// edge of board outline sel.
	CUR_BOARD_CORNER_SELECTED,	// corner of board outline sel.
	CUR_BOARD_SIDE_SELECTED,	// edge of board outline sel.
	CUR_PART_SELECTED,			// part selected
	CUR_REF_SELECTED,			// ref text in part selected
	CUR_PAD_SELECTED,			// pad in part selected
	CUR_SEG_SELECTED,			// trace segment selected
	CUR_RAT_SELECTED,			// unrouted trace segment selected
	CUR_VTX_SELECTED,			// trace vertex selected
	CUR_END_VTX_SELECTED,		// end vertex of stub trace selected
	CUR_CONNECT_SELECTED,		// entire connection selected
	CUR_NET_SELECTED,			// entire net selected
	CUR_TEXT_SELECTED,			// free text selected
	CUR_AREA_CORNER_SELECTED,	// corner of copper area selected
	CUR_AREA_SIDE_SELECTED,		// edge of copper area selected
	CUR_DRE_SELECTED,			// DRC error selected
	CUR_GROUP_SELECTED,			// multiple parts selected
	CUR_NUM_SELECTED_MODES,		// number of SELECTED modes

	CUR_ADD_BOARD,		// dragging starting point of board outline
	CUR_DRAG_BOARD_1,	// dragging first corner of board outline
	CUR_DRAG_BOARD,		// dragging next corner of board outline
	CUR_DRAG_BOARD_INSERT,	// dragging board corner being inserted
	CUR_DRAG_BOARD_MOVE,	// dragging board corner being moved
	CUR_DRAG_PART,		// dragging part
	CUR_DRAG_REF,		// dragging ref text of part
	CUR_DRAG_RAT,		// dragging ratline for trace segment
	CUR_DRAG_VTX,		// dragging trace vertex
	CUR_DRAG_VTX_INSERT,	// dragging new vertex being inserted
	CUR_DRAG_END_VTX,	// dragging end vertex being moved
	CUR_DRAG_TEXT,		// dragging text box
	CUR_ADD_AREA,		// setting starting point for copper area
	CUR_DRAG_AREA_1,	// dragging first corner for copper area
	CUR_DRAG_AREA,		// dragging next corner for copper area
	CUR_DRAG_AREA_INSERT,	// dragging area corner being inserted
	CUR_DRAG_AREA_MOVE,		// dragging area corner being moved
	CUR_ADD_AREA_CUTOUT,	// setting starting point for area cutout
	CUR_DRAG_AREA_CUTOUT_1,	// dragging first corner for area cutout
	CUR_DRAG_AREA_CUTOUT,	// dragging next corner for area cutout
	CUR_ADD_SMCUTOUT,		// setting starting point of solder mask cutout
	CUR_DRAG_SMCUTOUT_1,	// dragging first corner of solder mask cutout
	CUR_DRAG_SMCUTOUT,		// dragging next corner of solder mask cutout
	CUR_DRAG_SMCUTOUT_INSERT,	// dragging solder mask cutout corner being inserted
	CUR_DRAG_SMCUTOUT_MOVE,		// dragging solder mask cutout corner being moved
	CUR_DRAG_STUB,		// dragging ratline to next stub endpoint
	CUR_DRAG_CONNECT,	// dragging ratline to new connection
	CUR_DRAG_RAT_PIN,	// dragging ratline to new end pin of trace
	CUR_MOVE_ORIGIN,	// dragging new origin
	CUR_DRAG_GROUP,		// dragging a group of parts/segments
	CUR_DRAG_GROUP_ADD,	// dragging a group being added
	CUR_NUM_MODES		// number of modes
};

// function key options
enum {
	FK_NONE = 0,
	FK_MOVE_PART,
	FK_MOVE_REF,
	FK_ROTATE_PART,
	FK_ROTATE_REF,
	FK_SIDE,
	FK_ROUTE,
	FK_UNROUTE,
	FK_REROUTE,
	FK_COMPLETE,
	FK_ADD_PART,
	FK_ADD_NET,
	FK_ADD_TEXT,
	FK_ADD_GRAPHIC,
	FK_REDO_RATLINES,
	FK_ADD_AREA,
	FK_DELETE_PART,
	FK_DELETE_VERTEX,
	FK_MOVE_VERTEX,
	FK_MOVE_CORNER,
	FK_ADD_CORNER,
	FK_DELETE_CORNER,
	FK_ADD_CONNECT,
	FK_DETACH_NET,
	FK_ATTACH_NET,
	FK_DELETE_CONNECT,
	FK_FORCE_VIA,
	FK_SET_WIDTH,
	FK_LOCK_CONNECT,
	FK_UNLOCK_CONNECT,
	FK_MOVE_TEXT,
	FK_ROTATE_TEXT,
	FK_DELETE_TEXT,
	FK_POLY_STRAIGHT,
	FK_POLY_ARC_CW,
	FK_POLY_ARC_CCW,
	FK_EDIT_PART,
	FK_EDIT_FOOTPRINT,
	FK_GLUE_PART,
	FK_UNGLUE_PART,
	FK_UNDO,
	FK_SET_SIZE,
	FK_START_STUB,
	FK_EDIT_TEXT,
	FK_SET_POSITION,
	FK_DELETE_OUTLINE,
	FK_DELETE_AREA,
	FK_DELETE_CUTOUT,
	FK_ADD_SEGMENT,
	FK_ADD_VIA,
	FK_DELETE_VIA,
	FK_DELETE_SEGMENT,
	FK_UNROUTE_TRACE,
	FK_CHANGE_PIN,
	FK_AREA_CUTOUT,
	FK_CHANGE_LAYER,
	FK_EDIT_NET,
	FK_MOVE_GROUP,
	FK_VIA_SIZE,
	FK_ADD_VERTEX,
	FK_NUM_OPTIONS,
	FK_ARROW
};

// function key menu strings
const char fk_str[FK_NUM_OPTIONS*2+2][32] = 
{ 
	"",			"",
	" Move",	" Part",
	" Move",	" Ref Text",
	" Rotate",	" Part",
	" Rotate",	" Ref Text",
	" Change",	" Side",
	" Route",	" Segment",
	" Unroute",	" Segment",
	" Reroute",	" Segment",
	" Complete"," Segment",
	" Add",		" Part",
	" Add",		" Net",
	" Add",		" Text",
	" Add",		" Graphics",
	" Recalc.",	" Ratlines",
	" Add",		" Area",
	" Delete",  " Part",
	" Delete",  " Vertex",
	" Move",	" Vertex",
	" Move",	" Corner",
	" Add",		" Corner",
	" Delete",	" Corner",
	" Connect",	" Pin",
	" Detach",	" Net",
	" Set",		" Net",
	" Delete",	" Connect",
	" Force",	" Via",
	" Set",		" Width",
	" Lock",	" Connect",
	" Unlock",	" Connect",
	" Move",	" Text",
	" Rotate",	" Text",
	" Delete",	" Text",
	" Straight"," Line",
	" Arc",		" (CW)",
	" Arc",		" (CCW)",
	" Edit",	" Part",
	" Edit",	" Footprint",
	" Glue",	" Part",
	" Unglue",	" Part",
	" Undo",	"",
	" Set",		" Size",
	" Start",	" Stub",
	" Edit",	" Text",
	" Set",		" Position",
	" Delete",	" Outline",
	" Delete",	" Area",
	" Delete",	" Cutout",
	" Add",		" Segment",
	" Add",		" Via",
	" Delete",	" Via",
	" Delete",	" Segment",
	" Unroute",	" Trace",
	" Change",	" Pin",
	" Add",		" Cutout",
	" Change",	" Layer",
	" Edit",	" Net",
	" Move",	" Group",
	" Set",		" Via Size",
	" Add",		" Vertex",
	" ****",	" ****"
};

// selection masks
enum {	SEL_MASK_PARTS = 0,
		SEL_MASK_REF,
		SEL_MASK_PINS,
		SEL_MASK_CON,
		SEL_MASK_VIA,
		SEL_MASK_AREAS,
		SEL_MASK_TEXT,
		SEL_MASK_SM,
		SEL_MASK_BOARD,
		SEL_MASK_DRC,
		NUM_SEL_MASKS
};

// snap modes
enum {	SM_GRID_POINTS,	// snap to grid points
		SM_GRID_LINES	// snap to grid lines
};

// selection mask menu strings
const char sel_mask_str[NUM_SEL_MASKS][32] = 
{
	"parts",
	"ref des",
	"pins",
	"traces/ratlines",
	"vertices/vias",
	"copper areas",
	"text",
	"sm cutouts",
	"board outline",
	"DRC errors"
};

class CFreePcbView : public CView
{
public: // create from serialization only
	CFreePcbView();
	DECLARE_DYNCREATE(CFreePcbView)

// Attributes
public:
	CFreePcbDoc* GetDocument();

// member variables
public:
	CFreePcbDoc * m_Doc;	// the document
	CDisplayList * m_dlist;	// the display list

	// Windows fonts
	CFont m_small_font;

	// cursor mode
	int m_cursor_mode;		// see enum above

	// debug flag
	int m_debug_flag;

	// flag to indicate that a newly-created item is being dragged,
	// as opposed to an existing item
	// if so, right-clicking or ESC will delete item not restore it
	BOOL m_dragging_new_item;

	// parameters for dragging selection rectangle
	BOOL m_bLButtonDown;
	BOOL m_bDraggingRect;
	CPoint m_start_pt;
	CRect m_drag_rect, m_last_drag_rect;
	CRect m_sel_rect;		// rectangle used for selection

	// mode for drawing new polyline segments
	int m_polyline_style;	// STRAIGHT, ARC_CW or ARC_CCW
	int m_polyline_hatch;	// NONE, DIAGONAL_FULL or DIAGONAL_EDGE
	int m_polyline_layer;	// layer being drawn

	// flag to disable context menu on right-click,
	// if right-click handled some other way
	int m_disable_context_menu;

	// selection mask
	int m_sel_mask;
	id m_mask_id[NUM_SEL_MASKS];

	// selected items
	id m_sel_id;			// id of selected item
	cpart * m_sel_part;		// pointer to part, if selected
	cnet * m_sel_net;		// pointer to net, if selected
	CText * m_sel_text;		// pointer to text, if selected
	DRError * m_sel_dre;	// pointer to DRC error, if selected
	int m_sel_layer;		// layer of selected item
	CArray<id> m_sel_ids;	// array of multiple selections
	CArray<void*> m_sel_ptrs;	// array of pointers to selected items

#define m_sel_ic m_sel_id.i		// index of selected connection
#define m_sel_ia m_sel_id.i		// index of selected area
#define m_sel_is m_sel_id.ii	// index of selected side, segment, or corner
#define m_sel_iv m_sel_id.ii	// index of selected vertex
#define m_sel_con m_sel_net->connect[m_sel_ic]	// selected connection
#define m_sel_seg m_sel_con.seg[m_sel_is]		// selected side or segment
#define m_sel_vtx m_sel_con.vtx[m_sel_is]		// selected vertex
#define m_sel_next_vtx m_sel_con.vtx[m_sel_is+1]	// next vertex
#define m_sel_start_pin m_sel_net->pin[m_sel_con.start_pin]
#define m_sel_end_pin m_sel_net->pin[m_sel_con.end_pin]

	// direction of routing
	int m_dir;			// 0 = forward, 1 = back

	// display coordinate mapping
	double m_pcbu_per_pixel;	// pcb units per pixel
	double m_org_x;				// x-coord of left side of screen in pcb units
	double m_org_y;				// y-coord of bottom of screen in pcb units

	// grid stuff
	CPoint m_snap_angle_ref;	// reference point for snap angle
	int m_snap_mode;			// snap mode
	int m_inflection_mode;		// inflection mode for routing

	// window parameters
	CRect m_client_r;		// in device coords
	int m_left_pane_w;		// width of pane at left of screen for layer selection, etc.
	int m_bottom_pane_h;	// height of pane at bottom of screen for key assignments, etc.
	CRgn m_pcb_rgn;			// region for the pcb
	BOOL m_left_pane_invalid;	// flag to erase and redraw left pane

	// active layer for routing and placement
	int m_active_layer;

	// starting point for a new copper area 
	int m_area_start_x;
	int m_area_start_y;
	
	// mouse
	CPoint m_last_mouse_point;	// last mouse position
	CPoint m_last_cursor_point;	// last cursor position (may be different from mouse)
	CPoint m_from_pt;			// for dragging rect, origin

	// function key shortcuts
	int m_fkey_option[12];
	int m_fkey_command[12];
	char m_fkey_str[24][32];

	// memory DC and bitmap
	BOOL m_memDC_created;
	CDC m_memDC;
	CBitmap m_bitmap;
	CBitmap * m_old_bitmap;
	CRect m_bitmap_rect;
	
// Operations
public:
	void InitInstance();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CFreePcbView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CFreePcbView();
	void InitializeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	int SetDCToWorldCoords( CDC * pDC );
	CPoint ScreenToPCB( CPoint point );
	CPoint PCBToScreen( CPoint point );
	CPoint WindowToPCB( CPoint point );
	void SetCursorMode( int mode );
	void SetFKText( int mode );
	void DrawBottomPane();
	int ShowCursor();
	int ShowSelectStatus();
	void ShowRelativeDistance( int x, int y );
	int ShowActiveLayer();
	int SelectPart( cpart * part );
	void CancelSelection();
	int SetWidth( int mode );
	int GetWidthsForSegment( int * w, int * via_w, int * via_hole_w );
	void ChangeTraceLayer( int mode, int old_layer=0 );
	void MoveOrigin( int x_off, int y_off );
	void SelectItemsInRect( CRect r, BOOL bAddToGroup );
	void SetSelMaskArray( int mask );
	void StartDraggingGroup( BOOL bAdd=FALSE, int x=0, int y=0 );
	void CancelDraggingGroup();
	void MoveGroup( int dx, int dy );
	void HighlightGroup();
	int FindItemInGroup( void * ptr, id * tid );	
	BOOL GluedPartsInGroup();
	void UngluePartsInGroup();
	BOOL CurNone();
	BOOL CurSelected();
	BOOL CurDragging();
	BOOL CurDraggingRouting();
	BOOL CurDraggingPlacement();
	void SnapCursorPoint( CPoint wp, UINT nFlags );
	void InvalidateLeftPane(){ m_left_pane_invalid = TRUE; }
	void SaveUndoInfoForNet( cnet * net, int type, BOOL new_event=TRUE );
	void SaveUndoInfoForNetAndConnections( cnet * net, int type=CNetList::UNDO_NET_MODIFY, BOOL new_event=TRUE );
	void SaveUndoInfoForConnection( cnet * net, int ic, BOOL new_event=TRUE );
	void SaveUndoInfoForPart( cpart * part, int type, BOOL new_event=TRUE );
	void SaveUndoInfoForPartRename( cpart * part, CString * old_ref_des, BOOL new_event=TRUE );
	void SaveUndoInfoForPartAndNets( cpart * part, int type, BOOL new_event=TRUE );
	void SaveUndoInfoFor2PartsAndNets( cpart * part1, cpart * part2, BOOL new_event=TRUE );
	void SaveUndoInfoForArea( cnet * net, int iarea, int type, BOOL new_event=TRUE );
	void SaveUndoInfoForAllAreasInNet( cnet * net, BOOL new_event=TRUE );
	void SaveUndoInfoForNetAndConnectionsAndArea( cnet * net, int iarea, int type, BOOL new_event=TRUE );
	void SaveUndoInfoForNetAndConnectionsAndAreas( cnet * net, BOOL new_event=TRUE );
	void SaveUndoInfoForAllNetsAndConnectionsAndAreas( BOOL new_event=TRUE );
	void SaveUndoInfoForAllNets( BOOL new_event=TRUE );
	void SaveUndoInfoForMoveOrigin( int x_off, int y_off );
	void SaveUndoInfoForBoardOutlines( int type, BOOL new_event=TRUE );
	void SaveUndoInfoForSMCutouts( int type, BOOL new_event=TRUE );
	void SaveUndoInfoForText( CText * text, int type, BOOL new_event=TRUE );
	void SaveUndoInfoForGroup( int type );
	void OnExternalChangeFootprint( CShape * fp );
	void HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags);
	void CFreePcbView::TryToReselectAreaCorner( int x, int y );
	void ReselectNetItemIfConnectionsChanged( int new_ic );
protected:

// Generated message map functions
protected:
	//{{AFX_MSG(CFreePcbView)
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
//	afx_msg void OnSysChar(UINT nChar, UINT nRepCnt, UINT nFlags);
//	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnContextMenu(CWnd* /*pWnd*/, CPoint /*point*/);
	afx_msg void OnPartMove();
	afx_msg void OnTextAdd();
	afx_msg void OnTextDelete();
	afx_msg void OnTextMove();
	afx_msg void OnPartGlue();
	afx_msg void OnPartUnglue();
	afx_msg void OnPartDelete();
	afx_msg void OnPartOptimize();
	afx_msg void OnRefMove();
	afx_msg void OnPadOptimize();
	afx_msg void OnPadAddToNet();
	afx_msg void OnPadDetachFromNet();
	afx_msg void OnPadConnectToPin();
	afx_msg void OnSegmentSetWidth();
	afx_msg void OnSegmentUnroute();
	afx_msg void OnRatlineRoute();
	afx_msg void OnRatlineOptimize();
	afx_msg void OnVertexMove();
	afx_msg void OnVertexConnectToPin();
	afx_msg void OnVertexSize();
	afx_msg void OnVertexDelete();
	afx_msg void OnRatlineComplete();
	afx_msg void OnRatlineSetWidth();
	afx_msg void OnRatlineDeleteConnection();
	afx_msg void OnRatlineLockConnection();
	afx_msg void OnRatlineUnlockConnection();
	afx_msg void OnRatlineChangeEndPin();
	afx_msg void OnTextEdit();
	afx_msg void OnAddBoardOutline();
	afx_msg void OnBoardCornerMove();
	afx_msg void OnBoardCornerEdit();
	afx_msg void OnBoardCornerDelete();
	afx_msg void OnBoardSideAddCorner();
	afx_msg void OnBoardDeleteOutline();
	afx_msg void OnPadStartStubTrace();
	afx_msg void OnSegmentDelete();
	afx_msg void OnEndVertexMove();
	afx_msg void OnEndVertexAddSegments();
	afx_msg void OnEndVertexAddConnection();
	afx_msg void OnEndVertexDelete();
	afx_msg void OnEndVertexEdit();
	afx_msg void OnAreaCornerMove();
	afx_msg void OnAreaCornerDelete();
	afx_msg void OnAreaCornerDeleteArea();
	afx_msg void OnAreaSideAddCorner();
	afx_msg void OnAreaSideDeleteArea();
	afx_msg void OnAddArea();
	afx_msg void OnAreaAddCutout();
	afx_msg void OnAreaDeleteCutout();
	afx_msg void OnEndVertexAddVia();
	afx_msg void OnEndVertexRemoveVia();
	afx_msg void OnSegmentDeleteTrace();
	afx_msg void OnAreaCornerProperties();
	afx_msg void OnRefProperties();
	afx_msg void OnVertexProperties();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnBoardSideConvertToStraightLine();
	afx_msg void OnBoardSideConvertToArcCw();
	afx_msg void OnBoardSideConvertToArcCcw();
	afx_msg void OnUnrouteTrace();
	afx_msg void OnViewEntireBoard();
	afx_msg void OnViewAllElements();
	afx_msg void OnAreaEdgeHatchStyle();
	afx_msg void OnPartEditFootprint();
	afx_msg void OnPartEditThisFootprint();
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnViewFindpart();
	afx_msg void OnFootprintWizard();
	afx_msg void OnFootprintEditor();
	afx_msg void OnCheckPartsAndNets();
	afx_msg void OnDrc();
	afx_msg void OnClearDRC();
	afx_msg void OnViewAll();
	afx_msg void OnAddSoldermaskCutout();
	afx_msg void OnSmCornerMove();
	afx_msg void OnSmCornerSetPosition();
	afx_msg void OnSmCornerDeleteCorner();
	afx_msg void OnSmCornerDeleteCutout();
	afx_msg void OnSmSideInsertCorner();
	afx_msg void OnSmSideHatchStyle();
	afx_msg void OnSmSideDeleteCutout();
	afx_msg void OnPartChangeSide();
	afx_msg void OnPartRotate();
	afx_msg void OnNetSetWidth();
	afx_msg void OnConnectSetWidth();
	afx_msg void OnConnectUnroutetrace();
	afx_msg void OnConnectDeletetrace();
	afx_msg void OnSegmentChangeLayer();
	afx_msg void OnConnectChangeLayer();
	afx_msg void OnNetChangeLayer();
	afx_msg void OnNetEditnet();
	afx_msg void OnToolsMoveOrigin();
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnGroupMove();
	afx_msg void OnAddSimilarArea();
	afx_msg void OnSegmentAddVertex();
	LONG OnChangeVisibleGrid( UINT wp, LONG lp );
	LONG OnChangePlacementGrid( UINT wp, LONG lp );
	LONG OnChangeRoutingGrid( UINT wp, LONG lp );
	LONG OnChangeSnapAngle( UINT wp, LONG lp );
	LONG OnChangeUnits( UINT wp, LONG lp );
	afx_msg void OnAreaChangeLayer();
	afx_msg void OnAreaEdgeApplyClearances();
	afx_msg void OnGroupSaveToFile();
	afx_msg void OnGroupCopy();
	afx_msg void OnGroupPaste();
	afx_msg void OnEditCopy();
	afx_msg void OnEditPaste();
};

#ifndef _DEBUG  // debug version in FreePcbView.cpp
inline CFreePcbDoc* CFreePcbView::GetDocument()
   { return (CFreePcbDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_FREEPCBVIEW_H__BE1CA173_E2B9_4252_8422_0B9767B01566__INCLUDED_)
