// make Gerber file
#include "stdafx.h"

enum {
	GERBER_BOARD_OUTLINE = 0x1,
	GERBER_AUTO_MOIRES = 0x2,
	GERBER_LAYER_TEXT = 0x4,
	GERBER_PILOT_HOLES = 0x8,
	GERBER_NO_PIN_THERMALS = 0x10,
	GERBER_NO_VIA_THERMALS = 0x20,
	GERBER_MASK_VIAS = 0x40
};

class CAperture;

typedef CArray<CAperture,CAperture> aperture_array;

class CAperture
{
public:
	enum {
		AP_NONE = 0,
		AP_CIRCLE,
		AP_SQUARE,
		AP_RECT,
		AP_MOIRE,
		AP_THERMAL,
		AP_OCTAGON,
		AP_OVAL
	};
	int m_type;				// type of aperture, see enum above
	int m_size1, m_size2;	// in NM

	CAperture();
	CAperture( int type, int size1, int size2 );
	~CAperture();
	BOOL Equals( CAperture * ap );
	int FindInArray( aperture_array * ap_array, BOOL ok_to_add=FALSE );
};

int WriteGerberFile( CStdioFile * f, int flags, int layer, 
					CDlgLog * log,
					int fill_clearance, int mask_clearance, int pilot_diameter,
					int min_silkscreen_stroke_wid, int thermal_wid,
					int outline_width, int hole_clearance,
					CArray<CPolyLine> * bd, CArray<CPolyLine> * sm, CPartList * pl, 
					CNetList * nl, CTextList * tl, CDisplayList * dl );
int WriteDrillFile( CStdioFile * file, CPartList * pl, CNetList * nl );

