/*
    pixilang_vm_container.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"

#include "zlib.h"

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) slog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif

static int check_container_parameters( PIX_INT& xsize, PIX_INT& ysize, int& type )
{
    if( xsize <= 0 || ysize <= 0 ) return -1;
    if( type == 32 )
    {
	if( COLORLEN == 1 ) type = PIX_CONTAINER_TYPE_INT8;
	if( COLORLEN == 2 ) type = PIX_CONTAINER_TYPE_INT16;
	if( COLORLEN == 4 ) type = PIX_CONTAINER_TYPE_INT32;
	if( COLORLEN == 8 ) type = PIX_CONTAINER_TYPE_INT64;
    }    
    if( (unsigned)type >= PIX_CONTAINER_TYPES ) return -1;
    return 0;
}

PIX_CID pix_vm_new_container( PIX_CID cnum, PIX_INT xsize, PIX_INT ysize, int type, void* data, pix_vm* vm )
{
    PIX_CID rv = -1;
    
    if( check_container_parameters( xsize, ysize, type ) != 0 )
	return -1;
    
    if( !vm->c_ignore_mutex ) smutex_lock( &vm->c_mutex );
    
    if( cnum >= 0 ) 
	rv = cnum;
    else
    {
	for( PIX_CID i = 0; i < vm->c_num; i++ )
	{
	    if( vm->c[ vm->c_counter ] == 0 )
	    {
		rv = vm->c_counter;
		break;
	    }
	    vm->c_counter++;
	    if( vm->c_counter >= vm->c_num ) vm->c_counter = 0;
	}
    }
    if( rv < 0 )
    {
	PIX_VM_LOG( "Can't create a new container. All slots (%d) are busy.\n", vm->c_num );
    }
    else 
    {
	pix_vm_container* cont = (pix_vm_container*)smem_new( sizeof( pix_vm_container ) );
	if( !cont ) 
	{
	    PIX_VM_LOG( "Can't create a new container. Memory allocation error.\n" );
	    rv = -1;
	}
	else
	{
	    smem_zero( cont );
	    vm->c[ rv ] = cont;
	}
    }
    
    if( !vm->c_ignore_mutex ) smutex_unlock( &vm->c_mutex );
    
    if( rv >= 0 )
    {
	pix_vm_container* c = vm->c[ rv ];
	c->type = (pix_container_type)type;
	c->xsize = xsize;
	c->ysize = ysize;
	c->size = (size_t)xsize * (size_t)ysize;
	c->alpha = -1;

	DPRINT( "New container %d: %dx%d; type:%s\n", (PIX_CID)rv, (int)xsize, (int)ysize, g_pix_container_type_names[ type ] );

	if( data )
	{
	    c->data = data;
	}
	else
	{
	    size_t data_size = xsize * ysize * g_pix_container_type_sizes[ type ];
	    if( data_size )
	    {
		c->data = smem_new( data_size );
		if( !c->data )
		{
		    PIX_VM_LOG( "Can't create a new container. Memory allocation error.\n" );
		    vm->c[ rv ] = NULL;
		    rv = -1;
		}
	    }
	    else
		c->data = NULL;
	}
    }

    return rv;
}

void pix_vm_remove_container( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	if( vm->c[ cnum ] )
	{
	    if( cnum == vm->screen )
	    {
		vm->screen_ptr = 0;
		vm->screen = -1;
	    }  
	    pix_vm_container* c = vm->c[ cnum ];
	    size_t size = c->size;
	    uint flags = c->flags;
	    int type = c->type;
	    if( !( c->flags & PIX_CONTAINER_FLAG_STATIC_DATA ) )
	    {
		smem_free( c->data );
	    }
	    if( c->opt_data )
	    {
		pix_symtab_deinit( &c->opt_data->props );
		pix_vm_remove_container_hdata( cnum, vm );
#ifdef OPENGL
		pix_vm_remove_container_gl_data( cnum, vm );
#endif
		smem_free( c->opt_data );
	    }
	    smem_free( c );
	    if( !vm->c_ignore_mutex ) smutex_lock( &vm->c_mutex );
	    vm->c[ cnum ] = 0;
	    vm->c_counter = cnum;
	    if( !vm->c_ignore_mutex ) smutex_unlock( &vm->c_mutex );
	    DPRINT( "Container %d removed.\n", (int)cnum );
	    if( vm->c_show_debug_messages && ( flags & PIX_CONTAINER_FLAG_SYSTEM_MANAGED ) == 0 ) 
		PIX_VM_LOG( "Container removed: %d; size:%d; type:%d;\n", (int)cnum, (int)size, type );
	}
    }
}

int pix_vm_resize_container( PIX_CID cnum, PIX_INT new_xsize, PIX_INT new_ysize, int type, uint flags, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	if( vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    
	    PIX_INT old_xsize = c->xsize;
	    PIX_INT old_ysize = c->ysize;
	    if( new_xsize == -1 ) new_xsize = old_xsize;
	    if( new_ysize == -1 ) new_ysize = old_ysize;
	    if( type == -1 ) type = c->type;
	    if( new_xsize == old_xsize && new_ysize == old_ysize && type == c->type ) return( 0 );
	    
	    if( check_container_parameters( new_xsize, new_ysize, type ) == 0 )
	    {
		DPRINT( "Resize container %d: %dx%d; type:%s\n", (int)cnum, (int)new_xsize, (int)new_ysize, g_pix_container_type_names[ type ] );

		if( c->data )
		{
		    if( !( c->flags & PIX_CONTAINER_FLAG_STATIC_DATA ) )
		    {
			bool interp = false;
			if( ( flags & PIX_RESIZE_MASK_INTERPOLATION ) && type == c->type ) 
			{
			    if( ( PIX_RESIZE_INTERP_OPTIONS( flags ) == PIX_RESIZE_INTERP_COLOR ) && g_pix_container_type_sizes[ type ] != COLORLEN )
				PIX_VM_LOG( "resize(): container must be of PIXEL type when using RESIZE_COLOR_* flags\n" )
			    else
				interp = true;
			}
			if( interp )
			{
			    //Interpolation:
			    void* new_data = smem_new( new_xsize * new_ysize * g_pix_container_type_sizes[ type ] );
			    if( new_data == 0 ) return 1;

			    pix_vm_resize_pars resize_pars;
			    smem_clear( &resize_pars, sizeof( resize_pars ) );
			    resize_pars.dest = new_data;
			    resize_pars.src = c->data;
			    resize_pars.resize_flags = flags;
			    resize_pars.type = type;
			    resize_pars.dest_xsize = new_xsize;
			    resize_pars.dest_ysize = new_ysize;
			    resize_pars.dest_rect_xsize = new_xsize;
			    resize_pars.dest_rect_ysize = new_ysize;
			    resize_pars.src_xsize = old_xsize;
			    resize_pars.src_ysize = old_ysize;
			    resize_pars.src_rect_xsize = old_xsize;
			    resize_pars.src_rect_ysize = old_ysize;
			    pix_vm_copy_and_resize( &resize_pars );

			    smem_free( c->data );
			    c->data = new_data;
			}
			else
			{
			    //No interpolation:
			    c->data = smem_resize( c->data, new_xsize * new_ysize * g_pix_container_type_sizes[ type ] );
			}
		    }
		}
		else
		{
		    c->data = smem_new( new_xsize * new_ysize * g_pix_container_type_sizes[ type ] );
		}
		if( c->data == 0 ) return 1;

		c->type = (pix_container_type)type;
		c->xsize = new_xsize;
		c->ysize = new_ysize;
		c->size = (size_t)new_xsize * (size_t)new_ysize;

		if( cnum == vm->screen )
		{
		    pix_vm_gfx_set_screen( cnum, vm );
		}

#ifdef OPENGL
                pix_vm_remove_container_gl_data( cnum, vm );
#endif
	    }
	    else 
	    {
		return 2;
	    }
	}
    }
    
    return 0;
}

#define PIX_VM_ROTATE_BLOCK( type ) \
    { \
	type* data = (type*)ptr[ 0 ]; \
        type* new_data = (type*)new_data_v; \
        size_t p1 = 0; \
        for( PIX_INT y = 0; y < ysize[ 0 ]; y++ ) \
        { \
            size_t p22 = p2; \
            for( PIX_INT x = 0; x < xsize[ 0 ]; x++ ) \
            { \
        	new_data[ p22 ] = data[ p1 ]; \
                p22 += p2_xstep; \
                p1++; \
            } \
            p2 += p2_ystep; \
        } \
    }

#define PIX_VM_ROTATE_BLOCK2( type ) \
    { \
	type* data = (type*)ptr[ 0 ]; \
	type* new_data; \
	if( save_to ) \
	    new_data = (type*)save_to; \
	else \
	    new_data = (type*)ptr[ 0 ]; \
        for( size_t p1 = 0, p2 = xsize[ 0 ] * ysize[ 0 ] - 1; p1 < ( xsize[ 0 ] * ysize[ 0 ] ) / 2; p1++, p2-- ) \
        { \
    	    type temp = data[ p1 ]; new_data[ p1 ] = data[ p2 ]; new_data[ p2 ] = temp; \
    	} \
    }

//angle*90 degrees (clockwise (по часовой стрелке))
int pix_vm_rotate_block( void** ptr, PIX_INT* xsize, PIX_INT* ysize, int type, int angle, void* save_to )
{
    if( *ptr == 0 ) return -1;
    angle &= 3;
    switch( angle )
    {
	case 0:
	    break;
	case 1:
	case 3:
	    {
		size_t p2;
		int p2_xstep;
		int p2_ystep;
		if( angle == 1 )
		{
		    p2 = ysize[ 0 ] - 1;
		    p2_xstep = ysize[ 0 ];
		    p2_ystep = -1;
		}
		else
		{
		    p2 = ( xsize[ 0 ] * ysize[ 0 ] ) - ysize[ 0 ];
		    p2_xstep = -ysize[ 0 ];
		    p2_ystep = 1;
		}
		void* new_data_v;
		if( save_to )
		    new_data_v = save_to;
		else
		    new_data_v = smem_new( xsize[ 0 ] * ysize[ 0 ] * g_pix_container_type_sizes[ type ] );
		if( new_data_v == 0 ) return -1;
		switch( type )
            	{
            	    case PIX_CONTAINER_TYPE_INT8: PIX_VM_ROTATE_BLOCK( int8_t ); break;
            	    case PIX_CONTAINER_TYPE_INT16: PIX_VM_ROTATE_BLOCK( int16_t ); break;
            	    case PIX_CONTAINER_TYPE_INT32: 
            	    case PIX_CONTAINER_TYPE_FLOAT32: 
            		PIX_VM_ROTATE_BLOCK( int ); 
            		break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
            	    case PIX_CONTAINER_TYPE_INT64: 
            	    case PIX_CONTAINER_TYPE_FLOAT64:
            		PIX_VM_ROTATE_BLOCK( int64_t ); 
            		break;
#endif
                }
                if( save_to == 0 )
                {
            	    smem_free( ptr[ 0 ] );
            	    ptr[ 0 ] = new_data_v;
            	}
                PIX_INT temp_xsize = xsize[ 0 ];
                xsize[ 0 ] = ysize[ 0 ];
                ysize[ 0 ] = temp_xsize;
	    }
	    break;
	case 2:
	    switch( type )
    	    {
                case PIX_CONTAINER_TYPE_INT8: PIX_VM_ROTATE_BLOCK2( int8_t ); break;
                case PIX_CONTAINER_TYPE_INT16: PIX_VM_ROTATE_BLOCK2( int16_t ); break;
                case PIX_CONTAINER_TYPE_INT32: 
                case PIX_CONTAINER_TYPE_FLOAT32: 
        	    PIX_VM_ROTATE_BLOCK2( int ); 
        	    break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
            	case PIX_CONTAINER_TYPE_INT64: 
            	case PIX_CONTAINER_TYPE_FLOAT64:
            	    PIX_VM_ROTATE_BLOCK2( int64_t ); 
            	    break;
#endif
            }
    }
    return 0;
}

int pix_vm_rotate_container( PIX_CID cnum, int angle, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	if( vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    if( pix_vm_rotate_block( &c->data, &c->xsize, &c->ysize, c->type, angle, 0 ) != 0 ) return 1;
	}
    }
    
    return 0;
}

int pix_vm_convert_container_type( PIX_CID cnum, int type, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	if( vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    if( c->data == 0 ) return 1;
	    
	    PIX_INT xsize = 1;
	    PIX_INT ysize = 1;
	    if( check_container_parameters( xsize, ysize, type ) == 0 )
	    {
		if( c->type == type ) return 0;
		size_t old_size = c->size * g_pix_container_type_sizes[ c->type ];
		size_t new_size = c->size * g_pix_container_type_sizes[ type ];
		if( new_size > old_size )
		{
		    if( !( c->flags & PIX_CONTAINER_FLAG_STATIC_DATA ) )
			c->data = smem_resize( c->data, new_size );
		    if( c->data == 0 ) return 1;
		}
		size_t i;
		int add;
		if( old_size < new_size )
		{
		    i = c->size - 1;
		    add = -1;
		}
		else 
		{
		    i = 0;
		    add = 1;
		}
		for( size_t i2 = 0; i2 < c->size; i2++ )
		{
		    int8_t t;
		    int64_t v0;
		    double v1;
		    switch( c->type )
		    {
			case PIX_CONTAINER_TYPE_INT8: v0 = ((int8_t*)c->data)[ i ]; t = 0; break;
			case PIX_CONTAINER_TYPE_INT16: v0 = ((int16_t*)c->data)[ i ]; t = 0; break;
			case PIX_CONTAINER_TYPE_INT32: v0 = ((int32_t*)c->data)[ i ]; t = 0; break;
#ifdef PIX_INT64_ENABLED
			case PIX_CONTAINER_TYPE_INT64: v0 = ((int64_t*)c->data)[ i ]; t = 0; break;
#endif
			case PIX_CONTAINER_TYPE_FLOAT32: v1 = ((float*)c->data)[ i ]; t = 1; break;
#ifdef PIX_FLOAT64_ENABLED
			case PIX_CONTAINER_TYPE_FLOAT64: v1 = ((double*)c->data)[ i ]; t = 1; break;
#endif
			default:
			    break;
		    }
		    switch( type )
		    {
			case PIX_CONTAINER_TYPE_INT8: if( t == 0 ) ((int8_t*)c->data)[ i ] = v0; else ((int8_t*)c->data)[ i ] = v1; break;
			case PIX_CONTAINER_TYPE_INT16: if( t == 0 ) ((int16_t*)c->data)[ i ] = v0; else ((int16_t*)c->data)[ i ] = v1; break;
			case PIX_CONTAINER_TYPE_INT32: if( t == 0 ) ((int32_t*)c->data)[ i ] = v0; else ((int32_t*)c->data)[ i ] = v1; break;
#ifdef PIX_INT64_ENABLED
			case PIX_CONTAINER_TYPE_INT64: if( t == 0 ) ((int64_t*)c->data)[ i ] = v0; else ((int64_t*)c->data)[ i ] = v1; break;
#endif
			case PIX_CONTAINER_TYPE_FLOAT32: if( t == 0 ) ((float*)c->data)[ i ] = v0; else ((float*)c->data)[ i ] = v1; break;
#ifdef PIX_FLOAT64_ENABLED
			case PIX_CONTAINER_TYPE_FLOAT64: if( t == 0 ) ((double*)c->data)[ i ] = v0; else ((double*)c->data)[ i ] = v1; break;
#endif
			default:
			    break;
		    }
		    i += add;
		}
		c->type = (pix_container_type)type;
		if( new_size < old_size )
		{
		    c->data = smem_resize( c->data, new_size );
		    if( c->data == 0 ) return 1;
		}
#ifdef OPENGL
                pix_vm_remove_container_gl_data( cnum, vm );
#endif
	    }
	    else
	    {
		return 2;
	    }
	}
    }

    return 0;
}

void pix_vm_clean_container( PIX_CID cnum, int8_t v_type, PIX_VAL v, PIX_INT offset, PIX_INT size, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( c && c->data )
	{
	    if( size < 0 ) size = c->size;
	    if( (unsigned)offset >= (unsigned)c->size ) return;
	    if( offset + size > c->size )
	    {
		size = c->size - offset;
	    }
	    if( size == 0 ) return;
	    if( v_type == 0 && v.i == 0 )
	    {
		smem_clear( (int8_t*)c->data + offset * g_pix_container_type_sizes[ c->type ], size * g_pix_container_type_sizes[ c->type ] );
	    }
	    else 
	    {
		switch( c->type )
		{
		    case PIX_CONTAINER_TYPE_INT8:
			if( v_type == 0 ) 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (int8_t*)c->data )[ i ] = (int8_t)v.i; }
			else 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (int8_t*)c->data )[ i ] = (int8_t)v.f; }
			    break;
		    case PIX_CONTAINER_TYPE_INT16:
			if( v_type == 0 ) 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (int16_t*)c->data )[ i ] = (int16_t)v.i; }
			else 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (int16_t*)c->data )[ i ] = (int16_t)v.f; }
			    break;
		    case PIX_CONTAINER_TYPE_INT32:
			if( v_type == 0 ) 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (int32_t*)c->data )[ i ] = (int32_t)v.i; }
			else 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (int32_t*)c->data )[ i ] = (int32_t)v.f; }
			    break;
#ifdef PIX_INT64_ENABLED
		    case PIX_CONTAINER_TYPE_INT64:
			if( v_type == 0 ) 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (int64_t*)c->data )[ i ] = (int64_t)v.i; }
			else 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (int64_t*)c->data )[ i ] = (int64_t)v.f; }
			    break;
#endif
		    case PIX_CONTAINER_TYPE_FLOAT32:
			if( v_type == 0 ) 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (float*)c->data )[ i ] = (float)v.i; }
			else 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (float*)c->data )[ i ] = (float)v.f; }
			    break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64:
			if( v_type == 0 ) 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (double*)c->data )[ i ] = (double)v.i; }
			else 
			    for( size_t i = offset; i < offset + size; i++ ) { ( (double*)c->data )[ i ] = (double)v.f; }
			    break;
#endif
		}
	    }
	}
    }
}

PIX_CID pix_vm_clone_container( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	
	PIX_CID new_cnum = pix_vm_new_container( -1, c->xsize, c->ysize, c->type, 0, vm );
	if( new_cnum == -1 ) return -1;
	pix_vm_container* c2 = vm->c[ new_cnum ];
	
	size_t data_size = c->xsize * c->ysize * g_pix_container_type_sizes[ c->type ];
	if( data_size )
	    smem_copy( c2->data, c->data, data_size );
	c2->flags = c->flags;
	c2->key = c->key;
	c2->alpha = c->alpha;
	if( c->opt_data )
	{
	    c2->opt_data = (pix_vm_container_opt_data*)smem_new( sizeof( pix_vm_container_opt_data ) );
	    if( c2->opt_data )
	    {
		smem_copy( c2->opt_data, c->opt_data, sizeof( pix_vm_container_opt_data ) );
		pix_symtab_clone( &c2->opt_data->props, &c->opt_data->props );
		if( c->opt_data->hdata )
		{
	    	    pix_vm_clone_container_hdata( new_cnum, cnum, vm );
		}
#ifdef OPENGL
		c2->opt_data->gl = 0;
#endif
	    }
	}
	
	return new_cnum;
    }
    
    return -1;
}

PIX_CID pix_vm_zlib_pack_container( PIX_CID cnum, int level, pix_vm* vm )
{
    PIX_CID rv = -1;
    while( 1 )
    {
	if( (unsigned)cnum > (unsigned)vm->c_num ) break;
	pix_vm_container* c = vm->c[ cnum ];
	if( c == 0 ) break;
	if( c->data == 0 ) break;

	z_stream strm;
	smem_clear( &strm, sizeof( z_stream ) );
	int zlib_err;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	zlib_err = deflateInit( &strm, level );
	if( zlib_err != Z_OK )
	{
    	    PIX_VM_LOG( "zlib deflateInit error %d\n", zlib_err );
    	    break;
	}

	size_t in_size = c->xsize * c->ysize * g_pix_container_type_sizes[ c->type ];
	size_t in_p = 0;
	size_t out_p = 32; //Pixilang zlib header
	size_t zlib_chunk_size = 16384;
	size_t out_size = zlib_chunk_size;
	uint8_t* out = (uint8_t*)smem_new( out_size );
	{
	    //Prepare header:
	    uint8_t* p;
	    smem_clear( out, 32 );
	    p = out; *((PIX_INT*)p) = c->xsize;
	    p = out + 8; *((PIX_INT*)p) = c->ysize;
	    p = out + 16; *p = (uint8_t)c->type;
	}
	if( out )
	{
	    in_p = 0;
	    while( in_p < in_size )
	    {
		int flush = Z_NO_FLUSH;
		size_t size = in_size - in_p;
		if( size > zlib_chunk_size )
		    size = zlib_chunk_size;
		else
		    flush = Z_FINISH;
		strm.avail_in = size;
		strm.next_in = (uint8_t*)c->data + in_p;
		do {
		    size_t avail = out_size - out_p;
		    strm.avail_out = avail;
		    strm.next_out = out + out_p;
		    zlib_err = deflate( &strm, flush );
		    if( zlib_err < 0 ) 
		    {
			PIX_VM_LOG( "zlib deflate error %d\n", zlib_err );
			break;
		    }
		    size_t size2 = avail - strm.avail_out;
		    out_p += size2;
		    if( out_p >= out_size )
		    {
			out_size += zlib_chunk_size;
			out = (uint8_t*)smem_resize( out, out_size );
			if( out == 0 ) break;
		    }
		} while( strm.avail_out == 0 );
		in_p += size;
	    }
	    if( out_p > 0 )
	    {
		out = (uint8_t*)smem_resize( out, out_p );
		if( out )
		{
		    rv = pix_vm_new_container( -1, out_p, 1, PIX_CONTAINER_TYPE_INT8, out, vm );
		}
	    }
	}

	deflateEnd( &strm );
	break;
    }
    return rv;
}

PIX_CID pix_vm_zlib_unpack_container( PIX_CID cnum, pix_vm* vm )
{
    PIX_CID rv = -1;
    while( 1 )
    {
	if( (unsigned)cnum > (unsigned)vm->c_num ) break;
	pix_vm_container* c = vm->c[ cnum ];
	if( c == 0 ) break;
	if( c->data == 0 ) break;
	if( c->size <= 32 ) break;
	
	//Read the header:
	PIX_INT xsize, ysize;
	int type;
	{
	    uint8_t* p = (uint8_t*)c->data;
	    xsize = *((PIX_INT*)p); p += 8;
	    ysize = *((PIX_INT*)p); p += 8;
	    type = *p; p++;
	}
	
	z_stream strm;
	smem_clear( &strm, sizeof( z_stream ) );
	int zlib_err;
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	zlib_err = inflateInit( &strm );
	if( zlib_err != Z_OK )
	{
	    PIX_VM_LOG( "zlib inflateInit error %d\n", zlib_err );
	    break;
	}

	size_t in_p = 0;
	size_t out_p = 0;
	size_t zlib_chunk_size = 16384;
	size_t in_size = c->size - 32;
	size_t out_size = xsize * ysize * g_pix_container_type_sizes[ type ];
	uint8_t* in = (uint8_t*)c->data + 32;
	uint8_t* out = (uint8_t*)smem_new( out_size );
	if( out )
	{
	    in_p = 0;
	    while( in_p < in_size )
	    {
		size_t size = in_size - in_p;
		if( size > zlib_chunk_size )
		    size = zlib_chunk_size;
		strm.avail_in = size;
		strm.next_in = in + in_p;
		do {
		    size_t avail = out_size - out_p;
		    strm.avail_out = avail;
		    strm.next_out = out + out_p;
		    zlib_err = inflate( &strm, Z_NO_FLUSH );
		    if( zlib_err < 0 ) 
		    {
			PIX_VM_LOG( "zlib inflate error %d\n", zlib_err );
			break;
		    }
		    size_t size2 = avail - strm.avail_out;
		    out_p += size2;
		    if( out_p >= out_size ) break;
		} while( strm.avail_out == 0 );
		in_p += size;
	    }
	    rv = pix_vm_new_container( -1, xsize, ysize, type, out, vm );
	}

	inflateEnd( &strm );
	break;
    }
    return rv;
}

PIX_INT pix_vm_get_container_int_element( PIX_CID cnum, size_t elnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( elnum >= c->size ) return 0;
	switch( c->type )
        {
            case PIX_CONTAINER_TYPE_INT8:
        	return ((int8_t*)c->data)[ elnum ];
        	break;
            case PIX_CONTAINER_TYPE_INT16:
        	return ((int16_t*)c->data)[ elnum ];
        	break;
            case PIX_CONTAINER_TYPE_INT32:
        	return ((int32_t*)c->data)[ elnum ];
        	break;
#ifdef PIX_INT64_ENABLED
            case PIX_CONTAINER_TYPE_INT64:
        	return ((int64_t*)c->data)[ elnum ];
        	break;
#endif
            case PIX_CONTAINER_TYPE_FLOAT32:
        	return ((float*)c->data)[ elnum ];
        	break;
#ifdef PIX_FLOAT64_ENABLED
            case PIX_CONTAINER_TYPE_FLOAT64:
        	return ((double*)c->data)[ elnum ];
        	break;
#endif
        }
    }
    
    return 0;
}

PIX_FLOAT pix_vm_get_container_float_element( PIX_CID cnum, size_t elnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( elnum >= c->size ) return 0;
	switch( c->type )
        {
            case PIX_CONTAINER_TYPE_INT8:
        	return ((int8_t*)c->data)[ elnum ];
        	break;
            case PIX_CONTAINER_TYPE_INT16:
        	return ((int16_t*)c->data)[ elnum ];
        	break;
            case PIX_CONTAINER_TYPE_INT32:
        	return ((int32_t*)c->data)[ elnum ];
        	break;
#ifdef PIX_INT64_ENABLED
            case PIX_CONTAINER_TYPE_INT64:
        	return ((int64_t*)c->data)[ elnum ];
        	break;
#endif
            case PIX_CONTAINER_TYPE_FLOAT32:
        	return ((float*)c->data)[ elnum ];
        	break;
#ifdef PIX_FLOAT64_ENABLED        	
            case PIX_CONTAINER_TYPE_FLOAT64:
        	return ((double*)c->data)[ elnum ];
        	break;
#endif
        }
    }
    
    return 0;
}

void pix_vm_set_container_int_element( PIX_CID cnum, size_t elnum, PIX_INT val, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( elnum >= c->size ) return;
	switch( c->type )
        {
            case PIX_CONTAINER_TYPE_INT8:
        	((int8_t*)c->data)[ elnum ] = val;
        	break;
            case PIX_CONTAINER_TYPE_INT16:
        	((int16_t*)c->data)[ elnum ] = val;
        	break;
            case PIX_CONTAINER_TYPE_INT32:
        	((int32_t*)c->data)[ elnum ] = val;
        	break;
#ifdef PIX_INT64_ENABLED
            case PIX_CONTAINER_TYPE_INT64:
        	((int64_t*)c->data)[ elnum ] = val;
        	break;
#endif
            case PIX_CONTAINER_TYPE_FLOAT32:
        	((float*)c->data)[ elnum ] = val;
        	break;
#ifdef PIX_FLOAT64_ENABLED
            case PIX_CONTAINER_TYPE_FLOAT64:
        	((double*)c->data)[ elnum ] = val;
        	break;
#endif
        }
    }
}

void pix_vm_set_container_float_element( PIX_CID cnum, size_t elnum, PIX_FLOAT val, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( elnum >= c->size ) return;
	switch( c->type )
        {
            case PIX_CONTAINER_TYPE_INT8:
        	((int8_t*)c->data)[ elnum ] = val;
        	break;
            case PIX_CONTAINER_TYPE_INT16:
        	((int16_t*)c->data)[ elnum ] = val;
        	break;
            case PIX_CONTAINER_TYPE_INT32:
        	((int32_t*)c->data)[ elnum ] = val;
        	break;
#ifdef PIX_INT64_ENABLED
            case PIX_CONTAINER_TYPE_INT64:
        	((int64_t*)c->data)[ elnum ] = val;
        	break;
#endif
            case PIX_CONTAINER_TYPE_FLOAT32:
        	((float*)c->data)[ elnum ] = val;
        	break;
#ifdef PIX_FLOAT64_ENABLED
            case PIX_CONTAINER_TYPE_FLOAT64:
        	((double*)c->data)[ elnum ] = val;
        	break;
#endif
        }
    }
}

size_t pix_vm_get_container_strlen( PIX_CID cnum, size_t offset, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( c )
	{
	    size_t size = c->size * g_pix_container_type_sizes[ c->type ];
	    size_t len;
	    if( offset >= size )
		len = 0;
	    else
	    {
		char* s = (char*)c->data;
		for( len = offset; len < size; len++ )
		    if( s[ len ] == 0 ) break;
		len -= offset;
	    }
	    return len;
	}
    }
    
    return 0;
}

char* pix_vm_make_cstring_from_container( PIX_CID cnum, bool* need_to_free, pix_vm* vm )
{
    *need_to_free = 0;

    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( c )
	{
	    size_t size = c->size * g_pix_container_type_sizes[ c->type ];
	    size_t str_len;
	    char* str_ptr = (char*)c->data;
	    for( str_len = 0; str_len < size; str_len++ )
		if( str_ptr[ str_len ] == 0 ) break;
	    if( str_len == size )
	    {
		char* str = (char*)smem_new( str_len + 1 );
		smem_copy( str, str_ptr, str_len );
		str[ str_len ] = 0;
		*need_to_free = 1;
		return str;
	    }
	    else
	    {
		*need_to_free = 0;
		return str_ptr;
	    }
	}
    }

    return NULL;
}

PIX_CID pix_vm_make_container_from_cstring( const char* str, pix_vm* vm )
{
    PIX_CID rv = -1;
    if( str )
    {
        size_t l = smem_strlen( str );
        if( l )
        {
    	    rv = pix_vm_new_container( -1, l, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	    smem_copy( pix_vm_get_container_data( rv, vm ), str, l );
    	}
    }
    return rv;
}

//
// Properties
//

pix_sym* pix_vm_get_container_property( PIX_CID cnum, const char* prop_name, int prop_hash, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( c && c->opt_data )
	{
	    return pix_symtab_lookup( prop_name, prop_hash, 0, SYMTYPE_NUM_I, 0, 0, 0, &c->opt_data->props );
	}
    }

    return 0;
}

PIX_INT pix_vm_get_container_property_i( PIX_CID cnum, const char* prop_name, int prop_hash, pix_vm* vm )
{
    pix_sym* s = pix_vm_get_container_property( cnum, prop_name, prop_hash, vm );
    if( s )
    {
	if( s->type == SYMTYPE_NUM_F )
	    return (PIX_INT)s->val.f;
	else
	    return s->val.i;
    }
    return 0;
}

void pix_vm_set_container_property( PIX_CID cnum, const char* prop_name, int prop_hash, int8_t val_type, PIX_VAL val, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( c )
	{
	    if( !c->opt_data )
	    {
		c->opt_data = (pix_vm_container_opt_data*)smem_new( sizeof( pix_vm_container_opt_data ) );
		smem_zero( c->opt_data );
		if( !c->opt_data ) return;
	    }
	    if( !c->opt_data->props.symtab )
	    {
		pix_symtab_init( PIX_CONTAINER_SYMTAB_SIZE, &c->opt_data->props );
	    }
	    pix_sym* sym = pix_symtab_lookup( prop_name, prop_hash, 1, SYMTYPE_NUM_I, 0, 0, 0, &c->opt_data->props );
	    if( sym )
	    {
		if( val_type == 0 )
		    sym->type = SYMTYPE_NUM_I;
		else
		    sym->type = SYMTYPE_NUM_F;
		sym->val = val;
	    }
	}
    }
}

//
// Hidden data
//

void* pix_vm_get_container_hdata( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
	if( c && c->opt_data && c->opt_data->hdata )
	{
	    return c->opt_data->hdata;
	}
    }
    
    return 0;
}

int pix_vm_create_container_hdata( PIX_CID cnum, uint8_t hdata_type, size_t hdata_size, pix_vm* vm )
{
    int rv = -1;

    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
	    if( c->opt_data == 0 )
	    {
		c->opt_data = (pix_vm_container_opt_data*)smem_new( sizeof( pix_vm_container_opt_data ) );
		smem_zero( c->opt_data );
	    }
	    if( c->opt_data )
	    {
		c->opt_data->hdata = smem_new( hdata_size );
		if( c->opt_data->hdata )
		{
		    smem_zero( c->opt_data->hdata );
		    ((uint8_t*)c->opt_data->hdata)[ 0 ] = hdata_type;
		    rv = 0;
		}
	    }
	}
    }
    
    return rv;
}

void pix_vm_remove_container_hdata( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c && c->opt_data && c->opt_data->hdata )
        {
    	    int hdata_type = ((uint8_t*)c->opt_data->hdata)[ 0 ];
    	    switch( hdata_type )
    	    {
    		case pix_vm_container_hdata_type_anim:
		    {
			pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
			for( int i = 0; i < hdata->frame_count; i++ )
			{
			    smem_free( hdata->frames[ i ].pixels );
			}
			smem_free( hdata->frames );
		    }
    		    break;
    		default:
    		    break;
    	    }
    	    smem_free( c->opt_data->hdata );
    	    c->opt_data->hdata = 0;
        }
    }
}

size_t pix_vm_get_container_hdata_size( PIX_CID cnum, pix_vm* vm )
{
    size_t rv = 0;
    
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c && c->opt_data && c->opt_data->hdata )
        {
	    rv += 1; //type
    	    int hdata_type = ((uint8_t*)c->opt_data->hdata)[ 0 ];
    	    switch( hdata_type )
    	    {
    		case pix_vm_container_hdata_type_anim:
		    {
			rv += 4; //frame_count
			pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
			for( int i = 0; i < hdata->frame_count; i++ )
			{
			    rv += 1; //frame type
			    rv += 8; //xsize
			    rv += 8; //ysize
			    rv += 8; //data size
			    rv += smem_get_size( hdata->frames[ i ].pixels );
			}
		    }
    		    break;
    		default:
    		    break;
    	    }
        }
    }
    
    return rv;
}

size_t pix_vm_save_container_hdata( PIX_CID cnum, sfs_file f, pix_vm* vm )
{
    size_t rv = 0;
    
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c && c->opt_data && c->opt_data->hdata )
        {
    	    int hdata_type = ((uint8_t*)c->opt_data->hdata)[ 0 ];
	    if( sfs_putc( hdata_type, f ) == EOF ) return rv;
	    rv += 1;
    	    switch( hdata_type )
    	    {
    		case pix_vm_container_hdata_type_anim:
		    {
			pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
			if( sfs_write( &hdata->frame_count, 1, 4, f ) != 4 ) return rv;
			rv += 4;
			for( int i = 0; i < hdata->frame_count; i++ )
			{
			    pix_vm_anim_frame* frame = &hdata->frames[ i ];
			    if( sfs_putc( frame->type, f ) == EOF ) return rv;
			    rv += 1;
			    uint64_t v;
			    size_t size = smem_get_size( frame->pixels );
			    v = frame->xsize; if( sfs_write( &v, 1, sizeof( uint64_t ), f ) != sizeof( uint64_t ) ) return rv;
			    v = frame->ysize; if( sfs_write( &v, 1, sizeof( uint64_t ), f ) != sizeof( uint64_t ) ) return rv;
			    v = size; if( sfs_write( &v, 1, sizeof( uint64_t ), f ) != sizeof( uint64_t ) ) return rv;
			    rv += 8;
			    rv += 8;
			    rv += 8;
			    if( sfs_write( frame->pixels, 1, size, f ) != size ) return rv;
			    rv += size;
			}
		    }
    		    break;
    		default:
    		    break;
    	    }
        }
    }
    
    return rv;
}

size_t pix_vm_load_container_hdata( PIX_CID cnum, sfs_file f, pix_vm* vm )
{
    size_t rv = 0;

    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c == 0 ) return 0;
            
	int hdata_type = sfs_getc( f );
	if( hdata_type < 0 ) return rv;
	rv += 1;
	switch( hdata_type )
	{
    	    case pix_vm_container_hdata_type_anim:
        	{
	    	    if( pix_vm_create_container_hdata( cnum, hdata_type, sizeof( pix_vm_container_hdata_anim ), vm ) ) return rv;
	    	    if( c->opt_data == 0 ) return rv;
	    	    if( c->opt_data->hdata == 0 ) return rv;
	    	    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
	    	    if( sfs_read( &hdata->frame_count, 1, 4, f ) != 4 ) return rv;
	    	    rv += 4;
	    	    hdata->frames = (pix_vm_anim_frame*)smem_new( sizeof( pix_vm_anim_frame ) * hdata->frame_count );
	    	    if( hdata->frames == 0 ) return rv;
	    	    smem_zero( hdata->frames );
	    	    for( int i = 0; i < hdata->frame_count; i++ )
            	    {
            		pix_vm_anim_frame* frame = &hdata->frames[ i ];
            		int frame_type = sfs_getc( f );
            		if( frame_type < 0 ) return rv;
            		frame->type = (pix_container_type)frame_type;
            		rv += 1;
            		uint64_t xsize = 0;
            		uint64_t ysize = 0;
            		uint64_t size = 0;
            		if( sfs_read( &xsize, 1, sizeof( uint64_t ), f ) != sizeof( uint64_t ) ) return rv;
            		if( sfs_read( &ysize, 1, sizeof( uint64_t ), f ) != sizeof( uint64_t ) ) return rv;
            		if( sfs_read( &size, 1, sizeof( uint64_t ), f ) != sizeof( uint64_t ) ) return rv;
            		frame->xsize = (PIX_INT)xsize;
            		frame->ysize = (PIX_INT)ysize;
            		rv += 8;
            		rv += 8;
            		rv += 8;
            		frame->pixels = (COLORPTR)smem_new( size );
            		if( frame->pixels == 0 ) return rv;
            		if( sfs_read( frame->pixels, 1, size, f ) != size ) return rv;
            		rv += size;
        	    }
		}
		break;
	    default:
		break;
	}
    }
    
    return rv;
}

int pix_vm_clone_container_hdata( PIX_CID new_cnum, PIX_CID old_cnum, pix_vm* vm )
{
    int rv = -1;

    if( (unsigned)new_cnum < (unsigned)vm->c_num && (unsigned)old_cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c2 = vm->c[ new_cnum ];
        pix_vm_container* c1 = vm->c[ old_cnum ];
        if( c1 && c1->opt_data && c1->opt_data->hdata && c2 )
        {
    	    if( c2->opt_data == 0 )
    	    {
    		c2->opt_data = (pix_vm_container_opt_data*)smem_new( sizeof( pix_vm_container_opt_data ) );
    		if( c2->opt_data == 0 ) return -1;
    		smem_zero( c2->opt_data );
    	    }
    	    int hdata_type = ((uint8_t*)c1->opt_data->hdata)[ 0 ];
    	    size_t hdata_size = smem_get_size( c1->opt_data->hdata );
    	    c2->opt_data->hdata = smem_new( hdata_size );
    	    if( c2->opt_data->hdata == 0 ) return -1;
    	    smem_copy( c2->opt_data->hdata, c1->opt_data->hdata, hdata_size );
    	    switch( hdata_type )
    	    {
    		case pix_vm_container_hdata_type_anim:
		    {
			pix_vm_container_hdata_anim* hdata1 = (pix_vm_container_hdata_anim*)c1->opt_data->hdata;
			pix_vm_container_hdata_anim* hdata2 = (pix_vm_container_hdata_anim*)c2->opt_data->hdata;
			hdata2->frames = (pix_vm_anim_frame*)smem_new( sizeof( pix_vm_anim_frame ) * hdata1->frame_count );
			if( hdata2->frames == 0 ) return -1;
			smem_zero( hdata2->frames );
			for( int i = 0; i < hdata1->frame_count; i++ )
			{
			    smem_copy( &hdata2->frames[ i ], &hdata1->frames[ i ], sizeof( pix_vm_anim_frame ) );
			    size_t frame_size = smem_get_size( hdata1->frames[ i ].pixels );
			    hdata2->frames[ i ].pixels = (COLORPTR)smem_new( frame_size );
			    if( hdata2->frames[ i ].pixels )
			    {
				smem_copy( hdata2->frames[ i ].pixels, hdata1->frames[ i ].pixels, frame_size );
			    }
			}
		    }
    		    break;
    		default:
    		    break;
    	    }
	    rv = 0;
        }
    }
    
    return rv;
}

PIX_INT pix_vm_container_get_cur_frame( PIX_CID cnum, pix_vm* vm )
{
    pix_vm_container* c = pix_vm_get_container( cnum, vm );
    if( c && c->opt_data && c->opt_data->hdata )
    {
	PIX_INT repeat = pix_vm_get_container_property_i( cnum, "repeat", -1, vm );
	PIX_INT cur_frame = pix_vm_get_container_property_i( cnum, "frame", -1, vm );
	if( cur_frame < 0 ) cur_frame = 0;
	switch( ((uint8_t*)c->opt_data->hdata)[ 0 ] )
	{
	    case pix_vm_container_hdata_type_anim:
		{
		    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
		    if( repeat >= 0 )
		    {
			if( cur_frame >= ( 1 + repeat ) * hdata->frame_count )
			    cur_frame = ( 1 + repeat ) * hdata->frame_count - 1;
		    }
		    cur_frame %= hdata->frame_count;
		}
		break;
	}
	return cur_frame;
    }
    return 0;
}

int pix_vm_container_hdata_get_frame_count( PIX_CID cnum, pix_vm* vm )
{
    int rv = 0;
    
    pix_vm_container* c = pix_vm_get_container( cnum, vm );
    if( c && c->opt_data && c->opt_data->hdata )
    {
	switch( ((uint8_t*)c->opt_data->hdata)[ 0 ] )
	{
	    case pix_vm_container_hdata_type_anim:
		{
		    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
		    if( hdata->frames )
		    {
			rv = hdata->frame_count;
		    }
		}
		break;
	}
    }
    
    return rv;
}

int pix_vm_container_hdata_get_frame_size( PIX_CID cnum, int cur_frame, pix_container_type* type, int* xsize, int* ysize, pix_vm* vm )
{
    int rv = -1;
    
    pix_vm_container* c = pix_vm_get_container( cnum, vm );
    if( c && c->opt_data && c->opt_data->hdata )
    {
	switch( ((uint8_t*)c->opt_data->hdata)[ 0 ] )
	{
	    case pix_vm_container_hdata_type_anim:
		{
		    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
		    if( hdata->frames )
		    {
			pix_vm_anim_frame* f = &hdata->frames[ cur_frame ];
			if( type ) *type = f->type;
			if( xsize ) *xsize = f->xsize;
			if( ysize ) *ysize = f->ysize;
			rv = 0;
		    }
		}
		break;
	}
    }
    
    return rv;
}

int pix_vm_container_hdata_unpack_frame_to_buf( PIX_CID cnum, int cur_frame, COLORPTR buf, pix_vm* vm )
{
    int rv = -1;
    if( buf == 0 ) return -1;
    
    pix_vm_container* c = pix_vm_get_container( cnum, vm );
    if( c && c->opt_data && c->opt_data->hdata )
    {
#ifdef OPENGL
	pix_vm_remove_container_gl_data( cnum, vm );
#endif
	switch( ((uint8_t*)c->opt_data->hdata)[ 0 ] )
	{
	    case pix_vm_container_hdata_type_anim:
		{
		    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
		    if( hdata->frames )
		    {
			pix_vm_anim_frame* f = &hdata->frames[ cur_frame ];
			
			z_stream strm;
			smem_clear( &strm, sizeof( z_stream ) );
			int zlib_err;
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			strm.avail_in = 0;
			strm.next_in = Z_NULL;
			zlib_err = inflateInit( &strm );
			if( zlib_err != Z_OK )
			{
			    PIX_VM_LOG( "zlib inflateInit error %d\n", zlib_err );
			    break;
			}
			
			size_t in_p = 0;
			size_t out_p = 0;
			size_t zlib_chunk_size = 16384;
			size_t in_size = smem_get_size( f->pixels );
			size_t out_size = f->xsize * f->ysize * g_pix_container_type_sizes[ f->type ];
			uint8_t* in = (uint8_t*)f->pixels;
			uint8_t* out = (uint8_t*)buf;
			if( out )
			{
			    in_p = 0;
			    while( in_p < in_size )
			    {
				size_t size = in_size - in_p;
				if( size > zlib_chunk_size )
				    size = zlib_chunk_size;
				strm.avail_in = size;
				strm.next_in = in + in_p;
				do {
				    size_t avail = out_size - out_p;
				    strm.avail_out = avail;
				    strm.next_out = out + out_p;
				    zlib_err = inflate( &strm, Z_NO_FLUSH );
				    if( zlib_err < 0 ) 
				    {
					PIX_VM_LOG( "zlib inflate error %d\n", zlib_err );
					break;
				    }
				    size_t size2 = avail - strm.avail_out;
				    out_p += size2;
				    if( out_p >= out_size ) break;
				} while( strm.avail_out == 0 );
				in_p += size;
			    }
			    rv = 0;
			}
			
			inflateEnd( &strm );
		    }
		}
		break;
	}
    }
    
    return rv;
}

int pix_vm_container_hdata_unpack_frame( PIX_CID cnum, pix_vm* vm )
{
    int rv = -1;
    pix_container_type type;
    int xsize, ysize;
    int cur_frame = pix_vm_container_get_cur_frame( cnum, vm );
    rv = pix_vm_container_hdata_get_frame_size( cnum, cur_frame, &type, &xsize, &ysize, vm );
    if( rv < 0 ) return rv;
    pix_vm_resize_container( cnum, xsize, ysize, type, 0, vm );
    COLORPTR cdata = (COLORPTR)pix_vm_get_container_data( cnum, vm );
    if( cdata )
    {
	rv = pix_vm_container_hdata_unpack_frame_to_buf( cnum, cur_frame, cdata, vm );
    }
    else rv = -1;
    return rv;
}

int pix_vm_container_hdata_pack_frame_from_buf( PIX_CID cnum, int cur_frame, COLORPTR buf, pix_container_type type, int xsize, int ysize, pix_vm* vm )
{
    int rv = -1;
    if( buf == 0 ) return -1;
    
    pix_vm_container* c = pix_vm_get_container( cnum, vm );
    if( c && c->opt_data && c->opt_data->hdata )
    {
	PIX_INT repeat = pix_vm_get_container_property_i( cnum, "repeat", -1, vm );
	switch( ((uint8_t*)c->opt_data->hdata)[ 0 ] )
	{
	    case pix_vm_container_hdata_type_anim:
		{
		    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
		    if( hdata->frames )
		    {
			pix_vm_anim_frame* f = &hdata->frames[ cur_frame ];
			
			z_stream strm;
			smem_clear( &strm, sizeof( z_stream ) );
			int zlib_err;
			strm.zalloc = Z_NULL;
			strm.zfree = Z_NULL;
			strm.opaque = Z_NULL;
			zlib_err = deflateInit( &strm, 2 );
			if( zlib_err != Z_OK )
			{
			    PIX_VM_LOG( "zlib deflateInit error %d\n", zlib_err );
			    break;
			}
			
			size_t frame_size = xsize * ysize * g_pix_container_type_sizes[ type ];
			size_t in_p = 0;
			size_t out_p = 0;
			size_t zlib_chunk_size = 16384;
			size_t out_size = zlib_chunk_size;
			uint8_t* out = (uint8_t*)smem_new( out_size );
			if( out )
			{
			    in_p = 0;
			    while( in_p < frame_size )
			    {
				int flush = Z_NO_FLUSH;
				size_t size = frame_size - in_p;
				if( size > zlib_chunk_size )
				    size = zlib_chunk_size;
				else
				    flush = Z_FINISH;
				strm.avail_in = size;
				strm.next_in = (uint8_t*)buf + in_p;
				do {
				    size_t avail = out_size - out_p;
				    strm.avail_out = avail;
				    strm.next_out = out + out_p;
				    zlib_err = deflate( &strm, flush );
				    if( zlib_err < 0 ) 
				    {
					PIX_VM_LOG( "zlib deflate error %d\n", zlib_err );
					break;
				    }
				    size_t size2 = avail - strm.avail_out;
				    out_p += size2;
				    if( out_p >= out_size )
				    {
					out_size += zlib_chunk_size;
					out = (uint8_t*)smem_resize( out, out_size );
					if( out == 0 ) break;
				    }
				} while( strm.avail_out == 0 );
				in_p += size;
			    }
			    if( out_p > 0 )
			    {
				out = (uint8_t*)smem_resize( out, out_p );
				if( out )
				{
				    smem_free( f->pixels );
				    f->type = type;
				    f->xsize = xsize;
				    f->ysize = ysize;
				    f->pixels = (COLORPTR)out;
				    rv = 0;
				}
			    }
			}
			
			deflateEnd( &strm );
		    }
		}
		break;
	}
    }
    
    return rv;
}

int pix_vm_container_hdata_pack_frame( PIX_CID cnum, pix_vm* vm )
{
    int rv = -1;
    pix_vm_container* c = pix_vm_get_container( cnum, vm );
    if( c && c->data )
    {
	int cur_frame = pix_vm_container_get_cur_frame( cnum, vm );
	rv = pix_vm_container_hdata_pack_frame_from_buf( cnum, cur_frame, (COLORPTR)c->data, c->type, c->xsize, c->ysize, vm );
    }
    return rv;
}

int pix_vm_container_hdata_clone_frame( PIX_CID cnum, pix_vm* vm )
{	
    int rv = -1;
    
    pix_vm_container* c = pix_vm_get_container( cnum, vm );
    if( c && c->opt_data && c->opt_data->hdata )
    {
	int cur_frame = pix_vm_container_get_cur_frame( cnum, vm );
	switch( ((uint8_t*)c->opt_data->hdata)[ 0 ] )
	{
	    case pix_vm_container_hdata_type_anim:
		{
		    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
		    hdata->frames = (pix_vm_anim_frame*)smem_resize( hdata->frames, smem_get_size( hdata->frames ) + sizeof( pix_vm_anim_frame ) );
		    if( hdata->frames )
		    {
			hdata->frame_count++;
			PIX_VAL prop_val;
			prop_val.i = hdata->frame_count; pix_vm_set_container_property( cnum, "frames", -1, 0, prop_val, vm );
			for( int i = hdata->frame_count - 1; i > cur_frame; i-- )
			{
			    smem_copy( &hdata->frames[ i ], &hdata->frames[ i - 1 ], sizeof( pix_vm_anim_frame ) );
			}
			pix_vm_anim_frame* f = &hdata->frames[ cur_frame ];
			pix_vm_anim_frame* f2 = &hdata->frames[ cur_frame + 1 ];
			f->pixels = (COLORPTR)smem_new( smem_get_size( f2->pixels ) );
			if( f->pixels )
			{
			    smem_copy( f->pixels, f2->pixels, smem_get_size( f2->pixels ) );
			    rv = 0;
			}
		    }
		}
		break;
	}
    }
    
    return rv;
}

int pix_vm_container_hdata_remove_frame( PIX_CID cnum, pix_vm* vm )
{	
    int rv = -1;
    
    pix_vm_container* c = pix_vm_get_container( cnum, vm );
    if( c && c->opt_data && c->opt_data->hdata )
    {
	int cur_frame = pix_vm_container_get_cur_frame( cnum, vm );
	switch( ((uint8_t*)c->opt_data->hdata)[ 0 ] )
	{
	    case pix_vm_container_hdata_type_anim:
		{
		    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
		    if( hdata->frames && hdata->frame_count > 1 )
		    {
			pix_vm_anim_frame* f = &hdata->frames[ cur_frame ];
			smem_free( f->pixels );
			for( int i = cur_frame; i < hdata->frame_count; i++ )
			{
			    smem_copy( &hdata->frames[ i ], &hdata->frames[ i + 1 ], sizeof( pix_vm_anim_frame ) );
			}
			hdata->frame_count--;
			PIX_VAL prop_val;
			prop_val.i = hdata->frame_count; pix_vm_set_container_property( cnum, "frames", -1, 0, prop_val, vm );
			hdata->frames = (pix_vm_anim_frame*)smem_resize( hdata->frames, hdata->frame_count * sizeof( pix_vm_anim_frame ) );
			if( hdata->frames )
			    rv = 0;
		    }
		}
		break;
	}
    }
    
    return rv;
}

int pix_vm_container_hdata_autoplay_control( PIX_CID cnum, pix_vm* vm )
{
    int rv = -1;
    
    PIX_VAL prop_val;

    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c && c->opt_data && c->opt_data->hdata )
        {
	    if( pix_vm_get_container_property_i( cnum, "play", -1, vm ) )
	    {
		uint frame_count = 0;
		int hdata_type = ((uint8_t*)c->opt_data->hdata)[ 0 ];
		switch( hdata_type )
		{
		    case pix_vm_container_hdata_type_anim:
			{
			    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)c->opt_data->hdata;
			    frame_count = hdata->frame_count;
			}
			break;
		    default:
			break;
		}
		if( frame_count > 1 )
		{
		    PIX_INT start_frame = pix_vm_get_container_property_i( cnum, "start_frame", -1, vm );
		    uint start_time = pix_vm_get_container_property_i( cnum, "start_time", -1, vm );
		    uint cur_time = (uint)stime_ticks();
		    uint t = cur_time - start_time;
		    PIX_INT fps = pix_vm_get_container_property_i( cnum, "fps", -1, vm );
		    if( fps <= 0 ) fps = 1;
		    PIX_INT repeat = pix_vm_get_container_property_i( cnum, "repeat", -1, vm );
		    int64_t ff = ( t * fps ) / stime_ticks_per_second();
		    PIX_INT f = start_frame + ff;

		    if( repeat >= 0 )
		    {
			if( f >= ( 1 + repeat ) * frame_count )
			{
			    prop_val.i = 0; pix_vm_set_container_property( cnum, "play", -1, 0, prop_val, vm );
			    f = ( 1 + repeat ) * frame_count - 1;
			}
		    }

		    PIX_INT prev_frame = pix_vm_get_container_property_i( cnum, "frame", -1, vm );
		    if( f != prev_frame )
		    {
			prop_val.i = f; pix_vm_set_container_property( cnum, "frame", -1, 0, prop_val, vm );
			rv = f % frame_count;
		    }
		}
	    }
	}
    }
    
    return rv;
}
