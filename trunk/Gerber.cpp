// make Gerber file from partlist, netlist, textlist, etc.
//
#include "stdafx.h"
#include "Gerber.h"
#include <math.h>
#include <afxcoll.h>
#include <afxtempl.h>

#define pi  3.14159265359

#define CLEARANCE_POLY_STROKE_MILS 1

class c_cutout {
public:
	cnet * net;
	int ia;
	int ic;
};

class c_area {
public:
	cnet * net;
	int ia;
	CArray< c_cutout > containers;
};

// constructor
CAperture::CAperture()
{
	m_type = 0;
	m_size1 = 0;
	m_size2 = 0;
}

// constructor
CAperture::CAperture( int type, int size1, int size2 ) 
{
	m_type = type;
	m_size1 = size1;
	m_size2 = size2;
}

// destructor
CAperture::~CAperture()
{
}

// test for equality
//
BOOL CAperture::Equals( CAperture * ap )
{
	if( m_type == ap->m_type )
	{
		if( m_type == AP_CIRCLE && m_size1 == ap->m_size1 )
			return TRUE;
		if( m_type == AP_SQUARE && m_size1 == ap->m_size1 )
			return TRUE;
		if( m_type == AP_OCTAGON && m_size1 == ap->m_size1 )
			return TRUE;
		if( m_type == AP_THERMAL && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
		if( m_type == AP_MOIRE && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
		if( m_type == AP_OVAL && m_size1 == ap->m_size1 && m_size2 == ap->m_size2 )
			return TRUE;
	}
	return FALSE;
}

// find aperture in array 
// if not found and ok_to_add == TRUE, add to array
// return posiiton in array, or -1 if not found and not added
//
int CAperture::FindInArray( aperture_array * ap_array, BOOL ok_to_add )
{
	int na = ap_array->GetSize();
	int ifound = -1;
	for( int ia=0; ia<na; ia++ )
	{
		if( this->Equals( &(ap_array->GetAt(ia)) ) )
		{
			ifound = ia;
			break;
		}
	}
	if( ifound != -1 )
		return ifound;
	if( ok_to_add )
	{
		ap_array->Add( *this );
		return ap_array->GetSize()-1;
	}
	return -1;
}

// things that you can do with a light
enum {
	LIGHT_NONE,
	LIGHT_ON,
	LIGHT_OFF,
	LIGHT_FLASH
};

// generate a "moveto" string for the Gerber file
// enter with:
//	x, y = coords in NM
//
void WriteMoveTo( CStdioFile * f, int x, int y, int light_state )
{
	_int64 x_10 = (_int64)1000*x/NM_PER_MIL;
	_int64 y_10 = (_int64)1000*y/NM_PER_MIL;
	CString str;
	if( light_state == LIGHT_NONE )
		ASSERT(0);
	else
		str.Format( "G01X%I64dY%I64dD0%d*\n", x_10, y_10, light_state );
	f->WriteString( str );
}

// draw one side of a CPolyLine by writing commands to Gerber file
// the side may be straight or an arc
// arc is aproximated by straight-line segments
// assumes that plotter already at x1, y1
// does not turn the light on or off
// dimensions are in NM
//
void WritePolygonSide( CStdioFile * f, int x1, int y1, int x2, int y2, int style, int nsteps, int light_state )
{
	int n;	// number of steps for arcs
	n = (abs(x2-x1)+abs(y2-y1))/(5*NM_PER_MIL);	// step size approx. 3 to 5 mil
	n = max( n, 18 );	// or at most 5 degrees of arc
	CString line;
	double xo, yo, a, b, theta1, theta2;
	a = fabs( (double)(x1 - x2) );
	b = fabs( (double)(y1 - y2) );

	if( style == CPolyLine::STRAIGHT )
	{
		// just draw a straight line with linear interpolation
		WriteMoveTo( f, x2, y2, light_state );
		return;
	}
	else if( style == CPolyLine::ARC_CW )
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
	else if( style == CPolyLine::ARC_CCW )
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
	else
		ASSERT(0);
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
		WriteMoveTo( f, x, y, light_state );
	}
}

// write the Gerber file for a layer
// assumes that the file is already open for writing
//
int WriteGerberFile( CStdioFile * f, int flags, int layer,
					CDlgLog * log,
					int fill_clearance, int mask_clearance, int pilot_diameter,
					int min_silkscreen_stroke_wid, int thermal_wid,
					int outline_width, int hole_clearance,
					int annular_ring_pins, int annular_ring_vias,
					CArray<CPolyLine> * bd, CArray<CPolyLine> * sm, CPartList * pl, 
					CNetList * nl, CTextList * tl, CDisplayList * dl )
{
#define LAYER_TEXT_HEIGHT			100*NM_PER_MIL	// for layer ID sring
#define	LAYER_TEXT_STROKE_WIDTH		10*NM_PER_MIL

	aperture_array ap_array;
	int current_iap = -1;
	CAperture current_ap;
	int bd_min_x = INT_MAX;
	int bd_min_y = INT_MAX;
	int bd_max_x = INT_MIN;
	int bd_max_y = INT_MIN;
	const double cos_oct = cos( pi/8.0 ); 
	CString str;

	// get boundaries of board outline (in mils)
	for( int ib=0; ib<bd->GetSize(); ib++ ) 
	{
		for( int ic=0; ic<(*bd)[ib].GetNumCorners(); ic++ )
		{
			int x = (*bd)[ib].GetX(ic)/NM_PER_MIL;
			if( x < bd_min_x )
				bd_min_x = x;
			if( x > bd_max_x )
				bd_max_x = x;
			int y = (*bd)[ib].GetY(ic)/NM_PER_MIL;
			if( y < bd_min_y )
				bd_min_y = y;
			if( y > bd_max_y )
				bd_max_y = y;
		}
	}

	// perform two passes through data, first just get apertures, then write file
	for( int ipass=0; ipass<2; ipass++ )
	{
		#define PASS0 (ipass==0)	
		#define PASS1 (ipass==1)

		CString line;
		if( PASS1 )
		{
			// ******************** apertures created, now write them **********************
			line.Format( "G04 %s layer *\n", &layer_str[layer][0] );
			f->WriteString( line );
			f->WriteString( "G04 Scale: 100 percent, Rotated: No, Reflected: No *\n" );
			f->WriteString( "%FSLAX26Y26*%\n" );
			f->WriteString( "%MOIN*%\n" );
			f->WriteString( "%LNTop*%\n" );
			// define all of the apertures
			for( int ia=0; ia<ap_array.GetSize(); ia++ )
			{
				if( ap_array[ia].m_type == CAperture::AP_CIRCLE )
				{
					f->WriteString( "%" );
					line.Format( "ADD%dC,%.6f*", ia+10, (double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( line );
					f->WriteString( "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_SQUARE )
				{
					f->WriteString( "%" );
					line.Format( "ADD%dR,%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size1/25400000.0, (double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( line );
					f->WriteString( "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_THERMAL )
				{
					f->WriteString( "%" );
					line.Format( "AMTHERM%d*7,0,0,%.6f,%.6f,%.6f,0.0*", ia+10,
						(double)ap_array[ia].m_size1/25400000.0,	// outer diam
						(double)ap_array[ia].m_size2/25400000.0,	// inner diam
						(double)thermal_wid/25400000.0 );			// cross-hair width
					f->WriteString( line );
					f->WriteString( "%\n%" );
					line.Format( "ADD%dTHERM%d*", ia+10, ia+10 );
					f->WriteString( line );
					f->WriteString( "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_MOIRE )
				{
					f->WriteString( "%" );
					line.Format( "AMMOIRE%d*6,0,0,%.6f,0.005,0.050,3,0.005,%.6f,0.0*", ia+10,
						(double)ap_array[ia].m_size2/25400000.0,
						(double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( line );
					f->WriteString( "%\n%" );
					line.Format( "ADD%dMOIRE%d*", ia+10, ia+10 );
					f->WriteString( line );
					f->WriteString( "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_OCTAGON )
				{
					f->WriteString( "%" );
					line.Format( "ADD%dP,%.6fX8X22.5*", ia+10, 
						((double)ap_array[ia].m_size1/25400000.0 )/cos_oct );
					f->WriteString( line );
					f->WriteString( "%\n" );
				}
				else if( ap_array[ia].m_type == CAperture::AP_OVAL ) 
				{
					f->WriteString( "%" );
					line.Format( "ADD%dO,%.6fX%.6f*", ia+10, 
						(double)ap_array[ia].m_size2/25400000.0, (double)ap_array[ia].m_size1/25400000.0 );
					f->WriteString( line );
					f->WriteString( "%\n" );
				}
				else
					ASSERT(0);
			}
			f->WriteString( "G90*\n" );			// absolute format
			f->WriteString( "G70D02*\n" );	// use inches
		}

		// draw moires
		if( bd && (flags & GERBER_AUTO_MOIRES) )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 ----------------------- Draw moires (positive)*\n" );
				f->WriteString( "%LPD*%\n" );
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			CAperture moire_ap( CAperture::AP_MOIRE, 400*NM_PER_MIL, 350*NM_PER_MIL );
			int iap = moire_ap.FindInArray( &ap_array, PASS0 );
			if( iap == -1 )
				ASSERT(0);	// aperture not found
			if( PASS1 )
			{
				line.Format( "G54D%2d*\n", iap+10 );
				f->WriteString( line );			// select aperture
				// now flash 3 moires
				int x = (bd_min_x - 500)*NM_PER_MIL;
				int y = bd_min_y*NM_PER_MIL;
				::WriteMoveTo( f, x, y, LIGHT_FLASH );	// lower left
				x = (bd_max_x + 500)*NM_PER_MIL;
				::WriteMoveTo( f, x, y, LIGHT_FLASH );	// lower right
				x = (bd_min_x - 500)*NM_PER_MIL;
				y = bd_max_y*NM_PER_MIL;
				::WriteMoveTo( f, x, y, LIGHT_FLASH );	// upper left
			}
		}
		// draw board outline
		if( bd && (flags & GERBER_BOARD_OUTLINE) )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 ----------------------- Draw board outline (positive)*\n" );
				f->WriteString( "%LPD*%\n" );
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			for( int ib=0; ib<bd->GetSize(); ib++ )
			{
				CPolyLine * b = &(*bd)[ib];
				int nc = b->GetNumCorners();
				CAperture bd_ap( CAperture::AP_CIRCLE, outline_width, 0 );
				int iap = bd_ap.FindInArray( &ap_array, PASS0 );
				if( PASS1 )
				{
					if( iap == -1 )
						ASSERT(0);	// aperture not found
					line.Format( "G54D%2d*\n", iap+10 );
					f->WriteString( line );			// select aperture
					current_ap = bd_ap;
					// turn on linear interpolation, move to first corner
					::WriteMoveTo( f, b->GetX(0), b->GetY(0), LIGHT_OFF );
					for( int ic=1; ic<nc; ic++ )
					{
						int x = b->GetX(ic);
						int y = b->GetY(ic);
						::WritePolygonSide( f, b->GetX(ic-1), b->GetY(ic-1),
							b->GetX(ic), b->GetY(ic), b->GetSideStyle(ic-1), 10, LIGHT_ON ); 
						line.Format( "G04 end of side %d*\n", ic );
						f->WriteString( line );

					}
					::WritePolygonSide( f, b->GetX(nc-1), b->GetY(nc-1), 
						b->GetX(0), b->GetY(0), b->GetSideStyle(nc-1), 10, LIGHT_ON ); 
				}
			}
		}
		// establish nesting order of copper areas and cutouts:
		//	- for each area, tabulate all cutouts that contain it
		//	LOOP:
		//	- draw all areas that are only contained by cutouts that are already drawn
		//	- draw cutouts for those areas
		//  END_LOOP:
		//
		// loop through all nets
		BOOL areas_present = FALSE;
		int num_area_nets = 0;		// 0, 1 or 2 (2 if more than 1) 
		cnet * first_area_net = NULL;
		CArray<cnet*> area_net_list;
		CArray<carea*> area_list;
		cnet * net = nl->GetFirstNet();
		while( net )
		{
			// loop through all areas
			for( int ia=0; ia<net->nareas; ia++ )
			{
				CPolyLine * p = net->area[ia].poly;
				if( p->GetLayer() == layer )
				{
					// area on this layer, add to lists
					areas_present = TRUE;
					area_net_list.Add( net );
					area_list.Add( &net->area[ia] );
					// keep track of whether we have areas on separate nets
					if( num_area_nets == 0 )
					{
						num_area_nets = 1;
						first_area_net = net;
					}
					else if( num_area_nets == 1 && net != first_area_net )
						num_area_nets = 2;
				}
			}
			net = nl->GetNextNet();
		}

		if( PASS1 )
		{
			CArray< c_area > ca;
			cnet * net = nl->GetFirstNet();
			while( net )
			{
				// loop through all areas
				for( int ia=0; ia<net->nareas; ia++ )
				{
					net->area[ia].utility = 0;		// mark as undrawn
					CPolyLine * p = net->area[ia].poly;
					if( p->GetLayer() == layer )
					{
						// area on this layer, add to lists
						areas_present = TRUE;
						area_net_list.Add( net );
						area_list.Add( &net->area[ia] );
						// keep track of whether we have areas on separate nets
						if( num_area_nets == 0 )
						{
							num_area_nets = 1;
							first_area_net = net;
						}
						else if( num_area_nets == 1 && net != first_area_net )
							num_area_nets = 2;
						// now find any cutouts that contain this area
						double x = p->GetX(0);
						double y = p->GetY(0);
						int ica = ca.GetSize();
						ca.SetSize(ica+1);
						ca[ica].net = net;
						ca[ica].ia = ia;
						carea * a = &net->area[ia];
						// loop through all nets to find cutouts
						cnet * cutout_net = nl->GetFirstNet();
						while( cutout_net )
						{
							// loop through all areas to find cutouts
							for( int cutout_ia=0; cutout_ia<cutout_net->nareas; cutout_ia++ )
							{
								carea * cutout_a = &cutout_net->area[cutout_ia];
								CPolyLine * cutout_p = cutout_a->poly;
								if( cutout_p->GetLayer() == layer )
								{
									// loop through all cutouts
									for( int cutout_ic=0; cutout_ic<p->GetNumContours()-1; cutout_ic++ )
									{
										// test whether area (net, ia) is contained by
										// cutout (cut_net, cut_ia, cut_ic)
										BOOL b = cutout_p->TestPointInsideContour( cutout_ic+1, x, y );
										if( b )
										{
											// yes, enter it in array 
											int ci = ca[ica].containers.GetSize();
											ca[ica].containers.SetSize(ci+1);
											ca[ica].containers[ci].net = cutout_net;
											ca[ica].containers[ci].ia = cutout_ia;
											ca[ica].containers[ci].ic = cutout_ic;
										}
									}
								}
							}
							cutout_net = nl->GetNextNet();
						}
					}
				}
				net = nl->GetNextNet();
			}


			// set order for drawing areas, save in net->area[ia].utility
			int area_pass = 0;
			while( ca.GetSize() > 0 )
			{
				// loop through all areas in ca, draw those that are contained by
				// cutouts that are already drawn
				area_pass++;
				for( int ica=0; ica<ca.GetSize(); ica++ )
				{
					BOOL bDrawIt = TRUE;
					// test whether all cutouts that contain it have been drawn
					for( int ic=0; ic<ca[ica].containers.GetSize(); ic++ )
					{
						cnet * cutout_net = ca[ica].containers[ic].net;
						int cutout_ia = ca[ica].containers[ic].ia;
						if( cutout_net->area[cutout_ia].utility == 0 )
						{
							bDrawIt = FALSE;
							break;	// no, can't draw this area
						}
					}
					if( bDrawIt )
					{
						cnet * net = ca[ica].net;
						int ia = ca[ica].ia;
						net->area[ia].utility = area_pass;	// mark as drawn in this pass
						ca.RemoveAt(ica);	// remove from list
					}
				}
			}
			ca.RemoveAll();

			// now actually draw areas and cutouts
			area_pass = 0;
			int n_undrawn = 1;
			BOOL bLastLayerNegative = FALSE;
			while( n_undrawn )
			{
				n_undrawn = 0;
				area_pass++;
				// draw areas
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
				net = nl->GetFirstNet();
				while( net )
				{
					for( int ia=0; ia<net->nareas; ia++ )
					{
						carea * a = &net->area[ia];
						if ( a->utility == area_pass )
						{
							// draw outline polygon
							// make GpcPoly for outer contour of area
							a->poly->MakeGpcPoly();
							// draw area
							areas_present = TRUE;
							f->WriteString( "\nG04 ----------------------- Draw copper area (positive)*\n" );
							if( bLastLayerNegative )
							{
								f->WriteString( "%LPD*%\n" );
								bLastLayerNegative = FALSE;
							}
							f->WriteString( "G36*\n" );
							int x, y, style;
							int last_x = a->poly->GetX(0);
							int last_y = a->poly->GetY(0);
							::WriteMoveTo( f, last_x, last_y, LIGHT_OFF );
							int nc = a->poly->GetContourSize(0);
							for( int ic=1; ic<nc; ic++ )
							{
								x = a->poly->GetX(ic);
								y = a->poly->GetY(ic);
								style = a->poly->GetSideStyle(ic-1);
								::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
								last_x = x;
								last_y = y;
							}
							x = a->poly->GetX(0);
							y = a->poly->GetY(0);
							style = a->poly->GetSideStyle(nc-1);
							::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
							f->WriteString( "G37*\n" );
						}
						else if( a->utility > area_pass )
							n_undrawn++;
					}
					net = nl->GetNextNet();
				}
				// draw area cutouts
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
				net = nl->GetFirstNet();
				while( net )
				{
					for( int ia=0; ia<net->nareas; ia++ )
					{
						carea * a = &net->area[ia];
						if ( a->utility == area_pass )
						{
							// draw cutout polygons
							// make clearances for area cutouts
							CPolyLine * p = net->area[ia].poly;
							if( p->GetLayer() == layer )
							{
								for( int icont=1; icont<p->GetNumContours(); icont++ )
								{
									int ic_st = p->GetContourStart( icont );
									int ic_end = p->GetContourEnd( icont );
									// draw it
									f->WriteString( "\nG04 -------------------- Draw copper area cutout (negative)*\n" );
									if( !bLastLayerNegative )
									{
										f->WriteString( "%LPC*%\n" );
										bLastLayerNegative = TRUE;
									}
									f->WriteString( "G36*\n" );
									int x, y, style;
									int last_x = net->area[ia].poly->GetX(ic_st);
									int last_y = net->area[ia].poly->GetY(ic_st);
									::WriteMoveTo( f, last_x, last_y, LIGHT_OFF );
									for( int ic=ic_st+1; ic<=ic_end; ic++ )
									{
										x = net->area[ia].poly->GetX(ic);
										y = net->area[ia].poly->GetY(ic);
										style = net->area[ia].poly->GetSideStyle(ic-1);
										::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
										last_x = x;
										last_y = y;
									}
									x = net->area[ia].poly->GetX(ic_st);
									y = net->area[ia].poly->GetY(ic_st);
									style = net->area[ia].poly->GetSideStyle(ic_end);
									::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
									f->WriteString( "G37*\n" );
								}
							}
						}
					}
					net = nl->GetNextNet();
				}
			}
		}

		if( areas_present )
		{
			// ********** draw pad, trace, and via clearances and thermals ***********
			if( PASS1 ) 
			{
				f->WriteString( "\nG04 -------------------- Draw copper area clearances (negative)*\n" );
				f->WriteString( "%LPC*%\n" );
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			if( pl ) 
			{
				// iterate through all parts for pad clearances and thermals
				if( PASS1 )
				{
					f->WriteString( "\nG04 Draw clearances for pads*\n" );
				}
				cpart * part = pl->m_start.next;
				while( part->next != 0 ) 
				{
					CShape * s = part->shape;
					if( s )
					{
						// iterate through all pins
						for( int ip=0; ip<s->GetNumPins(); ip++ )
						{
							// get pad info
							int pad_x;
							int pad_y;
							int pad_w;
							int pad_l;
							int pad_r;
							int pad_type;
							int pad_hole;
							int pad_connect;
							int pad_angle;
							cnet * pad_net;
							BOOL bPad = pl->GetPadDrawInfo( part, ip, layer, annular_ring_pins, mask_clearance,
								&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, &pad_hole, &pad_angle,
								&pad_net, &pad_connect );

							if( bPad )
							{
								// pad or hole exists on this layer
								int type = CAperture::AP_NONE;
								int size1=0, size2=0;
								if( pad_type == PAD_NONE && pad_hole > 0 )
								{
									// just make hole clearance
									type = CAperture::AP_CIRCLE;
									size1 = (pad_hole + 2*hole_clearance);
								}
								else if( pad_type != PAD_NONE )
								{
									if( pad_connect & CPartList::THERMAL_CONNECT ) 
									{
										if( !(flags & GERBER_NO_PIN_THERMALS) )
										{
											// make thermal for pad
											if( pad_type == PAD_ROUND )
											{
												type = CAperture::AP_THERMAL;
												size1 = max( pad_w + 2*fill_clearance, pad_hole + 2*hole_clearance );
												size2 = pad_w;	// inner diameter
											}
											if( pad_type == PAD_OCTAGON )
											{
												type = CAperture::AP_THERMAL; 
												size1 = (max( pad_w + 2*fill_clearance, pad_hole + 2*hole_clearance ))/cos_oct;
												size2 = pad_w;	// inner diameter
											}
											else if( pad_type == PAD_RECT || pad_type == PAD_RRECT || pad_type == PAD_OVAL 
												|| pad_type == PAD_SQUARE )
											{
												// can't use an aperture for this pad, need to draw a polygon
												// if hole, check hole clearance
												int x1, x2, y1, y2;
												if( pad_type == PAD_RECT || pad_type == PAD_RRECT || pad_type == PAD_OVAL )
												{
													if( pad_angle == 90 )
													{
														x1 = pad_x - pad_w/2;
														x2 = pad_x + pad_w/2;
														y1 = pad_y - pad_l/2;
														y2 = pad_y + pad_l/2;
													}
													else if( pad_angle == 0 ) 
													{
														x1 = pad_x - pad_l/2;
														x2 = pad_x + pad_l/2;
														y1 = pad_y - pad_w/2;
														y2 = pad_y + pad_w/2;
													}
													else
														ASSERT(0);
												}
												else
												{
													x1 = pad_x - pad_w/2;
													x2 = pad_x + pad_w/2;
													y1 = pad_y - pad_w/2;
													y2 = pad_y + pad_w/2;
												}
												int x_clearance = fill_clearance;
												int y_clearance = fill_clearance;
												if( pad_hole )
												{
													// if necessary, adjust for hole clearance
													x_clearance = max( fill_clearance, hole_clearance+pad_hole/2-((x2-x1)/2));
													y_clearance = max( fill_clearance, hole_clearance+pad_hole/2-((y2-y1)/2));
												}
												// add clearance
												x1 -= x_clearance;
												x2 += x_clearance;
												y1 -= y_clearance;
												y2 += y_clearance;
												// now draw 4 rectangles
												if( PASS1 )
												{
													f->WriteString( "G36*\n" );
													WriteMoveTo( f, x1, y1, LIGHT_OFF );
													WriteMoveTo( f, x1, pad_y - thermal_wid/2, LIGHT_ON );
													WriteMoveTo( f, pad_x - thermal_wid/2, pad_y - thermal_wid/2, LIGHT_ON );
													WriteMoveTo( f, pad_x - thermal_wid/2, y1, LIGHT_ON );
													WriteMoveTo( f, x1, y1, LIGHT_ON );
													f->WriteString( "G37*\n" );
													//
													f->WriteString( "G36*\n" );
													WriteMoveTo( f, pad_x + thermal_wid/2, y1, LIGHT_OFF );
													WriteMoveTo( f, pad_x + thermal_wid/2, pad_y - thermal_wid/2, LIGHT_ON );
													WriteMoveTo( f, x2, pad_y - thermal_wid/2, LIGHT_ON );
													WriteMoveTo( f, x2, y1, LIGHT_ON );
													WriteMoveTo( f, pad_x + thermal_wid/2, y1, LIGHT_ON );
													f->WriteString( "G37*\n" );
													//
													f->WriteString( "G36*\n" );
													WriteMoveTo( f, pad_x + thermal_wid/2, pad_y + thermal_wid/2, LIGHT_OFF );
													WriteMoveTo( f, pad_x + thermal_wid/2, y2, LIGHT_ON );
													WriteMoveTo( f, x2, y2, LIGHT_ON );
													WriteMoveTo( f, x2, pad_y + thermal_wid/2, LIGHT_ON );
													WriteMoveTo( f, pad_x + thermal_wid/2, pad_y + thermal_wid/2, LIGHT_ON );
													f->WriteString( "G37*\n" );
													//
													f->WriteString( "G36*\n" );
													WriteMoveTo( f, x1, pad_y + thermal_wid/2, LIGHT_OFF );
													WriteMoveTo( f, x1, y2, LIGHT_ON );
													WriteMoveTo( f, pad_x - thermal_wid/2, y2, LIGHT_ON );
													WriteMoveTo( f, pad_x - thermal_wid/2, pad_y + thermal_wid/2, LIGHT_ON );
													WriteMoveTo( f, x1, pad_y + thermal_wid/2, LIGHT_ON );
													f->WriteString( "G37*\n" );
												}
											}
										}
									}
									else
									{
										// make clearance for pad
										size1 = max ( pad_w + 2*fill_clearance, pad_hole + 2*hole_clearance );
										size2 = 0;
										if( pad_type == PAD_ROUND )
											type = CAperture::AP_CIRCLE;
										else if( pad_type == PAD_SQUARE )
											type = CAperture::AP_SQUARE;
										else if( pad_type == PAD_OCTAGON )
											type = CAperture::AP_OCTAGON;
										else if( pad_type == PAD_OVAL ) 
										{
											type = CAperture::AP_OVAL;
											size2 = max ( pad_l + 2*fill_clearance, pad_hole + 2*hole_clearance );
											if( pad_angle == 90 )
											{
												int temp = size1;
												size1 = size2;
												size2 = temp;
											}
										}
										else if( pad_type == PAD_RECT || pad_type == PAD_RRECT )
										{
											// can't use an aperture for this pad, need to draw a polygon
											int x1, x2, y1, y2;
											if( pad_angle == 90 )
											{
												x1 = pad_x - pad_w/2;
												x2 = pad_x + pad_w/2;
												y1 = pad_y - pad_l/2;
												y2 = pad_y + pad_l/2;
											}
											else if( pad_angle == 0 ) 
											{
												x1 = pad_x - pad_l/2;
												x2 = pad_x + pad_l/2;
												y1 = pad_y - pad_w/2;
												y2 = pad_y + pad_w/2;
											}
											else
												ASSERT(0);
											int x_clearance = fill_clearance;
											int y_clearance = fill_clearance;
											if( pad_hole )
											{
												// if necessary, adjust for hole clearance
												x_clearance = max( fill_clearance, hole_clearance+pad_hole/2-((x2-x1)/2));
												y_clearance = max( fill_clearance, hole_clearance+pad_hole/2-((y2-y1)/2));
											}
											// add clearance
											x1 -= x_clearance;
											x2 += x_clearance;
											y1 -= y_clearance;
											y2 += y_clearance;
											if( PASS1 )
											{
												f->WriteString( "G36*\n" );
												WriteMoveTo( f, x1, y1, LIGHT_OFF );
												WriteMoveTo( f, x1, y2, LIGHT_ON );
												WriteMoveTo( f, x2, y2, LIGHT_ON );
												WriteMoveTo( f, x2, y1, LIGHT_ON );
												WriteMoveTo( f, x1, y1, LIGHT_ON );
												f->WriteString( "G37*\n" );
											}
										}
									}
								}
								// now flash the aperture
								if( type != CAperture::AP_NONE )
								{
									CAperture pad_ap( type, size1, size2 );
									if( !current_ap.Equals( &pad_ap ) )
									{
										// need to change aperture
										current_iap = pad_ap.FindInArray( &ap_array, PASS0 );
										if( PASS1 )
										{
											if( current_iap < 0 )
												ASSERT(0);	// aperture not found in list
											current_ap = pad_ap;
											line.Format( "G54D%2d*\n", current_iap+10 );
											f->WriteString( line );	 // select new aperture
										}
									}
									if( PASS1 )
									{
										// now flash the pad
										::WriteMoveTo( f, part->pin[ip].x, part->pin[ip].y, LIGHT_FLASH );
									}
								}
							}
						}
					}
					part = part->next;
				}
			} // end if pl
			if( nl ) 
			{
				// iterate through all nets and draw trace and via clearances
				if( PASS1 )
				{
					f->WriteString( "\nG04 Draw clearances for traces*\n" );
				}
				POSITION pos;
				CString name;
				void * ptr;
				for( pos = nl->m_map.GetStartPosition(); pos != NULL; )
				{
					nl->m_map.GetNextAssoc( pos, name, ptr );
					cnet * net = (cnet*)ptr;
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						int nsegs = net->connect[ic].nsegs;
						cconnect * c = &net->connect[ic]; 
						for( int is=0; is<nsegs; is++ )
						{
							// get segment and vertices
							cseg * s = &c->seg[is];
							cvertex * pre_vtx = &c->vtx[is];
							cvertex * post_vtx = &c->vtx[is+1];
							double xi = pre_vtx->x;
							double yi = pre_vtx->y;
							double xf = post_vtx->x;
							double yf = post_vtx->y;
							double seg_angle = atan2( yf - yi, xf - xi );
							double w = (double)fill_clearance + (double)(s->width)/2.0
											- (double)CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL/2;
							int test = nl->GetViaConnectionStatus( net, ic, is+1, layer );
							// flash the via clearance if necessary
							if( post_vtx->via_w && layer >= LAY_TOP_COPPER )
							{
								// via exists and this is a copper layer
								// set aperture to draw normal via clearance
								int type = CAperture::AP_CIRCLE;
								int size1 = max( post_vtx->via_w + 2*fill_clearance, post_vtx->via_hole_w + 2*hole_clearance );
								int size2 = 0;
								if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
								{
									// inner layer and no trace or thermal, just make hole clearance
									type = CAperture::AP_CIRCLE;
									size1 = post_vtx->via_hole_w + 2*hole_clearance;
								}
								else if( layer > LAY_BOTTOM_COPPER && test & CNetList::VIA_THERMAL && !(test & CNetList::VIA_TRACE) )
								{
									if( flags & GERBER_NO_VIA_THERMALS )
									{
										type = CAperture::AP_NONE;
									}
									else
									{
										// small thermal
										type = CAperture::AP_THERMAL;
										size1 = max( post_vtx->via_hole_w + 2*annular_ring_vias + 2*fill_clearance, 
											post_vtx->via_hole_w + 2*hole_clearance );
										size2 = post_vtx->via_hole_w + 2*annular_ring_vias;
									}
								}
								else if( test & CNetList::VIA_THERMAL )
								{
									if( flags & GERBER_NO_VIA_THERMALS )
									{
										type = CAperture::AP_NONE;
									}
									else
									{
										// thermal
										type = CAperture::AP_THERMAL;
										size1 = max( post_vtx->via_w + 2*fill_clearance, post_vtx->via_hole_w + 2*hole_clearance );
										size2 = post_vtx->via_w;
									}
								}
								if( type != CAperture::AP_NONE )
								{
									CAperture via_ap( type, size1, size2 );
									if( !current_ap.Equals( &via_ap ) )
									{
										// change aperture
										current_iap = via_ap.FindInArray( &ap_array, PASS0 );
										if( PASS1 )
										{
											if( current_iap < 0 )
												ASSERT(0);	// aperture not found in list
											current_ap = via_ap;
											line.Format( "G54D%2d*\n", current_iap+10 );
											f->WriteString( line );	 // select new aperture
										}
									}
									if( PASS1 )
									{
										// flash the via clearance
										WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_FLASH );
									}
								}
							}

							if( s->layer == layer && num_area_nets == 1 && net != first_area_net ) 
							{
								// segment is on this layer and there is a single copper area
								// on this layer not on the same net, draw clearance
								int type = CAperture::AP_CIRCLE;
								int size1 = s->width + 2*fill_clearance;
								CAperture seg_ap( type, size1, 0 );
								if( !current_ap.Equals( &seg_ap ) )
								{
									// change aperture
									current_iap = seg_ap.FindInArray( &ap_array, PASS0 );
									if( PASS1 )
									{
										if( current_iap < 0 )
											ASSERT(0);	// aperture not found in list
										current_ap = seg_ap;
										line.Format( "G54D%2d*\n", current_iap+10 );
										f->WriteString( line );	 // select new aperture
									}
								}
								if( PASS1 )
								{
									WriteMoveTo( f, xi, yi, LIGHT_OFF );
									WriteMoveTo( f, xf, yf, LIGHT_ON );
								}
							}
							else if( s->layer == layer && num_area_nets == 2 )
							{
								// test for segment intersection with area on own net
								BOOL bIntOwnNet = FALSE;
								for( int ia=0; ia<net->nareas; ia++ )
								{
									CPolyLine * poly = net->area[ia].poly; 
									if( poly->TestPointInside( xi, yi ) )
									{
										bIntOwnNet = TRUE;
										break;
									}
									if( poly->TestPointInside( xf, yf ) )
									{
										bIntOwnNet = TRUE;
										break;
									}
									for( int icont=0; icont<poly->GetNumContours(); icont++ )
									{
										int cont_start = poly->GetContourStart(icont);
										int cont_end = poly->GetContourEnd(icont);
										for( int is=cont_start; is<=cont_end; is++ )
										{
											// test for clearance from area sides < fill_clearance
											int x2i = poly->GetX(is);
											int y2i = poly->GetY(is);
											int ic2 = is+1;
											if( ic2 > cont_end )
												ic2 = cont_start;
											int x2f = poly->GetX(ic2);
											int y2f = poly->GetY(ic2);
											int style2 = poly->GetSideStyle( is );
											int d = ::GetClearanceBetweenSegments( xi, yi, xf, yf, CPolyLine::STRAIGHT, s->width,
												x2i, y2i, x2f, y2f, style2, 0, fill_clearance, 0, 0 );
											if( d < fill_clearance )
											{
												bIntOwnNet = TRUE;
												break;
											}
										}
										if( bIntOwnNet )
											break;
									}
								}
								if( bIntOwnNet )
								{
									// set aperture for stroke outline
									if( PASS0 )
									{
										int type = CAperture::AP_CIRCLE;
										int size1 = CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL;
										CAperture seg_ap( type, size1, 0 );
										if( !current_ap.Equals( &seg_ap ) )
										{
											// change aperture
											current_iap = seg_ap.FindInArray( &ap_array, PASS0 );
										}
									}
									if( PASS1 )
									{
										// handle segment that crosses from an area on its own net to
										// an area on a foreign net
										BOOL bClearanceMade = FALSE;
										gpc_vertex_list gpc_contour;
										gpc_polygon gpc_seg_poly;
										// now loop through all areas on foreign nets
										for( int ia=0; ia<area_list.GetSize(); ia++ )
										{
											if( area_net_list[ia] != net )
											{
												// foreign net, see if possible intersection 
												carea * area = area_list[ia];
												CPolyLine * p = area->poly;
												BOOL bIntersection = FALSE;
												if( p->TestPointInside( xi, yi ) )
													bIntersection = TRUE;
												if( !bIntersection )
													if( p->TestPointInside( xf, yf ) )
														bIntersection = TRUE;
												if( !bIntersection )
												{
													for( int icont=0; icont<p->GetNumContours(); icont++ )
													{
														int cont_start = p->GetContourStart(icont);
														int cont_end = p->GetContourEnd(icont);
														for( int is=cont_start; is<=cont_end; is++ )
														{
															int ic2 = is+1;
															if( ic2 > cont_end )
																ic2 = cont_start;
															int w = s->width;
															int d = GetClearanceBetweenSegments( xi, yi, xf, yf, CPolyLine::STRAIGHT, w,
																p->GetX(is), p->GetY(is), p->GetX(ic2), p->GetY(ic2), p->GetSideStyle(is), 0, 
																25*NM_PER_MIL, NULL, NULL );
															if( d < 25*NM_PER_MIL )
															{
																bIntersection = TRUE;
																break;
															}
														}
													}
												}
												if( bIntersection )
												{
													if( !bClearanceMade )
													{
														// construct a gpc_poly for the clearance
														int npoints = 18;	// number of points in poly
														gpc_contour.num_vertices = npoints;
														gpc_contour.vertex = (gpc_vertex*)calloc( 2 * npoints, sizeof(double) );
														if( !gpc_contour.vertex )
															ASSERT(0);
														double x,y;
														// create points around beginning of segment
														double angle = seg_angle + pi/2.0;		// rotate 90 degrees ccw
														double angle_step = pi/(npoints/2-1);
														for( int i=0; i<npoints/2; i++ )
														{
															x = xi + w*cos(angle);
															y = yi + w*sin(angle);
															gpc_contour.vertex[i].x = x;
															gpc_contour.vertex[i].y = y;
															angle += angle_step;
														}
														// create points around end of segment
														angle = seg_angle - pi/2.0;
														for( int i=npoints/2; i<npoints; i++ )
														{
															x = xf + w*cos(angle);
															y = yf + w*sin(angle);
															gpc_contour.vertex[i].x = x;
															gpc_contour.vertex[i].y = y;
															angle += angle_step;
														}
														gpc_seg_poly.num_contours = 1;
														gpc_seg_poly.hole = new int;
														gpc_seg_poly.hole[0] = 0;
														gpc_seg_poly.contour = &gpc_contour;
														bClearanceMade = TRUE;
													}
													// intersect area and clearance polys
													gpc_polygon gpc_intersection;
													gpc_intersection.num_contours = 0;
													gpc_intersection.hole = NULL;
													gpc_intersection.contour = NULL;
													gpc_polygon_clip( GPC_INT, &gpc_seg_poly, area->poly->GetGpcPoly(),
														&gpc_intersection );
													int ncontours = gpc_intersection.num_contours;
													for( int ic=0; ic<ncontours; ic++ )
													{
														// draw clearance
														gpc_vertex * gpv = gpc_intersection.contour[ic].vertex;
														int nv = gpc_intersection.contour[ic].num_vertices;
														if( PASS1 )
														{
															f->WriteString( "G36*\n" );
															WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_OFF );
															for( int iv=1; iv<nv; iv++ )
																WriteMoveTo( f, gpv[iv].x, gpv[iv].y, LIGHT_ON );
															WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_ON );
															f->WriteString( "G37*\n" );
														}
														// now stroke outline to remove any truncation artifacts
														int type = CAperture::AP_CIRCLE;
														int size1 = CLEARANCE_POLY_STROKE_MILS*NM_PER_MIL;
														CAperture seg_ap( type, size1, 0 );
														if( !current_ap.Equals( &seg_ap ) )
														{
															// change aperture
															current_iap = seg_ap.FindInArray( &ap_array, PASS0 );
															if( current_iap < 0 )
																ASSERT(0);	// aperture not found in list
															current_ap = seg_ap;
															line.Format( "G54D%2d*\n", current_iap+10 );
															f->WriteString( line );	 // select new aperture
														}
														WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_OFF );
														for( int iv=1; iv<nv; iv++ )
															WriteMoveTo( f, gpv[iv].x, gpv[iv].y, LIGHT_ON );
														WriteMoveTo( f, gpv[0].x, gpv[0].y, LIGHT_ON );
													}
													for( int ic=0; ic<ncontours; ic++ )
														free( gpc_intersection.contour[ic].vertex );
													// free intersection
													free( gpc_intersection.hole );
													free( gpc_intersection.contour );
												}
											}
										}
										if( bClearanceMade )
										{
											free( gpc_contour.vertex );
											delete gpc_seg_poly.hole;
										}
									}
								}
								else
								{
									// segment does not intersect area on own net, just make clearance
									int w = s->width + 2*fill_clearance;
									CAperture seg_ap( CAperture::AP_CIRCLE, w, 0 );
									if( !seg_ap.Equals( &current_ap ) )
									{
										// change aperture
										current_iap = seg_ap.FindInArray( &ap_array, PASS0 );
										if( PASS1 )
										{
											if( current_iap < 0 )
												ASSERT(0);	// aperture not found in list
											current_ap = seg_ap;
											line.Format( "G54D%2d*\n", current_iap+10 );
											f->WriteString( line );	 // select new aperture
										}
									}
									if( PASS1 )
									{
										WriteMoveTo( f, pre_vtx->x, pre_vtx->y, LIGHT_OFF );
										WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_ON );
									}
								}
							}
						}
					}
#if 0
					if( PASS1 )
					{
						for( int ia=0; ia<net->nareas; ia++ )
						{
							// make clearances for area cutouts
							CPolyLine * p = net->area[ia].poly;
							if( p->GetLayer() == layer )
							{
								for( int icont=1; icont<p->GetNumContours(); icont++ )
								{
									int ic_st = p->GetContourStart( icont );
									int ic_end = p->GetContourEnd( icont );
									// draw it
									f->WriteString( "G36*\n" );
									int x, y, style;
									int last_x = net->area[ia].poly->GetX(ic_st);
									int last_y = net->area[ia].poly->GetY(ic_st);
									::WriteMoveTo( f, last_x, last_y, LIGHT_OFF );
									for( int ic=ic_st+1; ic<=ic_end; ic++ )
									{
										x = net->area[ia].poly->GetX(ic);
										y = net->area[ia].poly->GetY(ic);
										style = net->area[ia].poly->GetSideStyle(ic-1);
										::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
										last_x = x;
										last_y = y;
									}
									x = net->area[ia].poly->GetX(ic_st);
									y = net->area[ia].poly->GetY(ic_st);
									style = net->area[ia].poly->GetSideStyle(ic_end);
									::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
									f->WriteString( "G37*\n" );
								}
							}
						}
					}
#endif
				}
			}		
			if( tl )
			{
				// draw clearances for text
				if( PASS1 )
				{
					f->WriteString( "\nG04 Draw clearances for text*\n" );
				}
				for( int it=0; it<tl->text_ptr.GetSize(); it++ )
				{
					CText * t = tl->text_ptr[it];
					if( t->m_layer == layer )
					{
						// draw text
						int w = t->m_stroke_width + 2*fill_clearance;
						CAperture text_ap( CAperture::AP_CIRCLE, w, 0 );
						if( !text_ap.Equals( &current_ap ) )
						{
							// change aperture
							current_iap = text_ap.FindInArray( &ap_array, PASS0 );
							if( PASS1 )
							{
								if( current_iap < 0 )
									ASSERT(0);	// aperture not found in list
								current_ap = text_ap;
								line.Format( "G54D%2d*\n", current_iap+10 );
								f->WriteString( line );	 // select new aperture
							}
						}
						if( PASS1 )
						{
							for( int istroke=0; istroke<t->m_stroke.GetSize(); istroke++ )
							{
								::WriteMoveTo( f, t->m_stroke[istroke].xi, t->m_stroke[istroke].yi, LIGHT_OFF );
								::WriteMoveTo( f, t->m_stroke[istroke].xf, t->m_stroke[istroke].yf, LIGHT_ON );
							}
						}
					}
				}
			} // end loop through nets
		}

		// ********************** draw pads, vias and traces **************************
		if( PASS1 )
		{
			f->WriteString( "\nG04 -------------- Draw Parts, Pads, Traces, Vias and Text (positive)*\n" );
			f->WriteString( "%LPD*%\n" );
			current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
		}
		// draw pads and reference designators
		if( pl )
		{
			// iterate through all parts and draw pads
			cpart * part = pl->m_start.next;
			while( part->next != 0 )
			{
				CShape * s = part->shape;
				if( s )
				{
					if( PASS1 )
					{
						line.Format( "G04 Draw part %s*\n", part->ref_des ); 
						f->WriteString( line );
					}
					for( int ip=0; ip<s->GetNumPins(); ip++ )
					{
						// get pad info
						int pad_x;
						int pad_y;
						int pad_w;
						int pad_l;
						int pad_r;
						int pad_type;
						int pad_hole;
						int pad_connect;
						int pad_angle;
						cnet * pad_net;
						BOOL bPad = pl->GetPadDrawInfo( part, ip, layer, annular_ring_pins, mask_clearance,
							&pad_type, &pad_x, &pad_y, &pad_w, &pad_l, &pad_r, &pad_hole, &pad_angle,
							&pad_net, &pad_connect );

						// draw pad
						if( bPad && pad_type != PAD_NONE )
						{
							int type, size1, size2;
							if( pad_type == PAD_ROUND || pad_type == PAD_SQUARE 
								|| pad_type == PAD_OCTAGON || pad_type == PAD_OVAL )
							{
								type = CAperture::AP_CIRCLE;
								size1 = pad_w;
								size2 = 0;
								if( pad_type == PAD_SQUARE )
									type = CAperture::AP_SQUARE;
								else if( pad_type == PAD_OCTAGON )
									type = CAperture::AP_OCTAGON;
								else if( pad_type == PAD_OVAL )
								{
									type = CAperture::AP_OVAL;
									size2 = pad_l;
									if( pad_angle == 90 )
									{
										int temp = size1;
										size1 = size2;
										size2 = temp;
									}
								}
								CAperture pad_ap( type, size1, size2 );
								if( !pad_ap.Equals( &current_ap ) )
								{
									// need to change aperture
									current_iap = pad_ap.FindInArray( &ap_array, PASS0 );
									if( PASS1 )
									{
										if( current_iap < 0 )
											ASSERT(0);	// aperture not found in list
										current_ap = pad_ap;
										line.Format( "G54D%2d*\n", current_iap+10 );
										f->WriteString( line );	 // select new aperture
									}
								}
								if( PASS1 )
								{
									// now flash the pad
									::WriteMoveTo( f, pad_x, pad_y, LIGHT_FLASH );
								}
							}
							else if( pad_type == PAD_RECT || pad_type == PAD_RRECT )
							{
								// test code
								int x1, x2, y1, y2;
								if( pad_angle == 90 )
								{
									x1 = pad_x - pad_w/2;
									x2 = pad_x + pad_w/2;
									y1 = pad_y - pad_l/2;
									y2 = pad_y + pad_l/2;
								}
								else if( pad_angle == 0 ) 
								{
									x1 = pad_x - pad_l/2;
									x2 = pad_x + pad_l/2;
									y1 = pad_y - pad_w/2;
									y2 = pad_y + pad_w/2;
								}
								else
									ASSERT(0);
								if( pad_type == PAD_RECT )
								{
									if( PASS1 )
									{
										f->WriteString( "G36*\n" );
										WriteMoveTo( f, x1, y1, LIGHT_OFF );
										WriteMoveTo( f, x1, y2, LIGHT_ON );
										WriteMoveTo( f, x2, y2, LIGHT_ON );
										WriteMoveTo( f, x2, y1, LIGHT_ON );
										WriteMoveTo( f, x1, y1, LIGHT_ON );
										f->WriteString( "G37*\n" );
									}
								}
								else
								{
									// get aperture for corners
									type = CAperture::AP_CIRCLE;
									size1 = 2*pad_r;
									size2 = 0;
									CAperture corner_ap( type, size1, size2 );
									if( !corner_ap.Equals( &current_ap ) )
									{
										current_iap = corner_ap.FindInArray( &ap_array, PASS0 );
										if( PASS1 )
										{
											if( current_iap < 0 )
												ASSERT(0);	// aperture not found in list
											current_ap = corner_ap;
											line.Format( "G54D%2d*\n", current_iap+10 );
											f->WriteString( line );	 // select new aperture
										}
									}
									if( PASS1 )
									{
										// now flash the corners
										::WriteMoveTo( f, x1+pad_r, y1+pad_r, LIGHT_FLASH );
										::WriteMoveTo( f, x2-pad_r, y1+pad_r, LIGHT_FLASH );
										::WriteMoveTo( f, x1+pad_r, y2-pad_r, LIGHT_FLASH );
										::WriteMoveTo( f, x2-pad_r, y2-pad_r, LIGHT_FLASH );
										// draw rectangles
										f->WriteString( "G36*\n" );
										WriteMoveTo( f, x1+pad_r, y1, LIGHT_OFF );
										WriteMoveTo( f, x1+pad_r, y2, LIGHT_ON );
										WriteMoveTo( f, x2-pad_r, y2, LIGHT_ON );
										WriteMoveTo( f, x2-pad_r, y1, LIGHT_ON );
										WriteMoveTo( f, x1+pad_r, y1, LIGHT_ON );
										f->WriteString( "G37*\n" );
										f->WriteString( "G36*\n" );
										WriteMoveTo( f, x1, y1+pad_r, LIGHT_OFF );
										WriteMoveTo( f, x1, y2-pad_r, LIGHT_ON );
										WriteMoveTo( f, x2, y2-pad_r, LIGHT_ON );
										WriteMoveTo( f, x2, y1+pad_r, LIGHT_ON );
										WriteMoveTo( f, x1, y1+pad_r, LIGHT_ON );
										f->WriteString( "G37*\n" );
									}
								}
							}
						}
					}
				}
				// now draw silkscreen items
				if( layer == LAY_SILK_TOP && part->side == 0 
					|| layer == LAY_SILK_BOTTOM && part->side == 1 )
				{
					// draw part outline
					if( PASS1 )
					{
						line.Format( "G04 draw part outline for part %s*\n", part->ref_des ); 
						f->WriteString( line );
					}
					int nstrokes = part->m_outline_stroke.GetSize();
					if( nstrokes )
					{
						for( int ips=0; ips<nstrokes; ips++ )
						{
							int s_w = max( part->m_outline_stroke[ips].w, min_silkscreen_stroke_wid );
							CAperture outline_ap( CAperture::AP_CIRCLE, s_w, 0 );
							if( !outline_ap.Equals( &current_ap ) )
							{
								// need to change aperture
								current_iap = outline_ap.FindInArray( &ap_array, PASS0 );
								if( PASS1 )
								{
									if( current_iap < 0 )
										ASSERT(0);	// aperture not found in list
									current_ap = outline_ap;
									line.Format( "G54D%2d*\n", current_iap+10 );
									f->WriteString( line );	 // select new aperture
								}
							}
							// move to start of stroke
							if( PASS1 )
							{
								::WriteMoveTo( f, part->m_outline_stroke[ips].xi, 
									part->m_outline_stroke[ips].yi, LIGHT_OFF );
								int type;
								if( part->m_outline_stroke[ips].type == DL_LINE )
									type = CPolyLine::STRAIGHT;
								else if( part->m_outline_stroke[ips].type == DL_ARC_CW )
									type = CPolyLine::ARC_CW;
								else if( part->m_outline_stroke[ips].type == DL_ARC_CCW )
									type = CPolyLine::ARC_CCW;
								else
									ASSERT(0);
								::WritePolygonSide( f, 
									part->m_outline_stroke[ips].xi, 
									part->m_outline_stroke[ips].yi, 
									part->m_outline_stroke[ips].xf, 
									part->m_outline_stroke[ips].yf, 
									type, 10, LIGHT_ON );
							}
						}
					}
					// draw reference designator text
					if( part->m_ref_size )
					{
						if( PASS1 )
						{
							line.Format( "G04 draw reference designator for part %s*\n", part->ref_des ); 
							f->WriteString( line );
						}
						int s_w = max( part->m_ref_w, min_silkscreen_stroke_wid );
						CAperture ref_ap( CAperture::AP_CIRCLE, s_w, 0 );
						if( !ref_ap.Equals( &current_ap ) )
						{
							// need to change aperture
							current_iap = ref_ap.FindInArray( &ap_array, PASS0 );
							if( PASS1 )
							{
								if( current_iap < 0 )
									ASSERT(0);	// aperture not found in list
								current_ap = ref_ap;
								line.Format( "G54D%2d*\n", current_iap+10 );
								f->WriteString( line );	 // select new aperture
							}
						}
						if( PASS1 )
						{
							for( int istroke=0; istroke<part->ref_text_stroke.GetSize(); istroke++ )
							{
								::WriteMoveTo( f, part->ref_text_stroke[istroke].xi, 
									part->ref_text_stroke[istroke].yi, LIGHT_OFF );
								::WriteMoveTo( f, part->ref_text_stroke[istroke].xf, 
									part->ref_text_stroke[istroke].yf, LIGHT_ON );
							}
						}
					}
				}
				// go to next part
				part = part->next;
			}
		}
		// draw vias and traces
		if( nl )
		{
			// iterate through all nets
			if( PASS1 )
			{
				f->WriteString( "\nG04 Draw traces*\n" );
			}
			POSITION pos;
			CString name;
			void * ptr;
			for( pos = nl->m_map.GetStartPosition(); pos != NULL; )
			{
				nl->m_map.GetNextAssoc( pos, name, ptr );
				cnet * net = (cnet*)ptr;
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					int nsegs = net->connect[ic].nsegs;
					for( int is=0; is<nsegs; is++ )
					{
						// get segment
						cseg * s = &(net->connect[ic].seg[is]);
						cvertex * pre_vtx = &(net->connect[ic].vtx[is]);
						cvertex * post_vtx = &(net->connect[ic].vtx[is+1]);
						int test = nl->GetViaConnectionStatus( net, ic, is+1, layer );
						if( s->layer == layer )
						{
							// segment is on this layer, draw it
							int w = s->width;
							CAperture seg_ap( CAperture::AP_CIRCLE, w, 0 );
							if( !seg_ap.Equals( &current_ap ) )
							{
								// change aperture
								current_iap = seg_ap.FindInArray( &ap_array, PASS0 );
								if( PASS1 )
								{
									if( current_iap < 0 )
										ASSERT(0);	// aperture not found in list
									current_ap = seg_ap;
									line.Format( "G54D%2d*\n", current_iap+10 );
									f->WriteString( line );	 // select new aperture
								}
							}
							if( PASS1 )
							{
								WriteMoveTo( f, pre_vtx->x, pre_vtx->y, LIGHT_OFF );
								WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_ON );
							}
						}
						if( post_vtx->via_w )
						{
							// via exists
							CAperture via_ap( CAperture::AP_CIRCLE, 0, 0 );
							int w = 0;
							if( layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM )
							{
								if( !(flags & GERBER_MASK_VIAS) )
								{
									// solder mask layer, add mask clearance
									w = post_vtx->via_w + 2*mask_clearance;
								}
							}
							else if( layer >= LAY_TOP_COPPER )
							{
								// copper layer, set aperture to normal via
								w = post_vtx->via_w;
								if( layer > LAY_BOTTOM_COPPER && test == CNetList::VIA_NO_CONNECT )
								{
									// inner layer and no trace or thermal, so no via pad
									w = 0;
								}
								else if( layer > LAY_BOTTOM_COPPER && (test & CNetList::VIA_THERMAL) && !(test & CNetList::VIA_TRACE) )
								{
									// inner layer with small thermal, use annular ring
									w = post_vtx->via_hole_w + 2*annular_ring_vias;
								}
							}
							if( w )
							{
								via_ap.m_size1 = w;
								if( !via_ap.Equals( &current_ap ) )
								{
									// change aperture
									current_iap = via_ap.FindInArray( &ap_array, PASS0 );
									if( PASS1 )
									{
										if( current_iap < 0 )
											ASSERT(0);	// aperture not found in list
										current_ap = via_ap;
										line.Format( "G54D%2d*\n", current_iap+10 );
										f->WriteString( line );	 // select new aperture
									}
								}
								// flash the via
								if( PASS1 )
								{
									WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_FLASH );
								}
							}
						}
					}
				}
			}
		}
		// draw text
		if( tl )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 Draw Text*\n" );
			}
			//draw layer identification string if requested
			CText * layer_text = NULL;
			if( flags & GERBER_LAYER_TEXT )
			{
				CString str = "";
				switch( layer )
				{
				case LAY_MASK_TOP: str = "Top Solder Mask"; break;
				case LAY_MASK_BOTTOM: str = "Bottom Solder Mask"; break;
				case LAY_SILK_TOP: str = "Top Silkscreen"; break;
				case LAY_SILK_BOTTOM: str = "Bottom Silkscreen"; break;
				case LAY_TOP_COPPER: str = "Top Copper Layer"; break;
				case LAY_BOTTOM_COPPER: str = "Bottom Copper Layer"; break;
				}
				if( layer > LAY_BOTTOM_COPPER )
					str.Format( "Inner %d Copper Layer", layer - LAY_BOTTOM_COPPER );
				layer_text = tl->AddText( bd_min_x, bd_min_y-LAYER_TEXT_HEIGHT*2, 0, 0, LAY_SILK_TOP,
					LAYER_TEXT_HEIGHT, LAYER_TEXT_STROKE_WIDTH, &str );
			}
			for( int it=0; it<tl->text_ptr.GetSize(); it++ )
			{
				CText * t = tl->text_ptr[it];
				if( t->m_font_size )
				{
					if( t->m_layer == layer || t == layer_text )
					{
						// draw text
						int w = max( t->m_stroke_width, min_silkscreen_stroke_wid );
						CAperture text_ap( CAperture::AP_CIRCLE, w, 0 );
						if( !text_ap.Equals( &current_ap ) )
						{
							// change aperture
							current_iap = text_ap.FindInArray( &ap_array, PASS0 );
							if( PASS1 )
							{
								if( current_iap < 0 )
									ASSERT(0);	// aperture not found in list
								current_ap = text_ap;
								line.Format( "G54D%2d*\n", current_iap+10 );
								f->WriteString( line );	 // select new aperture
							}
						}
						if( PASS1 )
						{
							for( int istroke=0; istroke<t->m_stroke.GetSize(); istroke++ )
							{
								::WriteMoveTo( f, t->m_stroke[istroke].xi, t->m_stroke[istroke].yi, LIGHT_OFF );
								::WriteMoveTo( f, t->m_stroke[istroke].xf, t->m_stroke[istroke].yf, LIGHT_ON );
							}
						}
					}
				}
			}
			if( layer_text )
				tl->RemoveText( layer_text );
		}

		// draw solder mask cutouts
		if( sm && (layer == LAY_MASK_TOP || layer == LAY_MASK_BOTTOM ) )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 Draw solder mask cutouts*\n" );
			}
			CAperture sm_ap( CAperture::AP_CIRCLE, mask_clearance*2, 0 );
			if( !sm_ap.Equals( &current_ap ) )
			{
				// change aperture
				current_iap = sm_ap.FindInArray( &ap_array, PASS0 );
				if( PASS1 )
				{
					if( current_iap < 0 )
						ASSERT(0);	// aperture not found in list
					current_ap = sm_ap;
					line.Format( "G54D%2d*\n", current_iap+10 );
					f->WriteString( line );	 // select new aperture
				}
			}
			if( PASS1 )
			{
				for( int i=0; i<sm->GetSize(); i++ )
				{
					CPolyLine * poly = &(*sm)[i];
					if( layer == LAY_MASK_TOP && poly->GetLayer() == LAY_SM_TOP 
						|| layer == LAY_MASK_BOTTOM && poly->GetLayer() == LAY_SM_BOTTOM )
					{
						// draw cutout on this layer
						f->WriteString( "G36*\n" );
						int x, y, style;
						int last_x = poly->GetX(0);
						int last_y = poly->GetY(0);
						::WriteMoveTo( f, last_x, last_y, LIGHT_OFF );
						int nc = poly->GetContourSize(0);
						for( int ic=1; ic<nc; ic++ )
						{
							x = poly->GetX(ic);
							y = poly->GetY(ic);
							style = poly->GetSideStyle(ic-1);
							::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
							last_x = x;
							last_y = y;
						}
						x = poly->GetX(0);
						y = poly->GetY(0);
						style = poly->GetSideStyle(nc-1);
						::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
						f->WriteString( "G37*\n" );
						// draw outline with aperture to make clearance
						last_x = poly->GetX(0);
						last_y = poly->GetY(0);
						::WriteMoveTo( f, last_x, last_y, LIGHT_OFF );
						nc = poly->GetContourSize(0);
						for( int ic=1; ic<nc; ic++ )
						{
							x = poly->GetX(ic);
							y = poly->GetY(ic);
							style = poly->GetSideStyle(ic-1);
							::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
							last_x = x;
							last_y = y;
						}
						x = poly->GetX(0);
						y = poly->GetY(0);
						style = poly->GetSideStyle(nc-1);
						::WritePolygonSide( f, last_x, last_y, x, y, style, 10, LIGHT_ON );
					}
				}
			}
		}

		// draw pilot holes for pads and vias
		if( (flags & GERBER_PILOT_HOLES) && pilot_diameter && (layer == LAY_TOP_COPPER || layer == LAY_BOTTOM_COPPER ) )
		{
			if( PASS1 )
			{
				f->WriteString( "\nG04 ----------------------- Draw Pilot Holes (scratch)*\n" );
				f->WriteString( "%LPC*%\n" );
				current_ap.m_type = CAperture::AP_NONE;	// force selection of aperture
			}
			if( pl )
			{
				// iterate through all parts
				cpart * part = pl->m_start.next;
				while( part->next != 0 )
				{
					CShape * s = part->shape;
					if( s )
					{
						if( PASS1 )
						{
							line.Format( "G04 draw pilot holes for part %s*\n", part->ref_des ); 
							f->WriteString( line );
						}
						for( int ip=0; ip<s->GetNumPins(); ip++ )
						{
							pad * p = 0;
							padstack * ps = &s->m_padstack[ip];
							if( ps->hole_size )
							{
								p = &ps->top;
								// check current aperture and change if needed
								CAperture pad_ap( CAperture::AP_CIRCLE, pilot_diameter, 0 );
								if( !pad_ap.Equals( &current_ap ) )
								{
									// need to change aperture
									current_iap = pad_ap.FindInArray( &ap_array, PASS0 );
									if( PASS1 )
									{
										if( current_iap < 0 )
											ASSERT(0);	// aperture not found in list
										current_ap = pad_ap;
										line.Format( "G54D%2d*\n", current_iap+10 );
										f->WriteString( line );	 // select new aperture
									}
								}
								// now flash the pad
								if( PASS1 )
								{
									::WriteMoveTo( f, part->pin[ip].x, part->pin[ip].y, LIGHT_FLASH );
								}
							}
						}
					}
					// go to next part
					part = part->next;
				}
			}
			// draw pilot holes for vias
			if( nl )
			{
				// iterate through all nets
				if( PASS1 )
				{
					f->WriteString( "\nG04 Draw pilot holes for vias*\n" );
				}
				POSITION pos;
				CString name;
				void * ptr;
				for( pos = nl->m_map.GetStartPosition(); pos != NULL; )
				{
					nl->m_map.GetNextAssoc( pos, name, ptr );
					cnet * net = (cnet*)ptr;
					for( int ic=0; ic<net->nconnects; ic++ )
					{
						int nsegs = net->connect[ic].nsegs;
						for( int is=0; is<nsegs; is++ )
						{
							// get segment
							cseg * s = &(net->connect[ic].seg[is]);
							cvertex * post_vtx = &(net->connect[ic].vtx[is+1]);
							if( post_vtx->via_w )
							{
								// via exists
								CAperture via_ap( CAperture::AP_CIRCLE, pilot_diameter, 0 );
								if( !via_ap.Equals( &current_ap ) )
								{
									// change aperture
									current_iap = via_ap.FindInArray( &ap_array, PASS0 );
									if( PASS1 )
									{
										if( current_iap < 0 )
											ASSERT(0);	// aperture not found in list
										current_ap = via_ap;
										line.Format( "G54D%2d*\n", current_iap+10 );
										f->WriteString( line );	 // select new aperture
									}
								}
								// flash the via
								if( PASS1 )
									::WriteMoveTo( f, post_vtx->x, post_vtx->y, LIGHT_FLASH );
							}
						}
					}
				}
			}
		}

		// end of file
		if( PASS1 )
			f->WriteString( "M00*\n" );

	}	// end of pass
	return 0;
}

// find value in CArray<int> and return position in array
// if not found, add to array if add_ok = TRUE, otherwise return -1
//
int AddToArray( int value, CArray<int,int> * array )
{
	for( int i=0; i<array->GetSize(); i++ )
		if( value == array->GetAt(i) )
			return i;
	array->Add( value );
	return array->GetSize()-1;
}

// write NC drill file
//
int WriteDrillFile( CStdioFile * file, CPartList * pl, CNetList * nl )
{
	CArray<int,int> diameter;
	diameter.SetSize(0);

	// first, find all hole diameters for parts
	if( pl )
	{
		// iterate through all parts
		cpart * part = pl->m_start.next;
		while( part->next != 0 )
		{
			CShape * s = part->shape;
			if( s )
			{
				// get all pins
				for( int ip=0; ip<s->GetNumPins(); ip++ )
				{
					padstack * ps = &s->m_padstack[ip];
					if( ps->hole_size )
						::AddToArray( ps->hole_size/NM_PER_MIL, &diameter );
				}
			}
			// go to next part
			part = part->next;
		}
	}
	// now find hole diameters for vias
	if( nl )
	{
		// iterate through all nets
		// traverse map
		POSITION pos;
		CString name;
		void * ptr;
		for( pos = nl->m_map.GetStartPosition(); pos != NULL; )
		{
			nl->m_map.GetNextAssoc( pos, name, ptr );
			cnet * net = (cnet*)ptr;
			for( int ic=0; ic<net->nconnects; ic++ )
			{
				int nsegs = net->connect[ic].nsegs;
				for( int is=0; is<nsegs; is++ )
				{
					cvertex * v = &(net->connect[ic].vtx[is+1]);
					if( v->via_w )
					{
						// via
						int w = v->via_w;
						int h_w = v->via_hole_w;
						if( w && h_w )
							::AddToArray( h_w/NM_PER_MIL, &diameter );
					}
				}
			}
		}
	}

	// now, write data to file
	CString str;
	for( int id=0; id<diameter.GetSize(); id++ )
	{
		str.Format( ";Holesize %d = %6.1f PLATED MILS\n", 
			id+1, (double)diameter[id] );
		file->WriteString( str );
	}
	file->WriteString( "M48\n" );	// start header
	file->WriteString( "INCH\n" );
	for( int id=0; id<diameter.GetSize(); id++ )
	{
		// write hole sizes
		int d = diameter[id];
		str.Format( "T%02dC%5.3f\n", id+1, (double)diameter[id]/1000.0 ); 
		file->WriteString( str );
	}
	file->WriteString( "%\n" );		// start data
	file->WriteString( "G05\n" );	// drill mode
	file->WriteString( "G90\n" );	// absolute data
	for( int id=0; id<diameter.GetSize(); id++ )
	{
		// now write hole size and all holes
		int d = diameter[id];
		str.Format( "T%02d\n", id+1 ); 
		file->WriteString( str );
		if( pl )
		{
			// iterate through all parts
			cpart * part = pl->m_start.next;
			while( part->next != 0 )
			{
				CShape * s = part->shape;
				if( s )
				{
					// get all pins
					for( int ip=0; ip<s->GetNumPins(); ip++ )
					{
						padstack * ps = &s->m_padstack[ip];
						if( ps->hole_size )
						{
							part_pin * p = &part->pin[ip];
							if( d == ps->hole_size/NM_PER_MIL )
							{
//								str.Format( "X%06dY%06d\n", p.x/(NM_PER_MIL/10), p.y/(NM_PER_MIL/10) );
								str.Format( "X%.6dY%.6d\n", p->x/(NM_PER_MIL/10), p->y/(NM_PER_MIL/10) );
								file->WriteString( str );
							}
						}
					}
				}
				// go to next part
				part = part->next;
			}
		}
		// now find hole diameters for vias
		if( nl )
		{
			// iterate through all nets
			// traverse map
			POSITION pos;
			CString name;
			void * ptr;
			for( pos = nl->m_map.GetStartPosition(); pos != NULL; )
			{
				nl->m_map.GetNextAssoc( pos, name, ptr );
				cnet * net = (cnet*)ptr;
				for( int ic=0; ic<net->nconnects; ic++ )
				{
					int nsegs = net->connect[ic].nsegs;
					for( int is=0; is<nsegs; is++ )
					{
						cvertex * v = &(net->connect[ic].vtx[is+1]);
						if( v->via_w )
						{
							// via
							int h_w = v->via_hole_w;
							if( h_w )
							{
								if( d == h_w/NM_PER_MIL )
								{
//									str.Format( "X%06dY%06d\n", v->x/(NM_PER_MIL/10), v->y/(NM_PER_MIL/10) );
									str.Format( "X%.6dY%.6d\n", v->x/(NM_PER_MIL/10), v->y/(NM_PER_MIL/10) );
									file->WriteString( str );
								}
							}
						}
					}
				}
			}
		}
	}
	file->WriteString( "M30\n" );	// program end
	return 0;
}

