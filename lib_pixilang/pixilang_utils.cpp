/*
    pixilang_utils.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"

char* pix_get_base_path( const char* src_name )
{
    int i;
    for( i = (int)smem_strlen( src_name ) - 1; i >= 0; i-- )
    {
	if( src_name[ i ] == '/' ) break;
    }
    if( i <= 0 )
    {
	char* rv = (char*)smem_new( 1 * sizeof( char ) );
	rv[ 0 ] = 0;
	return rv;
    }
    else
    {
	char* rv = (char*)smem_new( ( i + 2 ) * sizeof( char ) );
	smem_copy( rv, src_name, ( i + 1 ) * sizeof( char ) );
	rv[ i + 1 ] = 0;
	return rv;
    }
}

char* pix_compose_full_path( char* base_path, char* file_name, pix_vm* vm )
{
    if( !file_name ) return NULL;
    char* new_name = NULL;
    int name_len = (int)smem_strlen( file_name );
    if( vm && vm->virt_disk0 && name_len > 3 )
    {
	if( file_name[ 0 ] == '0' && file_name[ 1 ] == ':' && file_name[ 2 ] == '/' )
	{
	    //Virtual disk0:
	    new_name = (char*)smem_new( name_len + 16 );
	    sprintf( new_name, "vfs%d:/%s", vm->virt_disk0, file_name + 3 );
	    return new_name;
	}
    }
    if( ( name_len > 1 && file_name[ 0 ] == '/' ) ||
        ( name_len > 2 && file_name[ 1 ] == ':' ) )
    {
	//File name with absolute path:
	new_name = (char*)smem_new( name_len + 1 );
	smem_copy( new_name, file_name, name_len + 1 );
    }
    else 
    {
	if( base_path == 0 || base_path[ 0 ] == 0 )
	{
	    //Base path is empty:
	    new_name = (char*)smem_new( name_len + 1 );
	    smem_copy( new_name, file_name, name_len + 1 );
	}
	else 
	{
	    size_t len = smem_strlen( base_path );
	    new_name = (char*)smem_new( len + 1 + name_len + 1 );
	    new_name[ 0 ] = 0;
	    smem_strcat_resize( new_name, base_path );
	    if( base_path[ len - 1 ] != '/' )
		smem_strcat_resize( new_name, "/" );
	    smem_strcat_resize( new_name, file_name );
	}
    }
    return new_name;
}

void pix_str_to_num( const char* str, int str_len, PIX_VAL* v, int8_t* t, pix_vm* vm )
{
    PIX_VAL rv;
    int8_t rv_t = 0;
    rv.i = 0;
    while( 1 )
    {
	if( !str || str_len < 1 ) break;
	bool neg = 0;
	if( str[ 0 ] == '-' ) { neg = 1; str++; str_len--; }
        if( str_len < 1 ) break;
        if( str[ 0 ] == '#' )
	{
    	    //HEX COLOR:
    	    for( int i = 1; i < str_len; i++ )
    	    {
    		rv.i <<= 4;
        	if( str[ i ] < 58 ) rv.i += str[ i ] - '0';
        	else if( str[ i ] > 64 && str[ i ] < 91 ) rv.i += str[ i ] - 'A' + 10;
        	else rv.i += str[ i ] - 'a' + 10;
    	    }
    	    rv.i = get_color( ( rv.i >> 16 ) & 255, ( rv.i >> 8 ) & 255, rv.i & 255 );
	}
	else
	{
    	    if( str_len > 2 && str[ 1 ] == 'x' )
    	    {
        	//HEX:
        	for( int i = 2; i < str_len; i++ )
        	{
            	    rv.i <<= 4;
            	    if( str[ i ] < 58 ) rv.i += str[ i ] - '0';
            	    else if( str[ i ] > 64 && str[ i ] < 91 ) rv.i += str[ i ] - 'A' + 10;
            	    else rv.i += str[ i ] - 'a' + 10;
        	}
    	    }
    	    else if( str_len > 2 && str[ 1 ] == 'b' )
    	    {
        	//BIN:
        	for( int i = 2; i < str_len; i++ )
        	{
        	    rv.i <<= 1;
            	    rv.i += str[ i ] - '0';
        	}
    	    }
    	    else
    	    {
        	bool float_num = false;
        	for( int i = 0; i < str_len; i++ ) if( str[ i ] == '.' ) { float_num = true; break; }
        	if( float_num )
        	{
            	    //FLOATING POINT:
            	    if( str_len > 128 ) str_len = 128;
            	    char ts[ 128 + 1 ];
            	    smem_copy( ts, str, str_len );
            	    ts[ str_len ] = 0;
            	    rv_t = 1;
            	    rv.f = atof( ts );
        	}
        	else
        	{
            	    //DEC:
            	    for( int i = 0; i < str_len; i++ )
            	    {
                	rv.i *= 10;
                	rv.i += str[ i ] - '0';
            	    }
        	}
    	    }
	}
	if( neg )
	{
	    if( rv_t )
		rv.f = -rv.f;
	    else
		rv.i = -rv.i;
	}
	break;
    }
    *v = rv;
    *t = rv_t;
}
