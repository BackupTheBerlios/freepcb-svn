// PartList.cpp : implementation of class CPartList
//
// this is a linked-list of parts on a PCB board
//
#include "stdafx.h"
#include <math.h>

#define PL_MAX_SIZE		5000		// default max. size

//******** constructors and destructors *********
 
cpart::cpart()
{
	// zero out pointers
	dl_sel = 0;
	dl_ref_sel = 0;
	shape = 0;
	drawn = FALSE;
}

cpart::~cpart()
{
}

CPartList::CPartList( CDisplayList * dlist, SMFontUtil * fontutil )
{
	m_start.prev = 0;		// dummy first element in list
	m_start.next = &m_end;
	m_end.next = 0;			// dummy last element in list
	m_end.prev = &m_start;
	m_max_size = PL_MAX_SIZE;	// size limit
	m_size = 0;					// current size
	m_dlist = dlist;
	m_fontutil = fontutil;
	m_footprint_cache_map = NULL;
}

CPartList::~CPartList()
{
	// traverse list, removing all parts
	while( m_end.prev != &m_start )
		Remove( m_end.prev );
}

// Create new empty part and add to end of list
// return pointer to element created.
//
cpart * CPartList::Add()
{
	if(m_size >= m_max_size )
	{
		AfxMessageBox( "Maximum number of parts exceeded" );
		return 0;
	}

	// create new instance and link into list
	cpart * part = new cpart;
	part->prev = m_end.prev;
	part->next = &m_end;
	part->prev->next = part;
	part->next->prev = part;

	return part;
}

// Create new part, add to end of list, set part data 
// return pointer to element created.
//
cpart * CPartList::Add( CShape * shape, CString * ref_des, CString * package, 
							int x, int y, int side, int angle, int visible, int glued )
{
	if(m_size >= m_max_size )
	{
		AfxMessageBox( "Maximum number of parts exceeded" );
		return 0;
	}

	// create new instance and link into list
	cpart * part = Add();
	// set data
	SetPartData( part, shape, ref_des, package, x, y, side, angle, visible, glued );
	return part;
}

// Set part data
//
int CPartList::SetPartData( cpart * part, CShape * shape, CString * ref_des, CString * package, 
							int x, int y, int side, int angle, int visible, int glued )
{
	UndrawPart( part );

	// now copy data into part
	id id( ID_PART );
	part->visible = visible;
	part->ref_des = *ref_des;
	if( package )
		part->package = *package;
	else
		part->package = "??????";
	part->m_id = id;
	part->x = x;
	part->y = y;
	part->side = side;
	part->angle = angle;
	part->glued = glued;

	if( !shape )
	{
		part->shape = NULL;
		part->pin.SetSize(0);
		part->m_ref_xi = 0;
		part->m_ref_yi = 0;
		part->m_ref_angle = 0;
		part->m_ref_size = 0;
		part->m_ref_w = 0;
	}
	else
	{
		part->shape = shape;
		part->pin.SetSize( shape->m_padstack.GetSize() );
		part->m_ref_xi = shape->m_ref_xi;
		part->m_ref_yi = shape->m_ref_yi;
		part->m_ref_angle = shape->m_ref_angle;
		part->m_ref_size = shape->m_ref_size;
		part->m_ref_w = shape->m_ref_w;
	}

	part->m_outline_stroke.SetSize(0);
	part->ref_text_stroke.SetSize(0);
	m_size++;

	// now draw part instance into display list
	if( part->shape )
		DrawPart( part );

	return 0;
}

// Highlight part
//
int CPartList::HighlightPart( cpart * part )
{
	// highlight it by making its selection rectangle visible
	m_dlist->HighLight( DL_HOLLOW_RECT, 
				m_dlist->Get_x( part->dl_sel) , 
				m_dlist->Get_y( part->dl_sel),
				m_dlist->Get_xf(part->dl_sel), 
				m_dlist->Get_yf(part->dl_sel), 1 );
	return 0;
}

// Highlight part ref_text
//
int CPartList::SelectRefText( cpart * part )
{
	// highlight it by making its selection rectangle visible
	if( part->dl_ref_sel )
	{
		m_dlist->HighLight( DL_HOLLOW_RECT, 
			m_dlist->Get_x(part->dl_ref_sel), 
			m_dlist->Get_y(part->dl_ref_sel),
			m_dlist->Get_xf(part->dl_ref_sel), 
			m_dlist->Get_yf(part->dl_ref_sel), 1 );
	}
	return 0;
}

void CPartList:: HighlightAllPadsOnNet( cnet * net )
{
	cpart * part = GetFirstPart();
	while( part )
	{
		if( part->shape )
		{
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				if( net == part->pin[ip].net )
					SelectPad( part, ip );
			}
		}
		part = GetNextPart( part );
	}
}

// Select part pad
//
int CPartList::SelectPad( cpart * part, int i )
{
	// select it by making its selection rectangle visible
	if( part->pin[i].dl_sel )
	{
		m_dlist->HighLight( DL_RECT_X, 
			m_dlist->Get_x(part->pin[i].dl_sel), 
			m_dlist->Get_y(part->pin[i].dl_sel),
			m_dlist->Get_xf(part->pin[i].dl_sel), 
			m_dlist->Get_yf(part->pin[i].dl_sel), 
			1, GetPinLayer( part, i ) );
	}
	return 0;
}

// Test for hit on pad
//
BOOL CPartList::TestHitOnPad( cpart * part, CString * pin_name, int x, int y, int layer )
{
	if( !part )
		return FALSE;
	if( !part->shape )
		return FALSE;
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	if( pin_index == -1 )
		return FALSE;

	int xx = part->pin[pin_index].x;
	int yy = part->pin[pin_index].y;
	padstack * ps = &part->shape->m_padstack[pin_index];
	pad * p;
	if( ps->hole_size == 0 )
	{
		// SMT pad
		if( layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else
			return FALSE;
	}
	else
	{
		// TH pad
		if( layer == LAY_TOP_COPPER && part->side == 0 )
			p = &ps->top;
		else if( layer == LAY_TOP_COPPER && part->side == 1 )
			p = &ps->bottom;
		else if( layer == LAY_BOTTOM_COPPER && part->side == 1 )
			p = &ps->top;
		else if( layer == LAY_BOTTOM_COPPER && part->side == 0 )
			p = &ps->bottom;
		else
			p = &ps->inner;
	}
	double dx = abs( xx-x );
	double dy = abs( yy-y );
	double dist = sqrt( dx*dx + dy*dy );
	if( dist < ps->hole_size/2 )
		return TRUE;
	switch( p->shape )
	{
	case PAD_NONE: 
		break;
	case PAD_ROUND: 
		if( dist < (p->size_h/2) ) 
			return TRUE; 
		break;
	case PAD_SQUARE:
		if( dx < (p->size_h/2) && dy < (p->size_h/2) )
			return TRUE;
		break;
	case PAD_RECT:
	case PAD_RRECT:
	case PAD_OVAL:
		int pad_angle = part->angle + ps->angle;
		if( pad_angle > 270 )
			pad_angle -= 360;
		if( pad_angle == 0 || pad_angle == 180 )
		{
			if( dx < (p->size_l) && dy < (p->size_h/2) )
				return TRUE;
		}
		else
		{
			if( dx < (p->size_h/2) && dy < (p->size_l) )
				return TRUE;
		}
		break;
	}
	return FALSE;
}


// Move element with given id to new position, angle and side
// x and y are in world coords
//
int CPartList::Move( cpart * part, int x, int y, int angle, int side )
{
	// remove all display list elements
	UndrawPart( part );
	// move part
	part->x = x;
	part->y = y;
	part->angle = angle;
	part->side = side;
	// now redraw it
	DrawPart( part );
	return PL_NOERR;
}

// Move ref text with given id to new position and angle
// x and y are in absolute world coords
// angle is relative to part angle
//
CPartList::MoveRefText( cpart * part, int x, int y, int angle, int size, int w )
{
	// remove all display list elements
	UndrawPart( part );
	
	// get position of new text box origin relative to part
	CPoint part_org, tb_org;
	tb_org.x = x - part->x;
	tb_org.y = y - part->y;
	
	// correct for rotation of part
	RotatePoint( &tb_org, 360-part->angle, zero );
	
	// correct for part on bottom of board (reverse relative x-axis)
	if( part->side == 1 )
		tb_org.x = -tb_org.x;
	
	// reset ref text position
	part->m_ref_xi = tb_org.x;
	part->m_ref_yi = tb_org.y;
	part->m_ref_angle = angle;
	part->m_ref_size = size;
	part->m_ref_w = w;
	
	// now redraw part
	DrawPart( part );
	return PL_NOERR;
}

// Resize ref text for part
//
void CPartList::ResizeRefText( cpart * part, int size, int width )
{
	if( part->shape )
	{
		// remove all display list elements
		UndrawPart( part );
		// change ref text size
		part->m_ref_size = size;
		part->m_ref_w = width;	
		// now redraw part
		DrawPart( part );
	}
}

// Get side of part
//
int CPartList::GetSide( cpart * part )
{
	return part->side;
}

// Get angle of part
//
int CPartList::GetAngle( cpart * part )
{
	return part->angle;
}

// Get angle of ref text for part
//
int CPartList::GetRefAngle( cpart * part )
{
	return part->m_ref_angle;
}

CPoint CPartList::GetRefPoint( cpart * part )
{
	CPoint ref_pt;

	// move origin of text box to position relative to part
	ref_pt.x = part->m_ref_xi;
	ref_pt.y = part->m_ref_yi;
	// flip if part on bottom
	if( part->side )
	{
		ref_pt.x = -ref_pt.x;
	}
	// rotate with part about part origin
	RotatePoint( &ref_pt, part->angle, zero );
	ref_pt.x += part->x;
	ref_pt.y += part->y;
	return ref_pt;
}

// Get pin info from part
//
CPoint CPartList::GetPinPoint( cpart * part, CString * pin_name )
{
	// get pin coords relative to part origin
	CPoint pp;
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	if( pin_index == -1 )
		ASSERT(0);
	pp.x = part->shape->m_padstack[pin_index].x_rel;
	pp.y = part->shape->m_padstack[pin_index].y_rel;
	// flip if part on bottom
	if( part->side )
	{
		pp.x = -pp.x;
	}
	// rotate if necess.
	int angle = part->angle;
	if( angle > 0 )
	{
		CPoint org;
		org.x = 0;
		org.y = 0;
		RotatePoint( &pp, angle, org );
	}
	// add coords of part origin
	pp.x = part->x + pp.x;
	pp.y = part->y + pp.y;
	return pp;
}

// Get pin layer
// returns LAY_TOP_COPPER, LAY_BOTTOM_COPPER or LAY_PAD_THRU
//
CPartList::GetPinLayer( cpart * part, CString * pin_name )
{
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	return GetPinLayer( part, pin_index );
}

// Get pin layer
// returns LAY_TOP_COPPER, LAY_BOTTOM_COPPER or LAY_PAD_THRU
//
CPartList::GetPinLayer( cpart * part, int pin_index )
{
	if( part->shape->m_padstack[pin_index].hole_size )
		return LAY_PAD_THRU;
	else if( part->side == 0 && part->shape->m_padstack[pin_index].top.shape != PAD_NONE 
		|| part->side == 1 && part->shape->m_padstack[pin_index].bottom.shape != PAD_NONE )
		return LAY_TOP_COPPER;
	else
		return LAY_BOTTOM_COPPER;
}

// Get pin net
//
cnet * CPartList::GetPinNet( cpart * part, CString * pin_name )
{
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	return part->pin[pin_index].net;
}

// Get pin net
//
cnet * CPartList::GetPinNet( cpart * part, int pin_index )
{
	return part->pin[pin_index].net;
}

// Get pin pad width
// enter with pin_num = pin # (1-based)
//
CPartList::GetPinWidth( cpart * part, CString * pin_name )
{
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	return( part->shape->m_padstack[pin_index].top.size_h );
}

// Get bounding rect for all pins
// Currently, just uses selection rect
// returns 1 if successful
//
int CPartList::GetPartBoundingRect( cpart * part, CRect * part_r )
{
	CRect r;

	if( !part )
		return 0;
	if( !part->shape )
		return 0;
	if( part->dl_sel )
	{
		r.left = NM_PER_MIL * min( part->dl_sel->x, part->dl_sel->xf );
		r.right = NM_PER_MIL * max( part->dl_sel->x, part->dl_sel->xf );
		r.bottom = NM_PER_MIL * min( part->dl_sel->y, part->dl_sel->yf );
		r.top = NM_PER_MIL * max( part->dl_sel->y, part->dl_sel->yf );
		*part_r = r;
		return 1;
	}
	return 0;
}

// get bounds of rectangle which encloses all pins
// return 0 if no parts found, else return 1
//
int CPartList::GetPartBoundaries( CRect * part_r )
{
	int min_x = INT_MAX;
	int max_x = INT_MIN;
	int min_y = INT_MAX;
	int max_y = INT_MIN;
	int parts_found = 0;
	// iterate
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if( part->dl_sel )
		{
			int x = part->dl_sel->x;
			int y = part->dl_sel->y;
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			x = part->dl_sel->xf;
			y = part->dl_sel->yf;
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			parts_found = 1;
		}
		if( part->dl_ref_sel )
		{
			int x = part->dl_ref_sel->x;
			int y = part->dl_ref_sel->y;
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			x = part->dl_ref_sel->xf;
			y = part->dl_ref_sel->yf;
			max_x = max( x, max_x);
			min_x = min( x, min_x);
			max_y = max( y, max_y);
			min_y = min( y, min_y);
			parts_found = 1;
		}
		part = part->next;
	}
	part_r->left = min_x * NM_PER_MIL;
	part_r->right = max_x * NM_PER_MIL;
	part_r->bottom = min_y * NM_PER_MIL;
	part_r->top = max_y * NM_PER_MIL;
	return parts_found;
}

// Get pointer to part in part_list with given ref
//
cpart * CPartList::GetPart( CString * ref_des )
{
	// find element with given ref_des, return pointer to element
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if(  part->ref_des == *ref_des  )
			return part;
		part = part->next;
	}
	return NULL;	// if unable to find part
}

// Iterate through parts
//
cpart * CPartList::GetFirstPart()
{
	cpart * p = m_start.next;
	if( p->next )
		return p;
	else
		return NULL;
}

cpart * CPartList::GetNextPart( cpart * part )
{
	cpart * p = part->next;
	if( !p )
		return NULL;
	if( !p->next )
		return NULL;
	else
		return p;
}

// get number of times a particular shape is used
//
int CPartList::GetNumFootprintInstances( CShape * shape )
{
	int n = 0;

	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if(  part->shape == shape  )
			n++;
		part = part->next;
	}
	return n;
}

// Purge unused footprints from cache
//
void CPartList::PurgeFootprintCache()
{
	POSITION pos;
	CString key;
	void * ptr;

	if( !m_footprint_cache_map )
		ASSERT(0);

	for( pos = m_footprint_cache_map->GetStartPosition(); pos != NULL; )
	{
		m_footprint_cache_map->GetNextAssoc( pos, key, ptr );
		CShape * shape = (CShape*)ptr;
		if( GetNumFootprintInstances( shape ) == 0 )
		{
			// purge this footprint
			delete shape;
			m_footprint_cache_map->RemoveKey( key );
		}
	}
}

// Remove part from list and delete it
//
int CPartList::Remove( cpart * part )
{
	// delete all entries in display list
	UndrawPart( part );

	// remove links to this element
	part->next->prev = part->prev;
	part->prev->next = part->next;
	// destroy part
	m_size--;
	delete( part );

	return 0;
}

// Remove all parts from list
//
void CPartList::RemoveAllParts()
{
	// traverse list, removing all parts
	while( m_end.prev != &m_start )
		Remove( m_end.prev );
}

// Set utility flag for all parts
//
void CPartList::MarkAllParts( int mark )
{
	cpart * part = GetFirstPart();
	while( part )
	{
		part->utility = mark;
		part = GetNextPart( part );
	}
}

// Draw part into display list
int CPartList::DrawPart( cpart * part )
{
	int i;

	// this part
	CShape * shape = part->shape;
	int x = part->x;
	int y = part->y;
	int angle = part->angle;
	id id = part->m_id;

	// draw selection rectangle (layer = top or bottom copper, depending on side)
	CRect sel;
	int sel_layer;
	if( !part->side )
	{
		// part on top
		sel.left = shape->m_sel_xi; 
		sel.right = shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
		sel_layer = LAY_SELECTION;
	}
	else
	{
		// part on bottom
		sel.right = - shape->m_sel_xi;
		sel.left = - shape->m_sel_xf;
		sel.bottom = shape->m_sel_yi;
		sel.top = shape->m_sel_yf;
		sel_layer = LAY_SELECTION;
	}
	if( angle > 0 )
		RotateRect( &sel, angle, zero );
	id.st = ID_SEL_RECT;
	part->dl_sel = m_dlist->AddSelector( id, part, sel_layer, DL_HOLLOW_RECT, 1,
		0, 0, x + sel.left, y + sel.bottom, x + sel.right, y + sel.top, x, y );
	m_dlist->Set_sel_vert( part->dl_sel, 0 );
	if( angle == 90 || angle ==  270 )
		m_dlist->Set_sel_vert( part->dl_sel, 1 );
	
	// draw ref designator text
	int silk_lay = LAY_SILK_TOP;
	if( part->side )
		silk_lay = LAY_SILK_BOTTOM;

	int nstrokes = 0;
	CArray<stroke> m_stroke;
	m_stroke.SetSize( 1000 );
	id.st = ID_STROKE;

	double x_scale = (double)part->m_ref_size/22.0;
	double y_scale = (double)part->m_ref_size/22.0;
	double y_offset = 9.0*y_scale;
	i = 0;
	double xc = 0.0;
	CPoint si, sf;
	int w = part->m_ref_w;
	int xmin = INT_MAX;
	int xmax = INT_MIN;
	int ymin = INT_MAX;
	int ymax = INT_MIN;
	for( int ic=0; ic<part->ref_des.GetLength(); ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes = m_fontutil->GetCharStrokes( part->ref_des[ic], SIMPLEX, 
			&min_x, &min_y, &max_x, &max_y, coord, 64 );
		for( int is=0; is<nstrokes; is++ )
		{
			xi = (coord[is][0] - min_x)*x_scale + xc;
			yi = coord[is][1]*y_scale + y_offset;
			xf = (coord[is][2] - min_x)*x_scale + xc;
			yf = coord[is][3]*y_scale + y_offset;
			xmax = max( xi+w/2, xmax );
			xmax = max( xf+w/2, xmax );
			xmin = min( xi-w/2, xmin );
			xmin = min( xf-w/2, xmin );
			ymax = max( yi+w/2, ymax );
			ymax = max( yf+w/2, ymax );
			ymin = min( yi-w/2, ymin );
			ymin = min( yf-w/2, ymin );
			// get stroke relative to text box
			if( yi > yf )
			{
				si.x = xi;
				sf.x = xf;
				si.y = yi;
				sf.y = yf;
			}
			else
			{
				si.x = xf;
				sf.x = xi;
				si.y = yf;
				sf.y = yi;
			}
			// rotate about text box origin
			RotatePoint( &si, part->m_ref_angle, zero );
			RotatePoint( &sf, part->m_ref_angle, zero );
			// move origin of text box to position relative to part
			si.x += part->m_ref_xi;
			sf.x += part->m_ref_xi;
			si.y += part->m_ref_yi;
			sf.y += part->m_ref_yi;
			// flip if part on bottom
			if( part->side )
			{
				si.x = -si.x;
				sf.x = -sf.x;
			}
			// rotate with part about part origin
			RotatePoint( &si, angle, zero );
			RotatePoint( &sf, angle, zero );
			// add x, y to part origin and draw
			id.i = i;
			m_stroke[i].w = part->m_ref_w;
			m_stroke[i].xi = x + si.x;
			m_stroke[i].yi = y + si.y;
			m_stroke[i].xf = x + sf.x;
			m_stroke[i].yf = y + sf.y;
			m_stroke[i].dl_el = m_dlist->Add( id, this, 
				silk_lay, DL_LINE, 1, part->m_ref_w, 0, 
				x+si.x, y+si.y, x+sf.x, y+sf.y, 0, 0 );
			i++;
			if( i >= m_stroke.GetSize() )
				m_stroke.SetSize( i + 100 );
		}
		xc += (max_x - min_x + 8.0)*x_scale;
	}
	m_stroke.SetSize( i );
	part->ref_text_stroke.SetSize( i );
	for( int is=0; is<i; is++ )
	{
		part->ref_text_stroke[is] = m_stroke[is];
	}

	// draw selection rectangle for ref text
	// get text box relative to ref text origin, angle = 0
	si.x = xmin;
	sf.x = xmax;
	si.y = ymin;
	sf.y = ymax;
	// rotate to ref text angle
	RotatePoint( &si, part->m_ref_angle, zero );
	RotatePoint( &sf, part->m_ref_angle, zero );
	// move to position relative to part
	si.x += part->m_ref_xi;
	sf.x += part->m_ref_xi;
	si.y += part->m_ref_yi;
	sf.y += part->m_ref_yi;
	// flip if part on bottom
	if( part->side )
	{
		si.x = -si.x;
		sf.x = -sf.x;
	}
	// rotate to part angle
	RotatePoint( &si, angle, zero );
	RotatePoint( &sf, angle, zero );
	id.st = ID_SEL_REF_TXT;
	// move to part position and draw
	part->dl_ref_sel = m_dlist->AddSelector( id, part, silk_lay, DL_HOLLOW_RECT, 1,
		0, 0, x + si.x, y + si.y, x + sf.x, y + sf.y, x + si.x, y + si.y );

	// draw part outline
	part->m_outline_stroke.SetSize(0);
	for( int ip=0; ip<shape->m_outline_poly.GetSize(); ip++ )
	{
		int pos = part->m_outline_stroke.GetSize();
		int nsides;
		if( shape->m_outline_poly[ip].GetClosed() )
			nsides = shape->m_outline_poly[ip].GetNumCorners();
		else
			nsides = shape->m_outline_poly[ip].GetNumCorners() - 1;
		part->m_outline_stroke.SetSize( pos + nsides );
		int w = shape->m_outline_poly[ip].GetW();
		for( i=0; i<nsides; i++ )
		{
			int g_type;
			if( shape->m_outline_poly[ip].GetSideStyle( i ) == CPolyLine::STRAIGHT )
				g_type = DL_LINE;
			else if( shape->m_outline_poly[ip].GetSideStyle( i ) == CPolyLine::ARC_CW )
				g_type = DL_ARC_CW;
			else if( shape->m_outline_poly[ip].GetSideStyle( i ) == CPolyLine::ARC_CCW )
				g_type = DL_ARC_CCW;
			si.x = shape->m_outline_poly[ip].GetX( i );
			si.y = shape->m_outline_poly[ip].GetY( i );
			if( i == (nsides-1) && shape->m_outline_poly[ip].GetClosed() )
			{
				sf.x = shape->m_outline_poly[ip].GetX( 0 );
				sf.y = shape->m_outline_poly[ip].GetY( 0 );
			}
			else
			{
				sf.x = shape->m_outline_poly[ip].GetX( i+1 );
				sf.y = shape->m_outline_poly[ip].GetY( i+1 );
			}
			// flip if part on bottom
			if( part->side )
			{
				si.x = -si.x;
				sf.x = -sf.x;
				if( g_type == DL_ARC_CW )
					g_type = DL_ARC_CCW;
				else if( g_type == DL_ARC_CCW )
					g_type = DL_ARC_CW;
			}
			// rotate with part and draw
			RotatePoint( &si, angle, zero );
			RotatePoint( &sf, angle, zero );
			part->m_outline_stroke[i+pos].xi = x+si.x;
			part->m_outline_stroke[i+pos].xf = x+sf.x;
			part->m_outline_stroke[i+pos].yi = y+si.y;
			part->m_outline_stroke[i+pos].yf = y+sf.y;
			part->m_outline_stroke[i+pos].type = g_type;
			part->m_outline_stroke[i+pos].w = w;
			part->m_outline_stroke[i+pos].dl_el = m_dlist->Add( part->m_id, part, silk_lay, 
				g_type, 1, w, 0, x+si.x, y+si.y, x+sf.x, y+sf.y, 0, 0 );
		}
	}

	// draw text
	for( int it=0; it<part->shape->m_tl->text_ptr.GetSize(); it++ )
	{
		CText * t = part->shape->m_tl->text_ptr[it];
		int nstrokes = 0;
		CArray<stroke> m_stroke;
		m_stroke.SetSize( 1000 );
		id.st = ID_STROKE;

		double x_scale = (double)t->m_font_size/22.0;
		double y_scale = (double)t->m_font_size/22.0;
		double y_offset = 9.0*y_scale;
		i = 0;
		double xc = 0.0;
		CPoint si, sf;
		int w = t->m_stroke_width;
		int xmin = INT_MAX;
		int xmax = INT_MIN;
		int ymin = INT_MAX;
		int ymax = INT_MIN;
		for( int ic=0; ic<t->m_str.GetLength(); ic++ )
		{
			// get stroke info for character
			int xi, yi, xf, yf;
			double coord[64][4];
			double min_x, min_y, max_x, max_y;
			int nstrokes = m_fontutil->GetCharStrokes( t->m_str[ic], SIMPLEX, 
				&min_x, &min_y, &max_x, &max_y, coord, 64 );
			for( int is=0; is<nstrokes; is++ )
			{
				xi = (coord[is][0] - min_x)*x_scale + xc;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (coord[is][2] - min_x)*x_scale + xc;
				yf = coord[is][3]*y_scale + y_offset;
				xmax = max( xi+w/2, xmax );
				xmax = max( xf+w/2, xmax );
				xmin = min( xi-w/2, xmin );
				xmin = min( xf-w/2, xmin );
				ymax = max( yi+w/2, ymax );
				ymax = max( yf+w/2, ymax );
				ymin = min( yi-w/2, ymin );
				ymin = min( yf-w/2, ymin );
				// get stroke relative to text box
				if( yi > yf )
				{
					si.x = xi;
					sf.x = xf;
					si.y = yi;
					sf.y = yf;
				}
				else
				{
					si.x = xf;
					sf.x = xi;
					si.y = yf;
					sf.y = yi;
				}
				// rotate about text box origin
				RotatePoint( &si, t->m_angle, zero );
				RotatePoint( &sf, t->m_angle, zero );
				// move origin of text box to position relative to part
				si.x += t->m_x;
				sf.x += t->m_x;
				si.y += t->m_y;
				sf.y += t->m_y;
				// flip if part on bottom
				if( part->side )
				{
					si.x = -si.x;
					sf.x = -sf.x;
				}
				// rotate with part about part origin
				RotatePoint( &si, angle, zero );
				RotatePoint( &sf, angle, zero );
				// add x, y to part origin and draw
				id.i = i;
				m_stroke[i].type = DL_LINE;
				m_stroke[i].w = part->m_ref_w;
				m_stroke[i].xi = x + si.x;
				m_stroke[i].yi = y + si.y;
				m_stroke[i].xf = x + sf.x;
				m_stroke[i].yf = y + sf.y;
				m_stroke[i].dl_el = m_dlist->Add( id, this, 
					silk_lay, DL_LINE, 1, t->m_stroke_width, 0, 
					x+si.x, y+si.y, x+sf.x, y+sf.y, 0, 0 );
				i++;
				if( i >= m_stroke.GetSize() )
					m_stroke.SetSize( i + 100 );
			}
			xc += (max_x - min_x + 8.0)*x_scale;
		}
		for( int is=0; is<i; is++ )
		{
			part->m_outline_stroke.Add( m_stroke[is] );
		}
	}

	// draw padstacks and save absolute position of pins
	CPoint pin_pt;
	CPoint pad_pi;
	CPoint pad_pf;
	for( i=0; i<shape->GetNumPins(); i++ ) 
	{
		// set layer for pads
		padstack * ps = &shape->m_padstack[i];
		part_pin * pin = &part->pin[i];
		pin->dl_els.SetSize(m_layers);
		pad * p;
		int pad_layer;
		// iterate through all copper layers 
		pad * any_pad = NULL;
		for( int il=0; il<m_layers; il++ )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			pad_layer = il + LAY_TOP_COPPER;
			pin->dl_els[il] = NULL;
			// get appropriate pad
			padstack * ps = &shape->m_padstack[i];
			pad * p = NULL;
			if( pad_layer == LAY_TOP_COPPER && part->side == 0 )
				p = &ps->top;
			else if( pad_layer == LAY_TOP_COPPER && part->side == 1 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && part->side == 0 )
				p = &ps->bottom;
			else if( pad_layer == LAY_BOTTOM_COPPER && part->side == 1 )
				p = &ps->top;
			else if( ps->hole_size )
				p = &ps->inner;
			int sel_layer = pad_layer;
			if( ps->hole_size )
				sel_layer = LAY_SELECTION;
			if( p )
			{
				if( p->shape != PAD_NONE )
					any_pad = p;

				// draw pad
				dl_element * pad_el = NULL;
				if( p->shape == PAD_NONE )
				{
				}
				else if( p->shape == PAD_ROUND )
				{
					// flip if part on bottom
					if( part->side )
						pin_pt.x = -pin_pt.x;
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					// add to display list
					id.st = ID_PAD;
					id.i = i;
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = m_dlist->Add( id, part, pad_layer, 
						DL_CIRC, 1, 
						p->size_h,
						0, 
						x + pin_pt.x, y + pin_pt.y, 0, 0, 0, 0 );
					if( !pin->dl_sel )
					{
						id.st = ID_SEL_PAD;
						pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,  
							pin->y-p->size_h/2, 
							pin->x+p->size_h/2, 
							pin->y+p->size_h/2, 0, 0 );
					}
				}
				else if( p->shape == PAD_SQUARE )
				{
					// flip if part on bottom
					if( part->side )
					{
						pin_pt.x = -pin_pt.x;
					}
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					id.st = ID_PAD;
					id.i = i;
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = m_dlist->Add( part->m_id, part, pad_layer, 
						DL_SQUARE, 1, 
						p->size_h,
						0, 
						pin->x, pin->y, 
						0, 0, 
						0, 0 );
					if( !pin->dl_sel )
					{
						id.st = ID_SEL_PAD;
						pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,  
							pin->y-p->size_h/2, 
							pin->x+p->size_h/2, 
							pin->y+p->size_h/2, 0, 0 );
					}
				}
				else if( p->shape == PAD_RECT 
					|| p->shape == PAD_RRECT 
					|| p->shape == PAD_OVAL )
				{
					int gtype;
					if( p->shape == PAD_RECT )
						gtype = DL_RECT;
					else if( p->shape == PAD_RRECT )
						gtype = DL_RRECT;
					else
						gtype = DL_OVAL;
					pad_pi.x = pin_pt.x - p->size_l;
					pad_pi.y = pin_pt.y - p->size_h/2;
					pad_pf.x = pin_pt.x + p->size_r;
					pad_pf.y = pin_pt.y + p->size_h/2;
					// rotate pad about pin if necessary
					if( shape->m_padstack[i].angle > 0 )
					{
						RotatePoint( &pad_pi, ps->angle, pin_pt );
						RotatePoint( &pad_pf, ps->angle, pin_pt );
					}

					// flip if part on bottom
					if( part->side )
					{
						pin_pt.x = -pin_pt.x;
						pad_pi.x = -pad_pi.x;
						pad_pf.x = -pad_pf.x;
					}
					// rotate part about 
					if( angle > 0 )
					{
						RotatePoint( &pin_pt, angle, zero );
						RotatePoint( &pad_pi, angle, zero );
						RotatePoint( &pad_pf, angle, zero );
					}
					id.st = ID_PAD;
					id.i = i;
					int radius = p->radius;
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = m_dlist->Add( part->m_id, part, pad_layer, 
						gtype, 1, 
						0,
						0, 
						x + pad_pi.x, y + pad_pi.y, 
						x + pad_pf.x, y + pad_pf.y, 
						x + pin_pt.x, y + pin_pt.y, 
						p->radius );
					if( !pin->dl_sel )
					{
						id.st = ID_SEL_PAD;
						pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							x + pad_pi.x, y + pad_pi.y, 
							x + pad_pf.x, y + pad_pf.y,
							0, 0 );
					}
				}
				else if( p->shape == PAD_OCTAGON )
				{
					// flip if part on bottom
					if( part->side )
					{
						pin_pt.x = -pin_pt.x;
					}
					// rotate
					if( angle > 0 )
						RotatePoint( &pin_pt, angle, zero );
					id.st = ID_PAD;
					id.i = i;
					pin->x = x + pin_pt.x;
					pin->y = y + pin_pt.y;
					pad_el = m_dlist->Add( part->m_id, part, pad_layer, 
						DL_OCTAGON, 1, 
						p->size_h,
						0, 
						pin->x, pin->y, 
						0, 0, 
						0, 0 );
					if( !pin->dl_sel )
					{
						id.st = ID_SEL_PAD;
						pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
							DL_HOLLOW_RECT, 1, 1, 0,
							pin->x-p->size_h/2,  
							pin->y-p->size_h/2, 
							pin->x+p->size_h/2, 
							pin->y+p->size_h/2, 0, 0 );
					}
				}
				pin->dl_els[il] = pad_el;
				pin->dl_hole = pad_el;
			}
		}
		// if through-hole pad, just draw hole and set pin_dl_el;
		if( ps->hole_size )
		{
			pin_pt.x = ps->x_rel;
			pin_pt.y = ps->y_rel;
			// flip if part on bottom
			if( part->side )
			{
				pin_pt.x = -pin_pt.x;
			}
			// rotate
			if( angle > 0 )
				RotatePoint( &pin_pt, angle, zero );
			// add to display list
			id.st = ID_PAD;
			id.i = i;
			pin->x = x + pin_pt.x;
			pin->y = y + pin_pt.y;
			pin->dl_hole = m_dlist->Add( id, part, LAY_PAD_THRU, 
								DL_HOLE, 1, 
								ps->hole_size,
								0, 
								pin->x, pin->y, 0, 0, 0, 0 );  
			if( !pin->dl_sel )
			{
				// make selector for pin with hole only
				id.st = ID_SEL_PAD;
				pin->dl_sel = m_dlist->AddSelector( id, part, sel_layer, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin->x-ps->hole_size/2,  
					pin->y-ps->hole_size/2,  
					pin->x+ps->hole_size/2,  
					pin->y+ps->hole_size/2,  
					0, 0 );
			}
		}
		else
			pin->dl_hole = NULL;
	}
	part->drawn = TRUE;
	return PL_NOERR;
}

// Undraw part from display list
//
int CPartList::UndrawPart( cpart * part )
{
	int i;

	if( part->drawn == FALSE )
		return 0;

	CShape * shape = part->shape;
	if( shape )
	{
		// undraw selection rectangle
		m_dlist->Remove( part->dl_sel );
		part->dl_sel = 0;

		// undraw selection rectangle for ref text
		m_dlist->Remove( part->dl_ref_sel );
		part->dl_ref_sel = 0;

		// undraw ref designator text
		int nstrokes = part->ref_text_stroke.GetSize();
		for( i=0; i<nstrokes; i++ )
		{
			m_dlist->Remove( part->ref_text_stroke[i].dl_el );
			part->ref_text_stroke[i].dl_el = 0;
		}

		// undraw part outline
		for( i=0; i<part->m_outline_stroke.GetSize(); i++ )
		{
			m_dlist->Remove( (dl_element*)part->m_outline_stroke[i].dl_el );
			part->m_outline_stroke[i].dl_el = 0;
		}

		// undraw padstacks
		for( i=0; i<shape->GetNumPins(); i++ )
		{
			part_pin * pin = &part->pin[i];
			if( pin->dl_els.GetSize()>0 )
			{
				for( int il=0; il<pin->dl_els.GetSize(); il++ )
				{
					if( pin->dl_els[il] != pin->dl_hole )
						m_dlist->Remove( pin->dl_els[il] );
				}
				pin->dl_els.RemoveAll();
			}
			m_dlist->Remove( pin->dl_hole );
			m_dlist->Remove( pin->dl_sel );
			pin->dl_hole = NULL;
			pin->dl_sel = NULL;
		}
	}
	part->drawn = FALSE;
	return PL_NOERR;
}

// the footprint was changed for a particular part
//
void CPartList::PartFootprintChanged( cpart * part, CShape * shape )
{
	UndrawPart( part );
	part->shape = shape;
	part->pin.SetSize( shape->GetNumPins() );
	DrawPart( part );
	m_nlist->PartFootprintChanged( part );
}

// the footprint was modified, apply to all parts using it
//
void CPartList::FootprintChanged( CShape * shape )
{
	// find all parts with given shape and update them
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if(  part->shape->m_name == shape->m_name  )
		{
			PartFootprintChanged( part, shape );
		}
		part = part->next;
	}
}

// the ref text height and width were modified, apply to all parts using it
//
void CPartList::RefTextSizeChanged( CShape * shape )
{
	// find all parts with given shape and update them
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		if(  part->shape->m_name == shape->m_name  )
		{
			ResizeRefText( part, shape->m_ref_size, shape->m_ref_w );
		}
		part = part->next;
	}
}

// Make part visible or invisible, including thermal reliefs
//
void CPartList::MakePartVisible( cpart * part, BOOL bVisible )
{
	// make part elements invisible, including copper area connections
	// outline strokes
	for( int i=0; i<part->m_outline_stroke.GetSize(); i++ )
	{
		dl_element * el = part->m_outline_stroke[i].dl_el;
		el->visible = bVisible;
	}
	// pins
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		// pin pads
		dl_element * el = part->pin[ip].dl_hole;
		if( el )
			el->visible = bVisible;
		for( int i=0; i<part->pin[ip].dl_els.GetSize(); i++ )
		{
			if( part->pin[ip].dl_els[i] )
				part->pin[ip].dl_els[i]->visible = bVisible;
		}
		// pin copper area connections
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			for( int ia=0; ia<net->nareas; ia++ )
			{
				for( int i=0; i<net->area[ia].npins; i++ )
				{
					if( net->pin[net->area[ia].pin[i]].part == part )
					{
						m_dlist->Set_visible( net->area[ia].dl_thermal[i], bVisible );
					}
				}
			}
		}
	}
	// ref text strokes
	for( int is=0; is<part->ref_text_stroke.GetSize(); is++ )
	{
		void * ptr = part->ref_text_stroke[is].dl_el;
		dl_element * el = (dl_element*)ptr;
		el->visible = bVisible;
	}
}

// Start dragging part by setting up display list
//
int CPartList::StartDraggingPart( CDC * pDC, cpart * part )
{
	// make part invisible
	MakePartVisible( part, FALSE );
	m_dlist->CancelHighLight();

	// create drag lines
	CPoint zero(0,0);
	m_dlist->MakeDragLineArray( 2*part->shape->m_padstack.GetSize() + 4 );
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
	m_dlist->AddDragLine( p1, p2 ); 
	m_dlist->AddDragLine( p2, p3 ); 
	m_dlist->AddDragLine( p3, p4 ); 
	m_dlist->AddDragLine( p4, p1 ); 

	// add up all the connections that need dragging and make array for ratlines
	int n_drag_ratlines = 0;
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		// get endpoints for any connection segments
		cnet * n = (cnet*)part->pin[ip].net;
		if( n )
		{
			if( n->visible )
			{
				for( int ic=0; ic<n->nconnects; ic++ )
				{
					int pin1 = n->connect[ic].start_pin;
					cpart * pin1_part = n->pin[pin1].part;
					int pin1_index = pin1_part->shape->GetPinIndexByName( &n->pin[pin1].pin_name );
					cpart * pin2_part = NULL;
					int pin2 = n->connect[ic].end_pin;
					int pin2_index = -1;
					if( pin2 != cconnect::NO_END )
					{
						pin2_part = n->pin[pin2].part;
						pin2_index = pin2_part->shape->GetPinIndexByName( &n->pin[pin2].pin_name );
					}
					int nsegs = n->connect[ic].nsegs;
					if( pin1_part == part && pin1_index == ip )
					{
						// ip is the start pin for the connection
						if( pin2_part != part || nsegs > 1 )
						{
							// segment does not end on this part, need to drag ratline
							n_drag_ratlines++;
						}
					}
					else if( pin2_part == part && pin2_index == ip )
					{
						// ip is the end pin for the connection
						if( pin1_part != part || nsegs > 1 )
						{
							// start pin is not on this part, need to drag ratline
							n_drag_ratlines++;
						}
					}
				}
			}
		}
	}
	m_dlist->MakeDragRatlineArray( n_drag_ratlines, 1 );

	// now loop through all pins in part
	for( ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		// make X for each pin
		int d = part->shape->m_padstack[ip].top.size_h/2;
		CPoint p(part->shape->m_padstack[ip].x_rel,part->shape->m_padstack[ip].y_rel);
		xi = p.x-d;
		yi = p.y-d;
		xf = p.x+d;
		yf = p.y+d;
		// reverse if on other side of board
		if( part->side )
		{
			xi = -xi;
			xf = -xf;
			p.x = -p.x;
		}
		p1.x = xi;
		p1.y = yi;
		p2.x = xf;
		p2.y = yi;
		p3.x = xf;
		p3.y = yf;
		p4.x = xi;
		p4.y = yf;
		// rotate by part.angle
		RotatePoint( &p1, part->angle, zero );
		RotatePoint( &p2, part->angle, zero );
		RotatePoint( &p3, part->angle, zero );
		RotatePoint( &p4, part->angle, zero );
		RotatePoint( &p, part->angle, zero );
		// draw X
		m_dlist->AddDragLine( p1, p3 ); 
		m_dlist->AddDragLine( p2, p4 );
		// get endpoints for any connection segments
		cnet * n = (cnet*)part->pin[ip].net;
		if( n )
		{
			if( n->visible )
			{
				for( int ic=0; ic<n->nconnects; ic++ )
				{
					int pin1 = n->connect[ic].start_pin;
					cpart * pin1_part = n->pin[pin1].part;
					int pin1_index = pin1_part->shape->GetPinIndexByName( &n->pin[pin1].pin_name );
					cpart * pin2_part = NULL;
					int pin2 = n->connect[ic].end_pin;
					int pin2_index = -1;
					if( pin2 != cconnect::NO_END )
					{
						pin2_part = n->pin[pin2].part;
						pin2_index = pin2_part->shape->GetPinIndexByName( &n->pin[pin2].pin_name );
					}
					int nsegs = n->connect[ic].nsegs;
					if( pin1_part == part && pin1_index == ip )
					{
						// ip is the start pin for the connection
						int xi = n->connect[ic].vtx[0].x;
						int yi = n->connect[ic].vtx[0].y;
						// OK, get next vertex, add ratline and hide segment
						if( pin2_part != part || nsegs > 1 )
						{
							CPoint vx( n->connect[ic].vtx[1].x, n->connect[ic].vtx[1].y );
							m_dlist->AddDragRatline( vx, p );
						}
						m_dlist->Set_visible( n->connect[ic].seg[0].dl_el, 0 );
					}
					else if( pin2_part == part && pin2_index == ip )
					{
						// ip is the end pin for the connection
						int xi = n->connect[ic].vtx[nsegs].x;
						int yi = n->connect[ic].vtx[nsegs].y;
						// OK, get prev vertex, add ratline and hide segment
						if( pin1_part != part || nsegs > 1 )
						{
							CPoint vx( n->connect[ic].vtx[nsegs-1].x, n->connect[ic].vtx[nsegs-1].y );
							m_dlist->AddDragRatline( vx, p );
						}
						m_dlist->Set_visible( n->connect[ic].seg[nsegs-1].dl_el, 0 );
					}
				}
			}
		}
	}
	int vert = 0;
	if( part->angle == 90 || part->angle == 270 )
		vert = 1;
	m_dlist->StartDragging( pDC, part->x, part->y, vert, LAY_SELECTION );
	return 0;
}

// start dragging ref text
//
int CPartList::StartDraggingRefText( CDC * pDC, cpart * part )
{
	// make ref text elements invisible
	for( int is=0; is<part->ref_text_stroke.GetSize(); is++ )
	{
		void * ptr = part->ref_text_stroke[is].dl_el;
		dl_element * el = (dl_element*)ptr;
		el->visible = 0;
	}
	// cancel selection 
	m_dlist->CancelHighLight();
	// drag
	m_dlist->StartDraggingRectangle( pDC, 
						m_dlist->Get_x(part->dl_ref_sel), 
						m_dlist->Get_y(part->dl_ref_sel),
						m_dlist->Get_x(part->dl_ref_sel) - m_dlist->Get_x_org(part->dl_ref_sel),
						m_dlist->Get_y(part->dl_ref_sel) - m_dlist->Get_y_org(part->dl_ref_sel),
						m_dlist->Get_xf(part->dl_ref_sel) - m_dlist->Get_x_org(part->dl_ref_sel),
						m_dlist->Get_yf(part->dl_ref_sel) - m_dlist->Get_y_org(part->dl_ref_sel), 
						0, LAY_SELECTION );
	return 0;
}

// cancel dragging, return to pre-dragging state
//
int CPartList::CancelDraggingPart( cpart * part )
{
	// make part visible again
	MakePartVisible( part, TRUE );

	// get any connecting segments and make visible
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		cnet * n = (cnet*)part->pin[ip].net;
		if( n )
		{
			if( n->visible )
			{
				for( int ic=0; ic<n->nconnects; ic++ )
				{
					int pin1 = n->connect[ic].start_pin;
					int pin2 = n->connect[ic].end_pin;
					int nsegs = n->connect[ic].nsegs;
					if( n->pin[pin1].part == part )
					{
						// start pin
						m_dlist->Set_visible( n->connect[ic].seg[0].dl_el, 1 );
					}
					if( pin2 != cconnect::NO_END )
					{
						if( n->pin[pin2].part == part )
						{
							// end pin
							m_dlist->Set_visible( n->connect[ic].seg[nsegs-1].dl_el, 1 );
						}
					}
				}
			}
		}
	}
	m_dlist->StopDragging();
	return 0;
}

// cancel dragging of ref text, return to pre-dragging state
int CPartList::CancelDraggingRefText( cpart * part )
{
	// make ref text elements invisible
	for( int is=0; is<part->ref_text_stroke.GetSize(); is++ )
	{
		void * ptr = part->ref_text_stroke[is].dl_el;
		dl_element * el = (dl_element*)ptr;
		el->visible = 1;
	}
	m_dlist->StopDragging();
	return 0;
}

// normal completion of any dragging operation
//
int CPartList::StopDragging()
{
	m_dlist->StopDragging();
	return 0;
}

// create part from string
//
cpart * CPartList::AddFromString( CString * str )
{
	CShape * s = NULL;
	CString in_str, key_str;
	CArray<CString> p;
	int pos = 0;
	int len = str->GetLength();
	int np;
	CString ref_des;
	int ref_size = 0;
	int ref_width = 0;
	int ref_angle = 0;
	int ref_xi = 0;
	int ref_yi = 0;
	CString package;
	int x;
	int y;
	int side;
	int angle;
	int glued;
	cpart * part = Add();

	in_str = str->Tokenize( "\n", pos );
	while( in_str != "" )
	{
		np = ParseKeyString( &in_str, &key_str, &p );
		if( key_str == "ref" )
		{
			ref_des = in_str.Right( in_str.GetLength()-4 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( key_str == "part" )
		{
			ref_des = in_str.Right( in_str.GetLength()-5 );
			ref_des.Trim();
			ref_des = ref_des.Left(MAX_REF_DES_SIZE);
		}
		else if( key_str == "ref_text" )
		{
			if( np == 6 )
			{
				ref_size = my_atoi( &p[0] );
				ref_width = my_atoi( &p[1] );
				ref_angle = my_atoi( &p[2] );
				ref_xi = my_atoi( &p[3] );
				ref_yi = my_atoi( &p[4] );
			}
		}
		else if( key_str == "package" )
		{
			if( np == 2 )
				package = p[0];
			else
				package = "";
			package = package.Left(CShape::MAX_NAME_SIZE);
		}
		else if( key_str == "shape" )
		{
			// lookup shape in cache
			s = NULL;
			void * ptr;
			CString name = p[0];
			name = name.Left(CShape::MAX_NAME_SIZE);
			int err = m_footprint_cache_map->Lookup( name, ptr );
			if( err )
			{
				// found in cache
				s = (CShape*)ptr; 
			}
		}
		else if( key_str == "pos" )
		{
			if( np == 6 )
			{
				x = my_atoi( &p[0] );
				y = my_atoi( &p[1] );
				side = my_atoi( &p[2] );
				angle = my_atoi( &p[3] );
				glued = my_atoi( &p[4] );
			}
			else
			{
				x = 0;
				y = 0;
				side = 0;
				angle = 0;
				glued = 0;
			}
		}
		in_str = str->Tokenize( "\n", pos );
	}
	SetPartData( part, s, &ref_des, &package, x, y, side, angle, 1, glued );
	if( part->shape )
	{
		part->m_ref_xi = ref_xi;
		part->m_ref_yi = ref_yi;
		part->m_ref_angle = ref_angle;
		ResizeRefText( part, ref_size, ref_width );
	}
	return part;
}

// read partlist
//
int CPartList::ReadParts( CStdioFile * pcb_file )
{
	int pos, err;
	CString in_str, key_str;
	CArray<CString> p;

	// find beginning of [parts] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess;
			mess.Format( "Unable to find [parts] section in file" );
			AfxMessageBox( mess );
			return 0;
		}
		in_str.Trim();
	}
	while( in_str != "[parts]" );

	// get each part in [parts] section
	while( 1 )
	{
		pos = pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString( "unexpected EOF in project file" );
			throw err_str;
		}
		in_str.Trim();
		if( in_str[0] == '[' && in_str != "[parts]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			break;		// next section, exit
		}
		else if( in_str.Left(4) == "ref:" || in_str.Left(5) == "part:" )
		{
			CString str;
			do
			{
				str.Append( in_str );
				str.Append( "\n" );
				pos = pcb_file->GetPosition();
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );
					throw err_str;
				}
				in_str.Trim();
			} while( (in_str.Left(4) != "ref:" && in_str.Left(5) != "part:" )
						&& in_str[0] != '[' );
			pcb_file->Seek( pos, CFile::begin );

			// now add part to partlist
			cpart * part = AddFromString( &str );
		}
	}
	return 0;
}

// set CString to description of part
//
int CPartList::SetPartString( cpart * part, CString * str )
{
	CString line;

	line.Format( "part: %s\n", part->ref_des );
	*str = line;
	if( part->shape )
		line.Format( "  ref_text: %d %d %d %d %d\n", part->m_ref_size, 
		part->m_ref_w, part->m_ref_angle,
		part->m_ref_xi, part->m_ref_yi );
	else
		line.Format( "  ref_text: \n" );
	str->Append( line );
	line.Format( "  package: \"%s\"\n", part->package );
	str->Append( line );
	if( part->shape )
		line.Format( "  shape: \"%s\"\n", part->shape->m_name );
	else
		line.Format( "  shape: \n" );
	str->Append( line );
	line.Format( "  pos: %d %d %d %d %d\n", part->x, part->y, part->side, part->angle, part->glued );
	str->Append( line );

	line.Format( "\n" );
	str->Append( line );
	return 0;
}

// create record describing part for use by CUndoList
//
void * CPartList::CreatePartUndoRecord( cpart * part )
{
	int size = sizeof( undo_part ) + part->shape->GetNumPins()*(CShape::MAX_PIN_NAME_SIZE+1);
	undo_part * upart = (undo_part*)malloc( size );
	// set pointer to pin net name array
	char * chptr = (char*)upart;
	chptr += sizeof(undo_part);

	upart->m_plist = this;
	upart->m_id = part->m_id;
	upart->visible = part->visible;
	upart->x = part->x;
	upart->y = part->y;
	upart->side = part->side;
	upart->angle = part->angle;
	upart->glued = part->glued;
	upart->m_ref_xi = part->m_ref_xi;
	upart->m_ref_yi = part->m_ref_yi;
	upart->m_ref_angle = part->m_ref_angle;
	upart->m_ref_size = part->m_ref_size;
	upart->m_ref_w = part->m_ref_w;
	strcpy( upart->ref_des, part->ref_des );
	strcpy( upart->package , part->package );
	strcpy( upart->shape_name, part->shape->m_name );
	upart->shape = part->shape;
	if( part->shape )
	{
		// save names of nets attached to each pin
		for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
		{
			if( cnet * net = part->pin[ip].net )
				strcpy( chptr, net->name );
			else
				*chptr = 0;
			chptr += CShape::MAX_PIN_NAME_SIZE + 1;
		}
	}
	return (void*)upart;
}

// write all parts and footprints to file
//
int CPartList::WriteParts( CStdioFile * file )
{
	CMapStringToPtr shape_map;
	cpart * el = m_start.next;
	CString line;
	CString key;
	try
	{
		// now write all parts
		line.Format( "[parts]\n\n" );
		file->WriteString( line );
		el = m_start.next;
		while( el->next != 0 )
		{
			// test
			CString test;
			SetPartString( el, &test );
			file->WriteString( test );
			el = el->next;
		}
		
	}
	catch( CFileException * e )
	{
		CString str;
		if( e->m_lOsError == -1 )
			str.Format( "File error: %d\n", e->m_cause );
		else
			str.Format( "File error: %d %ld (%s)\n", e->m_cause, e->m_lOsError,
			_sys_errlist[e->m_lOsError] );
		return 1;
	}

	return 0;
}

// utility function to rotate a point clockwise about another point
// currently, angle must be 0, 90, 180 or 270
//
CPartList::RotatePoint( CPoint *p, int angle, CPoint org )
{
	CRect tr;
	if( angle == 90 )
	{
		int tempy = org.y + (org.x - p->x);
		p->x = org.x + (p->y - org.y);
		p->y = tempy;
	}
	else if( angle > 90 )
	{
		for( int i=0; i<(angle/90); i++ )
			RotatePoint( p, 90, org );
	}
	return PL_NOERR;
}

// utility function to rotate a rectangle clockwise about a point
// currently, angle must be 0, 90, 180 or 270
// assumes that (r->right) > (r->left), (r->top) > (r->bottom)
//
CPartList::RotateRect( CRect *r, int angle, CPoint org )
{
	CRect tr;
	if( angle == 90 )
	{
		tr.left = org.x + (r->bottom - org.y);
		tr.right = org.x + (r->top - org.y);
		tr.bottom = org.y + (org.x - r->right);
		tr.top = org.y + (org.x - r->left);
	}
	else if( angle > 90 )
	{
		tr = *r;
		for( int i=0; i<(angle/90); i++ )
			RotateRect( &tr, 90, org );
	}
	*r = tr;
	return PL_NOERR;
}

// export part list data into partlist_info structure for editing in dialog
// if test_part != NULL, returns index of test_part in partlist_info
//
int CPartList::ExportPartListInfo( partlist_info * pl, cpart * test_part )
{
	// traverse part list to find number of parts
	int ipart = -1;
	int nparts = 0;
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		nparts++;
		part = part->next;
	}
	// now make struct
	pl->SetSize( nparts );
	int i = 0;
	part = m_start.next;
	while( part->next != 0 )
	{
		if( part == test_part )
			ipart = i;
		(*pl)[i].part = part;
		(*pl)[i].shape = part->shape;
		(*pl)[i].bShapeChanged = FALSE;
		(*pl)[i].ref_des = part->ref_des;
		if( part->shape )
		{
			(*pl)[i].ref_size = part->m_ref_size;
			(*pl)[i].ref_width = part->m_ref_w;
		}
		else
		{
			(*pl)[i].ref_size = 0;
			(*pl)[i].ref_width = 0;
		}
		(*pl)[i].package = part->package;
		(*pl)[i].x = part->x;
		(*pl)[i].y = part->y;
		(*pl)[i].angle = part->angle;
		(*pl)[i].side = part->side;
		(*pl)[i].deleted = FALSE;
		(*pl)[i].bOffBoard = FALSE;
		i++;
		part = part->next;
	}
	return ipart;
}

// import part list data from struct partlist_info
//
void CPartList::ImportPartListInfo( partlist_info * pl, int flags, CDlgLog * log )
{
	CString mess; 

	// grid for positioning parts off-board
	int pos_x = 0;
	int pos_y = 0;
	enum { GRID_X = 100, GRID_Y = 50 };
	BOOL * grid = (BOOL*)calloc( GRID_X*GRID_Y, sizeof(BOOL) );
	int grid_num = 0;

	// now find parts in project that are not in partlist_info
	// loop through all parts
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		// loop through the partlist_info array
		BOOL bFound = FALSE;
		part->bPreserve = FALSE;
		for( int i=0; i<pl->GetSize(); i++ )
		{
			part_info * pi = &(*pl)[i];
			if( pi->ref_des == part->ref_des )
			{
				// part exists in partlist_info
				bFound = TRUE;
				break;
			}
		}
		cpart * next_part = part->next;
		if( !bFound )
		{
			// part not in partlist_info
			if( flags & KEEP_PARTS_AND_CON )
			{
				// set flag
				part->bPreserve = TRUE;
				if( log )
				{
					mess.Format( "  Keeping part %s and connections\r\n", part->ref_des );
					log->AddLine( &mess );
				}
			}
			else if( flags & KEEP_PARTS_NO_CON )
			{
				// keep part but remove connections from netlist
				if( log )
				{
					mess.Format( "  Keeping part %s but removing connections\r\n", part->ref_des );
					log->AddLine( &mess );
				}
				m_nlist->PartDeleted( part );
			}
			else
			{
				// remove it
				if( log )
				{
					mess.Format( "  Removing part %s\r\n", part->ref_des );
					log->AddLine( &mess );
				}
				m_nlist->PartDeleted( part );
				Remove( part );
			}
		}
		part = next_part;
	}

	// loop through partlist_info array, changing partlist as necessary
	for( int i=0; i<pl->GetSize(); i++ )
	{
		part_info * pi = &(*pl)[i];
		if( pi->part == 0 && pi->deleted )
		{
			// new part was added but then deleted, ignore it
			continue;
		}
		if( pi->part != 0 && pi->deleted )
		{
			// old part was deleted, remove it
			m_nlist->PartDisconnected( pi->part );
			Remove( pi->part );
			continue;
		}

		if( pi->part == 0 )
		{
			// new part is being imported
			cpart * old_part = GetPart( &pi->ref_des );
			if( old_part )
			{
				// new part has the same refdes as an existing part
				if( old_part->shape )
				{
					// existing part has a footprint
					if( flags & KEEP_FP )
					{
						// replace new part with old
						pi->part = old_part;
						pi->ref_size = old_part->m_ref_size; 
						pi->ref_width = old_part->m_ref_w;
						pi->x = old_part->x; 
						pi->y = old_part->y;
						pi->angle = old_part->angle;
						pi->side = old_part->side;
						pi->shape = old_part->shape;
					}
					else if( pi->shape )
					{
						// use new footprint, but preserve position
						pi->ref_size = old_part->m_ref_size; 
						pi->ref_width = old_part->m_ref_w;
						pi->x = old_part->x; 
						pi->y = old_part->y;
						pi->angle = old_part->angle;
						pi->side = old_part->side;
						pi->part = old_part;
						pi->bShapeChanged = TRUE;
						if( log && old_part->shape->m_name != pi->package )
						{
							mess.Format( "  Changing footprint of part %s from \"%s\" to \"%s\"\r\n", 
								old_part->ref_des, old_part->shape->m_name, pi->shape->m_name );
							log->AddLine( &mess );
						}
					}
					else
					{
						// new part does not have footprint, remove old part
						if( log && old_part->shape->m_name != pi->package )
						{
							mess.Format( "  Changing footprint of part %s from \"%s\" to \"%s\" (not found)\r\n", 
								old_part->ref_des, old_part->shape->m_name, pi->package );
							log->AddLine( &mess );
						}
						m_nlist->PartDisconnected( old_part );
						Remove( old_part );
					}
				}
				else
				{
					// remove old part (which did not have a footprint)
					if( log && old_part->package != pi->package )
					{
						mess.Format( "  Changing footprint of part %s from \"%s\" to \"%s\"\r\n", 
							old_part->ref_des, old_part->package, pi->package );
						log->AddLine( &mess );
					}
					m_nlist->PartDisconnected( old_part );
					Remove( old_part );
				}
			}
		}

		if( pi->part )
		{
			if( pi->part->shape != pi->shape || pi->bShapeChanged == TRUE )
			{
				// old part exists, but footprint was changed
				if( pi->part->shape == NULL )
				{
					// old part did not have a footprint before, so remove it
					// and treat as new part
					m_nlist->PartDisconnected( pi->part );
					Remove( pi->part );
					pi->part = NULL;
				}
			}
		}

		if( pi->part == 0 )
		{
			// new part is being imported (with or without footprint)
			if( pi->shape && pi->bOffBoard )
			{
				// place new part offboard, using grid 
				int ix, iy;	// grid indices
				// find size of part in 100 mil units
				BOOL OK = FALSE;
				int w = abs( pi->shape->m_sel_xf - pi->shape->m_sel_xi )/(100*PCBU_PER_MIL)+2;
				int h = abs( pi->shape->m_sel_yf - pi->shape->m_sel_yi )/(100*PCBU_PER_MIL)+2;
				// now find space in grid for part
				for( ix=0; ix<GRID_X; ix++ )
				{
					iy = 0;
					while( iy < (GRID_Y - h) )
					{
						if( !grid[ix+GRID_X*iy] )
						{
							// see if enough space
							OK = TRUE;
							for( int iix=ix; iix<(ix+w); iix++ )
								for( int iiy=iy; iiy<(iy+h); iiy++ )
									if( grid[iix+GRID_X*iiy] )
										OK = FALSE;
							if( OK )
								break;
						}
						iy++;
					}
					if( OK )
						break;
				}
				if( OK )
				{
					// place part
					pi->side = 0;
					pi->angle = 0;
					if( grid_num == 0 )
					{
						// first grid, to left and above origin
						pi->x = -(ix+w)*100*PCBU_PER_MIL;
						pi->y = iy*100*PCBU_PER_MIL;
					}
					else if( grid_num == 1 )
					{
						// second grid, to left and below origin
						pi->x = -(ix+w)*100*PCBU_PER_MIL;
						pi->y = -(iy+h)*100*PCBU_PER_MIL;
					}
					else if( grid_num == 2 )
					{
						// third grid, to right and below origin
						pi->x = ix*100*PCBU_PER_MIL;
						pi->y = -(iy+h)*100*PCBU_PER_MIL;
					}
					// remove space in grid
					for( int iix=ix; iix<(ix+w); iix++ )
						for( int iiy=iy; iiy<(iy+h); iiy++ )
							grid[iix+GRID_X*iiy] = TRUE;
				}
				else
				{
					// fail, go to next grid
					if( grid_num == 2 )
						ASSERT(0);		// ran out of grids
					else
					{
						// zero grid
						for( int j=0; j<GRID_Y; j++ )
							for( int i=0; i<GRID_X; i++ )
								grid[j*GRID_X+i] = FALSE;
						grid_num++;
					}
				}
				// now offset for part origin
				pi->x -= pi->shape->m_sel_xi;
				pi->y -= pi->shape->m_sel_yi;
			}
			// now place part
			cpart * part = Add( pi->shape, &pi->ref_des, &pi->package, pi->x, pi->y,
				pi->side, pi->angle, TRUE, FALSE );
			if( part->shape )
			{
				ResizeRefText( part, pi->ref_size, pi->ref_width );
			}
			m_nlist->PartAdded( part );
		}
		else
		{
			// part existed before but may have been modified
			if( pi->part->package != pi->package )
			{
				// package changed
				pi->part->package = pi->package;
			}

			if( pi->part->shape != pi->shape || pi->bShapeChanged == TRUE )
			{
				// footprint was changed
				if( pi->part->shape == NULL )
				{
					ASSERT(0);	// should never get here
				}
				else if( pi->shape && !(flags & KEEP_FP) )
				{
					// change footprint to new one
					PartFootprintChanged( pi->part, pi->shape );
					ResizeRefText( pi->part, pi->ref_size, pi->ref_width );
				}
			}
			if( pi->x != pi->part->x 
				|| pi->y != pi->part->y
				|| pi->angle != pi->part->angle
				|| pi->side != pi->part->side )
			{
				// part was moved
				Move( pi->part, pi->x, pi->y, pi->angle, pi->side );
				m_nlist->PartMoved( pi->part );
			}
		}
	}
//	PurgeFootprintCache();
	free( grid );
}

// note that this is a static function, for use as a callback
//
void CPartList::PartUndoCallback( int type, void * ptr, BOOL undo )
{
	undo_part * upart = (undo_part*)ptr;

	if( undo )
	{
		// perform undo
		CString ref = upart->ref_des;
		CPartList * pl = upart->m_plist;
		cpart * part = pl->GetPart( &ref );
		if( type == UNDO_PART_ADD )
		{
			pl->m_nlist->PartDeleted( part );
			pl->Remove( part );
		}
		else if( type == UNDO_PART_DELETE )
		{
			// part was deleted, lookup shape in cache
			CShape * s;
			void * s_ptr;
			int err = pl->m_footprint_cache_map->Lookup( upart->shape_name, s_ptr );
			if( err )
			{
				// found in cache
				s = (CShape*)s_ptr; 
			}
			else
				ASSERT(0);	// shape not found
			CString ref_des = upart->ref_des;
			CString package = upart->package;
			part = pl->Add( s, &ref_des, &package, upart->x, upart->y,
				upart->side, upart->angle, upart->visible, upart->glued );
			part->m_ref_xi = upart->m_ref_xi;
			part->m_ref_yi = upart->m_ref_yi;
			part->m_ref_angle = upart->m_ref_angle;
			pl->ResizeRefText( part, upart->m_ref_size, upart->m_ref_w );
			pl->m_nlist->PartAdded( part );
		}
		else if( type == UNDO_PART_MODIFY )
		{
			// part was moved or modified
			if( upart->shape != part->shape )
			{
				// footprint was changed
				pl->PartFootprintChanged( part, upart->shape );
				pl->m_nlist->PartFootprintChanged( part );
			}
			if( upart->x != part->x
				|| upart->y != part->y 
				|| upart->angle != part->angle 
				|| upart->side != part->side )
			{
				pl->Move( part, upart->x, upart->y, upart->angle, upart->side );
				pl->m_nlist->PartMoved( part );
			}
			part->glued = upart->glued; 
			part->m_ref_xi = upart->m_ref_xi;
			part->m_ref_yi = upart->m_ref_yi;
			part->m_ref_angle = upart->m_ref_angle;
			pl->ResizeRefText( part, upart->m_ref_size, upart->m_ref_w );
			char * chptr = (char*)ptr + sizeof( undo_part );
			for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
			{
				if( *chptr != 0 )
				{
					CString net_name = chptr;
					cnet * net = pl->m_nlist->GetNetPtrByName( &net_name );
					part->pin[ip].net = net;
				}
				else
					part->pin[ip].net = NULL;
				chptr += MAX_NET_NAME_SIZE + 1;
			}
		}
		else
			ASSERT(0);
	}
	free(ptr);	// delete the undo record
}

// checks to see if a pin is connected with a trace or a thermal on a
// particular layer
//
// returns NO_CONNECT, TRACE_CONNECT or THERMAL_CONNECT
//
int CPartList::GetPinConnectionStatus( cpart * part, CString * pin_name, int layer )
{
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	cnet * net = part->pin[pin_index].net;
	if( !net )
		return NOT_CONNECTED;

	int status = ON_NET;

	// now check for traces
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		int nsegs = net->connect[ic].nsegs;
		int p1 = net->connect[ic].start_pin;
		int p2 = net->connect[ic].end_pin;
		if( net->pin[p1].part == part &&
			net->pin[p1].pin_name == *pin_name &&
			net->connect[ic].seg[0].layer == layer )
		{
			// first segment connects to pin on this layer
			status |= TRACE_CONNECT;
		}
		else if( p2 == cconnect::NO_END )
		{
			// stub trace, ignore end pin
		}
		else if( net->pin[p2].part == part &&
			net->pin[p2].pin_name == *pin_name &&
			net->connect[ic].seg[nsegs-1].layer == layer )
		{
			// last segment connects to pin on this layer
			status |= TRACE_CONNECT;
			break;
		}
	}
	// now check for connection via thermal relief
	for( int ia=0; ia<net->nareas; ia++ )
	{
		carea * a = &net->area[ia];
		for( int ip=0; ip<a->npins; ip++ )
		{
			cpin * pin = &net->pin[a->pin[ip]];
			if( pin->part == part 
				&& pin->pin_name == *pin_name
				&& a->poly->GetLayer() == layer )
			{
				status |= THERMAL_CONNECT;
				break;
			}
		}
	}
	return status;
}

// Get enough info to draw the pad in Gerber file
// Also used by DRC routines
// returns 0 if no footprint for part,
// or no pad and no hole on this layer
//
int CPartList::GetPadDrawInfo( cpart * part, int ipin, int layer, int annular_ring, int mask_clearance,
							  int * type, int * x, int * y, int * w, int * l, int * r, int * hole,
							  int * angle, cnet ** net, int * connection_type )
{
	// get footprint
	CShape * s = part->shape;
	if( !s )
		return 0;

	// use copper layers for mask parameters
	int use_layer = layer;
	if( layer == LAY_MASK_TOP )
		use_layer = LAY_TOP_COPPER;
	else if( layer == LAY_MASK_BOTTOM )
		use_layer = LAY_BOTTOM_COPPER;
	if( use_layer < LAY_TOP_COPPER )
		return 0;

	// get padstack info
	padstack * ps = &s->m_padstack[ipin];
	CString pin_name = s->GetPinNameByIndex( ipin );
	int connect_status = GetPinConnectionStatus( part, &pin_name, use_layer );
	int xx = part->pin[ipin].x;
	int yy = part->pin[ipin].y;
	int ww = 0;
	int ll = 0;
	int rr = 0;
	int ttype = PAD_NONE;
	int aangle = s->m_padstack[ipin].angle + part->angle;
	aangle = aangle%180;
	int hole_size = s->m_padstack[ipin].hole_size;
	cnet * nnet = part->pin[ipin].net;

	// get pad info
	pad * p = NULL;
	if( (use_layer == LAY_TOP_COPPER && part->side == 0 )
		|| (use_layer == LAY_BOTTOM_COPPER && part->side == 1 ) ) 
	{
		// top pad is on this layer 
		p = &ps->top;
	}
	else if( (use_layer == LAY_TOP_COPPER && part->side == 1 )
			|| (use_layer == LAY_BOTTOM_COPPER && part->side == 0 ) ) 
	{
		// bottom pad is on this layer
		p = &ps->bottom;
	}
	else if( use_layer > LAY_BOTTOM_COPPER && ps->hole_size != 0 )
	{
		// inner pad is on this layer
		p = &ps->inner;
	}
	if( p )
	{
		rr = p->radius;
		if( p->shape == PAD_NONE && ps->hole_size == 0 )
		{
			// no pad, no hole
			return 0;
		}
		if( p->shape == PAD_NONE && ps->hole_size > 0 )
		{
			// no pad, but hole
			if( p == &ps->inner 
				&& (connect_status & (CPartList::THERMAL_CONNECT | CPartList::TRACE_CONNECT) ) )
			{
				// connected by trace or thermal on inner layer, make annular ring 
				ttype = PAD_ROUND;
				ww = 2*annular_ring + hole_size;
			}
		}
		else if( p->shape != PAD_NONE )
		{
			// pad exists
			if( p == &ps->inner && !(connect_status & (CPartList::THERMAL_CONNECT | CPartList::TRACE_CONNECT) ) )
			{
				// inner layer, no connection on this layer, no pad
				if( !hole_size )
					return 0;
			}
			else if( p == &ps->inner && (connect_status & CPartList::THERMAL_CONNECT) && !(connect_status & CPartList::TRACE_CONNECT) )
			{
				// inner layer, thermal and no trace, use annular ring for small thermal
				ttype = PAD_ROUND;
				ww = 2*annular_ring + hole_size;
			}
			else
			{
				// just use pad
				ttype = p->shape;
				ww = p->size_h;
				ll = 2*p->size_l;
			}
		}
	}
	if( layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM )
	{
		ww += 2*mask_clearance;
		ll += 2*mask_clearance;
	}
	if( x )
		*x = xx;
	if( y )
		*y = yy;
	if( type )
		*type = ttype;
	if( w )
		*w = ww;
	if( l )
		*l = ll;
	if( r )
		*r = rr;
	if( hole )
		*hole = hole_size;
	if( angle )
		*angle = aangle;
	if( connection_type )
		*connection_type = connect_status;
	if( net )
		*net = nnet;
	return 1;
}

// Design rule check
//
void CPartList::DRC( CDlgLog * log, int copper_layers, 
					int units, BOOL check_unrouted,
					CPolyLine * board_outline,
					DesignRules * dr, DRErrorList * drelist )
{
	CString d_str;
	CString str;
	CString str2;
	long nerrors = 0;

	// iterate through parts, checking pads and setting DRC params
	str.Format( "Checking parts:\r\n" );
	log->AddLine( &str );
	cpart * part = GetFirstPart();
	while( part )
	{
		CShape * s = part->shape;
		if( s )
		{
			// set DRC params for part
			part->hole_flag = FALSE;
			part->min_x = INT_MAX;
			part->max_x = INT_MIN;
			part->min_y = INT_MAX;
			part->max_y = INT_MIN;
			part->layers = 0;

			// iterate through pins in test_part
			for( int ip=0; ip<s->GetNumPins(); ip++ )
			{
				drc_pin * drp = &part->pin[ip].drc;
				drp->hole_size = 0;
				drp->min_x = INT_MAX;
				drp->max_x = INT_MIN;
				drp->min_y = INT_MAX;
				drp->max_y = INT_MIN;
				drp->max_r = INT_MIN;
				drp->layers = 0;

				id id1 = part->m_id;
				id1.st = ID_PAD;
				id1.i = ip;

				// iterate through copper layers
				for( int il=0; il<copper_layers; il++ )
				{
					int layer = LAY_TOP_COPPER + il;
					int layer_bit = 1<<il;

					// get test pad info
					int x, y, w, l, r, type, hole, connect, angle;
					cnet * net;
					BOOL bPad = GetPadDrawInfo( part, ip, layer, dr->annular_ring_pins, 0,
						&type, &x, &y, &w, &l, &r, &hole, &angle,
						&net, &connect );
					if( bPad )
					{
						// pad or hole present
						if( hole )
						{
							drp->hole_size = hole;
							part->hole_flag = TRUE;
							// test clearance to board edge
							if( board_outline )
							{
								for( int ibc=0; ibc<board_outline->GetNumCorners(); ibc++ )
								{
									int x1 = board_outline->GetX(ibc);
									int y1 = board_outline->GetY(ibc);
									int x2 = board_outline->GetX(0);
									int y2 = board_outline->GetY(0);
									if( ibc != board_outline->GetNumCorners()-1 )
									{
										x2 = board_outline->GetX(ibc+1);
										y2 = board_outline->GetY(ibc+1);
									}
									// for now, only works for straight board edge segments
									if( board_outline->GetSideStyle(ibc) == CPolyLine::STRAIGHT )
									{
										int d = ::GetClearanceBetweenSegmentAndPad( x1, y1, x2, y2, 0,
											PAD_ROUND, x, y, hole, 0, 0, 0 );
										if( d < dr->board_edge_copper )
										{
											// BOARDEDGE_PADHOLE error
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											str.Format( "%ld: %s.%s pad hole to board edge (%s)\r\n",  
												nerrors, part->ref_des, s->m_padstack[ip].name, d_str );
											DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_PADHOLE, &str,
												&part->ref_des, NULL, id1, id1, x, y, 0, 0, w+20*NM_PER_MIL, 0 );
											if( dre )
											{
												nerrors++;
												log->AddLine( &str );
											}
										}

									}
								}
							}
						}
						if( type != PAD_NONE )
						{
							int wid = w;
							int len = wid;
							if( type == PAD_RECT || type == PAD_RRECT || type == PAD_OVAL )
								len = l;
							if( angle == 90 )
							{
								wid = len;
								len = w;
							}
							drp->min_x = min( drp->min_x, x - len/2 );
							drp->max_x = max( drp->max_x, x + len/2 );
							drp->min_y = min( drp->min_y, y - wid/2 );
							drp->max_y = max( drp->max_y, y + wid/2 );
							drp->max_r = max( drp->max_r, Distance( 0, 0, len/2, wid/2 ) );
							part->min_x = min( part->min_x, x - len/2 );
							part->max_x = max( part->max_x, x + len/2 );
							part->min_y = min( part->min_y, y - wid/2 );;
							part->max_y = max( part->max_y, y + wid/2 );;
							drp->layers |= layer_bit;
							part->layers |= layer_bit;
							if( hole && part->pin[ip].net )
							{
								// test annular ring
								int d = (w - hole)/2;
								if( type == PAD_RECT || type == PAD_RRECT || type == PAD_OVAL )
									d = (min(w,l) - hole)/2;
								if( d < dr->annular_ring_pins )
								{
									// RING_PAD
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									str.Format( "%ld: %s.%s annular ring (%s)\r\n",  
										nerrors, part->ref_des, s->m_padstack[ip].name, d_str );
									DRError * dre = drelist->Add( nerrors, DRError::RING_PAD, &str,
										&part->ref_des, NULL, id1, id1, x, y, 0, 0, w+20*NM_PER_MIL, 0 );
									if( dre )
									{
										nerrors++;
										log->AddLine( &str );
									}
								}
							}
							// test clearance to board edge
							if( board_outline )
							{
								for( int ibc=0; ibc<board_outline->GetNumCorners(); ibc++ )
								{
									int x1 = board_outline->GetX(ibc);
									int y1 = board_outline->GetY(ibc);
									int x2 = board_outline->GetX(0);
									int y2 = board_outline->GetY(0);
									if( ibc != board_outline->GetNumCorners()-1 )
									{
										x2 = board_outline->GetX(ibc+1);
										y2 = board_outline->GetY(ibc+1);
									}
									// for now, only works for straight board edge segments
									if( board_outline->GetSideStyle(ibc) == CPolyLine::STRAIGHT )
									{
										int d = ::GetClearanceBetweenSegmentAndPad( x1, y1, x2, y2, 0,
											type, x, y, w, l, r, angle );
										if( d < dr->board_edge_copper )
										{
											// BOARDEDGE_PAD error
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											str.Format( "%ld: %s.%s pad to board edge (%s)\r\n",  
												nerrors, part->ref_des, s->m_padstack[ip].name, d_str );
											DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_PAD, &str,
												&part->ref_des, NULL, id1, id1, x, y, 0, 0, w+20*NM_PER_MIL, 0 );
											if( dre )
											{
												nerrors++;
												log->AddLine( &str );
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
		part = GetNextPart( part );
	}

	// iterate through parts again, checking against all other parts
	for( cpart * t_part=GetFirstPart(); t_part; t_part=GetNextPart(t_part) )
	{
		CShape * t_s = t_part->shape;
		if( t_s )
		{
			// now iterate through parts that follow in the partlist
			for( cpart * part=GetNextPart(t_part); part; part=GetNextPart(part) )
			{
				CShape * s = part->shape;
				if( s )
				{
					// now see if part and t_part pads might intersect
					// get max. clearance violation
					int clr = max( dr->pad_pad, dr->hole_copper );
					clr = max( clr, dr->hole_hole );
					// see if pads on same layers
					if( !(part->layers & t_part->layers) )
					{
						// no pads on same layers,check for holes
						if( !part->hole_flag && !t_part->hole_flag ) 
							continue;	// no, go to next part
					}

					// now check for clearance of rectangles
					if( part->min_x - t_part->max_x > clr )
						continue;	// next part
					if( t_part->min_x - part->max_x > clr )
						continue;	// next part
					if( part->min_y - t_part->max_y > clr )
						continue;	// next part
					if( t_part->min_y - part->max_y > clr )
						continue;	// next part

					// no clearance, we need to test pins in these parts
					// iterate through pins in t_part
					for( int t_ip=0; t_ip<t_s->GetNumPins(); t_ip++ )
					{
						padstack * t_ps = &t_s->m_padstack[t_ip];
						part_pin * t_pin = &t_part->pin[t_ip];
						drc_pin * t_drp = &t_pin->drc;
						id id1 = part->m_id;
						id1.st = ID_PAD;
						id1.i = t_ip;

						// iterate through pins in part
						for( int ip=0; ip<s->GetNumPins(); ip++ )
						{
							padstack * ps = &s->m_padstack[ip];
							part_pin * pin = &part->pin[ip];
							drc_pin * drp = &pin->drc;
							id id2 = part->m_id;
							id2.st = ID_PAD;
							id2.i = ip;

							// test for hole-hole violation
							if( drp->hole_size && t_drp->hole_size )
							{
								// test for hole-to-hole violation
								int dist = Distance( pin->x, pin->y, t_pin->x, t_pin->y );
								int h_h = max( 0, dist - (ps->hole_size + t_ps->hole_size)/2 );
								if( h_h < dr->hole_hole )
								{
									// PADHOLE_PADHOLE
									::MakeCStringFromDimension( &d_str, h_h, units, TRUE, TRUE, TRUE, 1 );
									str.Format( "%ld: %s.%s pad hole to %s.%s pad hole (%s)\r\n",  
										nerrors, part->ref_des, s->m_padstack[ip].name,
										t_part->ref_des, t_s->m_padstack[t_ip].name,
										d_str );
									DRError * dre = drelist->Add( nerrors, DRError::PADHOLE_PADHOLE, &str,
										&t_part->ref_des, &part->ref_des, id1, id2, 
										pin->x, pin->y, t_pin->x, t_pin->y, 0, 0 );
									if( dre )
									{
										nerrors++;
										log->AddLine( &str );
									}
								}
							}

							// see if pads on same layers
							if( !(drp->layers & t_drp->layers) )
							{
								// no, see if either has a hole
								if( !drp->hole_size && !t_drp->hole_size )
								{
									// no, go to next pin
									continue;
								}
							}

							// see if padstacks might intersect
							if( drp->min_x - t_drp->max_x > clr )
								continue;	// no, next pin
							if( t_drp->min_x - drp->max_x > clr )
								continue;	// no, next pin
							if( drp->min_y - t_drp->max_y > clr )
								continue;	// no, next pin
							if( t_drp->min_y - drp->max_y > clr )
								continue;	// no, next pin

							// OK, pads might be too close
							// check for pad clearance violations on each layer
							for( int il=0; il<copper_layers; il++ )
							{
								int layer = il + LAY_TOP_COPPER;
								CString lay_str = layer_str[layer];
								int t_pad_x, t_pad_y, t_pad_w, t_pad_l, t_pad_r;
								int t_pad_type, t_pad_hole, t_pad_connect, t_pad_angle;
								cnet * t_pad_net;

								// test for pad-pad violation
								BOOL t_bPad = GetPadDrawInfo( t_part, t_ip, layer, dr->annular_ring_pins, 0,
									&t_pad_type, &t_pad_x, &t_pad_y, &t_pad_w, &t_pad_l, &t_pad_r, 
									&t_pad_hole, &t_pad_angle,
									&t_pad_net, &t_pad_connect );
								if( t_bPad )
								{
									// get pad info for pin
									int pad_x, pad_y, pad_w, pad_l, pad_r;
									int pad_type, pad_hole, pad_connect, pad_angle;
									cnet * pad_net;
									BOOL bPad = GetPadDrawInfo( part, ip, layer, dr->annular_ring_pins, 0,
										&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, 
										&pad_hole, &pad_angle, &pad_net, &pad_connect );
									if( bPad )
									{
										if( pad_hole )
										{
											// test for pad-padhole violation
											int dist = GetClearanceBetweenPads( t_pad_type, t_pad_x, t_pad_y, 
												t_pad_w, t_pad_l, t_pad_r, t_pad_angle,
												PAD_ROUND, pad_x, pad_y, pad_hole, 0, 0, 0 );
											if( dist < dr->hole_copper )
											{
												// PAD_PADHOLE 
												::MakeCStringFromDimension( &d_str, dist, units, TRUE, TRUE, TRUE, 1 );
												str.Format( "%ld: %s.%s pad hole to %s.%s pad (%s)\r\n",  
													nerrors, part->ref_des, s->m_padstack[ip].name,
													t_part->ref_des, t_s->m_padstack[t_ip].name,
													d_str );
												DRError * dre = drelist->Add( nerrors, DRError::PAD_PADHOLE, &str, 
													&t_part->ref_des, &part->ref_des, id1, id2, 
													pad_x, pad_y, t_pad_x, t_pad_y, 0, layer );
												if( dre )
												{
													nerrors++;
													log->AddLine( &str );
												}
												break;		// skip any more layers, go to next pin
											}
										}
										// test for pad-pad violation
										int dist = GetClearanceBetweenPads( t_pad_type, t_pad_x, t_pad_y, 
											t_pad_w, t_pad_l, t_pad_r, t_pad_angle,
											pad_type, pad_x, pad_y, pad_w, pad_l, pad_r, pad_angle );
										if( dist < dr->pad_pad )
										{
											// PAD_PAD 
											::MakeCStringFromDimension( &d_str, dist, units, TRUE, TRUE, TRUE, 1 );
											str.Format( "%ld: %s.%s pad to %s.%s pad (%s)\r\n",  
												nerrors, part->ref_des, s->m_padstack[ip].name,
												t_part->ref_des, t_s->m_padstack[t_ip].name,
												d_str );
											DRError * dre = drelist->Add( nerrors, DRError::PAD_PAD, &str, 
												&t_part->ref_des, &part->ref_des, id1, id2, 
												pad_x, pad_y, t_pad_x, t_pad_y, 0, layer );
											if( dre )
											{
												nerrors++;
												log->AddLine( &str );
											}
											break;		// skip any more layers, go to next pin
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// iterate through all nets
	str.Format( "\r\nChecking nets and parts:\r\n" );
	log->AddLine( &str );
	POSITION pos;
	void * ptr;
	CString name;
	for( pos = m_nlist->m_map.GetStartPosition(); pos != NULL; )
	{
		m_nlist->m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ia=0; ia<net->nareas; ia++ )
		{
			carea * a = &net->area[ia];
			for( int ic=0; ic<a->poly->GetNumCorners(); ic++ )
			{
				id id_a = net->id;
				id_a.st = ID_AREA;
				id_a.i = ia;
				id_a.sst = ID_SIDE;
				id_a.ii = ic;
				int x1 = a->poly->GetX(ic);
				int y1 = a->poly->GetY(ic);
				int x2 = a->poly->GetX(0);
				int y2 = a->poly->GetY(0);
				if( ic != a->poly->GetNumCorners()-1 )
				{
					x2 = a->poly->GetX(ic+1);
					y2 = a->poly->GetY(ic+1);
				}
				// test clearance to board edge
				if( board_outline )
				{
					for( int ibc=0; ibc<board_outline->GetNumCorners(); ibc++ )
					{
						int bx1 = board_outline->GetX(ibc);
						int by1 = board_outline->GetY(ibc);
						int bx2 = board_outline->GetX(0);
						int by2 = board_outline->GetY(0);
						if( ibc != board_outline->GetNumCorners()-1 )
						{
							bx2 = board_outline->GetX(ibc+1);
							by2 = board_outline->GetY(ibc+1);
						}
						// for now, only works for straight board edge segments
						if( board_outline->GetSideStyle(ibc) == CPolyLine::STRAIGHT )
						{
							int x, y;
							int d = ::GetClearanceBetweenSegments( bx1, by1, bx2, by2, 0,
								x1, y1, x2, y2, 0, &x, &y );
							if( d < dr->board_edge_copper )
							{
								// BOARDEDGE_COPPERAREA error
								::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
								str.Format( "%ld: \"%s\" copper area to board edge (%s)\r\n",  
									nerrors, net->name, d_str );
								DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_COPPERAREA, &str,
									&net->name, NULL, id_a, id_a, x, y, 0, 0, 0, 0 );
								if( dre )
								{
									nerrors++;
									log->AddLine( &str );
								}
							}
						}
					}
				}
			}
		}
		// iterate through all connections
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			// get DRC info for this connection
			// iterate through all segments and vertices
			c->min_x = INT_MAX;
			c->max_x = INT_MIN;
			c->min_y = INT_MAX;
			c->max_y = INT_MIN;
			c->vias_present = FALSE;
			c->seg_layers = 0;
			int max_w = 0;
			for( int is=0; is<c->nsegs; is++ )
			{
				id id_seg = net->id;
				id_seg.st = ID_CONNECT;
				id_seg.i = ic;
				id_seg.sst = ID_SEG;
				id_seg.ii = is;
				int x1 = c->vtx[is].x;
				int y1 = c->vtx[is].y;
				int x2 = c->vtx[is+1].x;
				int y2 = c->vtx[is+1].y;
				int w = c->seg[is].width;
				int layer = c->seg[is].layer;
				if( c->seg[is].layer >= LAY_TOP_COPPER )
				{
					int layer_bit = c->seg[is].layer - LAY_TOP_COPPER;
					c->seg_layers |= 1<<layer_bit;
				}
				max_w = max( max_w, w );
				// test trace width
				if( w > 0 && w < dr->trace_width )
				{
					// TRACE_WIDTH error
					int x = (x1+x2)/2;
					int y = (y1+y2)/2;
					::MakeCStringFromDimension( &d_str, w, units, TRUE, TRUE, TRUE, 1 );
					str.Format( "%ld: \"%s\" trace width (%s)\r\n", 
						nerrors, net->name, d_str );
					DRError * dre = drelist->Add( nerrors, DRError::TRACE_WIDTH, &str, 
						&net->name, NULL, id_seg, id_seg, x, y, 0, 0, 0, layer );
					if( dre )
					{
						nerrors++;
						log->AddLine( &str );
					}
				}
				// test clearance to board edge
				if( w > 0 && board_outline )
				{
					for( int ibc=0; ibc<board_outline->GetNumCorners(); ibc++ )
					{
						int bx1 = board_outline->GetX(ibc);
						int by1 = board_outline->GetY(ibc);
						int bx2 = board_outline->GetX(0);
						int by2 = board_outline->GetY(0);
						if( ibc != board_outline->GetNumCorners()-1 )
						{
							bx2 = board_outline->GetX(ibc+1);
							by2 = board_outline->GetY(ibc+1);
						}
						// for now, only works for straight board edge segments
						if( board_outline->GetSideStyle(ibc) == CPolyLine::STRAIGHT )
						{
							int x, y;
							int d = ::GetClearanceBetweenSegments( bx1, by1, bx2, by2, 0,
								x1, y1, x2, y2, w, &x, &y );
							if( d < dr->board_edge_copper )
							{
								// BOARDEDGE_TRACE error
								::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
								str.Format( "%ld: \"%s\" trace to board edge (%s)\r\n",  
									nerrors, net->name, d_str );
								DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_TRACE, &str,
									&net->name, NULL, id_seg, id_seg, x, y, 0, 0, 0, layer );
								if( dre )
								{
									nerrors++;
									log->AddLine( &str );
								}
							}
						}
					}
				}
			}
			for( int iv=0; iv<c->nsegs+1; iv++ )
			{
				cvertex * vtx = &c->vtx[iv];
				c->min_x = min( c->min_x, vtx->x - max_w );
				c->max_x = max( c->max_x, vtx->x + max_w );
				c->min_y = min( c->min_y, vtx->y - max_w );
				c->max_y = max( c->max_y, vtx->y + max_w );
				if( vtx->via_w )
				{
					id id_via = net->id;
					id_via.st = ID_CONNECT;
					id_via.i = ic;
					id_via.sst = ID_VIA;
					id_via.ii = iv;
					c->vias_present = TRUE;
					c->min_x = min( c->min_x, vtx->x - vtx->via_w );
					c->max_x = max( c->max_x, vtx->x + vtx->via_w );
					c->min_y = min( c->min_y, vtx->y - vtx->via_w );
					c->max_y = max( c->max_y, vtx->y + vtx->via_w );
					// check for RING_VIA error
					int d = (vtx->via_w - vtx->via_hole_w)/2;
					if( d < dr->annular_ring_vias )
					{
						// RING_VIA
						::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
						str.Format( "%ld: \"%s\" via annular ring (%s)\r\n", 
							nerrors, net->name, d_str );
						DRError * dre = drelist->Add( nerrors, DRError::RING_VIA, &str, 
							&net->name, NULL, id_via, id_via, vtx->x, vtx->y, 0, 0, vtx->via_w+20*NM_PER_MIL, 0 );
						if( dre )
						{
							nerrors++;
							log->AddLine( &str );
						}
					}
					// test clearance to board edge
					if( board_outline )
					{
						for( int ibc=0; ibc<board_outline->GetNumCorners(); ibc++ )
						{
							int bx1 = board_outline->GetX(ibc);
							int by1 = board_outline->GetY(ibc);
							int bx2 = board_outline->GetX(0);
							int by2 = board_outline->GetY(0);
							if( ibc != board_outline->GetNumCorners()-1 )
							{
								bx2 = board_outline->GetX(ibc+1);
								by2 = board_outline->GetY(ibc+1);
							}
							// for now, only works for straight board edge segments
							if( board_outline->GetSideStyle(ibc) == CPolyLine::STRAIGHT )
							{
								int d = ::GetClearanceBetweenSegmentAndPad( bx1, by1, bx2, by2, 0,
									PAD_ROUND, vtx->x, vtx->y, vtx->via_w, 0, 0, 0 );
								int dh = ::GetClearanceBetweenSegmentAndPad( bx1, by1, bx2, by2, 0,
									PAD_ROUND, vtx->x, vtx->y, vtx->via_hole_w, 0, 0, 0 );
								if( d < dr->board_edge_copper )
								{
									// BOARDEDGE_VIA error
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									str.Format( "%ld: \"%s\" via to board edge (%s)\r\n",  
										nerrors, net->name, d_str );
									DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_VIA, &str,
										&net->name, NULL, id_via, id_via, vtx->x, vtx->y, 0, 0, vtx->via_w+20*NM_PER_MIL, 0 );
									if( dre )
									{
										nerrors++;
										log->AddLine( &str );
									}
								}
								if( dh < dr->board_edge_hole )
								{
									// BOARDEDGE_VIAHOLE error
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									str.Format( "%ld: \"%s\" via hole to board edge (%s)\r\n",  
										nerrors, net->name, d_str );
									DRError * dre = drelist->Add( nerrors, DRError::BOARDEDGE_VIAHOLE, &str,
										&net->name, NULL, id_via, id_via, vtx->x, vtx->y, 0, 0, vtx->via_w+20*NM_PER_MIL, 0 );
									if( dre )
									{
										nerrors++;
										log->AddLine( &str );
									}
								}
							}
						}
					}
				}
			}
			// iterate through all parts
			cpart * part = GetFirstPart();
			for( ; part; part = GetNextPart( part ) )
			{
				CShape * s = part->shape;

				// if not on same layers, can't conflict
				if( !part->hole_flag && !c->vias_present && !(part->layers & c->seg_layers) )
					continue;	// next part

				// test for possible clearance violation
				if( part->min_x - c->max_x > dr->pad_trace )
					continue;	// next part
				if( c->min_x - part->max_x > dr->pad_trace )
					continue;	// next part
				if( part->min_y - c->max_y > dr->pad_trace )
					continue;	// next part
				if( c->min_y - part->max_y > dr->pad_trace )
					continue;	// next part

				// OK, now we have to test each pad
				for( int ip=0; ip<part->shape->GetNumPins(); ip++ )
				{
					padstack * ps = &s->m_padstack[ip];
					part_pin * pin = &part->pin[ip];
					drc_pin * drp = &pin->drc;
					id id_pad = part->m_id;
					id_pad.st = ID_PAD;
					id_pad.i = ip;

					// these values will be filled in if necessary
					int pad_x, pad_y, pad_w, pad_l, pad_r;
					int pad_type, pad_hole, pad_connect, pad_angle;
					cnet * pad_net;
					BOOL bPad;
					BOOL pin_info_valid = FALSE;
					int pin_info_layer = 0;

					if( drp->min_x - c->max_x > dr->pad_trace )
						continue;	// no, next pin
					if( c->min_x - drp->max_x > dr->pad_trace )
						continue;	// no, next pin
					if( drp->min_y - c->max_y > dr->pad_trace )
						continue;	// no, next pin
					if( c->min_y - drp->max_y > dr->pad_trace )
						continue;	// no, next pin

					// possible clearance violation, now test each segment and via on each layer
					for( int is=0; is<c->nsegs; is++ )
					{
						// get next segment
						cseg * s = &(net->connect[ic].seg[is]);
						cvertex * pre_vtx = &(net->connect[ic].vtx[is]);
						cvertex * post_vtx = &(net->connect[ic].vtx[is+1]);
						int w = s->width;
						int xi = pre_vtx->x;
						int yi = pre_vtx->y;
						int xf = post_vtx->x;
						int yf = post_vtx->y;
						int min_x = min( xi, xf ) - w/2;
						int max_x = max( xi, xf ) + w/2;
						int min_y = min( yi, yf ) - w/2;
						int max_y = max( yi, yf ) + w/2;
						// ids
						id id_seg = net->id;
						id_seg.st = ID_CONNECT;
						id_seg.i = ic;
						id_seg.sst = ID_SEG;
						id_seg.ii = is;
						id id_via = net->id;
						id_via.st = ID_CONNECT;
						id_via.i = ic;
						id_via.sst = ID_VIA;
						id_via.ii = is+1;

						// check all layers
						for( int il=0; il<copper_layers; il++ )
						{
							int layer = il + LAY_TOP_COPPER;
							int layer_bit = 1<<il;

							if( s->layer == layer )
							{
								// check segment clearances
								if( drp->hole_size && net != part->pin[ip].net )
								{
									// pad has hole, check segment to pad_hole clearance
									if( !(pin_info_valid && layer == pin_info_layer) )
									{
										bPad = GetPadDrawInfo( part, ip, layer, dr->annular_ring_pins, 0,
											&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, 
											&pad_hole, &pad_angle, &pad_net, &pad_connect );
										pin_info_valid = TRUE;
										pin_info_layer = layer;
									}
									int d = GetClearanceBetweenSegmentAndPad( xi, yi, xf, yf, w,
										PAD_ROUND, pad_x, pad_y, pad_hole, 0, 0, 0 );
									if( d < dr->pad_trace ) 
									{
										// SEG_PADHOLE
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										str.Format( "%ld: \"%s\" trace to %s.%s pad hole (%s)\r\n", 
											nerrors, net->name, part->ref_des, ps->name,
											d_str );
										DRError * dre = drelist->Add( nerrors, DRError::SEG_PAD, &str, 
											&net->name, &part->ref_des, id_seg, id_pad, pad_x, pad_y, pad_x, pad_y, 
											max(pad_w,pad_l)+20*NM_PER_MIL, layer );
										if( dre )
										{
											nerrors++;
											log->AddLine( &str );
										}
									}
								}
								if( layer_bit & drp->layers )
								{
									// pad is on this layer
									// get pad info for pin if necessary
									if( !(pin_info_valid && layer == pin_info_layer) )
									{
										bPad = GetPadDrawInfo( part, ip, layer, dr->annular_ring_pins, 0,
											&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r,
											&pad_hole, &pad_angle, &pad_net, &pad_connect );
										pin_info_valid = TRUE;
										pin_info_layer = layer;
									}
									if( bPad && pad_type != PAD_NONE && net != pad_net )
									{
										// check segment to pad clearance
										int d = GetClearanceBetweenSegmentAndPad( xi, yi, xf, yf, w,
											pad_type, pad_x, pad_y, pad_w, pad_l, pad_r, pad_angle );
										if( d < dr->pad_trace ) 
										{
											// SEG_PAD
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											str.Format( "%ld: \"%s\" trace to %s.%s pad (%s)\r\n", 
												nerrors, net->name, part->ref_des, ps->name,
												d_str );
											DRError * dre = drelist->Add( nerrors, DRError::SEG_PAD, &str, 
												&net->name, &part->ref_des, id_seg, id_pad, pad_x, pad_y, pad_x, pad_y, 
												max(pad_w,pad_l)+20*NM_PER_MIL, layer );
											if( dre )
											{
												nerrors++;
												log->AddLine( &str );
											}
										}
									}
								}
							}
							// get next via
							if( post_vtx->via_w )
							{
								// via exists
								int test = m_nlist->GetViaConnectionStatus( net, ic, is+1, layer );
								int w = 0;
								// copper layer, set aperture to normal via
								w = post_vtx->via_w;	// normal via pad
								if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
								{
									// inner layer and no trace or thermal, so no via pad
									w = 0;
								}
								else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_THERMAL) && !(test & CNetList::VIA_TRACE) )
								{
									// inner layer with small thermal, use annular ring
									w = post_vtx->via_hole_w + 2*dr->annular_ring_vias;	// TODO:
								}
								if( w )
								{
									// check via_pad to pin_pad clearance
									if( !(pin_info_valid && layer == pin_info_layer) )
									{
										bPad = GetPadDrawInfo( part, ip, layer, dr->annular_ring_pins, 0,
											&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, 
											&pad_hole, &pad_angle, &pad_net, &pad_connect );
										pin_info_valid = TRUE;
										pin_info_layer = layer;
									}
									if( bPad && pad_type != PAD_NONE && pad_net != net )
									{
										int d = GetClearanceBetweenPads( PAD_ROUND, xf, yf, w, 0, 0, 0,
											pad_type, pad_x, pad_y, pad_w, pad_l, pad_r, pad_angle );
										if( d < dr->pad_trace )
										{
											// VIA_PAD
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											str.Format( "%ld: \"%s\" via pad to %s.%s pad (%s)\r\n", 
												nerrors, net->name, part->ref_des, ps->name,
												d_str );
											DRError * dre = drelist->Add( nerrors, DRError::VIA_PAD, &str, 
												&net->name, &part->ref_des, id_via, id_pad, xf, yf, pad_x, pad_y, 0, layer );
											if( dre )
											{
												nerrors++;
												log->AddLine( &str );
											}
											break;  // skip more layers
										}
									}
									if( drp->hole_size && pad_net != net )
									{
										// pin has a hole, check via_pad to pin_hole clearance
										int d = Distance( xf, yf, pin->x, pin->y );
										d = max( 0, d - drp->hole_size/2 - w/2 );
										if( d < dr->hole_copper )
										{
											// VIA_PADHOLE
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											str.Format( "%ld: \"%s\" via pad to %s.%s pad hole (%s)\r\n", 
												nerrors, net->name, part->ref_des, ps->name,
												d_str );
											DRError * dre = drelist->Add( nerrors, DRError::VIA_PAD, &str, 
												&net->name, &part->ref_des, id_via, id_pad, xf, yf, pad_x, pad_y, 0, layer );
											if( dre )
											{
												nerrors++;
												log->AddLine( &str );
											}
											break;  // skip more layers
										}
									}
								}
								if( !(pin_info_valid && layer == pin_info_layer) )
								{
									bPad = GetPadDrawInfo( part, ip, layer, dr->annular_ring_pins, 0,
										&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r,
										&pad_hole, &pad_angle, &pad_net, &pad_connect );
									pin_info_valid = TRUE;
									pin_info_layer = layer;
								}
								if( bPad && pad_type != PAD_NONE && pad_net != net )
								{
									// check via_hole to pin_pad clearance
									int d = GetClearanceBetweenPads( PAD_ROUND, xf, yf, post_vtx->via_hole_w, 0, 0, 0,
										pad_type, pad_x, pad_y, pad_w, pad_l, pad_r, pad_angle );
									if( d < dr->hole_copper )
									{
										// VIAHOLE_PAD
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										str.Format( "%ld: \"%s\" via hole to %s.%s pad (%s)\r\n", 
											nerrors, net->name, part->ref_des, ps->name,
											d_str );
										DRError * dre = drelist->Add( nerrors, DRError::VIA_PAD, &str, 
											&net->name, &part->ref_des, id_via, id_pad, xf, yf, pad_x, pad_y, 0, layer );
										if( dre )
										{
											nerrors++;
											log->AddLine( &str );
										}
										break;  // skip more layers
									}
								}
								if( drp->hole_size && layer == LAY_TOP_COPPER )
								{
									// pin has a hole, check via_hole to pin_hole clearance
									int d = Distance( xf, yf, pin->x, pin->y );
									d = max( 0, d - drp->hole_size/2 - post_vtx->via_hole_w/2 );
									if( d < dr->hole_hole )
									{
										// VIAHOLE_PADHOLE
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										str.Format( "%ld: \"%s\" via hole to %s.%s pad hole (%s)\r\n", 
											nerrors, net->name, part->ref_des, ps->name,
											d_str );
										DRError * dre = drelist->Add( nerrors, DRError::VIA_PAD, &str, 
											&net->name, &part->ref_des, id_via, id_pad, xf, yf, pad_x, pad_y, 0, layer );
										if( dre )
										{
											nerrors++;
											log->AddLine( &str );
										}
										break;  // skip more layers
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// now check nets against other nets
	str.Format( "\r\nChecking nets:\r\n" );
	log->AddLine( &str );
	// get max clearance
	int cl = max( dr->hole_copper, dr->hole_hole );
	cl = max( cl, dr->trace_trace );
	// iterate through all nets
	for( pos = m_nlist->m_map.GetStartPosition(); pos != NULL; )
	{
		m_nlist->m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		// iterate through all connections
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];

			// iterate through all nets again
			POSITION pos2 = pos;
			void * ptr2;
			CString name2;
			while( pos2 != NULL )
			{
				m_nlist->m_map.GetNextAssoc( pos2, name2, ptr2 );
				cnet * net2 = (cnet*)ptr2;
				// iterate through all connections
				for( int ic2=0; ic2<net2->nconnects; ic2++ )
				{
					cconnect * c2 = &net2->connect[ic2];
					// look for possible clearance violations between c and c2
					if( c->min_x - c2->max_x > cl )
						continue;	// no, next connection
					if( c->min_y - c2->max_y > cl )
						continue;	// no, next connection
					if( c2->min_x - c->max_x > cl )
						continue;	// no, next connection
					if( c2->min_y - c->max_y > cl )
						continue;	// no, next connection

					// now we have to test all segments and vias in c
					for( int is=0; is<c->nsegs; is++ )
					{
						// get next segment and via
						cseg * s = &c->seg[is];
						cvertex * pre_vtx = &c->vtx[is];
						cvertex * post_vtx = &c->vtx[is+1];
						int seg_w = s->width;
						int vw = post_vtx->via_w;
						int max_w = max( seg_w, vw );
						int xi = pre_vtx->x;
						int yi = pre_vtx->y;
						int xf = post_vtx->x;
						int yf = post_vtx->y;
						// get bounding rect for segment and vias
						int min_x = min( xi, xf ) - max_w/2;
						int max_x = max( xi, xf ) + max_w/2;
						int min_y = min( yi, yf ) - max_w/2;
						int max_y = max( yi, yf ) + max_w/2;
						// ids
						id id_seg1( ID_NET, ID_CONNECT, ic, ID_SEG, is );
						id id_via1( ID_NET, ID_CONNECT, ic, ID_VIA, is+1 );;

						// iterate through all segments and vias in c2
						for( int is2=0; is2<c2->nsegs; is2++ )
						{
							// get next segment and via
							cseg * s2 = &c2->seg[is2];
							cvertex * pre_vtx2 = &c2->vtx[is2];
							cvertex * post_vtx2 = &c2->vtx[is2+1];
							int seg_w2 = s2->width;
							int vw2 = post_vtx2->via_w;
							int max_w2 = max( seg_w2, vw2 );
							int xi2 = pre_vtx2->x;
							int yi2 = pre_vtx2->y;
							int xf2 = post_vtx2->x;
							int yf2 = post_vtx2->y;
							// get bounding rect for this segment and attached vias
							int min_x2 = min( xi2, xf2 ) - max_w2/2;
							int max_x2 = max( xi2, xf2 ) + max_w2/2;
							int min_y2 = min( yi2, yf2 ) - max_w2/2;
							int max_y2 = max( yi2, yf2 ) + max_w2/2;
							// ids
							id id_seg2( ID_NET, ID_CONNECT, ic2, ID_SEG, is2 );
							id id_via2( ID_NET, ID_CONNECT, ic2, ID_VIA, is2+1 );;

							// see if segment bounding rects are too close
							if( min_x - max_x2 > cl )
								continue;	// no, next segment
							if( min_y - max_y2 > cl )
								continue;
							if( min_x2 - max_x > cl )
								continue;
							if( min_y2 - max_y > cl )
								continue;

							// check if segments on same layer
							if( s->layer == s2->layer && s->layer >= LAY_TOP_COPPER )
							{
								// yes, test clearances
								int xx, yy;
								int d = GetClearanceBetweenSegments( xi, yi, xf, yf, seg_w, 
									xi2, yi2, xf2, yf2, seg_w2, &xx, &yy );
								if( d < dr->trace_trace )
								{
									// SEG_SEG
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									str.Format( "%ld: \"%s\" trace to \"%s\" trace (%s)\r\n", 
										nerrors, net->name, net2->name,
										d_str );
									DRError * dre = drelist->Add( nerrors, DRError::SEG_SEG, &str, 
										&net->name, &net2->name, id_seg1, id_seg2, xx, yy, xx, yy, 0, s->layer );
									if( dre )
									{
										nerrors++;
										log->AddLine( &str );
									}
								}
							}
							// test clearances between net->segment and net2->via
							int layer = s->layer;
							if( layer >= LAY_TOP_COPPER && post_vtx2->via_w )
							{
								// via exists
								int test = m_nlist->GetViaConnectionStatus( net2, ic2, is2+1, layer );
								int via_w2 = post_vtx2->via_w;	// normal via pad
								if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
								{
									// inner layer and no trace or thermal, so no via pad
									via_w2 = 0;
								}
								else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_THERMAL) && !(test & CNetList::VIA_TRACE) )
								{
									// inner layer with small thermal, use annular ring
									via_w2 = post_vtx2->via_hole_w + 2*dr->annular_ring_vias;
								}
								// check clearance
								if( via_w2 )
								{
									// check clearance between segment and via pad
									int d = GetClearanceBetweenSegmentAndPad( xi, yi, xf, yf, seg_w,
										PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_w, 0, 0, 0 );
									if( d < dr->trace_trace )
									{
										// SEG_VIA
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										str.Format( "%ld: \"%s\" trace to \"%s\" via pad (%s)\r\n", 
											nerrors, net->name, net2->name,
											d_str );
										DRError * dre = drelist->Add( nerrors, DRError::SEG_VIA, &str, 
											&net->name, &net2->name, id_seg1, id_via2, xf2, yf2, xf2, yf2, 0, s->layer );
										if( dre )
										{
											nerrors++;
											log->AddLine( &str );
										}
									}
								}
								// check clearance between segment and via hole
								int d = GetClearanceBetweenSegmentAndPad( xi, yi, xf, yf, seg_w,
									PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_hole_w, 0, 0, 0 );
								if( d < dr->hole_copper )
								{
									// SEG_VIAHOLE
									::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
									str.Format( "%ld: \"%s\" trace to \"%s\" via hole (%s)\r\n", 
										nerrors, net->name, net2->name,
										d_str );
									DRError * dre = drelist->Add( nerrors, DRError::SEG_VIAHOLE, &str, 
										&net->name, &net2->name, id_seg1, id_via2, xf2, yf2, xf2, yf2, 0, s->layer );
									if( dre )
									{
										nerrors++;
										log->AddLine( &str );
									}
								}
							}
							// test clearances between net2->segment and net->via
							layer = s2->layer;
							if( post_vtx->via_w )
							{
								// via exists
								int test = m_nlist->GetViaConnectionStatus( net, ic, is+1, layer );
								int via_w = post_vtx->via_w;	// normal via pad
								if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
								{
									// inner layer and no trace or thermal, so no via pad
									via_w = 0;
								}
								else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_THERMAL) && !(test & CNetList::VIA_TRACE) )
								{
									// inner layer with small thermal, use annular ring
									via_w = post_vtx->via_hole_w + 2*dr->annular_ring_vias;
								}
								// check clearance
								if( via_w )
								{
									// check clearance between net2->segment and net->via_pad
									if( layer >= LAY_TOP_COPPER )
									{
										int d = GetClearanceBetweenSegmentAndPad( xi2, yi2, xf2, yf2, seg_w2,
											PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_w, 0, 0, 0 );
										if( d < dr->trace_trace )
										{
											// SEG_VIA
											::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
											str.Format( "%ld: \"%s\" via pad to \"%s\" trace (%s)\r\n", 
												nerrors, net->name, net2->name,
												d_str );
											DRError * dre = drelist->Add( nerrors, DRError::SEG_VIA, &str, 
												&net2->name, &net->name, id_seg2, id_via1, xf, yf, xf, yf, 
												post_vtx->via_w+20*NM_PER_MIL, 0 );
											if( dre )
											{
												nerrors++;
												log->AddLine( &str );
											}
										}
									}
								}
								// check clearance between net2->segment and net->via_hole
								if( layer >= LAY_TOP_COPPER )
								{
									int d = GetClearanceBetweenSegmentAndPad( xi2, yi2, xf2, yf2, seg_w2,
										PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_hole_w, 0, 0, 0 );
									if( d < dr->hole_copper )
									{
										// SEG_VIAHOLE
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										str.Format( "%ld: \"%s\" trace to \"%s\" via hole (%s)\r\n", 
											nerrors, net2->name, net->name,
											d_str );
										DRError * dre = drelist->Add( nerrors, DRError::SEG_VIAHOLE, &str, 
											&net2->name, &net->name, id_seg2, id_via1, xf, yf, xf, yf, 
											post_vtx->via_w+20*NM_PER_MIL, 0 );
										if( dre )
										{
											nerrors++;
											log->AddLine( &str );
										}
									}
								}
								// test clearances between net->via and net2->via
								if( post_vtx->via_w && post_vtx2->via_w )
								{
									for( int layer=LAY_TOP_COPPER; layer<(LAY_TOP_COPPER+copper_layers); layer++ )
									{
										// get size of net->via_pad
										int test = m_nlist->GetViaConnectionStatus( net, ic, is+1, layer );
										int via_w = post_vtx->via_w;	// normal via pad
										if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
										{
											// inner layer and no trace or thermal, so no via pad
											via_w = 0;
										}
										else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_THERMAL) && !(test & CNetList::VIA_TRACE) )
										{
											// inner layer with small thermal, use annular ring
											via_w = post_vtx->via_hole_w + 2*dr->annular_ring_vias;
										}
										// get size of net2->via_pad
										test = m_nlist->GetViaConnectionStatus( net2, ic2, is2+1, layer );
										int via_w2 = post_vtx2->via_w;	// normal via pad
										if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
										{
											// inner layer and no trace or thermal, so no via pad
											via_w2 = 0;
										}
										else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_THERMAL) && !(test & CNetList::VIA_TRACE) )
										{
											// inner layer with small thermal, use annular ring
											via_w2 = post_vtx2->via_hole_w + 2*dr->annular_ring_vias;
										}
										if( via_w && via_w2 )
										{
											//check net->via_pad to net2->via_pad clearance
											int d = GetClearanceBetweenPads( PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_w, 0, 0, 0, 
												PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_w, 0, 0, 0 );
											if( d < dr->trace_trace )
											{
												// VIA_VIA
												::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
												str.Format( "%ld: \"%s\" via pad to \"%s\" via pad (%s)\r\n", 
													nerrors, net->name, net2->name,
													d_str );
												DRError * dre = drelist->Add( nerrors, DRError::VIA_VIA, &str, 
													&net->name, &net2->name, id_via1, id_via2, xf, yf, xf2, yf2, 0, layer );
												if( dre )
												{
													nerrors++;
													log->AddLine( &str );
												}
											}
											// check net->via to net2->via_hole clearance
											d = GetClearanceBetweenPads( PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_w, 0, 0, 0,
												PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_hole_w, 0, 0, 0 );
											if( d < dr->hole_copper )
											{
												// VIA_VIAHOLE
												::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
												str.Format( "%ld: \"%s\" via pad to \"%s\" via hole (%s)\r\n", 
													nerrors, net->name, net2->name,
													d_str );
												DRError * dre = drelist->Add( nerrors, DRError::VIA_VIAHOLE, &str, 
													&net->name, &net2->name, id_via1, id_via2, xf, yf, xf2, yf2, 0, layer );
												if( dre )
												{
													nerrors++;
													log->AddLine( &str );
												}
											}
											// check net2->via to net->via_hole clearance
											d = GetClearanceBetweenPads( PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_hole_w, 0, 0, 0,
												PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_w, 0, 0, 0 );
											if( d < dr->hole_copper )
											{
												// VIA_VIAHOLE
												::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
												str.Format( "%ld: \"%s\" via pad to \"%s\" via hole (%s)\r\n", 
													nerrors, net2->name, net->name,
													d_str );
												DRError * dre = drelist->Add( nerrors, DRError::VIA_VIAHOLE, &str, 
													&net2->name, &net->name, id_via2, id_via1, xf, yf, xf2, yf2, 0, layer );
												if( dre )
												{
													nerrors++;
													log->AddLine( &str );
												}
											}
										}
									}
									// check net->via_hole to net2->via_hole clearance
									int d = GetClearanceBetweenPads( PAD_ROUND, post_vtx->x, post_vtx->y, post_vtx->via_hole_w, 0, 0, 0,
										PAD_ROUND, post_vtx2->x, post_vtx2->y, post_vtx2->via_hole_w, 0, 0,0  );
									if( d < dr->hole_hole )
									{
										// VIA_VIAHOLE
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										str.Format( "%ld: \"%s\" via hole to \"%s\" via hole (%s)\r\n", 
											nerrors, net2->name, net->name,
											d_str );
										DRError * dre = drelist->Add( nerrors, DRError::VIAHOLE_VIAHOLE, &str, 
											&net->name, &net2->name, id_via1, id_via2, xf, yf, xf2, yf2, 0, 0 );
										if( dre )
										{
											nerrors++;
											log->AddLine( &str );
										}
									}
								}
							}
						}
					}
				}
			}
		}
		// now iterate through all areas
		for( int ia=0; ia<net->nareas; ia++ )
		{
			carea * a = &net->area[ia];
			// iterate through all nets again
			POSITION pos2 = pos;
			void * ptr2;
			CString name2;
			while( pos2 != NULL )
			{
				m_nlist->m_map.GetNextAssoc( pos2, name2, ptr2 );
				cnet * net2 = (cnet*)ptr2;
				for( int ia2=0; ia2<net2->nareas; ia2++ )
				{
					carea * a2 = &net2->area[ia2];
					// test for same layer
					if( a->poly->GetLayer() == a2->poly->GetLayer() )
					{
						// now test spacing between areas
						for( int ic=0; ic<a->poly->GetNumCorners(); ic++ )
						{
							id id_a = net->id;
							id_a.st = ID_AREA;
							id_a.i = ia;
							id_a.sst = ID_SIDE;
							id_a.ii = ic;
							int ax1 = a->poly->GetX(ic);
							int ay1 = a->poly->GetY(ic);
							int ax2 = a->poly->GetX(0);
							int ay2 = a->poly->GetY(0);
							if( ic != a->poly->GetNumCorners()-1 )
							{
								ax2 = a->poly->GetX(ic+1);
								ay2 = a->poly->GetY(ic+1);
							}
							for( int ic2=0; ic2<a2->poly->GetNumCorners(); ic2++ )
							{
								id id_b = net2->id;
								id_b.st = ID_AREA;
								id_b.i = ia2;
								id_b.sst = ID_SIDE;
								id_b.ii = ic2;
								int bx1 = a2->poly->GetX(ic2);
								int by1 = a2->poly->GetY(ic2);
								int bx2 = a2->poly->GetX(0);
								int by2 = a2->poly->GetY(0);
								if( ic2 != a2->poly->GetNumCorners()-1 )
								{
									bx2 = a2->poly->GetX(ic2+1);
									by2 = a2->poly->GetY(ic2+1);
								}
								// for now, only works for straight copper area sides
								if( a->poly->GetSideStyle(ic) == CPolyLine::STRAIGHT
									&& a2->poly->GetSideStyle(ic2) == CPolyLine::STRAIGHT )
								{
									int x, y;
									int d = ::GetClearanceBetweenSegments( bx1, by1, bx2, by2, 0,
										ax1, ay1, ax2, ay2, 0, &x, &y );
									if( d < dr->copper_copper )
									{
										// COPPERAREA_COPPERAREA error
										::MakeCStringFromDimension( &d_str, d, units, TRUE, TRUE, TRUE, 1 );
										str.Format( "%ld: \"%s\" copper area to \"%s\" copper area (%s)\r\n",  
											nerrors, net->name, net2->name, d_str );
										DRError * dre = drelist->Add( nerrors, DRError::COPPERAREA_COPPERAREA, &str,
											&net->name, &net2->name, id_a, id_b, x, y, x, y, 0, 0 );
										if( dre )
										{
											nerrors++;
											log->AddLine( &str );
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	// now check for unrouted connections, if requested
	if( check_unrouted )
	{
		for( pos = m_nlist->m_map.GetStartPosition(); pos != NULL; )
		{
			m_nlist->m_map.GetNextAssoc( pos, name, ptr );
			cnet * net = (cnet*)ptr;
			// iterate through all connections
			// now check connections
			for( int ic=0; ic<net->connect.GetSize(); ic++ )
			{
				// check for unrouted or partially routed connection
				BOOL bUnrouted = FALSE;
				for( int is=0; is<net->connect[ic].nsegs; is++ )
				{
					if( net->connect[ic].seg[is].layer == LAY_RAT_LINE )
					{
						bUnrouted = TRUE;
						break;
					}
				}
				if( bUnrouted )
				{
					// unrouted or partially routed connection
					CString start_pin, end_pin;
					int istart = net->connect[ic].start_pin;
					cpart * start_part = net->pin[istart].part;
					start_pin = net->pin[istart].ref_des + "." + net->pin[istart].pin_name;
					int iend = net->connect[ic].end_pin;
					if( iend == cconnect::NO_END )
					{
						str.Format( "%ld: \"%s\": partially routed stub trace from %s\r\n",
							nerrors, net->name, start_pin );
						CPoint pt = GetPinPoint( start_part, &net->pin[istart].pin_name );
						id id_a = net->id;
						DRError * dre = drelist->Add( nerrors, DRError::UNROUTED, &str,
							&net->name, NULL, id_a, id_a, pt.x, pt.y, pt.x, pt.y, 0, 0 );
						if( dre )
						{
							nerrors++;
							log->AddLine( &str );
						}
					}
					else
					{
						end_pin = net->pin[iend].ref_des + "." + net->pin[iend].pin_name;
						if( net->connect[ic].nsegs > 1 )
						{
							str.Format( "%ld: \"%s\": partially routed connection from %s to %s\r\n",
								nerrors, net->name, start_pin, end_pin );
						}
						else
						{
							str.Format( "%ld: \"%s\": unrouted connection from %s to %s\r\n",
								nerrors, net->name, start_pin, end_pin );
						}
						CPoint pt = GetPinPoint( start_part, &net->pin[istart].pin_name );
						id id_a = net->id;
						DRError * dre = drelist->Add( nerrors, DRError::UNROUTED, &str,
							&net->name, NULL, id_a, id_a, pt.x, pt.y, pt.x, pt.y, 0, 0 );
						if( dre )
						{
							nerrors++;
							log->AddLine( &str );
						}
					}
				}
			}
		}
	}
	str = "\r\n***** DONE *****\r\n";
	log->AddLine( &str );
}

// check partlist for errors
//
int CPartList::CheckPartlist( CString * logstr )
{
	int nerrors = 0;
	int nwarnings = 0;
	CString str;
	CMapStringToPtr map;
	void * ptr;

	*logstr += "***** Checking Parts *****\r\n";

	// first, check for duplicate parts
	cpart * part = m_start.next;
	while( part->next != 0 )
	{
		CString ref_des = part->ref_des;
		BOOL test = map.Lookup( ref_des, ptr );
		if( test )
		{
			str.Format( "ERROR: Part \"%s\" duplicated\r\n", ref_des );
			str += "    ###   To fix this, delete one instance of the part, then save, close and re-open project\r\n";
			*logstr += str;
			nerrors++;
		}
		else
			map.SetAt( ref_des, NULL );

		// next part
		part = part->next;
	}

	// now check all parts
	part = m_start.next;
	while( part->next != 0 )
	{
		// check this part
		str = "";
		CString * ref_des = &part->ref_des;
		if( !part->shape )
		{
			// no footprint
			str.Format( "Warning: Part \"%s\" has no footprint\r\n",
				*ref_des );
			nwarnings++;
		}
		else
		{
			for( int ip=0; ip<part->pin.GetSize(); ip++ )
			{
				// check this pin
				cnet * net = part->pin[ip].net;
				CString * pin_name = &part->shape->m_padstack[ip].name;
				if( !net )
				{
					// part->pin->net is NULL, pin unconnected
					// this is not an error
					//				str.Format( "%s.%s unconnected\r\n",
					//					*ref_des, *pin_name );
				}
				else
				{
					cnet * netlist_net = m_nlist->GetNetPtrByName( &net->name );
					if( !netlist_net )
					{
						// part->pin->net->name doesn't exist in netlist
						str.Format( "ERROR: Part \"%s\" pin \"%s\" connected to net \"%s\" which doesn't exist in netlist\r\n",
							*ref_des, *pin_name, net->name );
						nerrors++;
					}
					else
					{
						if( net != netlist_net )
						{
							// part->pin->net doesn't match netlist->net
							str.Format( "ERROR: Part \"%s\" pin \"%s\" connected to net \"%s\" which doesn't match netlist\r\n",
								*ref_des, *pin_name, net->name );
							nerrors++;
						}
						else
						{
							// try to find pin in pin list for net
							int net_pin = -1;
							for( int ip=0; ip<net->npins; ip++ )
							{
								if( net->pin[ip].part == part )
								{
									if( net->pin[ip].pin_name == *pin_name )
									{
										net_pin = ip;
										break;
									}
								}
							}
							if( net_pin == -1 )
							{
								// pin not found
								str.Format( "ERROR: Part \"%s\" pin \"%s\" connected to net \"%\" but pin not in net\r\n",
									*ref_des, *pin_name, net->name );
								nerrors++;
							}
							else
							{
								// OK
							}

						}
					}
				}
			}
		}
		*logstr += str;

		// next part
		part = part->next;
	}
	str.Format( "***** %d ERROR(S), %d WARNING(S) *****\r\n", nerrors, nwarnings );
	*logstr += str;

	return nerrors;
}

void CPartList::MoveOrigin( int x_off, int y_off )
{
	cpart * part = GetFirstPart();
	while( part )
	{
		// move this part
		UndrawPart( part );
		part->x += x_off;
		part->y += y_off;
		for( int ip=0; ip<part->pin.GetSize(); ip++ )
		{
			part->pin[ip].x += x_off;
			part->pin[ip].y += y_off;
		}
		DrawPart( part );
		part = GetNextPart(part);
	}
}



