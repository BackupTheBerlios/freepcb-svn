// file name : file_io.cpp
//
// file i/o routines for FreePCB
//
#include "stdafx.h" 


// parse string in format "keyword: param1 param2 param3 ..."
//
// if successful, returns number of params+1 and sets key_str to keyword,
// fills array param_str[] with params
//
// ignores whitespace and trailing comments beginning with "/"
// if the entire line is a comment or blank, returns 0
// if param is a string, must be enclosed in " " if it includes spaces and must be first param
//		then quotes are stripped
// if unable to parse, returns -1
//
int ParseKeyString( CString * str, CString * key_str, CArray<CString> * param_str )
{
	int pos = 0;
	int last_pos;
	int np = 0;
	for( int ip=0; ip<param_str->GetSize(); ip++ )
		(*param_str)[ip] = "";

	str->Trim();
	char c = str->GetAt(0);
	if( str->GetLength() == 0 || c == '/' )
		return 0;
	CString keyword = str->Tokenize( " :,\n\t", pos );
	last_pos = pos;
	if( keyword == "" )
		return -1;
	// keyword found
	keyword.Trim();
	*key_str = keyword;
	// now get params
	if( keyword.GetLength() == str->GetLength() )
		return 1;
	CString param;
	CString right_str = str->Right( str->GetLength() - keyword.GetLength() - 1);
	right_str.Trim();
	pos = 0;
	if( right_str[0] == '\"' )
	{
		// param starts with ", remove it and tokenize to following "
		param = right_str.Tokenize( "\"", pos );
	}
	else
	{
		param = right_str.Tokenize( " ,\n\t", pos );
	}
	while( param != "" )
	{
		param_str->SetAtGrow( np, param );
		// now test for next parameter starting with "
		last_pos = pos;
		param = right_str.Tokenize( " ,\n\t", pos );
		param.Trim();
		if( param[0] == '\"' )
		{
			// retokenize to get rid of ""
			pos = last_pos;
			param = right_str.Tokenize( "\"", pos );
			param = right_str.Tokenize( "\"", pos );
		}
		np++;
	}

	return np+1;
}

// my_atoi ... calls atoi and throws * CString exception if error
// if input string ends in "MIL", "mil", "MM", or "mm" convert to NM
//
int my_atoi( CString * in_str )
{
	CString str = *in_str;
	BOOL mil_units = FALSE;
	BOOL mm_units = FALSE;
	int len = str.GetLength();
	if( len > 3 && ( str.Right(3) == "MIL" || str.Right(3) == "mil" ) )
	{
		mil_units = TRUE;
		str = str.Left(len-3);
		len = len-3;
	}
	else if( len > 2 && ( str.Right(2) == "MM" || str.Right(2) == "mm" ) )
	{
		mm_units = TRUE;
		str = str.Left(len-2);
		len = len-2;
	}
	int test = atoi( str );
	if( test == 0 )
	{
		for( int i=0; i<len; i++ )
		{
			char c = str[i];
			if( !( c == '+' || c == '-' || ( c >= '0' ) ) )
			{
				CString * err_str = new CString;
				err_str->Format( "my_atoi() unable to convert string \"%s\" to int", *in_str );
				throw err_str;
			}
		}
	}
	if( mil_units )
		test = test * NM_PER_MIL;
	else if( mm_units )
		test = test * 1000000;
	return test;
}

// my_atof ... calls atof and throws * CString exception if error
// if input string ends in "MIL", "mil", "MM", or "mm" convert to NM
//
double my_atof( CString * in_str )
{
	CString str = *in_str;
	BOOL mil_units = FALSE;
	BOOL mm_units = FALSE;
	int len = str.GetLength();
	if( len > 3 && ( str.Right(3) == "MIL" || str.Right(3) == "mil" ) )
	{
		mil_units = TRUE;
		str = str.Left(len-3);
		len = len-3;
	}
	else if( len > 2 && ( str.Right(2) == "MM" || str.Right(2) == "mm" ) )
	{
		mm_units = TRUE;
		str = str.Left(len-2);
		len = len-2;
	}
	double test = atof( str );
	if( test == 0.0 )
	{
		for( int i=0; i<len; i++ )
		{
			char c = str[i];
			if( !( c == '.' || c == '+' || c == '-' || c == '0' || c == ' ' ) )
			{
				CString * err_str = new CString;
				err_str->Format( "my_atof() unable to convert string \"%s\" to double", *in_str );
				throw err_str;
			}
		}
	}
	if( mil_units )
		test = test * NM_PER_MIL;
	else if( mm_units )
		test = test * 1000000.0;
	return test;
}
