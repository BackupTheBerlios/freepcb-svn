// PolyLine.cpp ... implementation of CPolyLine class

#include "stdafx.h"
#include "math.h"
#include "PolyLine.h"
#include "FreePcb.h"
#include "utility.h"
#include "layers.h"

#define pi  3.14159265359

// constructor:
//   dl is a pointer to CDisplayList for drawing graphic elements
//   if dl = NULL, doesn't draw anything but can still hold data
//
CPolyLine::CPolyLine( CDisplayList * dl )
{
	m_dlist = dl;
	m_ncorners = 0;
	m_ptr = 0;
	m_hatch = 0;
	m_sel_box = 0;
	m_gpc_poly.num_contours = 0;
}

CPolyLine::CPolyLine()
{
	m_dlist = NULL;
	m_ncorners = 0;
	m_ptr = 0;
	m_hatch = 0;
	m_sel_box = 0;
	m_gpc_poly.num_contours = 0;
}

// destructor, removes display elements
//
CPolyLine::~CPolyLine()
{
	Undraw();
	FreeGpcPoly();
//	if( m_closed )
//		DeleteObject( m_hrgn );
}

// use the General Polygon Clipping Library to clip contours
// returns number of contours, or -1 if error
//
int CPolyLine::NormalizeWithGpc()
{
	MakeGpcPoly( -1 );

	// now, recreate poly
	// first, find outside contours and holes
	int ext_cont;
	int n_ext_cont = 0;
	for( int ic=0; ic<m_gpc_poly.num_contours; ic++ )
	{
		if( !(m_gpc_poly.hole)[ic] )
		{
			ext_cont = ic;
			n_ext_cont++;
		}
	}
	if( n_ext_cont != 1 )
	{
		FreeGpcPoly();
		return -1;
	}

	// clear old poly corners and sides
	Undraw();
	corner.RemoveAll();
	side_style.RemoveAll();
	m_ncorners = 0;

	// start with external contour
	for( int i=0; i<m_gpc_poly.contour[ext_cont].num_vertices; i++ )
	{
		int x = ((m_gpc_poly.contour)[ext_cont].vertex)[i].x;
		int y = ((m_gpc_poly.contour)[ext_cont].vertex)[i].y;
		if( i==0 )
			Start( m_layer, m_w, m_sel_box, x, y, m_hatch, &m_id, m_ptr );
		else
			AppendCorner( x, y );
	}
	Close();

	// now add holes
	for( int ic=0; ic<m_gpc_poly.num_contours; ic++ )
	{
		if( ic != ext_cont )
		{
			for( int i=0; i<m_gpc_poly.contour[ic].num_vertices; i++ )
			{
				int x = ((m_gpc_poly.contour)[ic].vertex)[i].x;
				int y = ((m_gpc_poly.contour)[ic].vertex)[i].y;
				AppendCorner( x, y );
			}
			Close();
		}
	}
	Draw();
	FreeGpcPoly();
	return m_gpc_poly.num_contours;
}

// make a gpc_polygon for a closed polyline contour
// approximates arcs with multiple straight-line segments
// if icontour = -1, make polygon with all contours,
// combining intersecting contours if possible
//
int CPolyLine::MakeGpcPoly( int icontour )
{
	if( m_gpc_poly.num_contours )
		FreeGpcPoly();
	if( !GetClosed() && (icontour == (GetNumContours()-1) || icontour == -1))
		return 1;	// error

	// initialize m_gpc_poly
	m_gpc_poly.num_contours = 0;
	m_gpc_poly.hole = NULL;
	m_gpc_poly.contour = NULL;

	int first_contour = icontour;
	int last_contour = icontour;
	if( icontour == -1 )
	{
		first_contour = 0;
		last_contour = GetNumContours() - 1;
	}
	for( int icont=first_contour; icont<=last_contour; icont++ )
	{
		// make gpc_polygon for this contour
		gpc_polygon * gpc = new gpc_polygon;
		gpc->num_contours = 0;
		gpc->hole = NULL;
		gpc->contour = NULL;

		// first, calculate number of vertices in contour
		int n_vertices = 0;
		int ic_st = GetContourStart(icont);
		int ic_end = GetContourEnd(icont);
		for( int ic=ic_st; ic<=ic_end; ic++ )
		{
			int style = side_style[ic];
			int x1 = corner[ic].x;
			int y1 = corner[ic].y;
			int x2, y2;
			if( ic < ic_end )
			{
				x2 = corner[ic+1].x;
				y2 = corner[ic+1].y;
			}
			else
			{
				x2 = corner[ic_st].x;
				y2 = corner[ic_st].y;
			}
			if( style == STRAIGHT )
				n_vertices++;
			else
			{
				// style is ARC_CW or ARC_CCW
				int n;	// number of steps for arcs
				n = (abs(x2-x1)+abs(y2-y1))/(5*NM_PER_MIL);	// step size approx. 3 to 5 mil
				n = max( n, 18 );	// or at most 5 degrees of arc
				n_vertices += n;
			}
		}
		// now create gcp_vertex_list for this contour
		gpc_vertex_list * g_v_list = new gpc_vertex_list;
		g_v_list->vertex = (gpc_vertex*)calloc( sizeof(gpc_vertex), n_vertices );
		g_v_list->num_vertices = n_vertices;
		int ivtx = 0;
		for( int ic=ic_st; ic<=ic_end; ic++ )
		{
			int style = side_style[ic];
			int x1 = corner[ic].x;
			int y1 = corner[ic].y;
			int x2, y2;
			if( ic < (m_ncorners-1) )
			{
				x2 = corner[ic+1].x;
				y2 = corner[ic+1].y;
			}
			else
			{
				x2 = corner[0].x;
				y2 = corner[0].y;
			}
			if( style == STRAIGHT )
			{
				g_v_list->vertex[ivtx].x = x1;
				g_v_list->vertex[ivtx].y = y1;
				ivtx++;
			}
			else
			{
				// style is arc_cw or arc_ccw
				int n;	// number of steps for arcs
				n = (abs(x2-x1)+abs(y2-y1))/(5*NM_PER_MIL);	// step size approx. 3 to 5 mil
				n = max( n, 18 );	// or at most 5 degrees of arc
				double xo, yo, theta1, theta2, a, b;
				a = fabs( (double)(x1 - x2) );
				b = fabs( (double)(y1 - y2) );
				if( style == CPolyLine::ARC_CW )
				{
					// clockwise arc (ie.quadrant of ellipse)
					int i=0, j=0;
					if( x2 > x1 && y2 > y1 )
					{
						// first quadrant, draw second quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = pi;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						// second quadrant, draw third quadrant of ellipse
						xo = x1;	
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = pi;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						// third quadrant, draw fourth quadrant of ellipse
						xo = x2;	
						yo = y1;
						theta1 = 2.0*pi;
						theta2 = 3.0*pi/2.0;
					}
					else
					{
						xo = x1;	// fourth quadrant, draw first quadrant of ellipse
						yo = y2;
						theta1 = pi/2.0;
						theta2 = 0.0;
					}
				}
				else
				{
					// counter-clockwise arc
					int i=0, j=0;
					if( x2 > x1 && y2 > y1 )
					{
						xo = x1;	// first quadrant, draw fourth quadrant of ellipse
						yo = y2;
						theta1 = 3.0*pi/2.0;
						theta2 = 2.0*pi;
					}
					else if( x2 < x1 && y2 > y1 )
					{
						xo = x2;	// second quadrant
						yo = y1;
						theta1 = 0.0;
						theta2 = pi/2.0;
					}
					else if( x2 < x1 && y2 < y1 )	
					{
						xo = x1;	// third quadrant
						yo = y2;
						theta1 = pi/2.0;
						theta2 = pi;
					}
					else
					{
						xo = x2;	// fourth quadrant
						yo = y1;
						theta1 = pi;
						theta2 = 3.0*pi/2.0;
					}
				}
				// now write steps
				for( int is=1; is<=n; is++ )
				{
					double theta = theta1 + ((theta2-theta1)*(double)is)/n;
					double x = xo + a*cos(theta);
					double y = yo + b*sin(theta);
					if( is == n )
					{
						x = x2;
						y = y2;
					}
					g_v_list->vertex[ivtx].x = x;
					g_v_list->vertex[ivtx].y = y;
					ivtx++;
				}
			}
		}
		// add vertex_list to gpc
		gpc_add_contour( gpc, g_v_list, 0 );
		// now clip m_gpc_poly with gpc
		if( icontour == -1 && icont != 0 )
			gpc_polygon_clip( GPC_DIFF, &m_gpc_poly, gpc, &m_gpc_poly );	// hole
		else
			gpc_polygon_clip( GPC_UNION, &m_gpc_poly, gpc, &m_gpc_poly );	// outside
		gpc_free_polygon( gpc );
		delete gpc;
		delete g_v_list->vertex;
		delete g_v_list;
	}
	return 0;
}

int CPolyLine::FreeGpcPoly()
{
	if( m_gpc_poly.num_contours )
	{
		free( m_gpc_poly.contour->vertex );
		delete m_gpc_poly.contour;
		delete m_gpc_poly.hole;
	}
	m_gpc_poly.num_contours = 0;
	return 0;
}

// initialize new polyline
// set layer, width, selection box size, starting point, id and pointer
//
// if sel_box = 0, don't create selection elements at all
//
// if polyline is board outline, enter with:
//	id.type = ID_BOARD
//	id.st = ID_BOARD_OUTLINE
//	id.i = 0
//	ptr = NULL
//
// if polyline is copper area, enter with:
//	id.type = ID_NET;
//	id.st = ID_AREA
//	id.i = index to area
//	ptr = pointer to net
//
void CPolyLine::Start( int layer, int w, int sel_box, int x, int y, 
					  int hatch, id * id, void * ptr )
{
	m_layer = layer;
	m_w = w;
	m_sel_box = sel_box;
	if( id )
		m_id = *id;
	else
		m_id.Clear();
	m_ptr = ptr;
	m_ncorners = 1;
	m_hatch = hatch;
	corner.SetSize( 1 );
	side_style.SetSize( 0 );
	corner[0].x = x;
	corner[0].y = y;
	corner[0].end_contour = FALSE;
	if( m_sel_box && m_dlist )
	{
		dl_corner_sel.SetSize( 1 );
		m_id.sst = ID_SEL_CORNER;
		m_id.ii = 0;
		dl_corner_sel[0] = m_dlist->AddSelector( m_id, m_ptr, m_layer, DL_HOLLOW_RECT, 
			1, 0, 0, x-m_sel_box, y-m_sel_box, 
			x+m_sel_box, y+m_sel_box, 0, 0 );
	}
}

// add a corner to unclosed polyline
//
void CPolyLine::AppendCorner( int x, int y, int style )
{
	Undraw();
	// increase size of arrays
	corner.SetSize( m_ncorners+1 );
	side_style.SetSize( m_ncorners );

	// add entries for new corner and side
	corner[m_ncorners].x = x;
	corner[m_ncorners].y = y;
	corner[m_ncorners].end_contour = FALSE;
	if( !corner[m_ncorners-1].end_contour )
		side_style[m_ncorners-1] = style;
	int dl_type;
	if( style == CPolyLine::STRAIGHT )
		dl_type = DL_LINE;
	else if( style == CPolyLine::ARC_CW )
		dl_type = DL_ARC_CW;
	else if( style == CPolyLine::ARC_CCW )
		dl_type = DL_ARC_CCW;
	else
		ASSERT(0);
	m_ncorners++;
	Draw();
}

// close last polyline contour
//
void CPolyLine::Close( int style )
{
	if( GetClosed() )
		ASSERT(0);
	Undraw();
	side_style.SetAtGrow( m_ncorners-1, style );
	corner[m_ncorners-1].end_contour = TRUE;
	Draw();
}

// move corner of polyline
//
void CPolyLine::MoveCorner( int ic, int x, int y )
{
	Undraw();
	corner[ic].x = x;
	corner[ic].y = y;
	Draw();
}

// delete corner and adjust arrays
//
void CPolyLine::DeleteCorner( int ic )
{
	Undraw();
	int icont = GetContour( ic );
	int istart = GetContourStart( icont );
	int iend = GetContourEnd( icont );
	BOOL end_cont = corner[ic].end_contour; 
	BOOL closed = icont < GetNumContours()-1 || GetClosed();

	if( !closed )
	{
		// open contour, must be last contour
		corner.RemoveAt( ic ); 
		if( ic > istart )
			side_style.RemoveAt( ic-1 );
	}
	else
	{
		// closed contour
		corner.RemoveAt( ic ); 
		if( ic == istart )
			side_style.RemoveAt( iend );
		else
			side_style.RemoveAt( ic-1 );
		if( ic == iend )
			corner[ic-1].end_contour = TRUE;		
	}
	m_ncorners = corner.GetSize();
	if( closed && GetContourSize(icont) < 3 )
	{
		// delete the entire contour
		RemoveContour( icont );
	}
	m_ncorners = corner.GetSize();
	Draw();
}

void CPolyLine::RemoveContour( int icont )
{
	Undraw();
	int istart = GetContourStart( icont );
	int iend = GetContourEnd( icont );

	if( icont == 0 && GetNumContours() == 1 )
	{
		// remove the only contour
		ASSERT(0);
	}
	else if( icont == GetNumContours()-1 )
	{
		// remove last contour
		corner.SetSize( GetContourStart(icont) );
		side_style.SetSize( GetContourStart(icont) );
	}
	else
	{
		// remove closed contour
		for( int ic=iend; ic>=istart; ic-- )
		{
			corner.RemoveAt( ic );
			side_style.RemoveAt( ic );
		}
	}
	m_ncorners = corner.GetSize();
	Draw();
}

// insert a new corner between two existing corners
//
void CPolyLine::InsertCorner( int ic, int x, int y )
{
	Undraw();
	corner.InsertAt( ic, CPolyPt(x,y) );
	side_style.InsertAt( ic, STRAIGHT );
	m_ncorners++;
	if( ic )
	{
		if( corner[ic-1].end_contour )
		{
			corner[ic].end_contour = TRUE;
			corner[ic-1].end_contour = FALSE;
		}
	}
	Draw();
}

// undraw polyline by removing all graphic elements from display list
//
void CPolyLine::Undraw()
{
	if( m_dlist && bDrawn )
	{
		// remove display elements, if present
		for( int i=0; i<dl_side.GetSize(); i++ )
			m_dlist->Remove( dl_side[i] );
		for( int i=0; i<dl_side_sel.GetSize(); i++ )
			m_dlist->Remove( dl_side_sel[i] );
		for( int i=0; i<dl_corner_sel.GetSize(); i++ )
			m_dlist->Remove( dl_corner_sel[i] );
		for( int i=0; i<dl_hatch.GetSize(); i++ )
			m_dlist->Remove( dl_hatch[i] );

		// remove pointers
		dl_side.RemoveAll();
		dl_side_sel.RemoveAll();
		dl_corner_sel.RemoveAll();
		dl_hatch.RemoveAll();

		m_nhatch = 0;
	}
	bDrawn = FALSE;
}

// draw polyline by adding all graphics to display list
// if side style is ARC_CW or ARC_CCW but endpoints are not angled,
// convert to STRAIGHT
//
void CPolyLine::Draw(  CDisplayList * dl )
{
	int i_start_contour = 0;

	// first, undraw if necessary
	if( bDrawn )
		Undraw(); 

	// use new display list if provided
	if( dl )
		m_dlist = dl;

	if( m_dlist )
	{
		// set up CArrays
		dl_side.SetSize( m_ncorners );
		if( m_sel_box )
		{
			dl_side_sel.SetSize( m_ncorners );
			dl_corner_sel.SetSize( m_ncorners );
		}
		else
		{
			dl_side_sel.RemoveAll();
			dl_corner_sel.RemoveAll();
		}
		// now draw elements
		for( int ic=0; ic<m_ncorners; ic++ )
		{
			m_id.ii = ic;
			int xi = corner[ic].x;
			int yi = corner[ic].y;
			int xf, yf;
			if( corner[ic].end_contour == FALSE && ic < m_ncorners-1 )
			{
				xf = corner[ic+1].x;
				yf = corner[ic+1].y;
			}
			else
			{
				xf = corner[i_start_contour].x;
				yf = corner[i_start_contour].y;
				i_start_contour = ic+1;
			}
			// draw
			if( m_sel_box )
			{
				m_id.sst = ID_SEL_CORNER;
				dl_corner_sel[ic] = m_dlist->AddSelector( m_id, m_ptr, m_layer, DL_HOLLOW_RECT, 
					1, 0, 0, xi-m_sel_box, yi-m_sel_box, 
					xi+m_sel_box, yi+m_sel_box, 0, 0 );
			}
			if( ic<(m_ncorners-1) || corner[ic].end_contour )
			{
				// draw side
				if( xi == xf || yi == yf )
				{
					// if endpoints not angled, make side STRAIGHT
					side_style[ic] = STRAIGHT;
				}
				int g_type = DL_LINE;
				if( side_style[ic] == STRAIGHT )
					g_type = DL_LINE;
				else if( side_style[ic] == ARC_CW )
					g_type = DL_ARC_CW;
				else if( side_style[ic] == ARC_CCW )
					g_type = DL_ARC_CCW;
				m_id.sst = ID_SIDE;
				dl_side[ic] = m_dlist->Add( m_id, m_ptr, m_layer, g_type, 
					1, m_w, 0, xi, yi, xf, yf, 0, 0 );
				if( m_sel_box )
				{
					m_id.sst = ID_SEL_SIDE;
					dl_side_sel[ic] = m_dlist->AddSelector( m_id, m_ptr, m_layer, g_type, 
						1, m_w, 0, xi, yi, xf, yf, 0, 0 );
				}
			}
		}
		if( m_hatch )
			Hatch();
	}
	bDrawn = TRUE;
}

void CPolyLine::SetSideVisible( int is, int visible )
{
	if( m_dlist && dl_side.GetSize() > is )
	{
		m_dlist->Set_visible( dl_side[is], visible );
	}
}

// start dragging new corner to be inserted into side, make side and hatching invisible
//
void CPolyLine::StartDraggingToInsertCorner( CDC * pDC, int ic, int x, int y, int crosshair )
{
	if( !m_dlist )
		ASSERT(0);

	int icont = GetContour( ic );
	int istart = GetContourStart( icont );
	int iend = GetContourEnd( icont );
	int post_c;

	if( ic == iend )
		post_c = istart;
	else
		post_c = ic + 1;
	int xi = corner[ic].x;
	int yi = corner[ic].y;
	int xf = corner[post_c].x;
	int yf = corner[post_c].y;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, 
		LAY_SELECTION, LAY_SELECTION, 1, 1, DSS_ARC_STRAIGHT, DSS_ARC_STRAIGHT,
		0, 0, 0, 0, crosshair );
	m_dlist->CancelHighLight();
	m_dlist->Set_visible( dl_side[ic], 0 );
	for( int ih=0; ih<m_nhatch; ih++ )
		m_dlist->Set_visible( dl_hatch[ih], 0 );
}

// cancel dragging inserted corner, make side and hatching visible again
//
void CPolyLine::CancelDraggingToInsertCorner( int ic )
{
	if( !m_dlist )
		ASSERT(0);

	int post_c;
	if( ic == (m_ncorners-1) )
		post_c = 0;
	else
		post_c = ic + 1;
	m_dlist->StopDragging();
	m_dlist->Set_visible( dl_side[ic], 1 );
	for( int ih=0; ih<m_nhatch; ih++ )
		m_dlist->Set_visible( dl_hatch[ih], 1 );
}

// start dragging corner to new position, make adjacent sides and hatching invisible
//
void CPolyLine::StartDraggingToMoveCorner( CDC * pDC, int ic, int x, int y, int crosshair )
{
	if( !m_dlist )
		ASSERT(0);

	// see if corner is the first or last corner of an open contour
	int icont = GetContour( ic );
	int istart = GetContourStart( icont );
	int iend = GetContourEnd( icont );
	if( !GetClosed()
		&& icont == GetNumContours() - 1
		&& (ic == istart || ic == iend) )
	{
		// yes
		int style, xi, yi, iside;
		if( ic == istart )
		{
			// first corner
			iside = ic;
			xi = GetX( ic+1 );
			yi = GetY( ic+1 );
			style = GetSideStyle( iside );
			// reverse arc since we are drawing from corner 1 to 0
			if( style == CPolyLine::ARC_CW )
				style = CPolyLine::ARC_CCW;
			else if( style == CPolyLine::ARC_CCW )
				style = CPolyLine::ARC_CW;
		}
		else
		{
			// last corner
			iside = ic - 1;
			xi = GetX( ic-1 );
			yi = GetY( ic-1);
			style = GetSideStyle( iside );
		}		
		m_dlist->StartDraggingArc( pDC, style, GetX(ic), GetY(ic), xi, yi, LAY_SELECTION, 1, crosshair );
		m_dlist->CancelHighLight();
		m_dlist->Set_visible( dl_side[iside], 0 );
		for( int ih=0; ih<m_nhatch; ih++ )
			m_dlist->Set_visible( dl_hatch[ih], 0 );
	}
	else
	{
		// no
		// get indexes for preceding and following corners
		int pre_c, post_c;
		int poly_side_style1, poly_side_style2;
		int style1, style2;
		if( ic == istart )
		{
			pre_c = iend;
			post_c = istart+1;
			poly_side_style1 = side_style[iend];
			poly_side_style2 = side_style[istart];
		}
		else if( ic == iend )
		{
			// last side
			pre_c = ic-1;
			post_c = istart;
			poly_side_style1 = side_style[ic-1];
			poly_side_style2 = side_style[ic];
		}
		else
		{
			pre_c = ic-1;
			post_c = ic+1;
			poly_side_style1 = side_style[ic-1];
			poly_side_style2 = side_style[ic];
		}
		if( poly_side_style1 == STRAIGHT )
			style1 = DSS_ARC_STRAIGHT;
		else if( poly_side_style1 == ARC_CW )
			style1 = DSS_ARC_CW;
		else if( poly_side_style1 == ARC_CCW )
			style1 = DSS_ARC_CCW;
		if( poly_side_style2 == STRAIGHT )
			style2 = DSS_ARC_STRAIGHT;
		else if( poly_side_style2 == ARC_CW )
			style2 = DSS_ARC_CW;
		else if( poly_side_style2 == ARC_CCW )
			style2 = DSS_ARC_CCW;
		int xi = corner[pre_c].x;
		int yi = corner[pre_c].y;
		int xf = corner[post_c].x;
		int yf = corner[post_c].y;
		m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, 
			LAY_SELECTION, LAY_SELECTION, 1, 1, style1, style2, 
			0, 0, 0, 0, crosshair );
		m_dlist->CancelHighLight();
		m_dlist->Set_visible( dl_side[pre_c], 0 );
		m_dlist->Set_visible( dl_side[ic], 0 );
		for( int ih=0; ih<m_nhatch; ih++ )
			m_dlist->Set_visible( dl_hatch[ih], 0 );
	}
}

// cancel dragging corner to new position, make sides and hatching visible again
//
void CPolyLine::CancelDraggingToMoveCorner( int ic )
{
	if( !m_dlist )
		ASSERT(0);

	// get indexes for preceding and following sides
	int pre_c;
	if( ic == 0 )
	{
		pre_c = m_ncorners-1;
	}
	else
	{
		pre_c = ic-1;
	}
	m_dlist->StopDragging();
	m_dlist->Set_visible( dl_side[pre_c], 1 );
	m_dlist->Set_visible( dl_side[ic], 1 );
	for( int ih=0; ih<m_nhatch; ih++ )
		m_dlist->Set_visible( dl_hatch[ih], 1 );
}


// highlight side by drawing line over it
//
void CPolyLine::HighlightSide( int is )
{
	if( !m_dlist )
		ASSERT(0);
	if( GetClosed() && is >= m_ncorners )
		return;
	if( !GetClosed() && is >= (m_ncorners-1) )
		return;

	int style;
	if( side_style[is] == CPolyLine::STRAIGHT )
		style = DL_LINE;
	else if( side_style[is] == CPolyLine::ARC_CW )
		style = DL_ARC_CW;
	else if( side_style[is] == CPolyLine::ARC_CCW )
		style = DL_ARC_CCW;
	m_dlist->HighLight( style, 
		m_dlist->Get_x( dl_side_sel[is] ),
		m_dlist->Get_y( dl_side_sel[is] ),
		m_dlist->Get_xf( dl_side_sel[is] ),
		m_dlist->Get_yf( dl_side_sel[is] ),
		m_dlist->Get_w( dl_side_sel[is]) );
}

// highlight corner by drawing box around it
//
void CPolyLine::HighlightCorner( int ic )

{
	if( !m_dlist )
		ASSERT(0);

	m_dlist->HighLight( DL_HOLLOW_RECT, 
		m_dlist->Get_x( dl_corner_sel[ic] ),
		m_dlist->Get_y( dl_corner_sel[ic] ),
		m_dlist->Get_xf( dl_corner_sel[ic] ),
		m_dlist->Get_yf( dl_corner_sel[ic] ),
		m_dlist->Get_w( dl_corner_sel[ic]) );
}

int CPolyLine::GetX( int ic ) 
{	
	return corner[ic].x; 
}

int CPolyLine::GetY( int ic ) 
{	
	return corner[ic].y; 
}

int CPolyLine::GetEndContour( int ic ) 
{	
	return corner[ic].end_contour; 
}

void CPolyLine::MakeVisible( BOOL visible ) 
{	
	if( m_dlist )
	{
		int ns = m_ncorners-1;
		if( GetClosed() )
			ns = m_ncorners;
		for( int is=0; is<ns; is++ )
			m_dlist->Set_visible( dl_side[is], visible ); 
		for( int ih=0; ih<m_nhatch; ih++ )
			m_dlist->Set_visible( dl_hatch[ih], visible ); 
	}
} 

CRect CPolyLine::GetBounds()
{
	CRect r = GetCornerBounds();
	r.left -= m_w/2;
	r.right += m_w/2;
	r.bottom -= m_w/2;
	r.top += m_w/2;
	return r;
}

CRect CPolyLine::GetCornerBounds()
{
	CRect r;
	r.left = r.bottom = INT_MAX;
	r.right = r.top = INT_MIN;
	for( int i=0; i<m_ncorners; i++ )
	{
		r.left = min( r.left, corner[i].x );
		r.right = max( r.right, corner[i].x );
		r.bottom = min( r.bottom, corner[i].y );
		r.top = max( r.top, corner[i].y );
	}
	return r;
}

int CPolyLine::GetNumCorners() 
{	
	return m_ncorners;	
}

int CPolyLine::GetLayer() 
{	
	return m_layer;	
}

int CPolyLine::GetW() 
{	
	return m_w;	
}

int CPolyLine::GetSelBoxSize() 
{	
	return m_sel_box;	
}

int CPolyLine::GetNumContours()
{
	int ncont = 0;
	if( !m_ncorners )
		return 0;

	for( int ic=0; ic<m_ncorners; ic++ )
		if( corner[ic].end_contour )
			ncont++;
	if( !corner[m_ncorners-1].end_contour )
		ncont++;
	return ncont;
}

int CPolyLine::GetContour( int ic )
{
	int ncont = 0;
	for( int i=0; i<ic; i++ )
	{
		if( corner[i].end_contour )
			ncont++;
	}
	return ncont;
}

int CPolyLine::GetContourStart( int icont )
{
	if( icont == 0 )
		return 0;

	int ncont = 0;
	for( int i=0; i<m_ncorners; i++ )
	{
		if( corner[i].end_contour )
		{
			ncont++;
			if( ncont == icont )
				return i+1;
		}
	}
	ASSERT(0);
	return 0;
}

int CPolyLine::GetContourEnd( int icont )
{
	if( icont == GetNumContours()-1 )
		return m_ncorners-1;

	int ncont = 0;
	for( int i=0; i<m_ncorners; i++ )
	{
		if( corner[i].end_contour )
		{
			if( ncont == icont )
				return i;
			ncont++;
		}
	}
	ASSERT(0);
	return 0;
}

int CPolyLine::GetContourSize( int icont )
{
	return GetContourEnd(icont) - GetContourStart(icont) + 1;
}


void CPolyLine::SetSideStyle( int is, int style ) 
{	
	Undraw();
	CPoint p1, p2;
	if( is == (m_ncorners-1) )
	{
		p1.x = corner[m_ncorners-1].x;
		p1.y = corner[m_ncorners-1].y;
		p2.x = corner[0].x;
		p2.y = corner[0].y;
	}
	else
	{
		p1.x = corner[is].x;
		p1.y = corner[is].y;
		p2.x = corner[is+1].x;
		p2.y = corner[is+1].y;
	}
	if( p1.x == p2.x || p1.y == p2.y )
		side_style[is] = STRAIGHT;
	else
		side_style[is] = style;	
	Draw();
}

int CPolyLine::GetSideStyle( int is ) 
{	
	return side_style[is];	
}

// renumber ids
//
void CPolyLine::SetId( id * id )
{
	m_id = *id;
	if( m_dlist )
	{
		m_id.sst = ID_SIDE;
		for( int i=0; i<dl_side.GetSize(); i++ )
		{
			m_id.ii = i;
			m_dlist->Set_id( dl_side[i], &m_id );
		}
		m_id.sst = ID_SEL_SIDE;
		for( int i=0; i<dl_side_sel.GetSize(); i++ )
		{
			m_id.ii = i;
			m_dlist->Set_id( dl_side_sel[i], &m_id );
		}
		m_id.sst = ID_SEL_CORNER;
		for( int i=0; i<dl_corner_sel.GetSize(); i++ )
		{
			m_id.ii = i;
			m_dlist->Set_id( dl_corner_sel[i], &m_id );
		}
		m_id.sst = ID_HATCH;
		for( int ih=0; ih<dl_hatch.GetSize(); ih++ )
		{
			m_id.ii = ih;
			m_dlist->Set_id( dl_hatch[ih], &m_id ); 
		}
	}
}

// get root id
//
id CPolyLine::GetId()
{
	return m_id;
}

int CPolyLine::GetClosed() 
{	
	if( m_ncorners == 0 )
		return 0;
	else
		return corner[m_ncorners-1].end_contour; 
}

// draw hatch lines
//
void CPolyLine::Hatch()
{
	if( m_hatch == NO_HATCH )
	{
		m_nhatch = 0;
		return;
	}

	if( m_dlist && GetClosed() )
	{
		enum {
			MAXPTS = 100,
			MAXLINES = 1000
		};
		dl_hatch.SetSize( MAXLINES, MAXLINES );
		int xx[MAXPTS], yy[MAXPTS];

		// define range for hatch lines
		int min_x = corner[0].x;
		int max_x = corner[0].x;
		int min_y = corner[0].y;
		int max_y = corner[0].y;
		for( int ic=1; ic<m_ncorners; ic++ )
		{
			if( corner[ic].x < min_x )
				min_x = corner[ic].x;
			if( corner[ic].x > max_x )
				max_x = corner[ic].x;
			if( corner[ic].y < min_y )
				min_y = corner[ic].y;
			if( corner[ic].y > max_y )
				max_y = corner[ic].y;
		}
		int slope_flag = 1 - 2*(m_layer%2);	// 1 or -1
		double slope = 0.707106*slope_flag;
		int spacing;
		if( m_hatch == DIAGONAL_EDGE )
			spacing = 10*PCBU_PER_MIL;
		else
			spacing = 50*PCBU_PER_MIL;
		int max_a, min_a;
		if( slope_flag == 1 )
		{
			max_a = (int)(max_y - slope*min_x);
			min_a = (int)(min_y - slope*max_x);
		}
		else
		{
			max_a = (int)(max_y - slope*max_x);
			min_a = (int)(min_y - slope*min_x);
		}
		min_a = (min_a/spacing)*spacing;
		int offset;
		if( m_layer < (LAY_TOP_COPPER+2) )
			offset = 0;
		else if( m_layer < (LAY_TOP_COPPER+4) )
			offset = spacing/2;
		else if( m_layer < (LAY_TOP_COPPER+6) )
			offset = spacing/4;
		else if( m_layer < (LAY_TOP_COPPER+8) )
			offset = 3*spacing/4;
		min_a += offset;

		// now calculate and draw hatch lines
		int nc = m_ncorners;
		int nhatch = 0;
		// loop through hatch lines
		for( int a=min_a; a<max_a; a+=spacing )
		{
			// get intersection points for this hatch line
			int nloops = 0;
			int npts;
			// make this a loop in case my homebrew hatching algorithm screws up
			do
			{
				npts = 0;
				int i_start_contour = 0;
				for( ic=0; ic<nc; ic++ )
				{
					double x, y, x2, y2;
					int ok;
					if( corner[ic].end_contour )
					{
						ok = FindLineSegmentIntersection( a, slope, 
								corner[ic].x, corner[ic].y,
								corner[i_start_contour].x, corner[i_start_contour].y, 
								side_style[ic],
								&x, &y, &x2, &y2 );
						i_start_contour = ic + 1;
					}
					else
					{
						ok = FindLineSegmentIntersection( a, slope, 
								corner[ic].x, corner[ic].y, 
								corner[ic+1].x, corner[ic+1].y,
								side_style[ic],
								&x, &y, &x2, &y2 );
					}
					if( ok )
					{
						xx[npts] = (int)x;
						yy[npts] = (int)y;
						npts++;
						ASSERT( npts<MAXPTS );	// overflow
					}
					if( ok == 2 )
					{
						xx[npts] = (int)x2;
						yy[npts] = (int)y2;
						npts++;
						ASSERT( npts<MAXPTS );	// overflow
					}
				}
				nloops++;
				a += PCBU_PER_MIL/100;
			} while( npts%2 != 0 && nloops < 3 );
			ASSERT( npts%2==0 );	// odd number of intersection points, error

			// sort points in order of descending x (if more than 2)
			if( npts>2 )
			{
				for( int istart=0; istart<(npts-1); istart++ )
				{
					int max_x = INT_MIN;
					int imax;
					for( int i=istart; i<npts; i++ )
					{
						if( xx[i] > max_x )
						{
							max_x = xx[i];
							imax = i;
						}
					}
					int temp = xx[istart];
					xx[istart] = xx[imax];
					xx[imax] = temp;
					temp = yy[istart];
					yy[istart] = yy[imax];
					yy[imax] = temp;
				}
			}

			// draw lines
			for( int ip=0; ip<npts; ip+=2 )
			{
				id hatch_id = m_id;
				hatch_id.sst = ID_HATCH;
				hatch_id.ii = nhatch;
				double dx = xx[ip+1] - xx[ip];
				if( m_hatch == DIAGONAL_FULL || fabs(dx) < 40*NM_PER_MIL )
				{
					dl_element * dl = m_dlist->Add( hatch_id, 0, m_layer, DL_LINE, 1, 0, 0, 
						xx[ip], yy[ip], xx[ip+1], yy[ip+1], 0, 0 );
					dl_hatch.SetAtGrow(nhatch, dl);
					nhatch++;
				}
				else
				{
					double dy = yy[ip+1] - yy[ip];	
					double slope = dy/dx;
					if( dx > 0 )
						dx = 20*NM_PER_MIL;
					else
						dx = -20*NM_PER_MIL;
					double x1 = xx[ip] + dx;
					double x2 = xx[ip+1] - dx;
					double y1 = yy[ip] + dx*slope;
					double y2 = yy[ip+1] - dx*slope;
					dl_element * dl = m_dlist->Add( hatch_id, 0, m_layer, DL_LINE, 1, 0, 0, 
						xx[ip], yy[ip], x1, y1, 0, 0 );
					dl_hatch.SetAtGrow(nhatch, dl);
					dl = m_dlist->Add( hatch_id, 0, m_layer, DL_LINE, 1, 0, 0, 
						xx[ip+1], yy[ip+1], x2, y2, 0, 0 );
					dl_hatch.SetAtGrow(nhatch+1, dl);
					nhatch += 2;
				}
			}
		} // end for 
		m_nhatch = nhatch;
		dl_hatch.SetSize( m_nhatch );
	}
}

// test to see if a point is inside polyline
//
BOOL CPolyLine::TestPointInside( int x, int y )
{
	enum { MAXPTS = 100 };
	if( !GetClosed() )
		ASSERT(0);

	// define line passing through (x,y), with slope = 2/3;
	// get intersection points
	double xx[MAXPTS], yy[MAXPTS];
	double slope = (double)2.0/3.0;
	double a = y - slope*x;
	int nloops = 0;
	int npts;
	// make this a loop so if my homebrew algorithm screws up, we try it again
	do
	{
		// now find all intersection points of line with polyline sides
		npts = 0;
		for( int icont=0; icont<GetNumContours(); icont++ )
		{
			int istart = GetContourStart( icont );
			int iend = GetContourEnd( icont );
			for( int ic=istart; ic<=iend; ic++ )
			{
				double x, y, x2, y2;
				int ok;
				if( ic == istart )
					ok = FindLineSegmentIntersection( a, slope, 
					corner[iend].x, corner[iend].y,
					corner[istart].x, corner[istart].y, 
					side_style[m_ncorners-1],
					&x, &y, &x2, &y2 );
				else
					ok = FindLineSegmentIntersection( a, slope, 
					corner[ic-1].x, corner[ic-1].y, 
					corner[ic].x, corner[ic].y,
					side_style[ic-1],
					&x, &y, &x2, &y2 );
				if( ok )
				{
					xx[npts] = (int)x;
					yy[npts] = (int)y;
					npts++;
					ASSERT( npts<MAXPTS );	// overflow
				}
				if( ok == 2 )
				{
					xx[npts] = (int)x2;
					yy[npts] = (int)y2;
					npts++;
					ASSERT( npts<MAXPTS );	// overflow
				}
			}
		}
		nloops++;
		a += PCBU_PER_MIL/100;
	} while( npts%2 != 0 && nloops < 3 );
	ASSERT( npts%2==0 );	// odd number of intersection points, error

	// count intersection points to right of (x,y), if odd (x,y) is inside polyline
	int ncount = 0;
	for( int ip=0; ip<npts; ip++ )
	{
		if( xx[ip] == x && yy[ip] == y )
			return FALSE;	// (x,y) is on a side, call it outside
		else if( xx[ip] > x )
			ncount++;
	}
	if( ncount%2 )
		return TRUE;
	else
		return FALSE;
}

// Test for intersection of sides
//
int CPolyLine::TestIntersection( CPolyLine * poly )
{
	if( !GetClosed() )
		ASSERT(0);
	if( !poly->GetClosed() )
		ASSERT(0);
	for( int ic=0; ic<GetNumContours(); ic++ )
	{
		int istart = GetContourStart(ic);
		int iend = GetContourEnd(ic);
		for( int is=istart; is<=iend; is++ )
		{
			int xi = GetX(is);
			int yi = GetY(is);
			int xf, yf;
			if( is < GetContourEnd(ic) )
			{
				xf = GetX(is+1);
				yf = GetY(is+1);
			}
			else
			{
				xf = GetX(istart);
				yf = GetY(istart);
			}
			int style = GetSideStyle(is);
			for( int ic2=0; ic2<poly->GetNumContours(); ic2++ )
			{
				int istart2 = poly->GetContourStart(ic2);
				int iend2 = poly->GetContourEnd(ic2);
				for( int is2=istart2; is2<=iend2; is2++ )
				{
					int xi2 = poly->GetX(is2);
					int yi2 = poly->GetY(is2);
					int xf2, yf2;
					if( is2 < poly->GetContourEnd(ic2) )
					{
						xf2 = poly->GetX(is2+1);
						yf2 = poly->GetY(is2+1);
					}
					else
					{
						xf2 = poly->GetX(istart2);
						yf2 = poly->GetY(istart2);
					}
					int style2 = poly->GetSideStyle(is2);
					// test for intersection between side and side2
				}
			}
		}
	}
	return 0;
}

// set selection box size 
//
void CPolyLine::SetSelBoxSize( int sel_box )
{
//	Undraw();
	m_sel_box = sel_box;
//	Draw();
}

// set pointer to display list, and draw into display list
//
void CPolyLine::SetDisplayList( CDisplayList * dl )
{
	if( m_dlist )
		Undraw();
	m_dlist = dl;
	if( m_dlist )
		Draw();
}

// copy data from another poly, but don't draw it
//
void CPolyLine::Copy( CPolyLine * src )
{
	Undraw();
	m_dlist = src->m_dlist;
	m_id = src->m_id;
	m_ptr = src->m_ptr;
	m_layer = src->m_layer;
	m_w = src->m_w;
	m_sel_box = src->m_sel_box;
	m_ncorners = src->m_ncorners;
	m_hatch = src->m_hatch;
	m_nhatch = src->m_nhatch;
	// copy corners
	corner.SetSize( m_ncorners );
	for( int i=0; i<m_ncorners; i++ )
		corner[i] = src->corner[i];
	// copy side styles
	int nsides = src->side_style.GetSize();
	side_style.SetSize(nsides);
	for( i=0; i<nsides; i++ )
		side_style[i] = src->side_style[i];
	// don't copy the Gpc_poly, just clear the old one
	FreeGpcPoly();
}

void CPolyLine::MoveOrigin( int x_off, int y_off )
{
	Undraw();
	for( int ic=0; ic<GetNumCorners(); ic++ )
	{
		SetX( ic, GetX(ic) + x_off );
		SetY( ic, GetY(ic) + y_off );
	}
	Draw();
}


// Set various parameters:
//   the calling function should Undraw() before calling them,
//   and Draw() after
//
void CPolyLine::SetX( int ic, int x ) { corner[ic].x = x; }
void CPolyLine::SetY( int ic, int y ) { corner[ic].y = y; }
void CPolyLine::SetEndContour( int ic, BOOL end_contour ) { corner[ic].end_contour = end_contour; }
void CPolyLine::SetLayer( int layer ) { m_layer = layer; }
void CPolyLine::SetW( int w ) { m_w = w; }


