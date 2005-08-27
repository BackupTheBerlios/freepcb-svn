// Shape.h : interface for the CShape class
//
#pragma once

#include "stdafx.h"
#include "PolyLine.h"
#include "DisplayList.h"
#include "SMFontUtil.h"
#include "TextList.h"

class CTextList;

// pad shapes
enum
{
	PAD_NONE = 0,
	PAD_ROUND,
	PAD_SQUARE,
	PAD_RECT,
	PAD_RRECT,
	PAD_OVAL,
	PAD_OCTAGON
};

// error returns
enum
{
	PART_NOERR = 0,
	PART_ERR_TOO_MANY_PINS
};

// structure describing stroke (ie. line segment)
struct stroke
{
	int w, xi, yi, xf, yf;	// thickness + endpoints
	int type;				// CDisplayList g_type
	dl_element * dl_el;		// pointer to graphic element for stroke;
};

// structure describing mounting hole
// only used during conversion of Ivex files, then discarded
struct mtg_hole
{
	int pad_shape;		// used for pad on top and bottom
	int x, y, diam, pad_diam;
};


// structure describing pad
class pad
{
public:
	pad(){ radius=0; };
	BOOL operator==(pad p)
	{ return (	shape==p.shape 
				&& size_l==p.size_l 
				&& size_r==p.size_r
				&& size_h==p.size_h
				&& (shape!=PAD_RRECT || radius==p.radius) ); };

	int shape;	// see enum above
	int size_l, size_r, size_h, radius;
};

// padstack is pads and hole associated with a pin
class padstack
{
public:
	padstack(){ exists = FALSE; };
	BOOL operator==(padstack p)
	{ return (	name == p.name
				&& hole_size==p.hole_size 
				&& x_rel==p.x_rel 
				&& y_rel==p.y_rel
				&& angle==p.angle
				&& top==p.top
				&& bottom==p.bottom
				&& inner==p.inner				
				); };
	BOOL exists;		// only used when converting Ivex footprints or editing
	CString name;		// identifier such as "1" or "B24"
	int hole_size;		// 0 = no hole (i.e SMT)
	int x_rel, y_rel;	// position relative to part origin
	int angle;			// orientation: 0=left, 90=top, 180=right, 270=bottom
	pad top;
	pad bottom;
	pad inner;
};

// CShape class represents a footprint
//
class CShape
{
	// if variables are added, remember to modify Copy!
public:
	enum { MAX_NAME_SIZE = 59 };	// max. characters
	enum { MAX_PIN_NAME_SIZE = 39 };
	CString m_name;		// name of shape (e.g. "DIP20")
	CString m_author;
	CString m_source;
	CString m_desc;
	int m_units;		// units used for original definition (MM, NM or MIL)
	int m_sel_xi, m_sel_yi, m_sel_xf, m_sel_yf;			// selection rectangle
	int m_ref_size, m_ref_xi, m_ref_yi, m_ref_angle;	// ref text
	int m_ref_w;						// thickness of stroke for ref text
	CArray<padstack> m_padstack;		// array of padstacks for shape
	CArray<CPolyLine> m_outline_poly;	// array of polylines for part outline
	CTextList * m_tl;					// list of text strings

public:
	CShape();
	~CShape();
	void Clear();
	int MakeFromString( CString name, CString str );
	int MakeFromFile( CStdioFile * in_file, CString name, CString file_path, int pos );
	int WriteFootprint( CStdioFile * file );
	int GetNumPins();
	int GetPinIndexByName( CString * name );
	CString GetPinNameByIndex( int index );
	CRect GetBounds();
	CRect GetCornerBounds();
	CRect GetPadBounds( int i );
	CRect GetPadRowBounds( int i, int num );
	int Copy( CShape * shape );	// copy all data from shape
	BOOL Compare( CShape * shape );	// compare shapes, return true if same
	HENHMETAFILE CreateMetafile( CMetaFileDC * mfDC, CDC * pDC, int x_size, int y_size );
	HENHMETAFILE CreateWarningMetafile( CMetaFileDC * mfDC, CDC * pDC, int x_size, int y_size );
};


// CEditShape class represents a footprint whose elements can be edited
//
class CEditShape : public CShape
{
public:
	CEditShape();
	~CEditShape();
	void Clear();
	void Draw( CDisplayList * dlist, SMFontUtil * fontutil );
	void Undraw();
//	void Copy( CEditShape * eshape );
	void Copy( CShape * shape );
	void SelectPad( int i );
	void StartDraggingPad( CDC * pDC, int i );
	void CancelDraggingPad( int i );
	void StartDraggingPadRow( CDC * pDC, int i, int num );
	void CancelDraggingPadRow( int i, int num );
	void SelectRef();
	void StartDraggingRef( CDC * pDC );
	void CancelDraggingRef();
	void ShiftToInsertPadName( CString * astr, int n );

public:
	CDisplayList * m_dlist;
	CArray<dl_element*> m_pad_el;	// pad display element for hole 
	CArray<dl_element*> m_pad_top_el;		// top pad display element 
	CArray<dl_element*> m_pad_inner_el;		// inner pad display element 
	CArray<dl_element*> m_pad_bottom_el;	// bottom pad display element 
	CArray<dl_element*> m_pad_sel;	// pad selector
	CArray<dl_element*> m_ref_el;	// strokes for "REF"
	dl_element * m_ref_sel;			// ref selector
};
