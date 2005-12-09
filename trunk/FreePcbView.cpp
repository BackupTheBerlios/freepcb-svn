// FreePcbView.cpp : implementation of the CFreePcbView class
//

#include "stdafx.h"
#include "DlgAddText.h"
#include "DlgAssignNet.h"
#include "DlgSetSegmentWidth.h"
#include "DlgEditBoardCorner.h"
#include "DlgAddArea.h"
#include "DlgRefText.h"
#include "MyToolBar.h"
#include <Mmsystem.h>
#include <sys/timeb.h>
#include <time.h>
#include <math.h>
#include "freepcbview.h"
#include "DlgAddPart.h"
#include "DlgSetAreaHatch.h"
#include "DlgDupFootprintName.h"
#include "DlgFindPart.h"
#include "DlgAddMaskCutout.h"
#include "DlgChangeLayer.h"
#include "DlgEditNet.h"
#include "DlgMoveOrigin.h"

// globals
extern CFreePcbApp theApp;
BOOL t_pressed = FALSE;
BOOL n_pressed = FALSE;
BOOL gLastKeyWasArrow = FALSE;
int gTotalArrowMoveX = 0;
int gTotalArrowMoveY = 0;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define ZOOM_RATIO 1.4

// constants for function key menu
#define FKEY_OFFSET_X 4
#define FKEY_OFFSET_Y 4
#define	FKEY_R_W 70
#define FKEY_R_H 30
#define FKEY_STEP (FKEY_R_W+5)
#define FKEY_GAP 20
#define FKEY_SEP_W 16

// constants for layer list
#define VSTEP 14

// these must be changed if context menu is edited
enum {
	CONTEXT_NONE = 0,
	CONTEXT_PART,
	CONTEXT_REF_TEXT,
	CONTEXT_PAD,
	CONTEXT_SEGMENT,
	CONTEXT_RATLINE,
	CONTEXT_VERTEX,
	CONTEXT_TEXT,
	CONTEXT_AREA_CORNER,
	CONTEXT_AREA_EDGE,
	CONTEXT_BOARD_CORNER,
	CONTEXT_BOARD_SIDE,
	CONTEXT_END_VERTEX,
	CONTEXT_FP_PAD,
	CONTEXT_SM_CORNER,
	CONTEXT_SM_SIDE,
	CONTEXT_CONNECT,
	CONTEXT_NET,
	CONTEXT_GROUP
};

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView

IMPLEMENT_DYNCREATE(CFreePcbView, CView)

BEGIN_MESSAGE_MAP(CFreePcbView, CView)
	//{{AFX_MSG_MAP(CFreePcbView)
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_KEYDOWN()
	ON_WM_MOUSEMOVE()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_SYSKEYDOWN()
	ON_WM_SYSKEYUP()
	ON_WM_MOUSEWHEEL()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
//	ON_WM_SYSCHAR()
//ON_WM_SYSCOMMAND()
ON_WM_CONTEXTMENU()
ON_COMMAND(ID_PART_MOVE, OnPartMove)
ON_COMMAND(ID_NONE_ADDTEXT, OnTextAdd)
ON_COMMAND(ID_TEXT_DELETE, OnTextDelete)
ON_COMMAND(ID_TEXT_MOVE, OnTextMove)
ON_COMMAND(ID_PART_GLUE, OnPartGlue)
ON_COMMAND(ID_PART_UNGLUE, OnPartUnglue)
ON_COMMAND(ID_PART_DELETE, OnPartDelete)
ON_COMMAND(ID_PART_OPTIMIZE, OnPartOptimize)
ON_COMMAND(ID_REF_MOVE, OnRefMove)
ON_COMMAND(ID_PAD_OPTIMIZERATLINES, OnPadOptimize)
ON_COMMAND(ID_PAD_ADDTONET, OnPadAddToNet)
ON_COMMAND(ID_PAD_DETACHFROMNET, OnPadDetachFromNet)
ON_COMMAND(ID_PAD_CONNECTTOPIN, OnPadConnectToPin)
ON_COMMAND(ID_SEGMENT_SETWIDTH, OnSegmentSetWidth)
ON_COMMAND(ID_SEGMENT_UNROUTE, OnSegmentUnroute)
ON_COMMAND(ID_RATLINE_ROUTE, OnRatlineRoute)
ON_COMMAND(ID_RATLINE_OPTIMIZE, OnRatlineOptimize)
ON_COMMAND(ID_VERTEX_MOVE, OnVertexMove)
ON_COMMAND(ID_VERTEX_DELETE, OnVertexDelete)
ON_COMMAND(ID_RATLINE_COMPLETE, OnRatlineComplete)
ON_COMMAND(ID_RATLINE_SETWIDTH, OnRatlineSetWidth)
ON_COMMAND(ID_RATLINE_DELETECONNECTION, OnRatlineDeleteConnection)
ON_COMMAND(ID_RATLINE_LOCKCONNECTION, OnRatlineLockConnection)
ON_COMMAND(ID_RATLINE_UNLOCKCONNECTION, OnRatlineUnlockConnection)
ON_COMMAND(ID_TEXT_EDIT, OnTextEdit)
ON_COMMAND(ID_ADD_BOARDOUTLINE, OnAddBoardOutline)
ON_COMMAND(ID_NONE_ADDBOARDOUTLINE, OnAddBoardOutline)
ON_COMMAND(ID_BOARDCORNER_MOVE, OnBoardCornerMove)
ON_COMMAND(ID_BOARDCORNER_EDIT, OnBoardCornerEdit)
ON_COMMAND(ID_BOARDCORNER_DELETECORNER, OnBoardCornerDelete)
ON_COMMAND(ID_BOARDCORNER_DELETEOUTLINE, OnBoardDeleteOutline)
ON_COMMAND(ID_BOARDSIDE_INSERTCORNER, OnBoardSideAddCorner)
ON_COMMAND(ID_BOARDSIDE_DELETEOUTLINE, OnBoardDeleteOutline)
ON_COMMAND(ID_PAD_STARTSTUBTRACE, OnPadStartStubTrace)
ON_COMMAND(ID_SEGMENT_DELETE, OnSegmentDelete)
ON_COMMAND(ID_ENDVERTEX_MOVE, OnEndVertexMove)
ON_COMMAND(ID_ENDVERTEX_ADDSEGMENTS, OnEndVertexAddSegments)
ON_COMMAND(ID_ENDVERTEX_ADDCONNECTION, OnEndVertexAddConnection)
ON_COMMAND(ID_ENDVERTEX_DELETE, OnEndVertexDelete)
ON_COMMAND(ID_ENDVERTEX_EDIT, OnEndVertexEdit)
ON_COMMAND(ID_AREACORNER_MOVE, OnAreaCornerMove)
ON_COMMAND(ID_AREACORNER_DELETE, OnAreaCornerDelete)
ON_COMMAND(ID_AREACORNER_DELETEAREA, OnAreaCornerDeleteArea)
ON_COMMAND(ID_AREAEDGE_ADDCORNER, OnAreaSideAddCorner)
ON_COMMAND(ID_AREAEDGE_DELETE, OnAreaSideDeleteArea)
ON_COMMAND(ID_ADD_AREA, OnAddArea)
ON_COMMAND(ID_NONE_ADDCOPPERAREA, OnAddArea)
ON_COMMAND(ID_ENDVERTEX_ADDVIA, OnEndVertexAddVia)
ON_COMMAND(ID_ENDVERTEX_REMOVEVIA, OnEndVertexRemoveVia)
ON_MESSAGE( WM_USER_VISIBLE_GRID, OnChangeVisibleGrid )
ON_COMMAND(ID_ADD_TEXT, OnTextAdd)
ON_COMMAND(ID_SEGMENT_DELETETRACE, OnSegmentDeleteTrace)
ON_COMMAND(ID_AREACORNER_PROPERTIES, OnAreaCornerProperties)
ON_COMMAND(ID_REF_PROPERTIES, OnRefProperties)
ON_COMMAND(ID_VERTEX_PROPERITES, OnVertexProperties)
ON_WM_ERASEBKGND()
ON_COMMAND(ID_BOARDSIDE_CONVERTTOSTRAIGHTLINE, OnBoardSideConvertToStraightLine)
ON_COMMAND(ID_BOARDSIDE_CONVERTTOARC_CW, OnBoardSideConvertToArcCw)
ON_COMMAND(ID_BOARDSIDE_CONVERTTOARC_CCW, OnBoardSideConvertToArcCcw)
ON_COMMAND(ID_SEGMENT_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_VERTEX_UNROUTETRACE, OnUnrouteTrace)
ON_COMMAND(ID_VIEW_ENTIREBOARD, OnViewEntireBoard)
ON_COMMAND(ID_VIEW_ALLELEMENTS, OnViewAllElements)
ON_COMMAND(ID_AREAEDGE_HATCHSTYLE, OnAreaedgeHatchstyle)
ON_COMMAND(ID_PART_EDITFOOTPRINT, OnPartEditThisFootprint)
ON_COMMAND(ID_PART_SET_REF, OnRefProperties)
ON_COMMAND(ID_RATLINE_CHANGEPIN, OnRatlineChangeEndPin)
ON_WM_KEYUP()
ON_COMMAND(ID_VIEW_FINDPART, OnViewFindpart)
ON_COMMAND(ID_NONE_FOOTPRINTWIZARD, OnFootprintWizard)
ON_COMMAND(ID_NONE_FOOTPRINTEDITOR, OnFootprintEditor)
ON_COMMAND(ID_NONE_CHECKPARTSANDNETS, OnCheckPartsAndNets)
ON_COMMAND(ID_NONE_DRC, OnDrc)
ON_COMMAND(ID_NONE_CLEARDRCERRORS, OnClearDRC)
ON_COMMAND(ID_NONE_VIEWALL, OnViewAll)
ON_COMMAND(ID_ADD_SOLDERMASKCUTOUT, OnAddSoldermaskCutout)
ON_COMMAND(ID_SMCORNER_MOVE, OnSmCornerMove)
ON_COMMAND(ID_SMCORNER_SETPOSITION, OnSmCornerSetPosition)
ON_COMMAND(ID_SMCORNER_DELETECORNER, OnSmCornerDeleteCorner)
ON_COMMAND(ID_SMCORNER_DELETECUTOUT, OnSmCornerDeleteCutout)
ON_COMMAND(ID_SMSIDE_INSERTCORNER, OnSmSideInsertCorner)
ON_COMMAND(ID_SMSIDE_HATCHSTYLE, OnSmSideHatchStyle)
ON_COMMAND(ID_SMSIDE_DELETECUTOUT, OnSmSideDeleteCutout)
ON_COMMAND(ID_PART_CHANGESIDE, OnPartChangeSide)
ON_COMMAND(ID_PART_ROTATE, OnPartRotate)
ON_COMMAND(ID_AREAEDGE_ADDCUTOUT, OnAreaAddCutout)
ON_COMMAND(ID_AREACORNER_ADDCUTOUT, OnAreaAddCutout)
ON_COMMAND(ID_NET_SETWIDTH, OnNetSetWidth)
ON_COMMAND(ID_CONNECT_SETWIDTH, OnConnectSetWidth)
ON_COMMAND(ID_CONNECT_UNROUTETRACE, OnConnectUnroutetrace)
ON_COMMAND(ID_CONNECT_DELETETRACE, OnConnectDeletetrace)
ON_COMMAND(ID_SEGMENT_CHANGELAYER, OnSegmentChangeLayer)
ON_COMMAND(ID_CONNECT_CHANGELAYER, OnConnectChangeLayer)
ON_COMMAND(ID_NET_CHANGELAYER, OnNetChangeLayer)
ON_COMMAND(ID_NET_EDITNET, OnNetEditnet)
ON_COMMAND(ID_TOOLS_MOVEORIGIN, OnToolsMoveOrigin)
ON_WM_LBUTTONUP()
ON_COMMAND(ID_GROUP_MOVE, OnGroupMove)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView construction/destruction

CFreePcbView::CFreePcbView()
{
	// GetDocument() is not available at this point
	m_small_font.CreateFont( 14, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH | FF_DONTCARE, "Arial" );
#if 0
	m_small_font.CreateFont( 10, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET,
		OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS, DEFAULT_QUALITY, 
		DEFAULT_PITCH | FF_DONTCARE, "MS Sans Serif" );
#endif
	m_Doc = NULL;
	m_dlist = 0;
	m_last_mouse_point.x = 0;
	m_last_mouse_point.y = 0;
	m_last_cursor_point.x = 0;
	m_last_cursor_point.y = 0;
	m_left_pane_w = 110;	// the left pane on screen is this wide (pixels)
	m_bottom_pane_h = 40;	// the bottom pane on screen is this high (pixels)
	m_memDC_created = FALSE;
	m_dragging_new_item = FALSE;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	CalibrateTimer();
 }

// initialize the view object
// this code can't be placed in the constructor, because it depends on document
// don't try to draw window until this function has been called
// need only be called once
//
void CFreePcbView::InitInstance()
{
	// this should be called from InitInstance function of CApp,
	// after the document is created
	m_Doc = GetDocument();
	ASSERT_VALID(m_Doc);
	m_Doc->m_edit_footprint = FALSE;
	m_dlist = m_Doc->m_dlist;
	InitializeView();
	m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, 
		m_pcbu_per_pixel, m_org_x, m_org_y );
	for(int i=0; i<m_Doc->m_num_layers; i++ )
		m_dlist->SetLayerRGB( i, m_Doc->m_rgb[i][0], m_Doc->m_rgb[i][1], m_Doc->m_rgb[i][2] );
	ShowSelectStatus();
	ShowActiveLayer();
	m_Doc->m_view = this;
	// set up array of mask ids
	if( NUM_SEL_MASKS != 10 )
		ASSERT(0);
	m_mask_id[SEL_MASK_PARTS].Set( ID_PART, ID_SEL_RECT );
	m_mask_id[SEL_MASK_REF].Set( ID_PART, ID_SEL_REF_TXT );
	m_mask_id[SEL_MASK_PINS].Set( ID_PART, ID_SEL_PAD );
	m_mask_id[SEL_MASK_CON].Set( ID_NET, ID_CONNECT, 0, ID_SEL_SEG );
	m_mask_id[SEL_MASK_VIA].Set( ID_NET, ID_CONNECT, 0, ID_SEL_VERTEX );
	m_mask_id[SEL_MASK_AREAS].Set( ID_NET, ID_AREA );
	m_mask_id[SEL_MASK_TEXT].Set( ID_TEXT );
	m_mask_id[SEL_MASK_SM].Set( ID_SM_CUTOUT );
	m_mask_id[SEL_MASK_BOARD].Set( ID_BOARD );
	m_mask_id[SEL_MASK_DRC].Set( ID_DRC );
}

// initialize view with defaults for a new project
// should be called each time a new project is created
//
void CFreePcbView::InitializeView()
{
	if( !m_dlist )
		ASSERT(0);

	// set defaults
	SetCursorMode( CUR_NONE_SELECTED );
	m_sel_id.Clear();
	m_sel_layer = 0;
	m_dir = 0;
	m_debug_flag = 0;
	m_dragging_new_item = 0;
	m_active_layer = LAY_TOP_COPPER;
	m_bDraggingRect = FALSE;
	m_bLButtonDown = FALSE;
	m_sel_mask = 0xffff;
	SetSelMaskArray( m_sel_mask );

	// default screen coords in world units (i.e. display units)
	m_pcbu_per_pixel = 5.0*PCBU_PER_MIL;	// 5 mils per pixel
	m_org_x = -100.0*PCBU_PER_MIL;			// lower left corner of window
	m_org_y = -100.0*PCBU_PER_MIL;

	// grid defaults
	m_Doc->m_snap_angle = 45;
	
	m_left_pane_invalid = TRUE;
	Invalidate( FALSE );
}

// destructor
CFreePcbView::~CFreePcbView()
{
}

BOOL CFreePcbView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView drawing

void CFreePcbView::OnDraw(CDC* pDC)
{

	// get client rectangle
	GetClientRect( &m_client_r );

	// clear screen to black if no project open
	if( !m_Doc )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}
	if( !m_Doc->m_project_open )
	{
		pDC->FillSolidRect( m_client_r, RGB(0,0,0) );
		return;
	}

	// draw stuff on left pane
	CRect r = m_client_r;
	int y_off = 10;
	int x_off = 10;
	if( m_left_pane_invalid )
	{
		// erase previous contents if changed
		CBrush brush( RGB(255, 255, 255) );
		CPen pen( PS_SOLID, 1, RGB(255, 255, 255) );
		CBrush * old_brush = pDC->SelectObject( &brush );
		CPen * old_pen = pDC->SelectObject( &pen );
		// erase left pane
		r.right = m_left_pane_w;
		r.bottom -= m_bottom_pane_h;
		pDC->Rectangle( &r );
		// erase bottom pane
		r = m_client_r;
		r.top = r.bottom - m_bottom_pane_h;
		pDC->Rectangle( &r );
		pDC->SelectObject( old_brush );
		pDC->SelectObject( old_pen );
		m_left_pane_invalid = FALSE;
	}
	CFont * old_font = pDC->SelectObject( &m_small_font );
	int index_to_active_layer;
	for( int i=0; i<m_Doc->m_num_layers; i++ )
	{
		// i = position index
		r.left = x_off;
		r.right = x_off+12;
		r.top = i*VSTEP+y_off;
		r.bottom = i*VSTEP+12+y_off;
		// il = layer index, since copper layers are displayed out of order
		int il = i;
		if( i == m_Doc->m_num_layers-1 && m_Doc->m_num_copper_layers > 1 )
			il = LAY_BOTTOM_COPPER;
		else if( i > LAY_TOP_COPPER )
			il = i+1;
		CBrush brush( RGB(m_Doc->m_rgb[il][0], m_Doc->m_rgb[il][1], m_Doc->m_rgb[il][2]) );
		if( m_Doc->m_vis[il] )
		{
			// if layer is visible, draw colored rectangle
			CBrush * old_brush = pDC->SelectObject( &brush );
			pDC->Rectangle( &r );
			pDC->SelectObject( old_brush );
		}
		else
		{
			// if layer is invisible, draw box with X
			pDC->Rectangle( &r );
			pDC->MoveTo( r.left, r.top );
			pDC->LineTo( r.right, r.bottom );
			pDC->MoveTo( r.left, r.bottom );
			pDC->LineTo( r.right, r.top );
		}
		r.left += 20;
		r.right += 120;
		r.bottom += 5;
		if( il == LAY_TOP_COPPER )
			pDC->DrawText( "top copper", -1, &r, DT_TOP );
		else if( il == LAY_BOTTOM_COPPER )
			pDC->DrawText( "bottom", -1, &r, 0 );
		else if( il == LAY_PAD_THRU )
			pDC->DrawText( "drilled hole", -1, &r, 0 );
		else
			pDC->DrawText( &layer_str[il][0], -1, &r, 0 );
		if( il >= LAY_TOP_COPPER )
		{
			CString num_str;
			num_str.Format( "[%c*]", layer_char[i-LAY_TOP_COPPER] );
			CRect nr = r;
			nr.left = nr.right - 55;
			pDC->DrawText( num_str, -1, &nr, DT_TOP );
		}
		CRect ar = r;
		ar.left = 2;
		ar.right = 8;
		ar.bottom -= 5;
		if( il == m_active_layer )
		{
			// draw arrowhead
			pDC->MoveTo( ar.left, ar.top+1 );
			pDC->LineTo( ar.right-1, (ar.top+ar.bottom)/2 );
			pDC->LineTo( ar.left, ar.bottom-1 );
			pDC->LineTo( ar.left, ar.top+1 );
		}
		else
		{
			// erase arrowhead
			pDC->FillSolidRect( &ar, RGB(255,255,255) ); 
		}
	}
	r.left = x_off;
	r.bottom += VSTEP*2;
	r.top += VSTEP*2;
	pDC->DrawText( "SELECTION MASK", -1, &r, DT_TOP ); 
	y_off = r.bottom;
	for( int i=0; i<NUM_SEL_MASKS; i++ )
	{
		// i = position index
		r.left = x_off;
		r.right = x_off+12;
		r.top = i*VSTEP+y_off;
		r.bottom = i*VSTEP+12+y_off;
		CBrush green_brush( RGB(0, 255, 0) );
		CBrush red_brush( RGB(255, 0, 0) );
		if( m_sel_mask & (1<<i) )
		{
			// if mask is selected is visible, draw green rectangle
			CBrush * old_brush = pDC->SelectObject( &green_brush );
			pDC->Rectangle( &r );
			pDC->SelectObject( old_brush );
		}
		else
		{
			// if mask not selected, draw red
			CBrush * old_brush = pDC->SelectObject( &red_brush );
			pDC->Rectangle( &r );
			pDC->SelectObject( old_brush );
		}
		r.left += 20;
		r.right += 120;
		r.bottom += 5;
		pDC->DrawText( sel_mask_str[i], -1, &r, DT_TOP );
	}
	r.left = x_off;
	r.bottom += VSTEP*2;
	r.top += VSTEP*2;
	pDC->DrawText( "* Use these", -1, &r, DT_TOP );
	r.bottom += VSTEP;
	r.top += VSTEP;
	pDC->DrawText( "keys to change", -1, &r, DT_TOP );
	r.bottom += VSTEP;
	r.top += VSTEP;
	pDC->DrawText( "routing layer", -1, &r, DT_TOP );

	// draw function keys on bottom pane
	DrawBottomPane();

	//** this is for testing only, needs to be converted to PCB coords
#if 0
	if( b_update_rect )
	{
		// clip to update rectangle
		CRgn rgn;
		rgn.CreateRectRgn( m_left_pane_w + update_rect.left, 
			update_rect.bottom, 
			m_left_pane_w + update_rect.right, 
			update_rect.top  );
		pDC->SelectClipRgn( &rgn );
	}
	else
#endif
	{
		// clip to pcb drawing region
		pDC->SelectClipRgn( &m_pcb_rgn );
	}

	// now draw the display list
	SetDCToWorldCoords( pDC );
	m_dlist->Draw( pDC );
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView printing

BOOL CFreePcbView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CFreePcbView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CFreePcbView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView diagnostics

#ifdef _DEBUG
void CFreePcbView::AssertValid() const
{
	CView::AssertValid();
}

void CFreePcbView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CFreePcbDoc* CFreePcbView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CFreePcbDoc)));
	return (CFreePcbDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CFreePcbView message handlers

// Window was resized
//
void CFreePcbView::OnSize(UINT nType, int cx, int cy) 
{

	CView::OnSize(nType, cx, cy);
	
	// update client rect and create clipping region
	GetClientRect( &m_client_r );
	m_pcb_rgn.DeleteObject();
	m_pcb_rgn.CreateRectRgn( m_left_pane_w, m_client_r.bottom-m_bottom_pane_h,
		m_client_r.right, m_client_r.top );

	// update display mapping for display list
	if( m_dlist )
		m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
					m_org_x, m_org_y );
	
	// create memory DC and DDB
	if( !m_memDC_created && m_client_r.right != 0 )
	{
		CDC * pDC = GetDC();
		m_memDC.CreateCompatibleDC( pDC );
		m_memDC_created = TRUE;
//		m_bitmap.CreateCompatibleBitmap( GetDC(), m_client_r.right, m_client_r.bottom );
		m_bitmap.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap = m_memDC.SelectObject( &m_bitmap );
		m_bitmap_rect = m_client_r;
		ReleaseDC( pDC );
	}
	else if( m_memDC_created && (m_bitmap_rect != m_client_r) )
	{
		CDC * pDC = GetDC();
		m_memDC.SelectObject( m_old_bitmap );
		m_bitmap.DeleteObject();
//		m_bitmap.CreateCompatibleBitmap( GetDC(), m_client_r.right, m_client_r.bottom );
		m_bitmap.CreateCompatibleBitmap( pDC, m_client_r.right, m_client_r.bottom );
		m_old_bitmap = m_memDC.SelectObject( &m_bitmap );
		m_bitmap_rect = m_client_r;
		ReleaseDC( pDC );
	}
}

// Left mouse button released, we should probably do something
//
void CFreePcbView::OnLButtonUp(UINT nFlags, CPoint point) 
{
	if( !m_bLButtonDown )
	{
		// this avoids problems with opening a project with the button held down
		CView::OnLButtonUp(nFlags, point);
		return;
	}
	
	CDC * pDC = NULL;
	CPoint tp = WindowToPCB( point );

	m_bLButtonDown = FALSE;
	gLastKeyWasArrow = FALSE;	// cancel series of arrow keys
	if( m_bDraggingRect )
	{
		// we were dragging selection rect, handle it
		m_last_drag_rect.NormalizeRect();
		CPoint tl = WindowToPCB( m_last_drag_rect.TopLeft() );
		CPoint br = WindowToPCB( m_last_drag_rect.BottomRight() );
		m_sel_rect = CRect( tl, br );
		if( nFlags & MK_CONTROL )
		{
			// control key held down
			if( m_cursor_mode == CUR_PART_SELECTED )
			{
				if( m_sel_id.type != ID_PART && m_sel_id.st != ID_SEL_RECT )
					ASSERT(0);
				m_sel_ids.Add( m_sel_id );
				m_sel_ptrs.Add( m_sel_part );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			else if( m_cursor_mode == CUR_TEXT_SELECTED )
			{
				if( m_sel_id.type != ID_TEXT )
					ASSERT(0);
				m_sel_ids.Add( m_sel_id );
				m_sel_ptrs.Add( m_sel_text );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			else if( m_cursor_mode == CUR_SEG_SELECTED )
			{
				if( m_sel_id.type != ID_NET )
					ASSERT(0);
				m_sel_ids.Add( m_sel_id );
				m_sel_ptrs.Add( m_sel_net );
				SetCursorMode( CUR_GROUP_SELECTED );
			}
			if( m_cursor_mode == CUR_GROUP_SELECTED )
			{
				SelectItemsInRect( m_sel_rect, TRUE );
			}
			else
				SelectItemsInRect( m_sel_rect, FALSE );
		}
		else
		{
			SelectItemsInRect( m_sel_rect, FALSE );
		}
		m_bDraggingRect = FALSE;
		Invalidate( FALSE );
		CView::OnLButtonUp(nFlags, point);
		return;
	}

	if( point.y > (m_client_r.bottom-m_bottom_pane_h) )
	{
		// clicked in bottom pane, test for hit on function key rectangle
		for( int i=0; i<8; i++ )
		{
			CRect r( FKEY_OFFSET_X+i*FKEY_STEP+(i/4)*FKEY_GAP, 
				m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H, 
				FKEY_OFFSET_X+i*FKEY_STEP+(i/4)*FKEY_GAP+FKEY_R_W,
				m_client_r.bottom-FKEY_OFFSET_Y );
			if( r.PtInRect( point ) )
			{
				// fake function key pressed
				int nChar = i + 112;
				HandleKeyPress( nChar, 0, 0 );
			}
		}
	}
	else if( point.x < m_left_pane_w )
	{
		// clicked in left pane
		CRect r = m_client_r;
		int y_off = 10;
		int x_off = 10;
		for( int i=0; i<m_Doc->m_num_layers; i++ )
		{
			// i = position index
			// il = layer index, since copper layers are displayed out of order
			int il = i;
			if( i == m_Doc->m_num_layers-1 && m_Doc->m_num_copper_layers > 1 )
				il = LAY_BOTTOM_COPPER;
			else if( i > LAY_TOP_COPPER )
				il = i+1;
			// get color square
			r.left = x_off;
			r.right = x_off+12;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			if( r.PtInRect( point ) && il > LAY_BACKGND )
			{
				// clicked in color square
				m_Doc->m_vis[il] = !m_Doc->m_vis[il]; 
				m_dlist->SetLayerVisible( il, m_Doc->m_vis[il] );
				Invalidate( FALSE );
			}
			else
			{
				// get layer name rect
				r.left += 20;
				r.right += 120;
				r.bottom += 5;
				if( r.PtInRect( point ) )
				{
					// clicked on layer name
					if( i >= LAY_TOP_COPPER )
					{
						int nChar = '1' + i - LAY_TOP_COPPER;
						HandleKeyPress( nChar, 0, 0 );
						Invalidate( FALSE );
					}
				}
			}
		}
		y_off = r.bottom + 2*VSTEP;
		for( int i=0; i<NUM_SEL_MASKS; i++ )
		{
			// get color square
			r.left = x_off;
			r.right = x_off+12+120;
			r.top = i*VSTEP+y_off;
			r.bottom = i*VSTEP+12+y_off;
			if( r.PtInRect( point ) )
			{
				// clicked in color square or name
				m_sel_mask = m_sel_mask ^ (1<<i);
				SetSelMaskArray( m_sel_mask );				
				Invalidate( FALSE );
			}
		}
	}
	else
	{
		// clicked in PCB pane
		if(	CurNone() || CurSelected() )
		{

			// see if new item selected
			CPoint p = WindowToPCB( point );
			id sid;
			void * sel_ptr = NULL;
			if( m_sel_id.type == ID_PART )
				sel_ptr = m_sel_part;
			else if( m_sel_id.type == ID_NET )
				sel_ptr = m_sel_net;
			else if( m_sel_id.type == ID_TEXT )
				sel_ptr = m_sel_text;
			else if( m_sel_id.type == ID_DRC )
				sel_ptr = m_sel_dre;
			void * ptr = m_dlist->TestSelect( p.x, p.y, &sid, &m_sel_layer, &m_sel_id, sel_ptr,
				m_mask_id, NUM_SEL_MASKS );

			// check for second pad selected while holding down 's'
			SHORT kc = GetKeyState( 'S' );
			if( kc & 0x8000 && m_cursor_mode == CUR_PAD_SELECTED )
			{
				if( sid.type == ID_PART && sid.st == ID_SEL_PAD )
				{
					CString mess;
					// OK, now swap pads
					cpart * part1 = m_sel_part;
					CString pin_name1 = part1->shape->GetPinNameByIndex( m_sel_id.i );
					cnet * net1 = m_Doc->m_plist->GetPinNet(part1, &pin_name1);
					CString net_name1 = "unconnected";
					if( net1 )
						net_name1 = net1->name;
					cpart * part2 = (cpart*)ptr;
					CString pin_name2 = part2->shape->GetPinNameByIndex( sid.i );
					cnet * net2 = m_Doc->m_plist->GetPinNet(part2, &pin_name2);
					CString net_name2 = "unconnected";
					if( net2 )
						net_name2 = net2->name;
					if( net1 == NULL && net2 == NULL )
					{
						AfxMessageBox( "No connections to swap" );
						return;
					}
					mess.Format( "Swap %s.%s (\"%s\") and %s.%s (\"%s\") ?", 
						part1->ref_des, pin_name1, net_name1,
						part2->ref_des, pin_name2, net_name2 );
					int ret = AfxMessageBox( mess, MB_OKCANCEL );
					if( ret == IDOK )
					{
						SaveUndoInfoFor2PartsAndNets( part1, part2 );
						m_Doc->m_nlist->SwapPins( part1, &pin_name1, part2, &pin_name2 );
						m_Doc->ProjectModified( TRUE );
						ShowSelectStatus();
						Invalidate( FALSE );
					}
					return;
				}
			}

			// now handle new selection
			if( nFlags & MK_CONTROL )
			{
				// control key held down
				if(    sid.type == ID_PART
					|| sid.type == ID_TEXT
					|| (sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_SEG
						&& ((cnet*)ptr)->connect[sid.i].seg[sid.ii].layer != LAY_RAT_LINE) 
					|| sid.type == ID_NET && sid.st == ID_AREA && sid.sst == ID_SEL_SIDE
					|| sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT && sid.sst == ID_SEL_SIDE )
				{
					// legal selection for group
					if( sid.type == ID_PART )
					{
						sid.st = ID_SEL_RECT;
						sid.i = 0;
						sid.sst = 0;
						sid.ii = 0;
					}
					// if previous single selection, convert to group
					if( m_cursor_mode == CUR_PART_SELECTED )
					{
						if( m_sel_ids.GetSize() )
							ASSERT(0);
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( m_sel_part );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					else if( m_cursor_mode == CUR_SEG_SELECTED )
					{
						if( m_sel_ids.GetSize() )
							ASSERT(0);
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( m_sel_net );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					else if( m_cursor_mode == CUR_AREA_SIDE_SELECTED )
					{
						if( m_sel_ids.GetSize() )
							ASSERT(0);
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( m_sel_net );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					else if( m_cursor_mode == CUR_SMCUTOUT_SIDE_SELECTED )
					{
						if( m_sel_ids.GetSize() )
							ASSERT(0);
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( &m_Doc->m_sm_cutout[m_sel_id.i] );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					else if( m_cursor_mode == CUR_TEXT_SELECTED )
					{
						if( m_sel_ids.GetSize() )
							ASSERT(0);
						m_sel_ids.Add( m_sel_id );
						m_sel_ptrs.Add( m_sel_text );
						SetCursorMode( CUR_GROUP_SELECTED );
						m_sel_id.type = ID_MULTI;
					}
					// now add or remove from group
					if( m_cursor_mode == CUR_GROUP_SELECTED )
					{
						BOOL bFound = FALSE;
						for( int i=0; i<m_sel_ids.GetSize(); i++ )
						{
							id tid = m_sel_ids[i];
							void * tptr = m_sel_ptrs[i];
							if( tid == sid && m_sel_ptrs[i] == ptr )
							{
								bFound = TRUE;
								m_sel_ptrs.RemoveAt(i);
								m_sel_ids.RemoveAt(i);
							}
						}
						if( !bFound )
						{
							m_sel_ids.Add( sid );
							m_sel_ptrs.Add( ptr );
						}
						if( m_sel_ids.GetSize() == 0 )
						{
							CancelSelection();
						}
						else
						{
							HighlightGroup();
						}
						Invalidate( FALSE );
					}
				}
			}
			else if( sid.type == ID_DRC && sid.st == ID_SEL_DRE )
			{
				CancelSelection();
				DRError * dre = (DRError*)ptr;
				m_sel_id = sid;
				m_sel_dre = dre;
				m_Doc->m_drelist->HighLight( m_sel_dre );
				SetCursorMode( CUR_DRE_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_BOARD && sid.st == ID_BOARD_OUTLINE 
				&& sid.sst == ID_SEL_CORNER )
			{
				CancelSelection();
				m_Doc->m_board_outline->HighlightCorner( sid.ii );
				m_sel_id = sid;
				SetCursorMode( CUR_BOARD_CORNER_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_BOARD && sid.st == ID_BOARD_OUTLINE 
				&& sid.sst == ID_SEL_SIDE )
			{
				CancelSelection();
				m_Doc->m_board_outline->HighlightSide( sid.ii );
				m_sel_id = sid;
				SetCursorMode( CUR_BOARD_SIDE_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT 
				&& sid.sst == ID_SEL_CORNER )
			{
				CancelSelection();
				m_Doc->m_sm_cutout[sid.i].HighlightCorner( sid.ii );
				m_sel_id = sid;
				SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT 
				&& sid.sst == ID_SEL_SIDE )
			{
				CancelSelection();
				m_Doc->m_sm_cutout[sid.i].HighlightSide( sid.ii );
				m_sel_id = sid;
				SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
				Invalidate( FALSE );
			}
			else if( sid.type == ID_PART )
			{
				CancelSelection();
				m_sel_part = (cpart*)ptr;
				m_sel_id = sid;
				if( (GetKeyState('N') & 0x8000) && sid.st == ID_SEL_PAD )
				{
					// pad selected and if "n" held down, select net
					cnet * net = m_Doc->m_plist->GetPinNet( m_sel_part, sid.i );
					if( net )
					{
						m_sel_net = net;
						m_sel_id = net->id;
						m_sel_id.st = ID_ENTIRE_NET;
						m_Doc->m_nlist->HighlightNet( m_sel_net );
						m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
						SetCursorMode( CUR_NET_SELECTED );
					}
					else
					{
						m_Doc->m_plist->SelectPad( m_sel_part, sid.i );
						SetCursorMode( CUR_PAD_SELECTED );
						Invalidate( FALSE );
					}
				}
				else if( sid.st == ID_SEL_RECT )
				{
					SelectPart( m_sel_part );
				}
				else if( sid.st == ID_SEL_REF_TXT )
				{
					m_Doc->m_plist->SelectRefText( m_sel_part );
					SetCursorMode( CUR_REF_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_SEL_PAD )
				{
					m_Doc->m_plist->SelectPad( m_sel_part, sid.i );
					SetCursorMode( CUR_PAD_SELECTED );
					Invalidate( FALSE );
				}
			}
			else if( sid.type == ID_NET )
			{
				CancelSelection();
				m_sel_net = (cnet*)ptr;
				m_sel_id = sid;
				if( (GetKeyState('N') & 0x8000) && sid.st == ID_CONNECT )
				{
					// if "n" held down, select entire net
					m_sel_id.st = ID_ENTIRE_NET;
					m_Doc->m_nlist->HighlightNet( m_sel_net );
					m_Doc->m_plist->HighlightAllPadsOnNet( m_sel_net );
					SetCursorMode( CUR_NET_SELECTED );
					Invalidate( FALSE );
				}
				else if( (GetKeyState('T') & 0x8000) && sid.st == ID_CONNECT
					&& sid.sst == ID_SEL_SEG
					&& m_sel_net->connect[sid.i].seg[sid.ii].layer != LAY_RAT_LINE )
				{
					// segment selected with "t" held down, select trace
					m_sel_id.sst = ID_ENTIRE_CONNECT;
					m_Doc->m_nlist->HighlightConnection( m_sel_net, sid.i );
					SetCursorMode( CUR_CONNECT_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_CONNECT && sid.sst == ID_SEL_SEG )
				{
					// select segment
					m_Doc->m_nlist->HighlightSegment( m_sel_net, sid.i, sid.ii );
					if( m_sel_net->connect[sid.i].seg[sid.ii].layer != LAY_RAT_LINE )
						SetCursorMode( CUR_SEG_SELECTED );
					else
						SetCursorMode( CUR_RAT_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_CONNECT && sid.sst == ID_SEL_VERTEX )
				{
					// select vertex
					cconnect * c = &m_sel_net->connect[sid.i];
					if( c->end_pin == cconnect::NO_END && sid.ii == c->nsegs )
						SetCursorMode( CUR_END_VTX_SELECTED );
					else
						SetCursorMode( CUR_VTX_SELECTED );
					m_Doc->m_nlist->SelectVertex( m_sel_net, sid.i, sid.ii );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_AREA && sid.sst == ID_SEL_SIDE )
				{
					// select copper area side
					m_Doc->m_nlist->SelectAreaSide( m_sel_net, sid.i, sid.ii );
					SetCursorMode( CUR_AREA_SIDE_SELECTED );
					Invalidate( FALSE );
				}
				else if( sid.st == ID_AREA && sid.sst == ID_SEL_CORNER )
				{
					// select copper area corner
					m_Doc->m_nlist->SelectAreaCorner( m_sel_net, sid.i, sid.ii );
					SetCursorMode( CUR_AREA_CORNER_SELECTED );
					Invalidate( FALSE );
				}
				else
					ASSERT(0);
			}
			else if( sid.type == ID_TEXT )
			{
				CancelSelection();
				m_sel_text = (CText*)ptr;
				m_sel_id = sid;
				m_Doc->m_tlist->HighlightText( m_sel_text );
				SetCursorMode( CUR_TEXT_SELECTED );
				Invalidate( FALSE );
			}
			else
			{
				// nothing selected
				CancelSelection();
				m_sel_id.Clear();
				Invalidate( FALSE );
			}
		}
		else if( m_cursor_mode == CUR_DRAG_PART )
		{
			// complete move
			SetCursorMode( CUR_PART_SELECTED );
			CPoint p = WindowToPCB( point );
			m_Doc->m_plist->StopDragging();
			int old_angle = m_Doc->m_plist->GetAngle( m_sel_part );
			int angle = old_angle + m_dlist->GetDragAngle();
			if( angle>270 )
				angle = angle - 360;
			int old_side = m_sel_part->side;
			int side = old_side + m_dlist->GetDragSide();
			if( side > 1 )
				side = side - 2;

			// save undo info for part and attached nets
			if( !m_dragging_new_item )
				SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY );
			m_dragging_new_item = FALSE;

			// now move it
			m_sel_part->glued = 0;
			m_Doc->m_plist->Move( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y, 
				angle, side );
			m_Doc->m_plist->HighlightPart( m_sel_part );
			m_Doc->m_nlist->PartMoved( m_sel_part );
			m_Doc->m_nlist->OptimizeConnections( m_sel_part );
			SetFKText( m_cursor_mode );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
#if 0
			//** for testing only
			CRect test_rect( 0, 0, 100, 100 );
			InvalidateRect( test_rect, FALSE );
			OnDraw( GetDC() );
#endif
		}		
		else if( m_cursor_mode == CUR_DRAG_GROUP )
		{
			// complete move
			m_Doc->m_dlist->StopDragging();
			SaveUndoInfoForGroup( 0 );
			MoveGroup( m_last_cursor_point.x - m_from_pt.x, m_last_cursor_point.y - m_from_pt.y );
			m_dlist->SetLayerVisible( LAY_HILITE, TRUE );
			HighlightGroup();
			SetCursorMode( CUR_GROUP_SELECTED );
			m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_MOVE_ORIGIN )
		{
			// complete move
			SetCursorMode( CUR_NONE_SELECTED );
			CPoint p = WindowToPCB( point );
			m_Doc->m_dlist->StopDragging();
			SaveUndoInfoForMoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y );
			MoveOrigin( -m_last_cursor_point.x, -m_last_cursor_point.y );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_REF )
		{
			// complete move
			SetCursorMode( CUR_REF_SELECTED );
			CPoint p = WindowToPCB( point );
			m_Doc->m_plist->StopDragging();
			int drag_angle = m_dlist->GetDragAngle();
			// if part on bottom of board, drag angle is CCW instead of CW
			if( m_Doc->m_plist->GetSide( m_sel_part ) && drag_angle )
				drag_angle = 360 - drag_angle;
			int angle = m_Doc->m_plist->GetRefAngle( m_sel_part ) + drag_angle;
			if( angle>270 )
				angle = angle - 360;
			// save undo info
			SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY );
			// now move it
			m_Doc->m_plist->MoveRefText( m_sel_part, m_last_cursor_point.x, m_last_cursor_point.y, 
				angle, m_sel_part->m_ref_size, m_sel_part->m_ref_w );
			m_Doc->m_plist->SelectRefText( m_sel_part );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_RAT )
		{
			// add trace segment and vertex
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			m_dlist->StopDragging();

			// make undo record
			SaveUndoInfoForConnection( m_sel_net, m_sel_ic );

			int insert_flag;
			int w = m_Doc->m_trace_w;
			int via_w = m_Doc->m_via_w;
			int via_hole_w = m_Doc->m_via_hole_w;
			GetWidthsForSegment( &w, &via_w, &via_hole_w ); 
			insert_flag = m_Doc->m_nlist->InsertSegment( m_sel_net, m_sel_ic, m_sel_is, 
				m_last_cursor_point.x, m_last_cursor_point.y, 
				m_active_layer, w, via_w, via_hole_w, m_dir );
			if( insert_flag )
			{
				// test for hit destination pad
				BOOL btest = m_Doc->m_nlist->TestHitOnConnectionEndPad( m_last_cursor_point.x, m_last_cursor_point.y,
					m_sel_net, m_sel_ic, m_active_layer, m_dir );
				if( m_dir == 0 )
				{
					// select next segment
					m_sel_id.ii++;
				}
				if( btest )
				{
					// finish trace to pad
					m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is,
						m_active_layer, w, via_w, via_hole_w );
					SetCursorMode( CUR_NONE_SELECTED );
					m_sel_id.Clear();
				}
				else
				{
					// no hit on destination pad, test for hit on any pad in net
					int ip = m_Doc->m_nlist->TestHitOnAnyPadInNet( m_last_cursor_point.x, 
						m_last_cursor_point.y,
						m_active_layer, m_sel_net );
					if( ip != -1 
						&& ip != m_sel_con.start_pin
						&& ip != m_sel_con.end_pin )
					{
						// OK, connect it and complete it
						cpart * hit_part = m_sel_net->pin[ip].part;
						CString * hit_pin_name = &m_sel_net->pin[ip].pin_name;
						m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, 1-m_dir, hit_part, hit_pin_name );
						// finish trace to pad
						m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is,
							m_active_layer, w, via_w, via_hole_w );
						//						SetCursorMode( CUR_SEG_SELECTED );
						//						m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
						SetCursorMode( CUR_NONE_SELECTED );
						m_sel_id.Clear();
					}
					else
					{
						// continue routing
						m_Doc->m_nlist->StartDraggingSegment( pDC, m_sel_net, 
							m_sel_id.i, m_sel_id.ii,
							m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer, 
							LAY_SELECTION, w,
							m_active_layer, via_w, via_hole_w, m_dir, 2 );
					}
				}
				m_snap_angle_ref = m_last_cursor_point;
			}
			else
			{
				// trace completed
				SetCursorMode( CUR_NONE_SELECTED );
				m_sel_id.Clear();
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_BOARD )
		{
			// place first corner of board outline
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			// make new board outline 
			if( m_Doc->m_board_outline )
				delete m_Doc->m_board_outline;
			m_Doc->m_board_outline = new CPolyLine( m_dlist );
			m_Doc->m_board_outline->Start( LAY_BOARD_OUTLINE, 1, 20*NM_PER_MIL, p.x, p.y, 
				0, &m_sel_id, NULL );
			m_sel_id.sst = ID_SEL_CORNER;
			m_sel_id.ii = 0;
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_BOARD_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_BOARD_1 )
		{
			// place second corner of board outline
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_Doc->m_board_outline->AppendCorner( p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii++;
			SetCursorMode( CUR_DRAG_BOARD );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_BOARD )
		{
			// place subsequent corners of board outline
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			if( p.x == m_Doc->m_board_outline->GetX(0)
				&& p.y == m_Doc->m_board_outline->GetY(0) )
			{
				// this point is the start point, close the polyline and quit
				m_Doc->m_board_outline->Close( m_polyline_style );
				SetCursorMode( CUR_NONE_SELECTED );
				m_Doc->m_dlist->StopDragging();
				SaveUndoInfoForBoardOutline( UNDO_BOARD_ADD );
			}
			else
			{
				// add corner to polyline
				m_Doc->m_board_outline->AppendCorner( p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii++;
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_BOARD_INSERT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			SaveUndoInfoForBoardOutline( UNDO_BOARD_MODIFY );
			m_Doc->m_board_outline->InsertCorner( m_sel_id.ii+1, p.x, p.y );
			m_Doc->m_board_outline->HighlightCorner( m_sel_id.ii+1 );
			SetCursorMode( CUR_BOARD_CORNER_SELECTED );
			m_sel_id.Set( ID_BOARD, ID_BOARD_OUTLINE, 0, ID_SEL_CORNER, m_sel_id.ii+1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_BOARD_MOVE )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			SaveUndoInfoForBoardOutline( UNDO_BOARD_MODIFY );
			m_Doc->m_board_outline->MoveCorner( m_sel_id.ii, p.x, p.y );
			m_Doc->m_board_outline->HighlightCorner( m_sel_id.ii );
			SetCursorMode( CUR_BOARD_CORNER_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_AREA )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int iarea = m_Doc->m_nlist->AddArea( m_sel_net, m_active_layer, p.x, p.y, m_polyline_hatch );
			m_sel_id.Set( m_sel_net->id.type, ID_AREA, iarea, ID_SEL_CORNER, 1 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_AREA_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_1 )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 2;
			SetCursorMode( CUR_DRAG_AREA );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_AREA )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			if( p.x == m_sel_net->area[m_sel_id.i].poly->GetX(0)
				&& p.y == m_sel_net->area[m_sel_id.i].poly->GetY(0) )
			{
				// cursor point is first point, close area
				SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_ADD );
				m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
				SetCursorMode( CUR_NONE_SELECTED );
				m_Doc->m_dlist->StopDragging();
				m_Doc->m_nlist->OptimizeConnections( m_sel_net );
			}
			else
			{
				// add cursor point
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii = m_sel_id.ii + 1;
				SetCursorMode( CUR_DRAG_AREA );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net );  
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is, p.x, p.y );
			int n_poly = m_Doc->m_nlist->AreaModified( m_sel_net, m_sel_ia );
			if( n_poly == -1 )
			{
				// error
				AfxMessageBox( "Error: Arc cannot intersect any other side of copper area" );
				CancelSelection();
				m_Doc->OnEditUndo();
			}
			else
			{
				m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
				m_Doc->m_nlist->OptimizeConnections( m_sel_net );
				if( n_poly == 0 )
				{
					m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
					SetCursorMode( CUR_AREA_CORNER_SELECTED );
				}
				else
					CancelSelection();
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
		{
			SaveUndoInfoForAllAreasInNet( m_sel_net );  
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			m_Doc->m_nlist->InsertAreaCorner( m_sel_net, m_sel_ia, m_sel_is+1, p.x, p.y, CPolyLine::STRAIGHT );
			int n_poly = m_Doc->m_nlist->AreaModified( m_sel_net, m_sel_ia );
			if( n_poly == -1 )
			{
				// error
				AfxMessageBox( "Error: Arc cannot intersect any other side of copper area" );
				CancelSelection();
				m_Doc->OnEditUndo();
			}
			else
			{
				m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
				m_Doc->m_nlist->OptimizeConnections( m_sel_net );
				if( n_poly == 0 )
				{
					m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
					SetCursorMode( CUR_AREA_CORNER_SELECTED );
				}
				else
					CancelSelection();
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int ia = m_sel_id.i;
			carea * a = &m_sel_net->area[ia];
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
			m_sel_id.Set( m_sel_net->id.type, ID_AREA, ia, ID_SEL_CORNER, a->poly->GetNumCorners()-1 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_AREA_CUTOUT_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1 )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 2;
			SetCursorMode( CUR_DRAG_AREA_CUTOUT );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			CPolyLine * poly = m_sel_net->area[m_sel_id.i].poly;
			int icontour = poly->GetContour( poly->GetNumCorners()-1 );
			int istart = poly->GetContourStart( icontour );
			if( p.x == poly->GetX(istart)
				&& p.y == poly->GetY(istart) )
			{
				// cursor point is first point, close area
				SaveUndoInfoForAllAreasInNet( m_sel_net );  
				m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
				m_Doc->m_dlist->StopDragging();
				m_Doc->m_nlist->OptimizeConnections( m_sel_net );
				int n_old_areas = m_sel_net->area.GetSize();
				int n_areas = m_Doc->m_nlist->AreaModified( m_sel_net, m_sel_ia );
				CancelSelection();
				if( n_areas == -1 )
				{
					// error
					AfxMessageBox( "Error: Arc cannot intersect any other side of copper area" );
					m_Doc->OnEditUndo();
				}
			}
			else
			{
				// add cursor point
				m_Doc->m_nlist->AppendAreaCorner( m_sel_net, m_sel_ia, p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii = m_sel_id.ii + 1;
				SetCursorMode( CUR_DRAG_AREA_CUTOUT );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_SMCUTOUT )
		{
			// add poly for new cutout
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int ism = m_Doc->m_sm_cutout.GetSize();
			m_Doc->m_sm_cutout.SetSize( ism + 1 );
			CPolyLine * p_sm = &m_Doc->m_sm_cutout[ism];
			p_sm->SetDisplayList( m_Doc->m_dlist );
			id id_sm( ID_SM_CUTOUT, ID_SM_CUTOUT, ism );
			m_sel_id = id_sm;
			p_sm->Start( m_polyline_layer, 0, 10*NM_PER_MIL, p.x, p.y, m_polyline_hatch, &m_sel_id, NULL ); 
			m_sel_id.sst = ID_SEL_CORNER;
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 1;
			SetCursorMode( CUR_DRAG_SMCUTOUT_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_1 )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			CPolyLine * p_sm = &m_Doc->m_sm_cutout[m_sel_id.i];
			p_sm->AppendCorner( p.x, p.y, m_polyline_style );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			m_sel_id.ii = 2;
			SetCursorMode( CUR_DRAG_SMCUTOUT );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			CPolyLine * p_sm = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( p.x == p_sm->GetX(0)
				&& p.y == p_sm->GetY(0) )
			{
				// cursor point is first point, close area
				if( m_Doc->m_sm_cutout.GetSize() == 1 )
					SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT_NONE );
				else
					SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT );
				p_sm->Close( m_polyline_style );
				SetCursorMode( CUR_NONE_SELECTED );
				m_Doc->m_dlist->StopDragging();
			}
			else
			{
				// add cursor point
				p_sm->AppendCorner( p.x, p.y, m_polyline_style );
				m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
				m_sel_id.ii = m_sel_id.ii + 1;
				SetCursorMode( CUR_DRAG_SMCUTOUT );
				m_snap_angle_ref = m_last_cursor_point;
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE )
		{
			SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT );
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			poly->MoveCorner( m_sel_id.ii, p.x, p.y );
			poly->HighlightCorner( m_sel_id.ii );
			SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT )
		{
			SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT );
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			poly->InsertCorner( m_sel_id.ii+1, p.x, p.y );
			poly->HighlightCorner( m_sel_id.ii+1 );
			m_sel_id.Set( ID_SM_CUTOUT, ID_SM_CUTOUT, m_sel_id.i, ID_SEL_CORNER, m_sel_id.ii+1 );
			SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
		{
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			CPoint p;
			p = m_last_cursor_point;
			int ia = m_sel_id.i;
			carea * a = &m_sel_net->area[ia];
			m_Doc->m_nlist->AppendAreaCorner( m_sel_net, ia, p.x, p.y, m_polyline_style );
			m_sel_id.Set( m_sel_net->id.type, ID_AREA, ia, ID_SEL_CORNER, a->poly->GetNumCorners()-1 );
			m_dlist->StartDraggingArc( pDC, m_polyline_style, p.x, p.y, p.x, p.y, LAY_SELECTION, 1, 2 );
			SetCursorMode( CUR_DRAG_AREA_1 );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
			m_snap_angle_ref = m_last_cursor_point;
		}
		else if( m_cursor_mode == CUR_DRAG_VTX )
		{
			// move vertex by modifying adjacent segments and reconciling via
			SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
			CPoint p;
			p = m_last_cursor_point;
			int ic = m_sel_id.i;
			int ivtx = m_sel_id.ii;
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is, p.x, p.y );
			m_Doc->m_nlist->SelectVertex( m_sel_net, ic, ivtx );
			SetCursorMode( CUR_VTX_SELECTED );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_END_VTX )
		{
			// move end-vertex of stub trace
			m_Doc->m_dlist->StopDragging();
			SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
			CPoint p;
			p = m_last_cursor_point;
			int ic = m_sel_id.i;
			int ivtx = m_sel_id.ii;
			m_Doc->m_nlist->MoveEndVertex( m_sel_net, ic, ivtx, p.x, p.y );
			m_Doc->m_nlist->SelectVertex( m_sel_net, ic, ivtx );
			SetCursorMode( CUR_END_VTX_SELECTED );
			m_Doc->ProjectModified( TRUE );
			m_Doc->m_nlist->OptimizeConnections( m_sel_net );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_CONNECT )
		{
			// see if pad selected
			CPoint p = WindowToPCB( point );
			id sel_id;	// id of selected item
			id pad_id( ID_PART, ID_SEL_PAD, 0, 0, 0 );	// force selection of pad
			void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
			if( ptr )
			{
				if( sel_id.type == ID_PART )
				{
					if( sel_id.st == ID_SEL_PAD )
					{
						// make new connection
						cpart * new_sel_part = (cpart*)ptr;
						cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.i].net;
						cnet * from_sel_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
						if( new_sel_net && from_sel_net && (new_sel_net != from_sel_net) )
						{
							// pins assigned to different nets, can't connect them
							CString mess;
							mess.Format( "You are trying to connect pins on different nets\nYou must detach one of them first" );
							AfxMessageBox( mess );
							m_Doc->m_dlist->StopDragging();
							SetCursorMode( CUR_PAD_SELECTED );
						}
						else
						{
							// we can connect these pins
							SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, TRUE );
							SaveUndoInfoForPart( new_sel_part, CPartList::UNDO_PART_MODIFY, FALSE );
							if( new_sel_net != from_sel_net )
							{
								// one pin is unassigned, assign it to net
								if( !new_sel_net )
								{
									// connecting to unassigned pin, assign it
									SaveUndoInfoForNetAndConnections( from_sel_net, CNetList::UNDO_NET_MODIFY, FALSE );
									CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
									m_Doc->m_nlist->AddNetPin( from_sel_net,
										&new_sel_part->ref_des,
										&pin_name );
									new_sel_net = from_sel_net;
								}
								else if( !from_sel_net )
								{
									// connecting from unassigned pin, assign it
									SaveUndoInfoForNetAndConnections( new_sel_net, CNetList::UNDO_NET_MODIFY, FALSE );
									CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&m_sel_part->ref_des,
										&pin_name );
									from_sel_net = new_sel_net;
								}
								else
									ASSERT(0);
							}
							else if( !new_sel_net && !m_sel_part->pin[m_sel_id.i].net )
							{
								// connecting 2 unassigned pins, select net
								DlgAssignNet assign_net_dlg;
								assign_net_dlg.m_map = &m_Doc->m_nlist->m_map;
								int ret = assign_net_dlg.DoModal();
								if( ret != IDOK )
								{
									m_Doc->m_dlist->StopDragging();
									SetCursorMode( CUR_PAD_SELECTED );
									goto goodbye;
								}
								CString name = assign_net_dlg.m_net_str;
								void * ptr;
								int test = m_Doc->m_nlist->m_map.Lookup( name, ptr );
								if( test )
								{
									// assign pins to existing net
									new_sel_net = (cnet*)ptr;
									SaveUndoInfoForNetAndConnections( new_sel_net, CNetList::UNDO_NET_MODIFY, FALSE );
									CString pin_name1 = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
									CString pin_name2 = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&m_sel_part->ref_des,
										&pin_name1 );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&new_sel_part->ref_des,
										&pin_name2 );
								}
								else
								{
									// make new net
									new_sel_net = m_Doc->m_nlist->AddNet( (char*)(LPCTSTR)name, 10, 0, 0, 0 );
									SaveUndoInfoForNetAndConnections( new_sel_net, CNetList::UNDO_NET_ADD, FALSE );
									CString pin_name1 = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
									CString pin_name2 = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&m_sel_part->ref_des,
										&pin_name1 );
									m_Doc->m_nlist->AddNetPin( new_sel_net,
										&new_sel_part->ref_des,
										&pin_name2 );
								}
							}
							// find pins in net and connect them
							int p1 = -1;
							int p2 = -1;
							for( int ip=0; ip<new_sel_net->npins; ip++ )
							{
								CString pin_name = new_sel_net->pin[ip].pin_name;
								if( new_sel_net->pin[ip].part == m_sel_part )
								{
									int pin_index = m_sel_part->shape->GetPinIndexByName( &pin_name );
									if( pin_index == m_sel_id.i )
									{
										// found starting pin in net
										p1 = ip;
									}
								}
								if( new_sel_net->pin[ip].part == new_sel_part )
								{
									int pin_index = new_sel_part->shape->GetPinIndexByName( &pin_name );
									if( pin_index == sel_id.i )
									{
										// found ending pin in net
										p2 = ip;
									}
								}
							}
							if( p1>=0 && p2>=0 )
								m_Doc->m_nlist->AddNetConnect( new_sel_net, p1, p2 );
							else
								ASSERT(0);	// couldn't find pins in net
							m_dlist->StopDragging();
							SetCursorMode( CUR_PAD_SELECTED );
						}
						m_Doc->ProjectModified( TRUE );
						Invalidate( FALSE );
					}
				}
			}
		}
		else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
		{
			// see if pad selected
			CPoint p = WindowToPCB( point );
			id sel_id;	// id of selected item
			id pad_id( ID_PART, ID_SEL_PAD, 0, 0, 0 );	// force selection of pad
			void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
			if( ptr )
			{
				if( sel_id.type == ID_PART )
				{
					if( sel_id.st == ID_SEL_PAD )
					{
						// see if we can connect to this pin
						cpart * new_sel_part = (cpart*)ptr;
						cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.i].net;
						CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );

						if( new_sel_net && (new_sel_net != m_sel_net) )
						{
							// pin assigned to different net, can't connect it
							CString mess;
							mess.Format( "You are trying to connect to a pin on a different net" );
							AfxMessageBox( mess );
							return;
						}
						else if( new_sel_net == 0 )
						{
							// unassigned pin, assign it
							SaveUndoInfoForPart( new_sel_part, CPartList::UNDO_PART_MODIFY, TRUE );
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, FALSE );
							m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des, &pin_name );
						}
						else
						{
							// pin already assigned to this net
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE );
						}
						m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, m_sel_is, 
							new_sel_part, &pin_name );
						m_dlist->Set_visible( m_sel_seg.dl_el, TRUE );
						m_dlist->StopDragging();
						m_dlist->CancelHighLight();
						SetCursorMode( CUR_RAT_SELECTED );
						m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
						m_Doc->m_nlist->SetAreaConnections( m_sel_net );
						m_Doc->ProjectModified( TRUE );
						Invalidate( FALSE );
					}
				}
			}
		}
		else if( m_cursor_mode == CUR_DRAG_STUB )
		{
			// see if cursor on pad
			CPoint p = WindowToPCB( point );
			id sel_id;	// id of selected item
			id pad_id( ID_PART, ID_SEL_PAD, 0, 0, 0 );	// test for hit on pad
			void * ptr = m_dlist->TestSelect( p.x, p.y, &sel_id, &m_sel_layer, NULL, NULL, &pad_id );
			if( ptr && sel_id.type == ID_PART && sel_id.st == ID_SEL_PAD )
			{
				// see if we can connect to this pin
				cpart * new_sel_part = (cpart*)ptr;
				cnet * new_sel_net = (cnet*)new_sel_part->pin[sel_id.i].net;
				CString pin_name = new_sel_part->shape->GetPinNameByIndex( sel_id.i );
				if( m_Doc->m_plist->TestHitOnPad( new_sel_part, &pin_name, p.x, p.y, m_active_layer ) )
				{
					// check for starting pad of stub trace
					cpart * origin_part = m_sel_start_pin.part;
					CString * origin_pin_name = &m_sel_start_pin.pin_name;
					if( origin_part != new_sel_part || *origin_pin_name != *pin_name )
					{
						// not starting pad
						if( new_sel_net && (new_sel_net != m_sel_net) )
						{
							// pin assigned to different net, can't connect it
							CString mess;
							mess.Format( "You are trying to connect to a pin on a different net" );
							AfxMessageBox( mess );
							return;
						}
						else if( new_sel_net == 0 )
						{
							// unassigned pin, assign it
							SaveUndoInfoForPart( new_sel_part, CPartList::UNDO_PART_MODIFY, TRUE );
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, FALSE );
							m_Doc->m_nlist->AddNetPin( m_sel_net, &new_sel_part->ref_des, &pin_name );
						}
						else
						{
							// pin already assigned to this net
							SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE );
						}
						CPoint pin_point = m_Doc->m_plist->GetPinPoint( new_sel_part, &pin_name );
						int w = m_Doc->m_trace_w;
						int via_w = m_Doc->m_via_w;
						int via_hole_w = m_Doc->m_via_hole_w;
						GetWidthsForSegment( &w, &via_w, &via_hole_w ); 
						m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic, 
							m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer, w, via_w, via_hole_w ); 
						if( m_last_cursor_point != pin_point )
						{
							m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic, 
								pin_point.x, pin_point.y, m_active_layer, w, via_w, via_hole_w ); 
						}
						m_Doc->m_nlist->ChangeConnectionPin( m_sel_net, m_sel_ic, TRUE, 
							new_sel_part, &pin_name );
						m_dlist->StopDragging();
						SetCursorMode( CUR_NONE_SELECTED );
						m_Doc->m_nlist->SetAreaConnections( m_sel_net );
						m_Doc->ProjectModified( TRUE );
						Invalidate( FALSE );
						return;
					}
				}
			}
			// come here if not connecting to pin
			SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
			pDC = GetDC();
			SetDCToWorldCoords( pDC );
			pDC->SelectClipRgn( &m_pcb_rgn );
			int w = m_Doc->m_trace_w;
			int via_w = m_Doc->m_via_w;
			int via_hole_w = m_Doc->m_via_hole_w;
			GetWidthsForSegment( &w, &via_w, &via_hole_w ); 
			m_sel_id.ii = m_Doc->m_nlist->AppendSegment( m_sel_net, m_sel_ic, 
				m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer, w, via_w, via_hole_w ); 
			m_dlist->StopDragging();
			m_sel_id.ii++;
			m_Doc->m_nlist->StartDraggingStub( pDC, m_sel_net, m_sel_ic, m_sel_is,
				m_last_cursor_point.x, m_last_cursor_point.y, m_active_layer, w, m_active_layer, 
				via_w, via_hole_w, 2 );
			m_snap_angle_ref = m_last_cursor_point;
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( m_cursor_mode == CUR_DRAG_TEXT )
		{
			CPoint p;
			p = m_last_cursor_point;
			m_dlist->StopDragging();
			if( !m_dragging_new_item )
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY );
			int old_angle = m_sel_text->m_angle;
			int angle = old_angle + m_dlist->GetDragAngle();
			if( angle>270 )
				angle = angle - 360;
			int old_mirror = m_sel_text->m_mirror;
			int mirror = (old_mirror + m_dlist->GetDragSide())%2;
			int layer = m_sel_text->m_layer;
			m_sel_text = m_Doc->m_tlist->MoveText( m_sel_text, m_last_cursor_point.x, m_last_cursor_point.y, 
				angle, mirror, layer );
			if( m_dragging_new_item )
			{
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD );
				m_dragging_new_item = FALSE;
			}
			SetCursorMode( CUR_TEXT_SELECTED );
			m_Doc->m_tlist->HighlightText( m_sel_text );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
goodbye:
		ShowSelectStatus();
	}
	if( pDC )
		ReleaseDC( pDC );
	CView::OnLButtonUp(nFlags, point);
}

// left double-click
//
void CFreePcbView::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
#if 0
	if( m_cursor_mode == CUR_PART_SELECTED )
	{
		SetCursorMode( CUR_DRAG_PART );
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CPoint p = m_last_mouse_point;
		m_dlist->StartDraggingSelection( pDC, p.x, p.y );
	}
	if( m_cursor_mode == CUR_REF_SELECTED )
	{
		SetCursorMode( CUR_DRAG_REF );
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		CPoint p = m_last_mouse_point;
		m_dlist->StartDraggingSelection( pDC, p.x, p.y );
	}
#endif
	CView::OnLButtonDblClk(nFlags, point);
}

// right mouse button
//
void CFreePcbView::OnRButtonDown(UINT nFlags, CPoint point) 
{
	m_disable_context_menu = 1;
	if( m_cursor_mode == CUR_DRAG_PART )	
	{
		m_Doc->m_plist->CancelDraggingPart( m_sel_part );
		if( m_dragging_new_item )
		{
			CancelSelection();
			m_Doc->OnEditUndo();	// remove the part
//**			m_Doc->m_plist->Remove( m_sel_part );
		}
		else
		{
			SetCursorMode( CUR_PART_SELECTED );
			m_Doc->m_plist->HighlightPart( m_sel_part );
		}
		m_dragging_new_item = FALSE;
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_REF )
	{
		m_Doc->m_plist->CancelDraggingRefText( m_sel_part );
		SetCursorMode( CUR_REF_SELECTED );
		m_Doc->m_plist->SelectRefText( m_sel_part );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT )
	{
		m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_RAT_SELECTED );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_RAT_PIN )
	{
		m_dlist->StopDragging();
		m_dlist->Set_visible( m_sel_seg.dl_el, TRUE );
		m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_RAT_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_VTX )
	{
		m_Doc->m_nlist->CancelDraggingVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_VTX_SELECTED );
		m_Doc->m_nlist->SelectVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_END_VTX )
	{
		m_Doc->m_nlist->CancelDraggingEndVertex( m_sel_net, m_sel_ic, m_sel_is );
		SetCursorMode( CUR_END_VTX_SELECTED );
		m_Doc->m_nlist->SelectVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}	
	else if( m_cursor_mode == CUR_DRAG_CONNECT )
	{
		m_Doc->m_dlist->StopDragging();
		SetCursorMode( CUR_PAD_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_TEXT )
	{
		m_Doc->m_tlist->CancelDraggingText( m_sel_text );
		if( m_dragging_new_item )
		{
			m_Doc->m_tlist->RemoveText( m_sel_text );
			CancelSelection();
			m_dragging_new_item = 0;
		}
		else
		{
			SetCursorMode( CUR_TEXT_SELECTED );
		}
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_SMCUTOUT
		  || m_cursor_mode == CUR_DRAG_SMCUTOUT_1 
		  || (m_cursor_mode == CUR_DRAG_SMCUTOUT && m_sel_id.ii<3) )
	{
		// dragging first, second or third corner of solder mask cutout
		// delete it and cancel dragging
		m_dlist->StopDragging();
		m_Doc->m_sm_cutout.RemoveAt( m_sel_id.i );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT )
	{
		// dragging fourth or higher corner of solder mask cutout, close it
		m_dlist->StopDragging();
		if( m_Doc->m_sm_cutout.GetSize() == 1 )
			SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT_NONE );
		else
			SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT );
		m_Doc->m_sm_cutout[m_sel_id.i].Close( m_polyline_style );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
		poly->MakeVisible();
		poly->HighlightSide( m_sel_id.ii );
		SetCursorMode( CUR_SMCUTOUT_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
		poly->MakeVisible();
		SetCursorMode( CUR_SMCUTOUT_CORNER_SELECTED );
		poly->HighlightCorner( m_sel_id.ii );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_BOARD
		  || m_cursor_mode == CUR_DRAG_BOARD_1 
		  || (m_cursor_mode == CUR_DRAG_BOARD && m_Doc->m_board_outline->GetNumCorners()<3) )
	{
		// dragging first, second or third corner of board outline
		// just delete it (if necessary) and cancel
		m_dlist->StopDragging();
		if( m_Doc->m_board_outline )
			OnBoardDeleteOutline();
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD )
	{
		// dragging fourth or higher corner of board outline, close it
		m_dlist->StopDragging();
		m_Doc->m_board_outline->Close( m_polyline_style );
		SaveUndoInfoForBoardOutline( UNDO_BOARD_ADD );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_INSERT )
	{
		m_dlist->StopDragging();
		m_Doc->m_board_outline->MakeVisible();
		m_Doc->m_board_outline->HighlightSide( m_sel_id.i );
		SetCursorMode( CUR_BOARD_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_BOARD_MOVE )
	{
		// get indexes for preceding and following corners
		m_dlist->StopDragging();
		m_Doc->m_board_outline->MakeVisible();
		SetCursorMode( CUR_BOARD_CORNER_SELECTED );
		m_Doc->m_board_outline->HighlightCorner( m_sel_id.i );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_AREA )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_1 
		  || (m_cursor_mode == CUR_DRAG_AREA && m_sel_id.ii<3) )
	{
		m_dlist->StopDragging();
		m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );	
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA)
	{
		m_dlist->StopDragging();
		SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_ADD );
		m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );	
		CancelSelection();
		m_Doc->m_nlist->OptimizeConnections( m_sel_net );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_ADD_AREA_CUTOUT )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1 
		  || (m_cursor_mode == CUR_DRAG_AREA_CUTOUT && m_sel_id.ii<3) )
	{
		m_dlist->StopDragging();
		CPolyLine * poly = m_sel_net->area[m_sel_id.i].poly;
		int ncont = poly->GetNumContours();
		poly->RemoveContour(ncont-1);
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_CUTOUT )
	{
		m_dlist->StopDragging();
		SetCursorMode( CUR_NONE_SELECTED );
		SaveUndoInfoForAllAreasInNet( m_sel_net );  
		m_Doc->m_nlist->CompleteArea( m_sel_net, m_sel_ia, m_polyline_style );
		int n_areas = m_Doc->m_nlist->AreaModified( m_sel_net, m_sel_ia );
		if( n_areas == -1 )
		{
			// error
			AfxMessageBox( "Error: Arc cannot intersect any other side of copper area" );
			m_Doc->OnEditUndo();
		}
		m_Doc->m_nlist->OptimizeConnections( m_sel_net );
		CancelSelection();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_INSERT )
	{
		m_Doc->m_nlist->CancelDraggingInsertedAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_Doc->m_nlist->SelectAreaSide( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_SIDE_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_AREA_MOVE )
	{
		m_Doc->m_nlist->CancelDraggingAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		m_Doc->m_nlist->SelectAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_STUB )
	{
		if( m_sel_id.ii > 0 )
		{
			m_Doc->m_nlist->CancelDraggingStub( m_sel_net, m_sel_ic, m_sel_is );
			// default is to add a via and optimize
			OnEndVertexAddVia();
			CancelSelection();
			m_Doc->m_nlist->OptimizeConnections( m_sel_net );
		}
		else
		{
			m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic );
			CancelSelection();
		}
		m_dlist->StopDragging();
		Invalidate( FALSE );
	}
	else if( m_cursor_mode == CUR_DRAG_GROUP )
	{
		CancelDraggingGroup();
		m_dlist->SetLayerVisible( LAY_RAT_LINE, m_Doc->m_vis[LAY_RAT_LINE] );
	}
	else
	{
		m_disable_context_menu = 0;
	}
	ShowSelectStatus();
	CView::OnRButtonDown(nFlags, point);
}

// System Key on keyboard pressed down
//
void CFreePcbView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if( nChar == 121 )
		OnKeyDown( nChar, nRepCnt, nFlags);
	else
		CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

// System Key on keyboard pressed down
//
void CFreePcbView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if( nChar != 121 )
		CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

// Key on keyboard pressed down
//
void CFreePcbView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if( nChar == 'D' )
	{
		// 'd'
		m_Doc->m_drelist->MakeSolidCircles();
		Invalidate( FALSE );
	}
	else
	{
		HandleKeyPress( nChar, nRepCnt, nFlags );
	}

	// don't pass through SysKey F10
	if( nChar != 121 )
		CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

// Key on keyboard pressed down
//
void CFreePcbView::HandleKeyPress(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	if( m_bDraggingRect )
		return;

	if( nChar == 27 )
	{
		// ESC key, if something selected, cancel it
		// otherwise, fake a right-click
		if( CurSelected() )
			CancelSelection();
		else
			OnRButtonDown( nFlags, CPoint(0,0) );
		return;
	}

	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	if( nChar == 8 )
	{
		// backspace, see if we are routing
		if( m_cursor_mode == CUR_DRAG_RAT )
		{
			// backup, if possible, by unrouting preceding segment and changing active layer
			if( m_dir == 0 && m_sel_is > 0 ) 
			{
				// routing forward
				SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
				m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
				int new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is-1].layer;
				m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is-1 );
				m_sel_is--;
				ShowSelectStatus();
				m_last_mouse_point.x = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].x;
				m_last_mouse_point.y = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].y;
				CPoint p = PCBToScreen( m_last_mouse_point );
				SetCursorPos( p.x, p.y );
				OnRatlineRoute();
				m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
				m_active_layer = new_active_layer;
				ShowActiveLayer();
			}
			else if( m_dir == 1 && m_sel_is < m_sel_net->connect[m_sel_ic].nsegs-1 
				&& !(m_sel_is == m_sel_net->connect[m_sel_ic].nsegs-2 
				&& m_sel_net->connect[m_sel_ic].end_pin == cconnect::NO_END ) )
			{
				// routing backward, not at end of stub trace
				SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
				m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
				int new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is+1].layer;
				m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is+1 );
				ShowSelectStatus();
				m_last_mouse_point.x = m_sel_net->connect[m_sel_ic].vtx[m_sel_is+1].x;
				m_last_mouse_point.y = m_sel_net->connect[m_sel_ic].vtx[m_sel_is+1].y;
				CPoint p = PCBToScreen( m_last_mouse_point );
				SetCursorPos( p.x, p.y );
				OnRatlineRoute();
				m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
				m_active_layer = new_active_layer;
				ShowActiveLayer();
			}
		}
		else if( m_cursor_mode == CUR_DRAG_STUB )
		{
			// routing stub trace
			m_Doc->m_nlist->CancelDraggingStub( m_sel_net, m_sel_ic, m_sel_is );
			int new_active_layer = m_sel_net->connect[m_sel_ic].seg[m_sel_is-1].layer;
			if( m_sel_is > 1 )
			{
				SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
				m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1 );
				m_sel_is--;
				ShowSelectStatus();
				m_last_mouse_point.x = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].x;
				m_last_mouse_point.y = m_sel_net->connect[m_sel_ic].vtx[m_sel_is].y;
				CPoint p = PCBToScreen( m_last_mouse_point );
				SetCursorPos( p.x, p.y );
				OnEndVertexAddSegments();
				m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
				m_active_layer = new_active_layer;
				ShowActiveLayer();
			}
			else
			{
				SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY );
				cpart * sel_part = m_Doc->m_plist->GetPart( &m_sel_start_pin.ref_des );
				int i = sel_part->shape->GetPinIndexByName( &m_sel_start_pin.pin_name );
				m_Doc->m_nlist->UndrawConnection( m_sel_net, m_sel_ic );
				m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic );
				CancelSelection();
				m_sel_net = NULL;
				m_sel_part = sel_part;
				m_sel_id = sel_part->m_id;
				m_sel_id.st = ID_PAD;
				m_sel_id.i = i;
				m_Doc->m_plist->SelectPad( sel_part, i );
				OnPadStartStubTrace();
			}
		}
		return;
	}
	int fk = FK_NONE;
	int dx = 0;
	int dy = 0;

	// get cursor position and convert to PCB coords
	CPoint p;
	GetCursorPos( &p );		// cursor pos in screen coords
	p = ScreenToPCB( p );	// convert to PCB coords

	char test_char = nChar;
	if( test_char >= 97 )
		test_char = '1' + nChar - 97;
	char * ch = strchr( layer_char, test_char );
	if( ch ) 
	{
		int ilayer = ch - layer_char;
		if( ilayer < m_Doc->m_num_copper_layers )
		{
			int new_active_layer = ilayer + LAY_TOP_COPPER;
			if( ilayer == m_Doc->m_num_copper_layers-1 )
				new_active_layer = LAY_BOTTOM_COPPER;
			else if( new_active_layer > LAY_TOP_COPPER )
				new_active_layer++;
			if( !m_Doc->m_vis[new_active_layer] )
			{
				PlaySound( TEXT("CriticalStop"), 0, 0 );
				AfxMessageBox( "Can't route on invisible layer" );
				ReleaseDC( pDC );
				return;
			}

			if( m_cursor_mode == CUR_DRAG_RAT || m_cursor_mode == CUR_DRAG_STUB)
			{
				// if we are routing, change layer
				pDC->SelectClipRgn( &m_pcb_rgn );
				SetDCToWorldCoords( pDC );
				if( m_sel_id.ii == 0 && m_dir == 0 )
				{
					// we are trying to change first segment from pad
					int p1 = m_sel_con.start_pin;
					CString pin_name = m_sel_net->pin[p1].pin_name;
					int pin_index = m_sel_net->pin[p1].part->shape->GetPinIndexByName( &pin_name );
					if( m_sel_net->pin[p1].part->shape->m_padstack[pin_index].hole_size == 0)
					{
						// SMT pad, this is illegal;
						new_active_layer = -1;
						PlaySound( TEXT("CriticalStop"), 0, 0 );
					}
				}
				else if( m_sel_id.ii == (m_sel_con.nsegs-1) && m_dir == 1 )
				{
					// we are trying to change last segment to pad
					int p2 = m_sel_con.end_pin;
					CString pin_name = m_sel_net->pin[p2].pin_name;
					int pin_index = m_sel_net->pin[p2].part->shape->GetPinIndexByName( &pin_name );
					if( m_sel_net->pin[p2].part->shape->m_padstack[pin_index].hole_size == 0)
					{
						// SMT pad
						new_active_layer = -1;
						PlaySound( TEXT("CriticalStop"), 0, 0 );
					}
				}
				if( new_active_layer != -1 )
				{
					m_dlist->ChangeRoutingLayer( pDC, new_active_layer, LAY_SELECTION, 0 );
					m_active_layer = new_active_layer;
					ShowActiveLayer();
				}
			}
			else
			{
				m_active_layer = new_active_layer;
				ShowActiveLayer();
			}
			return;
		}
	}

	// continue
	if( nChar >= 112 && nChar <= 123 )	
	{
		// function key pressed 
		fk = m_fkey_option[nChar-112];
	}
	if( nChar >= 37 && nChar <= 40 )
	{
		// arrow key
		BOOL bShift;
		SHORT kc = GetKeyState( VK_SHIFT );
		if( kc < 0 )
			bShift = TRUE;
		else
			bShift = FALSE;
		fk = FK_ARROW;
		int d;
		if( bShift && m_Doc->m_units == MM )
			d = 10000;		// 0.01 mm
		else if( bShift && m_Doc->m_units == MIL )
			d = 25400;		// 1 mil
		else if( m_sel_id.type == ID_NET )
			d = m_Doc->m_routing_grid_spacing;
		else
			d = m_Doc->m_part_grid_spacing;
		if( nChar == 37 )
			dx -= d;
		else if( nChar == 39 )
			dx += d;
		else if( nChar == 38 )
			dy += d;
		else if( nChar == 40 )
			dy -= d;
	}
	else
		gLastKeyWasArrow = FALSE;

	switch( m_cursor_mode )
	{
	case  CUR_NONE_SELECTED:
		if( fk == FK_ADD_AREA )
			OnAddArea();
		else if( fk == FK_ADD_TEXT )
			OnTextAdd();
		else if( fk == FK_ADD_PART )
			m_Doc->OnAddPart();
		else if( fk == FK_REDO_RATLINES )
		{
			SaveUndoInfoForAllNets();
			//			StartTimer();
			m_Doc->m_nlist->OptimizeConnections();
			//			double time = GetElapsedTime();
			Invalidate( FALSE );
		}
		break;

	case CUR_PART_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				if( m_sel_part->glued )
				{
					int ret = AfxMessageBox( "This part is glued, do you want to unglue it ?  ", MB_YESNO ); 
					if( ret == IDYES )
						m_sel_part->glued = 0;
					else
						return;
				}
				SaveUndoInfoForPartAndNets( m_sel_part, 
					CPartList::UNDO_PART_MODIFY );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_plist->Move( m_sel_part, 
				m_sel_part->x+dx,
				m_sel_part->y+dy, 
				m_sel_part->angle,
				m_sel_part->side );
			m_Doc->m_nlist->PartMoved( m_sel_part );
			m_Doc->m_nlist->OptimizeConnections( m_sel_part );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->HighlightPart( m_sel_part );
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_DELETE_PART || nChar == 46 )
			OnPartDelete();
		else if( fk == FK_EDIT_PART )
			m_Doc->OnPartProperties();
		else if( fk == FK_EDIT_FOOTPRINT )
		{			
			m_Doc->m_edit_footprint = TRUE;
			OnPartEditFootprint();
		}
		else if( fk == FK_GLUE_PART )
			OnPartGlue();
		else if( fk == FK_UNGLUE_PART )
			OnPartUnglue();
		else if( fk == FK_MOVE_PART )
			OnPartMove();
		else if( fk == FK_REDO_RATLINES )
			OnPartOptimize();
		break;

	case CUR_REF_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForPart( m_sel_part, 
				CPartList::UNDO_PART_MODIFY );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			CPoint ref_pt = m_Doc->m_plist->GetRefPoint( m_sel_part );
			m_Doc->m_plist->MoveRefText( m_sel_part, 
										ref_pt.x + dx,
										ref_pt.y + dy,
										m_sel_part->m_ref_angle,
										m_sel_part->m_ref_size,
										m_sel_part->m_ref_w );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			m_Doc->m_plist->SelectRefText( m_sel_part );
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_SIZE )
			OnRefProperties();
		else if( fk == FK_MOVE_REF )
			OnRefMove();
		break;

	case CUR_RAT_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnRatlineSetWidth();
		else if( fk == FK_LOCK_CONNECT )
			OnRatlineLockConnection();
		else if( fk == FK_UNLOCK_CONNECT )
			OnRatlineUnlockConnection();
		else if( fk == FK_ROUTE )
			OnRatlineRoute();
		else if( fk == FK_CHANGE_PIN )
			OnRatlineChangeEndPin();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnRatlineDeleteConnection();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();
		break;

	case  CUR_SEG_SELECTED:
		if( fk == FK_SET_WIDTH )
			OnSegmentSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnSegmentChangeLayer();
		else if( fk == FK_UNROUTE )
			OnSegmentUnroute();
		else if( fk == FK_DELETE_SEGMENT )
			OnSegmentDelete();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();	//**
		break;

	case  CUR_VTX_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForConnection( m_sel_net, m_sel_ic ); 
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
										m_sel_vtx.x + dx, m_sel_vtx.y + dy );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_nlist->SelectVertex( m_sel_net, m_sel_ic, m_sel_is );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_MOVE_VERTEX )
			OnVertexMove();
		else if( fk == FK_DELETE_VERTEX )
			OnVertexDelete();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		break;

	case  CUR_END_VTX_SELECTED:
		if( fk == FK_SET_POSITION )
			OnVertexProperties();
		else if( fk == FK_MOVE_VERTEX )
			OnEndVertexMove();
		else if( fk == FK_ADD_SEGMENT )
			OnEndVertexAddSegments();
		else if( fk == FK_ADD_VIA )
			OnEndVertexAddVia();
		else if( fk == FK_DELETE_VIA )
			OnEndVertexRemoveVia();
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		break;

	case  CUR_CONNECT_SELECTED: 
		if( fk == FK_SET_WIDTH )
			OnConnectSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnConnectChangeLayer();
		else if( fk == FK_UNROUTE_TRACE )
			OnUnrouteTrace();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();	//**
		else if( fk == FK_DELETE_CONNECT || nChar == 46 )
			OnSegmentDeleteTrace();
		break;

	case  CUR_NET_SELECTED: 
		if( fk == FK_SET_WIDTH )
			OnNetSetWidth();
		else if( fk == FK_CHANGE_LAYER )
			OnNetChangeLayer();
		else if( fk == FK_EDIT_NET )
			OnNetEditnet();
		else if( fk == FK_REDO_RATLINES )
			OnRatlineOptimize();	//**
		break;

	case  CUR_PAD_SELECTED:
		if( fk == FK_ATTACH_NET )
			OnPadAddToNet();
		else if( fk == FK_START_STUB )
			OnPadStartStubTrace();
		else if( fk == FK_ADD_CONNECT )
			OnPadConnectToPin();
		else if( fk == FK_DETACH_NET )
			OnPadDetachFromNet();
		else if( fk == FK_REDO_RATLINES )
			OnPadOptimize();
		break;

	case CUR_TEXT_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			m_dlist->CancelHighLight();
			m_sel_text = m_Doc->m_tlist->MoveText( m_sel_text, 
						m_sel_text->m_x + dx, m_sel_text->m_y + dy,
						m_sel_text->m_angle, m_sel_text->m_mirror,
						m_sel_text->m_layer );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->m_tlist->HighlightText( m_sel_text );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_EDIT_TEXT )
			OnTextEdit();
		else if( fk == FK_MOVE_TEXT )
			OnTextMove();
		else if( fk == FK_DELETE_TEXT || nChar == 46 )
			OnTextDelete();
		break;

	case CUR_BOARD_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForBoardOutline( UNDO_BOARD_MODIFY );;
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = m_Doc->m_board_outline;
			poly->MoveCorner( m_sel_is, 
				poly->GetX( m_sel_is ) + dx, 
				poly->GetY( m_sel_is ) + dy );
			m_dlist->CancelHighLight();
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			poly->HighlightCorner( m_sel_is );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnBoardCornerEdit();
		else if( fk == FK_MOVE_CORNER )
			OnBoardCornerMove();
		else if( fk == FK_DELETE_CORNER )
			OnBoardCornerDelete();
		else if( fk == FK_DELETE_OUTLINE || nChar == 46 )
			OnBoardDeleteOutline();
		break;

	case CUR_BOARD_SIDE_SELECTED:
		if( fk == FK_POLY_STRAIGHT )
			OnBoardSideConvertToStraightLine();
		else if( fk == FK_POLY_ARC_CW )
			OnBoardSideConvertToArcCw();
		else if( fk == FK_POLY_ARC_CCW )
			OnBoardSideConvertToArcCcw();
		else if( fk == FK_ADD_CORNER )
			OnBoardSideAddCorner();
		else if( fk == FK_DELETE_OUTLINE || nChar == 46 )
			OnBoardDeleteOutline();
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_ic];
			poly->MoveCorner( m_sel_is, 
				poly->GetX( m_sel_is ) + dx,
				poly->GetY( m_sel_is ) + dy );
			m_dlist->CancelHighLight();
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			poly->HighlightCorner( m_sel_is );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnSmCornerSetPosition();
		else if( fk == FK_MOVE_CORNER )
			OnSmCornerMove();
		else if( fk == FK_DELETE_CORNER )
			OnSmCornerDeleteCorner();
		else if( fk == FK_DELETE_CUTOUT || nChar == 46 )
			OnSmCornerDeleteCutout();
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( fk == FK_POLY_STRAIGHT )
			{
				m_dlist->CancelHighLight();
				m_polyline_style = CPolyLine::STRAIGHT;
				poly->SetSideStyle( m_sel_id.ii, m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.ii );
				Invalidate( FALSE );
				m_Doc->ProjectModified( TRUE );
			}
			else if( fk == FK_POLY_ARC_CW )
			{
				m_dlist->CancelHighLight();
				m_polyline_style = CPolyLine::ARC_CW;
				poly->SetSideStyle( m_sel_id.ii, m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.ii );
				Invalidate( FALSE );
				m_Doc->ProjectModified( TRUE );
			}
			else if( fk == FK_POLY_ARC_CCW )
			{
				m_dlist->CancelHighLight();
				m_polyline_style = CPolyLine::ARC_CCW;
				poly->SetSideStyle( m_sel_id.ii, m_polyline_style );
				SetFKText( m_cursor_mode );
				poly->HighlightSide( m_sel_id.ii );
				Invalidate( FALSE );
				m_Doc->ProjectModified( TRUE );
			}
			else if( fk == FK_ADD_CORNER )
				OnSmSideInsertCorner();
			else if( fk == FK_DELETE_CUTOUT || nChar == 46 )
				OnSmSideDeleteCutout();
		}
		break;

	case CUR_AREA_CORNER_SELECTED:
		if( fk == FK_ARROW )
		{
			if( !gLastKeyWasArrow )
			{
				SaveUndoInfoForAllAreasInNet( m_sel_net );  
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			CPolyLine * poly = m_sel_net->area[m_sel_ic].poly;
			poly->MoveCorner( m_sel_is, 
				poly->GetX( m_sel_is ) + dx, 
				poly->GetY( m_sel_is ) + dy );
			int n_poly = m_Doc->m_nlist->AreaModified( m_sel_net, m_sel_ia );
			if( n_poly == -1 )
			{
				// error
				AfxMessageBox( "Error: Arc cannot intersect any other side of copper area" );
				CancelSelection();
				m_Doc->OnEditUndo();
			}
			else
			{
				m_dlist->CancelHighLight();
				gTotalArrowMoveX += dx;
				gTotalArrowMoveY += dy;
				ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
				if( n_poly == 0 )
					poly->HighlightCorner( m_sel_is );
				else
					CancelSelection();
			}
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_SET_POSITION )
			OnAreaCornerProperties();
		else if( fk == FK_MOVE_CORNER )
			OnAreaCornerMove();
		else if( fk == FK_DELETE_CORNER )
			OnAreaCornerDelete();
		else if( fk == FK_DELETE_AREA )
			OnAreaCornerDeleteArea();
		else if( fk == FK_AREA_CUTOUT )
			OnAreaAddCutout();
		else if( fk == FK_DELETE_CUTOUT )
			OnAreaDeleteCutout();
		else if( nChar == 46 )
		{
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			if( poly->GetContour( m_sel_id.ii ) > 0 )
				OnAreaDeleteCutout();
			else
				OnAreaCornerDeleteArea();
		}
		break;

	case CUR_AREA_SIDE_SELECTED: 
		if( fk == FK_POLY_STRAIGHT ) 
		{
			SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY );
			m_polyline_style = CPolyLine::STRAIGHT;
			m_Doc->m_nlist->SetAreaSideStyle( m_sel_net, m_sel_ia, m_sel_is, m_polyline_style );
			m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
			SetFKText( m_cursor_mode );
			Invalidate( FALSE );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY );
			m_polyline_style = CPolyLine::ARC_CW;
			m_Doc->m_nlist->SetAreaSideStyle( m_sel_net, m_sel_ia, m_sel_is, m_polyline_style );
			m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
			SetFKText( m_cursor_mode );
			Invalidate( FALSE );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY );
			m_polyline_style = CPolyLine::ARC_CCW;
			m_Doc->m_nlist->SetAreaSideStyle( m_sel_net, m_sel_ia, m_sel_is, m_polyline_style );
			m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
			SetFKText( m_cursor_mode );
			Invalidate( FALSE );
			m_Doc->ProjectModified( TRUE );
		}
		else if( fk == FK_ADD_CORNER )
			OnAreaSideAddCorner();
		else if( fk == FK_DELETE_AREA )
			OnAreaSideDeleteArea();
		else if( fk == FK_AREA_CUTOUT )
			OnAreaAddCutout();
		else if( fk == FK_DELETE_CUTOUT )
			OnAreaDeleteCutout();
		else if( nChar == 46 )
		{
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			if( poly->GetContour( m_sel_id.ii ) > 0 )
				OnAreaDeleteCutout();
			else
				OnAreaSideDeleteArea();
		}
		break;

	case CUR_DRE_SELECTED:
		if( nChar == 46 )
		{
			CancelSelection();
			m_Doc->m_drelist->Remove( m_sel_dre );
			Invalidate( FALSE );
		}
		break;

	case CUR_GROUP_SELECTED:
		if( fk == FK_ARROW )
		{
			m_dlist->CancelHighLight();
			if( !gLastKeyWasArrow )
			{
				if( GluedPartsInGroup() )
				{
					int ret = AfxMessageBox( "This group contains glued parts, do you want to unglue them ?  ", MB_YESNO ); 
					if( ret != IDYES )
						return;
				}
				SaveUndoInfoForGroup( 0 );
				gTotalArrowMoveX = 0;
				gTotalArrowMoveY = 0;
				gLastKeyWasArrow = TRUE;
			}
			MoveGroup( dx, dy );
			gTotalArrowMoveX += dx;
			gTotalArrowMoveY += dy;
			HighlightGroup();
			ShowRelativeDistance( gTotalArrowMoveX, gTotalArrowMoveY );
			m_Doc->ProjectModified( TRUE );
			Invalidate( FALSE );
		}
		else if( fk == FK_MOVE_GROUP ) 
		{
			OnGroupMove();
		}
		break;

	case CUR_DRAG_RAT:
		if( fk == FK_COMPLETE )
		{
			SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
			int w, v_w, v_h_w;
			GetWidthsForSegment( &w, &v_w, &v_h_w );
			int test = m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, 
				m_sel_is, m_active_layer, w, v_w, v_h_w );
			if( !test )
			{
				m_Doc->m_nlist->CancelDraggingSegment( m_sel_net, m_sel_ic, m_sel_is );
				CancelSelection();
			}
			else
				PlaySound( TEXT("CriticalStop"), 0, 0 );
			Invalidate( FALSE );
			m_Doc->ProjectModified( TRUE );
		}
		break;

	case CUR_DRAG_STUB:
		break;

	case  CUR_DRAG_PART:
		if( fk == FK_ROTATE_PART )
			m_dlist->IncrementDragAngle( pDC );
		else if( fk == FK_SIDE )
			m_dlist->FlipDragSide( pDC );
		break;

	case  CUR_DRAG_REF: 
		if( fk == FK_ROTATE_REF )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_TEXT:
		if( fk == FK_ROTATE_TEXT )
			m_dlist->IncrementDragAngle( pDC );
		break;

	case  CUR_DRAG_BOARD:
	case  CUR_DRAG_BOARD_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	case  CUR_DRAG_AREA:
	case  CUR_DRAG_AREA_1:
	case  CUR_DRAG_AREA_CUTOUT:
	case  CUR_DRAG_AREA_CUTOUT_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	case  CUR_DRAG_SMCUTOUT:
	case  CUR_DRAG_SMCUTOUT_1:
		if( fk == FK_POLY_STRAIGHT )
		{
			m_polyline_style = CPolyLine::STRAIGHT;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CW )
		{
			m_polyline_style = CPolyLine::ARC_CW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		else if( fk == FK_POLY_ARC_CCW )
		{
			m_polyline_style = CPolyLine::ARC_CCW;
			m_dlist->SetDragArcStyle( m_polyline_style );
			m_dlist->Drag( pDC, p.x, p.y );
		}
		break;

	default: 
		break;
	}	// end switch

	if( nChar == ' ' )
	{
		// space bar pressed, center window on cursor then center cursor
		m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
		m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
			m_org_x, m_org_y );
		Invalidate( FALSE );
		p = PCBToScreen( p );
		SetCursorPos( p.x, p.y - 4 );
	}
	else if( nChar == VK_HOME )
	{
		// home key pressed, ViewAllParts
		OnViewAllElements();
	}
	else if( nChar == 33 )
	{
		// PgUp pressed, zoom in
		if( m_pcbu_per_pixel > 0.1 )
		{
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
			m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
				m_org_x, m_org_y );
			Invalidate( FALSE );
			p = PCBToScreen( p );
			SetCursorPos( p.x, p.y - 4 );
		}
	}
	else if( nChar == 34 )
	{
		// PgDn pressed, zoom out
		// first, make sure that window boundaries will be OK
		int org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
		int org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
		int max_x = org_x + (m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO;
		int max_y = org_y + (m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO;
		if( org_x > -PCB_BOUND && org_x < PCB_BOUND && max_x > -PCB_BOUND && max_x < PCB_BOUND
			&& org_y > -PCB_BOUND && org_y < PCB_BOUND && max_y > -PCB_BOUND && max_y < PCB_BOUND )
		{
			// OK, do it
			m_org_x = org_x;
			m_org_y = org_y;
			m_pcbu_per_pixel = m_pcbu_per_pixel*ZOOM_RATIO;
			m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
				m_org_x, m_org_y );
			Invalidate( FALSE );
			p = PCBToScreen( p );
			SetCursorPos( p.x, p.y - 4 );
		}
	}
	ReleaseDC( pDC );
	if( gLastKeyWasArrow == FALSE )
		ShowSelectStatus();
}

// Mouse moved
//
void CFreePcbView::OnMouseMove(UINT nFlags, CPoint point) 
{
	if( (nFlags & MK_LBUTTON) && m_bLButtonDown )
	{
		double d = abs(point.x-m_start_pt.x) + abs(point.y-m_start_pt.y);
		if( m_bDraggingRect 
			|| (d > 10 && !CurDragging() ) )
		{
			// we are dragging a selection rect
			SIZE s1;
			s1.cx = s1.cy = 1;
			m_drag_rect.TopLeft() = m_start_pt;
			m_drag_rect.BottomRight() = point;
			m_drag_rect.NormalizeRect();
			CDC * pDC = GetDC();
			if( !m_bDraggingRect )
			{
				//start dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, NULL, s1 );
			}
			else
			{
				// continue dragging rect
				pDC->DrawDragRect( &m_drag_rect, s1, &m_last_drag_rect, s1 );
			}
			m_bDraggingRect  = TRUE;
			m_last_drag_rect = m_drag_rect;
			ReleaseDC( pDC );
		}
	}
	m_last_mouse_point = WindowToPCB( point );
	SnapCursorPoint( m_last_mouse_point );
}

/////////////////////////////////////////////////////////////////////////
// Utility functions
//

// Set the device context to world coords
//
CFreePcbView::SetDCToWorldCoords( CDC * pDC )
{
	m_dlist->SetDCToWorldCoords( pDC, &m_memDC, m_pcbu_per_pixel, m_org_x, m_org_y,
										m_client_r, m_left_pane_w, m_bottom_pane_h );

	return 0;
}


// Convert point in window coords to PCB units (i.e. nanometers)
//
CPoint CFreePcbView::WindowToPCB( CPoint point )
{
	CPoint p;
	p.x = (point.x-m_left_pane_w)*m_pcbu_per_pixel + m_org_x;
	p.y = (m_client_r.bottom-m_bottom_pane_h-point.y)*m_pcbu_per_pixel + m_org_y;
	return p;
}

// Convert point in screen coords to PCB units
//
CPoint CFreePcbView::ScreenToPCB( CPoint point )
{
	CPoint p;
	CRect wr;
	GetWindowRect( &wr );		// client rect in screen coords
	p.x = point.x - wr.left;
	p.y = point.y - wr.top;
	p = WindowToPCB( p );
	return p;
}

// Convert point in PCB units to screen coords
//
CPoint CFreePcbView::PCBToScreen( CPoint point )
{
	CPoint p;
	CRect wr;
	GetWindowRect( &wr );		// client rect in screen coords
	p.x = (point.x - m_org_x)/m_pcbu_per_pixel+m_left_pane_w+wr.left;
	p.y = (m_org_y - point.y)/m_pcbu_per_pixel-m_bottom_pane_h+wr.bottom;
	return p;
}

// Set cursor mode, update function key menu if necessary
//
void CFreePcbView::SetCursorMode( int mode )
{
	if( mode != m_cursor_mode )
	{
		SetFKText( mode );
		m_cursor_mode = mode;
		ShowSelectStatus();
	}
}

// Set function key shortcut text
//
void CFreePcbView::SetFKText( int mode )
{
	for( int i=0; i<12; i++ )
	{
		m_fkey_option[i] = 0;
		m_fkey_command[i] = 0;
	}

	switch( mode )
	{
	case CUR_NONE_SELECTED:
		if( m_Doc->m_project_open )
		{
			m_fkey_option[1] = FK_ADD_AREA;
			m_fkey_option[2] = FK_ADD_TEXT;
			m_fkey_option[3] = FK_ADD_PART;
			m_fkey_option[7] = FK_REDO_RATLINES;
		}
		break;

	case CUR_PART_SELECTED:
		m_fkey_option[0] = FK_EDIT_PART;
		m_fkey_option[1] = FK_EDIT_FOOTPRINT;
		if( m_sel_part->glued )
			m_fkey_option[2] = FK_UNGLUE_PART;
		else
			m_fkey_option[2] = FK_GLUE_PART;
		m_fkey_option[3] = FK_MOVE_PART;
		m_fkey_option[6] = FK_DELETE_PART;
		m_fkey_option[7] = FK_REDO_RATLINES;
		break;

	case CUR_REF_SELECTED:
		m_fkey_option[0] = FK_SET_SIZE;
		m_fkey_option[3] = FK_MOVE_REF;
		break;

	case CUR_PAD_SELECTED:
		if( m_sel_part->pin[m_sel_id.i].net )
			m_fkey_option[0] = FK_DETACH_NET;
		else
			m_fkey_option[0] = FK_ATTACH_NET;
		m_fkey_option[2] = FK_START_STUB;
		m_fkey_option[3] = FK_ADD_CONNECT;
		m_fkey_option[7] = FK_REDO_RATLINES;
		break;

	case CUR_TEXT_SELECTED:
		m_fkey_option[0] = FK_EDIT_TEXT;
		m_fkey_option[3] = FK_MOVE_TEXT;
		m_fkey_option[6] = FK_DELETE_TEXT;
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		m_fkey_option[6] = FK_DELETE_CUTOUT;
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		{
			int style = m_Doc->m_sm_cutout[m_sel_id.i].GetSideStyle( m_sel_id.ii );
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
		}
		m_fkey_option[6] = FK_DELETE_CUTOUT;
		break;

	case CUR_BOARD_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		m_fkey_option[6] = FK_DELETE_OUTLINE;
		break;

	case CUR_BOARD_SIDE_SELECTED:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		{
			int style = m_Doc->m_board_outline->GetSideStyle( m_sel_id.ii );
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
		}
		m_fkey_option[6] = FK_DELETE_OUTLINE;
		break;

	case CUR_AREA_CORNER_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_CORNER;
		m_fkey_option[4] = FK_DELETE_CORNER;
		{
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			if( poly->GetContour( m_sel_id.ii ) > 0 )
				m_fkey_option[5] = FK_DELETE_CUTOUT;
			else
				m_fkey_option[5] = FK_AREA_CUTOUT;
		}
		m_fkey_option[6] = FK_DELETE_AREA;
		break;

	case CUR_AREA_SIDE_SELECTED:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		{
			int style = m_sel_net->area[m_sel_id.i].poly->GetSideStyle(m_sel_id.ii);
			if( style == CPolyLine::STRAIGHT )
				m_fkey_option[3] = FK_ADD_CORNER;
		}
		{
			CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
			if( poly->GetContour( m_sel_id.ii ) > 0 )
				m_fkey_option[5] = FK_DELETE_CUTOUT;
			else
				m_fkey_option[5] = FK_AREA_CUTOUT;
		}
		m_fkey_option[6] = FK_DELETE_AREA;
		break;

	case CUR_SEG_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		if( m_sel_con.end_pin == cconnect::NO_END )
		{
			// stub trace
			if( m_sel_con.nsegs == (m_sel_id.ii+1) )
			{
				// end segment of stub trace
				m_fkey_option[4] = FK_DELETE_SEGMENT;
			}
			else
			{
				// other segment of stub trace
				m_fkey_option[4] = FK_UNROUTE;
			}
		}
		else
		{
			// normal trace
			m_fkey_option[4] = FK_UNROUTE;
			m_fkey_option[5] = FK_UNROUTE_TRACE;
		}
		m_fkey_option[6] = FK_DELETE_CONNECT;
		m_fkey_option[7] = FK_REDO_RATLINES;
		break;

	case CUR_RAT_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		if( m_sel_con.locked )
			m_fkey_option[2] = FK_UNLOCK_CONNECT;
		else
			m_fkey_option[2] = FK_LOCK_CONNECT;
		m_fkey_option[3] = FK_ROUTE;
		if( m_sel_con.nsegs > 1
			&& ( m_sel_id.ii == 0 || m_sel_id.ii == (m_sel_con.nsegs-1) ) )
		{
			m_fkey_option[4] = FK_CHANGE_PIN;
		}
		m_fkey_option[6] = FK_DELETE_CONNECT;
		m_fkey_option[7] = FK_REDO_RATLINES;
		break;

	case CUR_VTX_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		m_fkey_option[4] = FK_DELETE_VERTEX;
		m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[6] = FK_DELETE_CONNECT;
		m_fkey_option[7] = FK_REDO_RATLINES;
		break;

	case CUR_END_VTX_SELECTED:
		m_fkey_option[0] = FK_SET_POSITION;
		m_fkey_option[1] = FK_ADD_SEGMENT;
		if( m_sel_vtx.via_w )
			m_fkey_option[2] = FK_DELETE_VIA;
		else
			m_fkey_option[2] = FK_ADD_VIA;
		m_fkey_option[3] = FK_MOVE_VERTEX;
		m_fkey_option[4] = FK_DELETE_VERTEX;
		m_fkey_option[6] = FK_DELETE_CONNECT;
		m_fkey_option[7] = FK_REDO_RATLINES;
		break;

	case CUR_CONNECT_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		m_fkey_option[5] = FK_UNROUTE_TRACE;
		m_fkey_option[6] = FK_DELETE_CONNECT;
		m_fkey_option[7] = FK_REDO_RATLINES;
		break;

	case CUR_NET_SELECTED:
		m_fkey_option[0] = FK_SET_WIDTH;
		m_fkey_option[1] = FK_CHANGE_LAYER;
		m_fkey_option[2] = FK_EDIT_NET;
		m_fkey_option[7] = FK_REDO_RATLINES;
		break;

	case CUR_GROUP_SELECTED:
		m_fkey_option[3] = FK_MOVE_GROUP;
		break;

	case CUR_DRAG_PART:
		m_fkey_option[1] = FK_SIDE;
		m_fkey_option[2] = FK_ROTATE_PART;
		break;

	case CUR_DRAG_REF:
		m_fkey_option[2] = FK_ROTATE_REF;
		break;

	case CUR_DRAG_TEXT:
		m_fkey_option[2] = FK_ROTATE_TEXT;
		break;

	case CUR_DRAG_VTX:
		break;

	case CUR_DRAG_RAT:
		m_fkey_option[3] = FK_COMPLETE;
		break;

	case CUR_DRAG_STUB:
		break;

	case CUR_DRAG_SMCUTOUT_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_SMCUTOUT:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_CUTOUT:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA_CUTOUT_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_AREA:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_BOARD:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;

	case CUR_DRAG_BOARD_1:
		m_fkey_option[0] = FK_POLY_STRAIGHT;
		m_fkey_option[1] = FK_POLY_ARC_CW;
		m_fkey_option[2] = FK_POLY_ARC_CCW;
		break;
	}

	for( i=0; i<12; i++ )
	{
		strcpy( m_fkey_str[2*i],   fk_str[2*m_fkey_option[i]] );
		strcpy( m_fkey_str[2*i+1], fk_str[2*m_fkey_option[i]+1] );
	}

	InvalidateLeftPane();
	Invalidate( FALSE );
}

// Draw bottom pane
//
void CFreePcbView::DrawBottomPane()
{
	CDC * pDC = GetDC();
	CFont * old_font = pDC->SelectObject( &m_small_font );

	// get client rectangle
	GetClientRect( &m_client_r );

	// draw labels for function keys at bottom of client area
	for( int j=0; j<2; j++ )
	{
		for( int i=0; i<4; i++ )
		{
			CRect r( FKEY_OFFSET_X+(j*4+i)*FKEY_STEP+j*FKEY_GAP, 
						m_client_r.bottom-FKEY_OFFSET_Y-FKEY_R_H, 
						FKEY_OFFSET_X+(j*4+i)*FKEY_STEP+j*FKEY_GAP+FKEY_R_W,
						m_client_r.bottom-FKEY_OFFSET_Y );
			pDC->Rectangle( &r );
			pDC->MoveTo( r.left+FKEY_SEP_W, r.top );
			pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 + 1 );
			pDC->MoveTo( r.left, r.top + FKEY_R_H/2 );
			pDC->LineTo( r.left+FKEY_SEP_W, r.top + FKEY_R_H/2 );
			r.top += 1;
			r.left += 2;
			char fkstr[3] = "F1";
			fkstr[1] = '1' + j*4+i;
			pDC->DrawText( fkstr, -1, &r, 0 );
			r.left += FKEY_SEP_W;
			char * str1 = &m_fkey_str[2*(j*4+i)][0];
			char * str2 = &m_fkey_str[2*(j*4+i)+1][0];
			pDC->DrawText( str1, -1, &r, 0 );
			r.top += FKEY_R_H/2 - 2;
			pDC->DrawText( str2, -1, &r, 0 );
		}
	}
	pDC->SelectObject( old_font );
	ReleaseDC( pDC );
}

void CFreePcbView::ShowRelativeDistance( int x, int y )
{
	CString str;
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( m_Doc->m_units == MIL )
		str.Format( "dx = %d, dy = %d", x/NM_PER_MIL, y/NM_PER_MIL );
	else
		str.Format( "dx = %.3f, dy = %.3f", x/1000000.0, y/1000000.0 );
	pMain->DrawStatus( 3, &str );
}

// display selected item in status bar 
//
int CFreePcbView::ShowSelectStatus()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;

	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED: 
		str.Format( "No selection" );
		break;

	case CUR_DRE_SELECTED: 
		str.Format( "DRE %s", m_sel_dre->str );
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED: 
		{
			CString lay_str;
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( poly->GetLayer() == LAY_SM_TOP )
				lay_str = "Top";
			else
				lay_str = "Bottom";
			str.Format( "Solder mask cutout %d: %s, corner %d, x %d, y %d", 
				m_sel_id.i+1, lay_str, m_sel_id.ii+1, 
				poly->GetX(m_sel_id.ii)/NM_PER_MIL,
				poly->GetY(m_sel_id.ii)/NM_PER_MIL );
		}
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED: 
		{
			CString style_str;
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( poly->GetSideStyle( m_sel_id.ii ) == CPolyLine::STRAIGHT )
				style_str = "straight";
			else if( poly->GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CW )
				style_str = "arc(cw)";
			else if( poly->GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CCW )
				style_str = "arc(ccw)";
			CString lay_str;
			if( poly->GetLayer() == LAY_SM_TOP )
				lay_str = "Top";
			else
				lay_str = "Bottom";
			str.Format( "Solder mask cutout %d: %s, side %d of %d, %s", 
				m_sel_id.i+1, lay_str, m_sel_id.ii+1, 
				poly->GetNumCorners(), style_str );
		} 
		break;

	case CUR_BOARD_CORNER_SELECTED: 
		str.Format( "board outline corner %d, x %d, y %d", 
			m_sel_id.ii+1,
			m_Doc->m_board_outline->GetX(m_sel_id.ii)/NM_PER_MIL,
			m_Doc->m_board_outline->GetY(m_sel_id.ii)/NM_PER_MIL );
		break;

	case CUR_BOARD_SIDE_SELECTED: 
		{
			CString style_str;
			if( m_Doc->m_board_outline->GetSideStyle( m_sel_id.ii ) == CPolyLine::STRAIGHT )
				style_str = "straight";
			else if( m_Doc->m_board_outline->GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CW )
				style_str = "arc(cw)";
			else if( m_Doc->m_board_outline->GetSideStyle( m_sel_id.ii ) == CPolyLine::ARC_CCW )
				style_str = "arc(ccw)";
			str.Format( "board outline side %d of %d, %s", m_sel_id.ii+1, 
				m_Doc->m_board_outline->GetNumCorners(), style_str );
		} 
		break;

	case CUR_PART_SELECTED: 
		{
			CString side = "top";
			if( m_sel_part->side )
				side = "bottom";
			str.Format( "part %s \"%s\", x %d, y %d, angle %d, %s", 
				m_sel_part->ref_des, m_sel_part->shape->m_name, 
				m_sel_part->x/NM_PER_MIL, m_sel_part->y/NM_PER_MIL, m_sel_part->angle, side );
		} 
		break;

	case CUR_REF_SELECTED:
		str.Format( "ref text %s", m_sel_part->ref_des );
		break;

	case CUR_PAD_SELECTED: 
		{
			cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
			int x = m_sel_part->pin[m_sel_id.i].x/NM_PER_MIL;
			int y = m_sel_part->pin[m_sel_id.i].y/NM_PER_MIL;
			if( pin_net )
			{
				// pad attached to net
				str.Format( "pin %s.%s on net \"%s\", x %d, y %d", 
					m_sel_part->ref_des, 
					m_sel_part->shape->GetPinNameByIndex(m_sel_id.i),
					pin_net->name, x, y );
			}
			else
			{
				// pad not attached to a net
				str.Format( "pin %s.%s unconnected, x %d, y %d", 
					m_sel_part->ref_des, 
					m_sel_part->shape->GetPinNameByIndex(m_sel_id.i),
					x, y );
			}
		} 
		break;

	case CUR_SEG_SELECTED: 
	case CUR_RAT_SELECTED:
	case CUR_DRAG_STUB:
	case CUR_DRAG_RAT:
		{
			if( m_sel_con.end_pin == cconnect::NO_END )
			{
				if( m_cursor_mode == CUR_DRAG_STUB )
				{
					// stub trace segment
					str.Format( "net \"%s\" stub from %s.%s, seg %d", 
						m_sel_net->name,
						m_sel_start_pin.ref_des, 
						m_sel_start_pin.pin_name, 
						m_sel_id.ii+1
						); 
				}
				else
				{
					// stub trace segment
					str.Format( "net \"%s\" stub from %s.%s, seg %d, width %d, v_w %d, v_h_w %d", 
						m_sel_net->name,
						m_sel_start_pin.ref_des, 
						m_sel_start_pin.pin_name, 
						m_sel_id.ii+1,
						m_sel_seg.width/NM_PER_MIL,
						m_sel_seg.via_w/NM_PER_MIL,
						m_sel_seg.via_hole_w/NM_PER_MIL
						); 
				}
			}
			else
			{
				// normal connected trace segment
				CString locked_flag = "";
				if( m_sel_con.locked )  
					locked_flag = " (L)";
				if( m_sel_con.nsegs == 1 )
				{
					str.Format( "net \"%s\" connection %s.%s-%s.%s%s, seg %d, width %d, v_w %d, v_h_w %d",  
						m_sel_net->name,
						m_sel_start_pin.ref_des,  
						m_sel_start_pin.pin_name, 
						m_sel_end_pin.ref_des, 
						m_sel_end_pin.pin_name,
						locked_flag, m_sel_id.ii+1,
						m_sel_seg.width/NM_PER_MIL,
						m_sel_seg.via_w/NM_PER_MIL,
						m_sel_seg.via_hole_w/NM_PER_MIL
						); 
				}
				else
				{
					str.Format( "net \"%s\" trace %s.%s-%s.%s%s, seg %d, width %d, v_w %d, v_h_w %d",  
						m_sel_net->name,
						m_sel_start_pin.ref_des,  
						m_sel_start_pin.pin_name, 
						m_sel_end_pin.ref_des, 
						m_sel_end_pin.pin_name,
						locked_flag, m_sel_id.ii+1,
						m_sel_seg.width/NM_PER_MIL,
						m_sel_seg.via_w/NM_PER_MIL,
						m_sel_seg.via_hole_w/NM_PER_MIL
						); 
				}
			}
		} 
		break;

	case CUR_VTX_SELECTED: 
		{
			CString locked_flag = "";
			if( m_sel_con.locked )
				locked_flag = " (L)";
			int via_w = m_sel_vtx.via_w;
			if( m_sel_con.end_pin == cconnect::NO_END )
			{
				// vertex of stub trace
				if( via_w )
				{
					// via
					str.Format( "net \"%s\" stub from %s.%s, vertex %d, x %d, y %d, via %d/%d", m_sel_net->name, 
						m_sel_start_pin.ref_des, 
						m_sel_start_pin.pin_name, 
						m_sel_id.ii,
						m_sel_vtx.x/NM_PER_MIL,
						m_sel_vtx.y/NM_PER_MIL,
						m_sel_vtx.via_w/NM_PER_MIL,
						m_sel_vtx.via_hole_w/NM_PER_MIL
						); 
				}
				else
				{
					// no via
					str.Format( "net \"%s\" stub from %s.%s, vertex %d, x %d, y %d", m_sel_net->name, 
						m_sel_start_pin.ref_des, 
						m_sel_start_pin.pin_name, 
						m_sel_id.ii,
						m_sel_vtx.x/NM_PER_MIL,
						m_sel_vtx.y/NM_PER_MIL
						); 
				}
			}
			else
			{
				// vertex of normal connected trace
				if( via_w )
				{
					// with via
					str.Format( "net \"%s\" trace %s.%s-%s.%s%s, vertex %d, x %d, y %d, via %d/%d", 
						m_sel_net->name, 
						m_sel_start_pin.ref_des, 
						m_sel_start_pin.pin_name, 
						m_sel_end_pin.ref_des, 
						m_sel_end_pin.pin_name,
						locked_flag,
						m_sel_id.ii,
						m_sel_vtx.x/NM_PER_MIL,
						m_sel_vtx.y/NM_PER_MIL,
						m_sel_vtx.via_w/NM_PER_MIL,
						m_sel_vtx.via_hole_w/NM_PER_MIL
						); 
				}
				else
				{
					// no via
					str.Format( "net \"%s\" trace %s.%s-%s.%s%s, vertex %d, x %d, y %d", 
						m_sel_net->name, 
						m_sel_start_pin.ref_des, 
						m_sel_start_pin.pin_name, 
						m_sel_end_pin.ref_des, 
						m_sel_end_pin.pin_name,
						locked_flag,
						m_sel_id.ii,
						m_sel_vtx.x/NM_PER_MIL,
						m_sel_vtx.y/NM_PER_MIL
						); 
				}
			}
		} 
		break;

	case CUR_END_VTX_SELECTED: 
		{
			int itest = m_sel_con.start_pin;
			int itest2 = m_sel_vtx.via_w;
			str.Format( "net \"%s\" stub end, x %d, y %d, via %d/%d", m_sel_net->name, 
				m_sel_vtx.x/NM_PER_MIL,
				m_sel_vtx.y/NM_PER_MIL,
				m_sel_vtx.via_w/NM_PER_MIL,
				m_sel_vtx.via_hole_w/NM_PER_MIL ); 
		} 
		break;

	case CUR_CONNECT_SELECTED:
		{
			CString locked_flag = "";
			if( m_sel_con.locked )
				locked_flag = " (L)";
			if( m_sel_con.end_pin == cconnect::NO_END )
			{
				// stub trace
				str.Format( "net \"%s\" stub trace from %s.%s%s", 
					m_sel_net->name, 
					m_sel_start_pin.ref_des, 
					m_sel_start_pin.pin_name, 
					locked_flag ); 
			}
			else
			{
				// normal trace
				str.Format( "net \"%s\" trace %s.%s-%s.%s%s", 
					m_sel_net->name, 
					m_sel_start_pin.ref_des, 
					m_sel_start_pin.pin_name, 
					m_sel_end_pin.ref_des, 
					m_sel_end_pin.pin_name,
					locked_flag ); 
			}
		}
		break;

	case CUR_NET_SELECTED: 
		str.Format( "net \"%s\"", m_sel_net->name ); 
		break;

	case CUR_TEXT_SELECTED:
		str.Format( "Text \"%s\"", m_sel_text->m_str ); 
		break;

	case CUR_AREA_CORNER_SELECTED:
		{
			CPoint p = m_Doc->m_nlist->GetAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
			str.Format( "\"%s\" copper area %d corner %d, x %d, y %d", 
				m_sel_net->name, m_sel_id.i+1, m_sel_id.ii+1,
				p.x/NM_PER_MIL, p.y/NM_PER_MIL ); 
		}
		break;

	case CUR_AREA_SIDE_SELECTED:
		{
			int ic = m_sel_id.ii;
			int ia = m_sel_id.i; 
			CPolyLine * p = m_sel_net->area[ia].poly;
			int ncont = p->GetContour(ic);
			if( ncont == 0 )
				str.Format( "\"%s\" copper area %d edge %d", m_sel_net->name, ia+1, ic+1 ); 
			else
			{
				str.Format( "\"%s\" copper area %d cutout %d edge %d", 
					m_sel_net->name, ia+1, ncont, ic+1-p->GetContourStart(ncont) ); 
			}
		}
		break;

	case CUR_GROUP_SELECTED:
		str.Format( "Group selected" );
		break;

	case CUR_ADD_BOARD:
		str.Format( "Placing first corner of board outline" );
		break;

	case CUR_DRAG_BOARD_1:
		str.Format( "Placing second corner of board outline" );
		break;

	case CUR_DRAG_BOARD:
		str.Format( "Placing corner %d of board outline", m_sel_id.ii+2 );
		break;

	case CUR_DRAG_BOARD_INSERT:
		str.Format( "Inserting corner %d of board outline", m_sel_id.ii+2 );
		break;

	case CUR_DRAG_BOARD_MOVE:
		str.Format( "Moving corner %d of board outline", m_sel_id.ii+1 );
		break;

	case CUR_DRAG_PART:
		str.Format( "Moving part %s", m_sel_part->ref_des );
		break;

	case CUR_DRAG_REF:
		str.Format( "Moving ref text for part %s", m_sel_part->ref_des );
		break;

	case CUR_DRAG_VTX:
		str.Format( "Routing net \"%s\"", m_sel_net->name );
		break;

	case CUR_DRAG_END_VTX:
		str.Format( "Routing net \"%s\"", m_sel_net->name );
		break;

	case CUR_DRAG_TEXT:
		str.Format( "Moving text \"%s\"", m_sel_text->m_str );
		break;

	case CUR_ADD_AREA:
		str.Format( "Placing first corner of copper area" );
		break;

	case CUR_DRAG_AREA_1:
		str.Format( "Placing second corner of copper area" );
		break;

	case CUR_DRAG_AREA:
		str.Format( "Placing corner %d of copper area", m_sel_id.ii+1 );
		break;

	case CUR_DRAG_AREA_INSERT:
		str.Format( "Inserting corner %d of copper area", m_sel_id.ii+2 );
		break;

	case CUR_DRAG_AREA_MOVE:
		str.Format( "Moving corner %d of copper area", m_sel_id.ii+1 );
		break;

	case CUR_DRAG_CONNECT:
		str.Format( "Adding connection to pin \"%s.%s",
			m_sel_part->ref_des, 
			m_sel_part->shape->GetPinNameByIndex(m_sel_id.i) );
		break;

	}
	pMain->DrawStatus( 3, &str );
	return 0;
}

// display cursor coords in status bar 
//
int CFreePcbView::ShowCursor()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;
	CPoint p;
	p = m_last_cursor_point;
	if( m_Doc->m_units == MIL )
	{
		str.Format( "X: %d", m_last_cursor_point.x/PCBU_PER_MIL );
		pMain->DrawStatus( 1, &str );
		str.Format( "Y: %d", m_last_cursor_point.y/PCBU_PER_MIL );
		pMain->DrawStatus( 2, &str );
	}
	else
	{
		str.Format( "X: %8.3f", m_last_cursor_point.x/1000000.0 );
		pMain->DrawStatus( 1, &str );
		str.Format( "Y: %8.3f", m_last_cursor_point.y/1000000.0 );
		pMain->DrawStatus( 2, &str );
	}
	return 0;
}

// display active layer in status bar and change layer order for DisplayList
//
int CFreePcbView::ShowActiveLayer()
{
	CMainFrame * pMain = (CMainFrame*) AfxGetApp()->m_pMainWnd;
	if( !pMain )
		return 1;

	CString str;
	if( m_active_layer == LAY_TOP_COPPER )
		str.Format( "Top" );
	else if( m_active_layer == LAY_BOTTOM_COPPER )
		str.Format( "Bottom" );
	else if( m_active_layer > LAY_BOTTOM_COPPER )
		str.Format( "Inner %d", m_active_layer - LAY_BOTTOM_COPPER );
	pMain->DrawStatus( 4, &str );
	for( int order=LAY_TOP_COPPER; order<LAY_TOP_COPPER+m_Doc->m_num_copper_layers; order++ )
	{
		if( order == LAY_TOP_COPPER )
			m_dlist->SetLayerDrawOrder( m_active_layer, order );
		else if( order <= m_active_layer )
			m_dlist->SetLayerDrawOrder( order-1, order );
		else
			m_dlist->SetLayerDrawOrder( order, order );
	}
	Invalidate( FALSE );
	return 0;
}

// handle mouse scroll wheel
//
BOOL CFreePcbView::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) 
{
#define MIN_WHEEL_DELAY 1.0

	static struct _timeb current_time;
	static struct _timeb last_time;
	static int first_time = 1;
	double diff;

	// ignore if cursor not in window
	CRect wr;
	GetWindowRect( wr );
	if( pt.x < wr.left || pt.x > wr.right || pt.y < wr.top || pt.y > wr.bottom )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	// ignore if we are dragging a selection rect
	if( m_bDraggingRect )
		return CView::OnMouseWheel(nFlags, zDelta, pt);

	// get current time
	_ftime( &current_time );
	
	if( first_time )
	{
		diff = 999.0;
		first_time = 0;
	}
	else
	{
		// get elapsed time since last wheel event
		diff = difftime( current_time.time, last_time.time );
		double diff_mil = (double)(current_time.millitm - last_time.millitm)*0.001;
		diff = diff + diff_mil;
	}

	if( diff > MIN_WHEEL_DELAY )
	{
		// first wheel movement in a while
		// center window on cursor then center cursor
		CPoint p;
		GetCursorPos( &p );		// cursor pos in screen coords
		p = ScreenToPCB( p );
		m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
		m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
		m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
		Invalidate( FALSE );
		p = PCBToScreen( p );
		SetCursorPos( p.x, p.y - 4 );
	}
	else
	{
		// serial movements, zoom in or out
		if( zDelta > 0 && m_pcbu_per_pixel > (0.1*PCBU_PER_WU) )
		{
			// wheel pushed, zoom in then center world coords and cursor
			CPoint p;
			GetCursorPos( &p );		// cursor pos in screen coords
			p = ScreenToPCB( p );	// convert to PCB coords
			m_pcbu_per_pixel = m_pcbu_per_pixel/ZOOM_RATIO;
			m_org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
			m_org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
			m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
			Invalidate( FALSE );
			p = PCBToScreen( p );
			SetCursorPos( p.x, p.y - 4 );
		}
		else if( zDelta < 0 )
		{
			// wheel pulled, zoom out then center
			// first, make sure that window boundaries will be OK
			CPoint p;
			GetCursorPos( &p );		// cursor pos in screen coords
			p = ScreenToPCB( p );
			int org_x = p.x - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
			int org_y = p.y - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO)/2;
			int max_x = org_x + (m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel*ZOOM_RATIO;
			int max_y = org_y + (m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel*ZOOM_RATIO;
			if( org_x > -PCB_BOUND && org_x < PCB_BOUND && max_x > -PCB_BOUND && max_x < PCB_BOUND
				&& org_y > -PCB_BOUND && org_y < PCB_BOUND && max_y > -PCB_BOUND && max_y < PCB_BOUND )
			{
				// OK, do it
				m_org_x = org_x;
				m_org_y = org_y;
				m_pcbu_per_pixel = m_pcbu_per_pixel*ZOOM_RATIO;
				m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, m_org_x, m_org_y );
				Invalidate( FALSE );
				p = PCBToScreen( p );
				SetCursorPos( p.x, p.y - 4 );
			}
		}
	}
	last_time = current_time;

	return CView::OnMouseWheel(nFlags, zDelta, pt);
}

// SelectPart...this is called from FreePcbDoc when a new part is added
// selects the new part as long as the cursor is not dragging something
//
int CFreePcbView::SelectPart( cpart * part ) 
{
	if(	!CurDragging() )	
	{
		// deselect previously selected item
		if( m_sel_id.type )
			CancelSelection();

		// select part
		m_sel_part = part;
		m_sel_id = part->m_id;
		m_sel_id.st = ID_SEL_RECT;
		m_Doc->m_plist->HighlightPart( m_sel_part );
		SetCursorMode( CUR_PART_SELECTED );
	}
	gLastKeyWasArrow = FALSE;
	Invalidate( FALSE );
	return 0;
}

// cancel selection
//
void CFreePcbView::CancelSelection()
{
	m_Doc->m_dlist->CancelHighLight();
	m_sel_ids.RemoveAll();
	m_sel_ptrs.RemoveAll();
	m_sel_id.Clear();
	SetCursorMode( CUR_NONE_SELECTED );
}

// set trace width using dialog
// enter with:
//	mode = 0 if called with segment selected
//	mode = 1 if called with connection selected	
//	mode = 2 if called with net selected	
//
int CFreePcbView::SetWidth( int mode )
{
	// set parameters for dialog
	DlgSetSegmentWidth seg_width_dlg;
	seg_width_dlg.m_w = &m_Doc->m_w;
	seg_width_dlg.m_v_w = &m_Doc->m_v_w;
	seg_width_dlg.m_v_h_w = &m_Doc->m_v_h_w;
	seg_width_dlg.m_init_w = m_Doc->m_trace_w;
	seg_width_dlg.m_init_via_w = m_Doc->m_via_w;
	seg_width_dlg.m_init_via_hole_w = m_Doc->m_via_hole_w;
	if( mode == 0 )
	{
		cseg * seg = &m_sel_seg;
		cconnect * con = &m_sel_con;
		int seg_w = seg->width;
		if( seg_w )
			seg_width_dlg.m_init_w = seg_w;
		else if( m_sel_net->def_width )
			seg_width_dlg.m_init_w = m_sel_net->def_width;
	}
	else
	{
		if( m_sel_net->def_width )
			seg_width_dlg.m_init_w = m_sel_net->def_width;
	}

	// launch dialog
	seg_width_dlg.m_mode = mode;
	int ret = seg_width_dlg.DoModal();
	int w = 0;
	int via_w = 0;
	int via_hole_w = 0;
	if( ret == IDOK )
	{
		// returned with "OK"
		CString w_str = seg_width_dlg.m_width_str;
		w = atoi((LPCSTR)w_str);
		CString via_w_str = seg_width_dlg.m_via_w_str;
		via_w = atoi((LPCSTR)via_w_str);
		CString via_hole_w_str = seg_width_dlg.m_via_hole_w_str;
		via_hole_w = atoi((LPCSTR)via_hole_w_str);
		if( w>0 && w<=1000 )
		{
			// valid width
			SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY );	// save old net info for undoing
			w = w*NM_PER_MIL;
			via_w = via_w*NM_PER_MIL;
			via_hole_w = via_hole_w*NM_PER_MIL;
			// set default values for net or connection
			if( seg_width_dlg.m_def == 2 )
			{
				// set default for net
				m_sel_net->def_width = w;
				m_sel_net->def_via_w = via_w;
				m_sel_net->def_via_hole_w = via_hole_w;
			}
			// apply new widths to net, connection or segment
			if( seg_width_dlg.m_apply == 3 )
			{
				// apply width to net
				m_Doc->m_nlist->SetNetWidth( m_sel_net, w, via_w, via_hole_w );
			}
			else if( seg_width_dlg.m_apply == 2 )
			{
				// apply width to connection
				m_Doc->m_nlist->SetConnectionWidth( m_sel_net, m_sel_ic, w, via_w, via_hole_w );
			}
			else if( seg_width_dlg.m_apply == 1 )
			{
				// apply width to segment
				m_Doc->m_nlist->SetSegmentWidth( m_sel_net, m_sel_ic,
					m_sel_id.ii, w, via_w, via_hole_w );
			}
		}
		else
		{
			// invalid width
			CString mess;
			AfxMessageBox( "illegal width" );
		}
	}
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
	return 0;
}

// Get trace and via widths
// tries default widths for net, then board
//
int CFreePcbView::GetWidthsForSegment( int * w, int * via_w, int * via_hole_w )
{
	*w = m_Doc->m_trace_w;
	if( m_sel_net->def_width )
		*w = m_sel_net->def_width;

	*via_w = m_Doc->m_via_w;
	if( m_sel_net->def_via_w )
		*via_w = m_sel_net->def_via_w;

	*via_hole_w = m_Doc->m_via_hole_w;
	if( m_sel_net->def_via_hole_w )
		*via_hole_w = m_sel_net->def_via_hole_w;

	if( *w == 0 || *via_w == 0 || *via_hole_w == 0 )
		ASSERT(0);

	return 0;
}

// context-sensitive menu invoked by right-click
//
void CFreePcbView::OnContextMenu(CWnd* pWnd, CPoint point )
{
	if( m_disable_context_menu )
	{
		// right-click already handled, don't pop up menu
		m_disable_context_menu = 0;
		return;
	}
	if( !m_Doc->m_project_open )	// no project open
		return;

	// OK, pop-up context menu
	CMenu menu;
	VERIFY(menu.LoadMenu(IDR_CONTEXT));
	CMenu* pPopup;
	int style;
	switch( m_cursor_mode )
	{
	case CUR_NONE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_NONE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_BOARD_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_BOARD_CORNER);
		ASSERT(pPopup != NULL);
		if( m_Doc->m_board_outline->GetNumCorners() < 4 )
				pPopup->EnableMenuItem( ID_BOARDCORNER_DELETECORNER, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_BOARD_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_BOARD_SIDE);
		ASSERT(pPopup != NULL);
		style = m_Doc->m_board_outline->GetSideStyle( m_sel_id.ii );
		if( style == CPolyLine::STRAIGHT )
		{
			int xi = m_Doc->m_board_outline->GetX( m_sel_id.ii );
			int yi = m_Doc->m_board_outline->GetY( m_sel_id.ii );
			int xf, yf;
			if( m_sel_id.ii != (m_Doc->m_board_outline->GetNumCorners()-1) )
			{
				xf = m_Doc->m_board_outline->GetX( m_sel_id.ii+1 );
				yf = m_Doc->m_board_outline->GetY( m_sel_id.ii+1 );
			}
			else
			{
				xf = m_Doc->m_board_outline->GetX( 0 );
				yf = m_Doc->m_board_outline->GetY( 0 );
			}
			if( xi == xf || yi == yf )
			{
				pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CW, MF_GRAYED );
				pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CCW, MF_GRAYED );
			}
			pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOSTRAIGHTLINE, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CW )
		{
			pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CW, MF_GRAYED );
			pPopup->EnableMenuItem( ID_BOARDSIDE_INSERTCORNER, MF_GRAYED );
		}
		else if( style == CPolyLine::ARC_CCW )
		{
			pPopup->EnableMenuItem( ID_BOARDSIDE_CONVERTTOARC_CCW, MF_GRAYED );
			pPopup->EnableMenuItem( ID_BOARDSIDE_INSERTCORNER, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_PART_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_PART);
		ASSERT(pPopup != NULL);
		if( m_sel_part->glued )
			pPopup->EnableMenuItem( ID_PART_GLUE, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_PART_UNGLUE, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_REF_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_REF_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_PAD_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_PAD);
		ASSERT(pPopup != NULL);
		if( m_sel_part->pin[m_sel_id.i].net )
			pPopup->EnableMenuItem( ID_PAD_ADDTONET, MF_GRAYED );		
		else
			pPopup->EnableMenuItem( ID_PAD_DETACHFROMNET, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SEG_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SEGMENT);
		ASSERT(pPopup != NULL);
		if( m_sel_con.end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		if( m_sel_con.end_pin == cconnect::NO_END
			&& m_sel_con.nsegs == (m_sel_id.ii+1) )
		{
			// last segment of stub trace
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTE, MF_GRAYED );
		}
		else
			pPopup->EnableMenuItem( ID_SEGMENT_DELETE, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_RAT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_RATLINE);
		ASSERT(pPopup != NULL);
		if( m_sel_con.locked )
			pPopup->EnableMenuItem( ID_RATLINE_LOCKCONNECTION, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_RATLINE_UNLOCKCONNECTION, MF_GRAYED );
		if( m_sel_con.end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_SEGMENT_UNROUTETRACE, MF_GRAYED );
		if( m_sel_con.nsegs == 1
			|| !(m_sel_id.ii == 0 || m_sel_id.ii == (m_sel_con.nsegs-1) ) )
			pPopup->EnableMenuItem( ID_RATLINE_CHANGEPIN, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_VERTEX);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_END_VTX_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_END_VERTEX);
		ASSERT(pPopup != NULL);
		if( m_sel_vtx.via_w )
			pPopup->EnableMenuItem( ID_ENDVERTEX_ADDVIA, MF_GRAYED );
		else
			pPopup->EnableMenuItem( ID_ENDVERTEX_REMOVEVIA, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_CONNECT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_CONNECT);
		ASSERT(pPopup != NULL);
		if( m_sel_con.end_pin == cconnect::NO_END )
			pPopup->EnableMenuItem( ID_CONNECT_UNROUTETRACE, MF_GRAYED );
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_NET_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_NET);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_TEXT_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_TEXT);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_AREA_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_AREA_CORNER);
		ASSERT(pPopup != NULL);
		{
			carea * area = &m_sel_net->area[m_sel_id.i];
			if( area->poly->GetNumCorners() < 4 )
				pPopup->EnableMenuItem( ID_AREACORNER_DELETE, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_AREA_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_AREA_EDGE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SMCUTOUT_SIDE_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SM_SIDE);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_SMCUTOUT_CORNER_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_SM_CORNER);
		ASSERT(pPopup != NULL);
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
			if( poly->GetNumCorners() < 4 )
				pPopup->EnableMenuItem( ID_SMCORNER_DELETECORNER, MF_GRAYED );
		}
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;

	case CUR_GROUP_SELECTED:
		pPopup = menu.GetSubMenu(CONTEXT_GROUP);
		ASSERT(pPopup != NULL);
		pPopup->TrackPopupMenu(TPM_LEFTALIGN | TPM_RIGHTBUTTON, point.x, point.y, pWnd );
		break;
	}
}

// add copper area
//
void CFreePcbView::OnAddArea()
{
	CDlgAddArea dlg;
	dlg.m_nlist = m_Doc->m_nlist;
	dlg.m_num_layers = m_Doc->m_num_layers;
	dlg.m_layer = m_Doc->m_active_layer;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( !dlg.m_net )
		{
			CString str;
			str.Format( "Net \"%s\" not found", dlg.m_net_name );
			AfxMessageBox( str, MB_OK );
		}
		else
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_ADD_AREA );
			m_active_layer = dlg.m_layer;
			m_sel_net = dlg.m_net;
			m_dlist->StartDragging( pDC, m_last_cursor_point.x, 
				m_last_cursor_point.y, 0, m_active_layer, 2 );
			m_polyline_style = CPolyLine::STRAIGHT;
			m_polyline_hatch = dlg.m_hatch;
			Invalidate( FALSE );
			ReleaseDC( pDC );
		}
	}
}

// add copper area cutout
//
void CFreePcbView::OnAreaAddCutout()
{
	// check if any non-straight sides
	BOOL bArcs = FALSE;
	CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
	int ns = poly->GetNumCorners();
#if 0
	for( int is=0; is<ns; is++ )
	{
		if( poly->GetSideStyle(is) != CPolyLine::STRAIGHT )
		{
			bArcs = TRUE;
			break;
		}
	}
	if( bArcs )
	{
		AfxMessageBox( "This function is unavailable if copper area\nor existing cutouts contain arcs" );
		return;
	}
#endif
	CDlgAddArea dlg;
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	SetCursorMode( CUR_ADD_AREA_CUTOUT );
	m_active_layer = dlg.m_layer;
	m_dlist->StartDragging( pDC, m_last_cursor_point.x, 
		m_last_cursor_point.y, 0, m_active_layer, 2 );
	m_polyline_style = CPolyLine::STRAIGHT;
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::OnAreaDeleteCutout()
{
	CPolyLine * poly = m_sel_net->area[m_sel_ia].poly;
	int icont = poly->GetContour( m_sel_id.ii );
	if( icont < 1 )
		ASSERT(0);
	SaveUndoInfoForArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY );
	poly->RemoveContour( icont );
	CancelSelection();
	Invalidate( FALSE );
}

// move part
//
void CFreePcbView::OnPartMove()
{
	// check for glue
	if( m_sel_part->glued )
	{
		int ret = AfxMessageBox( "This part is glued, do you want to unglue it ?  ", MB_YESNO ); 
		if( ret != IDYES )
			return;
	}
	// drag part
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint p;
	p.x  = m_sel_part->x;
	p.y  = m_sel_part->y;
	m_from_pt = p;
	CPoint cur_p = PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start dragging
	m_Doc->m_plist->StartDraggingPart( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_PART );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

// add text string
//
void CFreePcbView::OnTextAdd()
{
	// create, initialize and show dialog
	CDlgAddText add_text_dlg;
	add_text_dlg.m_num_layers = m_Doc->m_num_layers;
	add_text_dlg.m_drag_flag = 1;
	// defaults for dialog
	CString str = "";
	add_text_dlg.m_units = m_Doc->m_units;
	add_text_dlg.m_str = &str;
	add_text_dlg.m_layer = LAY_SILK_TOP;
	add_text_dlg.m_mirror = 0;
	add_text_dlg.m_angle = 0;
	add_text_dlg.m_height = 0;	// this will force default in dialog
	add_text_dlg.m_width = 0;	// this will force default in dialog
	add_text_dlg.m_x = 0;
	add_text_dlg.m_y = 0;
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;
	int x = add_text_dlg.m_x;
	int y = add_text_dlg.m_y;
	int mirror = add_text_dlg.m_mirror;
	int angle = add_text_dlg.m_angle;
	int font_size = add_text_dlg.m_height;
	int stroke_width = add_text_dlg.m_width;
	int layer = add_text_dlg.m_layer;

	// get cursor position and convert to PCB coords
	CPoint p;
	GetCursorPos( &p );		// cursor pos in screen coords
	p = ScreenToPCB( p );	// convert to PCB coords
	// set pDC to PCB coords
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	if( add_text_dlg.m_drag_flag )
	{
		m_sel_text = m_Doc->m_tlist->AddText( p.x, p.y, angle, mirror, 
			layer, font_size, stroke_width, &str );
		m_dragging_new_item = 1;
		m_Doc->m_tlist->StartDraggingText( pDC, m_sel_text );
		SetCursorMode( CUR_DRAG_TEXT );
	}
	else
	{
		m_sel_text = m_Doc->m_tlist->AddText( x, y, angle, mirror, 
			layer, font_size,  stroke_width, &str );
		SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_ADD );
		m_Doc->m_tlist->HighlightText( m_sel_text );
	}
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete text ... enter with text selected
//
void CFreePcbView::OnTextDelete()
{
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_DELETE );
	m_Doc->m_tlist->RemoveText( m_sel_text );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move text, enter with text selected
//
void CFreePcbView::OnTextMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to text origin
	CPoint p;
	p.x  = m_sel_text->m_x;
	p.y  = m_sel_text->m_y;
	CPoint cur_p = PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	// start moving
	m_dlist->CancelHighLight();
	m_dragging_new_item = 0;
	m_Doc->m_tlist->StartDraggingText( pDC, m_sel_text );
	SetCursorMode( CUR_DRAG_TEXT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// glue part
//
void CFreePcbView::OnPartGlue()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY );
	m_sel_part->glued = 1;
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}

// unglue part
//
void CFreePcbView::OnPartUnglue()
{
	SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY );
	m_sel_part->glued = 0;
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}

// delete part
//
void CFreePcbView::OnPartDelete()
{
	// delete part 
	CString mess;
	mess.Format( "Deleting part %s\nDo you wish to remove all references\nto this part from netlist ?",
		m_sel_part->ref_des );
	int ret = AfxMessageBox( mess, MB_YESNOCANCEL );
	if( ret == IDCANCEL )
		return;
	// save undo info
	SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_DELETE );
	// now do it
	if( ret == IDYES )
		m_Doc->m_nlist->PartDeleted( m_sel_part );
	else if( ret == IDNO )
		m_Doc->m_nlist->PartDisconnected( m_sel_part );
	m_Doc->m_plist->Remove( m_sel_part );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// optimize all nets to part
//
void CFreePcbView::OnPartOptimize()
{
	SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move ref. designator text for part
//
void CFreePcbView::OnRefMove()
{
	// move reference ID
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	// move cursor to part origin
	CPoint cur_p = PCBToScreen( m_last_cursor_point );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dragging_new_item = 0;
	m_Doc->m_plist->StartDraggingRefText( pDC, m_sel_part );
	SetCursorMode( CUR_DRAG_REF );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// optimize net for this pad
//
void CFreePcbView::OnPadOptimize()
{
	cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
	if( pin_net )
	{
		m_Doc->m_nlist->OptimizeConnections( pin_net );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// start stub trace from this pad
//
void CFreePcbView::OnPadStartStubTrace()
{
	cnet * net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
	if( net == NULL )
	{
		AfxMessageBox( "Pad must be assigned to a net before adding trace", MB_OK );
		return;
	}
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint pi = m_last_cursor_point;
	CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
	CPoint p = m_Doc->m_plist->GetPinPoint( m_sel_part, &pin_name );

	// force to layer of pad if SMT
	if( m_sel_part->shape->m_padstack[m_sel_id.i].hole_size == 0 )
	{
		if( m_sel_part->side )
			m_active_layer = LAY_BOTTOM_COPPER;
		else
			m_active_layer = LAY_TOP_COPPER;
		ShowActiveLayer();
	}

	// find starting pin in net
	int p1 = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].part == m_sel_part )
		{
			if( net->pin[ip].pin_name == m_sel_part->shape->GetPinNameByIndex( m_sel_id.i ) )
			{
				// found starting pin in net
				p1 = ip;
			}
		}
	}
	if( p1 == -1 )
		ASSERT(0);		// starting pin not found in net

	// add connection for stub trace
	m_sel_net = net;
	m_sel_id.Set( ID_NET, ID_CONNECT, 0, ID_SEL_SEG, 0 );
	m_sel_id.i = m_Doc->m_nlist->AddNetStub( net, p1 );  

	// start dragging line
	int w = m_Doc->m_trace_w;
	if( net->def_width )
		w = net->def_width;
	int via_w = m_Doc->m_via_w;
	if( net->def_via_w )
		via_w = net->def_via_w;
	int via_hole_w = m_Doc->m_via_hole_w;
	if( net->def_via_hole_w )
		via_hole_w = net->def_via_hole_w;
	m_Doc->m_nlist->StartDraggingStub( pDC, net, m_sel_id.i, m_sel_id.ii, 
		pi.x, pi.y, m_active_layer, w, m_active_layer, via_w, via_hole_w, 2 );  
	m_snap_angle_ref = p;
	SetCursorMode( CUR_DRAG_STUB );
	m_dlist->CancelHighLight();
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// attach this pad to a net
//
void CFreePcbView::OnPadAddToNet()
{
	DlgAssignNet assign_net_dlg;
	assign_net_dlg.m_map = &m_Doc->m_nlist->m_map;
	int ret = assign_net_dlg.DoModal();
	if( ret == IDOK )
	{
		CString name = assign_net_dlg.m_net_str;
		void * ptr;
		cnet * new_net = 0;
		int test = m_Doc->m_nlist->m_map.Lookup( name, ptr );
		if( !test )
		{
			// create new net if legal string
			name.Trim();
			if( name.GetLength() )
			{
				new_net = m_Doc->m_nlist->AddNet( (char*)(LPCTSTR)name, 10, 0, 0, 0 );
				SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_ADD );
			}
			else
			{
				// blank net name
				AfxMessageBox( "Illegal net name" );
				return;
			}
		}
		else
		{
			// use selected net
			new_net = (cnet*)ptr;
			SaveUndoInfoForNetAndConnections( new_net, CNetList::UNDO_NET_MODIFY );
		}
		// assign pin to net
		if( new_net )
		{
			SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY, FALSE );
			CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
			m_Doc->m_nlist->AddNetPin( new_net,
				&m_sel_part->ref_des,
				&pin_name );
			m_Doc->m_nlist->OptimizeConnections( new_net );
			SetFKText( m_cursor_mode );
		}
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// remove this pad from net
//
void CFreePcbView::OnPadDetachFromNet()
{
	cnet * pin_net = (cnet*)m_sel_part->pin[m_sel_id.i].net;
	SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY );
	CString pin_name = m_sel_part->shape->GetPinNameByIndex(m_sel_id.i); 
	m_Doc->m_nlist->RemoveNetPin( m_sel_part, &pin_name );
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// connect this pad to another pad
//
void CFreePcbView::OnPadConnectToPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CString pin_name = m_sel_part->shape->GetPinNameByIndex( m_sel_id.i );
	CPoint p = m_Doc->m_plist->GetPinPoint( m_sel_part, &pin_name );
	m_dragging_new_item = 0;
	m_dlist->StartDraggingRatLine( pDC, 0, 0, p.x, p.y, LAY_RAT_LINE, 1, 1 );  
	SetCursorMode( CUR_DRAG_CONNECT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// set width for this segment (not a ratline)
//
void CFreePcbView::OnSegmentSetWidth()
{
	SetWidth( 0 );
	m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, m_sel_is );
	Invalidate( FALSE );
}

// unroute this segment, convert to a ratline
//
void CFreePcbView::OnSegmentUnroute()
{
	// save undo info for connection
	SaveUndoInfoForConnection( m_sel_net, m_sel_ic );

	// edit connection segment
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	// see if segments to pin also need to be unrouted
	// see if start vertex of this segment is in start pad of connection
	int x = m_sel_vtx.x;
	int y = m_sel_vtx.y;
	int layer = m_sel_seg.layer;
	BOOL test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
		m_sel_id.i, layer, 1 );
	if( test )
	{
		// unroute preceding segments
		for( int is=m_sel_id.ii-1; is>=0; is-- )
			m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, is );	}
	// see if end vertex of this segment is in end pad of connection
	x = m_sel_next_vtx.x;
	y = m_sel_next_vtx.y;
	test = m_Doc->m_nlist->TestHitOnConnectionEndPad( x, y, m_sel_net,
		m_sel_id.i, layer, 0 );
	if( test )
	{
		// unroute following segments
		for( int is=m_sel_con.nsegs-1; is>m_sel_id.ii; is-- )
			m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, is );	
	}

	id id = m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, m_sel_is );
	CancelSelection();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, id.i, id.ii );
	m_sel_id = id;
	SetCursorMode( CUR_RAT_SELECTED );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// delete this segment, currently only used for the last segment of a stub trace
//
void CFreePcbView::OnSegmentDelete()
{
	// save undo info for connection
	SaveUndoInfoForConnection( m_sel_net, m_sel_ic );

	m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is );
	m_Doc->m_nlist->SetAreaConnections( m_sel_net );
	CancelSelection();
	ShowSelectStatus();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// route this ratline
//
void CFreePcbView::OnRatlineRoute()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	int last_seg_layer = 0;
	int n_segs = m_sel_con.nsegs;
	// get direction for routing, based on closest end of selected segment to cursor
	double d1x = p.x - m_sel_vtx.x;
	double d1y = p.y - m_sel_vtx.y;
	double d2x = p.x - m_sel_next_vtx.x;
	double d2y = p.y - m_sel_next_vtx.y;
	double d1 = d1x*d1x + d1y*d1y;
	double d2 = d2x*d2x + d2y*d2y;
	if( d1<d2 )
	{
		// route forward
		m_dir = 0;
		if( m_sel_id.ii > 0 )
			last_seg_layer = m_sel_con.seg[m_sel_id.ii-1].layer;
		m_snap_angle_ref.x = m_sel_vtx.x;
		m_snap_angle_ref.y = m_sel_vtx.y;
	}
	else
	{
		// route backward
		m_dir = 1;
		if( m_sel_id.ii < (m_sel_con.nsegs-1) )
			last_seg_layer = m_sel_con.seg[m_sel_id.ii+1].layer;
		m_snap_angle_ref.x = m_sel_next_vtx.x;
		m_snap_angle_ref.y = m_sel_next_vtx.y;
	}
	if( m_sel_id.ii == 0 && m_dir == 0)
	{
		// first segment, force to layer of starting pad if SMT
		int p1 = m_sel_con.start_pin;
		cpart * p = m_sel_net->pin[p1].part;
		CString pin_name = m_sel_net->pin[p1].pin_name;
		int pin_index = p->shape->GetPinIndexByName( &pin_name );
		if( p->shape->m_padstack[pin_index].hole_size == 0)
		{
			m_active_layer = m_Doc->m_plist->GetPinLayer( p, &pin_name );
			ShowActiveLayer();
		}
	}
	else if( m_sel_id.ii == (n_segs-1) && m_dir == 1 )
	{
		// last segment, force to layer of starting pad if SMT
		int p1 = m_sel_con.end_pin;
		cpart * p = m_sel_net->pin[p1].part;
		CString pin_name = m_sel_net->pin[p1].pin_name;
		int pin_index = p->shape->GetPinIndexByName( &pin_name );
		if( p1 != cconnect::NO_END )
		{
			if( p->shape->m_padstack[pin_index].hole_size == 0)
			{
				m_active_layer = m_Doc->m_plist->GetPinLayer( p, &pin_name );
				ShowActiveLayer();
			}
		}
	}
	// now start dragging new segment
	int w = m_Doc->m_trace_w;
	int via_w = m_Doc->m_via_w;
	int via_hole_w = m_Doc->m_via_hole_w;
	GetWidthsForSegment( &w, &via_w, &via_hole_w ); 
	m_dragging_new_item = 0;
	m_Doc->m_nlist->StartDraggingSegment( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, m_active_layer, 
		LAY_SELECTION, w,
		last_seg_layer, via_w, via_hole_w, m_dir, 2 );
	SetCursorMode( CUR_DRAG_RAT );
	ReleaseDC( pDC );
}

// optimize this connection
//
void CFreePcbView::OnRatlineOptimize()
{
	m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// optimize this connection
//
void CFreePcbView::OnRatlineChangeEndPin()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	m_dlist->CancelHighLight();
	cconnect * c = &m_sel_con;
	m_dlist->Set_visible( c->seg[m_sel_id.ii].dl_el, FALSE );
	int x, y;
	if( m_sel_id.ii == 0 )
	{
		// ratline is first segment of connection
		x = c->vtx[1].x;
		y = c->vtx[1].y;
	}
	else
	{
		// ratline is last segment of connection
		x = c->vtx[m_sel_id.ii].x;
		y = c->vtx[m_sel_id.ii].y;
	}
	m_dlist->StartDraggingRatLine( pDC, 0, 0, x, y, LAY_RAT_LINE, 1, 1 );  
	SetCursorMode( CUR_DRAG_RAT_PIN );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// move this vertex
//
void CFreePcbView::OnVertexMove()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	id id = m_sel_id;
	int ic = m_sel_id.i;
	int ivtx = m_sel_id.ii;
	m_dragging_new_item = 0;
	m_from_pt.x = m_sel_vtx.x;
	m_from_pt.y = m_sel_vtx.y;
	m_Doc->m_nlist->StartDraggingVertex( pDC, m_sel_net, ic, ivtx, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_VTX );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete this vertex, unroute adjacent segments
//
void CFreePcbView::OnVertexDelete()
{
	int ic = m_sel_id.i;
	int iv = m_sel_id.ii;
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	SaveUndoInfoForConnection( m_sel_net, ic );
	if( m_sel_net->connect[ic].end_pin == cconnect::NO_END
		&& iv == (m_sel_net->connect[ic].nsegs-1) )
	{
		// deleting next-to-last vertex of stub trace, just delete last 2 segments
		m_Doc->m_nlist->UnrouteSegment( m_sel_net, ic, iv-1 );
		m_Doc->m_nlist->RemoveSegment( m_sel_net, ic, iv );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net );
		CancelSelection();
	}
	else
	{
		// unroute both adjacent segments
		if( m_sel_net->connect[ic].seg[iv].layer != LAY_RAT_LINE )
			m_sel_id = m_Doc->m_nlist->UnrouteSegment( m_sel_net, ic, iv );
		if( m_sel_net->connect[ic].seg[iv-1].layer != LAY_RAT_LINE )
			m_sel_id = m_Doc->m_nlist->UnrouteSegment( m_sel_net, ic, iv-1 );
		CancelSelection();
		m_Doc->m_nlist->HighlightSegment( m_sel_net, ic, iv-1 );
		SetCursorMode( CUR_RAT_SELECTED );
	}
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// move the end vertex of a stub trace
//
void CFreePcbView::OnEndVertexMove()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	SetCursorMode( CUR_DRAG_END_VTX );
	m_Doc->m_nlist->StartDraggingEndVertex( pDC, m_sel_net, m_sel_ic, m_sel_is, 2 );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}


// force a via on end vertex
//
void CFreePcbView::OnEndVertexAddVia()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
//	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY );
	SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
	m_Doc->m_nlist->ForceVia( m_sel_net, m_sel_ic, m_sel_is );
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	Invalidate( FALSE );
}

// remove forced via on end vertex
//
void CFreePcbView::OnEndVertexRemoveVia()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
//	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY );
	SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
	m_Doc->m_nlist->UnforceVia( m_sel_net, m_sel_ic, m_sel_is );
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	Invalidate( FALSE );
}

// append more segments to this stub trace
//
void CFreePcbView::OnEndVertexAddSegments()
{
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	CDC * pDC = GetDC();
	SetDCToWorldCoords( pDC );
	pDC->SelectClipRgn( &m_pcb_rgn );
	CPoint p;
	p = m_last_cursor_point;
	m_sel_id.sst = ID_SEL_SEG;
	int w, via_w, via_hole_w;
	m_snap_angle_ref.x = m_sel_vtx.x;
	m_snap_angle_ref.y = m_sel_vtx.y;
	GetWidthsForSegment( &w, &via_w, &via_hole_w ); 
	m_Doc->m_nlist->StartDraggingStub( pDC, m_sel_net, m_sel_ic, m_sel_is,
		p.x, p.y, m_active_layer, w, m_active_layer, via_w, via_hole_w, 2 );
	SetCursorMode( CUR_DRAG_STUB );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// convert stub trace to regular connection by adding ratline to a pad
//
void CFreePcbView::OnEndVertexAddConnection()
{
	// TODO: Add your command handler code here
}

// end vertex selected, delete it and the adjacent segment
//
void CFreePcbView::OnEndVertexDelete()
{
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY );
	m_Doc->m_nlist->SetNetVisibility( m_sel_net, TRUE );
	m_Doc->m_nlist->RemoveSegment( m_sel_net, m_sel_ic, m_sel_is-1 );
	CancelSelection();
	m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// edit the position of an end vertex
//
void CFreePcbView::OnEndVertexEdit()
{
	DlgEditBoardCorner dlg;
	CString str = "Edit End Vertex Position";
	int x = m_sel_vtx.x;
	int y = m_sel_vtx.y;
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
		m_Doc->m_nlist->MoveEndVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.GetX(), dlg.GetY() );
		m_Doc->ProjectModified( TRUE );
		m_Doc->m_nlist->SelectVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
}

// finish routing a connection by making a segment to the destination pad
//
void CFreePcbView::OnRatlineComplete()
{
	SaveUndoInfoForConnection( m_sel_net, m_sel_ic );

	// complete routing to pin
	int w = m_Doc->m_trace_w;
	int via_w = m_Doc->m_via_w;
	int via_hole_w = m_Doc->m_via_hole_w;
	GetWidthsForSegment( &w, &via_w, &via_hole_w ); 
	int test = m_Doc->m_nlist->RouteSegment( m_sel_net, m_sel_ic, m_sel_is, m_active_layer, 
		w, via_w, via_hole_w );
	if( !test )
	{
		CancelSelection();
		Invalidate( FALSE );
	}
	else
	{
		// didn't work
		PlaySound( TEXT("CriticalStop"), 0, 0 );
		// TODO: eliminate undo
	}
	m_Doc->ProjectModified( TRUE );
}

// set width of a connection
//
void CFreePcbView::OnRatlineSetWidth()
{
	if( m_sel_con.nsegs == 1 )
		SetWidth( 2 );
	else
		SetWidth( 1 );
	Invalidate( FALSE ); 
}

// delete a connection
//
void CFreePcbView::OnRatlineDeleteConnection()
{
	if( m_sel_con.locked )
	{
		int ret = AfxMessageBox( "You are trying to delete a locked connection.\nAre you sure ? ",
			MB_YESNO );
		if( ret == IDNO )
			return;
	}
	SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY );
	m_Doc->m_nlist->RemoveNetConnect( m_sel_net, m_sel_ic );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// lock a connection
//
void CFreePcbView::OnRatlineLockConnection()
{
	SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
	m_sel_con.locked = 1;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
}

// unlock a connection
//
void CFreePcbView::OnRatlineUnlockConnection()
{
	SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
	m_sel_con.locked = 0;
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	Invalidate( FALSE );
	m_Doc->ProjectModified( TRUE );
}

// edit a text string
//
void CFreePcbView::OnTextEdit()
{
	// create dialog and pass parameters
	CDlgAddText add_text_dlg;
	add_text_dlg.m_units = m_Doc->m_units;
	add_text_dlg.m_num_layers = m_Doc->m_num_layers;
	CString test_str = m_sel_text->m_str;
	add_text_dlg.m_str = &test_str;
	add_text_dlg.m_mirror = m_sel_text->m_mirror;
	add_text_dlg.m_angle = m_sel_text->m_angle;
	add_text_dlg.m_height = m_sel_text->m_font_size;
	add_text_dlg.m_width = m_sel_text->m_stroke_width;
	add_text_dlg.m_x = m_sel_text->m_x;
	add_text_dlg.m_y = m_sel_text->m_y;
	add_text_dlg.m_layer = m_sel_text->m_layer;
	add_text_dlg.m_drag_flag = 0;
	int ret = add_text_dlg.DoModal();
	if( ret == IDCANCEL )
		return;

	// now replace old text with new one
	SaveUndoInfoForText( m_sel_text, CTextList::UNDO_TEXT_MODIFY );
	int x = add_text_dlg.m_x;
	int y = add_text_dlg.m_y;
	int mirror = add_text_dlg.m_mirror;
	int angle = add_text_dlg.m_angle;
	int font_size = add_text_dlg.m_height;
	int stroke_width = add_text_dlg.m_width;
	int layer = add_text_dlg.m_layer;
	m_dlist->CancelHighLight();
	CText * new_text = m_Doc->m_tlist->AddText( x, y, angle, mirror, layer, font_size,
		stroke_width, &test_str );
	m_Doc->m_tlist->RemoveText( m_sel_text );
	m_sel_text = new_text;
	m_Doc->m_tlist->HighlightText( m_sel_text );

	// start dragging if requested in dialog
	if( add_text_dlg.m_drag_flag )
		OnTextMove();
	else
		Invalidate( FALSE );
	m_Doc->ProjectModified( TRUE );
}

// start adding board outline by dragging line for first side
//
void CFreePcbView::OnAddBoardOutline()
{
	if( m_Doc->m_board_outline )
	{
		int ret = AfxMessageBox( "Board outline already exists\nDelete it ? ", MB_OKCANCEL );
		if( ret != IDOK )
			return;
		delete m_Doc->m_board_outline;
		m_Doc->m_board_outline = NULL;
	}
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_dlist->CancelHighLight();
	m_sel_id.Set( ID_BOARD, ID_BOARD_OUTLINE, 0, ID_SEL_CORNER, 0 );
	m_polyline_style = CPolyLine::STRAIGHT;
	m_dlist->StartDragging( pDC, p.x, p.y, 0, LAY_BOARD_OUTLINE, 2 );
	SetCursorMode( CUR_ADD_BOARD );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// move a board outline corner
//
void CFreePcbView::OnBoardCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = m_Doc->m_board_outline->GetX( m_sel_id.ii );
	m_from_pt.y = m_Doc->m_board_outline->GetY( m_sel_id.ii );
	m_Doc->m_board_outline->StartDraggingToMoveCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_BOARD_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// edit a board outline corner
//
void CFreePcbView::OnBoardCornerEdit()
{
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	int x = m_Doc->m_board_outline->GetX(m_sel_id.ii);
	int y = m_Doc->m_board_outline->GetY(m_sel_id.ii);
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForBoardOutline( UNDO_BOARD_MODIFY );
		m_Doc->m_board_outline->MoveCorner( m_sel_id.ii, 
			dlg.GetX(), dlg.GetY() );
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

// delete a board corner
//
void CFreePcbView::OnBoardCornerDelete()
{
	if( m_Doc->m_board_outline->GetNumCorners() < 4 )
	{
		AfxMessageBox( "Board outline has too few corners" );
		return;
	}
	SaveUndoInfoForBoardOutline( UNDO_BOARD_MODIFY );
	m_Doc->m_board_outline->DeleteCorner( m_sel_id.ii );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// insert a new corner in a side of a board outline
//
void CFreePcbView::OnBoardSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_Doc->m_board_outline->StartDraggingToInsertCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_BOARD_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete entire board outline
//
void CFreePcbView::OnBoardDeleteOutline()
{
	if( m_Doc->m_board_outline->GetClosed() )
		SaveUndoInfoForBoardOutline( UNDO_BOARD_DELETE );
	delete m_Doc->m_board_outline;
	m_Doc->m_board_outline = 0;
	m_Doc->ProjectModified( TRUE );
	CancelSelection();
	Invalidate( FALSE );
}

// move a copper area corner
//
void CFreePcbView::OnAreaCornerMove()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = m_sel_net->area[m_sel_id.i].poly->GetX( m_sel_id.ii );
	m_from_pt.y = m_sel_net->area[m_sel_id.i].poly->GetY( m_sel_id.ii );
	m_Doc->m_nlist->StartDraggingAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete a copper area corner
//
void CFreePcbView::OnAreaCornerDelete()
{
	carea * area;
	area = &m_sel_net->area[m_sel_id.i];
	if( area->poly->GetNumCorners() > 3 )
	{
		SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY );
		area->poly->DeleteCorner( m_sel_id.ii );
		m_dlist->CancelHighLight();
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		m_Doc->ProjectModified( TRUE );
		SetCursorMode( CUR_NONE_SELECTED );
		Invalidate( FALSE );
	}
	else
		OnAreaCornerDeleteArea();
}

// delete entire area
//
void CFreePcbView::OnAreaCornerDeleteArea()
{
	OnAreaSideDeleteArea();
	m_Doc->ProjectModified( TRUE );
}

//insert a new corner in a side of a copper area
//
void CFreePcbView::OnAreaSideAddCorner()
{
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_Doc->m_nlist->StartDraggingInsertedAreaCorner( pDC, m_sel_net, m_sel_ia, m_sel_is, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_AREA_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

// delete entire area
//
void CFreePcbView::OnAreaSideDeleteArea()
{
	SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_DELETE );
	m_Doc->m_nlist->RemoveArea( m_sel_net, m_sel_ia );
	m_Doc->m_nlist->OptimizeConnections( m_sel_net );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// detect state where nothing is selected or being dragged
//
BOOL CFreePcbView::CurNone()
{
	return( m_cursor_mode == CUR_NONE_SELECTED );
}

// detect any selected state
//
BOOL CFreePcbView::CurSelected()
{	
	return( m_cursor_mode > CUR_NONE_SELECTED && m_cursor_mode < CUR_NUM_SELECTED_MODES );
}

// detect any dragging state
//
BOOL CFreePcbView::CurDragging()
{
	return( m_cursor_mode > CUR_NUM_SELECTED_MODES && m_cursor_mode < CUR_NUM_MODES );	
}

// detect states using routing grid
//
BOOL CFreePcbView::CurDraggingRouting()
{
	return( m_cursor_mode == CUR_DRAG_RAT
		|| m_cursor_mode == CUR_DRAG_VTX
		|| m_cursor_mode == CUR_DRAG_END_VTX
		|| m_cursor_mode == CUR_ADD_AREA
		|| m_cursor_mode == CUR_DRAG_AREA_1
		|| m_cursor_mode == CUR_DRAG_AREA
		|| m_cursor_mode == CUR_DRAG_AREA_INSERT
		|| m_cursor_mode == CUR_DRAG_AREA_MOVE
		|| m_cursor_mode == CUR_ADD_AREA_CUTOUT
		|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
		|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT
		|| m_cursor_mode == CUR_ADD_SMCUTOUT
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT_1
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT_INSERT
		|| m_cursor_mode == CUR_DRAG_SMCUTOUT_MOVE
		|| m_cursor_mode == CUR_DRAG_STUB );
}

// detect states using placement grid
//
BOOL CFreePcbView::CurDraggingPlacement()
{
	return( m_cursor_mode == CUR_ADD_BOARD
		|| m_cursor_mode == CUR_DRAG_BOARD_1
		|| m_cursor_mode == CUR_DRAG_BOARD
		|| m_cursor_mode == CUR_DRAG_BOARD_INSERT
		|| m_cursor_mode == CUR_DRAG_BOARD_MOVE
		|| m_cursor_mode == CUR_DRAG_PART
		|| m_cursor_mode == CUR_DRAG_REF
		|| m_cursor_mode == CUR_DRAG_TEXT
		|| m_cursor_mode == CUR_DRAG_GROUP
		|| m_cursor_mode == CUR_MOVE_ORIGIN );
}

// snap cursor if required and set m_last_cursor_point
//
void CFreePcbView::SnapCursorPoint( CPoint wp )
{
	if( CurDraggingPlacement() || CurDraggingRouting() )
	{	
		int grid_spacing;
		if( CurDraggingPlacement() )
		{
			grid_spacing = m_Doc->m_part_grid_spacing;
		}
		else
		{
			grid_spacing = m_Doc->m_routing_grid_spacing;
		}

		// snap angle if needed
		if( m_Doc->m_snap_angle && (wp != m_snap_angle_ref) 
			&& ( m_cursor_mode == CUR_DRAG_RAT 
			|| m_cursor_mode == CUR_DRAG_STUB 
			|| m_cursor_mode == CUR_DRAG_AREA_1
			|| m_cursor_mode == CUR_DRAG_AREA
			|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT_1
			|| m_cursor_mode == CUR_DRAG_AREA_CUTOUT
			|| m_cursor_mode == CUR_DRAG_BOARD_1
			|| m_cursor_mode == CUR_DRAG_BOARD ) )
		{
			// snap to angle only if the starting point is on-grid
			double ddx = fmod( (double)(m_snap_angle_ref.x), grid_spacing );
			double ddy = fmod( (double)(m_snap_angle_ref.y), grid_spacing );
			if( fabs(ddx) < 0.5 && fabs(ddy) < 0.5 )
			{
				// starting point is on-grid, snap to angle
				// snap to n*45 degree angle
				const double pi = 3.14159265359;		
				double dx = wp.x - m_snap_angle_ref.x;
				double dy = wp.y - m_snap_angle_ref.y;
				double dist = sqrt( dx*dx + dy*dy );
				double dist45 = dist/sqrt(2.0);
				{
					int d;
					d = (int)(dist/grid_spacing+0.5);
					dist = d*grid_spacing;
					d = (int)(dist45/grid_spacing+0.5);
					dist45 = d*grid_spacing;
				}
				if( m_Doc->m_snap_angle == 45 )
				{
					// snap angle = 45 degrees, divide circle into 8 octants
					double angle = atan2( dy, dx );
					if( angle < 0.0 )
						angle = 2.0*pi + angle;
					angle += pi/8.0;
					double d_quad = angle/(pi/4.0);
					int oct = d_quad;
					switch( oct )
					{
					case 0:
						wp.x = m_snap_angle_ref.x + dist;
						wp.y = m_snap_angle_ref.y;
						break;
					case 1:
						wp.x = m_snap_angle_ref.x + dist45;
						wp.y = m_snap_angle_ref.y + dist45;
						break;
					case 2:
						wp.x = m_snap_angle_ref.x;
						wp.y = m_snap_angle_ref.y + dist;
						break;
					case 3:
						wp.x = m_snap_angle_ref.x - dist45;
						wp.y = m_snap_angle_ref.y + dist45;
						break;
					case 4:
						wp.x = m_snap_angle_ref.x - dist;
						wp.y = m_snap_angle_ref.y;
						break;
					case 5:
						wp.x = m_snap_angle_ref.x - dist45;
						wp.y = m_snap_angle_ref.y - dist45;
						break;
					case 6:
						wp.x = m_snap_angle_ref.x;
						wp.y = m_snap_angle_ref.y - dist;
						break;
					case 7:
						wp.x = m_snap_angle_ref.x + dist45;
						wp.y = m_snap_angle_ref.y - dist45;
						break;
					case 8:
						wp.x = m_snap_angle_ref.x + dist;
						wp.y = m_snap_angle_ref.y;
						break;
					default:
						ASSERT(0);
						break;
					}
				}
				else
				{
					// snap angle is 90 degrees, divide into 4 quadrants
					double angle = atan2( dy, dx );
					if( angle < 0.0 )
						angle = 2.0*pi + angle;
					angle += pi/4.0;
					double d_quad = angle/(pi/2.0);
					int quad = d_quad;
					switch( quad )
					{
					case 0:
						wp.x = m_snap_angle_ref.x + dist;
						wp.y = m_snap_angle_ref.y;
						break;
					case 1:
						wp.x = m_snap_angle_ref.x;
						wp.y = m_snap_angle_ref.y + dist;
						break;
					case 2:
						wp.x = m_snap_angle_ref.x - dist;
						wp.y = m_snap_angle_ref.y;
						break;
					case 3:
						wp.x = m_snap_angle_ref.x;
						wp.y = m_snap_angle_ref.y - dist;
						break;
					case 4:
						wp.x = m_snap_angle_ref.x + dist;
						wp.y = m_snap_angle_ref.y;
						break;
					default:
						ASSERT(0);
						break;
					}
				}
			}
		}
		// snap to grid
		// get position in integral units of grid_spacing
		if( wp.x > 0 )
			wp.x = (wp.x + grid_spacing/2)/grid_spacing;
		else
			wp.x = (wp.x - grid_spacing/2)/grid_spacing;
		if( wp.y > 0 )
			wp.y = (wp.y + grid_spacing/2)/grid_spacing;
		else
			wp.y = (wp.y - grid_spacing/2)/grid_spacing;
		// then multiply by grid spacing, adding or subracting 0.5 to prevent round-off
		// when using a fractional grid
		double test = wp.x * grid_spacing;
		if( test > 0.0 )
			test += 0.5;
		else
			test -= 0.5;
		wp.x = test;
		test = wp.y * grid_spacing;
		if( test > 0.0 )
			test += 0.5;
		else
			test -= 0.5;
		wp.y = test;
	}
	if( CurDragging() )
	{
		// update drag operation
		if( wp != m_last_cursor_point )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->Drag( pDC, wp.x, wp.y );
			ReleaseDC( pDC );
		}
	}
	// update cursor position
	m_last_cursor_point = wp;
	ShowCursor();
	// if dragging, show relative distance
	if( m_cursor_mode == CUR_DRAG_GROUP || m_cursor_mode == CUR_DRAG_PART
		|| m_cursor_mode == CUR_DRAG_VTX || m_cursor_mode ==  CUR_DRAG_BOARD_MOVE 
		|| m_cursor_mode == CUR_DRAG_AREA_MOVE || m_cursor_mode ==  CUR_DRAG_SMCUTOUT_MOVE 
		)
	{
		ShowRelativeDistance( wp.x - m_from_pt.x, wp.y - m_from_pt.y );
	}
}

LONG CFreePcbView::OnChangeVisibleGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_visual_grid_spacing = fabs( m_Doc->m_visible_grid[lp] );
	else
		ASSERT(0);
	m_dlist->SetVisibleGrid( TRUE, m_Doc->m_visual_grid_spacing );
	Invalidate( FALSE );
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangePlacementGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_part_grid_spacing = fabs( m_Doc->m_part_grid[lp] );
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangeRoutingGrid( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
		m_Doc->m_routing_grid_spacing = fabs( m_Doc->m_routing_grid[lp] );
	else
		ASSERT(0);
	SetFocus();
	m_Doc->ProjectModified( TRUE );
	return 0;
}

LONG CFreePcbView::OnChangeSnapAngle( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
			m_Doc->m_snap_angle = 45;
		else if( lp == 1 )
			m_Doc->m_snap_angle = 90;
		else
			m_Doc->m_snap_angle = 0;
	}
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}

LONG CFreePcbView::OnChangeUnits( UINT wp, LONG lp )
{
	if( wp == WM_BY_INDEX )
	{
		if( lp == 0 )
			m_Doc->m_units = MIL;
		else
			m_Doc->m_units = MM;
	}
	else
		ASSERT(0);
	m_Doc->ProjectModified( TRUE );
	SetFocus();
	return 0;
}


void CFreePcbView::OnSegmentDeleteTrace()
{
	OnRatlineDeleteConnection();
}

void CFreePcbView::OnAreaCornerProperties()
{
	// reuse board corner dialog
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	CPoint pt = m_Doc->m_nlist->GetAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
	dlg.Init( &str, m_Doc->m_units, pt.x, pt.y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForNetAndConnectionsAndArea( m_sel_net, m_sel_ia, CNetList::UNDO_AREA_MODIFY );
		m_Doc->m_nlist->MoveAreaCorner( m_sel_net, m_sel_ia, m_sel_is, 
			dlg.GetX(), dlg.GetY() );
		m_Doc->m_nlist->SetAreaConnections( m_sel_net, m_sel_ia );
		m_Doc->m_nlist->OptimizeConnections( m_sel_net );
		m_Doc->m_nlist->HighlightAreaCorner( m_sel_net, m_sel_ia, m_sel_is );
		SetCursorMode( CUR_AREA_CORNER_SELECTED );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnRefProperties()
{
	CDlgRefText dlg;
	dlg.Initialize( m_Doc->m_plist, m_sel_part, &m_Doc->m_footprint_cache_map );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		// edit this part
		SaveUndoInfoForPart( m_sel_part, CPartList::UNDO_PART_MODIFY );
		m_Doc->m_plist->ResizeRefText( m_sel_part, dlg.m_height, dlg.m_width );
		m_Doc->ProjectModified( TRUE );
		if( m_sel_part->m_ref_size )
			m_Doc->m_plist->SelectRefText( m_sel_part );
		else
		{
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_NONE_SELECTED );
		}
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnVertexProperties()
{
	DlgEditBoardCorner dlg;
	CString str = "Vertex Position";
	int x = m_sel_vtx.x;
	int y = m_sel_vtx.y;
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_Doc->m_nlist->MoveVertex( m_sel_net, m_sel_ic, m_sel_is,
			dlg.GetX(), dlg.GetY() );
		m_Doc->ProjectModified( TRUE );
		m_Doc->m_nlist->SelectVertex( m_sel_net, m_sel_ic, m_sel_is );
		Invalidate( FALSE );
	}
}


BOOL CFreePcbView::OnEraseBkgnd(CDC* pDC)
{
	// Erase the left and bottom panes, the PCB area is always redrawn
	m_left_pane_invalid = TRUE;
	return FALSE;
}

void CFreePcbView::OnBoardSideConvertToStraightLine()
{
	m_Doc->m_board_outline->SetSideStyle( m_sel_id.ii, CPolyLine::STRAIGHT );
	m_Doc->m_board_outline->HighlightSide( m_sel_id.ii );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnBoardSideConvertToArcCw()
{
	m_Doc->m_board_outline->SetSideStyle( m_sel_id.ii, CPolyLine::ARC_CW );
	m_Doc->m_board_outline->HighlightSide( m_sel_id.ii );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnBoardSideConvertToArcCcw()
{
	m_Doc->m_board_outline->SetSideStyle( m_sel_id.ii, CPolyLine::ARC_CCW );
	m_Doc->m_board_outline->HighlightSide( m_sel_id.ii );
	ShowSelectStatus();
	SetFKText( m_cursor_mode );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// unroute entire connection
//
void CFreePcbView::OnUnrouteTrace()
{
	SaveUndoInfoForConnection( m_sel_net, m_sel_ic );
	cconnect * c = &m_sel_con;
	int is = c->nsegs-1;
	while( c->nsegs > 1 )
	{
		m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, is );
		is--;
	}
//	m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, 0 );
	m_Doc->m_nlist->UnrouteSegment( m_sel_net, m_sel_ic, 0 );
	m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightSegment( m_sel_net, m_sel_ic, 0 );
	m_sel_id.st = ID_SEL_SEG;
	m_sel_id.ii = 0;
	SetCursorMode( CUR_RAT_SELECTED );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// save undo info for a group of parts and trace segments
//
void CFreePcbView::SaveUndoInfoForGroup( int type )
{
	void * ptr;

	// first, mark all nets as unselected and set new_event flag
	m_Doc->m_nlist->MarkAllNets(0);
	m_Doc->m_undo_list->NewEvent();

	// save info for all relevant nets
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			for( int ip=0; ip<part->pin.GetSize(); ip++ )
			{
				cnet * net = (cnet*)part->pin[ip].net;
				if( net )
				{
					if( net->utility == FALSE )
					{
						// unsaved
						SaveUndoInfoForNetAndConnectionsAndAreas( net, 0 );
						net->utility = TRUE;
					}
				}
			}
		}
		else if( sid.type == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			if( net )
			{
				if( net->utility == FALSE )
				{
					// unsaved
					SaveUndoInfoForNetAndConnectionsAndAreas( net, 0 );
					net->utility = TRUE;
				}
			}
		}
	}
	// save undo info for all parts and texts in group
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			SaveUndoInfoForPart( part, CPartList::UNDO_PART_MODIFY, FALSE );
		}
		else if( sid.type == ID_TEXT )
		{
			CText * text = (CText*)m_sel_ptrs[i];
			SaveUndoInfoForText( text, CTextList::UNDO_TEXT_MODIFY, FALSE );
		}
	}
	// save undo info for all sm cutouts
	BOOL any_sm_cutouts = FALSE;
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_SM_CUTOUT )
		{
			any_sm_cutouts = TRUE;
			break;
		}
	}
	if( any_sm_cutouts )
		SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT, FALSE );

}

// save undo info for a net (not connections or areas)
//
void CFreePcbView::SaveUndoInfoForNet( cnet * net, int type, BOOL new_event )
{
	if( type != CNetList::UNDO_NET_ADD && type != CNetList::UNDO_NET_MODIFY )
		ASSERT(0);
	void * ptr;
	if( new_event )
		m_Doc->m_undo_list->NewEvent();
	ptr = m_Doc->m_nlist->CreateNetUndoRecord( net );
	m_Doc->m_undo_list->Push( type, ptr,
		&(m_Doc->m_nlist->NetUndoCallback) );
}

// save undo info for a net and connections, not areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnections( cnet * net, int type, BOOL new_event )
{
	void * ptr;
	if( new_event )
		m_Doc->m_undo_list->NewEvent();
	if( type != CNetList::UNDO_NET_ADD )
		for( int ic=0; ic<net->nconnects; ic++ )
			SaveUndoInfoForConnection( net, ic, FALSE );
	SaveUndoInfoForNet( net, type, FALSE );
}

// save undo info for a connection
//
void CFreePcbView::SaveUndoInfoForConnection( cnet * net, int ic, BOOL new_event )
{
	if( new_event )
		m_Doc->m_undo_list->NewEvent();
	void * ptr = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
	m_Doc->m_undo_list->Push( CNetList::UNDO_CONNECT_MODIFY, ptr,
		&(m_Doc->m_nlist->ConnectUndoCallback) );
}

// save undo info for part
//
void CFreePcbView::SaveUndoInfoForPart( cpart * part, int type, BOOL new_event )
{
	if( new_event )
		m_Doc->m_undo_list->NewEvent();
	void * ptr = m_Doc->m_plist->CreatePartUndoRecord( part );
	m_Doc->m_undo_list->Push( type, ptr, 
		&m_Doc->m_plist->PartUndoCallback );
}

// save undo info for a part and all nets connected to it
// type may be:
//	CPartList::UNDO_PART_DELETE	  if part will be deleted
//	CPartList::UNDO_PART_MODIFY   if part will be modified
//	CPartList::UNDO_PART_ADD	  if part will be added	
//
void CFreePcbView::SaveUndoInfoForPartAndNets( cpart * part, int type, BOOL new_event )
{
	void * ptr;
	if( new_event )
		m_Doc->m_undo_list->NewEvent();
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
			net->utility = 0;
	}
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			if( net->utility == 0 )
			{
				SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, FALSE );
				net->utility = 1;
			}
		}
	}
	// now save undo info for part
	SaveUndoInfoForPart( part, type, FALSE );
}

// save undo info for two existing parts and all nets connected to them
//
void CFreePcbView::SaveUndoInfoFor2PartsAndNets( cpart * part1, cpart * part2, BOOL new_event )
{
	void * ptr;
	cpart * part;

	if( new_event )
		m_Doc->m_undo_list->NewEvent();
	for( int i=0; i<2; i++ )
	{
		if( i==0 )
			part = part1;
		else
			part = part2;
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			cnet * net = (cnet*)part->pin[ip].net;
			if( net )
				net->utility = 0;
		}
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			cnet * net = (cnet*)part->pin[ip].net;
			if( net )
			{
				if( net->utility == 0 )
				{
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						ptr = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
						m_Doc->m_undo_list->Push( CNetList::UNDO_CONNECT_MODIFY, ptr,
							&(m_Doc->m_nlist->ConnectUndoCallback) );
					}
					ptr = m_Doc->m_nlist->CreateNetUndoRecord( net );
					m_Doc->m_undo_list->Push( CNetList::UNDO_NET_MODIFY, ptr,
						&(m_Doc->m_nlist->NetUndoCallback) );
					net->utility = 1;
				}
			}
		}
	}
	// now save undo info for parts
	ptr = m_Doc->m_plist->CreatePartUndoRecord( part1 );
	m_Doc->m_undo_list->Push( CPartList::UNDO_PART_MODIFY, ptr, 
		&m_Doc->m_plist->PartUndoCallback );
	ptr = m_Doc->m_plist->CreatePartUndoRecord( part2 );
	m_Doc->m_undo_list->Push( CPartList::UNDO_PART_MODIFY, ptr, 
		&m_Doc->m_plist->PartUndoCallback );
}

// save undo info for net, all connections and all areas
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndAreas( cnet * net, BOOL new_event )
{
	SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, new_event );
	for( int ia=0; ia<net->nareas; ia++ )
		SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_MODIFY, FALSE );
}

// save undo info for net, all connections and
// a single copper area 
// type may be:
//	CNetList::UNDO_AREA_ADD		if area will be added
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForNetAndConnectionsAndArea( cnet * net, int iarea, int type, BOOL new_event )
{
	SaveUndoInfoForArea( net, iarea, type, new_event );
	SaveUndoInfoForNetAndConnections( net, CNetList::UNDO_NET_MODIFY, FALSE );
}

// save undo info for a modified, deleted or added copper area 
// type may be:
//	CNetList::UNDO_AREA_ADD		if area will be added
//	CNetList::UNDO_AREA_MODIFY	if area will be modified
//	CNetList::UNDO_AREA_DELETE	if area will be deleted
//
void CFreePcbView::SaveUndoInfoForArea( cnet * net, int iarea, int type, BOOL new_event )
{
	if( type != CNetList::UNDO_AREA_ADD
		&& type != CNetList::UNDO_AREA_MODIFY 
		&& type != CNetList::UNDO_AREA_DELETE )
		ASSERT(0);
	void *ptr;
	if( new_event )
		m_Doc->m_undo_list->NewEvent();
	undo_area * undo = m_Doc->m_nlist->CreateAreaUndoRecord( net, iarea, type );
	m_Doc->m_undo_list->Push( type, (void*)undo, &(m_Doc->m_nlist->AreaUndoCallback) );
}

// save undo info for all of the areas in a net
//
void CFreePcbView::SaveUndoInfoForAllAreasInNet( cnet * net, BOOL new_event )
{
	if( new_event )
		m_Doc->m_undo_list->NewEvent();		// flag new undo event
	for( int ia=net->area.GetSize()-1; ia>=0; ia-- )
		SaveUndoInfoForArea( net, ia, CNetList::UNDO_AREA_DELETE, FALSE );
	void * ptr = m_Doc->m_nlist->CreateAreaUndoRecord( net, 0, CNetList::UNDO_AREA_CLEAR_ALL );
	m_Doc->m_undo_list->Push( CNetList::UNDO_AREA_CLEAR_ALL, ptr, &(m_Doc->m_nlist->AreaUndoCallback) );
}

void CFreePcbView::SaveUndoInfoForAllNets( BOOL new_event )
{
	POSITION pos;
	CString name;
	CMapStringToPtr * m_map = &m_Doc->m_nlist->m_map;
	void * net_ptr;
	if( new_event )
		m_Doc->m_undo_list->NewEvent();		// flag new undo event
	// traverse map of nets
	for( pos = m_map->GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map->GetNextAssoc( pos, name, net_ptr );
		cnet * net = (cnet*)net_ptr;
		void * ptr;
		// loop through all connections in net
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			ptr = m_Doc->m_nlist->CreateConnectUndoRecord( net, ic );
			m_Doc->m_undo_list->Push( CNetList::UNDO_CONNECT_MODIFY, ptr,
				&(m_Doc->m_nlist->ConnectUndoCallback) );
		}
		ptr = m_Doc->m_nlist->CreateNetUndoRecord( net );
		m_Doc->m_undo_list->Push( CNetList::UNDO_NET_MODIFY, ptr,
			&(m_Doc->m_nlist->NetUndoCallback) );
	}
}

void CFreePcbView::SaveUndoInfoForMoveOrigin( int x_off, int y_off )
{
	// now push onto undo list
	undo_move_origin * undo = m_Doc->CreateMoveOriginUndoRecord( x_off, y_off );
	m_Doc->m_undo_list->NewEvent();		// flag new undo event
	m_Doc->m_undo_list->Push( 0, (void*)undo, &m_Doc->MoveOriginUndoCallback );
}

void CFreePcbView::SaveUndoInfoForBoardOutline( int type )
{
	// now push onto undo list
	undo_board_outline * undo = m_Doc->CreateBoardOutlineUndoRecord( type );
	m_Doc->m_undo_list->NewEvent();		// flag new undo event
	m_Doc->m_undo_list->Push( type, (void*)undo, &m_Doc->BoardOutlineUndoCallback );
}

void CFreePcbView::SaveUndoInfoForSMCutouts( int type, BOOL new_event )
{
	// now push onto undo list
	if( new_event )
		m_Doc->m_undo_list->NewEvent();		// flag new undo event

	// get last closed cutout
	int i_last_closed = -1;
	for( int i=0; i<m_Doc->m_sm_cutout.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[i];
		if( poly->GetClosed() )
			i_last_closed = i;
	}

	if( type == UNDO_SM_CUTOUT_NONE || i_last_closed == -1 )
	{
		// first cutout added, just flag it and push dummy data
		CPolyLine poly;
		undo_sm_cutout * undo = m_Doc->CreateSMCutoutUndoRecord( UNDO_SM_CUTOUT_NONE, &poly );
		m_Doc->m_undo_list->Push( type, (void*)undo, &m_Doc->SMCutoutUndoCallback );
	}
	else
	{
		// push all cutouts onto undo list
		for( int i=0; i<m_Doc->m_sm_cutout.GetSize(); i++ )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[i];
			if( poly->GetClosed() )
			{
				undo_sm_cutout * undo = m_Doc->CreateSMCutoutUndoRecord( type, poly );
				int new_type = UNDO_SM_CUTOUT;
				if( i == i_last_closed )
					new_type = UNDO_SM_CUTOUT_LAST;
				m_Doc->m_undo_list->Push( new_type, (void*)undo, &m_Doc->SMCutoutUndoCallback );
			}
		}
	}
}

void CFreePcbView::SaveUndoInfoForText( CText * text, int type, BOOL new_event )
{
	// push onto undo list
	undo_text * undo = m_Doc->m_tlist->CreateUndoRecord( text );
	if( new_event )
		m_Doc->m_undo_list->NewEvent();		// flag new undo event
	m_Doc->m_undo_list->Push( type, (void*)undo, &m_Doc->m_tlist->TextUndoCallback );
}


void CFreePcbView::OnViewEntireBoard()
{
	if( m_Doc->m_board_outline )
	{
		// get boundaries of board outline
		int max_x = INT_MIN;
		int min_x = INT_MAX;
		int max_y = INT_MIN;
		int min_y = INT_MAX;
		for( int ic=0; ic<m_Doc->m_board_outline->GetNumCorners(); ic++ )
		{
			if( m_Doc->m_board_outline->GetX( ic ) > max_x )
				max_x = m_Doc->m_board_outline->GetX( ic );
			if( m_Doc->m_board_outline->GetX( ic ) < min_x )
				min_x = m_Doc->m_board_outline->GetX( ic );
			if( m_Doc->m_board_outline->GetY( ic ) > max_y )
				max_y = m_Doc->m_board_outline->GetY( ic );
			if( m_Doc->m_board_outline->GetY( ic ) < min_y )
				min_y = m_Doc->m_board_outline->GetY( ic );
		}
		// reset window to enclose board outline
		m_org_x = min_x - (max_x - min_x)/20;
		m_org_y = min_y - (max_y - min_y)/20;
		double x_pcbu_per_pixel = 1.1 * (double)(max_x - min_x)/(m_client_r.right - m_left_pane_w); 
		double y_pcbu_per_pixel = 1.1 * (double)(max_y - min_y)/(m_client_r.bottom - m_bottom_pane_h);
		if( x_pcbu_per_pixel > y_pcbu_per_pixel )
			m_pcbu_per_pixel = x_pcbu_per_pixel;
		else
			m_pcbu_per_pixel = y_pcbu_per_pixel;
		m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
			m_org_x, m_org_y );
		Invalidate( FALSE );
	}
	else
	{
		AfxMessageBox( "Board outline does not exist" );
	}
}

void CFreePcbView::OnViewAllElements()
{
	// reset window to enclose all elements
	BOOL bOK = FALSE;
	CRect r;
	int test = m_Doc->m_plist->GetPartBoundaries( &r );
	if( test != 0 )
		bOK = TRUE;
	int max_x = r.right;
	int min_x = r.left;
	int max_y = r.top;
	int min_y = r.bottom;
	// now also include board outline
	if( m_Doc->m_board_outline )
	{
		r = m_Doc->m_board_outline->GetBounds();
		max_x = max( max_x, r.right );
		min_x = min( min_x, r.left );
		max_y = max( max_y, r.top );
		min_y = min( min_y, r.bottom );
		bOK = TRUE;
	}
	if( bOK )
	{
		// reset window
		m_org_x = min_x - (max_x - min_x)/20;
		m_org_y = min_y - (max_y - min_y)/20;
		double x_pcbu_per_pixel = 1.1 * (double)(max_x - min_x)/(m_client_r.right - m_left_pane_w); 
		double y_pcbu_per_pixel = 1.1 * (double)(max_y - min_y)/(m_client_r.bottom - m_bottom_pane_h);
		if( x_pcbu_per_pixel > y_pcbu_per_pixel )
			m_pcbu_per_pixel = x_pcbu_per_pixel;
		else
			m_pcbu_per_pixel = y_pcbu_per_pixel;
		m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
			m_org_x, m_org_y );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnAreaedgeHatchstyle()
{
	CDlgSetAreaHatch dlg;
	dlg.Init( m_sel_net->area[m_sel_id.i].poly->GetHatch() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int hatch = dlg.GetHatch();
		m_sel_net->area[m_sel_id.i].poly->SetHatch( hatch );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnPartEditFootprint()
{
	theApp.OnViewFootprint();
}

void CFreePcbView::OnPartEditThisFootprint()
{
	m_Doc->m_edit_footprint = TRUE;
	theApp.OnViewFootprint();
}

// Offer new footprint from the Footprint Editor
//
void CFreePcbView::OnExternalChangeFootprint( CShape * fp )
{
	CString str;
	str.Format( "Do you wish to replace the footprint of part \"%s\"\nwith the new footprint \"%s\" ?", 
		m_sel_part->ref_des, fp->m_name );
	int ret = AfxMessageBox( str, MB_YESNO );
	if( ret == IDYES )
	{
		// OK, see if a footprint of the same name is already in the cache
		void * ptr;
		BOOL found = m_Doc->m_footprint_cache_map.Lookup( fp->m_name, ptr );
		if( found )
		{
			// see how many parts are using it, not counting the current one
			CShape * old_fp = (CShape*)ptr;
			int num = m_Doc->m_plist->GetNumFootprintInstances( old_fp );
			if( m_sel_part->shape == old_fp )
				num--;
			if( num <= 0 )
			{
				// go ahead and replace it
				m_Doc->m_plist->UndrawPart( m_sel_part );
				old_fp->Copy( fp );
				m_Doc->m_plist->PartFootprintChanged( m_sel_part, old_fp );
				m_Doc->m_undo_list->Clear();
			}
			else
			{
				// offer to overwrite or rename it
				CDlgDupFootprintName dlg;
				CString mess;
				mess.Format( "Warning: A footprint named \"%s\"\r\nis already in use by other parts.\r\n", fp->m_name );
				mess += "Loading this new footprint will overwrite the old one\r\nunless you change its name\r\n";
				dlg.Initialize( &mess, &m_Doc->m_footprint_cache_map );
				int ret = dlg.DoModal();
				if( ret == IDOK )
				{
					// clicked "OK"
					if( dlg.m_replace_all_flag )
					{
						// replace all instances of footprint
						old_fp->Copy( fp );
						m_Doc->m_plist->FootprintChanged( old_fp );
						m_Doc->m_undo_list->Clear();
					}
					else
					{
						// assign new name to footprint and put in cache
						CShape * shape = new CShape;
						shape->Copy( fp );
						shape->m_name = *dlg.GetNewName();	
						m_Doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
						m_Doc->m_plist->PartFootprintChanged( m_sel_part, shape );
						m_Doc->m_undo_list->Clear();
					}
				}
			}
		}
		else
		{
			// footprint name not found in cache, add the new footprint
			CShape * shape = new CShape;
			shape->Copy( fp );
			m_Doc->m_footprint_cache_map.SetAt( shape->m_name, shape );
			m_Doc->m_plist->PartFootprintChanged( m_sel_part, shape );
			m_Doc->m_undo_list->Clear();
		}
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}


void CFreePcbView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if( nChar == 'D' )
	{
		// 'd'
		m_Doc->m_drelist->MakeHollowCircles();
		Invalidate( FALSE );
	}
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

// find a part in the layout, center window on it and select it
//
void CFreePcbView::OnViewFindpart()
{
	CDlgFindPart dlg;
	dlg.Initialize( m_Doc->m_plist );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CString * ref_des = &dlg.sel_ref_des;
		cpart * part = m_Doc->m_plist->GetPart( ref_des );
		if( part )
		{
			if( part->shape )
			{
				dl_element * dl_sel = part->dl_sel;
				int xc = NM_PER_MIL*(dl_sel->x + dl_sel->xf)/2;
				int yc = NM_PER_MIL*(dl_sel->y + dl_sel->yf)/2;
				m_org_x = xc - ((m_client_r.right-m_left_pane_w)*m_pcbu_per_pixel)/2;
				m_org_y = yc - ((m_client_r.bottom-m_bottom_pane_h)*m_pcbu_per_pixel)/2;
				m_dlist->SetMapping( &m_client_r, m_left_pane_w, m_bottom_pane_h, m_pcbu_per_pixel, 
					m_org_x, m_org_y );
				CPoint p(xc, yc);
				p = PCBToScreen( p );
				SetCursorPos( p.x, p.y - 4 );
				SelectPart( part );
				Invalidate( FALSE );
			}
			else
			{
				AfxMessageBox( "Sorry, this part doesn't have a footprint" );
			}
		}
		else
		{
			AfxMessageBox( "Sorry, this part doesn't exist" );
		}
	}
}

void CFreePcbView::OnFootprintWizard()
{
	m_Doc->OnToolsFootprintwizard();
}

void CFreePcbView::OnFootprintEditor()
{
	theApp.OnViewFootprint();
}

void CFreePcbView::OnCheckPartsAndNets()
{
	m_Doc->OnToolsCheckPartsAndNets();
}

void CFreePcbView::OnDrc()
{
	m_Doc->OnToolsDrc();
}

void CFreePcbView::OnClearDRC()
{
	m_Doc->OnToolsClearDrc();
}

void CFreePcbView::OnViewAll()
{
	OnViewAllElements();
}

void CFreePcbView::OnAddSoldermaskCutout()
{
	CDlgAddMaskCutout dlg;
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		CDC *pDC = GetDC();
		pDC->SelectClipRgn( &m_pcb_rgn );
		SetDCToWorldCoords( pDC );
		m_dlist->CancelHighLight();
		SetCursorMode( CUR_ADD_SMCUTOUT );
		m_polyline_layer = dlg.m_layer;
		m_dlist->StartDragging( pDC, m_last_cursor_point.x, 
			m_last_cursor_point.y, 0, m_active_layer, 2 );
		m_polyline_style = CPolyLine::STRAIGHT;
		m_polyline_hatch = dlg.m_hatch;
		Invalidate( FALSE );
		ReleaseDC( pDC );
	}
}

void CFreePcbView::OnSmCornerMove()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	m_from_pt.x = poly->GetX( m_sel_id.ii );
	m_from_pt.y = poly->GetY( m_sel_id.ii );
	poly->StartDraggingToMoveCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_SMCUTOUT_MOVE );
	ReleaseDC( pDC );
	Invalidate( FALSE );
}

void CFreePcbView::OnSmCornerSetPosition()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	DlgEditBoardCorner dlg;
	CString str = "Corner Position";
	int x = poly->GetX(m_sel_id.ii);
	int y = poly->GetY(m_sel_id.ii);
	dlg.Init( &str, m_Doc->m_units, x, y );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT );
		poly->MoveCorner( m_sel_id.ii, 
			dlg.GetX(), dlg.GetY() );
		CancelSelection();
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnSmCornerDeleteCorner()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	if( poly->GetNumCorners() < 4 )
	{
		AfxMessageBox( "Solder mask cutout has too few corners" );
		return;
	}
	SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT );
	poly->DeleteCorner( m_sel_id.ii );
	CancelSelection();
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

void CFreePcbView::OnSmCornerDeleteCutout()
{
	SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT );
	m_Doc->m_sm_cutout.RemoveAt( m_sel_id.i );
	id new_id = m_sel_id;
	for( int i=m_sel_id.i; i<m_Doc->m_sm_cutout.GetSize(); i++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[i];
		new_id.i = i;
		poly->SetId( &new_id );
	}
	m_Doc->ProjectModified( TRUE );
	CancelSelection();
	Invalidate( FALSE );
}

// insert corner into solder mask cutout side and start dragging
void CFreePcbView::OnSmSideInsertCorner()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p = m_last_mouse_point;
	poly->StartDraggingToInsertCorner( pDC, m_sel_id.ii, p.x, p.y, 2 );
	SetCursorMode( CUR_DRAG_SMCUTOUT_INSERT );
	ReleaseDC( pDC );
	Invalidate( FALSE );

}

// change hatch style for solder mask cutout
void CFreePcbView::OnSmSideHatchStyle()
{
	CPolyLine * poly = &m_Doc->m_sm_cutout[m_sel_id.i];
	CDlgSetAreaHatch dlg;
	dlg.Init( poly->GetHatch() );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		SaveUndoInfoForSMCutouts( UNDO_SM_CUTOUT );
		int hatch = dlg.GetHatch();
		poly->SetHatch( hatch );
		m_Doc->ProjectModified( TRUE );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnSmSideDeleteCutout()
{
	OnSmCornerDeleteCutout();
}

// change side of part
void CFreePcbView::OnPartChangeSide()
{
	SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->side = 1 - m_sel_part->side;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}

// rotate part clockwise 90 degrees
void CFreePcbView::OnPartRotate()
{
	SaveUndoInfoForPartAndNets( m_sel_part, CPartList::UNDO_PART_MODIFY );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_plist->UndrawPart( m_sel_part );
	m_sel_part->angle = (m_sel_part->angle + 90)%360;
	m_Doc->m_plist->DrawPart( m_sel_part );
	m_Doc->m_nlist->PartMoved( m_sel_part );
	m_Doc->m_nlist->OptimizeConnections( m_sel_part );
	m_Doc->m_plist->HighlightPart( m_sel_part );
	m_Doc->ProjectModified( TRUE );
	Invalidate( FALSE );
}


void CFreePcbView::OnNetSetWidth()
{
	SetWidth( 2 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightNet( m_sel_net );
}

void CFreePcbView::OnConnectSetWidth()
{ 
	SetWidth( 1 );
	m_Doc->m_dlist->CancelHighLight();
	m_Doc->m_nlist->HighlightConnection( m_sel_net, m_sel_ic );
}

void CFreePcbView::OnConnectUnroutetrace()
{
	OnUnrouteTrace();
}

void CFreePcbView::OnConnectDeletetrace()
{
	OnSegmentDeleteTrace();
}

void CFreePcbView::OnSegmentChangeLayer()
{
	ChangeTraceLayer( 0, m_sel_seg.layer );
}

void CFreePcbView::OnConnectChangeLayer()
{
	ChangeTraceLayer( 1 );
}

void CFreePcbView::OnNetChangeLayer()
{
	ChangeTraceLayer( 2 );
}

// change layer of routed trace segments
// if mode = 0, current segment
// if mode = 1, current connection
// if mode = 2, current net
//
void CFreePcbView::ChangeTraceLayer( int mode, int old_layer )
{
	CDlgChangeLayer dlg;
	dlg.Initialize( mode, old_layer, m_Doc->m_num_copper_layers );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		int err = 0;
		SaveUndoInfoForNetAndConnections( m_sel_net, CNetList::UNDO_NET_MODIFY, TRUE );
		cconnect * c = &m_sel_con;
		if( dlg.m_apply_to == 0 )
		{
			err = m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net, 
						m_sel_id.i, m_sel_id.ii, dlg.m_new_layer );
			if( err )
			{
				AfxMessageBox( "Unable to change layer for this segment" );
			}
			Invalidate( FALSE );
		}
		else if( dlg.m_apply_to == 1 )
		{
			for( int is=0; is<c->nsegs; is++ )
			{
				if( c->seg[is].layer >= LAY_TOP_COPPER )
				{
					err += m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net, 
						m_sel_id.i, is, dlg.m_new_layer );
				}
			}
			if( err )
			{
				AfxMessageBox( "Unable to change layer for all segments" );
			}
		}
		else if( dlg.m_apply_to == 2 )
		{
			for( int ic=0; ic<m_sel_net->nconnects; ic++ )
			{
				cconnect * c = &m_sel_net->connect[ic];
				for( int is=0; is<c->nsegs; is++ )
				{
					if( c->seg[is].layer >= LAY_TOP_COPPER )
					{
						err += m_Doc->m_nlist->ChangeSegmentLayer( m_sel_net, 
							ic, is, dlg.m_new_layer );
					}
				}
			}
			if( err )
			{
				AfxMessageBox( "Unable to change layer for all segments" );
			}
		}
		Invalidate( FALSE );
	}

}

void CFreePcbView::OnNetEditnet()
{
	CDlgEditNet dlg;
	netlist_info nl;
	m_Doc->m_nlist->ExportNetListInfo( &nl );
	int inet = -1;
	for( int i=0; i<nl.GetSize(); i++ )
	{
		if( nl[i].net == m_sel_net )
		{
			inet = i;
			break;
		}
	}
	if( inet == -1 )
		ASSERT(0);
	dlg.Initialize( &nl, inet, m_Doc->m_plist, FALSE, TRUE, m_Doc->m_units, 
		&m_Doc->m_w, &m_Doc->m_v_w, &m_Doc->m_v_h_w ); 
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		m_Doc->m_undo_list->Clear();	// clear undo list
		CancelSelection();
		m_Doc->m_nlist->ImportNetListInfo( &nl, 0, NULL, 
			m_Doc->m_trace_w, m_Doc->m_via_w, m_Doc->m_via_hole_w );
		Invalidate( FALSE );
	}
}

void CFreePcbView::OnToolsMoveOrigin()
{
	CDlgMoveOrigin dlg;
	dlg.Initialize( m_Doc->m_units );
	int ret = dlg.DoModal();
	if( ret == IDOK )
	{
		if( dlg.m_drag )
		{
			CDC *pDC = GetDC();
			pDC->SelectClipRgn( &m_pcb_rgn );
			SetDCToWorldCoords( pDC );
			m_dlist->CancelHighLight();
			SetCursorMode( CUR_MOVE_ORIGIN );
			m_dlist->StartDragging( pDC, m_last_cursor_point.x, 
				m_last_cursor_point.y, 0, LAY_SELECTION, 2 );
			Invalidate( FALSE );
			ReleaseDC( pDC );
		}
		else
		{
			SaveUndoInfoForMoveOrigin( -dlg.m_x, -dlg.m_x );
			MoveOrigin( -dlg.m_x, -dlg.m_y );
			Invalidate( FALSE );
		}
	}
}

// move origin of coord system by moving everything
// by (x_off, y_off)
//
void CFreePcbView::MoveOrigin( int x_off, int y_off )
{
	if( m_Doc->m_board_outline )
		m_Doc->m_board_outline->MoveOrigin( x_off, y_off );
	m_Doc->m_plist->MoveOrigin( x_off, y_off );
	m_Doc->m_nlist->MoveOrigin( x_off, y_off );
	m_Doc->m_tlist->MoveOrigin( x_off, y_off );
	for( int ism=0; ism<m_Doc->m_sm_cutout.GetSize(); ism++ )
		m_Doc->m_sm_cutout[ism].MoveOrigin( x_off, y_off );
}

void CFreePcbView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// save starting position in pixels
	m_bLButtonDown = TRUE;
	m_bDraggingRect = FALSE;
	m_start_pt = point;
	CView::OnLButtonDown(nFlags, point);
}

// Select all parts and trace segments in rectangle
// Fill arrays m_sel_ids[] and m_sel_ptrs[]
// Set utility flags for selected parts and segments
//
void CFreePcbView::SelectItemsInRect( CRect r, BOOL bAddToGroup )
{
	if( !bAddToGroup )
		CancelSelection();
	r.NormalizeRect();

	// find parts in rect
	if( m_sel_mask & (1<<SEL_MASK_PARTS ) )
	{
		cpart * part = m_Doc->m_plist->GetFirstPart();
		while( part )
		{
			CRect p_r;
			if( m_Doc->m_plist->GetPartBoundingRect( part, &p_r ) )
			{
				p_r.NormalizeRect();
				if( InRange( p_r.top, r.top, r.bottom )
					&& InRange( p_r.bottom, r.top, r.bottom )
					&& InRange( p_r.left, r.left, r.right )
					&& InRange( p_r.right, r.left, r.right ) )
				{
					// add part to selection list and highlight it
					id pid( ID_PART, ID_SEL_RECT, 0, 0, 0 );
					if( FindItemInGroup( part, &pid ) == -1 )
					{
						m_sel_ids.Add( pid );
						m_sel_ptrs.Add( part );
					}
				}
			}
			part = m_Doc->m_plist->GetNextPart( part );
		}
	}

	// find trace segments contained in rect
	if( m_sel_mask & (1<<SEL_MASK_CON ) )
	{
		cnet * net = m_Doc->m_nlist->GetFirstNet();
		while( net )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				for( int is=0; is<c->nsegs; is++ )
				{
					cvertex * pre_v = &c->vtx[is];
					cvertex * post_v = &c->vtx[is+1];
					cseg * s = &c->seg[is];
					if( InRange( pre_v->x, r.left, r.right )
						&& InRange( post_v->x, r.left, r.right )
						&& InRange( pre_v->y, r.top, r.bottom )
						&& InRange( post_v->y, r.top, r.bottom )
						&& s->layer >= LAY_TOP_COPPER 
						&& m_Doc->m_vis[s->layer] )
					{
						// add segment to selection list and highlight it
						id sid( ID_NET, ID_CONNECT, ic, ID_SEL_SEG, is );
						if( FindItemInGroup( net, &sid ) == -1 )
						{
							m_sel_ids.Add( sid );
							m_sel_ptrs.Add( net );
						}
					}
				}
			}
			net = m_Doc->m_nlist->GetNextNet();
		}
	}

	// find texts in rect
	if( m_sel_mask & (1<<SEL_MASK_TEXT ) )
	{
		CText * t = m_Doc->m_tlist->GetFirstText();
		while( t )
		{
			if( InRange( m_dlist->Get_x( t->dl_sel ), r.left, r.right )
				&& InRange( m_dlist->Get_xf( t->dl_sel ), r.left, r.right )
				&& InRange( m_dlist->Get_y( t->dl_sel ), r.top, r.bottom )
				&& InRange( m_dlist->Get_yf( t->dl_sel ), r.top, r.bottom ) 
				&& m_Doc->m_vis[t->m_layer] )
			{
				// add text to selection list and highlight it
				id sid( ID_TEXT, ID_SEL_TXT, 0, 0, 0 );
				if( FindItemInGroup( t, &sid ) == -1 )
				{
					m_sel_ids.Add( sid );
					m_sel_ptrs.Add( t );
				}
			}
			t = m_Doc->m_tlist->GetNextText();
		}
	}

	// find copper area sides in rect
	if( m_sel_mask & (1<<SEL_MASK_AREAS ) )
	{
		cnet * net = m_Doc->m_nlist->GetFirstNet();
		while( net )
		{
			if( net->nareas )
			{
				for( int ia=0; ia<net->nareas; ia++ )
				{
					carea * a = &net->area[ia];
					CPolyLine * poly = a->poly;
					for( int ic=0; ic<poly->GetNumContours(); ic++ )
					{
						int istart = poly->GetContourStart(ic);
						int iend = poly->GetContourEnd(ic);
						for( int is=istart; is<=iend; is++ )
						{
							int ic1, ic2;
							ic1 = is;
							if( is < iend )
								ic2 = is+1;
							else
								ic2 = istart;
							int x1 = poly->GetX(ic1);
							int y1 = poly->GetY(ic1);
							int x2 = poly->GetX(ic2);
							int y2 = poly->GetY(ic2);
							if( InRange( x1, r.left, r.right )
								&& InRange( x2, r.left, r.right )
								&& InRange( y1, r.top, r.bottom )
								&& InRange( y2, r.top, r.bottom )
								&& m_Doc->m_vis[poly->GetLayer()] )
							{
								id aid( ID_NET, ID_AREA, ia, ID_SEL_SIDE, is );
								if( FindItemInGroup( net, &aid ) == -1 )
								{
									m_sel_ids.Add( aid );
									m_sel_ptrs.Add( net );
								}
							}
						}
					}
				}
			}
			net = m_Doc->m_nlist->GetNextNet(); 
		}
	}

	// find solder mask cutout sides in rect
	if( m_sel_mask & (1<<SEL_MASK_SM ) )
	{
		for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[im];
			for( int ic=0; ic<poly->GetNumContours(); ic++ )
			{
				int istart = poly->GetContourStart(ic);
				int iend = poly->GetContourEnd(ic);
				for( int is=istart; is<=iend; is++ )
				{
					int ic1, ic2;
					ic1 = is;
					if( is < iend )
						ic2 = is+1;
					else
						ic2 = istart;
					int x1 = poly->GetX(ic1);
					int y1 = poly->GetY(ic1);
					int x2 = poly->GetX(ic2);
					int y2 = poly->GetY(ic2);
					if( InRange( x1, r.left, r.right )
						&& InRange( x2, r.left, r.right )
						&& InRange( y1, r.top, r.bottom )
						&& InRange( y2, r.top, r.bottom ) 
						&& m_Doc->m_vis[poly->GetLayer()] )
					{
						id smid( ID_SM_CUTOUT, ID_SM_CUTOUT, im, ID_SEL_SIDE, is );
						if( FindItemInGroup( poly, &smid ) == -1 )
						{
							m_sel_ids.Add( smid );
							m_sel_ptrs.Add( poly );
						}
					}
				}
			}
		}
	}

	// now highlight selected items
	if( m_sel_ids.GetSize() == 0 )
		CancelSelection();
	else
	{
		HighlightGroup();
		SetCursorMode( CUR_GROUP_SELECTED );
	}
	gLastKeyWasArrow = FALSE;
}

void CFreePcbView::StartDraggingGroup()
{
	// snap dragging point to placement grid
	SetCursorMode( CUR_DRAG_GROUP ); 
	SnapCursorPoint( m_last_mouse_point );
	m_from_pt = m_last_cursor_point;

	// make texts, parts and segments invisible
	m_dlist->SetLayerVisible( LAY_HILITE, FALSE );
	int n_parts = 0;
	int n_segs = 0;
	int n_texts = 0;
	int n_area_sides = 0;
	int n_sm_sides = 0;
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			m_Doc->m_plist->MakePartVisible( part, FALSE );
			n_parts++;
		}
		else if( sid.type == ID_NET && sid.st == ID_CONNECT 
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			dl_element * dl = net->connect[sid.i].seg[sid.ii].dl_el;
			m_dlist->Set_visible( dl, FALSE );
			m_Doc->m_nlist->SetViaVisible( net, sid.i, sid.ii, FALSE );
			m_Doc->m_nlist->SetViaVisible( net, sid.i, sid.ii+1, FALSE );
			n_segs++;
		}
		else if( sid.type == ID_NET && sid.st == ID_AREA 
			&& sid.sst == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.i];
//			a->poly->SetSideVisible( sid.ii, FALSE );
			a->poly->MakeVisible( FALSE );
			n_area_sides++;
		}
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT 
			&& sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.i];
//			poly->SetSideVisible( sid.ii, FALSE );
			poly->MakeVisible( FALSE );
			n_sm_sides++;
		}
		else if( sid.type == ID_TEXT )
		{
			// make text strokes invisible
			CText * text = (CText*)m_sel_ptrs[i];
			for( int is=0; is<text->m_stroke.GetSize(); is++ )
				((dl_element*)text->m_stroke[is].dl_el)->visible = 0;
			n_texts++;
		}
	}

	// set up dragline array
	m_dlist->MakeDragLineArray( n_parts*4 + n_segs + n_texts*4 + n_area_sides + n_sm_sides );
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			int xi = part->shape->m_sel_xi;
			int xf = part->shape->m_sel_xf;
			if( part->side )
			{
				xi = -xi;
				xf = -xf;
			}
			int yi = part->shape->m_sel_yi;
			int yf = part->shape->m_sel_yf;
			CPoint p1( xi, yi );
			CPoint p2( xf, yi );
			CPoint p3( xf, yf );
			CPoint p4( xi, yf );
			RotatePoint( &p1, part->angle, zero );
			RotatePoint( &p2, part->angle, zero );
			RotatePoint( &p3, part->angle, zero );
			RotatePoint( &p4, part->angle, zero );
			p1.x += part->x - m_from_pt.x;
			p2.x += part->x - m_from_pt.x;
			p3.x += part->x - m_from_pt.x;
			p4.x += part->x - m_from_pt.x;
			p1.y += part->y - m_from_pt.y;
			p2.y += part->y - m_from_pt.y;
			p3.y += part->y - m_from_pt.y;
			p4.y += part->y - m_from_pt.y;
			m_dlist->AddDragLine( p1, p2 ); 
			m_dlist->AddDragLine( p2, p3 ); 
			m_dlist->AddDragLine( p3, p4 ); 
			m_dlist->AddDragLine( p4, p1 ); 
		}
		else if( sid.type == ID_NET && sid.st == ID_CONNECT 
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			cconnect * c = &net->connect[sid.i];
			cseg * s = &c->seg[sid.ii];
			cvertex * v1 = &c->vtx[sid.ii];
			cvertex * v2 = &c->vtx[sid.ii+1];
			CPoint p1( v1->x - m_from_pt.x, v1->y - m_from_pt.y );
			CPoint p2( v2->x - m_from_pt.x, v2->y - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 ); 
		}
		else if( sid.type == ID_NET && sid.st == ID_AREA 
			&& sid.sst == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.i];
			CPolyLine * poly = a->poly;
			int icontour = poly->GetContour(sid.ii);
			int ic1 = sid.ii;
			int ic2 = sid.ii+1;
			if( ic2 > poly->GetContourEnd(icontour) )
				ic2 = poly->GetContourStart(icontour);
			CPoint p1( poly->GetX(ic1) - m_from_pt.x, poly->GetY(ic1) - m_from_pt.y );
			CPoint p2( poly->GetX(ic2) - m_from_pt.x, poly->GetY(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 ); 
		}
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT 
			&& sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.i];
			int icontour = poly->GetContour(sid.ii);
			int ic1 = sid.ii;
			int ic2 = sid.ii+1;
			if( ic2 > poly->GetContourEnd(icontour) )
				ic2 = poly->GetContourStart(icontour);
			CPoint p1( poly->GetX(ic1) - m_from_pt.x, poly->GetY(ic1) - m_from_pt.y );
			CPoint p2( poly->GetX(ic2) - m_from_pt.x, poly->GetY(ic2) - m_from_pt.y );
			m_dlist->AddDragLine( p1, p2 ); 
		}
		else if( sid.type == ID_TEXT )
		{
			CText * text = (CText*)m_sel_ptrs[i];
			CPoint p1( m_dlist->Get_x( text->dl_sel ), m_dlist->Get_y( text->dl_sel ) );
			CPoint p2( m_dlist->Get_xf( text->dl_sel ), m_dlist->Get_y( text->dl_sel ) );
			CPoint p3( m_dlist->Get_xf( text->dl_sel ), m_dlist->Get_yf( text->dl_sel ) );
			CPoint p4( m_dlist->Get_x( text->dl_sel ), m_dlist->Get_yf( text->dl_sel ) );
			p1 -= m_from_pt;
			p2 -= m_from_pt;
			p3 -= m_from_pt;
			p4 -= m_from_pt;
			m_dlist->AddDragLine( p1, p2 ); 
			m_dlist->AddDragLine( p2, p3 ); 
			m_dlist->AddDragLine( p3, p4 ); 
			m_dlist->AddDragLine( p4, p1 ); 
		}

	}
	CDC *pDC = GetDC();
	pDC->SelectClipRgn( &m_pcb_rgn );
	SetDCToWorldCoords( pDC );
	CPoint p;
	p.x  = m_from_pt.x;
	p.y  = m_from_pt.y;
	CPoint cur_p = PCBToScreen( p );
	SetCursorPos( cur_p.x, cur_p.y );
	m_dlist->StartDragging( pDC, m_from_pt.x, m_from_pt.y, 0, LAY_SELECTION, TRUE );
	Invalidate( FALSE );
	ReleaseDC( pDC );
}

void CFreePcbView::CancelDraggingGroup()
{
	m_dlist->StopDragging();
	// make elements visible again
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			m_Doc->m_plist->MakePartVisible( part, TRUE );
		}
		else if( sid.type == ID_NET && sid.st == ID_CONNECT 
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			dl_element * dl = net->connect[sid.i].seg[sid.ii].dl_el;
			m_dlist->Set_visible( dl, TRUE );
			m_Doc->m_nlist->SetViaVisible( net, sid.i, sid.ii, TRUE );
			m_Doc->m_nlist->SetViaVisible( net, sid.i, sid.ii+1, TRUE );
		}
		else if( sid.type == ID_NET && sid.st == ID_AREA 
			&& sid.sst == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			carea * a = &net->area[sid.i];
//			a->poly->SetSideVisible( sid.ii, TRUE );
			a->poly->MakeVisible( TRUE );
		}
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT 
			&& sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.i];
//			poly->SetSideVisible( sid.ii, TRUE );
			poly->MakeVisible( TRUE );
		}
		else if( sid.type == ID_TEXT )
		{
			// make text strokes invisible
			CText * text = (CText*)m_sel_ptrs[i];
			for( int is=0; is<text->m_stroke.GetSize(); is++ )
				((dl_element*)text->m_stroke[is].dl_el)->visible = TRUE;
		}
	}
	m_dlist->SetLayerVisible( LAY_HILITE, TRUE );
	SetCursorMode( CUR_GROUP_SELECTED );
	Invalidate( FALSE );
}

void CFreePcbView::OnGroupMove()
{
	if( GluedPartsInGroup() )
	{
		int ret = AfxMessageBox( "This group contains glued parts, unglue and move them ?", MB_OKCANCEL );
		if( ret != IDOK )
			return;
	}
	m_dlist->SetLayerVisible( LAY_RAT_LINE, FALSE );
	StartDraggingGroup();
}


// Move group of parts and trace segments
//
void CFreePcbView::MoveGroup( int dx, int dy )
{	
	UngluePartsInGroup();

	// mark all parts and nets as unselected
	m_Doc->m_nlist->MarkAllNets(0);
	m_Doc->m_plist->MarkAllParts(0);

	// mark connections and segments of selected nets as unselected
	// and vertices and area corners as unmoved
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			if( net->utility == FALSE )
			{
				// first time for this net, 
				net->utility = TRUE;
				// mark all connections and segments as unselected
				// and vertices as unmoved
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					net->connect[ic].utility = FALSE;
					for( int is=0; is<net->connect[ic].nsegs; is++ )
					{
						net->connect[ic].seg[is].utility = FALSE;
						net->connect[ic].vtx[is].utility = FALSE;
						net->connect[ic].vtx[is+1].utility = FALSE;
					}
				}
				// mark all area corners as unmoved
				for( int ia=0; ia<net->nareas; ia++ )
				{
					for( int is=0; is<net->area[ia].poly->GetNumCorners(); is++ )
					{
						net->area[ia].poly->SetUtility( is, 0 );
					}
				}
			}
		}
	}
	// mark all corners of solder mask cutouts as unmoved
	for( int im=0; im<m_Doc->m_sm_cutout.GetSize(); im++ )
	{
		CPolyLine * poly = &m_Doc->m_sm_cutout[im];
		for( int ic=0; ic<poly->GetNumCorners(); ic++ )
			poly->SetUtility( ic, 0 );
	}

	// mark all relevant parts, nets, connections and segments as selected
	// and move text and copper area corners
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT 
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.i;
			int is = sid.ii;
			cconnect * c = &net->connect[ic];	// this connection
			cseg * s = &c->seg[is];				// this segment
			cvertex * pre_v = &c->vtx[is];
			cvertex * post_v = &c->vtx[is+1];
			pre_v->utility = FALSE;
			post_v->utility = FALSE;
			c->utility = TRUE;					// mark connection selected
			s->utility = TRUE;					// mark segment selected
		}
		else if( sid.type == ID_PART && sid.st == ID_SEL_RECT )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->utility = TRUE;	// mark part selected
		}
		else if( sid.type == ID_TEXT && sid.st == ID_SEL_TXT )
		{
			CText * t = (CText*)m_sel_ptrs[i];
			m_sel_ptrs[i] = m_Doc->m_tlist->MoveText( t, t->m_x+dx, t->m_y+dy, t->m_angle,
				t->m_mirror, t->m_layer );
		}
		else if( sid.type == ID_NET && sid.st == ID_AREA && sid.sst == ID_SEL_SIDE )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			CPolyLine * poly = net->area[sid.i].poly;
			int icontour = poly->GetContour(sid.ii);
			int istart = poly->GetContourStart(icontour);
			int iend = poly->GetContourEnd(icontour);
			int ic1 = sid.ii;
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			poly->Undraw();
			if( !poly->GetUtility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->GetX(ic1) + dx );
				poly->SetY( ic1, poly->GetY(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->GetUtility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->GetX(ic2) + dx );
				poly->SetY( ic2, poly->GetY(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
			poly->Draw();
		}
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT && sid.sst == ID_SEL_SIDE )
		{
			CPolyLine * poly = &m_Doc->m_sm_cutout[sid.i];
			int icontour = poly->GetContour(0);
			int istart = poly->GetContourStart(icontour);
			int iend = poly->GetContourEnd(icontour);
			int ic1 = sid.ii;
			int ic2 = ic1+1;
			if( ic2 > iend )
				ic2 = istart;
			poly->Undraw();
			if( !poly->GetUtility(ic1) )
			{
				// unmoved, move it
				poly->SetX( ic1, poly->GetX(ic1) + dx );
				poly->SetY( ic1, poly->GetY(ic1) + dy );
				poly->SetUtility(ic1,1);
			}
			if( !poly->GetUtility(ic2) )
			{
				// unmoved, move it
				poly->SetX( ic2, poly->GetX(ic2) + dx );
				poly->SetY( ic2, poly->GetY(ic2) + dy );
				poly->SetUtility(ic2,1);
			}
			poly->Draw();
		}
		else
			ASSERT(0);
	}

	// assume utility flags have been set on selected parts, 
	// nets, connections and segments
	// mark all vertices in selected nets as unmoved
	cnet * net = m_Doc->m_nlist->GetFirstNet();
	while( net != NULL )
	{
		if( net->utility )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->utility )
				{
					for( int is=0; is<c->nsegs+1; is++ )
						c->vtx[is].utility = FALSE;
				}
			}
		}
		net = m_Doc->m_nlist->GetNextNet();
	}
	// move parts in group
	cpart * part = m_Doc->m_plist->GetFirstPart();
	while( part != NULL )
	{
		if( part->utility )
		{
			// move part
			m_Doc->m_plist->Move( part, part->x+dx, part->y+dy, part->angle, part->side );
			// find segments which connect to this part
			cnet * net;
			for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
			{
				net = (cnet*)part->pin[ip].net;
				if( net )
				{
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						cconnect * c = &net->connect[ic];
						int nsegs = c->nsegs;
						if( nsegs )
						{
							int p1 = c->start_pin;
							CString pin_name1 = net->pin[p1].pin_name;
							int pin_index1 = part->shape->GetPinIndexByName( &pin_name1 );
							int p2 = c->end_pin;
							if( net->pin[p1].part == part )
							{
								// starting pin is on part
								if( p2 == cconnect::NO_END && nsegs == 1 && c->utility == FALSE )
								{
									// unselected stub trace with one segment, move vertex
									m_Doc->m_nlist->MoveVertex( net, ic, 0, 
										part->pin[pin_index1].x, part->pin[pin_index1].y );
								}
								else
								{
									if( c->seg[0].layer != LAY_RAT_LINE )
									{
										// unselected first segment, unroute it
										if( !c->seg[0].utility )
											m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, 0 );
									}
									// modify vertex position
									if( !net->connect[ic].vtx[0].utility )
									{
										if( !net->connect[ic].vtx[0].utility )
										{
											m_Doc->m_nlist->MoveVertex( net, ic, 0, 
												part->pin[pin_index1].x, part->pin[pin_index1].y );
											c->vtx[0].utility = TRUE;
										}
									}
								}
							}
							if( p2 != cconnect::NO_END )
							{
								if( net->pin[p2].part == part )
								{
									// ending pin is on part, unroute last segment
									if( c->seg[nsegs-1].layer != LAY_RAT_LINE )
									{
										if( !c->seg[nsegs-1].utility )
										{
											m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, nsegs-1 );
										}
									}
									// modify vertex position if necessary
									CString pin_name2 = net->pin[p2].pin_name;
									int pin_index2 = part->shape->GetPinIndexByName( &pin_name2 );
									if( !c->vtx[nsegs].utility )
									{
										m_Doc->m_nlist->MoveVertex( net, ic, nsegs,
											part->pin[pin_index2].x, part->pin[pin_index2].y );
										c->vtx[nsegs].utility = TRUE;
									}
								}
							}
						}
					}
				}
			}
		}
		part = m_Doc->m_plist->GetNextPart( part );
	}
	// get selected segments
	net = m_Doc->m_nlist->GetFirstNet();
	while( net != NULL )
	{
		if( net->utility )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->utility )
				{
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].utility )
						{
							// move trace segment
							cseg * s = &c->seg[is];				// this segment
							cvertex * pre_v = &c->vtx[is];		// pre vertex
							cvertex * post_v = &c->vtx[is+1];	// post vertex
							CPoint old_pre_v_pt( pre_v->x, pre_v->y );		// pre vertex coords
							CPoint old_post_v_pt( post_v->x, post_v->y );	// post vertex coords
							cpart * part1 = net->pin[c->start_pin].part;	// connection starting part
							cpart * part2 = NULL;				// connection ending part or NULL
							if( c->end_pin != cconnect::NO_END )
								part2 = net->pin[c->end_pin].part;	

							// undraw entire trace
							m_Doc->m_nlist->UndrawConnection( net, ic );

							// move adjacent vertices, unless already moved
							if( !pre_v->utility )
							{
								pre_v->x += dx;
								pre_v->y += dy;
								pre_v->utility = TRUE;
							}
							if( !post_v->utility )
							{
								post_v->x += dx;
								post_v->y += dy;
								post_v->utility = TRUE;
							}

							// unroute adjacent segments unless they are also being moved
							if( is>0 )
							{
								// test for preceding segment
								if( !c->seg[is-1].utility )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is-1 );
							}
							if( is < c->nsegs-1 )
							{
								// test for following segment and not end of stub trace
								if( !c->seg[is+1].utility && (part2 || is < c->nsegs-2) )
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, is+1 );
							}
							m_Doc->m_nlist->DrawConnection( net, ic );

							// special case, first segment of trace selected but part not selected
							if( part1->utility == FALSE && is == 0 )
							{
								if( s->layer == LAY_RAT_LINE )
									ASSERT(0);
								// insert ratline as new first segment
								CPoint new_v_pt( pre_v->x, pre_v->y );
								m_Doc->m_nlist->MoveVertex( net, ic, 0, old_pre_v_pt.x, old_pre_v_pt.y ); 
								m_Doc->m_nlist->InsertSegment( net, ic, 0, new_v_pt.x, new_v_pt.y, LAY_RAT_LINE, 1, 0, 0, 0 );
								c->seg[0].utility = 0;
								is++;
							}

							// special case, last segment of trace selected but part not selected
							if( part2 )
							{
								if( part2->utility == FALSE && is == c->nsegs-1 )
								{
									// insert ratline as new last segment
									int old_w = c->seg[c->nsegs-1].width;
									int old_v_w = c->seg[c->nsegs-1].via_w;
									int old_v_h_w = c->seg[c->nsegs-1].via_hole_w;
									int old_layer = c->seg[c->nsegs-1].layer;
									m_Doc->m_nlist->UnrouteSegmentWithoutMerge( net, ic, c->nsegs-1 );
									CPoint new_v_pt( c->vtx[c->nsegs].x, c->vtx[c->nsegs].y );
									m_Doc->m_nlist->MoveVertex( net, ic, c->nsegs, old_post_v_pt.x, old_post_v_pt.y ); 
									int test_not_done = m_Doc->m_nlist->InsertSegment( net, ic, c->nsegs-1, 
										new_v_pt.x, new_v_pt.y, old_layer, old_w, old_v_w, old_v_h_w, 0 );
									c->seg[c->nsegs-2].utility = 1;
									c->seg[c->nsegs-1].utility = 0;
								}
							}
						}
					}
				}
			}
		}
		net = m_Doc->m_nlist->GetNextNet();
	}

	// merge unrouted segments for all traces
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT 
			&& sid.sst == ID_SEL_SEG )
		{
			cnet * net = (cnet*)m_sel_ptrs[i];
			int ic = sid.i;
			m_Doc->m_nlist->MergeUnroutedSegments( net, ic );
		}
	}

	m_Doc->m_nlist->OptimizeConnections();

	// regenerate selection list from utility flags
	// first, remove all segments
	for( int i=m_sel_ids.GetSize()-1; i>=0; i-- )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_SEG )
		{
			m_sel_ids.RemoveAt(i);
			m_sel_ptrs.RemoveAt(i);
		}
	}
	// add segments back in
	net = m_Doc->m_nlist->GetFirstNet();
	while( net )
	{
		if( net->utility )
		{
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->utility )
				{
					for( int is=0; is<c->nsegs; is++ )
					{
						if( c->seg[is].utility && c->seg[is].layer != LAY_RAT_LINE )
						{
							m_sel_ptrs.Add( net );
							id sid( ID_NET, ID_CONNECT, ic, ID_SEL_SEG, is );
							m_sel_ids.Add( sid );
						}
					}
				}
			}
		}
		net = m_Doc->m_nlist->GetNextNet();
	}
}

// Highlight group selection
// the only legal members are parts, texts, trace segments
// copper area sides and soler mask cutout sides
//
void CFreePcbView::HighlightGroup()
{
	m_dlist->CancelHighLight();
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		id sid = m_sel_ids[i];
		if( sid.type == ID_PART && sid.st == ID_SEL_RECT )
			m_Doc->m_plist->HighlightPart( (cpart*)m_sel_ptrs[i] );
		else if( sid.type == ID_NET && sid.st == ID_CONNECT && sid.sst == ID_SEL_SEG )
			m_Doc->m_nlist->HighlightSegment( (cnet*)m_sel_ptrs[i], sid.i, sid.ii );
		else if( sid.type == ID_TEXT && sid.st == ID_SEL_TXT )
			m_Doc->m_tlist->HighlightText( (CText*)m_sel_ptrs[i] );
		else if( sid.type == ID_NET && sid.st == ID_AREA && sid.sst == ID_SEL_SIDE )
			((cnet*)m_sel_ptrs[i])->area[sid.i].poly->HighlightSide(sid.ii);
		else if( sid.type == ID_SM_CUTOUT && sid.st == ID_SM_CUTOUT && sid.sst == ID_SEL_SIDE )
			m_Doc->m_sm_cutout[sid.i].HighlightSide(sid.ii);
		else
			ASSERT(0);
	}
}

// Find item in group
// returns index of item if found, otherwise -1
//
int CFreePcbView::FindItemInGroup( void * ptr, id * tid )
{
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		if( m_sel_ptrs[i] == ptr && m_sel_ids[i] == *tid )
			return i;
	}
	return -1;
}

// Test for glued parts in group
// returns index of item if found, otherwise -1
//
BOOL CFreePcbView::GluedPartsInGroup()
{
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		if( m_sel_ids[i].type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			if( part->glued )
				return TRUE;
		}
	}
	return FALSE;
}

// Unglue parts in group
// returns index of item if found, otherwise -1
//
void CFreePcbView::UngluePartsInGroup()
{
	for( int i=0; i<m_sel_ids.GetSize(); i++ )
	{
		if( m_sel_ids[i].type == ID_PART )
		{
			cpart * part = (cpart*)m_sel_ptrs[i];
			part->glued = FALSE;
		}
	}
}

// Set array of selection mask ids
//
void CFreePcbView::SetSelMaskArray( int mask )
{
	for( int i=0; i<NUM_SEL_MASKS; i++ )
	{
		if( mask & (1<<i) )
			m_mask_id[i].ii = 0;
		else
			m_mask_id[i].ii = 0xfffe;	// guaranteed not to exist
	}
}

