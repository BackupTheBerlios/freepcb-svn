// PartList.h : definition of CPartList class
//
// this is a linked-list of parts on a PCB board
//
#include "stdafx.h"
#pragma once
#include "Shape.h"
#include "smfontutil.h"
#include "DlgLog.h"

#define MAX_REF_DES_SIZE 39

class cpart;
class CPartList;
class CNetList;
class cnet;

#include "DesignRules.h"

// struct to hold data to undo an operation on a part
//
struct undo_part {
	id m_id;				// instance id for this part
	BOOL visible;			// FALSE=hide part
	int x,y;				// position of part origin on board 
	int side;				// 0=top, 1=bottom
	int angle;				// orientation (degrees)
	BOOL glued;				// TRUE=glued in place
	int m_ref_xi;			// ref text position, relative to part
	int m_ref_yi;			//		"
	int m_ref_angle;		// ref text angle, relative to part
	int m_ref_size;			// ref text height
	int m_ref_w;			// ref text stroke width
	char ref_des[MAX_REF_DES_SIZE+1];	// ref designator such as "U3"
	char package[CShape::MAX_NAME_SIZE+1];		// package
	char shape_name[CShape::MAX_NAME_SIZE+1];	// name of shape
	CShape * shape;			// pointer to the footprint of the part, may be NULL
	CPartList * m_plist;	// parent cpartlist	
	// here goes array of char[npins][40] for attached net names
};

// partlist_info is used to hold digest of CPartList 
// for editing in dialogs, or importing from netlist file
// notes:
//	package may be "" if no package assigned
//	shape may be NULL if no footprint assigned
//	may have package but no footprint, but not the reverse
typedef struct {
	cpart * part;		// pointer to original part, or NULL if new part added
	CString ref_des;	// ref designator string
	int ref_size;		// size of ref text characters
	int ref_width;		// stroke width of ref text characters
	CString package;	// package (from original imported netlist, don't edit)
	CShape * shape;		// pointer to shape (may be edited)
	BOOL deleted;		// flag to indicate that part was deleted
	BOOL bShapeChanged;	// flag to indicate that the shape has changed
	BOOL bOffBoard;		// flag to indicate that position has not been set
	int x, y;			// position (may be edited)
	int angle, side;	// angle and side (may be edited)
} part_info;

typedef CArray<part_info> partlist_info;

// error codes
enum
{
	PL_NOERR = 0,
	PL_NO_DLIST,
	PL_ERR
};

// struct used for DRC to store pin info
struct drc_pin {
	int hole_size;	// hole diameter or 0
	int min_x;		// bounding rect of padstack
	int max_x;
	int min_y;
	int max_y;
	int max_r;		// max. radius of padstack
	int layers;		// bit mask of layers with pads
};

// class part_pin represents a pin on a part
// note that pin numbers start at 1,
// so index to pin array is (pin_num-1)
class part_pin 
{
public:
	int x, y;				// position on PCB
	cnet * net;				// pointer to net, or NULL if not assigned
	drc_pin drc;			// drc info
	dl_element * dl_sel;	// pointer to graphic element for selection shape
	dl_element * dl_hole;	// pointer to graphic element for hole
	CArray<dl_element*> dl_els;	// array of pointers to graphic elements for pads
};

// class cpart represents a part
class cpart
{
public:
	cpart();
	~cpart();
	cpart * prev;		// link backward
	cpart * next;		// link forward
	id m_id;			// instance id for this part
	BOOL drawn;			// TRUE if part has been drawn to display list
	BOOL visible;		// 0 to hide part
	int x,y;			// position of part origin on board
	int side;			// 0=top, 1=bottom
	int angle;			// orientation
	BOOL glued;			// 1=glued in place
	int m_ref_xi;		// reference text position, relative to part
	int m_ref_yi;	
	int m_ref_angle; 
	int m_ref_size;
	int m_ref_w;
	dl_element * dl_sel;		// pointer to display list element for selection rect
	CString ref_des;			// ref designator such as "U3"
	dl_element * dl_ref_sel;	// pointer to selection rect for ref text 
	CString package;			// package (from original imported netlist, may be "")
	CShape * shape;				// pointer to the footprint of the part, may be NULL
	CArray<stroke> ref_text_stroke;		// strokes for ref. text
	CArray<stroke> m_outline_stroke;	// array of outline strokes
	CArray<part_pin> pin;				// array of all pins in part
	int utility;		// used for various temporary purposes
	// drc info
	BOOL hole_flag;	// TRUE if holes present
	int min_x;		// bounding rect of pads
	int max_x;
	int min_y;
	int max_y;
	int max_r;		// max. radius of pads
	int layers;		// bit mask for layers with pads
	// flag used for importing
	BOOL bPreserve;	// preserve connections to this part
};

// this is the partlist class
class CPartList
{
public:
	enum {
		NOT_CONNECTED = 0,		// pin not attached to net
		ON_NET = 1,				// pin is attached to a net
		TRACE_CONNECT = 2,		// trace connects on this layer
		AREA_CONNECT = 4		// thermal connects on this layer
	};
	cpart m_start, m_end;
private:
	int m_size, m_max_size;
	int m_layers;
	int m_annular_ring;
	CNetList * m_nlist;
	CDisplayList * m_dlist;
	SMFontUtil * m_fontutil;	// class for Hershey font
	CMapStringToPtr * m_footprint_cache_map;

public:
	enum { 
		UNDO_PART_DELETE=1, 
		UNDO_PART_MODIFY, 
		UNDO_PART_RENAME,
		UNDO_PART_ADD };	// undo types
	CPartList( CDisplayList * dlist, SMFontUtil * fontutil );
	~CPartList();
	void UseNetList( CNetList * nlist ){ m_nlist = nlist; };
	void SetShapeCacheMap( CMapStringToPtr * shape_cache_map )
	{ m_footprint_cache_map = shape_cache_map; };
	cpart * Add(); 
	cpart * Add( CShape * shape, CString * ref_des, CString * package, 
					int x, int y, int side, int angle, int visible, int glued ); 
	cpart * AddFromString( CString * str );
	void SetNumCopperLayers( int nlayers ){ m_layers = nlayers;};
	int SetPartData( cpart * part, CShape * shape, CString * ref_des, CString * package, 
					int x, int y, int side, int angle, int visible, int glued ); 
	void MarkAllParts( int mark );
	int Remove( cpart * element );
	void RemoveAllParts();
	int HighlightPart( cpart * part );
	void MakePartVisible( cpart * part, BOOL bVisible );
	int SelectRefText( cpart * part );
	int SelectPad( cpart * part, int i );
	void HighlightAllPadsOnNet( cnet * net );
	BOOL TestHitOnPad( cpart * part, CString * pin_name, int x, int y, int layer );
	void MoveOrigin( int x_off, int y_off );
	int Move( cpart * part, int x, int y, int angle, int side );
	int MoveRefText( cpart * part, int x, int y, int angle, int size, int w );
	void ResizeRefText( cpart * part, int size, int width );
	int DrawPart( cpart * el );
	int UndrawPart( cpart * el );
	void PartFootprintChanged( cpart * part, CShape * shape );
	void FootprintChanged( CShape * shape );
	void RefTextSizeChanged( CShape * shape );
	int RotatePoint( CPoint * p, int angle, CPoint org );
	int RotateRect( CRect * r, int angle, CPoint p );
	int GetSide( cpart * part );
	int GetAngle( cpart * part );
	int GetRefAngle( cpart * part );
	CPoint GetRefPoint( cpart * part );
	CPoint GetPinPoint(  cpart * part, CString * pin_name );
	int GetPinLayer( cpart * part, CString * pin_name );
	int GetPinLayer( cpart * part, int pin_index );
	cnet * GetPinNet( cpart * part, CString * pin_name );
	cnet * GetPinNet( cpart * part, int pin_index );
	int GetPinWidth( cpart * part, CString * pin_name );
	void SetPinAnnularRing( int ring ){ m_annular_ring = ring; };
	int GetPartBoundingRect( cpart * part, CRect * part_r );
	int GetPartBoundaries( CRect * part_r );
	int GetPinConnectionStatus( cpart * part, CString * pin_name, int layer );
	int GetPadDrawInfo( cpart * part, int ipin, int layer,  BOOL bUseThermals, 
		int mask_clearance,  int paste_mask_shrink,
		int * type, int * x, int * y, int * w, int * l, int * r, int * hole,
		int * angle, cnet ** net, int * connection_type );
	cpart * GetPart( CString * ref_des );
	cpart * GetFirstPart();
	cpart * GetNextPart( cpart * part );
	int StartDraggingPart( CDC * pDC, cpart * part, BOOL bRatlines=TRUE );
	int StartDraggingRefText( CDC * pDC, cpart * part );
	int CancelDraggingPart( cpart * part );
	int CancelDraggingRefText( cpart * part );
	int StopDragging();
	int WriteParts( CStdioFile * file );
	int ReadParts( CStdioFile * file );
	int GetNumFootprintInstances( CShape * shape );
	void PurgeFootprintCache();
	int ExportPartListInfo( partlist_info * pl, cpart * part );
	void ImportPartListInfo( partlist_info * pl, int flags, CDlgLog * log=NULL );
	int SetPartString( cpart * part, CString * str );
	int CheckPartlist( CString * logstr );
	void DRC( CDlgLog * log, int copper_layers, 
		int units, BOOL check_unrouted,
		CArray<CPolyLine> * board_outline,
		DesignRules * dr, DRErrorList * DRElist );
	void * CreatePartUndoRecord( cpart * part );
	void * CreatePartUndoRecordForRename( cpart * part, CString * old_ref_des );
	static void PartUndoCallback( int type, void * ptr, BOOL undo );
};
 