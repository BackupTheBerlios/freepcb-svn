// Shape.cpp : implementation of CShape class
//
#include "stdafx.h"
#include "afx.h"
#include "SMFontUtil.h"
#include <math.h>

#define NO_MM	// this restores backward compatibility for project files

// utility function make strings for dimensions
//
CString ws( int n, int units )
{
	CString str;
	MakeCStringFromDimension( &str, n, units, FALSE, FALSE, FALSE, 6 ); 
	return str;
}
 

// constructors

// this constructor creates an empty shape
//
CShape::CShape()
{
	m_tl = new CTextList;	
	Clear();
}

// destructor
//
CShape::~CShape()
{
	Clear();
	delete m_tl;
}

void CShape::Clear()
{
	m_name = "EMPTY_SHAPE";
	m_author = "";
	m_source = "";
	m_desc = "";
	m_units = MIL;
	m_sel_xi = m_sel_yi = 0;
	m_sel_xf = m_sel_yf = 500*NM_PER_MIL;
	m_ref_size = 100*NM_PER_MIL;
	m_ref_xi = 100*NM_PER_MIL;
	m_ref_yi = 200*NM_PER_MIL;
	m_ref_angle = 0;
	m_ref_w = 10*NM_PER_MIL;
	m_padstack.SetSize(0);
	m_outline_poly.SetSize(0);
	m_tl->RemoveAllTexts();
}

// function to create shape from definition string
// returns 0 if successful
//
int CShape::MakeFromString( CString name, CString str )
{
	enum {	
		MAX_PARAMS = 40,
		MAX_CHARS_PER_PARAM = 20
	};

	enum { P_UNDEFINED = -1 };

	// packages
	enum {
		HOLE,
		DIP,
		SIP,
		SOIC,
		PLCC,
		QFP,
		HDR,
		CHIP
	};

	// pin patterns
	enum {
		EDGE,
		GRID
	};

	// pin 1 position for EDGE patterns
	enum {
		P1BL,	// pin 1 at bottom-left corner
		P1BR,	// pin 1 at bottom-right corner	
		P1TL,	// pin 1 at top-left corner
		P1TR,	// pin 1 at top-right corner
		P1TC,	// pin 1 at top center
		P1LC,	// pin 1 at left center
		P1BC,	// pin 1 at bottom center
		P1RC	// pin 1 at right center
	};

	// pin numbering direction
	enum {
		// for EDGE pattern
		CCW,	// number pins counter-clockwise around edge of part
		CW,		// number pins clockwise around edge
		// for GRID pattern
		LRBT,	// number pins left->right, bottom->top
		LRTB,	// number pins left->right, top->bottom
		RLBT,	// number pins right->left, bottom->top
		RLTB,	// number pins right->left, top->bottom
		BTLR,	// number pins bottom->top, left->right
		TBLR,	// number pins top->bottom, left->right
		BTRL,	// number pins bottom->top, right->left
		TBRL 	// number pins top->bottom, right->left
	};

	// through-hole or SMT pads
	enum {
		TH,		// through-hole pads
		SMT		// surface-mount pads
	};

	// pad shape
	enum {
		NONE,
		RNDSQ1,		// pin 1 square, others round
		ROUND,		// round
		SQUARE,		// square
		RECT,		// rectangular
		RRECT,		// rounded-rectangle
		OVAL,		// oval,
		OCTAGON		// octagonal
	};

	// delete original shape info
	Clear();

	char param[MAX_PARAMS][MAX_CHARS_PER_PARAM];
	char * pstr;
	int np = 0;

	// parse string, separator is "_"
	pstr = mystrtok( (char*)LPCSTR(str) , "_" );
	while( pstr )
	{
		if( strlen( pstr ) < MAX_CHARS_PER_PARAM )
			strcpy( param[np++], pstr );
		else 
			return 1;
		if( np >= MAX_PARAMS )
			return 1;
		pstr = mystrtok( NULL , "_" );
	}

	// now parse parameters
	// defaults
	int package = P_UNDEFINED;			// package type, must be first param
	int npins = P_UNDEFINED;			// number of pins, must be second param
	int pattern = P_UNDEFINED;			// EDGE of GRID	
	int pin_dir = P_UNDEFINED;			// CW,CCW,RLTB, etc.
	int pin_start = P_UNDEFINED;		// (for EDGE only) P1BL, etc.	
	int npins_x = P_UNDEFINED;			// length of horizontal pin rows, or hor. grid size		
	int npins_y = P_UNDEFINED;			// length of vertical pin rows, or vert. grid size	
	int pin_spacing_x = P_UNDEFINED;	// spacing between pins in hor. rows	
	int pin_spacing_y = P_UNDEFINED;	// spacing between pins in vert. rows
	int offset_right = P_UNDEFINED;		// offset from part origin to right vert. row	
	int offset_top = P_UNDEFINED;		// offset from origin to top hor. row
	int offset_left = P_UNDEFINED;		// offset from origin to left vert. row
	int offset_bottom = P_UNDEFINED;	// offset from origin to bottom hor. row
	int pad_w = P_UNDEFINED;			// pad width
	int pad_len_int = P_UNDEFINED;		// pad length external to pin
	int pad_len_ext = P_UNDEFINED;		// pad length internal to pin
	int pad_radius = P_UNDEFINED;		// pad corner radius
	int ref_text_size = P_UNDEFINED;	// ref. text character height
	int pad_shape = P_UNDEFINED;		// pad shape (RNDSQ1, ROUND, RECT)
	int pad_hole_w = P_UNDEFINED;		// pad hole diameter, or 0 for SMT pad
	int units = MIL;
	
	// now loop through all substrings
	for( int ip=0; ip<np; ip++ )
	{	
		CString p = param[ip];	// param 
		CString p1 = "";		// param minus left-most char
		CString p2 = "";		// param minus left-most 2 characters
		int len = p.GetLength();
		if( len > 0 )
			p1 = p.Right( len-1 );
		if( len > 1 )
			p2 = p.Right( len-2 );

		// package type, must be first param
		if( ip == 0 )
		{
			if( p == "HOLE" )
			{
				package = HOLE;
				pattern = GRID;
				npins = 1;
				npins_x = 1;
				npins_y = 1;
				pin_start = P1BL;
				pin_dir = LRTB;
				pin_spacing_x = PCBU_PER_MIL*100;
				pin_spacing_y = PCBU_PER_MIL*100;
				pad_shape = NONE;
				pad_hole_w = 100*PCBU_PER_MIL;
			}
			else if( p == "DIP" )
			{
				package = DIP;
				pattern = EDGE;
				pin_start = P1BL;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*100;
				pad_shape = RNDSQ1;
				pad_hole_w = 24*PCBU_PER_MIL;
			}
			else if( p == "SIP" )
			{
				package = SIP;
				pattern = EDGE;
				pin_start = P1BL;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*100;
				pad_shape = RNDSQ1;
				pad_hole_w = 24*PCBU_PER_MIL;
			}
			else if( p == "SOIC" )
			{
				package = SOIC;
				pattern = EDGE;
				pin_start = P1BL;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*50;
				pad_shape = RECT;
				pad_hole_w = 0;
			}
			else if( p == "PLCC" )
			{
				package = PLCC;
				pattern = EDGE;
				pin_start = P1TC;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*50;
				pad_shape = RECT;
				pad_hole_w = 0;
			}
			else if( p == "QFP" )
			{
				package = QFP;
				pattern = EDGE;
				pin_start = P1TL;
				pin_dir = CCW;
				pin_spacing_x = PCBU_PER_MIL*50;
				pad_shape = RECT;
				pad_hole_w = 0;
			}
			else if( p == "HDR" )
			{
				package = HDR;
				pattern = GRID;
				pin_start = P1BL;
				pin_dir = LRTB;
				pin_spacing_x = PCBU_PER_MIL*100;
				pin_spacing_y = PCBU_PER_MIL*100;
				pad_shape = RNDSQ1;
				pad_hole_w = PCBU_PER_MIL*24;
			}
			else if( p == "CHIP" )
			{
				package = CHIP;
				pattern = GRID;
				pin_start = P1BL;
				pin_dir = LRTB;
				pin_spacing_x = PCBU_PER_MIL*100;
				pin_spacing_y = PCBU_PER_MIL*100;
				pad_shape = RECT;
				pad_hole_w = 0;
			}
			else
				return 1;
			pad_w = pin_spacing_x/2;
		}
		
		// number of pins, must be second param
		else if( ip == 1 )
		{
			if( p[0] >= '0' && p[0] <= '9' )
			{
				npins = atoi( p );
				if( npins<0 || npins>10000 )
					return 1;					// out of range
				if( package == DIP || package == SOIC )
				{
					npins_x = npins/2;
					npins_y = 0;	// not used for DIP or SOIC
				}
				else if( package == PLCC || package == QFP )
				{
					npins_x = npins/4;
					npins_y = npins/4;
				}
				else if( package == HDR || package == CHIP )
				{
					if( npins == 1 )
					{
						npins_x = 1;
						npins_y = 1;
					}
					else
					{
						npins_x = npins/2;
						npins_y = 2;
					}
				}
				else if( package == HOLE )
				{
					npins = 1;
					npins_x = 1;
					npins_y = 1;
				}
				else if( package == SIP )
				{
					npins_x = npins;
					npins_y = 1;
				}
			}
			else
				return 1;
		}

		// units
		else if( p[0] == 'U' )
		{
			if( p1 == "MM" )
				units = MM;
			else if( p1 == "MIL" )
				units = MIL;
			else if( p1 == "NM" )
				units = NM;
		}
		
		// pins on edge of part or in grid pattern
		else if( p == "GRID" )
			pattern = GRID;
		else if( p == "EDGE" )
			pattern = EDGE;

		// # pins horizontal (ignore if DIP, SOIC )
		else if( p[0] == 'H' && p[1]>='0' && p[1]<='9' )
		{
			npins_x = atoi( p1 );
			if( package == PLCC || package == QFP )
			{
				npins_y = (npins-2*npins_x)/2;
			}
			else if( package == HDR || package == CHIP )
			{
				npins_y = npins/npins_x;
			}
		}

		// starting corner
		else if( p == "P1BL" )
			pin_start = P1BL;
		else if( p == "P1BR" )
			pin_start = P1BR;
		else if( p == "P1TR" )
			pin_start = P1TR;
		else if( p == "P1TL" )
			pin_start = P1TL;
		else if( p == "P1TC" )
			pin_start = P1TC;
		else if( p == "P1BC" )
			pin_start = P1BC;
		else if( p == "P1LC" )
			pin_start = P1LC;
		else if( p == "P1RC" )
			pin_start = P1RC;
		else if( p == "P1BL" ) 
			pin_start = P1BL;	// default
		
		// direction to increment pins (0=CCW, 1=CW)
		else if( p == "CCW" )
			pin_dir = CCW;
		else if( p == "CW" )
			pin_dir = CW;
		else if( p == "CW" )
			pin_dir = CW;
		else if( p == "LRBT" )
			pin_dir = LRBT;
		else if( p == "LRTB" )
			pin_dir = LRTB;
		else if( p == "RLBT" )
			pin_dir = RLBT;
		else if( p == "RLTB" )
			pin_dir = RLTB;
		else if( p == "BTLR" )
			pin_dir = BTLR;
		else if( p == "TBLR" )
			pin_dir = TBLR;
		else if( p == "BTRL" )
			pin_dir = BTRL;
		else if( p == "TBRL" )
			pin_dir = TBRL;

		// horizontal pin spacing
		else if( p[0] == 'P' && p[1] == 'H' )
		{
			pin_spacing_x = GetDimensionFromString( &p2 );
			if( pin_spacing_x == 0 && npins_x > 1 )
				return 1;
			if( pin_spacing_y == P_UNDEFINED )
				pin_spacing_y = pin_spacing_x;
		}
		
		// vertical pin spacing
		else if( p[0] == 'P' && p[1] == 'V' )
		{
			pin_spacing_y = GetDimensionFromString( &p2 );
			if( pin_spacing_y == 0 && npins_y > 1 )
				return 1;
		}
		
		// pin row offsets (or use defaults)
		else if( p[0] == 'R' && p[1] == 'L' )
		{
			offset_left = GetDimensionFromString( &p2 );
		}
		else if( p[0] == 'R' && p[1] == 'R' )
		{
			offset_right = GetDimensionFromString( &p2 );
			if( offset_right == 0 )
				return 1;
		}
		else if( p[0] == 'R' && p[1] == 'B' )
		{
			offset_bottom = GetDimensionFromString( &p2 );
		}
		else if( p[0] == 'R' && p[1] == 'T' )
		{
			offset_top = GetDimensionFromString( &p2 );
			if( offset_top == 0 )
				return 1;
		}
		
		// pad shape 
		else if( p == "RNDSQ1" )
			pad_shape = RNDSQ1;
		else if( p == "ROUND" )
			pad_shape = ROUND;
		else if( p == "SQUARE" )
			pad_shape = SQUARE;
		else if( p == "RECT" )
			pad_shape = RECT;
		else if( p == "RNDRECT" )
			pad_shape = RRECT;
		else if( p == "OVAL" )
			pad_shape = OVAL;
		else if( p == "OCTAGON" )
			pad_shape = OCTAGON;
		else if( p == "NONE" )
			pad_shape = NONE;
		
		// pad width
		if( p[0] == 'P' && p[1] == 'W' )
		{
			pad_w = GetDimensionFromString( &p2 );
			if( pad_w == 0 && pad_shape != NONE )
				return 1;
		}
		
		// pad lengths
		else if( p[0] == 'P' && p[1] == 'E' )
		{
			pad_len_ext = GetDimensionFromString( &p2 );
		}
		else if( p[0] == 'P' && param[ip][1] == 'I' )
		{
			pad_len_int = GetDimensionFromString( &p2 );
		}
		
		// pad radius
		else if( p[0] == 'P' && p[1] == 'R' )
		{
			pad_radius = GetDimensionFromString( &p2 );
		}
		
		// hole width
		else if( param[ip][0] == 'H' && param[ip][1] == 'W' )
		{
			pad_hole_w = GetDimensionFromString( &p2 );
		}
		
		// text size
		else if( param[ip][0] == 'T' && param[ip][1] == 'S' )
		{
			ref_text_size = GetDimensionFromString( &p2 );
			if( ref_text_size == 0 )
				return 1;
		}
	}
	
	// now set any undefined parameters using defaults
	// pin spacing y
	if( pin_spacing_y == P_UNDEFINED )
		pin_spacing_y = pin_spacing_x;
	
	// offsets for edge part rows
	if( offset_left == P_UNDEFINED )
		offset_left = 0;
	if( offset_bottom == P_UNDEFINED )
		offset_bottom = 0;
	if( offset_right == P_UNDEFINED )
		offset_right = offset_left + (npins_x-1)*pin_spacing_x;
	if( offset_top == P_UNDEFINED )
		offset_top = offset_bottom + (npins_y-1)*pin_spacing_y;
	
	// pad width
	if( pad_w == P_UNDEFINED )
		pad_w = pin_spacing_x/2;
	if( pad_len_ext == P_UNDEFINED )
		pad_len_ext = pad_w;
	if( pad_len_int == P_UNDEFINED )
		pad_len_int = pad_w;

	// text size
	if( ref_text_size == P_UNDEFINED )
		ref_text_size = 50*PCBU_PER_MIL;

#undef MAX_PARAMS
#undef MAX_CHARS_PER_PARAM

	// now generate part outline and ref text
	int pad_len = pad_w/2;
	int pad_wid = pad_w/2;
	if( pad_shape == NONE )
	{
		pad_wid = pad_hole_w/2;
		pad_len = pad_hole_w/2;
	}
	else if( pad_shape == RECT )
		pad_len = pad_len_ext;
	if( pad_wid == 0 || pad_len == 0 )
		return 1;

	// create array of padstacks
	m_padstack.SetSize( npins );
	if( pattern == EDGE )
	{
		// edge pattern, such as DIP, SOIC, PLCC, etc.
		for( ip=0; ip<npins; ip++ )
		{
			// i is the actual pin number
			int i = ip;
			if( pin_start == P1LC )
				i = ip + npins_y/2 + 1;
			else if( pin_start == P1TL )
				i = ip + npins_y;
			else if( pin_start == P1TC )
				i = ip + npins_y + npins_x/2 + 1;
			else if( pin_start == P1TR )
				i = ip + npins_y + npins_x;
			else if( pin_start == P1RC )
				i = ip + npins_y + npins_x + npins_y/2 + 1;
			else if( pin_start == P1BR )
				i = ip + 2*npins_y + npins_x;
			else if( pin_start == P1BC )
				i = ip + 2*npins_y + npins_x + npins_x/2 + 1;
			if( i >= npins )
				i = i - npins;
			if( pattern == EDGE && pin_dir == CW )
			{
				if( pin_start == P1TL || pin_start == P1TR 
					|| pin_start == P1BR || pin_start == P1BL )
					i = (npins-1) - i;
				else
					i = npins - i;
			}
			if( i >= npins )
				i = i - npins;

			// set pin name
			CString name;
			name.Format( "%d", i+1 );
			m_padstack[i].name = name; 

			// set hole width and determine TH or SMT
			int pad_type;
			if( pad_hole_w )
				pad_type = TH;
			else
				pad_type = SMT;
			m_padstack[i].hole_size = pad_hole_w;

			// now set pin positions for bottom, left, top and right rows
			// and create padstack array
			// note that pin numbers are not necessarily in sequence
			if( ip<npins_x )
			{
				// bottom row of pins, left->right
				m_padstack[i].angle = 270;
				m_padstack[i].y_rel = -offset_bottom;
				m_padstack[i].x_rel = ip*pin_spacing_x;
			}
			else if( ip>=npins_x && ip<(npins_x+npins_y) )
			{
				// right row of pins
				m_padstack[i].angle = 180;
				m_padstack[i].y_rel = (ip-npins_x)*pin_spacing_y;
				m_padstack[i].x_rel = offset_right;
			}
			else if( ip>=(npins_x+npins_y) && ip<(2*npins_x+npins_y) )
			{
				// top row of pins
				m_padstack[i].angle = 90;
				m_padstack[i].y_rel = offset_top;
				m_padstack[i].x_rel = (2*npins_x+npins_y-ip-1)*pin_spacing_x;
			}
			else
			{
				// left row of pins
				m_padstack[i].angle = 0;
				m_padstack[i].y_rel = (2*npins_x+2*npins_y-ip-1)*pin_spacing_y;
				m_padstack[i].x_rel = -offset_left;
			}
			if( pad_shape == RNDSQ1 || pad_shape == SQUARE
				|| pad_shape == ROUND || pad_shape == OCTAGON )
			{
				int type = PAD_ROUND;
				if( pad_shape == SQUARE || (pad_shape == RNDSQ1 && ip == 0) )
					type = PAD_SQUARE;
				else if( pad_shape == OCTAGON )
					type = PAD_OCTAGON;
				m_padstack[i].top.shape = type;
				m_padstack[i].top.size_h = pad_w;
				if( pad_type == SMT )
				{
					m_padstack[i].bottom.shape = PAD_NONE;
					m_padstack[i].inner.shape = PAD_NONE;
				}
				else
				{
					m_padstack[i].bottom = m_padstack[i].top;
					m_padstack[i].inner.shape = PAD_ROUND;
					m_padstack[i].inner.size_h = pad_w;
				}
			}
			else if( pad_shape == ROUND || pad_shape == RNDSQ1 )
			{
				m_padstack[i].top.shape = PAD_ROUND;
				m_padstack[i].top.size_h = pad_w;
				if( pad_type == SMT )
				{
					m_padstack[i].bottom.shape = PAD_NONE;
					m_padstack[i].inner.shape = PAD_NONE;
				}
				else
				{
					m_padstack[i].bottom = m_padstack[i].top;
					m_padstack[i].inner.shape = PAD_ROUND;
					m_padstack[i].inner.size_h = pad_w;
				}
			}
			else if( pad_shape == RECT || pad_shape == OVAL )
			{
				int type = PAD_RECT;
				if( pad_shape == OVAL )
					type = PAD_OVAL;
				if( pad_type == SMT )
				{
					m_padstack[i].top.shape = type;
					m_padstack[i].top.size_h = pad_w;
					m_padstack[i].top.size_l = pad_len_ext;
					m_padstack[i].top.size_r = pad_len_int;
					m_padstack[i].bottom.shape = PAD_NONE;
					m_padstack[i].inner.shape = PAD_NONE;
				}
				else
				{
					m_padstack[i].top.shape = type;
					m_padstack[i].top.size_h = pad_w;
					m_padstack[i].top.size_l = pad_len_ext;
					m_padstack[i].top.size_r = pad_len_int;
					m_padstack[i].bottom = m_padstack[i].top;
					m_padstack[i].inner.shape = PAD_ROUND;
					m_padstack[i].inner.size_h = min(pad_w,pad_len_ext+pad_len_int);
				}
			}
			else if( pad_shape == RRECT )
			{
				if( pad_type == SMT )
				{
					m_padstack[i].top.shape = PAD_RRECT;
					m_padstack[i].top.size_h = pad_w;
					m_padstack[i].top.size_l = pad_len_ext;
					m_padstack[i].top.size_r = pad_len_int;
					m_padstack[i].top.radius = pad_radius;
					m_padstack[i].bottom.shape = PAD_NONE;
					m_padstack[i].inner.shape = PAD_NONE;
				}
				else
				{
					m_padstack[i].top.shape = PAD_RRECT;
					m_padstack[i].top.size_h = pad_w;
					m_padstack[i].top.size_l = pad_len_ext;
					m_padstack[i].top.size_r = pad_len_int;
					m_padstack[i].top.radius = pad_radius;
					m_padstack[i].bottom = m_padstack[i].top;
					m_padstack[i].inner.shape = PAD_ROUND;
					m_padstack[i].inner.size_h = min(pad_w,pad_len_ext+pad_len_int);
				}
			}
			else if( pad_shape == NONE )
			{
				if( pad_type == SMT )
				{
					m_padstack[i].top.shape = PAD_NONE;
					m_padstack[i].top.size_h = 0;
					m_padstack[i].top.size_l = 0;
					m_padstack[i].top.size_r = 0;
					m_padstack[i].bottom.shape = PAD_NONE;
					m_padstack[i].inner.shape = PAD_NONE;
				}
				else
				{
					m_padstack[i].top.shape = PAD_NONE;
					m_padstack[i].top.size_h = 0;
					m_padstack[i].top.size_l = 0;
					m_padstack[i].top.size_r = 0;
					m_padstack[i].inner = m_padstack[i].top;
					m_padstack[i].bottom = m_padstack[i].top;
				}
			}
		}
	}
	else if( pattern == GRID )
	{
		// grid pattern, such as HDR
		int pad_type;
		if( pad_hole_w )
			pad_type = TH;
		else
			pad_type = SMT;
		for( int ih=0; ih<npins_x; ih++ )
		{
			for( int iv=0; iv<npins_y; iv++ )
			{
				// get actual pin number
				int ip;
				if( pin_dir == LRBT )
					ip = ih + iv*npins_x; 
				else if( pin_dir == LRTB )
					ip = ih + (npins_y-iv-1)*npins_x; 
				else if( pin_dir == RLBT )
					ip = (npins_x-ih-1) + iv*npins_x; 
				else if( pin_dir == RLTB )
					ip = (npins_x-ih-1) + (npins_y-iv-1)*npins_x; 
				else if( pin_dir == BTLR )
					ip = iv + ih*npins_y; 
				else if( pin_dir == BTRL )
					ip = iv + (npins_x-ih-1)*npins_y; 
				else if( pin_dir == TBLR )
					ip = (npins_y-iv-1) + ih*npins_y; 
				else if( pin_dir == TBRL )
					ip = (npins_y-iv-1) + (npins_x-ih-1)*npins_y; 
				// if RECT pads are used, assume that there are 2 rows
				if( pad_shape == RECT )
				{
					if( npins_x == 2 )
					{
						if( ih == 0 )
							m_padstack[ip].angle = 0;
						else
							m_padstack[ip].angle = 180;
					}
					else if( npins_y == 2 )
					{
						if( iv == 0 )
							m_padstack[ip].angle = 270;
						else
							m_padstack[ip].angle = 90;
					}
				}
				else
					m_padstack[ip].angle = 0;
				m_padstack[ip].x_rel = ih * pin_spacing_x;
				m_padstack[ip].y_rel = iv * pin_spacing_y;
				m_padstack[ip].hole_size = pad_hole_w;
				if( pad_shape == RNDSQ1 || pad_shape == ROUND || pad_shape == SQUARE || pad_shape == OCTAGON )
				{
					if( (ip==0 && pad_shape == RNDSQ1) || pad_shape == SQUARE )
						m_padstack[ip].top.shape = PAD_SQUARE;
					else if( pad_shape == OCTAGON )
						m_padstack[ip].top.shape = PAD_OCTAGON;
					else
						m_padstack[ip].top.shape = PAD_ROUND;
					m_padstack[ip].top.size_h = pad_w;
					if( pad_type == SMT )
					{
						m_padstack[ip].bottom.shape = PAD_NONE;
						m_padstack[ip].inner.shape = PAD_NONE;
					}
					else
					{
						m_padstack[ip].bottom = m_padstack[ip].top;
						m_padstack[ip].inner.shape = PAD_NONE;
//						m_padstack[ip].inner.size_h = pad_w;
					}
				}
				else if( pad_shape == RECT || pad_shape == OVAL )
				{
					if( pad_shape == RECT )
						m_padstack[ip].top.shape = PAD_RECT;
					else
						m_padstack[ip].top.shape = PAD_OVAL;
					m_padstack[ip].top.size_h = pad_w;
					m_padstack[ip].top.size_l = pad_len_ext;
					m_padstack[ip].top.size_r = pad_len_int;
					if( pad_type == SMT )
					{
						m_padstack[ip].bottom.shape = PAD_NONE;
						m_padstack[ip].inner.shape = PAD_NONE;
					}
					else
					{
						m_padstack[ip].bottom = m_padstack[ip].top;
						m_padstack[ip].inner.shape = PAD_NONE;
//						m_padstack[ip].inner.size_h = pad_w;
					}
				}
				else if( pad_shape == RRECT )
				{
					m_padstack[ip].top.shape = PAD_RRECT;
					m_padstack[ip].top.size_h = pad_w;
					m_padstack[ip].top.size_l = pad_len_ext;
					m_padstack[ip].top.size_r = pad_len_int;
					m_padstack[ip].top.radius = pad_radius;
					if( pad_type == SMT )
					{
						m_padstack[ip].bottom.shape = PAD_NONE;
						m_padstack[ip].inner.shape = PAD_NONE;
					}
					else
					{
						m_padstack[ip].bottom = m_padstack[ip].top;
						m_padstack[ip].inner.shape = PAD_NONE;
//						m_padstack[ip].inner.size_h = pad_w;
					}
				}
			}
		}
		// now adjust offsets for pin 1 position, which may not be 0,0
		for( int ip=1; ip<npins; ip++ )
		{
			m_padstack[ip].x_rel = m_padstack[ip].x_rel - m_padstack[0].x_rel;
			m_padstack[ip].y_rel = m_padstack[ip].y_rel - m_padstack[0].y_rel;
		}
		for( int ip=0; ip<m_outline_poly.GetSize(); ip++ )
		{
			for( int ic=0; ic<m_outline_poly[0].GetNumCorners(); ic++ )
			{
				m_outline_poly[ip].SetX( ic, m_outline_poly[ip].GetX(ic) - m_padstack[0].x_rel );
				m_outline_poly[ip].SetY( ic, m_outline_poly[ip].GetY(ic) - m_padstack[0].y_rel );
			}
		}
		m_ref_xi -= m_padstack[0].x_rel;
		m_ref_yi -= m_padstack[0].y_rel;
		m_padstack[0].x_rel = 0;
		m_padstack[0].y_rel = 0;
	}
	else
		ASSERT(0);	// no pattern

	CRect pad_rect = GetBounds();
	m_outline_poly.SetSize(1);
	if( package == HOLE )
	{
		// create ref text box
		m_ref_size = ref_text_size;
		m_ref_xi = pin_spacing_x;
		m_ref_yi = pin_spacing_x;
		m_ref_angle = 0;
		m_ref_w = m_ref_size/10;
	}
	else if( package == DIP || package == SOIC )
	{
		// create ref text box
		m_ref_size = ref_text_size;
		m_ref_xi = pin_spacing_x;
		m_ref_yi = pin_spacing_x;
		m_ref_angle = 0;
		m_ref_w = m_ref_size/10;

		// create part outline
		m_outline_poly[0].Start( 0, 7*PCBU_PER_MIL, 7*PCBU_PER_MIL, 0, pad_len_int+offset_top/12, 0, 0, 0 );
		m_outline_poly[0].AppendCorner( (npins_x-1)*pin_spacing_x, pad_len_int+offset_top/12 );
		m_outline_poly[0].AppendCorner( (npins_x-1)*pin_spacing_x, 11*offset_top/12-pad_len_int );
		m_outline_poly[0].AppendCorner( 0, 11*offset_top/12-pad_len_int );
		m_outline_poly[0].AppendCorner( 0, 7*offset_top/12 );
		m_outline_poly[0].AppendCorner( pin_spacing_x/4, 7*offset_top/12 );
		m_outline_poly[0].AppendCorner( pin_spacing_x/4, 5*offset_top/12 );
		m_outline_poly[0].AppendCorner( 0, 5*offset_top/12 );
		m_outline_poly[0].Close();
	}
	else if( package == PLCC || package == QFP )
	{
		// create ref text box
		m_ref_size = ref_text_size;
		m_ref_xi = offset_right/4;
		m_ref_yi = offset_top/2;
		m_ref_angle = 0;
		m_ref_w = m_ref_size/10;

		// create part outline
		int xf = (npins_x-1)*pin_spacing_x;
		int yf = (npins_y-1)*pin_spacing_y;
		int corner = pin_spacing_x;
		int notch = pin_spacing_x/2;
		int notch_w = pin_spacing_x/4;
		if( pin_start == P1BL )
		{
			// notch bottom-left corner
			m_outline_poly[0].Start( 0, 7*PCBU_PER_MIL, 0, 0, corner, 0, 0, 0 );
			m_outline_poly[0].AppendCorner( corner, 0 );
		}
		else
			m_outline_poly[0].Start( 0, 7*PCBU_PER_MIL, 0, 0, 0, 0, 0, 0 );
		if( pin_start == P1BC )
		{
			// notch bottom side
			m_outline_poly[0].AppendCorner( xf/2-notch_w, 0 );
			m_outline_poly[0].AppendCorner( xf/2-notch_w, notch );
			m_outline_poly[0].AppendCorner( xf/2+notch_w, notch );
			m_outline_poly[0].AppendCorner( xf/2+notch_w, 0 );
		}
		if( pin_start == P1BR )
		{
			// notch bottom-right corner
			m_outline_poly[0].AppendCorner( xf-corner, 0 );
			m_outline_poly[0].AppendCorner( xf, corner );
		}
		else
			m_outline_poly[0].AppendCorner( xf, 0 );
		if( pin_start == P1RC )
		{
			// notch right side
			m_outline_poly[0].AppendCorner( xf,		yf/2-notch_w ); 
			m_outline_poly[0].AppendCorner( xf-notch, yf/2-notch_w ); 
			m_outline_poly[0].AppendCorner( xf-notch, yf/2+notch_w ); 
			m_outline_poly[0].AppendCorner( xf,		yf/2+notch_w ); 
		}
		if( pin_start == P1TR )
		{
			// notch top-right corner
			m_outline_poly[0].AppendCorner( xf, yf-corner ); 
			m_outline_poly[0].AppendCorner( xf-corner, yf ); 
		}
		else
			m_outline_poly[0].AppendCorner( xf, yf ); 
		if( pin_start == P1TC )
		{
			// notch top side
			m_outline_poly[0].AppendCorner( xf/2+notch_w, yf ); 
			m_outline_poly[0].AppendCorner( xf/2+notch_w, yf-notch ); 
			m_outline_poly[0].AppendCorner( xf/2-notch_w, yf-notch ); 
			m_outline_poly[0].AppendCorner( xf/2-notch_w, yf ); 
		}
		if( pin_start == P1TL )
		{
			// notch top-left corner
			m_outline_poly[0].AppendCorner( 0+corner, yf ); 
			m_outline_poly[0].AppendCorner( 0, yf-corner ); 
		}
		else
			m_outline_poly[0].AppendCorner( 0, yf); 
		if( pin_start == P1LC )
		{
			// notch left side
			m_outline_poly[0].AppendCorner( 0,		yf/2+notch_w );
			m_outline_poly[0].AppendCorner( notch,	yf/2+notch_w );
			m_outline_poly[0].AppendCorner( notch,	yf/2-notch_w );
			m_outline_poly[0].AppendCorner( 0,		yf/2-notch_w );
		}
		m_outline_poly[0].Close();
	}
	else if( package == HDR || package == CHIP || package == SIP )
	{
		// create ref text box
		m_ref_size = ref_text_size;
		m_ref_xi = offset_right/4;
		m_ref_yi = offset_top/2;
		m_ref_angle = 0;
		m_ref_w = m_ref_size/10;

		// create part outline with 7 mil line width and 13 mil clearance
		int clear = 13*PCBU_PER_MIL;
		m_outline_poly[0].Start( 0, 7*PCBU_PER_MIL, 0, pad_rect.left-clear, pad_rect.bottom-clear, 0, 0, 0 ); 
		m_outline_poly[0].AppendCorner( pad_rect.right+clear, pad_rect.bottom-clear ); 
		m_outline_poly[0].AppendCorner( pad_rect.right+clear, pad_rect.top+clear ); 
		m_outline_poly[0].AppendCorner( pad_rect.left-clear, pad_rect.top+clear ); 
		m_outline_poly[0].Close();
	}

	// now set selection rectangle
	CRect br = GetBounds();
	m_sel_xi = br.left - 10*NM_PER_MIL;
	m_sel_yi = br.bottom - 10*NM_PER_MIL;
	m_sel_xf = br.right + 10*NM_PER_MIL;
	m_sel_yf = br.top + 10*NM_PER_MIL;

	// now set pin names
	for( int ip=0; ip<m_padstack.GetSize(); ip++ )
	{
		CString str;
		str.Format( "%d", ip+1 );
		m_padstack[ip].name = str;
	}

	m_name = name.Left(MAX_NAME_SIZE);	// this is the limit
	m_units = units;
	return 0;
}

// MakeFromFile() ... create a footprint from a description in a text file
//
// enter with:
//	file = pointer to an open CStdioFile, or NULL if file not already open
//	file_path = full pathname to file if not already open
//	pos = position in file if not already open (i.e. position of "name: xxx" line)
//	name = name of footprint (if known), or "" if unknown
//
// returns 0 if successful
// returns 1 if unable to open file
// returns 2 if file i/o error
// returns 3 if unable to parse file
// returns 4 if name doesn't match (if name is known)
//
int CShape::MakeFromFile( CStdioFile * in_file, CString name, CString file_path, int pos )
{
	CString key_str;
	int err;
	CArray<CString> p;
	int ipin, file_pos;
	int num_pins;

	p.SetSize( 10 );

	// if in_file exists, use it, otherwise open file
	CStdioFile * file;
	if( !in_file )
	{
		file = new CStdioFile;
		err = file->Open( file_path, CFile::modeRead );
		if( !err )
			return 1;
	}
	else
		file = in_file;

	// delete any original shape info
	Clear();
	m_units = NM;
	int mult = 1;

	// now read lines from file and make footprint
	try
	{
		if( !in_file )
			file->Seek( pos, CFile::begin );	// set starting position
		int line_num = 0;
		CString name_str;
		int ok = file->ReadString( name_str );	// get first line from file
		if( !ok )
		{
			if( !in_file )
				delete file;
			return 2;
		}
		int np = ParseKeyString( &name_str, &key_str, &p );	// parse it
		if( np != 2 || key_str != "name" )
		{
			if( !in_file )
				delete file;
			return 4;	// first line should be "name: xxx"
		}
		if( name != "" && p[0] != name )
		{
			if( !in_file )
				delete file;
			return 4;	// if name defined, it should match
		}
		if( p[0].GetLength() > MAX_NAME_SIZE )
		{
			CString mess;
			mess.Format( "Footprint name \"%s\" too long\nTruncated to \"%s\"",
				p[0], p[0].Left(MAX_NAME_SIZE) );
			AfxMessageBox( mess );
		}
		m_name = p[0].Left(MAX_NAME_SIZE);

		// now get the rest of the lines from file
		while( 1 )
		{
			CString instr;
			file_pos = file->GetPosition();		// remember position
			int err = file->ReadString( instr );
			if( !err )
			{
				// EOF, this is not an error
				if( !in_file )
					delete file;
				if( m_units == NM )
					m_units = MM;
				return 0;
			}
			np = ParseKeyString( &instr, &key_str, &p );	// parse line
			if( np == 0 )
				continue;	// blank line or comment, skip it
			if( key_str == "end" )
			{
				// end of shape definition
				if( !in_file )
					delete file;
				break;	
			}
			if( key_str == "name" || key_str[0] == '[' )
			{
				// beginning of next shape or end of shapes section
				file->Seek( file_pos, CFile::begin );	// back up
				if( !in_file )
					delete file;
				break;	
			}
			if( key_str == "str" && np == 2 )
			{
				// keyword = "str", make footprint from string
				int err = MakeFromString( name, p[0] );
				if( !in_file )
					delete file;
				if( err )
					return 3;
				else
				{
					if( m_units == NM )
						m_units = MM;
					return 0;
				}
			}
			if( key_str == "author" && np == 2 )
			{
				m_author = p[0];
			}
			if( key_str == "source" && np == 2 )
			{
				m_source = p[0];
			}
			if( key_str == "description" && np == 2 )
			{
				m_desc = p[0];
			}
			if( key_str == "units" && np == 2 )
			{
				if( p[0] == "MIL" )
				{
					m_units = MIL;
					mult = 25400;
				}
				else if( p[0] == "MM" )
				{
					m_units = MM;
					mult = 1000000;
				}
				else if( p[0] == "NM" )
				{
					m_units = NM;
					mult = 1;
				}
			}
			if( key_str == "sel_rect" && np == 5 )
			{
				m_sel_xi = GetDimensionFromString( &p[0], m_units );
				m_sel_yi = GetDimensionFromString( &p[1], m_units);
				m_sel_xf = GetDimensionFromString( &p[2], m_units);
				m_sel_yf = GetDimensionFromString( &p[3], m_units);
			}
			else if( key_str == "ref_text" && np == 6 )
			{
				m_ref_size = GetDimensionFromString( &p[0], m_units);
				m_ref_xi = GetDimensionFromString( &p[1], m_units);
				m_ref_yi = GetDimensionFromString( &p[2], m_units);
				m_ref_angle = my_atoi( &p[3] ); 
				m_ref_w = GetDimensionFromString( &p[4], m_units);
			}
			else if( key_str == "text" && np == 7 )
			{
				int font_size = GetDimensionFromString( &p[1], m_units);
				int x = GetDimensionFromString( &p[2], m_units);
				int y = GetDimensionFromString( &p[3], m_units);
				int angle = my_atoi( &p[4] ); 
				int stroke_w = GetDimensionFromString( &p[5], m_units);
				m_tl->AddText( x, y, angle, 0, LAY_FP_SILK_TOP, font_size, stroke_w, &p[0] );
			}
			else if( key_str == "text" && np == 9 )
			{
				int font_size = GetDimensionFromString( &p[1], m_units);
				int x = GetDimensionFromString( &p[2], m_units);
				int y = GetDimensionFromString( &p[3], m_units);
				int angle = my_atoi( &p[4] ); 
				int stroke_w = GetDimensionFromString( &p[5], m_units);
				int mirror = my_atoi( &p[6] );
				int layer = my_atoi( &p[7] );
				m_tl->AddText( x, y, angle, mirror, layer, font_size, stroke_w, &p[0] );
			}
			else if( key_str == "outline_polygon" || key_str == "outline_polyline" )
			{
				int w = GetDimensionFromString( &p[0], m_units);
				int x = GetDimensionFromString( &p[1], m_units);
				int y = GetDimensionFromString( &p[2], m_units);
				int np = m_outline_poly.GetSize();
				m_outline_poly.SetSize(np+1);
				m_outline_poly[np].Start( 0, w, 0, x, y, 0, NULL, NULL );
			}
			else if( key_str == "next_corner" && np == 3 )
			{
				int x = GetDimensionFromString( &p[0], m_units);
				int y = GetDimensionFromString( &p[1], m_units);
				int np = m_outline_poly.GetSize();
				m_outline_poly[np-1].AppendCorner( x, y );
			}
			else if( key_str == "next_corner" && np == 4 )
			{
				int x = GetDimensionFromString( &p[0], m_units);
				int y = GetDimensionFromString( &p[1], m_units);
				int style = my_atoi( &p[2] );
				int np = m_outline_poly.GetSize();
				m_outline_poly[np-1].AppendCorner( x, y, style );
			}
			else if( key_str == "close_polyline" && np == 1 )
			{
				int np = m_outline_poly.GetSize();
				m_outline_poly[np-1].Close();
			}
			else if( key_str == "close_polyline" && np == 2 )
			{
				int style = my_atoi( &p[0] );
				int np = m_outline_poly.GetSize();
				m_outline_poly[np-1].Close( style );
			}
			else if( key_str == "n_pins" && np == 2 )
			{
				num_pins = my_atoi( &p[0] );
				m_padstack.SetSize( 0 );
			}
			else if( key_str == "pin" && np == 6 )
			{
				CString pin_name = p[0];
				if( pin_name.GetLength() > MAX_PIN_NAME_SIZE )
				{
					CString mess;
					mess.Format( "Footprint \"%s\": pin name \"%s\" too long\nTruncated to \"%s\"",
						m_name, pin_name, pin_name.Left(MAX_PIN_NAME_SIZE) );
					AfxMessageBox( mess );
					pin_name = pin_name.Left(MAX_PIN_NAME_SIZE);
				}
				ipin = m_padstack.GetSize();
				m_padstack.SetSize( ipin+1 );
				if( ipin >= num_pins )
				{
					if( !in_file )
						delete file;
					return 3;
				}
				m_padstack[ipin].name = pin_name; 
				m_padstack[ipin].hole_size = GetDimensionFromString( &p[1], m_units); 
				m_padstack[ipin].x_rel = GetDimensionFromString( &p[2], m_units); 
				m_padstack[ipin].y_rel = GetDimensionFromString( &p[3], m_units); 
				m_padstack[ipin].angle = my_atoi( &p[4] );			
			}
			else if( key_str == "top_pad" && np == 5 )
			{
				m_padstack[ipin].top.shape = my_atoi( &p[0] ); 
				m_padstack[ipin].top.size_h = GetDimensionFromString( &p[1], m_units); 
				m_padstack[ipin].top.size_l = GetDimensionFromString( &p[2], m_units); 
				m_padstack[ipin].top.size_r = GetDimensionFromString( &p[3], m_units);
				m_padstack[ipin].top.radius = 0;
			}
			else if( key_str == "top_pad" && np == 6 )
			{
				m_padstack[ipin].top.shape = my_atoi( &p[0] ); 
				m_padstack[ipin].top.size_h = GetDimensionFromString( &p[1], m_units); 
				m_padstack[ipin].top.size_l = GetDimensionFromString( &p[2], m_units); 
				m_padstack[ipin].top.size_r = GetDimensionFromString( &p[3], m_units);
				m_padstack[ipin].top.radius = GetDimensionFromString( &p[4], m_units);
			}
			else if( key_str == "inner_pad" && np == 5 )
			{
				m_padstack[ipin].inner.shape = my_atoi( &p[0] ); 
				m_padstack[ipin].inner.size_h = GetDimensionFromString( &p[1], m_units); 
				m_padstack[ipin].inner.size_l = GetDimensionFromString( &p[2], m_units); 
				m_padstack[ipin].inner.size_r = GetDimensionFromString( &p[3], m_units);
				m_padstack[ipin].inner.radius = 0;
			}
			else if( key_str == "inner_pad" && np == 6 )
			{
				m_padstack[ipin].inner.shape = my_atoi( &p[0] ); 
				m_padstack[ipin].inner.size_h = GetDimensionFromString( &p[1], m_units); 
				m_padstack[ipin].inner.size_l = GetDimensionFromString( &p[2], m_units); 
				m_padstack[ipin].inner.size_r = GetDimensionFromString( &p[3], m_units);
				m_padstack[ipin].inner.radius = GetDimensionFromString( &p[4], m_units);
			}
			else if( key_str == "bottom_pad" && np == 5 )
			{
				m_padstack[ipin].bottom.shape = my_atoi( &p[0] ); 
				m_padstack[ipin].bottom.size_h = GetDimensionFromString( &p[1], m_units); 
				m_padstack[ipin].bottom.size_l = GetDimensionFromString( &p[2], m_units); 
				m_padstack[ipin].bottom.size_r = GetDimensionFromString( &p[3], m_units);
				m_padstack[ipin].bottom.radius = 0;
			}
			else if( key_str == "bottom_pad" && np == 6 )
			{
				m_padstack[ipin].bottom.shape = my_atoi( &p[0] ); 
				m_padstack[ipin].bottom.size_h = GetDimensionFromString( &p[1], m_units); 
				m_padstack[ipin].bottom.size_l = GetDimensionFromString( &p[2], m_units); 
				m_padstack[ipin].bottom.size_r = GetDimensionFromString( &p[3], m_units);
				m_padstack[ipin].bottom.radius = GetDimensionFromString( &p[4], m_units);
			}
			else
			{
				//				AfxMessageBox( "unidentified keyword in pcb file" );
			}
			line_num++;
		}
		if( m_units == NM )
			m_units = MM;
		return 0;
	}

	catch( CFileException * e)
	{
		// file error
		if( !in_file )
			delete file;
		return 2;
	}

	catch( CString * err_str )
	{
		// parsing error
		delete err_str;
		if( !in_file )
			delete file;
		return 3;
	}
}

// copy another shape into this shape
//
int CShape::Copy( CShape * shape )
{
	m_name = shape->m_name;
	m_author = shape->m_author;
	m_source = shape->m_source;
	m_desc = shape->m_desc;
	m_units = shape->m_units;
	m_sel_xi = shape->m_sel_xi;
	m_sel_yi = shape->m_sel_yi;
	m_sel_xf = shape->m_sel_xf;
	m_sel_yf = shape->m_sel_yf;
	m_ref_size = shape->m_ref_size;
	m_ref_w = shape->m_ref_w;
	m_ref_xi = shape->m_ref_xi;
	m_ref_yi = shape->m_ref_yi;
	m_ref_angle = shape->m_ref_angle;
	// copy padstacks
	m_padstack.RemoveAll();
	int np = shape->m_padstack.GetSize();
	m_padstack.SetSize( np );
	for( int i=0; i<np; i++ )
		m_padstack[i] = shape->m_padstack[i];
	// copy outline polys
	m_outline_poly.RemoveAll();
	np = shape->m_outline_poly.GetSize();
	m_outline_poly.SetSize(np);
	for( int ip=0; ip<np; ip++ )
		m_outline_poly[ip].Copy( &shape->m_outline_poly[ip] );
	// copy text, but don't draw it
	m_tl->RemoveAllTexts();
	for( int it=0; it<shape->m_tl->text_ptr.GetSize(); it++ )
	{
		CText * t = shape->m_tl->text_ptr[it];
		m_tl->AddText( t->m_x, t->m_y, t->m_angle, t->m_mirror, LAY_FP_SILK_TOP, 
			t->m_font_size, t->m_stroke_width, &t->m_str, FALSE ); 
	}
	return PART_NOERR;
}

BOOL CShape::Compare( CShape * shape )
{
	if( m_name != shape->m_name 
		|| m_author != shape->m_author 
		|| m_source != shape->m_source 
		|| m_desc != shape->m_desc 
		|| m_sel_xi != shape->m_sel_xi 
		|| m_sel_yi != shape->m_sel_yi 
		|| m_sel_xf != shape->m_sel_xf 
		|| m_sel_yf != shape->m_sel_yf 
		|| m_ref_size != shape->m_ref_size 
		|| m_ref_w != shape->m_ref_w 
		|| m_ref_xi != shape->m_ref_xi 
		|| m_ref_yi != shape->m_ref_yi 
		|| m_ref_angle != shape->m_ref_angle )
			return FALSE;

	// padstacks
	int np = m_padstack.GetSize();
	if( np != shape->m_padstack.GetSize() )
		return FALSE;
	for( int i=0; i<np; i++ )
	{
		if(  !(m_padstack[i] == shape->m_padstack[i]) )
			return FALSE;
	}
	// outline polys
	np = m_outline_poly.GetSize();
	if( np != shape->m_outline_poly.GetSize() )
		return FALSE;
	for( int ip=0; ip<np; ip++ )
	{
		if (m_outline_poly[ip].GetLayer() != shape->m_outline_poly[ip].GetLayer() ) return FALSE;
		if (m_outline_poly[ip].GetClosed() != shape->m_outline_poly[ip].GetClosed() ) return FALSE;
		if (m_outline_poly[ip].GetHatch() != shape->m_outline_poly[ip].GetHatch() ) return FALSE;
		if (m_outline_poly[ip].GetW() != shape->m_outline_poly[ip].GetW() ) return FALSE;
//		if (m_outline_poly[ip].GetSelBoxSize() != shape->m_outline_poly[ip].GetSelBoxSize() ) return FALSE;
		if (m_outline_poly[ip].GetNumCorners() != shape->m_outline_poly[ip].GetNumCorners() ) return FALSE;
		for( int ic=0; ic<m_outline_poly[ip].GetNumCorners(); ic++ )
		{
			if (m_outline_poly[ip].GetX(ic) != shape->m_outline_poly[ip].GetX(ic) ) return FALSE;
			if (m_outline_poly[ip].GetY(ic) != shape->m_outline_poly[ip].GetY(ic) ) return FALSE;
			if( ic<(m_outline_poly[ip].GetNumCorners()-1) || m_outline_poly[ip].GetClosed() )
				if (m_outline_poly[ip].GetSideStyle(ic) != shape->m_outline_poly[ip].GetSideStyle(ic) ) return FALSE;
		}
	}
	// text
	int nt = m_tl->text_ptr.GetSize();
	if( nt != shape->m_tl->text_ptr.GetSize() )
		return FALSE;
	for( int it=0; it<m_tl->text_ptr.GetSize(); it++ )
	{
		CText * t = m_tl->text_ptr[it];
		CText * st = shape->m_tl->text_ptr[it];
		if( t->m_x != st->m_x
			|| t->m_y != st->m_y
			|| t->m_layer != st->m_layer
			|| t->m_angle != st->m_angle
			|| t->m_mirror != st->m_mirror
			|| t->m_font_size != st->m_font_size
			|| t->m_stroke_width != st->m_stroke_width
			|| t->m_str != st->m_str )
			return FALSE;
	}
	return TRUE;
}

CShape::GetNumPins()
{
	return m_padstack.GetSize();
}

int CShape::GetPinIndexByName( CString * name )
{	
	for( int ip=0; ip<m_padstack.GetSize(); ip++ )
	{
		if( m_padstack[ip].name == *name )
			return ip;
	}
	return -1;		// error
}

CString CShape::GetPinNameByIndex( int ip )
{
	return m_padstack[ip].name;
}

// write one footprint
//
int CShape::WriteFootprint( CStdioFile * file )
{
	CString line;
	CString key;
	try
	{
		line.Format( "name: \"%s\"\n", m_name.Left(MAX_NAME_SIZE) );
		file->WriteString( line );
		if( m_author != "" )
		{
			line.Format( "author: \"%s\"\n", m_author );
			file->WriteString( line );
		}
		if( m_source != "" )
		{
			line.Format( "source: \"%s\"\n", m_source );
			file->WriteString( line );
		}
		if( m_desc != "" )
		{
			line.Format( "description: \"%s\"\n", m_desc );
			file->WriteString( line );
		}
#ifdef NO_MM
		if( m_units == MM )
			m_units = NM;
#endif
		if( m_units == NM )
			file->WriteString( "  units: NM\n" );
		else if( m_units == MIL )
			file->WriteString( "  units: MIL\n" );
		else if( m_units == MM )
			file->WriteString( "  units: MM\n" );
		else
			ASSERT(0);
		line.Format( "  sel_rect: %s %s %s %s\n", 
			ws(m_sel_xi,m_units), ws(m_sel_yi,m_units), ws(m_sel_xf,m_units), ws(m_sel_yf,m_units) );
		file->WriteString( line );
		line.Format( "  ref_text: %s %s %s %d %s\n", 
			ws(m_ref_size,m_units), ws(m_ref_xi,m_units), ws(m_ref_yi,m_units), m_ref_angle, ws(m_ref_w,m_units) );
		file->WriteString( line );
		for( int it=0; it<m_tl->text_ptr.GetSize(); it++ )
		{
			CText * t = m_tl->text_ptr[it];
			line.Format( "  text: \"%s\" %s %s %s %d %s %d %d\n", t->m_str,
				ws(t->m_font_size,m_units), ws(t->m_x,m_units), ws(t->m_y,m_units), t->m_angle, 
				ws(t->m_stroke_width,m_units), t->m_mirror, t->m_layer );
				file->WriteString( line );  
		}
		for( int ip=0; ip<m_outline_poly.GetSize(); ip++ )
		{
			line.Format( "  outline_polyline: %s %s %s\n", ws(m_outline_poly[ip].GetW(),m_units),
				ws(m_outline_poly[ip].GetX(0),m_units), ws(m_outline_poly[ip].GetY(0),m_units) );
			file->WriteString( line );
			int nc = m_outline_poly[ip].GetNumCorners();
			for( int ic=1; ic<nc; ic++ )
			{
				line.Format( "    next_corner: %s %s %d\n",
					ws(m_outline_poly[ip].GetX(ic),m_units), ws(m_outline_poly[ip].GetY(ic),m_units),
					m_outline_poly[ip].GetSideStyle(ic-1) );
				file->WriteString( line );
			}
			if( m_outline_poly[ip].GetClosed() )
			{
				line.Format( "    close_polyline: %d\n", m_outline_poly[ip].GetSideStyle(nc-1) );
				file->WriteString( line );
			}
		}

		line.Format( "  n_pins: %d\n", m_padstack.GetSize() );
		file->WriteString( line );
		for( int ip=0; ip<m_padstack.GetSize(); ip++ )
		{
			padstack * p = &(m_padstack[ip]);
			line.Format( "    pin: \"%s\" %s %s %s %d\n",
				p->name, ws(p->hole_size,m_units), ws(p->x_rel,m_units), ws(p->y_rel,m_units), p->angle ); 
			file->WriteString( line );
			if( p->hole_size )
			{
				line.Format( "      top_pad: %d %s %s %s %s\n",
					p->top.shape, ws(p->top.size_h,m_units), ws(p->top.size_l,m_units), 
					ws(p->top.size_r,m_units), ws(p->top.radius,m_units) );
				file->WriteString( line );
				line.Format( "      inner_pad: %d %s %s %s %s\n",
					p->inner.shape, ws(p->inner.size_h,m_units), ws(p->inner.size_l,m_units), 
					ws(p->inner.size_r,m_units), ws(p->inner.radius,m_units) );
				file->WriteString( line );
				line.Format( "      bottom_pad: %d %s %s %s %s\n",
					p->bottom.shape, ws(p->bottom.size_h,m_units), ws(p->bottom.size_l,m_units), 
					ws(p->bottom.size_r,m_units), ws(p->bottom.radius,m_units) );
				file->WriteString( line );
			}
			else if( p->top.shape != PAD_NONE )
			{
				line.Format( "      top_pad: %d %s %s %s %s\n",
					p->top.shape, ws(p->top.size_h,m_units), ws(p->top.size_l,m_units), 
					ws(p->top.size_r,m_units), ws(p->top.radius,m_units) );
				file->WriteString( line );
			}
			else
			{
				line.Format( "      bottom_pad: %d %s %s %s %s\n",
					p->bottom.shape, ws(p->bottom.size_h,m_units), ws(p->bottom.size_l,m_units), 
					ws(p->bottom.size_r,m_units), ws(p->bottom.radius,m_units) );
				file->WriteString( line );
			}
		}
		file->WriteString( "\n" );

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

// create metafile and draw footprint into it
//
HENHMETAFILE CShape::CreateMetafile( CMetaFileDC * mfDC, CDC * pDC, int x_size, int y_size )
{
	// get bounds of shape
	int x_min = INT_MAX;
	int x_max = INT_MIN;
	int y_min = INT_MAX;
	int y_max = INT_MIN;
	// get bounds of pads
	for( int ip=0; ip<m_padstack.GetSize(); ip++ )
	{
		int x = m_padstack[ip].x_rel;
		int y = m_padstack[ip].y_rel;
		int angle = m_padstack[ip].angle;
		int hole_size = m_padstack[ip].hole_size;
		pad p = m_padstack[ip].top;
		if( m_padstack[ip].top.shape == PAD_NONE && m_padstack[ip].bottom.shape != PAD_NONE )
			p = m_padstack[ip].bottom;
		int p_x_min, p_x_max, p_y_min, p_y_max;
		if( p.shape == PAD_ROUND || p.shape == PAD_SQUARE || p.shape == PAD_OCTAGON )
		{
			p_x_min = x - p.size_h/2;
			p_x_max = x + p.size_h/2;
			p_y_min = y - p.size_h/2;
			p_y_max = y + p.size_h/2;
		}
		else if( p.shape == PAD_NONE )
		{
			p_x_min = x - hole_size/2;
			p_x_max = x + hole_size/2;
			p_y_min = y - hole_size/2;
			p_y_max = y + hole_size/2;
		}
		else if( p.shape == PAD_RECT || p.shape == PAD_RRECT || p.shape == PAD_OVAL )
		{
			CRect pr( -p.size_l, p.size_h/2, p.size_r, -p.size_h/2 );
			if( angle > 0 )
				RotateRect( &pr, angle, zero );
			p_x_min = x + pr.left;
			p_x_max = x + pr.right;
			p_y_min = y + pr.bottom;
			p_y_max = y + pr.top;
		}
		else
			ASSERT(0);
		x_min = min( x_min, p_x_min );
		x_max = max( x_max, p_x_max );
		y_min = min( y_min, p_y_min );
		y_max = max( y_max, p_y_max );
	}
	// get bounds of outline polys
	for( int ip=0; ip<m_outline_poly.GetSize(); ip++ )
	{
		CPolyLine * p = &m_outline_poly[ip];
		for( int ic=0; ic<p->GetNumCorners(); ic++ )
		{
			int x = p->GetX( ic );
			int y = p->GetY( ic );
			x_min = min( x_min, x );
			x_max = max( x_max, x );
			y_min = min( y_min, y );
			y_max = max( y_max, y );
		}
	}
	// get bounds of text
	for( int it=0; it<m_tl->text_ptr.GetSize(); it++ )
	{
		CText * t = m_tl->text_ptr[it];
		int thickness = t->m_stroke_width/NM_PER_MIL;
		SMFontUtil * smfontutil = ((CFreePcbApp*)AfxGetApp())->m_Doc->m_smfontutil;
		CString t_str = t->m_str;
		double x_scale = (double)t->m_font_size/22.0;
		double y_scale = (double)t->m_font_size/22.0;
		double y_offset = 9.0*y_scale;
		int i = 0;
		double xc = 0.0;
		CPoint si, sf;
		for( int ic=0; ic<t_str.GetLength(); ic++ )
		{
			// get stroke info for character
			int xi, yi, xf, yf;
			double coord[64][4];
			double min_x, min_y, max_x, max_y;
			int nstrokes = smfontutil->GetCharStrokes( t_str[ic], SIMPLEX, 
				&min_x, &min_y, &max_x, &max_y, coord, 64 );
			for( int is=0; is<nstrokes; is++ )
			{
				// get each
				xi = (coord[is][0] - min_x)*x_scale + xc;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (coord[is][2] - min_x)*x_scale + xc;
				yf = coord[is][3]*y_scale + y_offset;
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
				// move to origin of text box
				si.x += t->m_x;
				sf.x += t->m_x;
				si.y += t->m_y;
				sf.y += t->m_y;
				// rotate to angle 
				CPoint text_org( t->m_x, t->m_y );
				RotatePoint( &si, t->m_angle, text_org );
				RotatePoint( &sf, t->m_angle, text_org );
				// draw
				x_min = min( x_min, min(si.x,sf.x) );
				x_max = max( x_max, max(si.x,sf.x) );
				y_min = min( y_min, min(si.y,sf.y) );
				y_max = max( y_max, max(si.y,sf.y) );
			}
			xc += (max_x - min_x + 8.0)*x_scale;
		}
	}
	// get bounds of ref_text
	x_min = min( x_min, m_ref_xi );
	x_max = max( x_max, m_ref_xi+3*m_ref_size );
	y_min = min( y_min, m_ref_yi );
	y_max = max( y_max, m_ref_yi+m_ref_size );

	// convert to mils
	x_min = x_min/NM_PER_MIL;
	x_max = x_max/NM_PER_MIL;
	y_min = y_min/NM_PER_MIL;
	y_max = y_max/NM_PER_MIL;

	// set up scale factor for drawing, mils per pixel
	double x_scale = (double)(x_max-x_min)/(double)x_size;
	double y_scale = (double)(y_max-y_min)/(double)y_size;
	double scale = max( x_scale, y_scale) * 1.2;
	if( scale < 1.0 )
		scale = 1.0;
	double x_center = (x_max+x_min)/2;
	double y_center = (y_max+y_min)/2;

	// set up frame rectangle for metafile
	int widthmm = pDC->GetDeviceCaps( HORZSIZE );
	int heightmm = pDC->GetDeviceCaps( VERTSIZE );
	int widthpixel = pDC->GetDeviceCaps( HORZRES );
	int heightpixel = pDC->GetDeviceCaps( VERTRES ); 

	// set up bounding rectangle, with bottom > top, in mils
	double left = x_center - scale*x_size/2;
	double right = x_center + scale*x_size/2;
	double top = y_center - scale*y_size/2;
	double bottom = y_center + scale*y_size/2;
	CRect rNM;
	rNM.left = left*100.0*widthmm/widthpixel;
	rNM.right = right*100.0*widthmm/widthpixel;
	rNM.top = -bottom*100.0*heightmm/heightpixel;		// notice Y flipped
	rNM.bottom = -top*100.0*heightmm/heightpixel;
	// offsets to ensure that origin is in rNM
	int xoffset = -(rNM.left+rNM.right)/2;
	rNM.left += xoffset;
	rNM.right += xoffset;
	xoffset = xoffset/(100.0*widthmm/widthpixel);
	int yoffset = -(rNM.top+rNM.bottom)/2;
	rNM.top += yoffset;
	rNM.bottom += yoffset;
	yoffset = yoffset/(100.0*heightmm/heightpixel); 
	// create enhanced metafile
	mfDC->CreateEnhanced( NULL, NULL, rNM, NULL );
	mfDC->SetAttribDC( *pDC );

	// now we can draw using mils
	// flip Y since metafile likes top < bottom
	CPen * old_pen;
	CBrush * old_brush;
	CPen backgnd_pen( PS_SOLID, 1, RGB(0,0,0) );
	CBrush backgnd_brush( RGB(0,0,0) );
	CPen SMT_pad_pen( PS_SOLID, 1, RGB(0,255,0) );
	CBrush SMT_pad_brush( RGB(0,255,0) );
	CPen SMT_B_pad_pen( PS_SOLID, 1, RGB(255,0,0) );
	CBrush SMT_B_pad_brush( RGB(255,0,0) );
	CPen TH_pad_pen( PS_SOLID, 1, RGB(0,0,255) );
	CBrush TH_pad_brush( RGB(0,0,255) );

	// draw background
	old_pen = mfDC->SelectObject( &backgnd_pen );
	old_brush = mfDC->SelectObject( &backgnd_brush );
	mfDC->Rectangle( rNM );

	// draw pads and holes
	// iterate twice to draw top copper over bottom copper
	for( int ipass=0; ipass<2; ipass++ )
	{
		for( int ip=0; ip<m_padstack.GetSize(); ip++ )
		{
			int x = m_padstack[ip].x_rel/NM_PER_MIL;
			int y = m_padstack[ip].y_rel/NM_PER_MIL;
			int angle = m_padstack[ip].angle;
			int hole_size = m_padstack[ip].hole_size/NM_PER_MIL;
			pad * p = &m_padstack[ip].top;
			if( m_padstack[ip].top.shape == PAD_NONE && m_padstack[ip].bottom.shape != PAD_NONE )
				p = &m_padstack[ip].bottom;
			int p_x_min, p_x_max, p_y_min, p_y_max;
			if( hole_size && ipass == 0 )
				continue;
			if( p == &m_padstack[ip].top && ipass == 0 )
				continue;
			if( p == &m_padstack[ip].bottom && ipass == 1 )
				continue;
			if( hole_size )
			{
				mfDC->SelectObject( &TH_pad_pen );
				mfDC->SelectObject( &TH_pad_brush );
			}
			else
			{
				if( p == &m_padstack[ip].top )
				{
					mfDC->SelectObject( &SMT_pad_pen );
					mfDC->SelectObject( &SMT_pad_brush );
				}
				else
				{
					mfDC->SelectObject( &SMT_B_pad_pen );
					mfDC->SelectObject( &SMT_B_pad_brush );
				}
			}
			if( p->shape == PAD_NONE )
			{
				p_x_min = x - hole_size/2;
				p_x_max = x + hole_size/2;
				p_y_min = y - hole_size/2;
				p_y_max = y + hole_size/2;
				mfDC->SelectStockObject( NULL_BRUSH);
				mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
			}
			else if( p->shape == PAD_ROUND )
			{
				p_x_min = x - (p->size_h/NM_PER_MIL)/2;
				p_x_max = x + (p->size_h/NM_PER_MIL)/2;
				p_y_min = y - (p->size_h/NM_PER_MIL)/2;
				p_y_max = y + (p->size_h/NM_PER_MIL)/2;
				mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					p_x_min = x - hole_size/2;
					p_x_max = x + hole_size/2;
					p_y_min = y - hole_size/2;
					p_y_max = y + hole_size/2;
					mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				}
			}
			else if( p->shape == PAD_SQUARE )
			{
				p_x_min = x - (p->size_h/NM_PER_MIL)/2;
				p_x_max = x + (p->size_h/NM_PER_MIL)/2;
				p_y_min = y - (p->size_h/NM_PER_MIL)/2;
				p_y_max = y + (p->size_h/NM_PER_MIL)/2;
				mfDC->Rectangle( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					p_x_min = x - hole_size/2;
					p_x_max = x + hole_size/2;
					p_y_min = y - hole_size/2;
					p_y_max = y + hole_size/2;
					mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				}
			}
			else if( p->shape == PAD_RECT )
			{
				CRect pr( -p->size_l, p->size_h/2, p->size_r, -p->size_h/2 );
				if( angle > 0 )
					RotateRect( &pr, angle, zero );
				p_x_min = x + pr.left/NM_PER_MIL;
				p_x_max = x + pr.right/NM_PER_MIL;
				p_y_min = y + pr.bottom/NM_PER_MIL;
				p_y_max = y + pr.top/NM_PER_MIL;
				mfDC->Rectangle( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					p_x_min = x - hole_size/2;
					p_x_max = x + hole_size/2;
					p_y_min = y - hole_size/2;
					p_y_max = y + hole_size/2;
					mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				}
			}
			else if( p->shape == PAD_RRECT )
			{
				CRect pr( -p->size_l, p->size_h/2, p->size_r, -p->size_h/2 );
				if( angle > 0 )
					RotateRect( &pr, angle, zero );
				int xi = x + pr.left/NM_PER_MIL;
				int xf = x + pr.right/NM_PER_MIL;
				int yi = y + pr.bottom/NM_PER_MIL;
				int yf = y + pr.top/NM_PER_MIL;
				int h = xf - xi;
				int v = yf - yi;
				int radius = p->radius/NM_PER_MIL; 
				mfDC->RoundRect( xi+xoffset, -yi+yoffset, xf+xoffset, -yf+yoffset, 2*radius, 2*radius ); 
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					xi = x - hole_size/2;
					xf = x + hole_size/2;
					yi = y - hole_size/2;
					yf = y + hole_size/2;
					mfDC->Ellipse( xi+xoffset, -yi+yoffset, xf+xoffset, -yf+yoffset );
				}
			}
			else if( p->shape == PAD_OVAL )
			{
				CRect pr( -p->size_l, p->size_h/2, p->size_r, -p->size_h/2 );
				if( angle > 0 )
					RotateRect( &pr, angle, zero );
				int xi = x + pr.left/NM_PER_MIL;
				int xf = x + pr.right/NM_PER_MIL;
				int yi = y + pr.bottom/NM_PER_MIL;
				int yf = y + pr.top/NM_PER_MIL;
				int h = abs(xf-xi);
				int v = abs(yf-yi);
				int radius = min(h,v);
				mfDC->RoundRect( xi+xoffset, -yi+yoffset, xf+xoffset, -yf+yoffset, radius, radius ); 
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					xi = x - hole_size/2;
					xf = x + hole_size/2;
					yi = y - hole_size/2;
					yf = y + hole_size/2;
					mfDC->Ellipse( xi+xoffset, -yi+yoffset, xf+xoffset, -yf+yoffset );
				}
			}
			else if( p->shape == PAD_OCTAGON )
			{
				const double pi = 3.14159265359;
				POINT pt[8];
				double angle = pi/8.0;
				for( int iv=0; iv<8; iv++ ) 
				{
					pt[iv].x = x + (0.5*p->size_h/NM_PER_MIL) * cos(angle)+xoffset;
					pt[iv].y = -(y + (0.5*p->size_h/NM_PER_MIL) * sin(angle))+yoffset;
					angle += pi/4.0;
				}
				mfDC->Polygon( pt, 8 );
				if( hole_size )
				{
					mfDC->SelectObject( &backgnd_pen );
					mfDC->SelectObject( &backgnd_brush );
					p_x_min = x - hole_size/2;
					p_x_max = x + hole_size/2;
					p_y_min = y - hole_size/2;
					p_y_max = y + hole_size/2;
					mfDC->Ellipse( p_x_min+xoffset, -p_y_min+yoffset, p_x_max+xoffset, -p_y_max+yoffset );
				}
			}
		}
	}

	// draw part outline
	for( int ip=0; ip<m_outline_poly.GetSize(); ip++ )
	{
		CPolyLine * p = &m_outline_poly[ip];
		int thickness = p->GetW()/NM_PER_MIL;
		CPen silk_pen( PS_SOLID, thickness, RGB(255,255,0) );
		mfDC->SelectObject( &silk_pen );
		int x = p->GetX(0)/NM_PER_MIL;
		int y = p->GetY(0)/NM_PER_MIL;
		mfDC->MoveTo( x+xoffset, -y+yoffset );
		for( int ic=1; ic<p->GetNumCorners(); ic++ )
		{
			int next_x = p->GetX( ic )/NM_PER_MIL;
			int next_y = p->GetY( ic )/NM_PER_MIL;
			int style = p->GetSideStyle(ic-1);
			int dl_shape;
			if( style == CPolyLine::STRAIGHT )
				dl_shape = DL_LINE;
			else if( style == CPolyLine::ARC_CW ) 
				dl_shape = DL_ARC_CCW;				// notice CW/CCW flipped since Y flipped
			else if( style == CPolyLine::ARC_CCW )
				dl_shape = DL_ARC_CW;
			else
				ASSERT(0);
			::DrawArc( mfDC, dl_shape, x+xoffset, -y+yoffset, next_x+xoffset, -next_y+yoffset, TRUE );
			x = next_x; 
			y = next_y;
		}
		if( p->GetClosed() )
		{
			int next_x = p->GetX(0)/NM_PER_MIL;
			int next_y = p->GetY(0)/NM_PER_MIL;
			int style = p->GetSideStyle(p->GetNumCorners()-1);
			int dl_shape;
			if( style == CPolyLine::STRAIGHT )
				dl_shape = DL_LINE;
			else if( style == CPolyLine::ARC_CW )
				dl_shape = DL_ARC_CCW;				// notice CW/CCW flipped since Y flipped
			else if( style == CPolyLine::ARC_CCW )
				dl_shape = DL_ARC_CW;
			else
				ASSERT(0);
			::DrawArc( mfDC, dl_shape, x+xoffset, -y+yoffset, next_x+xoffset, -next_y+yoffset, TRUE );
		}
	}

	// draw ref text placeholder
	int thickness = m_ref_w/NM_PER_MIL;
	CPen ref_pen( PS_SOLID, thickness, RGB(255,255,0) );
	mfDC->SelectObject( &ref_pen );
	SMFontUtil * smfontutil = ((CFreePcbApp*)AfxGetApp())->m_Doc->m_smfontutil;
	CString ref = "REF";
	x_scale = (double)m_ref_size/22.0;
	y_scale = (double)m_ref_size/22.0;
	double y_offset = 9.0*y_scale;
	int i = 0;
	double xc = 0.0;
	CPoint si, sf;
	for( int ic=0; ic<ref.GetLength(); ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes = smfontutil->GetCharStrokes( ref[ic], SIMPLEX, 
			&min_x, &min_y, &max_x, &max_y, coord, 64 );
		for( int is=0; is<nstrokes; is++ )
		{
			// draw each stroke
			xi = (coord[is][0] - min_x)*x_scale + xc;
			yi = coord[is][1]*y_scale + y_offset;
			xf = (coord[is][2] - min_x)*x_scale + xc;
			yf = coord[is][3]*y_scale + y_offset;
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
			// move to origin of text box
			si.x += m_ref_xi;
			sf.x += m_ref_xi;
			si.y += m_ref_yi;
			sf.y += m_ref_yi;
			// rotate to angle 
			CPoint ref_org( m_ref_xi, m_ref_yi );
			RotatePoint( &si, m_ref_angle, ref_org );
			RotatePoint( &sf, m_ref_angle, ref_org );
			// draw
			mfDC->MoveTo( si.x/NM_PER_MIL+xoffset, -si.y/NM_PER_MIL+yoffset );
			mfDC->LineTo( sf.x/NM_PER_MIL+xoffset, -sf.y/NM_PER_MIL+yoffset );
		}
		xc += (max_x - min_x + 8.0)*x_scale;
	}

	// draw text
	for( int it=0; it<m_tl->text_ptr.GetSize(); it++ )
	{
		CText * t = m_tl->text_ptr[it];
		int thickness = t->m_stroke_width/NM_PER_MIL;
		CPen text_pen( PS_SOLID, thickness, RGB(255,255,0) );
		mfDC->SelectObject( &text_pen );
		SMFontUtil * smfontutil = ((CFreePcbApp*)AfxGetApp())->m_Doc->m_smfontutil;
		CString t_str = t->m_str;
		x_scale = (double)t->m_font_size/22.0;
		y_scale = (double)t->m_font_size/22.0;
		double y_offset = 9.0*y_scale;
		int i = 0;
		double xc = 0.0;
		CPoint si, sf;
		for( int ic=0; ic<t_str.GetLength(); ic++ )
		{
			// get stroke info for character
			int xi, yi, xf, yf;
			double coord[64][4];
			double min_x, min_y, max_x, max_y;
			int nstrokes = smfontutil->GetCharStrokes( t_str[ic], SIMPLEX, 
				&min_x, &min_y, &max_x, &max_y, coord, 64 );
			for( int is=0; is<nstrokes; is++ )
			{
				// draw each stroke
				xi = (coord[is][0] - min_x)*x_scale + xc;
				yi = coord[is][1]*y_scale + y_offset;
				xf = (coord[is][2] - min_x)*x_scale + xc;
				yf = coord[is][3]*y_scale + y_offset;
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
				// move to origin of text box
				si.x += t->m_x;
				sf.x += t->m_x;
				si.y += t->m_y;
				sf.y += t->m_y;
				// rotate to angle 
				CPoint text_org( t->m_x, t->m_y );
				RotatePoint( &si, t->m_angle, text_org );
				RotatePoint( &sf, t->m_angle, text_org );
				// draw
				mfDC->MoveTo( si.x/NM_PER_MIL+xoffset, -si.y/NM_PER_MIL+yoffset );
				mfDC->LineTo( sf.x/NM_PER_MIL+xoffset, -sf.y/NM_PER_MIL+yoffset );
			}
			xc += (max_x - min_x + 8.0)*x_scale;
		}
	}

	// draw selection rectangle
	CPen sel_pen( PS_SOLID, 1, RGB(255,255,255) );
	mfDC->SelectObject( &sel_pen );
	mfDC->MoveTo( m_sel_xi/NM_PER_MIL+xoffset, -m_sel_yi/NM_PER_MIL+yoffset );
	mfDC->LineTo( m_sel_xf/NM_PER_MIL+xoffset, -m_sel_yi/NM_PER_MIL+yoffset );
	mfDC->LineTo( m_sel_xf/NM_PER_MIL+xoffset, -m_sel_yf/NM_PER_MIL+yoffset );
	mfDC->LineTo( m_sel_xi/NM_PER_MIL+xoffset, -m_sel_yf/NM_PER_MIL+yoffset );
	mfDC->LineTo( m_sel_xi/NM_PER_MIL+xoffset, -m_sel_yi/NM_PER_MIL+yoffset );

	// restore DC
	mfDC->SelectObject( old_brush );
	mfDC->SelectObject( old_pen );
	HENHMETAFILE hMF = mfDC->CloseEnhanced();
	return hMF;
}

HENHMETAFILE CShape::CreateWarningMetafile( CMetaFileDC * mfDC, CDC * pDC, int x_size, int y_size )
{
	CRect rNM;
	rNM.left = 0;
	rNM.right = x_size;
	rNM.top = 0;		// notice Y flipped
	rNM.bottom = y_size;
	mfDC->CreateEnhanced( NULL, NULL, rNM, NULL );
	mfDC->SetAttribDC( *pDC );

	CPen * old_pen;
	CBrush * old_brush;
	CPen backgnd_pen( PS_SOLID, 1, RGB(0,0,0) );
	CBrush backgnd_brush( RGB(0,0,0) );

	// draw background
	old_pen = mfDC->SelectObject( &backgnd_pen );
	old_brush = mfDC->SelectObject( &backgnd_brush );
	mfDC->Rectangle( rNM );


	// restore DC
	mfDC->SelectObject( old_brush );
	mfDC->SelectObject( old_pen );
	HENHMETAFILE hMF = mfDC->CloseEnhanced();
	return hMF;
}

// Get bounding rectangle of pad
//
CRect CShape::GetPadBounds( int i )
{
	CRect r;
	int dx=0, dy=0;
	padstack * ps = &m_padstack[i];
	pad * p = &ps->top;
	if( ps->top.shape == PAD_NONE && ps->bottom.shape != PAD_NONE )
		p = &ps->bottom;
	if( p->shape == PAD_NONE )
	{
		{
			dx = ps->hole_size/2;
			dy = dx;
		}
	}
	else if( p->shape == PAD_SQUARE || p->shape == PAD_ROUND || p->shape == PAD_OCTAGON )
	{
		dx = p->size_h/2;
		dy = dx;
	}
	else if( p->shape == PAD_RECT || p->shape == PAD_RRECT || p->shape == PAD_OVAL )
	{
		if( ps->angle == 0 || ps->angle == 180 )
		{
			dx = p->size_l;
			dy = p->size_h/2;
		}
		else if( ps->angle == 90 || ps->angle == 270 )
		{
			dx = p->size_h/2;
			dy = p->size_l;
		}
		else
			ASSERT(0);	// illegal angle
	}
	r.left = ps->x_rel - dx;
	r.right = ps->x_rel + dx;
	r.bottom = ps->y_rel - dy;
	r.top = ps->y_rel + dy;
	return r;
}

// Get bounding rectangle of row of pads
//
CRect CShape::GetPadRowBounds( int i, int num )
{
	CRect rr;

	rr.left = rr.bottom = INT_MAX;
	rr.right = rr.top = INT_MIN;
	for( int ip=i; ip<(i+num); ip++ )
	{
		CRect r = GetPadBounds( ip );
		rr.left = min( r.left, rr.left );
		rr.bottom = min( r.bottom, rr.bottom );
		rr.right = max( r.right, rr.right );
		rr.top = max( r.top, rr.top );
	}
	return rr;
}

// Get bounding rectangle of footprint
//
CRect CShape::GetBounds()
{
	CRect rr;

	rr.left = rr.bottom = INT_MAX;
	rr.right = rr.top = INT_MIN;
	for( int ip=0; ip<GetNumPins(); ip++ )
	{
		CRect r = GetPadBounds( ip );
		rr.left = min( r.left, rr.left );
		rr.bottom = min( r.bottom, rr.bottom );
		rr.right = max( r.right, rr.right );
		rr.top = max( r.top, rr.top );
	}
	for( int i=0; i<m_outline_poly.GetSize(); i++ )
	{
		CRect r = m_outline_poly[i].GetBounds();
		rr.left = min( r.left, rr.left );
		rr.bottom = min( r.bottom, rr.bottom );
		rr.right = max( r.right, rr.right );
		rr.top = max( r.top, rr.top );
	}
	if( rr.left == INT_MAX )
	{
		// no pads or polylines, make it a 100 mil square
		rr.left = 0;
		rr.right = 100*NM_PER_MIL;
		rr.bottom = 0;
		rr.top = 100*NM_PER_MIL;
	}
	return rr;
}

// Get bounding rectangle of footprint, not including polyline widths
//
CRect CShape::GetCornerBounds()
{
	CRect rr;

	rr.left = rr.bottom = INT_MAX;
	rr.right = rr.top = INT_MIN;
	for( int ip=0; ip<GetNumPins(); ip++ )
	{
		CRect r = GetPadBounds( ip );
		rr.left = min( r.left, rr.left );
		rr.bottom = min( r.bottom, rr.bottom );
		rr.right = max( r.right, rr.right );
		rr.top = max( r.top, rr.top );
	}
	for( int i=0; i<m_outline_poly.GetSize(); i++ )
	{
		CRect r = m_outline_poly[i].GetCornerBounds();
		rr.left = min( r.left, rr.left );
		rr.bottom = min( r.bottom, rr.bottom );
		rr.right = max( r.right, rr.right );
		rr.top = max( r.top, rr.top );
	}
	if( rr.left == INT_MAX )
	{
		// no pads or polylines, make it a 100 mil square
		rr.left = 0;
		rr.right = 100*NM_PER_MIL;
		rr.bottom = 0;
		rr.top = 100*NM_PER_MIL;
	}
	return rr;
}


//********************************************************
//
// Methods for CEditShape
//
//********************************************************

// constructor
//
CEditShape::CEditShape()
{ 
	m_dlist = NULL; 
	Clear(); 
}

// destructor
//
CEditShape::~CEditShape()
{
	Clear();
}

// draw footprint into display list
//
void CEditShape::Draw( CDisplayList * dlist, SMFontUtil * fontutil )
{
	id id;
	id.type = ID_PART;

	// first, undraw
	Undraw();

	// draw pads
	m_dlist = dlist;
	id.st = ID_PAD;
	int npads = GetNumPins();
	m_pad_el.SetSize( npads );
	m_pad_top_el.SetSize( npads );
	m_pad_inner_el.SetSize( npads );
	m_pad_bottom_el.SetSize( npads );
	m_pad_sel.SetSize( npads );
	for( int i=0; i<npads; i++ )
	{
		padstack * ps = &m_padstack[i];
		CPoint pin;
		id.i = i;
		pin.x = ps->x_rel;
		pin.y = ps->y_rel;
		dl_element * pad_sel;
		m_pad_sel[i] = NULL;
		for( int il=0; il<3; il++ )
		{
			// set layer for pads
			int pad_lay = LAY_FP_TOP_COPPER + il;;
			pad * p; 
			dl_element ** pad_el;
			if( il == 0 )
			{
				p = &ps->top;
				pad_el = &m_pad_top_el[i];
			}
			else if( il == 1 )
			{
				p = &ps->inner;	
				pad_el = &m_pad_inner_el[i];
			}
			else if( il == 2 )
			{
				p = &ps->bottom;
				pad_el = &m_pad_bottom_el[i];
			}

			// draw pad
			if( p->shape == PAD_NONE )
			{
				pad_sel = NULL;
			}
			else if( p->shape == PAD_ROUND )
			{
				// add to display list
				id.st = ID_PAD;
				*pad_el = dlist->Add( id, NULL, pad_lay, 
					DL_CIRC, 1, 
					p->size_h,
					0, 
					pin.x, pin.y, 0, 0, pin.x, pin.y );
				id.st = ID_SEL_PAD;
				pad_sel = dlist->AddSelector( id, NULL, pad_lay, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin.x-p->size_h/2,  
					pin.y-p->size_h/2, 
					pin.x+p->size_h/2, 
					pin.y+p->size_h/2, 
					pin.x, pin.y);
			}
			else if( p->shape == PAD_SQUARE )
			{
				id.st = ID_PAD;
				*pad_el = dlist->Add( id, NULL, pad_lay, 
					DL_SQUARE, 1, 
					p->size_h,
					0, 
					pin.x, pin.y, 
					0, 0, 
					pin.x, pin.y );
				id.st = ID_SEL_PAD;
				pad_sel = dlist->AddSelector( id, NULL, pad_lay, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin.x-p->size_h/2,  
					pin.y-p->size_h/2, 
					pin.x+p->size_h/2, 
					pin.y+p->size_h/2, 
					pin.x, pin.y);
			}
			else if( p->shape == PAD_RECT 
				|| p->shape == PAD_RRECT 
				|| p->shape == PAD_OVAL )
			{
				CPoint pad_pf, pad_pi;
				pad_pi.x = pin.x - p->size_l;
				pad_pi.y = pin.y - p->size_h/2;
				pad_pf.x = pin.x + p->size_r;
				pad_pf.y = pin.y + p->size_h/2;
				// rotate pad about pin if necessary
				if( m_padstack[i].angle > 0 )
				{
					RotatePoint( &pad_pi, ps->angle, pin );
					RotatePoint( &pad_pf, ps->angle, pin );
				}
				// add pad and selector to dlist
				id.st = ID_PAD;
				int gtype = DL_RECT;
				if( p->shape == PAD_RRECT )
					gtype = DL_RRECT;
				if( p->shape == PAD_OVAL )
					gtype = DL_OVAL;
				*pad_el = dlist->Add( id, NULL, pad_lay, 
					gtype, 1, 
					0,
					0, 
					pad_pi.x, pad_pi.y, 
					pad_pf.x, pad_pf.y, 
					pin.x, pin.y, p->radius );
				id.st = ID_SEL_PAD;
				pad_sel = dlist->AddSelector( id, NULL, pad_lay, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pad_pi.x, pad_pi.y, 
					pad_pf.x, pad_pf.y,
					pin.x, pin.y );
			}
			else if( p->shape == PAD_OCTAGON )
			{
				id.st = ID_PAD;
				*pad_el = dlist->Add( id, NULL, pad_lay, 
					DL_OCTAGON, 1, 
					p->size_h,
					0, 
					pin.x, pin.y, 
					0, 0, 
					pin.x, pin.y );
				id.st = ID_SEL_PAD;
				pad_sel = dlist->AddSelector( id, NULL, pad_lay, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin.x-p->size_h/2,  
					pin.y-p->size_h/2, 
					pin.x+p->size_h/2, 
					pin.y+p->size_h/2, 
					pin.x, pin.y);
			}
			if( m_pad_sel[i] == NULL )
				m_pad_sel[i] = pad_sel;
		}
		if( ps->hole_size )
		{
			// add to display list
			id.st = ID_PAD;
			m_pad_el[i] = dlist->Add( id, NULL, LAY_FP_PAD_THRU, 
				DL_HOLE, 1, 
				ps->hole_size,
				0, 
				pin.x, pin.y, 0, 0, pin.x, pin.y );
			id.st = ID_SEL_PAD;
			if( !m_pad_sel[i] )
			{
				m_pad_sel[i] = dlist->AddSelector( id, NULL, LAY_FP_PAD_THRU, 
					DL_HOLLOW_RECT, 1, 1, 0,
					pin.x-ps->hole_size/2,  
					pin.y-ps->hole_size/2, 
					pin.x+ps->hole_size/2, 
					pin.y+ps->hole_size/2, 
					pin.x, pin.y );
			}
		}
	}

	// draw ref designator text
	int silk_lay = LAY_FP_SILK_TOP;
	int nstrokes = 0;
	m_ref_el.SetSize(0);
	id.st = ID_STROKE;
	double x_scale = (double)m_ref_size/22.0;
	double y_scale = (double)m_ref_size/22.0;
	double y_offset = 9.0*y_scale;
	i = 0;
	double xc = 0.0;
	CPoint si, sf, tb_org;
	char ref_str[] = "REF";
	for( int ic=0; ic<3; ic++ )
	{
		// get stroke info for character
		int xi, yi, xf, yf;
		double coord[64][4];
		double min_x, min_y, max_x, max_y;
		int nstrokes = fontutil->GetCharStrokes( ref_str[ic], SIMPLEX, 
			&min_x, &min_y, &max_x, &max_y, coord, 64 );
		for( int is=0; is<nstrokes; is++ )
		{
			xi = (coord[is][0] - min_x)*x_scale + xc;
			yi = coord[is][1]*y_scale + y_offset;
			xf = (coord[is][2] - min_x)*x_scale + xc;
			yf = coord[is][3]*y_scale + y_offset;
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
			// rotate with text box
			tb_org.x = m_ref_xi;
			tb_org.y = m_ref_yi;
			RotatePoint( &si, m_ref_angle, zero );
			RotatePoint( &sf, m_ref_angle, zero );
			// move to origin of text box
			si.x += tb_org.x;
			sf.x += tb_org.x;
			si.y += tb_org.y;
			sf.y += tb_org.y;
			// draw
			id.i = i;
			m_ref_el.SetSize(i+1);
			m_ref_el[i] = dlist->Add( id, this, 
				silk_lay, DL_LINE, 1, m_ref_w, 0, 
				si.x, si.y, sf.x, sf.y, 0, 0 );
			i++;
		}
		xc += (max_x - min_x + 8.0)*x_scale;
	}

	// draw selection rectangle for ref text
	id.st = ID_SEL_REF_TXT;
	id.i = 0;
	int width = xc - 3.0*x_scale;
	si.x = m_ref_xi;
	sf.x = m_ref_xi + width;
	si.y = m_ref_yi;
	sf.y = m_ref_yi + m_ref_size;
	// rotate rectangle relative to part
	RotatePoint( &sf, m_ref_angle, si );
	// draw
	m_ref_sel = dlist->AddSelector( id, NULL, silk_lay, DL_HOLLOW_RECT, 1,
		0, 0, si.x, si.y, sf.x, sf.y, si.x, si.y );

	// now draw outline polylines
	id.st = ID_OUTLINE;
	for( i=0; i<m_outline_poly.GetSize(); i++ )
	{
		id.i = i;
		m_outline_poly[i].SetLayer( LAY_FP_SILK_TOP );
		int sel_box_size = m_outline_poly[i].GetW();
		sel_box_size = max( sel_box_size, 5*NM_PER_MIL );
		m_outline_poly[i].SetSelBoxSize( sel_box_size );
		m_outline_poly[i].SetId( &id );
		m_outline_poly[i].Draw( dlist );
	}

	// draw text
	m_tl->m_dlist = m_dlist;
	m_tl->m_smfontutil = fontutil;
	for( int it=0; it<m_tl->text_ptr.GetSize(); it++ )
	{
		m_tl->text_ptr[it]->Draw( dlist, fontutil );
	}
}

void CEditShape::Clear()
{
	Undraw();
	CShape::Clear();
}

#if 0
void CEditShape::Copy( CEditShape * eshape )
{
	Undraw();
	CShape::Copy( eshape );
}
#endif

void CEditShape::Copy( CShape * shape )
{
	Undraw();
	CShape::Copy( shape );
}

void CEditShape::Undraw()
{
	if( !m_dlist )
		return;

	for( int i=0; i<m_pad_el.GetSize(); i++ )
	{
		m_dlist->Remove( m_pad_el[i] );
		m_dlist->Remove( m_pad_sel[i] );
		m_dlist->Remove( m_pad_top_el[i] );
		m_dlist->Remove( m_pad_inner_el[i] );
		m_dlist->Remove( m_pad_bottom_el[i] );
	}
	m_pad_el.RemoveAll();
	m_pad_sel.RemoveAll();
	m_pad_top_el.RemoveAll();
	m_pad_inner_el.RemoveAll();
	m_pad_bottom_el.RemoveAll();

	for( i=0; i<m_ref_el.GetSize(); i++ )
		m_dlist->Remove( m_ref_el[i] );
	m_ref_el.SetSize(0);

	m_dlist->Remove( m_ref_sel );
	m_ref_sel = NULL;

	for( i=0; i<m_outline_poly.GetSize(); i++ )
		m_outline_poly[i].Undraw();

	for( int it=0; it<m_tl->text_ptr.GetSize(); it++ )
	{
		m_tl->text_ptr[it]->Undraw();
	}

	m_dlist = NULL;
}

// Select part pad
//
void CEditShape::SelectPad( int i )
{
	// select it by making its selection rectangle visible
	m_dlist->HighLight( DL_RECT_X, 
		m_dlist->Get_x(m_pad_sel[i]), 
		m_dlist->Get_y(m_pad_sel[i]),
		m_dlist->Get_xf(m_pad_sel[i]), 
		m_dlist->Get_yf(m_pad_sel[i]), 
		1 );
}

// Start dragging pad
//
void CEditShape::StartDraggingPad( CDC * pDC, int i )
{
	// make pad invisible
	m_dlist->Set_visible( m_pad_el[i], 0 );
	// cancel selection 
	m_dlist->CancelHighLight();
	// drag
	CRect r = GetPadBounds( i );
	int x = m_padstack[i].x_rel;
	int y = m_padstack[i].y_rel;
	m_dlist->StartDraggingRectangle( pDC, 
						x, 
						y,
						r.left - x, 
						r.bottom - y,
						r.right - x, 
						r.top - y,
						0, LAY_FP_SELECTION );
}

// Cancel dragging pad
//
void CEditShape::CancelDraggingPad( int i )
{
	// make pad visible
	m_dlist->Set_visible( m_pad_el[i], 1 );
	// drag
	m_dlist->StopDragging();
}

// Start dragging row of pads
//
void CEditShape::StartDraggingPadRow( CDC * pDC, int i, int num )
{
	// cancel selection 
	m_dlist->CancelHighLight();
	// make pads invisible
	for( int ip=i; ip<(i+num); ip++ )
	{
		m_dlist->Set_visible( m_pad_el[ip], 0 );
		m_dlist->Set_visible( m_pad_top_el[ip], 0 );
		m_dlist->Set_visible( m_pad_inner_el[ip], 0 );
		m_dlist->Set_visible( m_pad_bottom_el[ip], 0 );
	}
	// drag
	CRect r = GetPadRowBounds( i, num );
	int x = m_padstack[i].x_rel;
	int y = m_padstack[i].y_rel;
	m_dlist->StartDraggingRectangle( pDC, 
						x, 
						y,
						r.left - x, 
						r.bottom - y,
						r.right - x, 
						r.top - y,
						0, LAY_FP_SELECTION );
}

// Cancel dragging row of pads
//
void CEditShape::CancelDraggingPadRow( int i, int num )
{
	// make pad visible
	for( int ip=i; ip<(i+num); ip++ )
		m_dlist->Set_visible( m_pad_el[ip], 1 );
	// drag
	m_dlist->StopDragging();
}

// Select ref text
//
void CEditShape::SelectRef()
{
	// select it by making its selection rectangle visible
	m_dlist->HighLight( DL_HOLLOW_RECT, 
		m_dlist->Get_x(m_ref_sel), 
		m_dlist->Get_y(m_ref_sel),
		m_dlist->Get_xf(m_ref_sel), 
		m_dlist->Get_yf(m_ref_sel), 
		1 );
}

// Start dragging ref text box
//
void CEditShape::StartDraggingRef( CDC * pDC )
{
	// make ref text invisible
	for( int i=0; i<m_ref_el.GetSize(); i++ )
		m_dlist->Set_visible( m_ref_el[i], 0 );
	// cancel selection 
	m_dlist->CancelHighLight();
	// drag
	m_dlist->StartDraggingRectangle( pDC, 
						m_dlist->Get_x_org(m_ref_sel), 
						m_dlist->Get_y_org(m_ref_sel),
						m_dlist->Get_x(m_ref_sel)-m_dlist->Get_x_org(m_ref_sel), 
						m_dlist->Get_y(m_ref_sel)-m_dlist->Get_y_org(m_ref_sel),
						m_dlist->Get_xf(m_ref_sel)-m_dlist->Get_x_org(m_ref_sel), 
						m_dlist->Get_yf(m_ref_sel)-m_dlist->Get_y_org(m_ref_sel),
						0, LAY_FP_SELECTION );
}

// Cancel dragging ref text box
//
void CEditShape::CancelDraggingRef()
{
	// make ref text visible
	for( int i=0; i<m_ref_el.GetSize(); i++ )
		m_dlist->Set_visible( m_ref_el[i], 1 );
	// drag
	m_dlist->StopDragging();
}


void CEditShape::ShiftToInsertPadName( CString * astr, int n )
{
	CString test;
	test.Format( "%s%d", *astr, n );
	int index = GetPinIndexByName( &test );	 // see if pin name already exists
	if( index == -1 )
	{
		// no conflict, shift not needed
		return;
	}
	else
	{
		// pin name exists, shift it up by one
		ShiftToInsertPadName( astr, n+1 );
		CString new_name;
		new_name.Format( "%s%d", *astr, n+1 );
		m_padstack[index].name = new_name;
	}
	return;
}



