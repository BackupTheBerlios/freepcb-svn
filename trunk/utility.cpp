// utility routines
//
#include "stdafx.h"
#include <math.h>
#include <time.h>

// globals for timer functions
LARGE_INTEGER PerfFreq, tStart, tStop;
int PerfFreqAdjust;
int OverheadTicks;

// function to rotate a point clockwise about another point
// currently, angle must be 0, 90, 180 or 270
//
void RotatePoint( CPoint *p, int angle, CPoint org )
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
}

// function to rotate a rectangle clockwise about a point
// angle must be 0, 90, 180 or 270
// on exit, r->top > r.bottom, r.right > r.left
//
void RotateRect( CRect *r, int angle, CPoint org )
{
	CRect tr;
	if( angle == 90 )
	{
		tr.left = org.x + (r->bottom - org.y);
		tr.right = org.x + (r->top - org.y);
		tr.top = org.y + (org.x - r->right);
		tr.bottom = org.y + (org.x - r->left);
		if( tr.left > tr.right )
		{
			int temp = tr.right;
			tr.left = tr.right;
			tr.left = temp;
		}
		if( tr.left > tr.right )
		{
			int temp = tr.right;
			tr.left = tr.right;
			tr.left = temp;
		}
		if( tr.bottom > tr.top )
		{
			int temp = tr.bottom;
			tr.bottom = tr.top;
			tr.top = temp;
		}
	}
	else if( angle > 90 )
	{
		tr = *r;
		for( int i=0; i<(angle/90); i++ )
			RotateRect( &tr, 90, org );
	}
	*r = tr;
}

// test for hit on line segment
// i.e. cursor within a given distance from segment
// enter with:	x,y = cursor coords
//				(xi,yi) and (xf,yf) are the end-points of the line segment
//				dist = maximum distance for hit
//
int TestLineHit( int xi, int yi, int xf, int yf, int x, int y, double dist )
{
	double dd;

	// test for vertical or horizontal segment
	if( xf==xi )
	{
		// vertical segment
		dd = fabs( (double)(x-xi) );
		if( dd<dist && ( (yf>yi && y<yf && y>yi) || (yf<yi && y>yf && y<yi) ) )
			return 1;
	}
	else if( yf==yi )
	{
		// horizontal segment
		dd = fabs( (double)(y-yi) );
		if( dd<dist && ( (xf>xi && x<xf && x>xi) || (xf<xi && x>xf && x<xi) ) )
			return 1;
	}
	else
	{
		// oblique segment
		// find a,b such that (xi,yi) and (xf,yf) lie on y = a + bx
		double b = (double)(yf-yi)/(xf-xi);
		double a = (double)yi-b*xi;
		// find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
		double d = -1.0/b;
		double c = (double)y-d*x;
		// find nearest point to (x,y) on line segment (xi,yi) to (xf,yf)
		double xp = (a-c)/(d-b);
		double yp = a + b*xp;
		// find distance
		dd = sqrt((x-xp)*(x-xp)+(y-yp)*(y-yp));
		if( fabs(b)>0.7 )
		{
			// line segment more vertical than horizontal
			if( dd<dist && ( (yf>yi && yp<yf && yp>yi) || (yf<yi && yp>yf && yp<yi) ) )
				return 1;
		}
		else
		{
			// line segment more horizontal than vertical
			if( dd<dist && ( (xf>xi && xp<xf && xp>xi) || (xf<xi && xp>xf && xp<xi) ) )
				return 1;
		}
	}	
	return 0;	// no hit
}

// function to read font file
// format is form "default.fnt" file from UnixPCB
//
// enter with:	fn = filename
//							
// return pointer to struct font, or 0 if error
//
int ReadFontFile( char * fn )
{
	return 0;
}

// safer version of strtok which buffers input string
// limited to strings < 256 characters
// returns pointer to substring if delimiter found
// returns 0 if delimiter not found
//
char * mystrtok( char * str, char * delim )
{
	static char s[256] = "";
	static int pos = 0;
	static int len = 0;
	int delim_len = strlen( delim ); 

	if( str )
	{
		len = strlen(str);
		if( len > 255 || len == 0 || delim_len == 0 )
		{
			len = 0;
			return 0;
		}
		strcpy( s, str );
		pos = 0;
	}
	else if( len == 0 )
		return 0;

	// now find delimiter, starting from pos
	int i = pos;
	while( i<=len )
	{
		for( int id=0; id<delim_len; id++ )
		{
			if( s[i] == delim[id] || s[i] == 0 )
			{
				// found delimiter, update pos and return
				int old_pos = pos;
				pos = i+1;
				s[i] = 0;
				return &s[old_pos]; 
			}
		}
		i++;
	}
	return 0;
}

// function to get dimension in PCB units from string
// string format:	nnnMIL for mils
//					nnn.nnMM for mm.
//					nnnnnnNM for nm.
//					nnnn for default units
// returns 0.0 if error
//
double GetDimensionFromString( CString * str, int def_units )
{
	double dim;
	int mult;

	if( def_units == MM )
		mult = NM_PER_MM;
	else if( def_units == MIL )
		mult = NM_PER_MIL;
	else if( def_units == NM )
		mult = 1;

	int len = str->GetLength();
	if( len > 2 )
	{
		if( str->Right(2) == "MM" )
			mult = NM_PER_MM;
		else if( str->Right(3) == "MIL" )
			mult = NM_PER_MIL;
		else if( str->Right(2) == "NM" )
			mult = 1;
	}
	dim = mult*atof( (LPCSTR)str->GetBuffer() );
	return dim;
}

// function to make string from dimension in NM, using requested units
// if append_units == TRUE, add unit string, like "10MIL"
// if lower_case == TRUE, use lower case for units, like "10mil"
// if space == TRUE, insert space, like "10 mil"
// max_dp is the maximum number of decimal places to include in string
//
void MakeCStringFromDimension( CString * str, int dim, int units, BOOL append_units, BOOL lower_case, BOOL space, int max_dp )
{
	if( units == MM )
		str->Format( "%11.6f", (double)dim/1000000.0 );
	else if( units == MIL )
		str->Format( "%11.6f", (double)dim/NM_PER_MIL );
	else if( units == NM )
		str->Format( "%d", dim );
	else
		ASSERT(0);
	str->Trim();

	// look for decimal point
	str->Trim();
	int dp_pos = str->Find( "." );

	// if decimal point, strip trailing zeros from MIL and MM strings
	if( dp_pos != -1 )
	{
		while(1 )
		{
			if( str->Right(1) == "0" )
				*str = str->Left( str->GetLength() - 1 );
			else if( str->Right(1) == "." )
			{
				*str = str->Left( str->GetLength() - 1 );
				break;
			}
			else
				break;
		}
	}

	// check number of decimal places and reduce if necessary
	int n_dp = 0;
	if( dp_pos != -1 )
	{
		// check to see if there are too many decimal places
		n_dp = str->GetLength() - dp_pos - 1;
		if( n_dp > max_dp )
			*str = str->Left( str->GetLength() - (n_dp-max_dp) );
		// see if there is now a trailing "."
		if( str->Right(1) == "." )
			*str = str->Left( str->GetLength()-1 );	// strip it, too
	}

	// append units if requested
	if( append_units )
	{
		if( units == MM && space == FALSE )
			*str = *str + "MM";
		else if( units == MM && space == TRUE )
			*str = *str + " MM";
		else if( units == MIL && space == FALSE )
			*str = *str + "MIL";
		else if( units == MIL && space == TRUE )
			*str = *str + " MIL";
		else if( units == NM && space == FALSE )
			*str = *str + "NM";
		else if( units == MIL && space == TRUE )
			*str = *str + " NM";
		if( lower_case )
			str->MakeLower();
	}
}

// function to make a CString from a double, stripping trailing zeros and "."
// allows maximum of 4 decimal places
//
void MakeCStringFromDouble( CString * str, double d )
{
	str->Format( "%12.4f", d );
	while(1 )
	{
		if( str->Right(1) == "0" )
			*str = str->Left( str->GetLength() - 1 );
		else if( str->Right(1) == "." )
		{
			*str = str->Left( str->GetLength() - 1 );
			break;
		}
		else
			break;
	}
	str->Trim();
}

// test for legal pin name, such as "1", "A4", "SOURCE", but not "1A"
// if astr != NULL, set to alphabetic part
// if nstr != NULL, set to numeric part
// if n != NULL, set to value of numeric part
//
BOOL CheckLegalPinName( CString * pinstr, CString * astr, CString * nstr, int * n )
{
	CString aastr;
	CString nnstr;
	int nn = -1;

	if( *pinstr == "" )
		return FALSE;
	if( -1 != pinstr->FindOneOf( " .,;:/!@#$%^&*(){}[]|<>?\\~\'\"" ) )
		return FALSE;
	int asize = pinstr->FindOneOf( "0123456789" );
	if( asize == -1 )
	{
		// starts with a non-number
		aastr = *pinstr;
	}
	else if( asize == 0 )
	{
		// starts with a number, illegal if any non-numbers
		nnstr = *pinstr;
		for( int ic=0; ic<nnstr.GetLength(); ic++ )
		{
			if( nnstr[ic] < '0' || nnstr[ic] > '9' )
				return FALSE;
		}
		nn = atoi( nnstr );
	}
	else
	{
		// both alpha and numeric parts
		// get alpha substring
		aastr = pinstr->Left( asize );
		int test = aastr.FindOneOf( "0123456789" );
		if( test != -1 )
			return FALSE;	// alpha substring contains a number
		// get numeric substring
		nnstr = pinstr->Right( pinstr->GetLength() - asize );
		CString teststr = nnstr.SpanIncluding( "0123456789" );
		if( teststr != nnstr )
			return FALSE;	// numeric substring contains non-number
		nn = atoi( nnstr );
	}
	if( astr )
		*astr = aastr;
	if( nstr )
		*nstr = nnstr;
	if( n )
		*n = nn;
	return TRUE;
}



// find intersection between y = a + bx and y = c + dx;
//
int FindLineIntersection( double a, double b, double c, double d, double * x, double * y )
{
	*x = (c-a)/(b-d);
	*y = a + b*(*x);
	return 0;
}

// find intersection between line segment (xi,yi) to (xf,yf)
// and line segment (xi2,yi2) to (xf2,yf2)
// the line segments may be arcs (i.e. quadrant of an ellipse) or straight
// return 0 if no intersection
// returns 1 or 2 if intersections found
// sets coords of intersections in *x1, *y1, *x2, *y2
//
int FindSegmentIntersection( int xi, int yi, int xf, int yf, int style, 
								 int xi2, int yi2, int xf2, int yf2, int style2,
								 double * x1, double * y1, double * x2, double * y2 )
{
	double xr[2], yr[2];
	int iret = 0;

	if( style == CPolyLine::STRAIGHT )
	{
		double b = (double)(yf-yi)/(double)(xf-xi);
		double a = yf - b*xf;
		double x1r, y1r, x2r, y2r;
		int ret = FindLineSegmentIntersection( a, b, xi2, yi2, xf2, yf2, style2,
									&x1r, &y1r, &x2r, &y2r );
		if( ret == 0 )
			return 0;
		else 
		{
			if( InRange( x1r, xi, xf ) && InRange( y1r, yi, yf ) )
			{
				xr[iret] = x1r;
				yr[iret] = y1r;
				iret++;
			}
			if( ret == 2 )
			{
				if( InRange( x2r, xi, xf ) && InRange( y2r, yi, yf ) )
				{
					xr[iret] = x2r;
					yr[iret] = y2r;
					iret++;
				}
			}
		}
	}
	else if( style2 == CPolyLine::STRAIGHT )
	{
		double b = (double)(yf2-yi2)/(double)(xf2-xi2);
		double a = yf2 - b*xf2;
		double x1r, y1r, x2r, y2r;
		int ret = FindLineSegmentIntersection( a, b, xi, yi, xf, yf, style,
									&x1r, &y1r, &x2r, &y2r );
		if( ret == 0 )
			return 0;
		else 
		{
			if( InRange( x1r, xi2, xf2 ) && InRange( y1r, yi2, yf2 ) )
			{
				xr[iret] = x1r;
				yr[iret] = y1r;
				iret++;
			}
			if( ret == 2 )
			{
				if( InRange( x2r, xi2, xf2 ) && InRange( y2r, yi2, yf2 ) )
				{
					xr[iret] = x2r;
					yr[iret] = y2r;
					iret++;
				}
			}
		}
	}
	else
	{
		// both segments are arcs
	}
	return iret;
}
// find intersection between y = a + bx and line segment (xi,yi) to (xf,yf)
// the line segment may be an arc (i.e. quadrant of an ellipse)
// return 0 if no intersection
// returns 1 or 2 if intersections found
// sets coords of intersections in *x1, *y1, *x2, *y2
//
int FindLineSegmentIntersection( double a, double b, int xi, int yi, int xf, int yf, int style, 
								double * x1, double * y1, double * x2, double * y2 )
{
	double xx, yy;
	if( xf != xi )
	{
		// horizontal or oblique line segment
		double d = (double)(yf-yi)/(double)(xf-xi);
		double c = yf - d*xf;
		if( b==d )
			return 0;	// lines parallel
		// get intersection
		if( style == CPolyLine::STRAIGHT || yf == yi )
		{
			// straight-line segment
			xx = (c-a)/(b-d);
			yy = a + b*(xx);
			// see if intersection is within the line segment
			if( yf == yi )
			{
				// horizontal line
				if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf) )
					return 0;
			}
			else
			{
				// oblique line
				if( (xx>=xi && xx>xf) || (xx<=xi && xx<xf) 
					|| (yy>yi && yy>yf) || (yy<yi && yy<yf) )
					return 0;
			}
		}
		else if( style == CPolyLine::ARC_CW || style == CPolyLine::ARC_CCW )
		{
			// arc (quadrant of ellipse)
			// convert to clockwise arc
			int xxi, xxf, yyi, yyf;
			if( style == CPolyLine::ARC_CCW )
			{
				xxi = xf;
				xxf = xi;
				yyi = yf;
				yyf = yi;
			}
			else
			{
				xxi = xi;
				xxf = xf;
				yyi = yi;
				yyf = yf;
			}
			// find center and radii of ellipse
			double xo, yo, rx, ry;
			if( xxf > xxi && yyf > yyi )
			{
				xo = xxf;
				yo = yyi;
			}
			else if( xxf < xxi && yyf > yyi )
			{
				xo = xxi;
				yo = yyf;
			}
			else if( xxf < xxi && yyf < yyi )
			{
				xo = xxf;
				yo = yyi;
			}
			else if( xxf > xxi && yyf < yyi )
			{
				xo = xxi;
				yo = yyf;
			}
			rx = fabs( (double)(xxi-xxf) );
			ry = fabs( (double)(yyi-yyf) );
			// now shift line to coordinate system of ellipse
			double aa = a + b*xo - yo;
			// now find intersections
			double xx1, xx2, yy1, yy2;
			BOOL test = FindLineEllipseIntersections( rx, ry, aa, b, &xx1, &xx2 );
			if( !test )
				return 0;
			else
			{
				// shift back to PCB coordinates
				yy1 = aa + b*xx1;
				xx1 += xo;
				yy1 += yo;
				yy2 = aa + b*xx2;
				xx2 += xo;
				yy2 += yo;
				int npts = 0;
				if( (xxf>xxi && xx1<xxf && xx1>xxi) || (xxf<xxi && xx1<xxi && xx1>xxf) )
				{
					if( (yyf>yyi && yy1<yyf && yy1>yyi) || (yyf<yyi && yy1<yyi && yy1>yyf) )
					{
						*x1 = xx1;
						*y1 = yy1;
						npts = 1;
					}
				}
				if( (xxf>xxi && xx2<xxf && xx2>xxi) || (xxf<xxi && xx2<xxi && xx2>xxf) )
				{
					if( (yyf>yyi && yy2<yyf && yy2>yyi) || (yyf<yyi && yy2<yyi && yy2>yyf) )
					{
						if( npts == 0 )
						{
							*x1 = xx2;
							*y1 = yy2;
							npts = 1;
						}
						else
						{
							*x2 = xx2;
							*y2 = yy2;
							npts = 2;
						}
					}
				}
				return npts;
			}
		}
		else
			ASSERT(0);
	}
	else
	{
		// vertical line segment
		xx = xi;
		yy = a + b*xx;
		if( (yy>=yi && yy>yf) || (yy<=yi && yy<yf) )
			return 0;
	}
	*x1 = xx;
	*y1 = yy;
	return 1;
}

// Test for intersection of line segments
// If lines are parallel, returns FALSE
// If TRUE, returns intersection coords in x, y
//
BOOL TestForIntersectionOfLineSegments( int x1i, int y1i, int x1f, int y1f, 
									   int x2i, int y2i, int x2f, int y2f,
									   int * x, int * y )
{
	double a, b;

	if( x1i == x1f && x2i == x2f )
	{
		// both segments are vertical, don't need to check
	}
	else if( y1i == y1f && y2i == y2f )
	{
		// both segments are vertical, don't need to check
	}
	else if( x1i == x1f && y2i == y2f )
	{
		// first seg. vertical, second horizontal, see if they cross
		if( InRange( x1i, x2i, x2f )
			&& InRange( y2i, y1i, y1f ) )
		{
			if( x )
				*x = x1i;
			if( y )
				*y = y2i;
			return TRUE;
		}
	}
	else if( y1i == y1f && x2i == x2f )
	{
		// first seg. horizontal, second vertical, see if they cross
		if( InRange( y1i, y2i, y2f )
			&& InRange( x2i, x1i, x1f ) )
		{
			if( x )
				*x = x2i;
			if( y )
				*y = y1i;
			return TRUE;
		}
	}
	else if( x1i == x1f )
	{
		// first segment vertical, second must be oblique
		// get a and b for second line segment, so that y = a + bx;
		b = (double)(y2f-y2i)/(x2f-x2i);
		a = (double)y2i - b*x2i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( !test )
			return FALSE;
		if( InRange( y1, y1i, y1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
		{
			if( x )
				*x = x1;
			if( y )
				*y = y1;
			return TRUE;
		}
	}
	else if( y1i == y1f )
	{
		// first segment horizontal, second must be oblique
		// get a and b for second line segment, so that y = a + bx;
		b = (double)(y2f-y2i)/(x2f-x2i);
		a = (double)y2i - b*x2i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( !test )
			return FALSE;
		if( InRange( x1, x1i, x1f ) && InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
		{
			if( x )
				*x = x1;
			if( y )
				*y = y1;
			return TRUE;
		}
	}
	else if( x2i == x2f )
	{
		// second segment vertical, first must be oblique
		// get a and b for first line segment, so that y = a + bx;
		b = (double)(y1f-y1i)/(x1f-x1i);
		a = (double)y1i - b*x1i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( !test )
			return FALSE;
		if( InRange( x1, x1i, x1f ) &&  InRange( y1, y1i, y1f ) && InRange( y1, y2i, y2f ) )
		{
			if( x )
				*x = x1;
			if( y )
				*y = y1;
			return TRUE;
		}
	}
	else if( y2i == y2f )
	{
		// second segment horizontal, first must be oblique
		// get a and b for second line segment, so that y = a + bx;
		b = (double)(y2f-y2i)/(x2f-x2i);
		a = (double)y2i - b*x2i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x1i, y1i, x1f, y1f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		if( !test )
			return FALSE;
		if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) && InRange( x1, x2i, x2f ) )
		{
			if( x )
				*x = x1;
			if( y )
				*y = y1;
			return TRUE;
		}
	}
	else
	{
		// both segments oblique
		// get a and b for first line segment, so that y = a + bx;
		b = (double)(y1f-y1i)/(x1f-x1i);
		a = (double)y1i - b*x1i;
		double x1, y1, x2, y2;
		int test = FindLineSegmentIntersection( a, b, x2i, y2i, x2f, y2f, CPolyLine::STRAIGHT,
			&x1, &y1, &x2, &y2 );
		// both segments oblique
		if( !test )
			return FALSE;
		if( InRange( x1, x1i, x1f ) && InRange( y1, y1i, y1f ) 
			&& InRange( x1, x2i, x2f ) && InRange( y1, y2i, y2f ) )
		{
			if( x )
				*x = x1;
			if( y )
				*y = y1;
			return TRUE;
		}
	}
	return FALSE;
}

// these functions are for profiling
//
void DunselFunction() { return; }

void CalibrateTimer()
{
	void (*pFunc)() = DunselFunction;

	if( QueryPerformanceFrequency( &PerfFreq ) )
	{
		// use hires timer
		PerfFreqAdjust = 0;
		int High32 = PerfFreq.HighPart;
		int Low32 = PerfFreq.LowPart;
		while( High32 )
		{
			High32 >>= 1;
			PerfFreqAdjust++;
		}
		return;
	}
	else
		ASSERT(0);
}

void StartTimer()
{
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
	QueryPerformanceCounter( &tStart );
}

double GetElapsedTime()
{
	QueryPerformanceCounter( &tStop );
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_NORMAL );

	double time;
	int ReduceMag = 0;
	LARGE_INTEGER Freq = PerfFreq;
	unsigned int High32 = tStop.HighPart - tStart.HighPart;
	while( High32 )
	{
		High32 >>= 1;
		ReduceMag++;
	}
	if( PerfFreqAdjust || ReduceMag )
	{
		if( PerfFreqAdjust > ReduceMag )
			ReduceMag = PerfFreqAdjust;
		tStart.QuadPart = Int64ShrlMod32( tStart.QuadPart, ReduceMag );
		tStop.QuadPart = Int64ShrlMod32( tStop.QuadPart, ReduceMag );
		Freq.QuadPart = Int64ShrlMod32( Freq.QuadPart, ReduceMag );
	}
	if( Freq.LowPart == 0 )
		time = 0.0;
	else
		time = ((double)(tStop.LowPart - tStart.LowPart))/Freq.LowPart;
	return time;
}

// quicksort algorithm
// sorts array numbers[], also moves elements of another array index[]
//
#define Q3WAY
void quickSort(int numbers[], int index[], int array_size)
{
#ifdef Q3WAY
  q_sort_3way(numbers, index, 0, array_size - 1);
#else
  q_sort(numbers, index, 0, array_size - 1);
#endif
}

// standard quicksort
//
void q_sort(int numbers[], int index[], int left, int right)
{
  int pivot, l_hold, r_hold;

  l_hold = left;
  r_hold = right;
  pivot = numbers[left];
  while (left < right)
  {
    while ((numbers[right] >= pivot) && (left < right))
      right--;
    if (left != right)
    {
      numbers[left] = numbers[right];
      index[left] = index[right];
      left++;
    }
    while ((numbers[left] <= pivot) && (left < right))
      left++;
    if (left != right)
    {
      numbers[right] = numbers[left];
      index[right] = index[left];
      right--;
    }
  }
  numbers[left] = pivot;
  pivot = left;
  left = l_hold;
  right = r_hold;
  if (left < pivot)
    q_sort(numbers, index, left, pivot-1);
  if (right > pivot)
    q_sort(numbers, index, pivot+1, right);
}

// 3-way quicksort...useful where there are duplicate values
//
void q_sort_3way( int a[], int b[], int l, int r )
{
	#define EXCH(i,j) {int temp=a[i]; a[i]=a[j]; a[j]=temp; temp=b[i]; b[i]=b[j]; b[j]=temp;}

	int i = l - 1;
	int j = r;
	int p = l - 1;
	int q = r;
	int v = a[r];

	if( r <= l )
		return;

	for(;;)
	{
		while( a[++i] < v );
		while( v < a[--j] )
			if( j == 1 )
				break;
		if( i >= j )
			break;
		EXCH( i, j );
		if( a[i] == v )
		{
			p++;
			EXCH( p, i );
		}
		if( v == a[j] )
		{
			q--;
			EXCH( j, q );
		}
	}
	EXCH( i, r );
	j = i - 1;
	i = i + 1;
	for( int k=l; k<p; k++, j-- )
		EXCH( k, j );
	for( int k=r-1; k>q; k--, i++ )
		EXCH( i, k );
	q_sort_3way( a, b, l, j );
	q_sort_3way( a, b, i, r );
}

// solves quadratic equation
// i.e.   ax**2 + bx + c = 0
// returns TRUE if solution exist, with solutions in x1 and x2
// else returns FALSE
//
BOOL Quadratic( double a, double b, double c, double *x1, double *x2 )
{
	double root = b*b - 4.0*a*c;
	if( root < 0.0 )
		return FALSE;
	root = sqrt( root );
	*x1 = (-b+root)/(2.0*a);
	*x2 = (-b-root)/(2.0*a);
	return TRUE;
}

// finds intersections of straight line y = c + dx
// with ellipse defined by (x^2)/(a^2) + (y^2)/(b^2) = 1;
// returns TRUE if solution exist, with solutions in x1 and x2
// else returns FALSE
//
BOOL FindLineEllipseIntersections( double a, double b, double c, double d, double *x1, double *x2 )
{
	// quadratic terms
	double A = d*d+b*b/(a*a);
	double B = 2.0*c*d;
	double C = c*c-b*b;
	return Quadratic( A, B, C, x1, x2 );
}

// finds intersections of ellipse defined by (x^2)/(a^2) + (y^2)/(b^2) = 1;
// with ellipse defined by (x^2)/(c^2) + (y^2)/(d^2) = 1;
// returns TRUE if solutions exist, with solutions in x1 and x2
// else returns FALSE
//
BOOL FindEllipseIntersections( double a, double b, double c, double d, double *x1, double *x2 )
{
	// quadratic terms
	double A = d*d+b*b/(a*a);
	double B = 2.0*c*d;
	double C = c*c-b*b;
	return Quadratic( A, B, C, x1, x2 );
}

// draw a straight line or an arc between xi,yi and xf,yf
//
void DrawArc( CDC * pDC, int shape, int xxi, int yyi, int xxf, int yyf, BOOL bMeta )
{
	int xi, yi, xf, yf;
	if( shape == DL_LINE || xxi == xxf || yyi == yyf )
	{
		// draw straight line
		pDC->MoveTo( xxi, yyi );
		pDC->LineTo( xxf, yyf );
	}
	else if( shape == DL_ARC_CCW || shape == DL_ARC_CW ) 
	{
		// set endpoints so we can always draw counter-clockwise arc
		if( shape == DL_ARC_CW )
		{
			xi = xxf;
			yi = yyf;
			xf = xxi; 
			yf = yyi;
		}
		else
		{
			xi = xxi;
			yi = yyi;
			xf = xxf;
			yf = yyf;
		}
		pDC->MoveTo( xi, yi );
		if( xf > xi && yf > yi )
		{
			// quadrant 1
			int w = (xf-xi)*2;
			int h = (yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xf-w, yi+h, xf, yi,
					xi, yi, xf, yf );
			else
				pDC->Arc( xf-w, yi, xf, yi+h,
					xf, yf, xi, yi );
		}
		else if( xf < xi && yf > yi )
		{
			// quadrant 2
			int w = -(xf-xi)*2;
			int h = (yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xi-w, yf, xi, yf-h,
					xi, yi, xf, yf );
			else
				pDC->Arc( xi-w, yf-h, xi, yf,
					xf, yf, xi, yi );
		}
		else if( xf < xi && yf < yi )
		{
			// quadrant 3
			int w = -(xf-xi)*2;
			int h = -(yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xf, yi, xf+w, yi-h,
					xi, yi, xf, yf ); 
			else
				pDC->Arc( xf, yi-h, xf+w, yi,
					xf, yf, xi, yi );
		}
		else if( xf > xi && yf < yi )
		{
			// quadrant 4
			int w = (xf-xi)*2;
			int h = -(yf-yi)*2;
			if( !bMeta )
				pDC->Arc( xi, yf+h, xi+w, yf,
					xi, yi, xf, yf );
			else
				pDC->Arc( xi, yf, xi+w, yf+h,
					xf, yf, xi, yi );
		}
		pDC->MoveTo( xxf, yyf );
	}
	else
		ASSERT(0);	// oops
}

// Get arrays of circles, rects and line segments to represent pad
// for purposes of drawing pad or calculating clearances
// margins of circles and line segments represent pad outline
// circles and rects are used to find points inside pad
//
void GetPadElements( int type, int x, int y, int wid, int len, int radius, int angle,
					int * nr, my_rect r[], int * nc, my_circle c[], int * ns, my_seg s[] )
{
	*nc = 0;
	*nr = 0;
	*ns = 0;
	if( type == PAD_ROUND )
	{
		*nc = 1;
		c[0] = my_circle(x,y,wid/2);
		return;
	}
	if( type == PAD_SQUARE )
	{
		*nr = 1;
		r[0] = my_rect(x-wid/2, y-wid/2,x+wid/2, y+wid/2);
		*ns = 4;
		s[0] = my_seg(x-wid/2, y+wid/2,x+wid/2, y+wid/2);	// top
		s[1] = my_seg(x-wid/2, y-wid/2,x+wid/2, y-wid/2);	// bottom
		s[2] = my_seg(x-wid/2, y-wid/2,x-wid/2, y+wid/2);	// left
		s[3] = my_seg(x+wid/2, y-wid/2,x+wid/2, y+wid/2);	// right
		return;
	}
	if( type == PAD_OCTAGON )
	{
		const double pi = 3.14159265359;
		*nc = 1;	// circle represents inside of polygon
		c[0] = my_circle(x, y, wid/2);
		*ns = 8;	// now create sides of polygon
		double theta = pi/8.0;
		double radius = 0.5*(double)wid/cos(theta);
		double last_x = x + radius*cos(theta);
		double last_y = y + radius*sin(theta);
		for( int is=0; is<8; is++ )
		{
			theta += pi/4.0;
			double dx = x + radius*cos(theta);
			double dy = y + radius*sin(theta);
			s[is] = my_seg(last_x, last_y, x, y);
			last_x = dx;
			last_y = dy;
		}
		return;
	}
	// 
	int h;
	int v;
	if( angle == 90 || angle == 270 )
	{
		h = wid;
		v = len;
	}
	else
	{
		v = wid;
		h = len;
	}
	if( type == PAD_RECT )
	{
		*nr = 1;
		r[0] = my_rect(x-h/2, y-v/2, x+h/2, y+v/2);
		*ns = 4;
		s[0] = my_seg(x-h/2, y+v/2,x+h/2, y+v/2);	// top
		s[1] = my_seg(x-h/2, y-v/2,x+h/2, y-v/2);	// bottom
		s[2] = my_seg(x-h/2, y-v/2,x-h/2, y+v/2);	// left
		s[3] = my_seg(x+h/2, y-v/2,x+h/2, y+v/2);	// right
		return;
	}
	if( type == PAD_RRECT )
	{
		*nc = 4;
		c[0] = my_circle(x-h/2+radius, y-v/2+radius, radius);	// bottom left circle
		c[1] = my_circle(x+h/2-radius, y-v/2+radius, radius);	// bottom right circle
		c[2] = my_circle(x-h/2+radius, y+v/2-radius, radius);	// top left circle
		c[3] = my_circle(x+h/2-radius, y+v/2-radius, radius);	// top right circle
		*ns = 4;
		s[0] = my_seg(x-h/2+radius, y+v/2, x+h/2-radius, y+v/2);	// top
		s[1] = my_seg(x-h/2+radius, y-v/2, x+h/2-radius, y+v/2);	// bottom
		s[2] = my_seg(x-h/2, y-v/2+radius, x-h/2, y+v/2-radius);	// left
		s[3] = my_seg(x+h/2, y-v/2+radius, x+h/2, y+v/2-radius);	// right
		return;
	}
	if( type == PAD_OVAL )
	{
		if( h > v )
		{
			// horizontal
			*nc = 2;
			c[0] = my_circle(x-h/2+v/2, y, v/2);	// left circle
			c[1] = my_circle(x+h/2-v/2, y, v/2);	// right circle
			*nr = 1;
			r[0] = my_rect(x-h/2+v/2, y-v/2, x+h/2-v/2, y+v/2);
			*ns = 2;
			s[0] = my_seg(x-h/2+v/2, y+v/2, x+h/2-v/2, y+v/2);	// top
			s[1] = my_seg(x-h/2+v/2, y-v/2, x+h/2-v/2, y-v/2);	// bottom
		}
		else
		{
			// vertical
			*nc = 2;
			c[0] = my_circle(x, y+v/2-h/2, h/2);	// top circle
			c[1] = my_circle(x, y-v/2+h/2, h/2);	// bottom circle
			*nr = 1;
			r[0] = my_rect(x-h/2, y-v/2+h/2, x+h/2, y+v/2-h/2);
			*ns = 2;
			s[0] = my_seg(x-h/2, y-v/2+h/2, x-h/2, y+v/2-h/2);	// left
			s[1] = my_seg(x+h/2, y-v/2+h/2, x+h/2, y+v/2-h/2);	// left
		}
		return;
	}
	ASSERT(0);
}

// Find distance from a line segment to a pad
//
int GetClearanceBetweenSegmentAndPad( int x1, int y1, int x2, int y2, int w,
								  int type, int x, int y, int wid, int len, int radius, int angle )
{
	if( type == PAD_NONE )
		return INT_MAX;
	else
	{
		int nc, nr, ns;
		my_circle c[4];
		my_rect r[2];
		my_seg s[8];
		GetPadElements( type, x, y, wid, len, radius, angle,
						&nr, r, &nc, c, &ns, s );
		// first test for endpoints of line segment in rectangle
		for( int ir=0; ir<nr; ir++ )
		{
			if( x1 >= r[ir].xlo && x1 <= r[ir].xhi && y1 >= r[ir].ylo && y1 <= r[ir].yhi )
				return 0;
			if( x2 >= r[ir].xlo && x2 <= r[ir].xhi && y2 >= r[ir].ylo && y2 <= r[ir].yhi )
				return 0;
		}
		// now get distance from elements of pad outline
		int dist = INT_MAX;
		for( int ic=0; ic<nc; ic++ )
		{
			int d = GetPointToLineSegmentDistance( c[ic].x, c[ic].y, x1, y1, x2, y2 ) - c[ic].r - w/2;
			dist = min(dist,d);
		}
		for( int is=0; is<ns; is++ )
		{
			int d = GetLineSegmentToLineSegmentDistance( s[is].xi, s[is].yi, s[is].xf, s[is].yf,
					x1, y1, x2, y2, NULL, NULL ) - w/2;
			dist = min(dist,d);
		}
		return max(0,dist);
	}
}

// Get clearance between 2 segments
// Returns point in segment closest to other segment in x, y
//
int GetClearanceBetweenSegments( int x1i, int y1i, int x1f, int y1f, int w1,
								   int x2i, int y2i, int x2f, int y2f, int w2,
								   int * x, int * y )
{
	int xx, yy;
	int d = GetLineSegmentToLineSegmentDistance( x1i, y1i, x1f, y1f, 
									x2i, y2i, x2f, y2f, &xx, &yy );
	d = max( 0, d - w1/2 - w2/2 );
	if( x )
		*x = xx;
	if( y )
		*y = yy;
	return d;
}



// Find clearance between pads
// For each pad:
//	type = PAD_ROUND, PAD_SQUARE, etc.
//	x, y = center position
//	w, l = width and length
//  r = corner radius
//	angle = 0 or 90 (if 0, pad length is along x-axis)
//
int GetClearanceBetweenPads( int type1, int x1, int y1, int w1, int l1, int r1, int angle1,
							 int type2, int x2, int y2, int w2, int l2, int r2, int angle2 )
{
	if( type1 == PAD_NONE )
		return INT_MAX;
	if( type2 == PAD_NONE )
		return INT_MAX;

	int dist = INT_MAX;
	int nr, nc, ns, nrr, ncc, nss;
	my_rect r[2], rr[2];
	my_circle c[4], cc[4];
	my_seg s[8], ss[8];

	GetPadElements( type1, x1, y1, w1, l1, r1, angle1,
					&nr, r, &nc, c, &ns, s );
	GetPadElements( type2, x2, y2, w2, l2, r2, angle2,
					&nrr, rr, &ncc, cc, &nss, ss );
	// now find distance from every element of pad1 to every element of pad2
	for( int ic=0; ic<nc; ic++ )
	{
		for( int icc=0; icc<ncc; icc++ )
		{
			int d = Distance( c[ic].x, c[ic].y, cc[icc].x, cc[icc].y )
						- c[ic].r - cc[icc].r;
			dist = min(dist,d);
		}
		for( int iss=0; iss<nss; iss++ )
		{
			int d = GetPointToLineSegmentDistance( c[ic].x, c[ic].y, 
						ss[iss].xi, ss[iss].yi, ss[iss].xf, ss[iss].yf ) - c[ic].r;
			dist = min(dist,d);
		}
	}
	for( int is=0; is<ns; is++ )
	{
		for( int icc=0; icc<ncc; icc++ )
		{
			int d = GetPointToLineSegmentDistance( cc[icc].x, cc[icc].y, 
						s[is].xi, s[is].yi, s[is].xf, s[is].yf ) - cc[icc].r;
			dist = min(dist,d);
		}
		for( int iss=0; iss<nss; iss++ )
		{
			int d =GetLineSegmentToLineSegmentDistance( s[is].xi, s[is].yi, s[is].xf, s[is].yf,
						ss[iss].xi, ss[iss].yi, ss[iss].xf, ss[iss].yf );
			dist = min(dist,d);
		}
	}
	return max(dist,0);
}

// Get distance between 2 line segments
// Returns coords of closest point in one of the segments
//
int GetLineSegmentToLineSegmentDistance( int x1i, int y1i, int x1f, int y1f,
										int x2i, int y2i, int x2f, int y2f,
										int * x, int * y )
{
	int xx, yy;
	// test for intersection
	if( TestForIntersectionOfLineSegments( x1i, y1i, x1f, y1f, x2i, y2i, x2f, y2f,
												&xx, &yy ) )
	{
		if( x )
			*x = xx;
		if( y )
			*y = yy;
		return 0;
	}

	// get shortest distance between each endpoint and the other line segment
	int d =     GetPointToLineSegmentDistance( x1i, y1i, x2i, y2i, x2f, y2f );
	xx = x1i;
	yy = y1i;
	int dd = GetPointToLineSegmentDistance( x1f, y1f, x2i, y2i, x2f, y2f );
	if( dd < d )
	{
		d = dd;
		xx = x1f;
		yy = y1f;
	}
	dd = GetPointToLineSegmentDistance( x2i, y2i, x1i, y1i, x1f, y1f );
	if( dd < d )
	{
		d = dd;
		xx = x2i;
		yy = y2i;
	}
	dd = GetPointToLineSegmentDistance( x2f, y2f, x1i, y1i, x1f, y1f );
	if( dd < d )
	{
		d = dd;
		xx = x2f;
		yy = y2f;
	}
	if( x )
		*x = xx;
	if( y )
		*y = yy;
	return d;
}

// Get distance between line segment and point
// enter with:	x,y = point
//				(xi,yi) and (xf,yf) are the end-points of the line segment
//
int GetPointToLineSegmentDistance( int x, int y, int xi, int yi, int xf, int yf )
{
	// test for vertical or horizontal segment
	if( xf==xi )
	{
		// vertical line segment
		if( InRange( y, yi, yf ) )
			return abs( x - xi );
		else
			return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
	}
	else if( yf==yi )
	{
		// horizontal line segment
		if( InRange( x, xi, xf ) )
			return abs( y - yi );
		else
			return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
	}
	else
	{
		// oblique segment
		// find a,b such that (xi,yi) and (xf,yf) lie on y = a + bx
		double b = (double)(yf-yi)/(xf-xi);
		double a = (double)yi-b*xi;
		// find c,d such that (x,y) lies on y = c + dx where d=(-1/b)
		double d = -1.0/b;
		double c = (double)y-d*x;
		// find nearest point to (x,y) on line through (xi,yi) to (xf,yf)
		double xp = (a-c)/(d-b);
		double yp = a + b*xp;
		// find distance
		if( InRange( xp, xi, xf ) && InRange( yp, yi, yf ) )
			return Distance( x, y, xp, yp );
		else
			return min( Distance( x, y, xi, yi ), Distance( x, y, xf, yf ) );
	}
	ASSERT(0);
}

// test for value within range
//
BOOL InRange( double x, double xi, double xf )
{
	if( xf>xi )
	{
		if( x >= xi && x <= xf )
			return TRUE;
	}
	else
	{
		if( x >= xf && x <= xi )
			return TRUE;
	}
	return FALSE;
}

// Get distance between 2 points
//
int Distance( int x1, int y1, int x2, int y2 )
{
	double d;
	d = sqrt( double(x1-x2)*(x1-x2) + double(y1-y2)*(y1-y2) );
	if( d > INT_MAX || d < INT_MIN )
		ASSERT(0);
	return (int)d;
}
