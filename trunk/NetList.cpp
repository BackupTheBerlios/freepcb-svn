// NetList.cpp: implementation of the CNetList class.
//

#include "stdafx.h"
#include <math.h>
#include <stdlib.h>
#include "DlgMyMessageBox.h"


#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//#define PROFILE		// profiles calls to OptimizeConnections() for "GND" 

POSITION g_pos;			// global iterator

BOOL bDontShowSelfIntersectionWarning = FALSE;
BOOL bDontShowSelfIntersectionArcsWarning = FALSE;
BOOL bDontShowIntersectionWarning = FALSE;
BOOL bDontShowIntersectionArcsWarning = FALSE;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

// carea constructor
carea::carea()
{
	m_dlist = 0;
	npins = 0;
	nstubs = 0;
	poly = 0;
	utility = 0;
}

void carea::Initialize( CDisplayList * dlist )
{
	m_dlist = dlist;
	poly = new CPolyLine( m_dlist );
}

// carea destructor
carea::~carea()
{
	if( m_dlist )
	{
		for( int ip=0; ip<npins; ip++ )
			m_dlist->Remove( dl_thermal[ip] );
		for( int is=0; is<nstubs; is++ )
			m_dlist->Remove( dl_stub_thermal[is] );
	}
	delete poly;
}

// carea copy constructor 
// doesn't actually copy anything but required for CArray<carea,carea>.InsertAt()
carea::carea( const carea& s )
{
	npins = 0;
	nstubs = 0;
	poly = new CPolyLine( m_dlist );
}

// carea assignment operator
// doesn't actually assign but required for CArray<carea,carea>.InsertAt to work
carea &carea::operator=( carea &a )
{
	return *this;
}

CNetList::CNetList( CDisplayList * dlist, CPartList * plist )
{
	m_dlist = dlist;			// attach display list
	m_plist = plist;			// attach part list
}

CNetList::~CNetList()
{
	RemoveAllNets();
}

// Add new net to netlist
//
cnet * CNetList::AddNet( CString name, int max_pins, int def_w, int def_via_w, int def_via_hole_w )
{
	// create new net
	cnet * new_net = new cnet( m_dlist );

	// set array sizes
	new_net->pin.SetSize( 0 );
	new_net->connect.SetSize( 0 );
	new_net->nconnects = 0;
	new_net->npins = 0;

	// zero areas
	new_net->nareas = 0;

	// set default trace width
	new_net->def_w = def_w;
	new_net->def_via_w = def_via_w;
	new_net->def_via_hole_w = def_via_hole_w;

	// create id and set name
	id id( ID_NET, 0 );
	new_net->id = id;
	new_net->name = name;

	// visible by default
	new_net->visible = 1;

	// add name and pointer to map
	m_map.SetAt( name, (void*)new_net );

	return new_net;
} 


// Remove net from list
//
void CNetList::RemoveNet( cnet * net )
{
	// remove pointers to net from pins on part
	for( int ip=0; ip<net->npins; ip++ )
	{
		cpart * pin_part = net->pin[ip].part;
		if( pin_part )
		{
			CShape * s = pin_part->shape; 
			if( s )
			{
				int pin_index = s->GetPinIndexByName( &net->pin[ip].pin_name );
				if( pin_index >= 0 )
					pin_part->pin[pin_index].net = NULL;
			}
		}
	}
	// destroy arrays
	net->connect.RemoveAll();
	net->pin.RemoveAll();
	net->area.RemoveAll();
	m_map.RemoveKey( net->name );
	delete( net );
}

// remove all nets from netlist
//
void CNetList::RemoveAllNets()
{
	// remove all nets
   POSITION pos;
   CString name;
   void * ptr;
   for( pos = m_map.GetStartPosition(); pos != NULL; )
   {
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		RemoveNet( net );
   }
   m_map.RemoveAll();
}

// Get first net in list
//
cnet * CNetList::GetFirstNet()
{
	CString name;
	void * ptr;

	g_pos = m_map.GetStartPosition(); 
	if( g_pos != NULL )
	{
		m_map.GetNextAssoc( g_pos, name, ptr );
		cnet * net = (cnet*)ptr;
		if( net == NULL )
			ASSERT(0);
		return net;
	}
	else
		return NULL;
}

// Get next net in list
//
cnet * CNetList::GetNextNet()
{
	CString name;
	void * ptr;

	if( g_pos == NULL )
		return NULL;
	else
	{
		m_map.GetNextAssoc( g_pos, name, ptr );
		cnet * net = (cnet*)ptr;
		if( net == NULL )
			ASSERT(0);
		return net;
	}
}

// set utility parameter of all nets
//
void CNetList::MarkAllNets( int utility )
{
	cnet * net = GetFirstNet();
	while( net != NULL )
	{
		net->utility = utility;
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			c->utility = FALSE;
			for( int is=0; is<c->nsegs+1; is++ )
			{
				if( is < c->nsegs )
					c->seg[is].utility = utility;
				c->vtx[is].utility = utility;
			}
		}
		net = GetNextNet();
	}
}

// move origin of coordinate system
//
void CNetList::MoveOrigin( int x_off, int y_off )
{
	// remove all nets
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c =  &net->connect[ic];
			UndrawConnection( net, ic );
			for( int iv=0; iv<=c->nsegs; iv++ )
			{
				cvertex * v = &c->vtx[iv];
				v->x += x_off;
				v->y += y_off;
			}
			DrawConnection( net, ic );
		}
		for( int ia=0; ia<net->area.GetSize(); ia++ )
		{
			carea * a = &net->area[ia];
			a->poly->MoveOrigin( x_off, y_off );
			SetAreaConnections( net, ia );
		}
	}
}

void CNetList::UndrawConnection( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	int nsegs = c->nsegs;
	int nvtx = nsegs + 1;
	for( int is=0; is<nsegs; is++ )
	{
		cseg * s = &c->seg[is];
		if( s->dl_el )
			m_dlist->Remove( s->dl_el );
		if( s->dl_sel )
			m_dlist->Remove( s->dl_sel );
		s->dl_el = NULL;
		s->dl_sel = NULL;
	}
	for( int iv=0; iv<nvtx; iv++ )
	{
		UndrawVia( net, ic, iv );
	}
}

void CNetList::DrawConnection( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	int nsegs = c->nsegs;
	for( int is=0; is<nsegs; is++ )
	{
		cseg * s = &c->seg[is];
		cvertex *pre_v = &c->vtx[is];
		cvertex *post_v = &c->vtx[is+1];
		id s_id = net->id;
		s_id.st = ID_CONNECT;
		s_id.i = ic;
		s_id.sst = ID_SEG;
		s_id.ii = is;
		s->dl_el = m_dlist->Add( s_id, net, s->layer, DL_LINE, net->visible, 
			s->width, 0, pre_v->x, pre_v->y, post_v->x, post_v->y,
			0, 0 );
		s_id.sst = ID_SEL_SEG;
		s->dl_sel = m_dlist->AddSelector( s_id, net, s->layer, DL_LINE, net->visible, 
			s->width, 0, pre_v->x, pre_v->y, post_v->x, post_v->y,
			0, 0 );
	}
	int nvtx;
	if( c->end_pin == cconnect::NO_END )
		nvtx = nsegs + 1;
	else
		nvtx = nsegs;
	for( int iv=1; iv<nvtx; iv++ )
		ReconcileVia( net, ic, iv );
}


// Add new pin to net
//
void CNetList::AddNetPin( cnet * net, CString * ref_des, CString * pin_name, BOOL set_areas )
{
	// set size of pin array
	net->pin.SetSize( net->npins + 1 );

	// add pin to array
	net->pin[net->npins].ref_des = *ref_des;
	net->pin[net->npins].pin_name = *pin_name;
	net->pin[net->npins].part = NULL;

	// now lookup part and hook to net if successful
	cpart * part = m_plist->GetPart( ref_des );
	if( part )
	{
		if( part->shape )
		{
			// hook part to net
			net->pin[net->npins].part = part;
			if( part->shape )
			{
				int pin_index = part->shape->GetPinIndexByName( pin_name );
				if( pin_index >= 0 )
				{
					// hook net to part
					part->pin[pin_index].net = net;
				}
			}
		}
	}

	net->npins++;
	// adjust connections to areas
	if( net->nareas && set_areas )
		SetAreaConnections( net );
}

// Remove pin from net (by reference designator and pin number)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part 
//
void CNetList::RemoveNetPin( cnet * net, CString * ref_des, CString * pin_name )
{
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].ref_des == *ref_des && net->pin[ip].pin_name == *pin_name )
		{
			net_pin = ip;
			break;
		}
	}
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part pin (if it exists)
	cpart * part = net->pin[net_pin].part;
	if( part )
	{
		if( part->shape )
		{
			int pin_index = part->shape->GetPinIndexByName( pin_name );
			if( pin_index != -1 )
				part->pin[pin_index].net = 0;
		}
	}
	// now remove pin from net
	net->pin.RemoveAt(net_pin);
	net->npins--;
	// now adjust pin numbers in remaining connections
	for( ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin > net_pin )
			c->start_pin--;
		if( c->end_pin > net_pin )
			c->end_pin--;
	}
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}

// Remove pin from net (by pin index)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part 
//
void CNetList::RemoveNetPin( cnet * net, int net_pin_index )
{
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin_index || c->end_pin == net_pin_index )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part pin (if it exists)
	cpart * part = net->pin[net_pin_index].part;
	if( part )
	{
		if( part->shape )
		{
			int part_pin_index = part->shape->GetPinIndexByName( &net->pin[net_pin_index].pin_name );
			if( part_pin_index != -1 )
				part->pin[part_pin_index].net = 0;
		}
	}
	// now remove pin from net
	net->pin.RemoveAt(net_pin_index);
	net->npins--;
	// now adjust pin numbers in remaining connections
	for( ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin > net_pin_index )
			c->start_pin--;
		if( c->end_pin > net_pin_index )
			c->end_pin--;
	}
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}

// Remove pin from net (by part and pin_name), including all connections to pin
//
void CNetList::RemoveNetPin( cpart * part, CString * pin_name )
{
	if( !part )
		ASSERT(0);
	if( !part->shape )
		ASSERT(0);
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	if( pin_index == -1 )
		ASSERT(0);
	cnet * net = (cnet*)part->pin[pin_index].net;
	if( net == 0 )
	{
		// no net attached to pin
		ASSERT(0);
	}
	// find pin in pin list for net
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
		ASSERT(0);
	}
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove pin from net
	net->pin.RemoveAt(net_pin);
	net->npins--;
	// now adjust pin numbers in remaining connections
	for( ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin > net_pin )
			c->start_pin--;
		if( c->end_pin > net_pin )
			c->end_pin--;
	}
	// now remove link to net from part
	part->pin[pin_index].net = NULL;
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}

// Remove connections to part->pin from part->pin->net
// set part->pin->net pointer and net->pin->part pointer to NULL
//
void CNetList::DisconnectNetPin( cpart * part, CString * pin_name )
{
	if( !part )
		ASSERT(0);
	if( !part->shape )
		ASSERT(0);
	int pin_index = part->shape->GetPinIndexByName( pin_name );
	if( pin_index == -1 )
		ASSERT(0);
	cnet * net = (cnet*)part->pin[pin_index].net;
	if( net == 0 )
	{
		return;
	}
	// find pin in pin list for net
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
		ASSERT(0);
	}
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part
	part->pin[pin_index].net = NULL;
	// now remove link to part from net
	net->pin[net_pin].part = NULL;
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}


// Disconnect pin from net (by reference designator and pin number)
// Use this if the part may not actually exist in the partlist,
// or the pin may not exist in the part 
//
void CNetList::DisconnectNetPin( cnet * net, CString * ref_des, CString * pin_name )
{
	// find pin in pin list for net
	int net_pin = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].ref_des == *ref_des && net->pin[ip].pin_name == *pin_name )
		{
			net_pin = ip;
			break;
		}
	}
	if( net_pin == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	// now remove all connections to/from this pin
	int ic = 0;
	while( ic<net->nconnects )
	{
		cconnect * c = &net->connect[ic];
		if( c->start_pin == net_pin || c->end_pin == net_pin )
			RemoveNetConnect( net, ic, FALSE );
		else
			ic++;
	}
	// now remove link to net from part pin (if it exists)
	cpart * part = net->pin[net_pin].part;
	if( part )
	{
		if( part->shape )
		{
			int pin_index = part->shape->GetPinIndexByName( pin_name );
			if( pin_index != -1 )
				part->pin[pin_index].net = NULL;
		}
	}
	net->pin[net_pin].part = NULL;
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}

// Add new connection to net, consisting of one unrouted segment
// p1 and p2 are indexes into pin array for this net
// returns index to connection, or -1 if fails
//
int CNetList::AddNetConnect( cnet * net, int p1, int p2 )
{
	if( net->nconnects != net->connect.GetSize() )
		ASSERT(0);

	net->connect.SetSize( net->nconnects + 1 );
	net->connect[net->nconnects].seg.SetSize( 1 );
	net->connect[net->nconnects].seg[0].Initialize( m_dlist );
	net->connect[net->nconnects].vtx.SetSize( 2 );
	net->connect[net->nconnects].vtx[0].Initialize( m_dlist );
	net->connect[net->nconnects].vtx[1].Initialize( m_dlist );
	net->connect[net->nconnects].nsegs = 1;
	net->connect[net->nconnects].locked = 0;
	net->connect[net->nconnects].start_pin = p1;
	net->connect[net->nconnects].end_pin = p2;

	// check for valid pins
	cpart * part1 = net->pin[p1].part;
	cpart * part2 = net->pin[p2].part;
	if( part1 == 0 || part2 == 0 )
		return -1;
	CShape * shape1 = part1->shape;
	CShape * shape2 = part2->shape;
	if( shape1 == 0 || shape2 == 0 )
		return -1;
	int pin_index1 = shape1->GetPinIndexByName( &net->pin[p1].pin_name );
	int pin_index2 = shape2->GetPinIndexByName( &net->pin[p2].pin_name );
	if( pin_index1 == -1 || pin_index2 == -1 )
		return -1;

	// add a single unrouted segment
	CPoint pi, pf;
	pi = m_plist->GetPinPoint( net->pin[p1].part, &net->pin[p1].pin_name );
	pf = m_plist->GetPinPoint( net->pin[p2].part, &net->pin[p2].pin_name );
	int xi = pi.x;
	int yi = pi.y;
	int xf = pf.x;
	int yf = pf.y;
	net->connect[net->nconnects].seg[0].layer = LAY_RAT_LINE;
	net->connect[net->nconnects].seg[0].width = 0;
	net->connect[net->nconnects].seg[0].selected = 0;

	net->connect[net->nconnects].vtx[0].x = xi;
	net->connect[net->nconnects].vtx[0].y = yi;
	net->connect[net->nconnects].vtx[0].pad_layer = m_plist->GetPinLayer( net->pin[p1].part, &net->pin[p1].pin_name );
	net->connect[net->nconnects].vtx[0].force_via_flag = 0;
	net->connect[net->nconnects].vtx[0].via_w = 0;
	net->connect[net->nconnects].vtx[0].via_hole_w = 0;

	net->connect[net->nconnects].vtx[1].x = xf;
	net->connect[net->nconnects].vtx[1].y = yf;
	net->connect[net->nconnects].vtx[1].pad_layer = m_plist->GetPinLayer( net->pin[p2].part, &net->pin[p2].pin_name );
	net->connect[net->nconnects].vtx[1].force_via_flag = 0;
	net->connect[net->nconnects].vtx[1].via_w = 0;
	net->connect[net->nconnects].vtx[1].via_hole_w = 0;

	// create id for this segment
	id id( ID_NET, ID_CONNECT, net->nconnects );

	// draw graphic elements for segment
	id.sst = ID_SEG;
	net->connect[net->nconnects].seg[0].dl_el = m_dlist->Add( id, net, LAY_RAT_LINE, DL_LINE, 
		net->visible, 0, 0, xi, yi, xf, yf, 0, 0 ); 
	id.sst = ID_SEL_SEG;
	net->connect[net->nconnects].seg[0].dl_sel = m_dlist->AddSelector( id, net, LAY_RAT_LINE, DL_LINE,
		net->visible, 0, 0, xi, yi, xf, yf, 0, 0 ); 
	net->connect[net->nconnects].vtx[0].dl_sel = NULL;
	net->connect[net->nconnects].vtx[1].dl_sel = NULL;
	net->nconnects++;

	return net->nconnects-1;
}

// add connection to net consisting of starting vertex only
// i.e. this will be a stub trace with no end pin
// returns index to connection or -1 if fails
//
int CNetList::AddNetStub( cnet * net, int p1 )
{
	if( net->nconnects != net->connect.GetSize() )
		ASSERT(0);

	if( net->pin[p1].part == 0 )
		return -1;

	net->connect.SetSize( net->nconnects + 1 );
	net->connect[net->nconnects].seg.SetSize( 0 );
	net->connect[net->nconnects].vtx.SetSize( 1 );
	net->connect[net->nconnects].vtx[0].Initialize( m_dlist );
	net->connect[net->nconnects].nsegs = 0;
	net->connect[net->nconnects].locked = 0;
	net->connect[net->nconnects].start_pin = p1;
	net->connect[net->nconnects].end_pin = cconnect::NO_END;

	// add a single vertex
	CPoint pi;
	pi = m_plist->GetPinPoint( net->pin[p1].part, &net->pin[p1].pin_name );
	net->connect[net->nconnects].vtx[0].x = pi.x;
	net->connect[net->nconnects].vtx[0].y = pi.y;
	net->connect[net->nconnects].vtx[0].pad_layer = m_plist->GetPinLayer( net->pin[p1].part, &net->pin[p1].pin_name );
	net->connect[net->nconnects].vtx[0].force_via_flag = 0;
	net->connect[net->nconnects].vtx[0].via_w = 0;
	net->connect[net->nconnects].vtx[0].via_hole_w = 0;
	net->connect[net->nconnects].vtx[0].dl_sel = 0;

	net->nconnects++;
	return net->nconnects-1;
}

// test for hit on end-pad of connection
// if dir == 0, check end pad
// if dir == 1, check start pad
//
BOOL CNetList::TestHitOnConnectionEndPad( int x, int y, cnet * net, int ic, 
										 int layer, int dir )
{
	int ip;
	cconnect * c =&net->connect[ic];
	if( dir == 1 )
	{
		// get first pad in connection
		ip = c->start_pin;
	}
	else
	{
		// get last pad in connection
		ip = c->end_pin;
	}
	if( ip != cconnect::NO_END )
	{
		cpart * part = net->pin[ip].part;
		CString pin_name = net->pin[ip].pin_name;
		if( !part )
			ASSERT(0);
		if( !part->shape )
			ASSERT(0);
		return( m_plist->TestHitOnPad( part, &pin_name, x, y, layer ) );
	}
	else
		return FALSE;
}

// test for hit on any pad on this net
// returns -1 if not found, otherwise index into net->pin[]
//
int CNetList::TestHitOnAnyPadInNet( int x, int y, int layer, cnet * net )
{
	int ix = -1;
	for( int ip=0; ip<net->pin.GetSize(); ip++ )
	{
		cpart * part = net->pin[ip].part;
		if( part )
		{
			CString pin_name = net->pin[ip].pin_name;
//			if( !part )
//				ASSERT(0);
//			if( !part->shape )
//				ASSERT(0);
			if( m_plist->TestHitOnPad( part, &pin_name, x, y, layer ) )
			{
				ix = ip;
				break;
			}
		}
	}
	return ix;
}

// Clean up connections by removing zero-length segments and combining segments
//
void CNetList::CleanUpConnections( cnet * net, CString * logstr )
{
	for( int ic=net->nconnects-1; ic>=0; ic-- )  
	{
		UndrawConnection( net, ic );
		cconnect * c = &net->connect[ic];
		for( int is=c->nsegs-1; is>=0; is-- )
		{
			int pre_layer, post_layer;
			if( is == 0 )
				pre_layer = c->vtx[0].pad_layer;
			else
				pre_layer = c->seg[is-1].layer;
			if( is == c->nsegs-1 && c->end_pin == cconnect::NO_END )
				post_layer = pre_layer;	// since there is really no post_layer
			else if( is == c->nsegs-1 && c->end_pin != cconnect::NO_END )
				post_layer = c->vtx[is+1].pad_layer;
			else
				post_layer = c->seg[is+1].layer;
			if( pre_layer == post_layer )
			{
				if( c->vtx[is].x == c->vtx[is+1].x && c->vtx[is].y == c->vtx[is+1].y )
				{
					if( is == c->nsegs-1 )
						c->vtx.RemoveAt(is);	// last segment, don't delete last vertex
					else
						c->vtx.RemoveAt(is+1);
					if( logstr )
					{
						CString str;
						if( c->end_pin == cconnect::NO_END )
						{
							str.Format( "net %s: stub trace from %s.%s: removing zero-length segment\r\n",
								net->name, 
								net->pin[c->start_pin].ref_des, net->pin[c->start_pin].pin_name ); 
						}
						else
						{
							str.Format( "net %s: trace %s.%s to %s.%s: removing zero-length segment\r\n",
								net->name, 
								net->pin[c->start_pin].ref_des, net->pin[c->start_pin].pin_name, 
								net->pin[c->end_pin].ref_des, net->pin[c->end_pin].pin_name ); 
						}
						*logstr += str;
					}
					c->seg.RemoveAt(is);
					c->nsegs--;
				}
			}
		}
		if( c->nsegs == 0 )
		{
			net->connect.RemoveAt(ic);
			net->nconnects--;
		}
		else
		{
			for( int is=c->nsegs-2; is>=0; is-- ) 
			{
				if( c->seg[is].layer == c->seg[is+1].layer && c->seg[is].width == c->seg[is+1].width )
				{ 
					double dx1 = c->vtx[is+1].x - c->vtx[is].x;
					double dy1 = c->vtx[is+1].y - c->vtx[is].y;
					double dx2 = c->vtx[is+2].x - c->vtx[is+1].x;
					double dy2 = c->vtx[is+2].y - c->vtx[is+1].y;
					if( dy1*dx2 == dy2*dx1 && (dx1*dx2>0.0 || dy1*dy2>0.0) )
					{
						// combine these segments
						if( logstr )
						{
							CString str;
							if( c->end_pin == cconnect::NO_END )
							{
								str.Format( "net %s: stub trace from %s.%s: combining colinear segments\r\n",
									net->name, 
									net->pin[c->start_pin].ref_des, net->pin[c->start_pin].pin_name ); 
							}
							else
							{
								str.Format( "net %s: trace %s.%s to %s.%s: combining colinear segments\r\n",
									net->name, 
									net->pin[c->start_pin].ref_des, net->pin[c->start_pin].pin_name, 
									net->pin[c->end_pin].ref_des, net->pin[c->end_pin].pin_name ); 
							}
							*logstr += str;
						}
						c->vtx.RemoveAt(is+1);
						c->seg.RemoveAt(is+1);
						c->nsegs--;
					}
				}
			}
			DrawConnection( net, ic );
		}
	}
	RenumberConnections( net );
}


void CNetList::CleanUpAllConnections( CString * logstr )
{
	cnet * net = GetFirstNet();
	while( net )
	{
		CleanUpConnections( net, logstr );
		net = GetNextNet();
	}
}


// Remove connection from net, including routed segments
// Leave pins in pin list for net
//
int CNetList::RemoveNetConnect( cnet * net, int ic, BOOL set_areas )
{
	net->connect.RemoveAt( ic );
	net->nconnects--;
	RenumberConnections( net );
	// adjust connections to areas
	if( net->nareas && set_areas )
		SetAreaConnections( net );
	return 0;
}

// Unroute connection
//
int CNetList::UnrouteNetConnect( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	UnrouteSegment( net, ic, 0 );
	while( c->seg.GetSize() > 1 )
	{
		UnrouteSegment( net, ic, 1 );
	}
	return 0;
}

// Change the start or end pin of a connection and redraw it
//
void CNetList::ChangeConnectionPin( cnet * net, int ic, int end_flag, 
								   cpart * part, CString * pin_name )
{
	// find pin in pin list for net
	int pin_index = -1;
	for( int ip=0; ip<net->npins; ip++ )
	{
		if( net->pin[ip].part == part && net->pin[ip].pin_name == *pin_name )
		{
			pin_index = ip;
			break;
		}
	}
	if( pin_index == -1 )
	{
		// pin not found
		ASSERT(0);
	}
	cconnect * c = &net->connect[ic];
	cpin * pin = &net->pin[pin_index];
	CPoint p = m_plist->GetPinPoint( part, pin_name );
	int layer = m_plist->GetPinLayer( part, pin_name );
	if( end_flag )
	{
		// change end pin
		int is = c->nsegs-1;
		c->end_pin = pin_index;
		c->vtx[is+1].x = p.x;
		c->vtx[is+1].y = p.y;
		c->vtx[is+1].pad_layer = layer;
		m_dlist->Set_xf( c->seg[is].dl_el, p.x );
		m_dlist->Set_yf( c->seg[is].dl_el, p.y );
		m_dlist->Set_xf( c->seg[is].dl_sel, p.x );
		m_dlist->Set_yf( c->seg[is].dl_sel, p.y );
	}
	else
	{
		// change start pin
		c->start_pin = pin_index;
		c->vtx[0].x = p.x;
		c->vtx[0].y = p.y;
		c->vtx[0].pad_layer = layer;
		m_dlist->Set_x( c->seg[0].dl_el, p.x );
		m_dlist->Set_y( c->seg[0].dl_el, p.y );
		m_dlist->Set_x( c->seg[0].dl_sel, p.x );
		m_dlist->Set_y( c->seg[0].dl_sel, p.y );
	}
}



// Unroute segment
// return id of new segment (since seg[] array may change as a result)
//
id CNetList::UnrouteSegment( cnet * net, int ic, int is )
{
	cconnect * c = &net->connect[ic];
	id seg_id = c->seg[is].dl_sel->id;
	UnrouteSegmentWithoutMerge( net, ic, is );
	id mid = MergeUnroutedSegments( net, ic );
	if( mid.type == 0 )
		mid = seg_id;
	return mid;
}

// Merge any adjacent unrouted segment of this connection
// Returns id of first merged segment in connection
//
id CNetList::MergeUnroutedSegments( cnet * net, int ic )
{
	id mid( ID_NET, ID_CONNECT, ic, ID_SEL_SEG, -1 );

	cconnect * c = &net->connect[ic];
	if( c->nsegs == 1 )
	{
		mid.Clear();
		return mid;
	}

	UndrawConnection( net, ic );
	for( int is=c->nsegs-2; is>=0; is-- )
	{
		cseg * post_s = &c->seg[is+1];
		cseg * s = &c->seg[is];
		if( post_s->layer == LAY_RAT_LINE && s->layer == LAY_RAT_LINE )
		{
			// this segment and next are unrouted, 
			// remove next segment and interposed vertex
			c->seg.RemoveAt(is+1);
			c->vtx.RemoveAt(is+1);
			c->nsegs = c->seg.GetSize();
			mid.ii = is;
		}
	}
	if( mid.ii == -1 )
		mid.Clear();
	DrawConnection( net, ic );
	return mid;
}

// Unroute segment, but don't merge with adjacent unrouted segments
//
void CNetList::UnrouteSegmentWithoutMerge( cnet * net, int ic, int is )
{
	cconnect * c = &net->connect[ic];
	// remove any adjacent vias
	UndrawVia( net, ic, is );
	c->vtx[is].force_via_flag = 0;
	c->vtx[is].via_w = 0;
	UndrawVia( net, ic, is+1 );
	c->vtx[is+1].force_via_flag = 0;
	c->vtx[is+1].via_w = 0;

	// unroute segment
	c->seg[is].layer = LAY_RAT_LINE;
	c->seg[is].width = 0;

	// redraw segment, unless previously undrawn
	if( c->seg[is].dl_el )
	{
		int xi, yi, xf, yf;
		xi = c->vtx[is].x;
		yi = c->vtx[is].y;
		xf = c->vtx[is+1].x;
		yf = c->vtx[is+1].y;
		id seg_id = c->seg[is].dl_el->id;
		id sel_id = c->seg[is].dl_sel->id;
		m_dlist->Remove( c->seg[is].dl_el );
		m_dlist->Remove( c->seg[is].dl_sel );
		c->seg[is].dl_el = m_dlist->Add( seg_id, net, LAY_RAT_LINE, DL_LINE, 
			net->visible, 1, 0, xi, yi, xf, yf, 0, 0 );
		c->seg[is].dl_sel = m_dlist->AddSelector( sel_id, net, LAY_RAT_LINE, DL_LINE, 
			net->visible, 1, 0, xi, yi, xf, yf, 0, 0 );
	}
}

// Remove segment ... currently only used for last segment of stub trace
// if previous segments are unrouted, removes them too
//
void CNetList::RemoveSegment( cnet * net, int ic, int is )
{
	cconnect * c = &net->connect[ic];
	if( is != (c->nsegs-1) )
		ASSERT(0);
	else
	{
		c->seg.RemoveAt(is);
		c->vtx.RemoveAt(is+1);
		c->nsegs--;
		if( c->nsegs == 0 )
		{
			net->connect.RemoveAt(ic);
			net->nconnects--;
			RenumberConnections( net );
		}
		else
		{
			c->vtx[c->nsegs].dl_sel->id.sst = ID_SEL_VERTEX;
			if( c->seg[is-1].layer == LAY_RAT_LINE )
			{
				c->seg.RemoveAt(is-1);
				c->vtx.RemoveAt(is);
				c->nsegs--;
				if( c->nsegs == 0 )
				{
					net->connect.RemoveAt(ic);
					net->nconnects--;
					RenumberConnections( net );
				}
				else
					c->vtx[c->nsegs].dl_sel->id.sst = ID_SEL_VERTEX;
			}
		}
	}
	// adjust connections to areas
	if( net->nareas )
		SetAreaConnections( net );
}

// renumber all ids and dl_elements for net connections
// should be used after deleting a connection
//
void CNetList::RenumberConnections( cnet * net )
{
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		RenumberConnection( net, ic );
	}
}

// renumber all ids and dl_elements for net connection
//
void CNetList::RenumberConnection( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->nsegs; is++ )
	{
		if( c->seg[is].dl_el )
		{
			c->seg[is].dl_el->id.i = ic;
			c->seg[is].dl_el->id.ii = is;
		}
		if( c->seg[is].dl_sel )
		{
			c->seg[is].dl_sel->id.i = ic;
			c->seg[is].dl_sel->id.ii = is;
		}
	}
	for( int iv=0; iv<=c->nsegs; iv++ )
	{
		for( int il=0; il<c->vtx[iv].dl_el.GetSize(); il++ )
		{
			if( c->vtx[iv].dl_el[il] )
			{
				c->vtx[iv].dl_el[il]->id.i = ic;
				c->vtx[iv].dl_el[il]->id.ii = iv;
			}
		}
		if( c->vtx[iv].dl_sel )
		{
			c->vtx[iv].dl_sel->id.i = ic;
			c->vtx[iv].dl_sel->id.ii = iv;
		}
		if( c->vtx[iv].dl_hole )
		{
			c->vtx[iv].dl_hole->id.i = ic;
			c->vtx[iv].dl_hole->id.ii = iv;
		}
	}
}

// renumber the ids for graphical elements in areas
// should be called after deleting an area
//
void CNetList::RenumberAreas( cnet * net )
{
	id a_id;
	for( int ia=0; ia<net->nareas; ia++ )
	{
		a_id = net->area[ia].poly->GetId();
		a_id.i = ia;
		net->area[ia].poly->SetId( &a_id );
		for( int ip=0; ip<net->area[ia].npins; ip++ )
		{
			id a_id = m_dlist->Get_id( net->area[ia].dl_thermal[ip] );
			a_id.i = ia;
			m_dlist->Set_id( net->area[ia].dl_thermal[ip], &a_id );
		}
	}
}

// Set segment layer (must be a copper layer, not the ratline layer)
// returns 1 if unable to comply due to SMT pad
//
int CNetList::ChangeSegmentLayer( cnet * net, int ic, int iseg, int layer )
{
	cconnect * c = &net->connect[ic];
	// check layer settings of adjacent vertices to make sure this is legal
	if( iseg == 0 )
	{
		// first segment, check starting pad layer
		int pad_layer = c->vtx[0].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if( iseg == (c->nsegs - 1) && c->end_pin != cconnect::NO_END )
	{
		// last segment, check destination pad layer
		int pad_layer = c->vtx[iseg+1].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	// change segment layer
	cseg * s = &c->seg[iseg];
	cvertex * pre_v = &c->vtx[iseg];
	cvertex * post_v = &c->vtx[iseg+1];
	s->layer = layer;

	// get old graphic elements
	dl_element * old_el = c->seg[iseg].dl_el;
	dl_element * old_sel = c->seg[iseg].dl_sel;

	// create new graphic elements
	dl_element * new_el = m_dlist->Add( old_el->id, old_el->ptr, layer, old_el->gtype,
		old_el->visible, s->width, 0, pre_v->x, pre_v->y,
		post_v->x, post_v->y, 0, 0, 0, layer );

	dl_element * new_sel = m_dlist->AddSelector( old_sel->id, old_sel->ptr, layer, 
		old_sel->gtype, old_sel->visible, s->width, 0, pre_v->x, pre_v->y,
		post_v->x, post_v->y, 0, 0, 0 );

	// remove old graphic elements
	m_dlist->Remove( old_el );
	m_dlist->Remove( old_sel );

	// add new graphics
	c->seg[iseg].dl_el = new_el;
	c->seg[iseg].dl_sel = new_sel;

	// now adjust vias
	ReconcileVia( net, ic, iseg );
	ReconcileVia( net, ic, iseg+1 );
	return 0;
}

// Convert segment from unrouted to routed
// returns 1 if segment can't be routed on given layer due to connection to SMT pad
// Adds/removes vias as necessary
//
int CNetList::RouteSegment( cnet * net, int ic, int iseg, int layer, int width )
{
	// check layer settings of adjacent vertices to make sure this is legal
	cconnect * c =&net->connect[ic];
	if( iseg == 0 )
	{
		// first segment, check starting pad layer
		int pad_layer = c->vtx[0].pad_layer;
		if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
			return 1;
	}
	if( iseg == (c->nsegs - 1) )
	{
		// last segment, check destination pad layer
		if( c->end_pin == cconnect::NO_END )
		{
		}
		else
		{
			int pad_layer = c->vtx[iseg+1].pad_layer;
			if( pad_layer != LAY_PAD_THRU && layer != pad_layer )
				return 1;
		}
	}

	// remove old graphic elements
	m_dlist->Remove( c->seg[iseg].dl_el );
	m_dlist->Remove( c->seg[iseg].dl_sel );

	// modify segment parameters
	c->seg[iseg].layer = layer;
	c->seg[iseg].width = width;
	c->seg[iseg].selected = 0;

	// draw elements
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	int xf = c->vtx[iseg+1].x;
	int yf = c->vtx[iseg+1].y;
	id id( ID_NET, ID_CONNECT, ic, ID_SEG, iseg );
	c->seg[iseg].dl_el = m_dlist->Add( id, net, layer, DL_LINE, 
		1, width, 0, xi, yi, xf, yf, 0, 0 );
	id.sst = ID_SEL_SEG;
	c->seg[iseg].dl_sel = m_dlist->AddSelector( id, net, layer, DL_LINE, 
		1, width, 0, xi, yi, xf, yf, 0, 0 ); 

	// now adjust vias
	ReconcileVia( net, ic, iseg );
	ReconcileVia( net, ic, iseg+1 );
	return 0;
}


// Append new segment to connection 
// this is mainly used for stub traces
// returns index to new segment
//
int CNetList::AppendSegment( cnet * net, int ic, int x, int y, int layer, int width,
						 int via_width, int via_hole_width )
{
	// add new vertex and segment
	cconnect * c =&net->connect[ic];
	c->seg.SetSize( c->nsegs + 1 );
	c->seg[c->nsegs].Initialize( m_dlist );
	c->vtx.SetSize( c->nsegs + 2 );
	c->vtx[c->nsegs].Initialize( m_dlist );;
	c->vtx[c->nsegs+1].Initialize( m_dlist );;
	int iseg = c->nsegs;

	// set position for new vertex, zero dl_element pointers
	c->vtx[iseg+1].x = x;
	c->vtx[iseg+1].y = y;
	UndrawVia( net, ic, iseg+1 );

	// create new segment
	c->seg[iseg].layer = layer;
	c->seg[iseg].width = width;
	c->seg[iseg].selected = 0;
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	id id( ID_NET, ID_CONNECT, ic, ID_SEG, iseg );
	c->seg[iseg].dl_el = m_dlist->Add( id, net, layer, DL_LINE, 
		1, width, 0, xi, yi, x, y, 0, 0 );
	id.sst = ID_SEL_SEG;
	c->seg[iseg].dl_sel = m_dlist->AddSelector( id, net, layer, DL_LINE, 
		1, width, 0, xi, yi, x, y, 0, 0 ); 
	id.sst = ID_SEL_VERTEX;
	id.ii = iseg+1;
	c->vtx[iseg+1].dl_sel = m_dlist->AddSelector( id, net, layer, DL_HOLLOW_RECT, 
		1, 0, 0, x-10*PCBU_PER_WU, y-10*PCBU_PER_WU, 
		x+10*PCBU_PER_WU, y+10*PCBU_PER_WU, 0, 0 ); 

	// done
	c->nsegs++;

	// take care of preceding via
	ReconcileVia( net, ic, iseg );

	return iseg;
}

// Insert new segment into connection, unless the new segment ends at the 
// endpoint of the old segment, then replace old segment 
// if dir=0 add forward in array, if dir=1 add backwards
// return 1 if segment inserted, 0 if replaced 
// tests position within +/- 10 nm.
//
int CNetList::InsertSegment( cnet * net, int ic, int iseg, int x, int y, int layer, int width,
						 int via_width, int via_hole_width, int dir )
{
	const int TOL = 10;

	// see whether we need to insert new segment or just modify old segment
	cconnect * c = &net->connect[ic];
	int insert_flag = 1;
	if( dir == 0 )
	{
		// routing forward
		if( (abs(x-c->vtx[iseg+1].x) + abs(y-c->vtx[iseg+1].y )) < TOL )
		{ 
			// new vertex is the same as end of old segment 
			if( iseg < (c->nsegs-1) )
			{
				// not the last segment
				if( layer == c->seg[iseg+1].layer )
				{
					// next segment routed on same layer, don't insert new seg
					insert_flag = 0;
				}
			}
			else if( iseg == (c->nsegs-1) )
			{
				// last segment, should connect to pad
				int pad_layer = c->vtx[iseg+1].pad_layer;
				if( pad_layer == LAY_PAD_THRU || layer == LAY_RAT_LINE
					|| (pad_layer == LAY_TOP_COPPER && layer == LAY_TOP_COPPER)
					|| (pad_layer == LAY_BOTTOM_COPPER && layer == LAY_BOTTOM_COPPER) )
				{
					// layer OK to connect to pad, don't insert new seg
					insert_flag = 0;
				}
			}
		}
	}
	else
	{
		// routing backward
		if( x == c->vtx[iseg].x 
			&& y == c->vtx[iseg].y )
		{ 
			// new vertex is the same as start of old segment 
			if( iseg >0 )
			{
				// not the first segment
				if( layer == c->seg[iseg-1].layer )
				{
					// prev segment routed on same layer, don't insert new seg
					insert_flag = 0;
				}
			}
			else if( iseg == 0 )
			{
				// first segment, should connect to pad
				int pad_layer = c->vtx[iseg].pad_layer;
				if( pad_layer == LAY_PAD_THRU || layer == LAY_RAT_LINE
					|| (pad_layer == LAY_TOP_COPPER && layer == LAY_TOP_COPPER)
					|| (pad_layer == LAY_BOTTOM_COPPER && layer == LAY_BOTTOM_COPPER) )
				{
					// layer OK to connect to pad, don't insert new seg
					insert_flag = 0;
				}
			}
		}
	}

	if( insert_flag )
	{
		// insert new vertex and segment
		c->seg.SetSize( c->nsegs + 1 );
		c->seg[c->nsegs].Initialize( m_dlist );
		c->vtx.SetSize( c->nsegs + 2 );
		c->vtx[c->nsegs].Initialize( m_dlist );
		c->vtx[c->nsegs+1].Initialize( m_dlist );

		// shift higher segments and vertices up to make room
		for( int i=c->nsegs; i>iseg; i-- )
		{
			c->seg[i] = c->seg[i-1];
			if( c->seg[i].dl_el )
				c->seg[i].dl_el->id.ii = i;
			if( c->seg[i].dl_sel )
				c->seg[i].dl_sel->id.ii = i;
			c->vtx[i+1] = c->vtx[i];
			if( c->vtx[i+1].dl_sel )
				c->vtx[i+1].dl_sel->id.ii = i+1;
			if( c->vtx[i+1].dl_hole )
				c->vtx[i+1].dl_hole->id.ii = i+1;
			for( int il=0; il<c->vtx[i+1].dl_el.GetSize(); il++ )
			{
				if( c->vtx[i+1].dl_el[il] )
					c->vtx[i+1].dl_el[il]->id.ii = i+1;
			}
			if( c->vtx[i+1].dl_hole )
				c->vtx[i+1].dl_hole->id.ii = i+1;
		}
		// note that seg[iseg+1] now duplicates seg[iseg], vtx[iseg+2] duplicates vtx[iseg+1]
		// we must replace or zero the dl_element pointers for seg[iseg+1]
		// 
		// set position for new vertex, zero dl_element pointers
		c->vtx[iseg+1].x = x;
		c->vtx[iseg+1].y = y;
		UndrawVia( net, ic, iseg+1 );
		
		// fill in data for new seg[iseg] or seg[is+1] (depending on dir)
		if( dir == 0 )
		{
			// route forward
			c->seg[iseg].layer = layer;
			c->seg[iseg].width = width;
			c->seg[iseg].selected = 0;
			int xi = c->vtx[iseg].x;
			int yi = c->vtx[iseg].y;
			id id( ID_NET, ID_CONNECT, ic, ID_SEG, iseg );
			c->seg[iseg].dl_el = m_dlist->Add( id, net, layer, DL_LINE, 
				1, width, 0, xi, yi, x, y, 0, 0 );
			id.sst = ID_SEL_SEG;
			c->seg[iseg].dl_sel = m_dlist->AddSelector( id, net, layer, DL_LINE, 
				1, width, 0, xi, yi, x, y, 0, 0 ); 
			id.sst = ID_SEL_VERTEX;
			id.ii = iseg+1;
			c->vtx[iseg+1].dl_sel = m_dlist->AddSelector( id, net, layer, DL_HOLLOW_RECT, 
				1, 1, 0, x-10*PCBU_PER_WU, y-10*PCBU_PER_WU, 
				x+10*PCBU_PER_WU, y+10*PCBU_PER_WU, 0, 0 ); 
		}
		else
		{
			// route backward
			c->seg[iseg+1].layer = layer;
			c->seg[iseg+1].width = width;
			c->seg[iseg+1].selected = 0;
			int xf = c->vtx[iseg+2].x;
			int yf = c->vtx[iseg+2].y;
			id id( ID_NET, ID_CONNECT, ic, ID_SEG, iseg+1 );;
			c->seg[iseg+1].dl_el = m_dlist->Add( id, net, layer, DL_LINE, 
				1, width, 0, x, y, xf, yf, 0, 0 );
			id.sst = ID_SEL_SEG;
			c->seg[iseg+1].dl_sel = m_dlist->AddSelector( id, net, layer, DL_LINE, 
				1, width, 0, x, y, xf, yf, 0, 0 ); 
			id.sst = ID_SEL_VERTEX;
			id.ii = iseg+1;
			c->vtx[iseg+1].dl_sel = m_dlist->AddSelector( id, net, layer, DL_HOLLOW_RECT, 
				1, 0, 0, x-10*PCBU_PER_WU, y-10*PCBU_PER_WU, x+10*PCBU_PER_WU, y+10*PCBU_PER_WU, 0, 0 ); 
		}
		
		// modify adjacent old segment for new endpoint
		if( dir == 0 ) 
		{
			// adjust next segment for new starting position, and make visible
			m_dlist->Set_x(c->seg[iseg+1].dl_el, x);
			m_dlist->Set_y(c->seg[iseg+1].dl_el, y);
			if( c->seg[iseg+1].dl_el )
				c->seg[iseg+1].dl_el->id.ii = iseg+1;
			m_dlist->Set_visible(c->seg[iseg+1].dl_el, 1);
			m_dlist->Set_x(c->seg[iseg+1].dl_sel, x);
			m_dlist->Set_y(c->seg[iseg+1].dl_sel, y);
			if( c->seg[iseg+1].dl_sel )
				m_dlist->Set_visible(c->seg[iseg+1].dl_sel, 1);
		}
		if( dir == 1 ) 
		{
			// adjust previous segment for new ending position, and make visible
			m_dlist->Set_xf(c->seg[iseg].dl_el, x);
			m_dlist->Set_yf(c->seg[iseg].dl_el, y);
			if( c->seg[iseg].dl_el )
				c->seg[iseg].dl_el->id.ii = iseg;
			m_dlist->Set_visible(c->seg[iseg].dl_el, 1);
			m_dlist->Set_xf(c->seg[iseg].dl_sel, x);
			m_dlist->Set_yf(c->seg[iseg].dl_sel, y);
			if( c->seg[iseg].dl_sel )
				c->seg[iseg].dl_sel->id.ii = iseg;
			m_dlist->Set_visible(c->seg[iseg].dl_sel, 1);
		}
		// done
		c->nsegs++;
	}
	else
	{
		// don't insert, just modify old segment
		int x = m_dlist->Get_x(c->seg[iseg].dl_el);
		int y = m_dlist->Get_y(c->seg[iseg].dl_el);
		int xf = m_dlist->Get_xf(c->seg[iseg].dl_el);
		int yf = m_dlist->Get_yf(c->seg[iseg].dl_el);
		id id  = c->seg[iseg].dl_el->id;
		m_dlist->Remove( c->seg[iseg].dl_el );
		c->seg[iseg].dl_el = m_dlist->Add( id, net, layer, DL_LINE, 
			1, width, 0, x, y, xf, yf, 0, 0 );
		m_dlist->Set_w(c->seg[iseg].dl_sel, width); 
		m_dlist->Set_visible(c->seg[iseg].dl_sel, 1); 
		m_dlist->Set_layer(c->seg[iseg].dl_sel, layer); 
		c->seg[iseg].selected = 0;
		c->seg[iseg].layer = layer;
		c->seg[iseg].width = width;
	}
	
	// clean up vias
	ReconcileVia( net, ic, iseg );
	ReconcileVia( net, ic, iseg+1 );
	if( (iseg+1) < c->nsegs )
		ReconcileVia( net, ic, iseg+2 );
	return insert_flag;
}

// Set trace width for routed segment (ignores unrouted segs)
// If w = 0, ignore it
// If via_w = 0, ignore via_w and via_hole_w
//
int CNetList::SetSegmentWidth( cnet * net, int ic, int is, int w, int via_w, int via_hole_w )
{
//	id id;
	cconnect * c = &net->connect[ic];
	if( c->seg[is].layer != LAY_RAT_LINE && w )
	{
		c->seg[is].width = w;
		m_dlist->Set_w( c->seg[is].dl_el, w );
		m_dlist->Set_w( c->seg[is].dl_sel, w );
	}
	if( c->vtx[is].via_w && via_w )
	{
		c->vtx[is].via_w = via_w;
		c->vtx[is].via_hole_w = via_hole_w;
		DrawVia( net, ic, is );
	}
	if( c->vtx[is+1].via_w && via_w )
	{
		c->vtx[is+1].via_w = via_w;
		c->vtx[is+1].via_hole_w = via_hole_w;
		DrawVia( net, ic, is+1 );
	}
	return 0;
}

int CNetList::SetConnectionWidth( cnet * net, int ic, int w, int via_w, int via_hole_w )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->nsegs; is++ )
	{
		SetSegmentWidth( net, ic, is, w, via_w, via_hole_w );
	}
	return 0;
}

int CNetList::SetNetWidth( cnet * net, int w, int via_w, int via_hole_w )
{
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		for( int is=0; is<c->nsegs; is++ )
		{
			SetSegmentWidth( net, ic, is, w, via_w, via_hole_w );
		}
	}
	return 0;
}

// part added, hook up to nets
//
void CNetList::PartAdded( cpart * part )
{
	CString ref_des = part->ref_des;

	// iterate through all nets, hooking up to part
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->npins; ip++ )
		{
			if( net->pin[ip].ref_des == ref_des )
			{
				// found net->pin which attaches to part
				net->pin[ip].part = part;	// set net->pin->part
				if( part->shape )
				{
					int pin_index = part->shape->GetPinIndexByName( &net->pin[ip].pin_name );
					if( pin_index != -1 )
					{
						// hook it up
						part->pin[pin_index].net = net;		// set part->pin->net
					}
				}
			}
		}
	}
}

// Swap 2 pins
//
void CNetList::SwapPins( cpart * part1, CString * pin_name1,
						cpart * part2, CString * pin_name2 )
{
	// get pin1 info
	int pin_index1 = part1->shape->GetPinIndexByName( pin_name1 );
	CPoint pin_pt1 = m_plist->GetPinPoint( part1, pin_name1 );
	int pin_lay1 = m_plist->GetPinLayer( part1, pin_name1 );
	cnet * net1 = m_plist->GetPinNet( part1, pin_name1 );
	int net1_pin_index = -1;
	if( net1 )
	{
		for( int ip=0; ip<net1->npins; ip++ )
		{
			if( net1->pin[ip].part == part1 && net1->pin[ip].pin_name == *pin_name1 )
			{
				net1_pin_index = ip;
				break;
			}
		}
		if( net1_pin_index ==  -1 )
			ASSERT(0);
	}

	// get pin2 info
	int pin_index2 = part2->shape->GetPinIndexByName( pin_name2 );
	CPoint pin_pt2 = m_plist->GetPinPoint( part2, pin_name2 );
	int pin_lay2 = m_plist->GetPinLayer( part2, pin_name2 );
	cnet * net2 = m_plist->GetPinNet( part2, pin_name2 );
	int net2_pin_index = -1;
	if( net2 )
	{
		// find net2_pin_index for part2->pin2
		for( int ip=0; ip<net2->npins; ip++ )
		{
			if( net2->pin[ip].part == part2 && net2->pin[ip].pin_name == *pin_name2 )
			{
				net2_pin_index = ip;
				break;
			}
		}
		if( net2_pin_index ==  -1 )
			ASSERT(0);
	}

	if( net1 == NULL && net2 == NULL )
	{
		// both pins unconnected, there is nothing to do
		return;
	}
	else if( net1 == net2 )
	{
		// both pins on same net
		for( int ic=0; ic<net1->nconnects; ic++ )
		{
			cconnect * c = &net1->connect[ic];
			int p1 = c->start_pin;
			int p2 = c->end_pin;
			int nsegs = c->nsegs;
			if( nsegs )
			{
				if( p1 == net1_pin_index && p2 == net2_pin_index )
					continue;
				else if( p1 == net2_pin_index && p2 == net1_pin_index )
					continue;
				if( p1 == net1_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net1->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, 0 );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].vtx[0].x = pin_pt2.x;
					net1->connect[ic].vtx[0].y = pin_pt2.y;
					net1->connect[ic].vtx[0].pad_layer = pin_lay2;
					// now draw
					m_dlist->Set_x( net1->connect[ic].seg[0].dl_el, pin_pt2.x );
					m_dlist->Set_y( net1->connect[ic].seg[0].dl_el, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[0].dl_el, net1->visible );
					m_dlist->Set_x( net1->connect[ic].seg[0].dl_sel, pin_pt2.x );
					m_dlist->Set_y( net1->connect[ic].seg[0].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[0].dl_sel, net1->visible );
				}
				if( p2 == net1_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net1->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, nsegs-1 );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].vtx[nsegs].x = pin_pt2.x;
					net1->connect[ic].vtx[nsegs].y = pin_pt2.y;
					net1->connect[ic].vtx[nsegs].pad_layer = pin_lay2;
					m_dlist->Set_xf( net1->connect[ic].seg[nsegs-1].dl_el, pin_pt2.x );
					m_dlist->Set_yf( net1->connect[ic].seg[nsegs-1].dl_el, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[nsegs-1].dl_el, net1->visible );
					m_dlist->Set_xf( net1->connect[ic].seg[nsegs-1].dl_sel, pin_pt2.x );
					m_dlist->Set_yf( net1->connect[ic].seg[nsegs-1].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[nsegs-1].dl_sel, net1->visible );
				}
				if( p1 == net2_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net2->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, 0 );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].vtx[0].x = pin_pt1.x;
					net2->connect[ic].vtx[0].y = pin_pt1.y;
					net2->connect[ic].vtx[0].pad_layer = pin_lay1;
					// now draw
					m_dlist->Set_x( net2->connect[ic].seg[0].dl_el, pin_pt1.x );
					m_dlist->Set_y( net2->connect[ic].seg[0].dl_el, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[0].dl_el, net2->visible );
					m_dlist->Set_x( net2->connect[ic].seg[0].dl_sel, pin_pt1.x );
					m_dlist->Set_y( net2->connect[ic].seg[0].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[0].dl_sel, net2->visible );
				}
				if( p2 == net2_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net2->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, nsegs-1 );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].vtx[nsegs].x = pin_pt1.x;
					net2->connect[ic].vtx[nsegs].y = pin_pt1.y;
					net2->connect[ic].vtx[nsegs].pad_layer = pin_lay1;
					m_dlist->Set_xf( net2->connect[ic].seg[nsegs-1].dl_el, pin_pt1.x );
					m_dlist->Set_yf( net2->connect[ic].seg[nsegs-1].dl_el, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[nsegs-1].dl_el, net2->visible );
					m_dlist->Set_xf( net2->connect[ic].seg[nsegs-1].dl_sel, pin_pt1.x );
					m_dlist->Set_yf( net2->connect[ic].seg[nsegs-1].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[nsegs-1].dl_sel, net2->visible );
				}
			}
		}
		// reassign pin1
		net2->pin[net2_pin_index].pin_name = *pin_name1;
		// reassign pin2
		net1->pin[net1_pin_index].pin_name = *pin_name2;
		SetAreaConnections( net1 );
		return;
	}

	// now move all part1->pin1 connections to part2->pin2
	// change part2->pin2->net to net1
	// change net1->pin->part to part2
	// change net1->pin->ref_des to part2->ref_des
	// change net1->pin->pin_name to pin_name2
	if( net1 )
	{
		// remove any stub traces with one segment
		for( int ic=0; ic<net1->nconnects; ic++ )
		{
			int nsegs = net1->connect[ic].nsegs;
			if( nsegs == 1)
			{
				int p1 = net1->connect[ic].start_pin;
				int p2 = net1->connect[ic].end_pin;
				if( p1 == net1_pin_index && p2 == cconnect::NO_END )
				{
					// stub trace with 1 segment, remove it
					RemoveNetConnect( net1, ic, FALSE );
					ic--;
					continue;	// next connection
				}
			}
		}
		// now check all connections
		for( int ic=0; ic<net1->nconnects; ic++ )
		{
			int nsegs = net1->connect[ic].nsegs;
			if( nsegs )
			{
				int p1 = net1->connect[ic].start_pin;
				int p2 = net1->connect[ic].end_pin;
				if( p1 == net1_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net1->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, 0 );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].vtx[0].x = pin_pt2.x;
					net1->connect[ic].vtx[0].y = pin_pt2.y;
					net1->connect[ic].vtx[0].pad_layer = pin_lay2;
					// now draw
					m_dlist->Set_x( net1->connect[ic].seg[0].dl_el, pin_pt2.x );
					m_dlist->Set_y( net1->connect[ic].seg[0].dl_el, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[0].dl_el, net1->visible );
					m_dlist->Set_x( net1->connect[ic].seg[0].dl_sel, pin_pt2.x );
					m_dlist->Set_y( net1->connect[ic].seg[0].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[0].dl_sel, net1->visible );
				}
				if( p2 == net1_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net1->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net1, ic, nsegs-1 );
						nsegs = net1->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net1->connect[ic].vtx[nsegs].x = pin_pt2.x;
					net1->connect[ic].vtx[nsegs].y = pin_pt2.y;
					net1->connect[ic].vtx[nsegs].pad_layer = pin_lay2;
					m_dlist->Set_xf( net1->connect[ic].seg[nsegs-1].dl_el, pin_pt2.x );
					m_dlist->Set_yf( net1->connect[ic].seg[nsegs-1].dl_el, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[nsegs-1].dl_el, net1->visible );
					m_dlist->Set_xf( net1->connect[ic].seg[nsegs-1].dl_sel, pin_pt2.x );
					m_dlist->Set_yf( net1->connect[ic].seg[nsegs-1].dl_sel, pin_pt2.y );
					m_dlist->Set_visible( net1->connect[ic].seg[nsegs-1].dl_sel, net1->visible );
				}
			}
		}
		// reassign pin2 to net1
		net1->pin[net1_pin_index].part = part2;
		net1->pin[net1_pin_index].ref_des = part2->ref_des;
		net1->pin[net1_pin_index].pin_name = *pin_name2;
		part2->pin[pin_index2].net = net1;
	}
	else
	{
		// pin2 is unconnected
		part2->pin[pin_index2].net = NULL;
	}
	// now move all part2->pin2 connections to part1->pin1
	// change part1->pin1->net to net2
	// change net2->pin->part to part1
	// change net2->pin->ref_des to part1->ref_des
	// change net2->pin->pin_name to pin_name1
	if( net2 )
	{
		// second pin is connected
		// remove any stub traces with one segment
		for( int ic=0; ic<net2->nconnects; ic++ )
		{
			int nsegs = net2->connect[ic].nsegs;
			if( nsegs == 1 )
			{
				int p1 = net2->connect[ic].start_pin;
				int p2 = net2->connect[ic].end_pin;
				if( p1 == net2_pin_index && p2 == cconnect::NO_END )
				{
					// stub trace with 1 segment, remove it
					RemoveNetConnect( net2, ic, FALSE );
					ic--;
					continue;
				}
			}
		}
		// now check all connections
		for( int ic=0; ic<net2->nconnects; ic++ )
		{
			int nsegs = net2->connect[ic].nsegs;
			if( nsegs )
			{
				int p1 = net2->connect[ic].start_pin;
				int p2 = net2->connect[ic].end_pin;
				if( p1 == net2_pin_index )
				{
					// starting pin is on part, unroute first segment
					if( net2->connect[ic].seg[0].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, 0 );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].vtx[0].x = pin_pt1.x;
					net2->connect[ic].vtx[0].y = pin_pt1.y;
					net2->connect[ic].vtx[0].pad_layer = pin_lay1;
					// now draw
					m_dlist->Set_x( net2->connect[ic].seg[0].dl_el, pin_pt1.x );
					m_dlist->Set_y( net2->connect[ic].seg[0].dl_el, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[0].dl_el, net2->visible );
					m_dlist->Set_x( net2->connect[ic].seg[0].dl_sel, pin_pt1.x );
					m_dlist->Set_y( net2->connect[ic].seg[0].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[0].dl_sel, net2->visible );
				}
				if( p2 == net2_pin_index )
				{
					// ending pin is on part, unroute last segment
					if( net2->connect[ic].seg[nsegs-1].layer != LAY_RAT_LINE )
					{
						UnrouteSegment( net2, ic, nsegs-1 );
						nsegs = net2->connect[ic].nsegs;
					}
					// modify vertex position and layer
					net2->connect[ic].vtx[nsegs].x = pin_pt1.x;
					net2->connect[ic].vtx[nsegs].y = pin_pt1.y;
					net2->connect[ic].vtx[nsegs].pad_layer = pin_lay1;
					m_dlist->Set_xf( net2->connect[ic].seg[nsegs-1].dl_el, pin_pt1.x );
					m_dlist->Set_yf( net2->connect[ic].seg[nsegs-1].dl_el, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[nsegs-1].dl_el, net2->visible );
					m_dlist->Set_xf( net2->connect[ic].seg[nsegs-1].dl_sel, pin_pt1.x );
					m_dlist->Set_yf( net2->connect[ic].seg[nsegs-1].dl_sel, pin_pt1.y );
					m_dlist->Set_visible( net2->connect[ic].seg[nsegs-1].dl_sel, net2->visible );
				}
			}
		}
		// reassign pin1 to net2
		net2->pin[net2_pin_index].part = part1;
		net2->pin[net2_pin_index].ref_des = part1->ref_des;
		net2->pin[net2_pin_index].pin_name = *pin_name1;
		part1->pin[pin_index1].net = net2;
	}
	else
	{
		// pin2 is unconnected
		part1->pin[pin_index1].net = NULL;
	}
	SetAreaConnections( net1 );
	SetAreaConnections( net2 );
}


// Part moved, so unroute starting and ending segments of connections
// to this part, and update positions of endpoints
// 
int CNetList::PartMoved( cpart * part )
{
	// first, mark all nets unchecked
	cnet * net;
	net = GetFirstNet();
	while( net )
	{
		net->utility = 0;
		net = GetNextNet();
	}

	// find nets that connect to this part
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		net = (cnet*)part->pin[ip].net;
		if( net )
		{
			if( net->utility == 0 )
			{
				net->utility = 1;
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					cconnect * c = &net->connect[ic];
					int nsegs = c->nsegs;
					if( nsegs )
					{
						// check this connection
						int p1 = c->start_pin;
						CString pin_name1 = net->pin[p1].pin_name;
						int pin_index1 = part->shape->GetPinIndexByName( &pin_name1 );
						int p2 = c->end_pin;
						cseg * s0 = &c->seg[0];
						cvertex * v0 = &c->vtx[0];
						if( p2 == cconnect::NO_END )
						{
							// stub trace 
							if( net->pin[p1].part == part && nsegs == 1 )
							{
								// that starts on this part and has one segment
								// modify starting vertex position and layer
								cvertex * v = &c->vtx[0];
								v->x = part->pin[pin_index1].x;
								v->y = part->pin[pin_index1].y;

								if( part->shape->m_padstack[pin_index1].hole_size )
									// through-hole pad
									v->pad_layer = LAY_PAD_THRU;
								else if( part->side == 0 && part->shape->m_padstack[pin_index1].top.shape != PAD_NONE
									|| part->side == 1 && part->shape->m_padstack[pin_index1].bottom.shape != PAD_NONE )
									// SMT pad on top
									v->pad_layer = LAY_TOP_COPPER;
								else
									// SMT pad on bottom
									v->pad_layer = LAY_BOTTOM_COPPER;
								// test for legal pad layer
								if( v->pad_layer != LAY_PAD_THRU
									&& v->pad_layer != c->seg[0].layer )
								{
									// no, remove stub trace
									RemoveNetConnect( net, ic, FALSE );
									ic--;
								}
								else
								{
									// yes, modify trace segmentnet->connect[ic].seg[0]
									m_dlist->Set_x( s0->dl_el, v->x );
									m_dlist->Set_y( s0->dl_el, v->y );
									m_dlist->Set_visible( s0->dl_el, net->visible );
									m_dlist->Set_x( s0->dl_sel, v->x );
									m_dlist->Set_y( s0->dl_sel, v->y );
									m_dlist->Set_visible( s0->dl_sel, net->visible );
								}
								continue;
							}
						}
						if( p2 != cconnect::NO_END )
						{
							// normal trace
							// test for connection that starts and ends on part
							CString pin_name2 = net->pin[p2].pin_name;
							int pin_index2 = part->shape->GetPinIndexByName( &pin_name2 );
							if( net->pin[p1].part == part && net->pin[p2].part == part )
							{
								// yes, unroute connection
								UnrouteNetConnect( net, ic );
								continue;
							}
						}
						if( net->pin[p1].part == part )
						{
							// start pin is on part, unroute first segment
							if( c->seg[0].layer != LAY_RAT_LINE )
							{
								UnrouteSegment( net, ic, 0 );
								nsegs = c->nsegs;
							}
							// modify vertex[0] position and layer
							v0->x = part->pin[pin_index1].x;
							v0->y = part->pin[pin_index1].y;
							if( part->shape->m_padstack[pin_index1].hole_size )
								// through-hole pad
								v0->pad_layer = LAY_PAD_THRU;
							else if( part->side == 0 && part->shape->m_padstack[pin_index1].top.shape != PAD_NONE
								|| part->side == 1 && part->shape->m_padstack[pin_index1].bottom.shape != PAD_NONE )
								// SMT pad on top
								v0->pad_layer = LAY_TOP_COPPER;
							else
								// SMT pad on bottom
								v0->pad_layer = LAY_BOTTOM_COPPER;
							// now draw
							m_dlist->Set_x( s0->dl_el, v0->x );
							m_dlist->Set_y( s0->dl_el, v0->y );
							m_dlist->Set_visible( s0->dl_el, net->visible );
							m_dlist->Set_x( s0->dl_sel, v0->x );
							m_dlist->Set_y( s0->dl_sel, v0->y );
							m_dlist->Set_visible( s0->dl_sel, net->visible );
							part->pin[pin_index1].net = net;
						}
						if( p2 != cconnect::NO_END )
						{
							if( net->pin[p2].part == part )
							{
								// end pin is on part, unroute last segment
								if( c->seg[nsegs-1].layer != LAY_RAT_LINE )
								{
									UnrouteSegment( net, ic, nsegs-1 );
									nsegs = c->nsegs;
								}
								// modify vertex position and layer
								CString pin_name2 = net->pin[p2].pin_name;
								int pin_index2 = part->shape->GetPinIndexByName( &pin_name2 );
								c->vtx[nsegs].x = part->pin[pin_index2].x;
								c->vtx[nsegs].y = part->pin[pin_index2].y;
								if( part->shape->m_padstack[pin_index2].hole_size )
									// through-hole pad
									c->vtx[nsegs].pad_layer = LAY_PAD_THRU;
								else if( part->side == 0 && part->shape->m_padstack[pin_index2].top.shape != PAD_NONE 
									|| part->side == 0 && part->shape->m_padstack[pin_index2].top.shape != PAD_NONE )
									// SMT pad, part on top
									c->vtx[nsegs].pad_layer = LAY_TOP_COPPER;
								else
									// SMT pad, part on bottom
									c->vtx[nsegs].pad_layer = LAY_BOTTOM_COPPER;
								m_dlist->Set_xf( c->seg[nsegs-1].dl_el, c->vtx[nsegs].x );
								m_dlist->Set_yf( c->seg[nsegs-1].dl_el, c->vtx[nsegs].y );
								m_dlist->Set_visible( c->seg[nsegs-1].dl_el, net->visible );
								m_dlist->Set_xf( c->seg[nsegs-1].dl_sel, c->vtx[nsegs].x );
								m_dlist->Set_yf( c->seg[nsegs-1].dl_sel, c->vtx[nsegs].y );
								m_dlist->Set_visible( c->seg[nsegs-1].dl_sel, net->visible );
								part->pin[pin_index2].net = net;
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

// Part footprint changed, check new pins and positions
// If changed, unroute starting and ending segments of connections
// to this part, and update positions of endpoints
// 
int CNetList::PartFootprintChanged( cpart * part )
{
	POSITION pos;
	CString name;
	void * ptr;

	// first, clear existing net assignments to part pins
	for( int ip=0; ip<part->pin.GetSize(); ip++ )
		part->pin[ip].net = NULL;

	// find nets which connect to this part
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		// check each connection in net
		for( int ic=net->nconnects-1; ic>=0; ic-- )
		{
			cconnect * c = &net->connect[ic];
			int nsegs = c->nsegs;
			if( nsegs )
			{
				int p1 = c->start_pin;
				int p2 = c->end_pin;
				if( net->pin[p1].part != part )
				{
					// connection doesn't start on this part
					if( p2 == cconnect::NO_END )
						continue; // stub trace, ignore it
					if( net->pin[p2].part != part )
						continue;	// doesn't end on part, ignore it
				}
				CString pin_name1 = net->pin[p1].pin_name;
				if( net->pin[p1].part == part )
				{
					// starting pin is on part, see if this pin still exists
					int pin_index1 = part->shape->GetPinIndexByName( &pin_name1 );
					if( pin_index1 == -1 )
					{
						// no, remove connection
						RemoveNetConnect( net, ic, FALSE );
						continue;
					}
					// yes, rehook pin to net
					part->pin[pin_index1].net = net;
					// see if position or pad type has changed
					int old_x = c->vtx[0].x;
					int old_y = c->vtx[0].y;
					int old_layer = c->seg[0].layer;
					int new_x = part->pin[pin_index1].x;
					int new_y = part->pin[pin_index1].y;
					int new_layer;
					if( part->side == 0 )
						new_layer = LAY_TOP_COPPER;
					else
						new_layer = LAY_BOTTOM_COPPER;
					BOOL layer_ok = new_layer == old_layer || part->shape->m_padstack[pin_index1].hole_size > 0;
					// see if pin position has changed
					if( old_x != new_x || old_y != new_y || !layer_ok )
					{
						// yes, unroute if necessary and update connection
						if( old_layer != LAY_RAT_LINE )
						{
							UnrouteSegment( net, ic, 0 );
							nsegs = c->nsegs;
						}
						// modify vertex position
						c->vtx[0].x = new_x;
						c->vtx[0].y = new_y;
						m_dlist->Set_x( c->seg[0].dl_el, c->vtx[0].x );
						m_dlist->Set_y( c->seg[0].dl_el, c->vtx[0].y );
						m_dlist->Set_visible( c->seg[0].dl_el, net->visible );
						m_dlist->Set_x( c->seg[0].dl_sel, c->vtx[0].x );
						m_dlist->Set_y( c->seg[0].dl_sel, c->vtx[0].y );
						m_dlist->Set_visible( c->seg[0].dl_sel, net->visible );
					}
				}
				if( p2 == cconnect::NO_END )
					continue;
				CString pin_name2 = net->pin[p2].pin_name;
				if( net->pin[p2].part == part )
				{
					// ending pin is on part, see if this pin still exists
					int pin_index2 = part->shape->GetPinIndexByName( &pin_name2 );
					if( pin_index2 == -1 )
					{
						// no, remove connection
						RemoveNetConnect( net, ic, FALSE );
						continue;
					}
					// rehook pin to net
					part->pin[pin_index2].net = net;
					// see if position has changed
					int old_x = c->vtx[nsegs].x;
					int old_y = c->vtx[nsegs].y;
					int old_layer = c->seg[nsegs-1].layer;
					int new_x = part->pin[pin_index2].x;
					int new_y = part->pin[pin_index2].y;
					int new_layer;
					if( part->side == 0 )
						new_layer = LAY_TOP_COPPER;
					else
						new_layer = LAY_BOTTOM_COPPER;
					BOOL layer_ok = (new_layer == old_layer) || (part->shape->m_padstack[pin_index2].hole_size > 0);
					if( old_x != new_x || old_y != new_y || !layer_ok )
					{
						// yes, unroute if necessary and update connection
						if( c->seg[nsegs-1].layer != LAY_RAT_LINE )
						{
							UnrouteSegment( net, ic, nsegs-1 );
							nsegs = c->nsegs;
						}
						// modify vertex position
						c->vtx[nsegs].x = new_x;
						c->vtx[nsegs].y = new_y;
						m_dlist->Set_xf( c->seg[nsegs-1].dl_el, c->vtx[nsegs].x );
						m_dlist->Set_yf( c->seg[nsegs-1].dl_el, c->vtx[nsegs].y );
						m_dlist->Set_visible( c->seg[nsegs-1].dl_el, net->visible );
						m_dlist->Set_xf( c->seg[nsegs-1].dl_sel, c->vtx[nsegs].x );
						m_dlist->Set_yf( c->seg[nsegs-1].dl_sel, c->vtx[nsegs].y );
						m_dlist->Set_visible( c->seg[nsegs-1].dl_sel, net->visible );
					}
				}
			}
		}
		// now see if new connections need to be added
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			if( net->pin[ip].ref_des == part->ref_des )
			{
				int pin_index = part->shape->GetPinIndexByName( &net->pin[ip].pin_name );
				if( pin_index == -1 )
				{
					// pin doesn't exist in part
				}
				else
				{
					// pin exists, see if connected to net
					if( part->pin[pin_index].net != net )
					{
						// no, make connection
						part->pin[pin_index].net = net;
						net->pin[ip].part = part;
					}
				}
			}
		}
		SetAreaConnections( net );
	}
	return 0;
}

// Part deleted, so unroute and remove all connections to this part
// and remove all references from netlist
// 
int CNetList::PartDeleted( cpart * part )
{
	// find nets which connect to this part, remove pins and adjust areas
	POSITION pos;
	CString name;
	void * ptr;

	// find nets which connect to this part
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->pin.GetSize(); )
		{
			if( net->pin[ip].ref_des == part->ref_des )
			{
				RemoveNetPin( net, &net->pin[ip].ref_des, &net->pin[ip].pin_name );
			}
			else
				ip++;
		}
		SetAreaConnections( net );
	}
	return 0;
}

// Part reference designator changed
// replace all references from netlist
// 
void CNetList::PartRefChanged( CString * old_ref_des, CString * new_ref_des )
{
	// find nets which connect to this part, adjust pin names
	POSITION pos;
	CString name;
	void * ptr;

	// find nets which connect to this part
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			if( net->pin[ip].ref_des == *old_ref_des )
				net->pin[ip].ref_des = *new_ref_des;
		}
	}
}

// Part disconnected, so unroute and remove all connections to this part
// Also remove net pointers from part->pins
// and part pointer from net->pins
// Do not remove pins from netlist, however
// 
int CNetList::PartDisconnected( cpart * part )
{
	// find nets which connect to this part, remove pins and adjust areas
	POSITION pos;
	CString name;
	void * ptr;

	// find nets which connect to this part
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			if( net->pin[ip].ref_des == part->ref_des )
			{
				DisconnectNetPin( net, &net->pin[ip].ref_des, &net->pin[ip].pin_name );
			}
		}
		SetAreaConnections( net );
	}
	return 0;
}

// utility function used by OptimizeConnections()
//
void AddPinsToGrid( char * grid, int p1, int p2, int npins )
{
#define GRID(a,b) grid[a*npins+b]
#define COPY_ROW(a,b) for(int k=0;k<npins;k++){ if( GRID(a,k) ) GRID(b,k) = 1; }

	// add p2 to row p1
	GRID(p1,p2) = 1;
	// now copy row p2 into p1
	COPY_ROW(p2,p1);
	// now copy row p1 into each row connected to p1
	for( int ip=0; ip<npins; ip++ )
		if( GRID(p1,ip) )
			COPY_ROW(p1, ip);
}

// optimize all unrouted connections
//
void CNetList::OptimizeConnections()
{
	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		OptimizeConnections( net );
	}
}

// optimize all unrouted connections for a part
//
void CNetList::OptimizeConnections( cpart * part )
{
	// find nets which connect to this part
	cnet * net;
	if( part->shape )
	{
		// mark all nets unoptimized
		for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
		{
			net = (cnet*)part->pin[ip].net;
			if( net )
				net->utility = 0;
		}
		// optimize each net and mark it optimized so it won't be repeated
		for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
		{
			net = (cnet*)part->pin[ip].net;
			if( net )
			{
				if( net->utility == 0 )
				{
					OptimizeConnections( net );
					net->utility = 1;
				}
			}
		}
	}
}

// optimize the unrouted connections for a net
//
void CNetList::OptimizeConnections( cnet * net )
{
#ifdef PROFILE
	StartTimer();	//****
#endif

	// get number of pins N and make grid[NxN] array and pair[N*2] array
	int npins = net->npins;
	char * grid = (char*)calloc( npins*npins, sizeof(char) );
	for( int ip=0; ip<npins; ip++ )
		grid[ip*npins+ip] = 1;
	CArray<int> pair;			// use collection class because size is unknown,
	pair.SetSize( 2*npins );	// although this should be plenty

	// go through net, deleting unrouted and unlocked connections
	// and recording pins of routed or locked connections
	for( int ic=0; ic<net->nconnects; /* ic++ is deferred */ )
	{
		cconnect * c = &net->connect[ic];
		int routed = 0;
		int is;
		for( is=0; is<c->nsegs; is++ )
		{
			if( c->seg[is].layer != LAY_RAT_LINE )
			{
				routed = 1;
				break;
			}
		}
		int p1, p2;
		if( routed || c->locked )
		{
			// routed or locked...don't delete connection
			// if not a stub, record pins in pair[] and grid[]
			p1 = c->start_pin;
			p2 = c->end_pin;
			if( p2 != cconnect::NO_END )
			{
				AddPinsToGrid( grid, p1, p2, npins );
			}
			// increment counter
			ic++;
		}
		else
		{
			// unrouted and unlocked, so delete connection
			// don't advance ic or n_routed
			RemoveNetConnect( net, ic, FALSE );
		}
	}

	//** TEMP
	if( net->visible == 0 )
	{
		free( grid );
		return;
	}

	// now add pins connected to copper areas
	for( int ia=0; ia<net->nareas; ia++ )
	{
		SetAreaConnections( net, ia );
		if( (net->area[ia].npins + net->area[ia].nstubs) > 1 )
		{
			int p1, p2, ic;
			if( net->area[ia].npins > 0 )
				p1 = net->area[ia].pin[0];
			else
			{
				 ic = net->area[ia].stub[0];
				cconnect * c = &net->connect[ic];
				 p1 = c->start_pin;
			}
			for( int ip=1; ip<net->area[ia].npins; ip++ )
			{
				p2 = net->area[ia].pin[ip];
				if( p2 != p1 )
				{
					AddPinsToGrid( grid, p1, p2, npins );
				}
			}
			for( int is=0; is<net->area[ia].nstubs; is++ )
			{
				ic = net->area[ia].stub[is];
				cconnect * c = &net->connect[ic];
				p2 = c->start_pin;
				if( p2 != p1 )
				{
					AddPinsToGrid( grid, p1, p2, npins );
				}
			}
		}
	}
#ifdef PROFILE
	double time1 = GetElapsedTime();
	StartTimer();
#endif

	// now optimize the unrouted and unlocked connections
	long num_loops = 0;
	int n_optimized = 0;
	int min_p1, min_p2, flag;
	double min_dist;

	// create arrays of pin params for efficiency
	CArray<BOOL>legal;
	CArray<double>x, y;
	CArray<double>d;
	x.SetSize(npins);
	y.SetSize(npins);
	d.SetSize(npins*npins);
	legal.SetSize(npins);
	CPoint p;
	for( ip=0; ip<npins; ip++ )
	{
		legal[ip] = FALSE;
		cpart * part = net->pin[ip].part;
		if( part )
			if( part->shape )
			{
				{
					CString pin_name = net->pin[ip].pin_name;
					int pin_index = part->shape->GetPinIndexByName( &pin_name );
					if( pin_index != -1 )
					{
						p = m_plist->GetPinPoint( net->pin[ip].part, &pin_name );
						x[ip] = p.x;
						y[ip] = p.y;
						legal[ip] = TRUE;
					}
				}
			}
	}
	for( int p1=0; p1<npins; p1++ )
	{
		for( int p2=0; p2<p1; p2++ )
		{
			if( legal[p1] && legal[p2] )
			{
				double dist = sqrt((x[p1]-x[p2])*(x[p1]-x[p2])+(y[p1]-y[p2])*(y[p1]-y[p2]));
				d[p1*npins+p2] = dist;
				d[p2*npins+p1] = dist;
			}
		}
	}

	//** testing
	// make array of distances for all pin pairs p1 and p2
	// where p2<p1 and index = (p1)*(p1-1)/2
	// first, get number of legal pins
	int n_legal = 0;
	for( int p1=0; p1<npins; p1++ )
		if( legal[p1] )
			n_legal++;

	int n_elements = (n_legal*(n_legal-1))/2;
	int * numbers = (int*)calloc( sizeof(int), n_elements );
	int * index = (int*)calloc( sizeof(int), n_elements );
	int i = 0;
	for( int p1=1; p1<npins; p1++ )
	{
		for( int p2=0; p2<p1; p2++ )
		{
			if( legal[p1] && legal[p2] )
			{
				index[i] = p1*npins + p2;
				double number = d[p1*npins+p2];
				if( number > INT_MAX )
					ASSERT(0);
				numbers[i] = number;
				i++;
				if( i > n_elements )
					ASSERT(0);
			}
		}
	}
	// sort
	::q_sort(numbers, index, 0, n_elements - 1);
	for( int i=0; i<n_elements; i++ )
	{
		int dd = numbers[i];
		int p1 = index[i]/npins;
		int p2 = index[i]%npins;
		if( i>0 )
		{
			if( dd < numbers[i-1] )
				ASSERT(0);
		}
	}

	// now make connections, shortest first
	for( int i=0; i<n_elements; i++ )
	{
		int p1 = index[i]/npins;
		int p2 = index[i]%npins;
		// find shortest connection between unconnected pins
		if( legal[p1] && legal[p2] && !grid[p1*npins+p2] )
		{
			// connect p1 to p2
			AddPinsToGrid( grid, p1, p2, npins );
			pair.SetAtGrow(n_optimized*2, p1);	
			pair.SetAtGrow(n_optimized*2+1, p2);		
			n_optimized++;
		}
	}
	free( numbers );
	free( index );

#if 0
	// now make connections, shortest first
	do
	{
		// find shortest connection between unconnected pins
		min_dist = INT_MAX;
		flag = 0;
		for( int p1=0; p1<npins; p1++ )
		{
			if( legal[p1] )
			{
				for( int p2=0; p2<p1; p2++ )
				{
					if( !grid[p1*npins+p2] && legal[p2] )
					{
						if( d[p1*npins+p2] < min_dist )
						{
							min_p1 = p1;
							min_p2 = p2;
							min_dist = d[p1*npins+p2];
							flag = 1;
						}
						num_loops++;
					}
				}
			}
		}
		if( flag )
		{
			// connect min_p1 to min_p2
			AddPinsToGrid( grid, min_p1, min_p2, npins );
			pair.SetAtGrow(n_optimized*2, min_p1);	
			pair.SetAtGrow(n_optimized*2+1, min_p2);		
			n_optimized++;
		}
	} while( flag );
#endif

	// add new optimized connections
	for( ic=0; ic<n_optimized; ic++ )
	{
		// make new connection with a single unrouted segment
		int p1 = pair[ic*2];
		int p2 = pair[ic*2+1];
		AddNetConnect( net, p1, p2 );
	}

	free( grid );

#ifdef PROFILE
	double time2 = GetElapsedTime();
	if( net->name == "GND" )
	{
		CString mess;
		mess.Format( "net \"%s\", %d pins\nloops = %ld\ntime1 = %f\ntime2 = %f", 
			net->name, net->npins, num_loops, time1, time2 );
		AfxMessageBox( mess );
	}
#endif
}

// reset pointers on part pins for net
//
int CNetList::RehookPartsToNet( cnet * net )
{
	for( int ip=0; ip<net->npins; ip++ )
	{
		CString ref_des = net->pin[ip].ref_des;
		CString pin_name = net->pin[ip].pin_name;
		cpart * part = m_plist->GetPart( &ref_des );
		if( part )
		{
			if( part->shape )
			{
				int pin_index = part->shape->GetPinIndexByName( &pin_name );
				if( pin_index != -1 )
					part->pin[pin_index].net = net;
			}
		}
	}
	return 0;
}

void CNetList::SetViaVisible( cnet * net, int ic, int iv, BOOL visible )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];
	for( int il=0; il<v->dl_el.GetSize(); il++ )
		if( v->dl_el[il] )
			v->dl_el[il]->visible = visible;
	if( v->dl_hole )
		v->dl_hole->visible = visible;
}

// start dragging end vertex of a stub trace to move it
//
void CNetList::StartDraggingEndVertex( CDC * pDC, cnet * net, int ic, int ivtx, int crosshair )
{
	cconnect * c = &net->connect[ic];
	m_dlist->CancelHighLight();
	c->seg[ivtx-1].dl_el->visible = 0;
	SetViaVisible( net, ic, ivtx, FALSE );
	for( int ia=0; ia<net->nareas; ia++ )
		for( int is=0; is<net->area[ia].nstubs; is++ )
			if( net->area[ia].stub[is] == ic && net->area[ia].dl_stub_thermal[is] != 0 )
				m_dlist->Set_visible( net->area[ia].dl_stub_thermal[is], 0 );
	m_dlist->StartDraggingLine( pDC,
		c->vtx[ivtx-1].x,
		c->vtx[ivtx-1].y,
		c->vtx[ivtx-1].x,
		c->vtx[ivtx-1].y,
		c->seg[ivtx-1].layer, 
		c->seg[ivtx-1].width,
		c->seg[ivtx-1].layer,
		0, 0, crosshair );
}

// cancel dragging end vertex of a stub trace
//
void CNetList::CancelDraggingEndVertex( cnet * net, int ic, int ivtx )
{
	cconnect * c = &net->connect[ic];
	m_dlist->StopDragging();
	c->seg[ivtx-1].dl_el->visible = 1;
	SetViaVisible( net, ic, ivtx, TRUE );
	for( int ia=0; ia<net->nareas; ia++ )
		for( int is=0; is<net->area[ia].nstubs; is++ )
			if( net->area[ia].stub[is] == ic && net->area[ia].dl_stub_thermal[is] != 0 )
				m_dlist->Set_visible( net->area[ia].dl_stub_thermal[is], 1 );
}

// move end vertex of a stub trace
//
void CNetList::MoveEndVertex( cnet * net, int ic, int ivtx, int x, int y )
{
	cconnect * c = &net->connect[ic];
	m_dlist->StopDragging();
	c->vtx[ivtx].x = x;
	c->vtx[ivtx].y = y;
	c->seg[ivtx-1].dl_el->xf = x/PCBU_PER_WU;
	c->seg[ivtx-1].dl_el->yf = y/PCBU_PER_WU;
	c->seg[ivtx-1].dl_el->visible = 1;
	c->seg[ivtx-1].dl_sel->xf = x/PCBU_PER_WU;
	c->seg[ivtx-1].dl_sel->yf = y/PCBU_PER_WU;
	c->vtx[ivtx].x = x;
	c->vtx[ivtx].y = y;
	c->vtx[ivtx].via_w = 0; // this forces redraw
	ReconcileVia( net, ic, ivtx );
	for( int ia=0; ia<net->nareas; ia++ )
		SetAreaConnections( net, ia );
}

// move vertex
//
void CNetList::MoveVertex( cnet * net, int ic, int ivtx, int x, int y )
{
	cconnect * c = &net->connect[ic];
	if( ivtx > c->nsegs )
		ASSERT(0);
	m_dlist->StopDragging();
	c->vtx[ivtx].x = x;
	c->vtx[ivtx].y = y;
	if( ivtx > 0 )
	{
		c->seg[ivtx-1].dl_el->xf = x/PCBU_PER_WU;
		c->seg[ivtx-1].dl_el->yf = y/PCBU_PER_WU;
		c->seg[ivtx-1].dl_el->visible = 1;
		c->seg[ivtx-1].dl_sel->xf = x/PCBU_PER_WU;
		c->seg[ivtx-1].dl_sel->yf = y/PCBU_PER_WU;
	}
	if( ivtx < c->nsegs )
	{
		c->seg[ivtx].dl_el->x = x/PCBU_PER_WU;
		c->seg[ivtx].dl_el->y = y/PCBU_PER_WU;
		c->seg[ivtx].dl_el->visible = 1;
		c->seg[ivtx].dl_sel->x = x/PCBU_PER_WU;
		c->seg[ivtx].dl_sel->y = y/PCBU_PER_WU;
	}
	ReconcileVia( net, ic, ivtx );
}

// Start dragging trace vertex
//
int CNetList::StartDraggingVertex( CDC * pDC, cnet * net, int ic, int ivtx,
								   int x, int y, int crosshair )
{
	// cancel previous selection and make segments and via invisible
	cconnect * c =&net->connect[ic];
	m_dlist->CancelHighLight();
	m_dlist->Set_visible(c->seg[ivtx-1].dl_el, 0);
	m_dlist->Set_visible(c->seg[ivtx].dl_el, 0);
	SetViaVisible( net, ic, ivtx, FALSE );

	// start dragging
	int xi = c->vtx[ivtx-1].x;
	int yi = c->vtx[ivtx-1].y;
	int xf = c->vtx[ivtx+1].x;
	int yf = c->vtx[ivtx+1].y;
	int layer1 = c->seg[ivtx-1].layer;
	int layer2 = c->seg[ivtx].layer;
	int w1 = c->seg[ivtx-1].width;
	int w2 = c->seg[ivtx].width;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1, 
								layer2, w1, w2, DSS_ARC_STRAIGHT, DSS_ARC_STRAIGHT, 
								0, 0, 0, 0, crosshair );
	return 0;
}

// Start dragging trace segment
//
int CNetList::StartDraggingSegment( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer1, int layer2, int w, 
								   int layer_no_via, int via_w, int via_hole_w, int dir,
								   int crosshair )
{
	// cancel previous selection and make segment invisible
	cconnect * c =&net->connect[ic];
	m_dlist->CancelHighLight();
	m_dlist->Set_visible(c->seg[iseg].dl_el, 0);
	// start dragging
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	int xf = c->vtx[iseg+1].x;
	int yf = c->vtx[iseg+1].y;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer1, 
								layer2, w, 1, DSS_ARC_STRAIGHT, DSS_ARC_STRAIGHT, 
								layer_no_via, via_w, via_hole_w, dir, crosshair );
	return 0;
}

// Start dragging new vertex in existing trace segment
//
int CNetList::StartDraggingSegmentNewVertex( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer, int w, int crosshair )
{
	// cancel previous selection and make segment invisible
	cconnect * c =&net->connect[ic];
	m_dlist->CancelHighLight();
	m_dlist->Set_visible(c->seg[iseg].dl_el, 0);
	// start dragging
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	int xf = c->vtx[iseg+1].x;
	int yf = c->vtx[iseg+1].y;
	m_dlist->StartDraggingLineVertex( pDC, x, y, xi, yi, xf, yf, layer, 
								layer, w, w, DSS_ARC_STRAIGHT, DSS_ARC_STRAIGHT, 
								layer, 0, 0, 0, crosshair );
	return 0;
}

// Start dragging stub trace segment, iseg is index of new segment
//
void CNetList::StartDraggingStub( CDC * pDC, cnet * net, int ic, int iseg,
								   int x, int y, int layer1, int w, 
								   int layer_no_via, int via_w, int via_hole_w,
								   int crosshair )
{
	cconnect * c = &net->connect[ic];
	m_dlist->CancelHighLight();
	SetViaVisible( net, ic, iseg, FALSE );
	for( int ia=0; ia<net->nareas; ia++ )
		for( int is=0; is<net->area[ia].nstubs; is++ )
			if( net->area[ia].stub[is] == ic && net->area[ia].dl_stub_thermal[is] != 0 )
				m_dlist->Set_visible( net->area[ia].dl_stub_thermal[is], 0 );
	// start dragging, start point is preceding vertex
	int xi = c->vtx[iseg].x;
	int yi = c->vtx[iseg].y;
	m_dlist->StartDraggingLine( pDC, x, y, xi, yi, layer1, 
								w, layer_no_via, via_w, via_hole_w, crosshair );
}

// Cancel dragging stub trace segment
//
void CNetList::CancelDraggingStub( cnet * net, int ic, int iseg )
{
	cconnect * c = &net->connect[ic];
	SetViaVisible( net, ic, iseg, TRUE );
	for( int ia=0; ia<net->nareas; ia++ )
		for( int is=0; is<net->area[ia].nstubs; is++ )
			if( net->area[ia].stub[is] == ic && net->area[ia].dl_stub_thermal[is] != 0 )
				m_dlist->Set_visible( net->area[ia].dl_stub_thermal[is], 1 );
	m_dlist->StopDragging();
	SetAreaConnections( net );
}

// Start dragging copper area corner to move it
//
int CNetList::StartDraggingAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair )
{
	net->area[iarea].poly->StartDraggingToMoveCorner( pDC, icorner, x, y, crosshair );
	return 0;
}

// Start dragging inserted copper area corner
//
int CNetList::StartDraggingInsertedAreaCorner( CDC *pDC, cnet * net, int iarea, int icorner, int x, int y, int crosshair )
{
	net->area[iarea].poly->StartDraggingToInsertCorner( pDC, icorner, x, y, crosshair );
	return 0;
}

// Cancel dragging inserted area corner
//
int CNetList::CancelDraggingInsertedAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].poly->CancelDraggingToInsertCorner( icorner );
	return 0;
}

// Cancel dragging area corner
//
int CNetList::CancelDraggingAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].poly->CancelDraggingToMoveCorner( icorner );
	return 0;
}

// Cancel dragging segment
//
int CNetList::CancelDraggingSegment( cnet * net, int ic, int iseg )
{
	// make segment visible
	cconnect * c =&net->connect[ic];
	m_dlist->Set_visible(c->seg[iseg].dl_el, 1);
	m_dlist->StopDragging();
	return 0;
}

// Cancel dragging vertex
//
int CNetList::CancelDraggingVertex( cnet * net, int ic, int ivtx )
{
	// make segments and via visible
	cconnect * c =&net->connect[ic];
	m_dlist->Set_visible(c->seg[ivtx-1].dl_el, 1);
	m_dlist->Set_visible(c->seg[ivtx].dl_el, 1);
	SetViaVisible( net, ic, ivtx, TRUE );
	m_dlist->StopDragging();
	return 0;
}

// Cancel dragging vertex inserted into segment
//
int CNetList::CancelDraggingSegmentNewVertex( cnet * net, int ic, int iseg )
{
	// make segment visible
	cconnect * c =&net->connect[ic];
	m_dlist->Set_visible(c->seg[iseg].dl_el, 1);
	m_dlist->StopDragging();
	return 0;
}

int CNetList::GetViaConnectionStatus( cnet * net, int ic, int iv, int layer )
{
	if( iv == 0 )
		ASSERT(0);

	int status = VIA_NO_CONNECT;
	cconnect * c;

	// check for trace connection
	c = &net->connect[ic];
	if( c->seg[iv-1].layer == layer )
		status |= VIA_TRACE;
	if( iv < c->nsegs )
		if( c->seg[iv].layer == layer )
			status |= VIA_TRACE;

	// check for thermal
	if( iv == c->nsegs && c->end_pin == cconnect::NO_END )
	{
		// this is the last via of a stub trace, 
		// see if it connects to any area in this net on this layer
		for( int ia=0; ia<net->nareas; ia++ )
		{
			// next area
			carea * a = &net->area[ia];
			if( a->poly->GetLayer() == layer )
			{
				// area is on this layer, loop through stub connections to area
				for( int istub=0; istub<a->nstubs; istub++ )
				{
					if( a->stub[istub] == ic )
					{
						// stub trace connects to area
						status |= VIA_THERMAL;
					}
				}
			}
		}
	}
	return status;
}

// add empty copper area to net
// return index to area (zero-based)
//
int CNetList::AddArea( cnet * net, int layer, int x, int y, int hatch )
{
	net->area.SetSize( net->nareas+1 );
	net->area[net->nareas].Initialize( m_dlist );
	id area_id( ID_NET, ID_AREA, net->nareas );
	net->area[net->nareas].poly->Start( layer, 1, 10*NM_PER_MIL, x, y, 
		hatch, &area_id, net );
	net->nareas++;
	return net->nareas-1;
}

// add empty copper area to net, inserting at net->area[iarea]
//
void CNetList::InsertArea( cnet * net, int iarea, int layer, int x, int y, int hatch )
{
	// make new area and insert it into area array
	carea test;
	test.Initialize( m_dlist );
	net->area.InsertAt( iarea, test ) ;
	net->area[iarea].Initialize( m_dlist );
	id area_id( ID_NET, ID_AREA, iarea );
	net->area[iarea].poly->Start( layer, 1, 10*NM_PER_MIL, x, y,
		hatch, &area_id, net );
	net->nareas++;
}

// add corner to copper area, apply style to preceding side
//
int CNetList::AppendAreaCorner( cnet * net, int iarea, int x, int y, int style, BOOL bDraw )
{
	net->area[iarea].poly->AppendCorner( x, y, style, bDraw );
	return 0;
}

// insert corner into copper area, apply style to preceding side
//
int CNetList::InsertAreaCorner( cnet * net, int iarea, int icorner, 
							int x, int y, int style )
{
	if( icorner == net->area[iarea].poly->GetNumCorners() && !net->area[iarea].poly->GetClosed() )
	{
		net->area[iarea].poly->AppendCorner( x, y, style );
		ASSERT(0);	// this is now an error, should be using AppendAreaCorner
	}
	else
	{
		net->area[iarea].poly->InsertCorner( icorner, x, y );
		net->area[iarea].poly->SetSideStyle( icorner-1, style );
	}
	return 0;
}

// move copper area corner
//
void CNetList::MoveAreaCorner( cnet * net, int iarea, int icorner, int x, int y )
{
	net->area[iarea].poly->MoveCorner( icorner, x, y );
}

// highlight
//
void CNetList::HighlightAreaCorner( cnet * net, int iarea, int icorner )
{
	net->area[iarea].poly->HighlightCorner( icorner );
}

// get copper area corner coords
//
CPoint CNetList::GetAreaCorner( cnet * net, int iarea, int icorner )
{
	CPoint pt;
	pt.x = net->area[iarea].poly->GetX( icorner );
	pt.y = net->area[iarea].poly->GetY( icorner );
	return pt;
}

// complete copper area contour by adding line to first corner
//
int CNetList::CompleteArea( cnet * net, int iarea, int style )
{
	if( net->area[iarea].poly->GetNumCorners() > 2 )
	{
		net->area[iarea].poly->Close( style );
		SetAreaConnections( net, iarea );
	}
	else
	{
		RemoveArea( net, iarea );
	}
	return 0;
}

// set connections for all areas
//
void CNetList::SetAreaConnections()
{
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		SetAreaConnections( net );
	}
}

// set connections for all areas
//
void CNetList::SetAreaConnections( cnet * net)
{
	if( net )
	{
		for( int ia=0; ia<net->nareas; ia++ )
			SetAreaConnections( net, ia );
	}
}

// set area connections for all nets on a part
// should be used when a part is added or moved
//
void CNetList::SetAreaConnections( cpart * part )
{
	// find nets which connect to this part and adjust areas
	for( int ip=0; ip<part->shape->m_padstack.GetSize(); ip++ )
	{
		cnet * net = (cnet*)part->pin[ip].net;
		if( net )
		{
			int set_area_flag = 1;
			// see if this net already encountered
			for( int ipp=0; ipp<ip; ipp++ )
				if( (cnet*)part->pin[ipp].net == net )
					set_area_flag = 0;
			// set area connections for net
			if( set_area_flag )
				SetAreaConnections( net );
		}
	}
}

// set arrays of pins and stub traces connected to area
//
void CNetList::SetAreaConnections( cnet * net, int iarea )
{
	carea * area = &net->area[iarea];
	// zero out previous arrays
	for( int ip=0; ip<area->npins; ip++ )
		m_dlist->Remove( area->dl_thermal[ip] );
	for( int is=0; is<area->nstubs; is++ )
		m_dlist->Remove( area->dl_stub_thermal[is] );
	area->npins = 0;
	area->nstubs = 0;
	area->pin.SetSize(0);
	area->dl_thermal.SetSize(0);
	area->stub.SetSize(0);
	area->dl_stub_thermal.SetSize(0);

	// test all through-hole pins in net for being inside copper area 
	id id( ID_NET, ID_AREA, iarea, ID_PIN_X );
	for( int ip=0; ip<net->npins; ip++ )
	{
		cpart * part = net->pin[ip].part;
		if( part )
		{
			if( part->shape )
			{
				CString part_pin_name = net->pin[ip].pin_name;
				int pin_index = part->shape->GetPinIndexByName( &part_pin_name );
				if( pin_index != -1 )
				{
					CPoint p = m_plist->GetPinPoint( part, &part_pin_name );
					if( area->poly->TestPointInside( p.x, p.y ) 
						&& m_plist->GetPinLayer( part, &part_pin_name ) == LAY_PAD_THRU )
					{
						// pin is inside copper area
						if( part->pin[pin_index].net != net )
							ASSERT(0);	// inconsistency between part->pin->net and net->pin->part
						area->pin.SetSize( area->npins+1 );
						area->pin[area->npins] = ip;
						id.ii = ip;
						int w = m_plist->GetPinWidth( part, &part_pin_name );
						int x = p.x - w/3;
						int y = p.y - w/3;
						int xf = p.x + w/3;
						int yf = p.y + w/3;
						dl_element * dl = m_dlist->Add( id, net, LAY_RAT_LINE, DL_X, net->visible,
							0, 0, x, y, xf, yf, 0, 0 );
						area->dl_thermal.SetAtGrow(area->npins, dl );
						area->npins++;
					}
				}
			}
		}
	}
	// test all end-points of stub traces for being inside copper area,
	// either on the same layer or ending in a via
	id.sst = ID_STUB_X;
	for( int ic=0; ic<net->nconnects; ic++ )
	{
		cconnect * c = &net->connect[ic];
		if( c->end_pin == cconnect::NO_END )
		{
			// stub trace
			int nsegs = c->nsegs;
			if( nsegs > 0 )
			{
				if( c->seg[nsegs-1].layer == area->poly->GetLayer() 
					|| c->vtx[nsegs].via_w != 0 )
				{
					// ends in via or on same layer as copper area
					int x = c->vtx[nsegs].x;
					int y = c->vtx[nsegs].y;
					if( area->poly->TestPointInside( x, y ) )
					{
						// end point of trace is inside copper area
						area->stub.SetSize( area->nstubs+1 );
						area->stub[area->nstubs] = ic;
						id.ii = ic;
						int w = c->vtx[nsegs].via_w;
						if( !w )
							w = c->seg[nsegs-1].width + 10*NM_PER_MIL;
						int xi = x - w/3;
						int yi = y - w/3;
						int xf = x + w/3;
						int yf = y + w/3;
						dl_element * dl = m_dlist->Add( id, net, LAY_RAT_LINE, DL_X, net->visible,
							0, 0, xi, yi, xf, yf, 0, 0 );
						area->dl_stub_thermal.SetAtGrow(area->nstubs, dl );
						area->nstubs++;
					}
				}
			}
		}
	}
}

// remove copper area from net
//
int CNetList::RemoveArea( cnet * net, int iarea )
{
	net->area.RemoveAt( iarea );
	net->nareas--;
	RenumberAreas( net );
	return 0;
}

// Get pointer to net with given name
//
cnet * CNetList::GetNetPtrByName( CString * name )
{
	// find element with name
	void * ptr;
	cnet * net;
	if( m_map.Lookup( *name, ptr ) )
	{
		net = (cnet*)ptr;
		return net;
	}
	return 0;
}
	
// Select copper area side
//
void CNetList::SelectAreaSide( cnet * net, int iarea, int iside )
{
	m_dlist->CancelHighLight();
	net->area[iarea].poly->HighlightSide( iside );
}

// Select copper area corner
//
void CNetList::SelectAreaCorner( cnet * net, int iarea, int icorner )
{
	m_dlist->CancelHighLight();
	net->area[iarea].poly->HighlightCorner( icorner );
}

// Set style for area side
//
void CNetList::SetAreaSideStyle( cnet * net, int iarea, int iside, int style )
{
	m_dlist->CancelHighLight();
	net->area[iarea].poly->SetSideStyle( iside, style );
	net->area[iarea].poly->HighlightSide( iside );
}


// Select all connections in net
//
void CNetList::HighlightNet( cnet * net )
{
	for( int ic=0; ic<net->nconnects; ic++ )
		HighlightConnection( net, ic );
}

// Select connection
//
void CNetList::HighlightConnection( cnet * net, int ic )
{
	cconnect * c = &net->connect[ic];
	for( int is=0; is<c->seg.GetSize(); is++ )
		HighlightSegment( net, ic, is );
}

// Select segment
//
void CNetList::HighlightSegment( cnet * net, int ic, int iseg )
{
	cconnect * c =&net->connect[ic];
	m_dlist->HighLight( DL_LINE, m_dlist->Get_x(c->seg[iseg].dl_el),
								m_dlist->Get_y(c->seg[iseg].dl_el),
								m_dlist->Get_xf(c->seg[iseg].dl_el),
								m_dlist->Get_yf(c->seg[iseg].dl_el),
								m_dlist->Get_w(c->seg[iseg].dl_el),
								m_dlist->Get_layer(c->seg[iseg].dl_el) );

}

// Select vertex
//
void CNetList::SelectVertex( cnet * net, int ic, int ivtx )
{
	// highlite square width is trace_width*2, via_width or 20 mils
	// whichever is greatest
	int w;
	cconnect * c =&net->connect[ic];
	if( ivtx > 0 && c->nsegs > ivtx )
		w = 2 * c->seg[ivtx-1].width; // w = width of following segment
	else 
		w = 0;
	if( c->nsegs > ivtx )
	{
		if ( (2*c->seg[ivtx].width) > w )
			w = 2 * c->seg[ivtx].width;		// w = width of preceding segment
	}
	if( c->vtx[ivtx].via_w > w )
		w = c->vtx[ivtx].via_w;
	if( w<(20*PCBU_PER_MIL) )
		w = 20*PCBU_PER_MIL;
	m_dlist->HighLight( DL_HOLLOW_RECT, 
		c->vtx[ivtx].x - w/2,
		c->vtx[ivtx].y - w/2, 
		c->vtx[ivtx].x + w/2,
		c->vtx[ivtx].y + w/2, 
		0 );
}

// force a via on a vertex
//
int CNetList::ForceVia( cnet * net, int ic, int ivtx, BOOL set_areas )
{
	cconnect * c = &net->connect[ic];
	c->vtx[ivtx].force_via_flag = 1;
	ReconcileVia( net, ic, ivtx );
	if( set_areas )
		SetAreaConnections( net );
	return 0;
}

// remove forced via on a vertex
//
int CNetList::UnforceVia( cnet * net, int ic, int ivtx )
{
	cconnect * c = &net->connect[ic];
	c->vtx[ivtx].force_via_flag = 0;
	ReconcileVia( net, ic, ivtx);
	SetAreaConnections( net );
	return 0;
}

// Reconcile via with preceding and following segments
// if a via is needed, use defaults for adjacent segments 
//
int CNetList::ReconcileVia( cnet * net, int ic, int ivtx )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[ivtx];

	// see if via needed
	int via_needed = 0;
	if( c->end_pin == cconnect::NO_END && ivtx == c->nsegs )
	{
		// end vertex of a stub trace, draw vertex 
		if( c->vtx[ivtx].force_via_flag )
		{
			via_needed = 1;
		}
	}
	else if( ivtx == 0 || ivtx == c->nsegs )
	{
		// first and last vertex are part pads
		return 0;
	}
	else
	{
		c->vtx[ivtx].pad_layer = 0;
		cseg * s1 = &c->seg[ivtx-1];
		cseg * s2 = &c->seg[ivtx];
		if( s1->layer != s2->layer && s1->layer != LAY_RAT_LINE && s2->layer != LAY_RAT_LINE )
		{
			via_needed = 1;
		}
	}

	if( via_needed )
	{
		// via needed, make sure it exists or create it
		if( v->via_w == 0 || v->via_hole_w == 0 )
		{
			// via doesn't already exist, set via width and hole width
			int w, via_w, via_hole_w;
			GetWidths( net, &w, &via_w, &via_hole_w );
			// set parameters for via
			v->via_w = via_w;
			v->via_hole_w = via_hole_w;
		}
	}
	else
	{
		// via not needed
		v->via_w = 0;
		v->via_hole_w = 0;
	}
	DrawVia( net, ic, ivtx );
	return 0;
}

// write nets to file
//
int CNetList::WriteNets( CStdioFile * file )
{
	CString line;
	cvertex * v;
	cseg * s;
	cnet * net;

	try
	{
		line.Format( "[nets]\n\n" );
		file->WriteString( line );

		// traverse map
		POSITION pos;
		CString name;
		void * ptr;
		for( pos = m_map.GetStartPosition(); pos != NULL; )
		{
			m_map.GetNextAssoc( pos, name, ptr ); 
			net = (cnet*)ptr;
			line.Format( "net: \"%s\" %d %d %d %d %d %d %d\n", 
							net->name, net->npins, net->nconnects, net->nareas,
							net->def_w, net->def_via_w, net->def_via_hole_w,
							net->visible );
			file->WriteString( line );
			for( int ip=0; ip<net->npins; ip++ )
			{
				line.Format( "  pin: %d %s.%s\n", ip+1, 
					net->pin[ip].ref_des, net->pin[ip].pin_name );
				file->WriteString( line );
			}
			for( int ic=0; ic<net->nconnects; ic++ ) 
			{
				cconnect * c = &net->connect[ic]; 
				line.Format( "  connect: %d %d %d %d %d\n", ic+1, 
					c->start_pin,
					c->end_pin, c->nsegs, c->locked );
				file->WriteString( line );
				int nsegs = c->nsegs;
				for( int is=0; is<=nsegs; is++ )
				{
					v = &(c->vtx[is]);
					if( is<nsegs )
					{
						line.Format( "    vtx: %d %d %d %d %d %d %d\n", 
							is+1, v->x, v->y, v->pad_layer, v->force_via_flag, v->via_w, v->via_hole_w );
						file->WriteString( line );
						s = &(c->seg[is]);
						line.Format( "    seg: %d %d %d 0 0\n", 
							is+1, s->layer, s->width );
						file->WriteString( line );
					}
					else
					{
						line.Format( "    vtx: %d %d %d %d %d %d %d\n", 
							is+1, v->x, v->y, v->pad_layer, v->force_via_flag, v->via_w, v->via_hole_w );
						file->WriteString( line );
					}
				}
			}
			for( int ia=0; ia<net->nareas; ia++ )
			{
				line.Format( "  area: %d %d %d %d\n", ia+1, 
					net->area[ia].poly->GetNumCorners(),
					net->area[ia].poly->GetLayer(),
					net->area[ia].poly->GetHatch()
					);
				file->WriteString( line );
				for( int icor=0; icor<net->area[ia].poly->GetNumCorners(); icor++ )
				{
					line.Format( "    corner: %d %d %d %d %d\n", icor+1,
						net->area[ia].poly->GetX( icor ),
						net->area[ia].poly->GetY( icor ),
						net->area[ia].poly->GetSideStyle( icor ),
						net->area[ia].poly->GetEndContour( icor )
						);
					file->WriteString( line );
				}
			}
			file->WriteString( "\n" );
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

// read netlist from file
// throws err_str on error
//
void CNetList::ReadNets( CStdioFile * pcb_file )
{
	int err, pos, np;
	CArray<CString> p;
	CString in_str, key_str;

	// find beginning of [nets] section
	do
	{
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			// error reading pcb file
			CString mess;
			mess.Format( "Unable to find [nets] section in file" );
			AfxMessageBox( mess );
			return;
		}
		in_str.Trim();
	}
	while( in_str != "[nets]" );

	// get each net in [nets] section
	while( 1 )
	{
		pos = (long)pcb_file->GetPosition();
		err = pcb_file->ReadString( in_str );
		if( !err )
		{
			CString * err_str = new CString( "unexpected EOF in project file" );
			throw err_str;
		}
		in_str.Trim();
		if( in_str[0] == '[' && in_str != "[nets]" )
		{
			pcb_file->Seek( pos, CFile::begin );
			return;		// start of next section, reset position and exit
		}
		else if( in_str.Left(4) == "net:" )
		{
			np = ParseKeyString( &in_str, &key_str, &p );
			CString net_name = p[0].Left(MAX_NET_NAME_SIZE);
			net_name.Trim();
			int npins = my_atoi( &p[1] ); 
			int nconnects = my_atoi( &p[2] );
			int nareas = my_atoi( &p[3] );
			int def_width = my_atoi( &p[4] );
			int def_via_w = my_atoi( &p[5] );
			int def_via_hole_w = my_atoi( &p[6] );
			int visible = 1;
			if( np == 9 )
				visible = my_atoi( &p[7] );
			cnet * net = AddNet( net_name, npins, def_width, def_via_w, def_via_hole_w );
			net->visible = visible;
			for( int ip=0; ip<npins; ip++ )
			{
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "pin" || np < 3 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );
					throw err_str;
				}
				CString pin_str = p[1].Left(CShape::MAX_PIN_NAME_SIZE);
				int dot_pos = pin_str.FindOneOf( "." );
				CString ref_str = pin_str.Left( dot_pos );
				CString pin_num_str = pin_str.Right( pin_str.GetLength()-dot_pos-1 );
				AddNetPin( net, &ref_str, &pin_num_str );
			}
			for( int ic=0; ic<nconnects; ic++ )
			{
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "connect" || np < 6 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );
					throw err_str;
				}
//				int nc = my_atoi( &p[0] );
//				if( (nc-1) != ic )
//				{
//					CString * err_str = new CString( "error parsing [nets] section of project file" );
//					throw err_str;
//				}
				int start_pin = my_atoi( &p[1] );
				int end_pin = my_atoi( &p[2] );
				int nsegs = my_atoi( &p[3] );
				int locked = my_atoi( &p[4] );
				int nc;
				if( end_pin != cconnect::NO_END )
					nc = AddNetConnect( net, start_pin, end_pin );
				else
					nc = AddNetStub( net, start_pin );
				if( nc == -1 )
				{
					// invalid connection, remove it with this ugly code
					ic--;
					nconnects--;
					net->connect.SetSize( ic+1 );
					for( int i=0; i<nsegs*2+1; i++ )
					{
						err = pcb_file->ReadString( in_str );
						if( !err )
						{
							CString * err_str = new CString( "unexpected EOF in project file" );
							throw err_str;
						}
					}
				}
				else
				{
					net->connect[ic].locked = locked;
					// skip first vertex
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString( "unexpected EOF in project file" );
						throw err_str;
					}
					// now add all segments
					int test_not_done = 1;
					int pre_via_w, pre_via_hole_w;
					for( int is=0; is<nsegs; is++ )
					{
						// read segment data
						err = pcb_file->ReadString( in_str );
						if( !err )
						{
							CString * err_str = new CString( "unexpected EOF in project file" );
							throw err_str;
						}
						np = ParseKeyString( &in_str, &key_str, &p );
						if( key_str != "seg" || np < 6 )
						{
							CString * err_str = new CString( "error parsing [nets] section of project file" );
							throw err_str;
						}
						int file_layer = my_atoi( &p[1] ); 
						int layer = m_layer_by_file_layer[file_layer]; 
						int seg_width = my_atoi( &p[2] ); 
//						int seg_via_w = my_atoi( &p[3] ); 
//						int seg_via_hole_w = my_atoi( &p[4] );
						// read following vertex data
						err = pcb_file->ReadString( in_str );
						if( !err )
						{
							CString * err_str = new CString( "unexpected EOF in project file" );
							throw err_str;
						}
						np = ParseKeyString( &in_str, &key_str, &p );
						if( key_str != "vtx" || np < 8 )
						{
							CString * err_str = new CString( "error parsing [nets] section of project file" );
							throw err_str;
						}
						if( test_not_done )
						{
							// only add segments if we are not done
							int x = my_atoi( &p[1] ); 
							int y = my_atoi( &p[2] ); 
							int file_layer = my_atoi( &p[3] ); 
							int pad_layer = m_layer_by_file_layer[file_layer]; 
							int force_via_flag = my_atoi( &p[4] ); 
							int via_w = my_atoi( &p[5] ); 
							int via_hole_w = my_atoi( &p[6] );
							if( end_pin != cconnect::NO_END )
							{
								test_not_done = InsertSegment( net, ic, is, x, y, layer, seg_width, 0, 0, 0 );
								// if test_not_done == 0, the vertex is on the end-pin and the trace will terminate
								// this should always be true on the last segment
								if( test_not_done && is == (nsegs-1) )
									ASSERT(0);
							}
							else
							{
								AppendSegment( net, ic, x, y, layer, seg_width, 0, 0 );
								// set widths of following vertex
								net->connect[ic].vtx[is].via_w = via_w;
								net->connect[ic].vtx[is].via_hole_w = via_hole_w;
							}
							if( force_via_flag )
								ForceVia( net, ic, is+1 );
							if( is != 0 )
							{
								// set widths of preceding vertex
								net->connect[ic].vtx[is].via_w = pre_via_w;
								net->connect[ic].vtx[is].via_hole_w = pre_via_hole_w;
								DrawVia( net, ic, is );
							}
							pre_via_w = via_w;
							pre_via_hole_w = via_hole_w;
						}
					}
				}
			}
			for( int ia=0; ia<nareas; ia++ )
			{
				err = pcb_file->ReadString( in_str );
				if( !err )
				{
					CString * err_str = new CString( "unexpected EOF in project file" );
					throw err_str;
				}
				np = ParseKeyString( &in_str, &key_str, &p );
				if( key_str != "area" || np < 4 )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );
					throw err_str;
				}
				int na = my_atoi( &p[0] );
				if( (na-1) != ia )
				{
					CString * err_str = new CString( "error parsing [nets] section of project file" );
					throw err_str;
				}
				int ncorners = my_atoi( &p[1] );
				int file_layer = my_atoi( &p[2] );
				int layer = m_layer_by_file_layer[file_layer]; 
				int hatch = 1;
				if( np == 5 )
					hatch = my_atoi( &p[3] );
				int last_side_style = CPolyLine::STRAIGHT;
				for( int icor=0; icor<ncorners; icor++ )
				{
					err = pcb_file->ReadString( in_str );
					if( !err )
					{
						CString * err_str = new CString( "unexpected EOF in project file" );
						throw err_str;
					}
					np = ParseKeyString( &in_str, &key_str, &p );
					if( key_str != "corner" || np < 4 )
					{
						CString * err_str = new CString( "error parsing [nets] section of project file" );
						throw err_str;
					} 
					int ncor = my_atoi( &p[0] );
					if( (ncor-1) != icor )
					{
						CString * err_str = new CString( "error parsing [nets] section of project file" );
						throw err_str;
					}
					int x = my_atoi( &p[1] );
					int y = my_atoi( &p[2] );
					if( icor == 0 )
						AddArea( net, layer, x, y, hatch );
					else
						AppendAreaCorner( net, ia, x, y, last_side_style, FALSE );
					if( np >= 5 )
						last_side_style = my_atoi( &p[3] );
					else
						last_side_style = CPolyLine::STRAIGHT;
					int end_cont = 0;
					if( np >= 6 )
						end_cont = my_atoi( &p[4] );
					if( icor == (ncorners-1) || end_cont )
					{
						CompleteArea( net, ia, last_side_style );
					}
				}
			}
			CleanUpConnections( net );
		}
	}
}

// undraw via
//
void CNetList::UndrawVia( cnet * net, int ic, int iv )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];
	for( int i=0; i<v->dl_el.GetSize(); i++ )
	{
		m_dlist->Remove( v->dl_el[i] );
		v->dl_el[i] = NULL;
	}
	m_dlist->Remove( v->dl_sel );
	m_dlist->Remove( v->dl_hole );
	v->dl_sel = NULL;
	v->dl_hole = NULL;
}

// draw vertex
//	i.e. draw selection box, draw via if needed
//
int CNetList::DrawVia( cnet * net, int ic, int iv )
{
	cconnect * c = &net->connect[ic];
	cvertex * v = &c->vtx[iv];

	// undraw previous via and selection box 
	UndrawVia( net, ic, iv );

	// draw via if (v->via_w) > 0
	id vid( ID_NET, ID_CONNECT, ic, ID_VERTEX, iv );
	if( v->via_w )
	{
		// draw via
		vid.sst = ID_VERTEX;
		v->dl_el.SetSize( m_layers );
		for( int il=0; il<m_layers; il++ )
		{
			int layer = LAY_TOP_COPPER + il;
			v->dl_el[il] = m_dlist->Add( vid, net, layer, DL_CIRC, 1, 
				v->via_w, 0, 
				v->x, v->y, 0, 0, 0, 0 );
		}
		v->dl_hole = m_dlist->Add( vid, net, LAY_PAD_THRU, DL_HOLE, 1, 
				v->via_hole_w, 0, 
				v->x, v->y, 0, 0, 0, 0 );
	}

	// draw selection box for vertex, using LAY_THRU_PAD if via or layer of adjacent
	// segments if no via
	vid.sst = ID_SEL_VERTEX;
	int sel_layer;
	if( v->via_w )
		sel_layer = LAY_SELECTION;
	else
		sel_layer = c->seg[iv-1].layer;
	v->dl_sel = m_dlist->AddSelector( vid, net, sel_layer, DL_HOLLOW_RECT, 
		1, 0, 0, v->x-10*PCBU_PER_WU, v->y-10*PCBU_PER_WU, v->x+10*PCBU_PER_WU, v->y+10*PCBU_PER_WU, 0, 0 );
	return 0;
}

void CNetList::SetNetVisibility( cnet * net, BOOL visible )
{
	if( net->visible == visible )
		return;
	else if( visible )
	{
		// make segments visible and enable selection items
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			for( int is=0; is<c->nsegs; is++ )
			{
				c->seg[is].dl_el->visible = TRUE;
				c->seg[is].dl_sel->visible = TRUE;
			}
		}
		// make thermals visible
		for( int ia=0; ia<net->nareas; ia++ )
		{
			for( int ip=0; ip<net->area[ia].npins; ip++ )
			{
				net->area[ia].dl_thermal[ip]->visible = TRUE;
			}
		}
	}
	else
	{
		// make ratlines invisible and disable selection items
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			cconnect * c = &net->connect[ic];
			for( int is=0; is<c->nsegs; is++ )
			{
				if( c->seg[is].layer == LAY_RAT_LINE )
				{
					c->seg[is].dl_el->visible = FALSE;
					c->seg[is].dl_sel->visible = FALSE;
				}
			}
		}
		// make thermals invisible
		for( int ia=0; ia<net->nareas; ia++ )
		{
			for( int ip=0; ip<net->area[ia].npins; ip++ )
			{
				net->area[ia].dl_thermal[ip]->visible = FALSE;
			}
		}
	}
	net->visible = visible;
}

BOOL CNetList::GetNetVisibility( cnet * net )
{
	return net->visible;
}

// export netlist data into a netlist_info structure so that it can
// be edited in a dialog
//
void CNetList::ExportNetListInfo( netlist_info * nl )
{
	// make copy of netlist data so that it can be edited
	POSITION pos;
	CString name;
	void * ptr;
	CString str;
	int i = 0;
	nl->SetSize( m_map.GetSize() );
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		(*nl)[i].name = net->name;
		(*nl)[i].net = net;
		(*nl)[i].visible = GetNetVisibility( net );
		(*nl)[i].w = net->def_w;
		(*nl)[i].v_w = net->def_via_w;
		(*nl)[i].v_h_w = net->def_via_hole_w;
		(*nl)[i].apply_widths = FALSE;
		(*nl)[i].modified = FALSE;
		(*nl)[i].deleted = FALSE;
		(*nl)[i].ref_des.SetSize(0);
		(*nl)[i].pin_name.SetSize(0);
		// now make copy of pin arrays
		(*nl)[i].ref_des.SetSize( net->npins );
		(*nl)[i].pin_name.SetSize( net->npins );
		for( int ip=0; ip<net->npins; ip++ )
		{
			(*nl)[i].ref_des[ip] = net->pin[ip].ref_des;
			(*nl)[i].pin_name[ip] = net->pin[ip].pin_name;
		}
		i++;
	}
}

// import netlist_info data back into netlist
//
void CNetList::ImportNetListInfo( netlist_info * nl, int flags, CDlgLog * log,
								 int def_w, int def_w_v, int def_w_v_h )
{
	CString mess;

	// loop through netlist_info and remove any nets that flagged for deletion
	int n_info_nets = nl->GetSize();
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( (*nl)[i].deleted && net )
		{
			// net was deleted, remove it
			if( log )
			{
				mess.Format( "  Removing net \"%s\"\r\n", net->name );
				log->AddLine( &mess );
			}
			RemoveNet( net );
			(*nl)[i].net = NULL;
		}
	}

	// now handle any nets that were renamed 
	// assumes that the new name is not a duplicate
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( net )
		{
			CString new_name = (*nl)[i].name;
			CString old_name = net->name;
			if( old_name != new_name )
			{
				m_map.RemoveKey( old_name );
				m_map.SetAt( new_name, net );
				net->name = new_name;	// rename net
			}
		}
	}

	// now check for existing nets that are not in netlist_info
	CArray<cnet*> delete_these;
	cnet * net = GetFirstNet();
	while( net )
	{
		// check if in netlist_info
		BOOL bFound = FALSE;
		for( int i=0; i<nl->GetSize(); i++ )
		{
			if( net->name == (*nl)[i].name )
			{
				bFound = TRUE;
				break;
			}
		}
		if( !bFound )
		{
			// net is not in netlist_info
			if( flags & KEEP_NETS )
			{
				if( log )
				{
					mess.Format( "  Keeping net \"%s\", not in imported netlist\r\n", net->name );
					log->AddLine( &mess );
				}
			}
			else
			{
				if( log )
				{
					mess.Format( "  Removing net \"%s\"\r\n", net->name );
					log->AddLine( &mess );
				}
				delete_these.Add( net );	// flag for deletion
			}
		}
		net = GetNextNet();
	}
	// delete them
	for( int i=0; i<delete_these.GetSize(); i++ )
	{
		RemoveNet( delete_these[i] );
	}


	// now reloop, adding and modifying nets and deleting pins as needed
	for( int i=0; i<n_info_nets; i++ )
	{
		// ignore info nets marked for deletion
		if( (*nl)[i].deleted )
			break;

		// try to find existing net with this name
		cnet * net = (*nl)[i].net;	// net from netlist_info (may be NULL)
		cnet * old_net = NULL;
		old_net = GetNetPtrByName( &(*nl)[i].name );
		if( net == NULL && old_net == NULL )
		{
			// no existing net, add to netlist
			if( (*nl)[i].w == -1 )
				(*nl)[i].w = 0;
			if( (*nl)[i].v_w == -1 )
				(*nl)[i].v_w = 0;
			if( (*nl)[i].v_h_w == -1 )
				(*nl)[i].v_h_w = 0;
			net = AddNet( (*nl)[i].name, (*nl)[i].ref_des.GetSize(), 
				(*nl)[i].w, (*nl)[i].v_w, (*nl)[i].v_h_w );
			(*nl)[i].net = net;
		}
		else if( net == NULL && old_net != NULL )
		{
			// no net from netlist_info but existing net with same name
			// use existing net and modify it
			(*nl)[i].modified = TRUE;
			net = old_net;
			(*nl)[i].net = net;
			if( (*nl)[i].w != -1 )
				net->def_w = (*nl)[i].w;
			if( (*nl)[i].v_w != -1 )
				net->def_via_w = (*nl)[i].v_w;
			if( (*nl)[i].v_h_w != -1 )
				net->def_via_hole_w = (*nl)[i].v_h_w;
		}
		else
		{
			// net from netlist_info and existing net have the same name
			if( net != old_net )
				ASSERT(0);	// make sure that they are actually the same net
			// modify existing net parameters, unless undefined
			if( (*nl)[i].w != -1 )
				net->def_w = (*nl)[i].w;
			if( (*nl)[i].v_w != -1 )
				net->def_via_w = (*nl)[i].v_w;
			if( (*nl)[i].v_h_w != -1 )
				net->def_via_hole_w = (*nl)[i].v_h_w;
		}

		// now set pin lists
		net->name = (*nl)[i].name;
		// now loop through net pins, deleting any which were removed
		for( int ipn=0; ipn<net->npins; )
		{
			CString ref_des = net->pin[ipn].ref_des;
			CString pin_name = net->pin[ipn].pin_name;
			BOOL pin_present = FALSE;
			for( int ip=0; ip<(*nl)[i].ref_des.GetSize(); ip++ )
			{
				if( ref_des == (*nl)[i].ref_des[ip]
				&& pin_name == (*nl)[i].pin_name[ip] )
				{
					// pin in net found in netlist_info
					pin_present = TRUE;
					break;
				}
			}
			if( !pin_present )
			{
				// pin in net but not in netlist_info 
				if( flags & KEEP_PARTS_AND_CON )
				{
					// we may want to preserve this pin
					cpart * part = m_plist->GetPart( &ref_des );
					if( !part )
						RemoveNetPin( net, &ref_des, &pin_name );
					else if( !part->bPreserve )
						RemoveNetPin( net, &ref_des, &pin_name );
					else
					{
						// preserve the pin
						ipn++;
					}
				}
				else
				{
					// delete it from net
					if( log )
					{
						mess.Format( "    Removing pin %s.%s from net \"%s\"\r\n", 
							ref_des, pin_name, net->name  );
						log->AddLine( &mess );
					}
					RemoveNetPin( net, &ref_des, &pin_name );
				}
			}
			else
			{
				ipn++;
			}
		}
	}

	// now reloop and add any pins that were added to netlist_info, 
	// and delete any duplicates
	// separate loop to ensure that pins were deleted from all nets
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( net && !(*nl)[i].deleted && (*nl)[i].modified )
		{
			// loop through local pins, adding any new ones to net
			int n_local_pins = (*nl)[i].ref_des.GetSize();
			for( int ipl=0; ipl<n_local_pins; ipl++ )
			{
				// delete this pin from any other nets
				cnet * test_net = GetFirstNet();
				while( test_net )
				{
					if( test_net != net )
					{
						// test for duplicate pins
						for( int test_ip=test_net->npins-1; test_ip>=0; test_ip-- )
						{
							if( test_net->pin[test_ip].ref_des == (*nl)[i].ref_des[ipl] 
							&& test_net->pin[test_ip].pin_name == (*nl)[i].pin_name[ipl] )
							{
								if( log )
								{
									mess.Format( "    Removing pin %s.%s from net \"%s\"\r\n", 
										test_net->pin[test_ip].ref_des,
										test_net->pin[test_ip].pin_name,
										test_net->name  );
									log->AddLine( &mess );
								}
								RemoveNetPin( test_net, test_ip );
							}
						}
					}
					test_net = GetNextNet();
				}
				// now test for pin already present in net
				BOOL pin_present = FALSE;
				for( int ipp=0; ipp<net->npins; ipp++ )
				{
					if( net->pin[ipp].ref_des == (*nl)[i].ref_des[ipl]
					&& net->pin[ipp].pin_name == (*nl)[i].pin_name[ipl] )
					{
						// pin in local array found in net
						pin_present = TRUE;
						break;
					}
				}
				if( !pin_present )
				{
					// pin not in net, add it
					AddNetPin( net, &(*nl)[i].ref_des[ipl], &(*nl)[i].pin_name[ipl] );
				}
			}
		}
	}
	// now set visibility and apply new widths, if requested
	for( int i=0; i<n_info_nets; i++ )
	{
		cnet * net = (*nl)[i].net;
		if( net )
		{
			SetNetVisibility( net, (*nl)[i].visible ); 
			if( (*nl)[i].apply_widths )
			{
				int w = (*nl)[i].w;
				int w_v = (*nl)[i].v_w;
				int w_v_h = (*nl)[i].v_h_w;
				if( !w )
					w = def_w;
				if( !w_v )
					w_v = def_w_v;
				if( !w_v_h )
					w_v_h = def_w_v_h;
				SetNetWidth( net, w, w_v, w_v_h ); 
			}
		}
	}
}

undo_con * CNetList::CreateConnectUndoRecord( cnet * net, int icon, BOOL set_areas )
{
	// calculate size needed, get memory
	cconnect * c = &net->connect[icon];
	int seg_offset = sizeof(undo_con);
	int vtx_offset = seg_offset + sizeof(undo_seg)*(c->nsegs);
	int size = vtx_offset + sizeof(undo_vtx)*(c->nsegs+1);
	void * ptr = malloc( size );
	undo_con * con = (undo_con*)ptr;
	undo_seg * seg = (undo_seg*)(seg_offset+(UINT)ptr);
	undo_vtx * vtx = (undo_vtx*)(vtx_offset+(UINT)ptr);
	strcpy( con->net_name, net->name );
	con->start_pin = c->start_pin;
	con->end_pin = c->end_pin;
	con->nsegs = c->nsegs;
	con->locked = c->locked;
	con->set_areas_flag = set_areas;
	con->seg_offset = seg_offset;
	con->vtx_offset = vtx_offset;
	for( int is=0; is<c->nsegs; is++ )
	{
		seg[is].layer = c->seg[is].layer;
		seg[is].width = c->seg[is].width;
	}
	for( int iv=0; iv<=con->nsegs; iv++ )
	{
		vtx[iv].x = c->vtx[iv].x;
		vtx[iv].y = c->vtx[iv].y;
		vtx[iv].pad_layer = c->vtx[iv].pad_layer;
		vtx[iv].force_via_flag = c->vtx[iv].force_via_flag;
		vtx[iv].via_w = c->vtx[iv].via_w;
		vtx[iv].via_hole_w = c->vtx[iv].via_hole_w;
	}
	con->nlist = this;
	return con;
}

// callback function for undoing connections
// note that this is declared static, since it is a callback
//
void CNetList::ConnectUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		undo_con * con = (undo_con*)ptr;
		CNetList * nl = con->nlist;
		if( type == UNDO_CONNECT_MODIFY )
		{
			// now recreate connection
			CString temp = con->net_name;
			cnet * net = nl->GetNetPtrByName( &temp ); 
			// get segment and vertex pointers
			undo_seg * seg = (undo_seg*)((UINT)ptr+con->seg_offset);
			undo_vtx * vtx = (undo_vtx*)((UINT)ptr+con->vtx_offset);
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				cconnect * c = &net->connect[ic];
				if( c->start_pin == con->start_pin
					&& c->end_pin == con->end_pin )
				{
					// if there is already a connection between these pins, remove it
					nl->RemoveNetConnect( net, ic, FALSE );
				}
			}
			// now add connection
			int nc;
			if( con->end_pin != cconnect::NO_END )
				nc = nl->AddNetConnect( net, con->start_pin, con->end_pin );
			else
				nc = nl->AddNetStub( net, con->start_pin );
			cconnect * c = &net->connect[nc];
			for( int is=0; is<con->nsegs; is++ )
			{
				if( con->end_pin != cconnect::NO_END )
					nl->InsertSegment( net, nc, is, vtx[is+1].x, vtx[is+1].y,
						seg[is].layer, seg[is].width, seg[is].via_w, seg[is].via_hole_w, 0 );
				else
					nl->AppendSegment( net, nc, vtx[is+1].x, vtx[is+1].y,
						seg[is].layer, seg[is].width, seg[is].via_w, seg[is].via_hole_w );
			}
			for( int is=0; is<con->nsegs; is++ )
			{
				c->vtx[is+1].via_w = vtx[is+1].via_w;
				c->vtx[is+1].via_hole_w = vtx[is+1].via_hole_w;
				if( vtx[is+1].force_via_flag )
					nl->ForceVia( net, nc, is+1, FALSE );
				nl->ReconcileVia( net, nc, is+1 );
			}
			// other parameters
			net->connect[nc].locked = con->locked; 
		}
		else
			ASSERT(0);
	}
	free( ptr );
}

// Create undo record for a net
// Just saves the name and pin list
// Assumes that connection undo records will be created separately
// for all connections
//
undo_net * CNetList::CreateNetUndoRecord( cnet * net )
{
	int size = sizeof(undo_net) + net->npins*sizeof(undo_pin);
	undo_net * undo = (undo_net*)malloc( size );
	strcpy( undo->name, net->name );
	undo->npins = net->npins;
	undo_pin * un_pin = (undo_pin*)((UINT)undo + sizeof(undo_net));
	for( int ip=0; ip<net->npins; ip++ )
	{
		strcpy( un_pin[ip].ref_des, net->pin[ip].ref_des );
		strcpy( un_pin[ip].pin_name, net->pin[ip].pin_name );;
	}
	undo->nlist = this;
	return undo;
}

// callback function for undoing modifications to nets
// removes all connections, and regenerates pin array
// assumes that subsequent callbacks will regenerate connections
// and copper areas
//
void CNetList::NetUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		// remove all connections from net 
		// assuming that they will be replaced by subsequent undo items
		// do not remove copper areas
		undo_net * undo = (undo_net*)ptr;
		undo_pin * un_pin = (undo_pin*)((UINT)ptr + sizeof(undo_net));
		CNetList * nl = undo->nlist;
		CString temp = undo->name;
		cnet * net = nl->GetNetPtrByName( &temp );
		if( type == UNDO_NET_ADD )
		{
			// just delete the net
			nl->RemoveNet( net );
		}
		else if( type == UNDO_NET_MODIFY )
		{
			// restore the net
			if( !net )
				ASSERT(0);
			for( int ic=(net->nconnects-1); ic>=0; ic-- )
				nl->RemoveNetConnect( net, ic, FALSE );

			// replace pin data
			net->pin.SetSize(0);
			net->npins = 0;
			for( int ip=0; ip<undo->npins; ip++ )
			{
				CString ref_str( un_pin[ip].ref_des );
				CString pin_name( un_pin[ip].pin_name );
				nl->AddNetPin( net, &ref_str, &pin_name, FALSE );
			}
			nl->RehookPartsToNet( net );
		}
		else
			ASSERT(0);
		// adjust connections to areas
//**		if( net->nareas )
//**			nl->SetAreaConnections( net );
	}
	free( ptr );
}

// create undo record for area
// only includes closed contours
//
undo_area * CNetList::CreateAreaUndoRecord( cnet * net, int iarea, int type )
{
	undo_area * un_a;
	if( type == CNetList::UNDO_AREA_CLEAR_ALL )
	{
		un_a = (undo_area*)malloc(sizeof(undo_area));
		strcpy( un_a->net_name, net->name );
		un_a->nlist = this;
		return un_a;
	}
	CPolyLine * p = net->area[iarea].poly;
	int n_cont = p->GetNumContours();
	if( !p->GetClosed() )
		n_cont--;
	int nc = p->GetContourEnd( n_cont-1 ) + 1;
	if( type == CNetList::UNDO_AREA_ADD )
		un_a = (undo_area*)malloc(sizeof(undo_area));
	else if( type == CNetList::UNDO_AREA_DELETE 
		|| type == CNetList::UNDO_AREA_MODIFY )
		un_a = (undo_area*)malloc(sizeof(undo_area)+nc*sizeof(undo_corner));
	else
		ASSERT(0);
	strcpy( un_a->net_name, net->name );
	un_a->iarea = iarea;
	un_a->ncorners = nc;
	un_a->layer = p->GetLayer();
	un_a->hatch = p->GetHatch();
	un_a->w = p->GetW();
	un_a->sel_box_w = p->GetSelBoxSize();
	if( type == CNetList::UNDO_AREA_DELETE 
		|| type == CNetList::UNDO_AREA_MODIFY )
	{
		undo_corner * un_c = (undo_corner*)((UINT)un_a + sizeof(undo_area));
		for( int ic=0; ic<nc; ic++ )
		{
			un_c[ic].x = p->GetX( ic );
			un_c[ic].y = p->GetY( ic );
			un_c[ic].end_contour = p->GetEndContour( ic );
			un_c[ic].style = p->GetSideStyle( ic );
		}
	}
	un_a->nlist = this;
	return un_a;
}

// callback function for undoing areas
// note that this is declared static, since it is a callback
//
void CNetList::AreaUndoCallback( int type, void * ptr, BOOL undo )
{
	if( undo )
	{
		undo_area * a = (undo_area*)ptr;
		CNetList * nl = a->nlist;
		CString temp = a->net_name;
		cnet * net = nl->GetNetPtrByName( &temp );
		if( !net )
			ASSERT(0);
		if( type == UNDO_AREA_CLEAR_ALL )
		{
			// delete all areas in this net
			for( int ia=net->area.GetSize()-1; ia>=0; ia-- )
				nl->RemoveArea( net, ia );
		}
		else if( type == UNDO_AREA_ADD )
		{
			// delete the area which was added
			nl->RemoveArea( net, a->iarea );
		}
		else if( type == UNDO_AREA_MODIFY 
				|| type == UNDO_AREA_DELETE )
		{
			undo_corner * c = (undo_corner*)((UINT)ptr+sizeof(undo_area));
			if( type == UNDO_AREA_MODIFY )
			{
				// remove area
				nl->RemoveArea( net, a->iarea );
			}
			// now recreate area at its original iarea in net->area[iarea]
			nl->InsertArea( net, a->iarea, a->layer, c[0].x, c[0].y, a->hatch );
			for( int ic=1; ic<a->ncorners; ic++ )
			{
				nl->AppendAreaCorner( net, a->iarea, 
					c[ic].x, c[ic].y, c[ic-1].style, FALSE ); 
				if( c[ic].end_contour )
					nl->CompleteArea( net, a->iarea, c[a->ncorners-1].style );
			}
			nl->RenumberAreas( net );
		}
		else
			ASSERT(0);
	}
	free( ptr );
}

// cross-check netlist with partlist, report results in logstr
//
int CNetList::CheckNetlist( CString * logstr )
{
	CString str;
	int nwarnings = 0;
	int nerrors = 0;
	int nfixed = 0;
	CMapStringToPtr net_map;
	CMapStringToPtr pin_map;

	*logstr += "***** Checking Nets *****\r\n";

	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		CString net_name = net->name;
		if( net_map.Lookup( net_name, ptr ) )
		{
			str.Format( "ERROR: Net \"%s\" is duplicate\r\n", net_name );
			str += "    ###   To fix this, delete one instance of the net, then save and re-open project\r\n";
			*logstr += str;
			nerrors++;
		}
		else
			net_map.SetAt( net_name, NULL );
		int npins = net->pin.GetSize();
		if( npins == 0 )
		{
			str.Format( "Warning: Net \"%s\": has no pins\r\n", net->name );
			*logstr += str;
			nwarnings++;
		}
		else if( npins == 1 )
		{
			str.Format( "Warning: Net \"%s\": has single pin\r\n", net->name );
			*logstr += str;
			nwarnings++;
		}
		for( int ip=0; ip<net->pin.GetSize(); ip++ )
		{
			// next pin in net
			CString * ref_des = &net->pin[ip].ref_des;
			CString * pin_name = &net->pin[ip].pin_name;
			CString pin_id = *ref_des + "." + *pin_name;
			void * ptr;
			BOOL test = pin_map.Lookup( pin_id, ptr );
			cnet * dup_net = (cnet*)ptr;
			if( test )
			{
				if( dup_net->name == net_name )
				{
					str.Format( "ERROR: Net \"%s\": pin \"%s\" is duplicate\r\n", 
						net->name, pin_id );
					*logstr += str;
					// reassign all connections
					//find index of first instance of pin
					int first_index = -1;
					for( int iip=0; iip<net->pin.GetSize(); iip++ )
					{
						if( net->pin[iip].ref_des == *ref_des && net->pin[iip].pin_name == *pin_name )
						{
							first_index = iip;
							break;
						}
					}
					if( first_index == -1 )
						ASSERT(0);
					// reassign connections
					for( int ic=0; ic<net->connect.GetSize(); ic++ )
					{
						cconnect * c = &net->connect[ic];
						if( c->start_pin == ip )
							c->start_pin = first_index;
						if( c->end_pin == ip )
							c->end_pin = first_index;
					}
					// remove pin
					RemoveNetPin( net, ip );
					RehookPartsToNet( net );
					str.Format( "              Fixed: Connections repaired\r\n" );
					*logstr += str;
					nerrors++;
					nfixed++;
					continue;		// no further testing on this pin
				}
				else
				{
					str.Format( "ERROR: Net \"%s\": pin \"%s\" already assigned to net \"%s\"\r\n", 
						net->name, pin_id, dup_net->name );
					str += "    ###   To fix this, delete pin from one of these nets, then save and re-open project\r\n";
					nerrors++;
					*logstr += str;
				}
			}
			else
				pin_map.SetAt( pin_id, net );

			cpart * part = net->pin[ip].part;
			if( !part )
			{
				// net->pin->part == NULL, find out why
				// see if part exists in partlist
				cpart * test_part = m_plist->GetPart( ref_des );
				if( !test_part )
				{
					// no
					str.Format( "Warning: Net \"%s\": pin \"%s.%s\" not connected, part doesn't exist\r\n", 
						net->name, *ref_des, *pin_name, net->name );
					*logstr += str;
					nwarnings++;
				}
				else
				{
					// yes, see if it has footprint
					if( !test_part->shape )
					{
						// no
						str.Format( "Warning: Net \"%s\": pin \"%s.%s\" connected, part doesn't have footprint\r\n", 
							net->name, *ref_des, *pin_name, net->name );
						*logstr += str;
						nwarnings++;
					}
					else
					{
						// yes, see if pin exists
						int pin_index = test_part->shape->GetPinIndexByName( pin_name );
						if( pin_index == -1 )
						{
							// no
							str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" not connected, but part exists although pin doesn't\r\n", 
								net->name, *ref_des, *pin_name, net->name );
							str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
							*logstr += str;
							nerrors++;
						}
						else
						{
							// yes
							str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" not connected, but part and pin exist\r\n", 
								net->name, *ref_des, *pin_name, net->name );
							str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
							*logstr += str;
							nerrors++;
						}
					}
				}
			}
			else
			{
				// net->pin->part exists, check parameters
				if( part->ref_des != *ref_des )
				{
					// net->pin->ref_des != net->pin->part->ref_des
					str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected to wrong part %s\r\n",
						net->name, *ref_des, *pin_name, part->ref_des );
					str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
					*logstr += str;
					nerrors++;
				}
				else
				{
					cpart * partlist_part = m_plist->GetPart( ref_des );
					if( !partlist_part )
					{
						// net->pin->ref_des not found in partlist
						str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected but part not in partlist\r\n",
							net->name, *ref_des, *pin_name );
						*logstr += str;
						nerrors++;
					}
					else
					{
						if( part != partlist_part )
						{
							// net->pin->ref_des found in partlist, but doesn't match net->pin->part
							str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected but net->pin->part doesn't match partlist\r\n",
								net->name, *ref_des, *pin_name );
							str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
							*logstr += str;
							nerrors++;
						}
						else
						{
							if( !part->shape )
							{
								// part matches, but no footprint
								str.Format( "Warning: Net \"%s\": pin \"%s.%s\" connected but part doesn't have footprint\r\n",
									net->name, *ref_des, *pin_name );
								*logstr += str;
								nwarnings++;
							}
							else
							{
								int pin_index = part->shape->GetPinIndexByName( pin_name );
								if( pin_index == -1 )
								{
									// net->pin->pin_name doesn't exist in part
									str.Format( "Warning: Net \"%s\": pin \"%s.%s\" connected but part doesn't have pin\r\n",
										net->name, *ref_des, *pin_name );
									*logstr += str;
									nwarnings++;
								}
								else
								{
									cnet * part_pin_net = part->pin[pin_index].net;
									if( part_pin_net != net )
									{
										// part->pin->net != net 
										str.Format( "ERROR: Net \"%s\": pin \"%s.%s\" connected but part->pin->net doesn't match\r\n",
											net->name, *ref_des, *pin_name );
										str += "    ###   To fix this, fix any other errors then save and re-open project\r\n";
										*logstr += str;
										nerrors++;
									}
									else
									{
										// OK, all is well, peace on earth
									}
								}
							}
						}
					}
				}
			}
		}
		// now check connections
		for( int ic=0; ic<net->connect.GetSize(); ic++ )
		{
			cconnect * c = &net->connect[ic];
			if( c->nsegs == 0 )
			{
				str.Format( "ERROR: Net \"%s\": connection with no segments\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
			else if( c->start_pin == c->end_pin )
			{
				str.Format( "ERROR: Net \"%s\": connection from pin to itself\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
		}
	}
	str.Format( "***** %d ERROR(S), %d FIXED, %d WARNING(S) *****\r\n",
		nerrors, nfixed, nwarnings );
	*logstr += str;
	return nerrors;
}

// cross-check netlist with partlist, report results in logstr
//
int CNetList::CheckConnectivity( CString * logstr )
{
	CString str;
	int nwarnings = 0;
	int nerrors = 0;
	int nfixed = 0;
	CMapStringToPtr net_map;
	CMapStringToPtr pin_map;

	// traverse map
	POSITION pos;
	CString name;
	void * ptr;
	for( pos = m_map.GetStartPosition(); pos != NULL; )
	{
		// next net
		m_map.GetNextAssoc( pos, name, ptr );
		cnet * net = (cnet*)ptr;
		CString net_name = net->name;
		// now check connections
		for( int ic=0; ic<net->connect.GetSize(); ic++ )
		{
			cconnect * c = &net->connect[ic];
			if( c->nsegs == 0 )
			{
				str.Format( "ERROR: Net \"%s\": connection with no segments\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			} 
			else if( c->start_pin == c->end_pin )
			{
				str.Format( "ERROR: Net \"%s\": connection from pin to itself\r\n",
					net->name );
				*logstr += str;
				RemoveNetConnect( net, ic, FALSE );
				str.Format( "              Fixed: Connection removed\r\n",
					net->name );
				*logstr += str;
				nerrors++;
				nfixed++;
			}
			else
			{
				// check for unrouted or partially routed connection
				BOOL bUnrouted = FALSE;
				for( int is=0; is<c->nsegs; is++ )
				{
					if( c->seg[is].layer == LAY_RAT_LINE )
					{
						bUnrouted = TRUE;
						break;
					}
				}
				if( bUnrouted )
				{
					CString start_pin, end_pin;
					int istart = c->start_pin;
					start_pin = net->pin[istart].ref_des + "." + net->pin[istart].pin_name;
					int iend = c->end_pin;
					if( iend == cconnect::NO_END )
					{
						str.Format( "Net \"%s\": partially routed stub trace from %s\r\n",
							net->name, start_pin );
						*logstr += str;
						nerrors++;
					}
					else
					{
						end_pin = net->pin[iend].ref_des + "." + net->pin[iend].pin_name;
						if( c->nsegs == 1 )
						{
							str.Format( "Net \"%s\": unrouted connection from %s to %s\r\n",
								net->name, start_pin, end_pin );
							*logstr += str;
							nerrors++;
						}
						else
						{
							str.Format( "Net \"%s\": partially routed connection from %s to %s\r\n",
								net->name, start_pin, end_pin );
							*logstr += str;
							nerrors++;
						}
					}
				}
			}
		}
	}
	return nerrors;
}

// Test an area for self-intersection.
// Returns:
//	-1 if arcs intersect other sides
//	 0 if no intersecting sides
//	 1 if intersecting sides, but no intersecting arcs
// Also sets utility2 flag of area with return value
//
int CNetList::TestAreaPolygon( cnet * net, int iarea )
{	
	CPolyLine * p = net->area[iarea].poly;
	// first, check for sides intersecting other sides, especially arcs 
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	int n_cont = p->GetNumContours();
	for( int icont=0; icont<n_cont; icont++ )
	{
		int is_start = p->GetContourStart(icont);
		int is_end = p->GetContourEnd(icont);
		for( int is=is_start; is<=is_end; is++ )
		{
			int is_prev = is - 1;
			if( is_prev < is_start )
				is_prev = is_end;
			int is_next = is + 1;
			if( is_next > is_end )
				is_next = is_start;
			int style = p->GetSideStyle( is );
			int x1i = p->GetX( is );
			int y1i = p->GetY( is );
			int x1f = p->GetX( is_next );
			int y1f = p->GetY( is_next );
			// check for intersection with any other sides
			for( int icont2=icont; icont2<n_cont; icont2++ )
			{
				int is2_start = p->GetContourStart(icont2);
				int is2_end = p->GetContourEnd(icont2);
				for( int is2=is2_start; is2<=is2_end; is2++ )
				{
					int is2_prev = is2 - 1;
					if( is2_prev < is2_start )
						is2_prev = is2_end;
					int is2_next = is2 + 1;
					if( is2_next > is2_end )
						is2_next = is2_start;
					if( icont != icont2 || (is2 != is && is2 != is_prev && is2 != is_next && is != is2_prev && is != is2_next ) )
					{
						int style2 = p->GetSideStyle( is2 );
						int x2i = p->GetX( is2 );
						int y2i = p->GetY( is2 );
						int x2f = p->GetX( is2_next );
						int y2f = p->GetY( is2_next );
						int ret = FindSegmentIntersections( x1i, y1i, x1f, y1f, style, x2i, y2i, x2f, y2f, style2 );
						if( ret )
						{
							// intersection between non-adjacent sides
							bInt = TRUE;
							if( style != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
							{
								bArcInt = TRUE;
								break;
							}
						}
					}
				}
				if( bArcInt )
					break;
			}
			if( bArcInt )
				break;
		}
		if( bArcInt )
			break;
	}
	if( bArcInt )
		net->area[iarea].utility2 = -1;
	else if( bInt )
		net->area[iarea].utility2 = 1;
	else 
		net->area[iarea].utility2 = 0;
	return net->area[iarea].utility2;
}

// Process an area that has been modified, by clipping its polygon against itself.
// This may change the number and order of copper areas in the net.
// If bMessageBox == TRUE, shows message box when clipping occurs.
// Returns:
//	-1 if arcs intersect other sides, so polygon can't be clipped
//	 0 if no intersecting sides
//	 1 if intersecting sides
// Also sets net->area->utility1 flags if areas are modified
//
int CNetList::ClipAreaPolygon( cnet * net, int iarea, BOOL bMessageBoxArc, BOOL bMessageBoxInt )
{	
	CPolyLine * p = net->area[iarea].poly;
	int test = TestAreaPolygon( net, iarea );	// this sets utility2 flag
	if( test == -1 )
	{
		// arc intersections, don't clip
		if( bMessageBoxArc && bDontShowSelfIntersectionArcsWarning == FALSE)
		{
			CString str;
			str.Format( "Area %d of net \"%s\" has arcs intersecting other sides.\n",
				iarea+1, net->name );
			str += "This may cause problems with other editing operations,\n";
			str += "such as adding cutouts. It can't be fixed automatically.\n";
			str += "Manual correction is recommended.\n";
			CDlgMyMessageBox dlg;
			dlg.Initialize( &str );
			dlg.DoModal();
			bDontShowSelfIntersectionArcsWarning = dlg.bDontShowBoxState;
		}
		return -1;	// arcs intersect with other sides, error
	}

	// mark all areas as unmodified except this one
	for( int ia=0; ia<net->nareas; ia++ )
		net->area[ia].utility = 0;
	net->area[iarea].utility = 1;

	if( test == 1 )
	{
		// non-arc, intersections, clip the polygon
		if( bMessageBoxInt && bDontShowSelfIntersectionWarning == FALSE)
		{
			CString str;
			str.Format( "Area %d of net \"%s\" is self-intersecting and will be clipped.\n",
				iarea+1, net->name );
			str += "This may result in splitting the area.\n";
			str += "If the area is complex, this may take a few seconds.";
//			AfxMessageBox( str );
			CDlgMyMessageBox dlg;
			dlg.Initialize( &str );
			dlg.DoModal();
			bDontShowSelfIntersectionWarning = dlg.bDontShowBoxState;
		}
		CArray<CPolyLine*> * pa = new CArray<CPolyLine*>;
		p->Undraw();
		int n_poly = net->area[iarea].poly->NormalizeWithGpc( pa, TRUE );
		if( n_poly > 1 )
		{
			for( int ip=1; ip<n_poly; ip++ )
			{
				// create new copper area and copy poly into it
				CPolyLine * new_p = (*pa)[ip-1];
				int ia = AddArea( net, 0, 0, 0, 0 );
				// remove the poly that was automatically created for the new area
				// and replace it with a poly from NormalizeWithGpc
				delete net->area[ia].poly;
				net->area[ia].poly = new_p;
				net->area[ia].poly->SetDisplayList( net->m_dlist );
				net->area[ia].poly->SetHatch( p->GetHatch() );
				net->area[ia].poly->SetLayer( p->GetLayer() );
				id p_id( ID_NET, ID_AREA, ia );
				net->area[ia].poly->SetId( &p_id );
				net->area[ia].poly->Draw();
				net->area[ia].utility = 1;
			}
		}
		p->Draw();
		delete pa;
	}
	return test;
}

// Process an area that has been modified, by clipping its polygon against
// itself and the polygons for any other areas on the same net.
// This may change the number and order of copper areas in the net.
// If bMessageBox == TRUE, shows message boxes when clipping occurs.
// Returns:
//	-1 if arcs intersect other sides, so polygon can't be clipped
//	 0 if no intersecting sides
//	 1 if intersecting sides, polygon clipped
//
int CNetList::AreaPolygonModified( cnet * net, int iarea, BOOL bMessageBoxArc, BOOL bMessageBoxInt )
{	
	// clip polygon against itself
	int test = ClipAreaPolygon( net, iarea, bMessageBoxArc, bMessageBoxInt );
	// now see if we need to clip against other areas
	BOOL bCheckAllAreas = FALSE;
	if( test == 1 )
		bCheckAllAreas = TRUE;
	else
		bCheckAllAreas = TestAreaIntersections( net, iarea );
	if( bCheckAllAreas )
		CombineAllAreasInNet( net, bMessageBoxInt, TRUE );
	SetAreaConnections( net );
	return test;
}

// Checks all copper areas in net for intersections, combining them if found
// If bUseUtility == TRUE, don't check areas if both utility flags are 0
// Sets utility flag = 1 for any areas modified
// If an area has self-intersecting arcs, doesn't try to combine it
//
int CNetList::CombineAllAreasInNet( cnet * net, BOOL bMessageBox, BOOL bUseUtility )
{
	if( net->nareas > 1 )
	{
		// start by testing all area polygons to set utility2 flags
		for( int ia=0; ia<net->nareas; ia++ )
			TestAreaPolygon( net, ia );
		// now loop through all combinations
		BOOL message_shown = FALSE;
		for( int ia1=0; ia1<net->nareas-1; ia1++ ) 
		{
			// legal polygon
			CRect b1 = net->area[ia1].poly->GetCornerBounds();
			BOOL mod_ia1 = FALSE;
			for( int ia2=net->nareas-1; ia2 > ia1; ia2-- )
			{
				if( net->area[ia1].poly->GetLayer() == net->area[ia2].poly->GetLayer() 
					&& net->area[ia1].utility2 != -1 && net->area[ia2].utility2 != -1 )
				{
					CRect b2 = net->area[ia2].poly->GetCornerBounds();
					if( !( b1.left > b2.right || b1.right < b2.left
						|| b1.bottom > b2.top || b1.top < b2.bottom ) )
					{
						// check ia2 against 1a1 
						if( net->area[ia1].utility || net->area[ia2].utility || bUseUtility == FALSE )
						{
							int ret = TestAreaIntersection( net, ia1, ia2 );
							if( ret == 1 )
								ret = CombineAreas( net, ia1, ia2 );
							if( ret == 1 )
							{
								if( bMessageBox && bDontShowIntersectionWarning == FALSE )
								{
									CString str;
									str.Format( "Areas %d and %d of net \"%s\" intersect and will be combined.\n",
										ia1+1, ia2+1, net->name );
									str += "If they are complex, this may take a few seconds.";
									CDlgMyMessageBox dlg;
									dlg.Initialize( &str );
									dlg.DoModal();
									bDontShowIntersectionWarning = dlg.bDontShowBoxState;
								}
								mod_ia1 = TRUE;
							}
							else if( ret == 2 )
							{
								if( bMessageBox && bDontShowIntersectionArcsWarning == FALSE )
								{
									CString str;
									str.Format( "Areas %d and %d of net \"%s\" intersect, but some of the intersecting sides are arcs.\n",
										ia1+1, ia2+1, net->name );
									str += "Therefore, these areas can't be combined.";
									CDlgMyMessageBox dlg;
									dlg.Initialize( &str );
									dlg.DoModal();
									bDontShowIntersectionArcsWarning = dlg.bDontShowBoxState;
								}
							}
						}
					}
				}
			}
			if( mod_ia1 )
				ia1--;		// if modified, we need to check it again
		}
	}
	return 0;
}

// Check for intersection of copper area with other areas in same net
//
BOOL CNetList::TestAreaIntersections( cnet * net, int ia )
{
	CPolyLine * poly1 = net->area[ia].poly;
	for( int ia2=0; ia2<net->nareas; ia2++ )
	{
		if( ia != ia2 )
		{
			// see if polygons are on same layer
			CPolyLine * poly2 = net->area[ia2].poly;
			if( poly1->GetLayer() != poly2->GetLayer() )
				continue;

			// test bounding rects
			CRect b1 = poly1->GetCornerBounds();
			CRect b2 = poly2->GetCornerBounds();
			if(    b1.bottom > b2.top
				|| b1.top < b2.bottom
				|| b1.left > b2.right
				|| b1.right < b2.left )
				continue;

			// test for intersecting segments
			BOOL bInt = FALSE;
			BOOL bArcInt = FALSE;
			for( int icont1=0; icont1<poly1->GetNumContours(); icont1++ )
			{
				int is1 = poly1->GetContourStart( icont1 );
				int ie1 = poly1->GetContourEnd( icont1 );
				for( int ic1=is1; ic1<=ie1; ic1++ )
				{
					int xi1 = poly1->GetX(ic1);
					int yi1 = poly1->GetY(ic1);
					int xf1, yf1, style1;
					if( ic1 < ie1 )
					{
						xf1 = poly1->GetX(ic1+1);
						yf1 = poly1->GetY(ic1+1);
					}
					else
					{
						xf1 = poly1->GetX(is1);
						yf1 = poly1->GetY(is1);
					}
					style1 = poly1->GetSideStyle( ic1 );
					for( int icont2=0; icont2<poly2->GetNumContours(); icont2++ )
					{
						int is2 = poly2->GetContourStart( icont2 );
						int ie2 = poly2->GetContourEnd( icont2 );
						for( int ic2=is2; ic2<=ie2; ic2++ )
						{
							int xi2 = poly2->GetX(ic2);
							int yi2 = poly2->GetY(ic2);
							int xf2, yf2, style2;
							if( ic2 < ie2 )
							{
								xf2 = poly2->GetX(ic2+1);
								yf2 = poly2->GetY(ic2+1);
							}
							else
							{
								xf2 = poly2->GetX(is2);
								yf2 = poly2->GetY(is2);
							}
							style2 = poly2->GetSideStyle( ic2 );
							int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
								xi2, yi2, xf2, yf2, style2 );
							if( n_int )
								return TRUE;
						}
					}
				}
			}
		}
	}
	return FALSE;
}

// Test for intersection of 2 copper areas
// ia2 must be > ia1
// returns: 0 if no intersection
//			1 if intersection
//			2 if arcs intersect
//
int CNetList::TestAreaIntersection( cnet * net, int ia1, int ia2 )
{
	// see if polygons are on same layer
	CPolyLine * poly1 = net->area[ia1].poly;
	CPolyLine * poly2 = net->area[ia2].poly;
	if( poly1->GetLayer() != poly2->GetLayer() )
		return 0;

	// test bounding rects
	CRect b1 = poly1->GetCornerBounds();
	CRect b2 = poly2->GetCornerBounds();
	if(    b1.bottom > b2.top
		|| b1.top < b2.bottom
		|| b1.left > b2.right
		|| b1.right < b2.left )
		return 0;

	// now test for intersecting segments
	BOOL bInt = FALSE;
	BOOL bArcInt = FALSE;
	for( int icont1=0; icont1<poly1->GetNumContours(); icont1++ )
	{
		int is1 = poly1->GetContourStart( icont1 );
		int ie1 = poly1->GetContourEnd( icont1 );
		for( int ic1=is1; ic1<=ie1; ic1++ )
		{
			int xi1 = poly1->GetX(ic1);
			int yi1 = poly1->GetY(ic1);
			int xf1, yf1, style1;
			if( ic1 < ie1 )
			{
				xf1 = poly1->GetX(ic1+1);
				yf1 = poly1->GetY(ic1+1);
			}
			else
			{
				xf1 = poly1->GetX(is1);
				yf1 = poly1->GetY(is1);
			}
			style1 = poly1->GetSideStyle( ic1 );
			for( int icont2=0; icont2<poly2->GetNumContours(); icont2++ )
			{
				int is2 = poly2->GetContourStart( icont2 );
				int ie2 = poly2->GetContourEnd( icont2 );
				for( int ic2=is2; ic2<=ie2; ic2++ )
				{
					int xi2 = poly2->GetX(ic2);
					int yi2 = poly2->GetY(ic2);
					int xf2, yf2, style2;
					if( ic2 < ie2 )
					{
						xf2 = poly2->GetX(ic2+1);
						yf2 = poly2->GetY(ic2+1);
					}
					else
					{
						xf2 = poly2->GetX(is2);
						yf2 = poly2->GetY(is2);
					}
					style2 = poly2->GetSideStyle( ic2 );
					int n_int = FindSegmentIntersections( xi1, yi1, xf1, yf1, style1,
									xi2, yi2, xf2, yf2, style2 );
					if( n_int )
					{
						bInt = TRUE;
						if( style1 != CPolyLine::STRAIGHT || style2 != CPolyLine::STRAIGHT )
							bArcInt = TRUE;
						break;
					}
				}
				if( bArcInt )
					break;
			}
			if( bArcInt )
				break;
		}
		if( bArcInt )
			break;
	}
	if( !bInt )
		return 0;
	if( bArcInt )
		return 2;
	return 1;
}

// If possible, combine 2 copper areas
// ia2 must be > ia1
// returns: 0 if no intersection
//			1 if intersection
//			2 if arcs intersect
//
int CNetList::CombineAreas( cnet * net, int ia1, int ia2 )
{
	if( ia2 <= ia1 )
		ASSERT(0);
#if 0
	// test for intersection
	int test = TestAreaIntersection( net, ia1, ia2 );
	if( test != 1 )
		return test;	// no intersection
#endif

	// polygons intersect, combine them
	CPolyLine * poly1 = net->area[ia1].poly;
	CPolyLine * poly2 = net->area[ia2].poly;
	CArray<CArc> arc_array1;
	CArray<CArc> arc_array2;
	poly1->MakeGpcPoly( -1, &arc_array1 );
	poly2->MakeGpcPoly( -1, &arc_array2 );
	int n_ext_cont1 = 0;
	for( int ic=0; ic<poly1->GetGpcPoly()->num_contours; ic++ )
		if( !((poly1->GetGpcPoly()->hole)[ic]) )
			n_ext_cont1++;
	int n_ext_cont2 = 0;
	for( int ic=0; ic<poly2->GetGpcPoly()->num_contours; ic++ )
		if( !((poly2->GetGpcPoly()->hole)[ic]) )
			n_ext_cont2++;

	gpc_polygon * union_gpc = new gpc_polygon;
	gpc_polygon_clip( GPC_UNION, poly1->GetGpcPoly(), poly2->GetGpcPoly(), union_gpc );

	// get number of outside contours
	int n_union_ext_cont = 0;
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
		if( !((union_gpc->hole)[ic]) )
			n_union_ext_cont++;

	// if no intersection, free new gpc and return
	if( n_union_ext_cont == n_ext_cont1 + n_ext_cont2 )
	{
		gpc_free_polygon( union_gpc );
		delete union_gpc;
		return 0;
	}

	// intersection, replace ia1 with combined areas and remove ia2
	RemoveArea( net, ia2 );
	int hatch = net->area[ia1].poly->GetHatch();
	id a_id = net->area[ia1].poly->GetId();
	int layer = net->area[ia1].poly->GetLayer();
	int w = net->area[ia1].poly->GetW();
	int sel_box = net->area[ia1].poly->GetSelBoxSize();
	RemoveArea( net, ia1 );
	// create area with external contour
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
	{
		if( !(union_gpc->hole)[ic] )
		{
			// external contour, replace this poly
			for( int i=0; i<union_gpc->contour[ic].num_vertices; i++ )
			{
				int x = ((union_gpc->contour)[ic].vertex)[i].x;
				int y = ((union_gpc->contour)[ic].vertex)[i].y;
				if( i==0 )
				{
					InsertArea( net, ia1, layer, x, y, hatch );
				}
				else
					AppendAreaCorner( net, ia1, x, y, CPolyLine::STRAIGHT, FALSE );
			}
			CompleteArea( net, ia1, CPolyLine::STRAIGHT );
			RenumberAreas( net );
		}
	}
	// add holes
	for( int ic=0; ic<union_gpc->num_contours; ic++ )
	{
		if( (union_gpc->hole)[ic] )
		{
			// hole
			for( int i=0; i<union_gpc->contour[ic].num_vertices; i++ )
			{
				int x = ((union_gpc->contour)[ic].vertex)[i].x;
				int y = ((union_gpc->contour)[ic].vertex)[i].y;
				AppendAreaCorner( net, ia1, x, y, CPolyLine::STRAIGHT, FALSE );
			}
			CompleteArea( net, ia1, CPolyLine::STRAIGHT );
		}
	}
	net->area[ia1].utility = 1;
	net->area[ia1].poly->RestoreArcs( &arc_array1 ); 
	net->area[ia1].poly->RestoreArcs( &arc_array2 );
	net->area[ia1].poly->Draw();
	gpc_free_polygon( union_gpc );
	delete union_gpc;
	return 1;
}

void CNetList::SetWidths( int w, int via_w, int via_hole_w )
{
	m_def_w = w; 
	m_def_via_w = via_w; 
	m_def_via_hole_w = via_hole_w;
}

void CNetList::GetWidths( cnet * net, int * w, int * via_w, int * via_hole_w )
{
	if( net->def_w == 0 )
		*w = m_def_w;
	else
		*w = net->def_w;

	if( net->def_via_w == 0 )
		*via_w = m_def_via_w;
	else
		*via_w = net->def_via_w;

	if( net->def_via_hole_w == 0 )
		*via_hole_w = m_def_via_hole_w;
	else
		*via_hole_w = net->def_via_hole_w;

}

// get bounding rectangle for all net elements
//
BOOL CNetList::GetNetBoundaries( CRect * r )
{
	BOOL bValid = FALSE;
	cnet * net = GetFirstNet();
	CRect br;
	br.bottom = INT_MAX;
	br.left = INT_MAX;
	br.top = INT_MIN;
	br.right = INT_MIN;
	while( net )
	{
		for( int ic=0; ic<net->nconnects; ic++ )
		{
			for( int iv=0; iv<net->connect[ic].vtx.GetSize(); iv++ )
			{
				cvertex * v = &net->connect[ic].vtx[iv];
				br.bottom = min( br.bottom, v->y - v->via_w );
				br.top = max( br.top, v->y + v->via_w );
				br.left = min( br.left, v->x - v->via_w );
				br.right = max( br.right, v->x + v->via_w );
				bValid = TRUE;
			}
		}
		for( int ia=0; ia<net->nareas; ia++ )
		{
			CRect r = net->area[ia].poly->GetBounds();
			br.bottom = min( br.bottom, r.bottom );
			br.top = max( br.top, r.top );
			br.left = min( br.left, r.left );
			br.right = max( br.right, r.right );
			bValid = TRUE;
		}
		net = GetNextNet();
	}
	*r = br;
	return bValid;
}
