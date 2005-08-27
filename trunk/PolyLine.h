// PolyLine.h ... definition of CPolyLine class
//
// A polyline contains one or more contours, where each contour
// is defined by a list of corners and side-styles
// There may be multiple contours in a polyline.
// The last contour may be open or closed, any others must be closed.
// All of the corners and side-styles are concatenated into 2 arrays,
// separated by setting the end_contour flag of the last corner of 
// each contour.
//
// When used for copper areas, the first contour is the outer edge 
// of the area, subsequent ones are "holes" in the copper.
//
// If a CDisplayList pointer is provided, the polyline can draw itself 


#pragma once
#include <afxcoll.h>
#include <afxtempl.h>
#include "DisplayList.h"
#include "gpc.h"

class CPolyPt
{
public:
	CPolyPt( int qx=0, int qy=0, BOOL qf=FALSE )
	{ x=qx; y=qy; end_contour=qf; utility = 0; };
	int x;
	int y;
	BOOL end_contour;
	int utility;
};

class CPolyLine
{
public:
	enum { STRAIGHT, ARC_CW, ARC_CCW };	// side styles
	enum { NO_HATCH, DIAGONAL_FULL, DIAGONAL_EDGE }; // hatch styles

	// constructors/destructor
	CPolyLine( CDisplayList * dl );
	CPolyLine();
	~CPolyLine();

	// functions for modifying polyline
	void Start( int layer, int w, int sel_box, int x, int y,
		int hatch, id * id, void * ptr );
	void AppendCorner( int x, int y, int style = STRAIGHT );
	void InsertCorner( int ic, int x, int y );
	void DeleteCorner( int ic );
	void MoveCorner( int ic, int x, int y );
	void Close( int style = STRAIGHT );
	void RemoveContour( int icont );

	// drawing functions
	void HighlightSide( int is );
	void HighlightCorner( int ic );
	void StartDraggingToInsertCorner( CDC * pDC, int ic, int x, int y, int crosshair = 1 );
	void StartDraggingToMoveCorner( CDC * pDC, int ic, int x, int y, int crosshair = 1 );
	void CancelDraggingToInsertCorner( int ic );
	void CancelDraggingToMoveCorner( int ic );
	void Undraw();
	void Draw( CDisplayList * dl = NULL );
	void Hatch();
	void MakeVisible( BOOL visible = TRUE );
	void MoveOrigin( int x_off, int y_off );
	void SetSideVisible( int is, int visible );

	// misc. functions
	CRect GetBounds();
	CRect GetCornerBounds();
	void Copy( CPolyLine * src );
	BOOL TestPointInside( int x, int y );
	int TestIntersection( CPolyLine * poly );
	// access functions
	int GetNumCorners();
	int GetClosed();
	int GetNumContours();
	int GetContour( int ic );
	int GetContourStart( int icont );
	int GetContourEnd( int icont );
	int GetContourSize( int icont );
	int GetX( int ic );
	int GetY( int ic );
	int GetEndContour( int ic );
	int GetUtility( int ic ){ return corner[ic].utility; };
	int GetLayer();
	int GetW();
	int GetSideStyle( int is );
	id  GetId();
	int GetSelBoxSize();
	int GetHatch(){ return m_hatch; }
	void SetX( int ic, int x );
	void SetY( int ic, int y );
	void SetEndContour( int ic, BOOL end_contour );
	void SetUtility( int ic, int utility ){ corner[ic].utility = utility; };
	void SetLayer( int layer );
	void SetW( int w );
	void SetSideStyle( int is, int style );
	void SetId( id * id );
	void SetSelBoxSize( int sel_box );
	void SetHatch( int hatch ){ Undraw(); m_hatch = hatch; Draw(); };
	void SetDisplayList( CDisplayList * dl );
	// GPC functions
	int MakeGpcPoly( int icontour=0 );
	int FreeGpcPoly();
	gpc_polygon * GetGpcPoly(){ return &m_gpc_poly; };
	int NormalizeWithGpc();

private:
	CDisplayList * m_dlist;		// display list 
	id m_id;		// root id
	void * m_ptr;	// pointer to parent object (or NULL)
	int m_layer;	// layer to draw on
	int m_w;		// line width
	int m_sel_box;	// corner selection box width/2
	int m_ncorners;	// number of corners
	int utility;
//	int m_ncontours;			// number of contours in closed poly
	CArray <CPolyPt> corner;	// array of points for corners
	CArray <int> side_style;	// array of styles for sides
	CArray <dl_element*> dl_side;	// graphic elements
	CArray <dl_element*> dl_side_sel;
	CArray <dl_element*> dl_corner_sel;
	int m_hatch;	// hatch style, see enum above
	int m_nhatch;	// number of hatch lines
	CArray <dl_element*>  dl_hatch;	// hatch lines	
	gpc_polygon m_gpc_poly;	// polygon in gpc format
	BOOL bDrawn;
};