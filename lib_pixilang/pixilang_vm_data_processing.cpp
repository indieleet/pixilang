/*
    pixilang_vm_data_processing.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"

#include "dsp.h"

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) slog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif

#define OP_MIN( CONT_IS_FLOAT, TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	if( CONT_IS_FLOAT == 0 ) \
	    { retval_type[ 0 ] = 0; retval[ 0 ].i = ( (TYPE*)data )[ i ]; } \
	else \
	    { retval_type[ 0 ] = 1; retval[ 0 ].f = ( (TYPE*)data )[ i ]; } \
	for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( CONT_IS_FLOAT == 0 ) \
		{ PIX_INT v = ( (TYPE*)data )[ i ]; if( v < retval[ 0 ].i ) retval[ 0 ].i = v; } \
	    else \
		{ PIX_FLOAT v = ( (TYPE*)data )[ i ]; if( v < retval[ 0 ].f ) retval[ 0 ].f = v; } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_MAX( CONT_IS_FLOAT, TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	if( CONT_IS_FLOAT == 0 ) \
	    { retval_type[ 0 ] = 0; retval[ 0 ].i = ( (TYPE*)data )[ i ]; } \
	else \
	    { retval_type[ 0 ] = 1; retval[ 0 ].f = ( (TYPE*)data )[ i ]; } \
	for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( CONT_IS_FLOAT == 0 ) \
		{ PIX_INT v = ( (TYPE*)data )[ i ]; if( v > retval[ 0 ].i ) retval[ 0 ].i = v; } \
	    else \
		{ PIX_FLOAT v = ( (TYPE*)data )[ i ]; if( v > retval[ 0 ].f ) retval[ 0 ].f = v; } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_MAXABS( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	PIX_FLOAT vv; \
	if( val_type == 0 ) vv = val.i; else vv = val.f; \
	retval_type[ 0 ] = 1; retval[ 0 ].f = ( (TYPE*)data )[ i ] + vv; \
	for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    PIX_FLOAT v = ( (TYPE*)data )[ i ] + vv; if( v < 0 ) v = -v; if( v > retval[ 0 ].f ) retval[ 0 ].f = v; \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_MAXABS_INT( TYPE, MIN, MAX ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	PIX_INT vv; \
	if( val_type == 0 ) vv = val.i; else vv = val.f; \
	retval_type[ 0 ] = 0; retval[ 0 ].i = ( (TYPE*)data )[ i ] + vv; \
	for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    PIX_INT v = ( (TYPE*)data )[ i ] + vv; if( v < 0 ) { if( v == MIN ) v = MAX; else v = -v; } if( v > retval[ 0 ].i ) retval[ 0 ].i = v; \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_SUM( CONT_IS_FLOAT, TYPE ) \
    { \
	if( CONT_IS_FLOAT == 0 ) \
	    { retval_type[ 0 ] = 0; retval[ 0 ].i = 0; } \
	else \
	    { retval_type[ 0 ] = 1; retval[ 0 ].f = 0; } \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( CONT_IS_FLOAT == 0 ) \
		{ retval[ 0 ].i += ( (TYPE*)data )[ i ]; } \
	    else \
		{ retval[ 0 ].f += ( (TYPE*)data )[ i ]; } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_ADD( SUB, TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( SUB ) \
		{ if( val_type == 0 ) ( (TYPE*)data )[ i ] -= val.i; else ( (TYPE*)data )[ i ] -= val.f; } \
	    else \
		{ if( val_type == 0 ) ( (TYPE*)data )[ i ] += val.i; else ( (TYPE*)data )[ i ] += val.f; } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_SADD( SUB, TYPE, MIN, MAX ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    PIX_INT v; \
	    if( SUB ) \
		{ if( val_type == 0 ) v = ( (TYPE*)data )[ i ] - val.i; else v = ( (TYPE*)data )[ i ] - val.f; } \
	    else \
		{ if( val_type == 0 ) v = ( (TYPE*)data )[ i ] + val.i; else v = ( (TYPE*)data )[ i ] + val.f; } \
	    if( v < MIN ) v = MIN; \
	    if( v > MAX ) v = MAX; \
	    ( (TYPE*)data )[ i ] = v; \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_SUB2( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) ( (TYPE*)data )[ i ] = (TYPE)val.i - ( (TYPE*)data )[ i ]; else ( (TYPE*)data )[ i ] = (TYPE)val.f - ( (TYPE*)data )[ i ]; \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_COLOR_ADD() \
    { \
	COLOR v2; if( val_type == 0 ) v2 = (COLOR)val.i; else v2 = (COLOR)val.f; \
	if( v2 & COLORMASK ) { \
	    int r2 = red( v2 ); int g2 = green( v2 ); int b2 = blue( v2 ); \
    	    PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		COLOR v1 = ( (COLORPTR)data )[ i ]; \
		int r1 = red( v1 ); int g1 = green( v1 ); int b1 = blue( v1 ); \
		r1 += r2; if( r1 > 255 ) r1 = 255; \
		g1 += g2; if( g1 > 255 ) g1 = 255; \
		b1 += b2; if( b1 > 255 ) b1 = 255; \
		( (COLORPTR)data )[ i ] = get_color( r1, g1, b1 ); \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_COLOR_SUB() \
    { \
	COLOR v2; if( val_type == 0 ) v2 = (COLOR)val.i; else v2 = (COLOR)val.f; \
	if( v2 & COLORMASK ) { \
	    int r2 = red( v2 ); int g2 = green( v2 ); int b2 = blue( v2 ); \
    	    PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		COLOR v1 = ( (COLORPTR)data )[ i ]; \
		int r1 = red( v1 ); int g1 = green( v1 ); int b1 = blue( v1 ); \
		r1 -= r2; if( r1 < 0 ) r1 = 0; \
		g1 -= g2; if( g1 < 0 ) g1 = 0; \
		b1 -= b2; if( b1 < 0 ) b1 = 0; \
		( (COLORPTR)data )[ i ] = get_color( r1, g1, b1 ); \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_COLOR_SUB2() \
    { \
	COLOR v2; if( val_type == 0 ) v2 = (COLOR)val.i; else v2 = (COLOR)val.f; \
	if( v2 & COLORMASK ) { \
	    int r2 = red( v2 ); int g2 = green( v2 ); int b2 = blue( v2 ); \
    	    PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		COLOR v1 = ( (COLORPTR)data )[ i ]; \
		int r1 = r2 - red( v1 ); int g1 = g2 - green( v1 ); int b1 = b2 - blue( v1 ); \
		if( r1 < 0 ) r1 = 0; \
		if( g1 < 0 ) g1 = 0; \
		if( b1 < 0 ) b1 = 0; \
		( (COLORPTR)data )[ i ] = get_color( r1, g1, b1 ); \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_MUL( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	if( val_type == 0 ) { \
	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		( (TYPE*)data )[ i ] *= val.i; \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
	else { \
	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		( (TYPE*)data )[ i ] *= val.f; \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_SMUL( TYPE, MIN, MAX ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	if( val_type == 0 ) { \
	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		PIX_INT v; \
		v = ( (TYPE*)data )[ i ] * val.i; \
		if( v < MIN ) v = MIN; \
		if( v > MAX ) v = MAX; \
		( (TYPE*)data )[ i ] = v; \
		i++; \
	    } i += c->xsize - xsize; } \
	} else { \
	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		PIX_INT v; \
		v = ( (TYPE*)data )[ i ] * val.f; \
		if( v < MIN ) v = MIN; \
		if( v > MAX ) v = MAX; \
		( (TYPE*)data )[ i ] = v; \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_MUL_RSHIFT15( TYPE ) \
    { \
        PIX_INT i = y * c->xsize + x; \
        if( val_type == 0 ) { \
    	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
            	PIX_INT v = ( (TYPE*)data )[ i ]; v = ( v * val.i ) >> 15; ( (TYPE*)data )[ i ] = v; \
        	i++; \
    	    } i += c->xsize - xsize; } \
    	} else { \
    	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
            	PIX_FLOAT v = ( (TYPE*)data )[ i ]; PIX_INT v2 = (PIX_INT)( v * val.f ) >> 15; ( (TYPE*)data )[ i ] = v2; \
        	i++; \
    	    } i += c->xsize - xsize; } \
    	} \
    }
#define OP_MUL_RSHIFT14( TYPE ) \
    { \
        PIX_INT i = y * c->xsize + x; \
        if( val_type == 0 ) { \
    	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
            	PIX_INT v = ( (TYPE*)data )[ i ]; v = ( v * val.i ) >> 14; ( (TYPE*)data )[ i ] = v; \
        	i++; \
    	    } i += c->xsize - xsize; } \
    	} else { \
    	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
            	PIX_FLOAT v = ( (TYPE*)data )[ i ]; PIX_INT v2 = (PIX_INT)( v * val.f ) >> 15; ( (TYPE*)data )[ i ] = v2; \
        	i++; \
    	    } i += c->xsize - xsize; } \
    	} \
    }
#define OP_COLOR_MUL() \
    { \
	COLOR v2; if( val_type == 0 ) v2 = (COLOR)val.i; else v2 = (COLOR)val.f; \
	if( ( v2 & COLORMASK ) == 0 ) { \
	    PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		( (COLORPTR)data )[ i ] = CLEARCOLOR; \
		i++; \
	    } i += c->xsize - xsize; } \
	} else if( ( v2 & COLORMASK ) != COLORMASK ) { \
	    int r2 = red( v2 ); int g2 = green( v2 ); int b2 = blue( v2 ); \
	    PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		COLOR v1 = ( (COLORPTR)data )[ i ]; \
		int r1 = red( v1 ); int g1 = green( v1 ); int b1 = blue( v1 ); \
		r1 *= r2; r1 >>= 8; \
		g1 *= g2; g1 >>= 8; \
		b1 *= b2; b1 >>= 8; \
		( (COLORPTR)data )[ i ] = get_color( r1, g1, b1 ); \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_DIV( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	if( val_type == 0 ) { \
	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		( (TYPE*)data )[ i ] /= val.i; \
		i++; \
	    } i += c->xsize - xsize; } \
	} else { \
	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		( (TYPE*)data )[ i ] /= val.f; \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_DIV2( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	if( val_type == 0 ) { \
	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		TYPE cv = ( (TYPE*)data )[ i ]; \
		if( cv == 0 ) ( (TYPE*)data )[ i ] = 0; else ( (TYPE*)data )[ i ] = val.i / cv; \
		i++; \
	    } i += c->xsize - xsize; } \
	} else { \
	    for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		( (TYPE*)data )[ i ] = val.f / ( (TYPE*)data )[ i ]; \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_COLOR_DIV() \
    { \
	COLOR v2; if( val_type == 0 ) v2 = (COLOR)val.i; else v2 = (COLOR)val.f; \
	if( ( v2 & COLORMASK ) == COLORMASK ) { \
	    PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		( (COLORPTR)data )[ i ] = CLEARCOLOR; \
		i++; \
	    } i += c->xsize - xsize; } \
	} else if( ( v2 & COLORMASK ) != 0 ) { \
	    int r2 = red( v2 ); int g2 = green( v2 ); int b2 = blue( v2 ); \
	    PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		COLOR v1 = ( (COLORPTR)data )[ i ]; \
		int r1 = red( v1 ); int g1 = green( v1 ); int b1 = blue( v1 ); \
		r1 *= ( 255 - r2 ); r1 >>= 8; \
		g1 *= ( 255 - g2 ); g1 >>= 8; \
		b1 *= ( 255 - b2 ); b1 >>= 8; \
		( (COLORPTR)data )[ i ] = get_color( r1, g1, b1 ); \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_AND( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) & val.i ); else ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) & (PIX_INT)val.f ); \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_OR( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) | val.i ); else ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) | (PIX_INT)val.f ); \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_XOR( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) ^ val.i ); else ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) ^ (PIX_INT)val.f ); \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_LSHIFT( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) << val.i ); else ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) << (PIX_INT)val.f ); \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_RSHIFT( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) >> val.i ); else ( (TYPE*)data )[ i ] = (TYPE)( (PIX_INT)( ( (TYPE*)data )[ i ] ) >> (PIX_INT)val.f ); \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_LIMIT_TOP( TYPE, MAX ) \
    { \
	bool ignore = 0; \
	if( MAX != 0 ) \
	{ \
	    if( val_type == 0 ) \
	    { \
		if( val.i >= MAX ) ignore = 1; \
	    } else { \
		if( val.f >= MAX ) ignore = 1; \
	    } \
	} \
	if( ignore == 0 ) \
	{ \
	    PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( val_type == 0 ) \
		    { if( ( (TYPE*)data )[ i ] > val.i ) ( (TYPE*)data )[ i ] = val.i; } \
		else \
		    { if( ( (TYPE*)data )[ i ] > val.f ) ( (TYPE*)data )[ i ] = val.f; } \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_LIMIT_BOTTOM( TYPE, MAX ) \
    { \
	bool ignore = 0; \
	if( MAX != 0 ) \
	{ \
	    if( val_type == 0 ) \
	    { \
		if( val.i <= MAX ) ignore = 1; \
	    } else { \
		if( val.f <= MAX ) ignore = 1; \
	    } \
	} \
	if( ignore == 0 ) \
	{ \
	    PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( val_type == 0 ) \
		    { if( ( (TYPE*)data )[ i ] < val.i ) ( (TYPE*)data )[ i ] = val.i; } \
		else \
		    { if( ( (TYPE*)data )[ i ] < val.f ) ( (TYPE*)data )[ i ] = val.f; } \
		i++; \
	    } i += c->xsize - xsize; } \
	} \
    }
#define OP_EQUAL( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) \
		{ if( ( (TYPE*)data )[ i ] == val.i ) ( (TYPE*)data )[ i ] = 1; else ( (TYPE*)data )[ i ] = 0; } \
	    else \
		{ if( ( (TYPE*)data )[ i ] == val.f ) ( (TYPE*)data )[ i ] = 1; else ( (TYPE*)data )[ i ] = 0; } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_LESS( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) \
		{ if( ( (TYPE*)data )[ i ] < val.i ) ( (TYPE*)data )[ i ] = 1; else ( (TYPE*)data )[ i ] = 0; } \
	    else \
		{ if( ( (TYPE*)data )[ i ] < val.f ) ( (TYPE*)data )[ i ] = 1; else ( (TYPE*)data )[ i ] = 0; } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_GREATER( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) \
		{ if( ( (TYPE*)data )[ i ] > val.i ) ( (TYPE*)data )[ i ] = 1; else ( (TYPE*)data )[ i ] = 0; } \
	    else \
		{ if( ( (TYPE*)data )[ i ] > val.f ) ( (TYPE*)data )[ i ] = 1; else ( (TYPE*)data )[ i ] = 0; } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_COPY( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) ( (TYPE*)data )[ i ] = val.i; else ( (TYPE*)data )[ i ] = val.f; \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_COPY_LESS( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) \
		{ if( ( (TYPE*)data )[ i ] < val.i ) ( (TYPE*)data )[ i ] = val.i; } \
	    else \
		{ if( ( (TYPE*)data )[ i ] < val.f ) ( (TYPE*)data )[ i ] = val.f; } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_COPY_GREATER( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( val_type == 0 ) \
		{ if( ( (TYPE*)data )[ i ] > val.i ) ( (TYPE*)data )[ i ] = val.i; } \
	    else \
		{ if( ( (TYPE*)data )[ i ] > val.f ) ( (TYPE*)data )[ i ] = val.f; } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_ABS( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    if( ( (TYPE*)data )[ i ] < 0 ) ( (TYPE*)data )[ i ] = -( (TYPE*)data )[ i ]; \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_ABS_INT( TYPE, MIN, MAX ) \
    { \
	PIX_INT i = y * c->xsize + x; for( PIX_INT y = 0; y < ysize; y++ ) { for( PIX_INT x = 0; x < xsize; x++ ) { \
	    PIX_INT v = ( (TYPE*)data )[ i ]; \
	    if( v < 0 ) { \
		if( v == MIN ) ( (TYPE*)data )[ i ] = MAX; else	( (TYPE*)data )[ i ] = -v; \
	    } \
	    i++; \
	} i += c->xsize - xsize; } \
    }
#define OP_H_INTEGRAL( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	for( PIX_INT yy = 0; yy < ysize; yy++ ) { \
	    TYPE acc = 0; \
	    for( PIX_INT xx = 0; xx < xsize; xx++ ) { \
		acc += ( (TYPE*)data )[ i ]; \
		( (TYPE*)data )[ i ] = acc; \
		i++; \
	    } i += c->xsize - xsize; \
	} \
    }
#define OP_V_INTEGRAL( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	for( PIX_INT xx = 0; xx < xsize; xx++ ) { \
	    PIX_INT i2 = i + xx; \
	    TYPE acc = 0; \
	    for( PIX_INT yy = 0; yy < ysize; yy++ ) { \
		acc += ( (TYPE*)data )[ i2 ]; \
		( (TYPE*)data )[ i2 ] = acc; \
		i2 += c->xsize; \
	    } \
	} \
    }
#define OP_H_DERIVATIVE( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	for( PIX_INT yy = 0; yy < ysize; yy++ ) { \
	    TYPE prev_val = 0; \
	    for( PIX_INT xx = 0; xx < xsize; xx++ ) { \
		TYPE cur_val = ( (TYPE*)data )[ i ]; \
		( (TYPE*)data )[ i ] = ( (TYPE*)data )[ i ] - prev_val; \
		prev_val = cur_val; \
		i++; \
	    } i += c->xsize - xsize; \
	} \
    }
#define OP_V_DERIVATIVE( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	for( PIX_INT xx = 0; xx < xsize; xx++ ) { \
	    PIX_INT i2 = i + xx; \
	    TYPE prev_val = 0; \
	    for( PIX_INT yy = 0; yy < ysize; yy++ ) { \
		TYPE cur_val = ( (TYPE*)data )[ i2 ]; \
		( (TYPE*)data )[ i2 ] = ( (TYPE*)data )[ i2 ] - prev_val; \
		prev_val = cur_val; \
		i2 += c->xsize; \
	    } \
	} \
    }
#define OP_H_FLIP( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	for( PIX_INT yy = 0; yy < ysize; yy++ ) { \
	    TYPE* line = ( (TYPE*)data ) + i; \
	    for( PIX_INT xx = 0; xx < xsize / 2; xx++ ) { \
		TYPE temp = line[ xx ];	line[ xx ] = line[ xsize - 1 - xx ]; line[ xsize - 1 - xx ] = temp; \
	    } \
	    i += c->xsize; \
	} \
    }
#define OP_V_FLIP( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	for( PIX_INT yy = 0; yy < ysize / 2; yy++ ) { \
	    TYPE* line1 = ( (TYPE*)data ) + i + yy * c->xsize; \
	    TYPE* line2 = ( (TYPE*)data ) + i + ( ysize - 1 - yy ) * c->xsize; \
	    for( PIX_INT xx = 0; xx < xsize; xx++ ) { TYPE temp = line1[ xx ]; line1[ xx ] = line2[ xx ]; line2[ xx ] = temp; } \
	} \
    }

inline int pix_vm_op_cc_size_control( 
    pix_vm_container* dest,
    pix_vm_container* src,
    PIX_INT& dest_x, 
    PIX_INT& dest_y, 
    PIX_INT& src_x, 
    PIX_INT& src_y, 
    PIX_INT& xsize, 
    PIX_INT& ysize )
{
    if( xsize == 0 && ysize == 0 )
    {
	//Whole destination:
	dest_x = 0;
	dest_y = 0;
	xsize = dest->xsize;
	ysize = dest->ysize;
    }

    if( ysize )
    {
	if( dest_x < 0 ) { xsize += dest_x; src_x -= dest_x; dest_x = 0; }
	if( dest_y < 0 ) { ysize += dest_y; src_y -= dest_y; dest_y = 0; }
	if( src_x < 0 ) { xsize += src_x; dest_x -= src_x; src_x = 0; }
	if( src_y < 0 ) { ysize += src_y; dest_y -= src_y; src_y = 0; }
	if( dest_x + xsize > dest->xsize ) xsize = dest->xsize - dest_x;
	if( dest_y + ysize > dest->ysize ) ysize = dest->ysize - dest_y;
	if( src_x + xsize > src->xsize ) xsize = src->xsize - src_x;
	if( src_y + ysize > src->ysize ) ysize = src->ysize - src_y;
	if( xsize <= 0 ) return 1;
	if( ysize <= 0 ) return 1;
    }
    else
    {
	//One axis mode:
	if( dest_x < 0 ) { xsize += dest_x; src_x -= dest_x; dest_x = 0; }
	if( src_x < 0 ) { xsize += src_x; dest_x -= src_x; src_x = 0; }
	if( dest_x + xsize > dest->size ) xsize = dest->size - dest_x;
	if( src_x + xsize > src->size ) xsize = src->size - src_x;
	if( xsize <= 0 ) return 1;
	ysize = 1;
    }

    return 0;
}

inline int pix_vm_op_c_size_control( 
    pix_vm_container* dest,
    PIX_INT& dest_x, 
    PIX_INT& dest_y, 
    PIX_INT& xsize, 
    PIX_INT& ysize )
{
    if( xsize == 0 )
    {
	//Whole destination:
	dest_x = 0;
	dest_y = 0;
	xsize = dest->xsize;
	ysize = dest->ysize;
	return 0;
    }

    if( ysize )
    {
	if( dest_x < 0 ) { xsize += dest_x; dest_x = 0; }
	if( dest_y < 0 ) { ysize += dest_y; dest_y = 0; }
	if( dest_x + xsize > dest->xsize ) xsize = dest->xsize - dest_x;
	if( dest_y + ysize > dest->ysize ) ysize = dest->ysize - dest_y;
	if( xsize <= 0 ) return 1;
	if( ysize <= 0 ) return 1;
    }
    else
    {
	//One axis mode:
	if( dest_x < 0 ) { xsize += dest_x; dest_x = 0; }
	if( dest_x + xsize > dest->size ) xsize = dest->size - dest_x;
	if( xsize <= 0 ) return 1;
	ysize = 1;
    }

    return 0;
}

void pix_vm_op_cn( int opcode, PIX_CID cnum, int8_t val_type, PIX_VAL val, PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, PIX_VAL* retval, int8_t* retval_type, pix_vm* vm )
{
    PIX_INT rv = 0;
    if( (unsigned)cnum >= (unsigned)vm->c_num ) { retval[ 0 ].i = -1; return; }
    pix_vm_container* c = vm->c[ cnum ];
    if( !c ) { retval[ 0 ].i = -1; return; }

    if( pix_vm_op_c_size_control( c, x, y, xsize, ysize ) ) { retval[ 0 ].i = -1; return; }

    void* data = c->data;

op_cn_try_again:

    switch( opcode )
    {
	case PIX_DATA_OPCODE_MIN:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_MIN( 0, int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_MIN( 0, int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_MIN( 0, int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_MIN( 0, int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_MIN( 1, float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_MIN( 1, double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_MAX:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_MAX( 0, int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_MAX( 0, int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_MAX( 0, int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_MAX( 0, int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_MAX( 1, float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_MAX( 1, double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_MAXABS:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_MAXABS_INT( int8_t, -128, 127 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_MAXABS_INT( int16_t, -32768, 32767 ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_MAXABS_INT( int32_t, (int32_t)0x80000000, (int32_t)0x7FFFFFFF ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_MAXABS_INT( int64_t, (int64_t)0x8000000000000000, (int64_t)0x7FFFFFFFFFFFFFFF ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_MAXABS( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_MAXABS( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_SUM:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_SUM( 0, int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_SUM( 0, int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_SUM( 0, int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_SUM( 0, int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_SUM( 1, float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_SUM( 1, double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_LIMIT_TOP:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_LIMIT_TOP( int8_t, 127 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_LIMIT_TOP( int16_t, 32767 ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_LIMIT_TOP( int32_t, (int32_t)0x7FFFFFFF ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_LIMIT_TOP( int64_t, (int64_t)0x7FFFFFFFFFFFFFFF ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_LIMIT_TOP( float, 0 ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_LIMIT_TOP( double, 0 ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_LIMIT_BOTTOM:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_LIMIT_BOTTOM( int8_t, -128 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_LIMIT_BOTTOM( int16_t, -32768 ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_LIMIT_BOTTOM( int32_t, (int32_t)0x80000000 ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_LIMIT_BOTTOM( int64_t, (int64_t)0x8000000000000000 ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_LIMIT_BOTTOM( float, 0 ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_LIMIT_BOTTOM( double, 0 ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_ABS:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_ABS_INT( int8_t, -128, 127 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_ABS_INT( int16_t, -32768, 32767 ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_ABS_INT( int32_t, (int32_t)0x80000000, (int32_t)0x7FFFFFFF ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_ABS_INT( int64_t, (int64_t)0x8000000000000000, (int64_t)0x7FFFFFFFFFFFFFFF ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_ABS( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_ABS( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	case PIX_DATA_OPCODE_ADD:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_ADD( 0, int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_ADD( 0, int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_ADD( 0, int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_ADD( 0, int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_ADD( 0, float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_ADD( 0, double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_SADD:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_SADD( 0, int8_t, -128, 127 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_SADD( 0, int16_t, -32768, 32767 ); break;
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COLOR_ADD:
	    if( sizeof( COLOR ) == g_pix_container_type_sizes[ c->type ] )
	    {
		OP_COLOR_ADD();
	    }
	    else retval[ 0 ].i = -1; 
	    break;
	case PIX_DATA_OPCODE_SUB:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_ADD( 1, int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_ADD( 1, int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_ADD( 1, int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_ADD( 1, int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_ADD( 1, float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_ADD( 1, double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_SSUB:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_SADD( 1, int8_t, -128, 127 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_SADD( 1, int16_t, -32768, 32767 ); break;
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_SUB2:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_SUB2( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_SUB2( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_SUB2( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_SUB2( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_SUB2( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_SUB2( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COLOR_SUB:
	    if( sizeof( COLOR ) == g_pix_container_type_sizes[ c->type ] )
	    {
		OP_COLOR_SUB();
	    }
	    else retval[ 0 ].i = -1;
	    break;
	case PIX_DATA_OPCODE_COLOR_SUB2:
	    if( sizeof( COLOR ) == g_pix_container_type_sizes[ c->type ] )
	    {
		OP_COLOR_SUB2();
	    }
	    else retval[ 0 ].i = -1;
	    break;
	case PIX_DATA_OPCODE_MUL:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_MUL( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_MUL( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_MUL( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_MUL( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_MUL( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_MUL( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_SMUL:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_SMUL( int8_t, -128, 127 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_SMUL( int16_t, -32768, 32767 ); break;
		default: opcode = PIX_DATA_OPCODE_MUL; goto op_cn_try_again; break;
	    }
	    break;
	case PIX_DATA_OPCODE_MUL_RSHIFT15:
	    switch( c->type )
            {
                case PIX_CONTAINER_TYPE_INT8: OP_MUL_RSHIFT15( int8_t ); break;
                case PIX_CONTAINER_TYPE_INT16: OP_MUL_RSHIFT15( int16_t ); break;
                case PIX_CONTAINER_TYPE_INT32: OP_MUL_RSHIFT15( int32_t ); break;
#ifdef PIX_INT64_ENABLED
                case PIX_CONTAINER_TYPE_INT64: OP_MUL_RSHIFT15( int64_t ); break;
#endif
                case PIX_CONTAINER_TYPE_FLOAT32: OP_MUL_RSHIFT15( float ); break;
#ifdef PIX_FLOAT64_ENABLED
                case PIX_CONTAINER_TYPE_FLOAT64: OP_MUL_RSHIFT15( double ); break;
#endif
                default: retval[ 0 ].i = -1; break;
            }
	    break;
	case PIX_DATA_OPCODE_COLOR_MUL:
	    if( sizeof( COLOR ) == g_pix_container_type_sizes[ c->type ] )
	    {
		OP_COLOR_MUL();
	    }
	    else retval[ 0 ].i = -1; 
	    break;
	case PIX_DATA_OPCODE_DIV:
	    if( val_type == 0 && val.i == 0 ) break;
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_DIV( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_DIV( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_DIV( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_DIV( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_DIV( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_DIV( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_DIV2:
	    if( val_type == 0 && val.i == 0 ) break;
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_DIV2( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_DIV2( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_DIV2( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_DIV2( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_DIV2( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_DIV2( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COLOR_DIV:
	    if( sizeof( COLOR ) == g_pix_container_type_sizes[ c->type ] )
	    {
		OP_COLOR_DIV();
	    }
	    else retval[ 0 ].i = -1; 
	    break;
	case PIX_DATA_OPCODE_AND:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_AND( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_AND( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_AND( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_AND( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_AND( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_AND( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_OR:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_OR( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_OR( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_OR( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_OR( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_OR( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_OR( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_XOR:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_XOR( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_XOR( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_XOR( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_XOR( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_XOR( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_XOR( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_LSHIFT:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_LSHIFT( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_LSHIFT( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_LSHIFT( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_LSHIFT( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_LSHIFT( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_LSHIFT( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_RSHIFT:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_RSHIFT( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_RSHIFT( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_RSHIFT( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_RSHIFT( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_RSHIFT( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_RSHIFT( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_EQUAL:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_EQUAL( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_EQUAL( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_EQUAL( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_EQUAL( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_EQUAL( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_EQUAL( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_LESS:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_LESS( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_LESS( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_LESS( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_LESS( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_LESS( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_LESS( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_GREATER:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_GREATER( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_GREATER( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_GREATER( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_GREATER( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_GREATER( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_GREATER( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COPY:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_COPY( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_COPY( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_COPY( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_COPY( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_COPY( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_COPY( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COPY_LESS:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_COPY_LESS( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_COPY_LESS( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_COPY_LESS( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_COPY_LESS( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_COPY_LESS( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_COPY_LESS( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COPY_GREATER:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_COPY_GREATER( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_COPY_GREATER( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_COPY_GREATER( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_COPY_GREATER( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_COPY_GREATER( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_COPY_GREATER( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_H_INTEGRAL:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_H_INTEGRAL( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_H_INTEGRAL( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_H_INTEGRAL( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_H_INTEGRAL( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_H_INTEGRAL( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_H_INTEGRAL( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_V_INTEGRAL:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_V_INTEGRAL( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_V_INTEGRAL( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_V_INTEGRAL( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_V_INTEGRAL( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_V_INTEGRAL( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_V_INTEGRAL( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_H_DERIVATIVE:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_H_DERIVATIVE( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_H_DERIVATIVE( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_H_DERIVATIVE( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_H_DERIVATIVE( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_H_DERIVATIVE( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_H_DERIVATIVE( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_V_DERIVATIVE:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_V_DERIVATIVE( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_V_DERIVATIVE( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_V_DERIVATIVE( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_V_DERIVATIVE( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_V_DERIVATIVE( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_V_DERIVATIVE( double ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_H_FLIP:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_H_FLIP( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_H_FLIP( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: 
		case PIX_CONTAINER_TYPE_FLOAT32: 
		    OP_H_FLIP( int32_t ); 
		    break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
		case PIX_CONTAINER_TYPE_INT64: 
		case PIX_CONTAINER_TYPE_FLOAT64: 
		    OP_H_FLIP( int64_t ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_V_FLIP:
	    switch( c->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_V_FLIP( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_V_FLIP( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: 
		case PIX_CONTAINER_TYPE_FLOAT32:
		    OP_V_FLIP( int32_t ); 
		    break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
		case PIX_CONTAINER_TYPE_INT64: 
		case PIX_CONTAINER_TYPE_FLOAT64: 
		    OP_V_FLIP( int64_t ); break;
#endif
		default: retval[ 0 ].i = -1; break;
	    }
	    break;
	default:
	    PIX_VM_LOG( "op_cn: unknown opcode %d\n", opcode );
	    retval[ 0 ].i = -1;
	    break;
    }
}

#define OP_CC_ADD( SUB, TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( SUB ) *dest_ptr -= *src_ptr; else *dest_ptr += *src_ptr;\
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_SADD( SUB, TYPE, MIN, MAX ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		PIX_INT v = *dest_ptr; \
		if( SUB ) v -= *src_ptr; else v += *src_ptr; \
		if( v < MIN ) v = MIN; \
		if( v > MAX ) v = MAX; \
		*dest_ptr = v; \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_COLOR_ADD() \
    { \
	COLORPTR dest_ptr = (COLORPTR)data1 + dest_y * c1->xsize + dest_x; \
	COLORPTR src_ptr = (COLORPTR)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		COLOR v1 = *dest_ptr; \
		COLOR v2 = *src_ptr; \
		if( v2 & COLORMASK ) { \
		    int r1 = red( v1 ); int g1 = green( v1 ); int b1 = blue( v1 ); \
		    int r2 = red( v2 ); int g2 = green( v2 ); int b2 = blue( v2 ); \
		    r1 += r2; if( r1 > 255 ) r1 = 255; \
		    g1 += g2; if( g1 > 255 ) g1 = 255; \
		    b1 += b2; if( b1 > 255 ) b1 = 255; \
		    *dest_ptr = get_color( r1, g1, b1 ); \
		} \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_COLOR_SUB() \
    { \
	COLORPTR dest_ptr = (COLORPTR)data1 + dest_y * c1->xsize + dest_x; \
	COLORPTR src_ptr = (COLORPTR)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		COLOR v1 = *dest_ptr; \
		COLOR v2 = *src_ptr; \
		if( v2 & COLORMASK ) { \
		    int r1 = red( v1 ); int g1 = green( v1 ); int b1 = blue( v1 ); \
		    int r2 = red( v2 ); int g2 = green( v2 ); int b2 = blue( v2 ); \
		    r1 -= r2; if( r1 < 0 ) r1 = 0; \
		    g1 -= g2; if( g1 < 0 ) g1 = 0; \
		    b1 -= b2; if( b1 < 0 ) b1 = 0; \
		    *dest_ptr = get_color( r1, g1, b1 ); \
		} \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_MUL( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		*dest_ptr *= *src_ptr; \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_SMUL( TYPE, MIN, MAX ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		PIX_INT v = *dest_ptr; \
		v *= *src_ptr; \
		if( v < MIN ) v = MIN; \
		if( v > MAX ) v = MAX; \
		*dest_ptr = v; \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_BMUL( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( (*src_ptr) == 0 ) *dest_ptr = 0; \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_MUL_RSHIFT15( TYPE ) \
    { \
        TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
        TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
        for( PIX_INT y = 0; y < ysize; y++ ) { \
            for( PIX_INT x = 0; x < xsize; x++ ) { \
                PIX_INT v = ( (PIX_INT)dest_ptr[ 0 ] * (PIX_INT)src_ptr[ 0 ] ) >> 15; \
                dest_ptr[ 0 ] = v; \
                dest_ptr++; src_ptr++; \
            } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
        } \
    }
#define OP_CC_COLOR_MUL() \
    { \
	COLORPTR dest_ptr = (COLORPTR)data1 + dest_y * c1->xsize + dest_x; \
	COLORPTR src_ptr = (COLORPTR)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		COLOR v1 = *dest_ptr; \
		COLOR v2 = *src_ptr; \
		if( ( v2 & COLORMASK ) != COLORMASK ) { \
		    if( ( v2 & COLORMASK ) == 0 ) *dest_ptr = CLEARCOLOR; else { \
			int r1 = red( v1 ); int g1 = green( v1 ); int b1 = blue( v1 ); \
			int r2 = red( v2 ); int g2 = green( v2 ); int b2 = blue( v2 ); \
			r1 *= r2; r1 >>= 8; \
			g1 *= g2; g1 >>= 8; \
			b1 *= b2; b1 >>= 8; \
			*dest_ptr = get_color( r1, g1, b1 ); \
		    } \
		} \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_DIV( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		*dest_ptr /= *src_ptr; \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_COLOR_DIV() \
    { \
	COLORPTR dest_ptr = (COLORPTR)data1 + dest_y * c1->xsize + dest_x; \
	COLORPTR src_ptr = (COLORPTR)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		COLOR v1 = *dest_ptr; \
		COLOR v2 = *src_ptr; \
		if( ( v2 & COLORMASK ) != 0 ) { \
		    if( ( v2 & COLORMASK ) == COLORMASK ) *dest_ptr = CLEARCOLOR; else { \
			int r1 = red( v1 ); int g1 = green( v1 ); int b1 = blue( v1 ); \
			int r2 = red( v2 ); int g2 = green( v2 ); int b2 = blue( v2 ); \
			r1 *= ( 255 - r2 ); r1 >>= 8; \
			g1 *= ( 255 - g2 ); g1 >>= 8; \
			b1 *= ( 255 - b2 ); b1 >>= 8; \
			*dest_ptr = get_color( r1, g1, b1 ); \
		    } \
		} \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_AND( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		*dest_ptr = (TYPE)( (PIX_INT)( *dest_ptr ) & (PIX_INT)( *src_ptr ) ); \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_OR( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		*dest_ptr = (TYPE)( (PIX_INT)( *dest_ptr ) | (PIX_INT)( *src_ptr ) ); \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_XOR( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		*dest_ptr = (TYPE)( (PIX_INT)( *dest_ptr ) ^ (PIX_INT)( *src_ptr ) ); \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_LSHIFT( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		*dest_ptr = (TYPE)( (PIX_INT)( *dest_ptr ) << (PIX_INT)( *src_ptr ) ); \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_RSHIFT( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		*dest_ptr = (TYPE)( (PIX_INT)( *dest_ptr ) >> (PIX_INT)( *src_ptr ) ); \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_EQUAL( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( *dest_ptr == *src_ptr ) *dest_ptr = 1; else *dest_ptr = 0; \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_LESS( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( *dest_ptr < *src_ptr ) *dest_ptr = 1; else *dest_ptr = 0; \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_GREATER( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( *dest_ptr > *src_ptr ) *dest_ptr = 1; else *dest_ptr = 0; \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_COPY( TYPE, COND ) \
    { \
        TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
        TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
        for( PIX_INT y = 0; y < ysize; y++ ) { \
            for( PIX_INT x = 0; x < xsize; x++ ) { \
                if( COND == 0 ) { *dest_ptr = *src_ptr; } \
                if( COND == -1 ) { if( *dest_ptr < *src_ptr ) *dest_ptr = *src_ptr; } \
                if( COND == 1 ) { if( *dest_ptr > *src_ptr ) *dest_ptr = *src_ptr; } \
                dest_ptr++; src_ptr++; \
            } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
        } \
    }
#define OP_CC_EXCHANGE( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		TYPE temp = *dest_ptr; *dest_ptr = *src_ptr; *src_ptr = temp; \
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_CC_COMPARE( TYPE ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		TYPE v1, v2; \
		v1 = *dest_ptr; \
		v2 = *src_ptr; \
		if( v1 > v2 ) { rv = 1; y = ysize; break; } else { if( v1 < v2 ) { rv = -1; y = ysize; break; } }\
		dest_ptr++; src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }

PIX_INT pix_vm_op_cc( int opcode, PIX_CID cnum1, PIX_CID cnum2, PIX_INT dest_x, PIX_INT dest_y, PIX_INT src_x, PIX_INT src_y, PIX_INT xsize, PIX_INT ysize, pix_vm* vm )
{
    PIX_INT rv = 0;
    if( (unsigned)cnum1 >= (unsigned)vm->c_num ) return -1;
    if( (unsigned)cnum2 >= (unsigned)vm->c_num ) return -1;
    pix_vm_container* c1 = vm->c[ cnum1 ];
    pix_vm_container* c2 = vm->c[ cnum2 ];
    if( !c1 ) return -1;
    if( !c2 ) return -1;

    /*
    //SIMD test
    char* pp = (char*)c1->data;
    for( int i = 0; i < xsize; i++ )
    {
	pp[ i ] = 4;
    }
    */

    if( c1->type != c2->type )
    {
	PIX_VM_LOG( "op_cc: dest.type(%s) != src.type(%s); source type must be equal to destination type\n", g_pix_container_type_names[ c1->type ], g_pix_container_type_names[ c2->type ] );
	return -1;
    }
    if( pix_vm_op_cc_size_control( c1, c2, dest_x, dest_y, src_x, src_y, xsize, ysize ) ) return -1;

    void* data1 = c1->data;
    void* data2 = c2->data;

    switch( opcode )
    {
	case PIX_DATA_OPCODE_ADD:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_ADD( 0, int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_ADD( 0, int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_ADD( 0, int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_ADD( 0, int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_ADD( 0, float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_ADD( 0, double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_SADD:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_SADD( 0, int8_t, -128, 127 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_SADD( 0, int16_t, -32768, 32767 ); break;
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COLOR_ADD:
	    if( sizeof( COLOR ) == g_pix_container_type_sizes[ c1->type ] &&
		sizeof( COLOR ) == g_pix_container_type_sizes[ c2->type ] )
	    {
		OP_CC_COLOR_ADD();
	    }
	    else rv = -1;
	    break;
	case PIX_DATA_OPCODE_SUB:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_ADD( 1, int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_ADD( 1, int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_ADD( 1, int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_ADD( 1, int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_ADD( 1, float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_ADD( 1, double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_SSUB:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_SADD( 1, int8_t, -128, 127 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_SADD( 1, int16_t, -32768, 32767 ); break;
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COLOR_SUB:
	    if( sizeof( COLOR ) == g_pix_container_type_sizes[ c1->type ] &&
		sizeof( COLOR ) == g_pix_container_type_sizes[ c2->type ] )
	    {
		OP_CC_COLOR_SUB();
	    }
	    else rv = -1;
	    break;
	case PIX_DATA_OPCODE_MUL:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_MUL( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_MUL( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_MUL( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_MUL( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_MUL( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_MUL( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_SMUL:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_SMUL( int8_t, -128, 127 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_SMUL( int16_t, -32768, 32767 ); break;
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_BMUL:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_BMUL( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_BMUL( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_BMUL( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_BMUL( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_BMUL( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_BMUL( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_MUL_RSHIFT15:
	    switch( c1->type )
            {
                case PIX_CONTAINER_TYPE_INT8: OP_CC_MUL_RSHIFT15( int8_t ); break;
                case PIX_CONTAINER_TYPE_INT16: OP_CC_MUL_RSHIFT15( int16_t ); break;
                case PIX_CONTAINER_TYPE_INT32: OP_CC_MUL_RSHIFT15( int32_t ); break;
#ifdef PIX_INT64_ENABLED
                case PIX_CONTAINER_TYPE_INT64: OP_CC_MUL_RSHIFT15( int64_t ); break;
#endif
                case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_MUL_RSHIFT15( float ); break;
#ifdef PIX_FLOAT64_ENABLED
                case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_MUL_RSHIFT15( double ); break;
#endif
                default: rv = -1; break;
            }
	    break;
	case PIX_DATA_OPCODE_COLOR_MUL:
	    if( sizeof( COLOR ) == g_pix_container_type_sizes[ c1->type ] &&
		sizeof( COLOR ) == g_pix_container_type_sizes[ c2->type ] )
	    {
		OP_CC_COLOR_MUL();
	    }
	    else rv = -1;
	    break;
	case PIX_DATA_OPCODE_DIV:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_DIV( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_DIV( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_DIV( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_DIV( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_DIV( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_DIV( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COLOR_DIV:
	    if( sizeof( COLOR ) == g_pix_container_type_sizes[ c1->type ] &&
		sizeof( COLOR ) == g_pix_container_type_sizes[ c2->type ] )
	    {
		OP_CC_COLOR_DIV();
	    }
	    else rv = -1;
	    break;
	case PIX_DATA_OPCODE_AND:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_AND( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_AND( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_AND( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_AND( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_AND( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_AND( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_OR:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_OR( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_OR( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_OR( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_OR( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_OR( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_OR( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;	
	case PIX_DATA_OPCODE_XOR:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_XOR( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_XOR( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_XOR( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_XOR( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_XOR( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_XOR( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_LSHIFT:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_LSHIFT( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_LSHIFT( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_LSHIFT( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_LSHIFT( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_LSHIFT( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_LSHIFT( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_RSHIFT:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_RSHIFT( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_RSHIFT( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_RSHIFT( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_RSHIFT( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_RSHIFT( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_RSHIFT( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_EQUAL:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_EQUAL( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_EQUAL( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_EQUAL( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_EQUAL( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_EQUAL( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_EQUAL( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_LESS:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_LESS( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_LESS( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_LESS( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_LESS( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_LESS( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_LESS( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_GREATER:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_GREATER( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_GREATER( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_GREATER( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_GREATER( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_GREATER( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_GREATER( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COPY:
	    switch( c1->type )
	    {
	        case PIX_CONTAINER_TYPE_INT8: OP_CC_COPY( int8_t, 0 ); break;
	        case PIX_CONTAINER_TYPE_INT16: OP_CC_COPY( int16_t, 0 ); break;
	        case PIX_CONTAINER_TYPE_INT32: 
	        case PIX_CONTAINER_TYPE_FLOAT32: 
	    	    OP_CC_COPY( int32_t, 0 ); 
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
#endif
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64:
#endif
#if defined( PIX_INT64_ENABLED ) || defined( PIX_FLOAT64_ENABLED )
		    OP_CC_COPY( int64_t, 0 ); 
		    break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COPY_LESS:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_COPY( int8_t, -1 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_COPY( int16_t, -1 ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_COPY( int32_t, -1 ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_COPY( int64_t, -1 ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_COPY( float, -1 ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_COPY( double, -1 ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COPY_GREATER:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_COPY( int8_t, 1 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_COPY( int16_t, 1 ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_COPY( int32_t, 1 ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_COPY( int64_t, 1 ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_COPY( float, 1 ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_COPY( double, 1 ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_EXCHANGE:
	    switch( c1->type )
	    {
	        case PIX_CONTAINER_TYPE_INT8: OP_CC_EXCHANGE( int8_t ); break;
	        case PIX_CONTAINER_TYPE_INT16: OP_CC_EXCHANGE( int16_t ); break;
	        case PIX_CONTAINER_TYPE_INT32: 
	        case PIX_CONTAINER_TYPE_FLOAT32: 
	    	    OP_CC_EXCHANGE( int32_t ); 
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
#endif
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64:
#endif
#if defined( PIX_INT64_ENABLED ) || defined( PIX_FLOAT64_ENABLED )
		    OP_CC_EXCHANGE( int64_t ); 
		    break;
#endif
		default: rv = -1; break;
	    }
	    break;
	case PIX_DATA_OPCODE_COMPARE:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_CC_COMPARE( int8_t ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_CC_COMPARE( int16_t ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_CC_COMPARE( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_CC_COMPARE( int64_t ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_CC_COMPARE( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_CC_COMPARE( double ); break;
#endif
		default: rv = -1; break;
	    }
	    break;
	default: 
	    PIX_VM_LOG( "op_cc: unknown opcode %d\n", opcode );
	    rv = -1; 
	    break;
    }
    
    return rv;
}

#define OP_MUL_DIV( TYPE, INT ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( INT ) { \
		    if( val_type == 0 ) \
			*dest_ptr = (PIX_INT)( *dest_ptr ) * (PIX_INT)( *src_ptr ) / val.i; \
		    else \
			*dest_ptr = (PIX_FLOAT)( (PIX_INT)( *dest_ptr ) * (PIX_INT)( *src_ptr ) ) / val.f; \
		} else { \
		    if( val_type == 0 ) \
			*dest_ptr = (PIX_FLOAT)( *dest_ptr ) * (PIX_FLOAT)( *src_ptr ) / val.i; \
		    else \
			*dest_ptr = (PIX_FLOAT)( *dest_ptr ) * (PIX_FLOAT)( *src_ptr ) / val.f; \
		} \
		dest_ptr++; \
		src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }
#define OP_MUL_RSHIFT( TYPE, INT ) \
    { \
	TYPE* dest_ptr = (TYPE*)data1 + dest_y * c1->xsize + dest_x; \
	TYPE* src_ptr = (TYPE*)data2 + src_y * c2->xsize + src_x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( INT ) { *dest_ptr = (TYPE)( (PIX_INT)( (PIX_INT)( *dest_ptr ) * (PIX_INT)( *src_ptr ) ) >> val_i ); } \
		     else { *dest_ptr = (TYPE)( (PIX_INT)( (PIX_FLOAT)( *dest_ptr ) * (PIX_FLOAT)( *src_ptr ) ) >> val_i ); } \
		dest_ptr++; \
		src_ptr++; \
	    } dest_ptr += c1->xsize - xsize; src_ptr += c2->xsize - xsize; \
	} \
    }

PIX_INT pix_vm_op_ccn( int opcode, PIX_CID cnum1, PIX_CID cnum2, int8_t val_type, PIX_VAL val, PIX_INT dest_x, PIX_INT dest_y, PIX_INT src_x, PIX_INT src_y, PIX_INT xsize, PIX_INT ysize, pix_vm* vm )
{
    PIX_INT rv = 0;
    if( (unsigned)cnum1 >= (unsigned)vm->c_num ) return -1;
    if( (unsigned)cnum2 >= (unsigned)vm->c_num ) return -1;
    pix_vm_container* c1 = vm->c[ cnum1 ];
    pix_vm_container* c2 = vm->c[ cnum2 ];
    if( !c1 ) return -1;
    if( !c2 ) return -1;

    if( c1->type != c2->type ) 
    {
	PIX_VM_LOG( "op_ccn: dest.type(%s) != src.type(%s); source type must be equal to destination type\n", g_pix_container_type_names[ c1->type ], g_pix_container_type_names[ c2->type ] );
	return -1;
    }
    if( pix_vm_op_cc_size_control( c1, c2, dest_x, dest_y, src_x, src_y, xsize, ysize ) ) return -1;

    void* data1 = c1->data;
    void* data2 = c2->data;

    PIX_INT val_i;
    if( val_type == 0 )
	val_i = val.i;
    else
	val_i = (PIX_INT)val.f;
    
    switch( opcode )
    {
	case PIX_DATA_OPCODE_MUL_DIV:
	    if( val_type == 0 )
	    {
		if( val.i == 0 ) break;
	    }
	    else
	    {
		if( val.f == 0 ) break;
	    }
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_MUL_DIV( int8_t, 1 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_MUL_DIV( int16_t, 1 ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_MUL_DIV( int32_t, 1 ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_MUL_DIV( int64_t, 1 ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_MUL_DIV( float, 0 ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_MUL_DIV( double, 0 ); break;
#endif
		default: rv = -1;break;
	    }
	    break;
	case PIX_DATA_OPCODE_MUL_RSHIFT:
	    switch( c1->type )
	    {
		case PIX_CONTAINER_TYPE_INT8: OP_MUL_RSHIFT( int8_t, 1 ); break;
		case PIX_CONTAINER_TYPE_INT16: OP_MUL_RSHIFT( int16_t, 1 ); break;
		case PIX_CONTAINER_TYPE_INT32: OP_MUL_RSHIFT( int32_t, 1 ); break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64: OP_MUL_RSHIFT( int64_t, 1 ); break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32: OP_MUL_RSHIFT( float, 0 ); break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64: OP_MUL_RSHIFT( double, 0 ); break;
#endif
		default: rv = -1;break;
	    }
	    break;
	default:
	    PIX_VM_LOG( "op_ccn: unknown opcode %d\n", opcode );
	    rv = -1;
	    break;
    }
    
    return rv;
}

#define GENERATOR_OP_SIN( TYPE ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    PIX_FLOAT phase2 = phase; \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		( (TYPE*)data )[ i ] = (TYPE)( sin( phase2 ) * amp ); \
		i++; phase2 += delta1; \
	    } i += c->xsize - xsize; phase += delta2; \
	} \
    }
#define GENERATOR_OP_SIN8( TYPE, INT ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    PIX_INT phase2 = phase; \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( INT ) \
		{ \
		    if( ( phase2 / 32768 ) & 256 ) \
			( (TYPE*)data )[ i ] = (TYPE)( (int)( -(int)g_hsin_tab[ ( phase2 / 32768 ) & 255 ] * amp ) / 256 ); \
		    else \
			( (TYPE*)data )[ i ] = (TYPE)( (int)( (int)g_hsin_tab[ ( phase2 / 32768 ) & 255 ] * amp ) / 256 ); \
		} else { \
		    if( ( phase2 / 32768 ) & 256 ) \
			( (TYPE*)data )[ i ] = (TYPE)( -(TYPE)g_hsin_tab[ ( phase2 / 32768 ) & 255 ] * amp_f ) / 256.0F; \
		    else \
			( (TYPE*)data )[ i ] = (TYPE)( (TYPE)g_hsin_tab[ ( phase2 / 32768 ) & 255 ] * amp_f ) / 256.0F; \
		} \
		i++; phase2 += delta1; \
	    } i += c->xsize - xsize; phase += delta2; \
	} \
    }
#define GENERATOR_OP_RAND( TYPE, INT ) \
    { \
	PIX_INT i = y * c->xsize + x; \
	for( PIX_INT y = 0; y < ysize; y++ ) { \
	    for( PIX_INT x = 0; x < xsize; x++ ) { \
		if( INT ) \
		    ( (TYPE*)data )[ i ] = (TYPE)( ( ( (int)( pseudo_random() & 0xFFF ) - 0x800 ) * amp ) / 0x800 ); \
		else \
		    ( (TYPE*)data )[ i ] = (TYPE)( ( ( (TYPE)( pseudo_random() & 0xFFF ) - 0x800 ) * amp_f ) / 0x800 ); \
		i++; \
	    } i += c->xsize - xsize; \
	} \
    }

PIX_INT pix_vm_generator( int opcode, PIX_CID cnum, PIX_FLOAT* fval, PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, pix_vm* vm )
{
    PIX_INT rv = 0;
    if( (unsigned)cnum >= (unsigned)vm->c_num ) return -1;
    pix_vm_container* c = vm->c[ cnum ];
    if( !c ) return -1;

    if( pix_vm_op_c_size_control( c, x, y, xsize, ysize ) ) return -1;

    void* data = c->data;

    switch( opcode )
    {
	case PIX_DATA_OPCODE_SIN:
	    {
		PIX_FLOAT phase = fval[ 0 ];
		PIX_FLOAT amp = fval[ 1 ];
		PIX_FLOAT delta1 = fval[ 2 ];
		PIX_FLOAT delta2 = fval[ 3 ];
		switch( c->type )
		{
		    case PIX_CONTAINER_TYPE_INT8: GENERATOR_OP_SIN( int8_t ); break;
		    case PIX_CONTAINER_TYPE_INT16: GENERATOR_OP_SIN( int16_t ); break;
		    case PIX_CONTAINER_TYPE_INT32: GENERATOR_OP_SIN( int32_t ); break;
#ifdef PIX_INT64_ENABLED
		    case PIX_CONTAINER_TYPE_INT64: GENERATOR_OP_SIN( int64_t ); break;
#endif
		    case PIX_CONTAINER_TYPE_FLOAT32: GENERATOR_OP_SIN( float ); break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64: GENERATOR_OP_SIN( double ); break;
#endif
		    default: rv = -1; break;
		}
	    }
	    break;
	case PIX_DATA_OPCODE_SIN8:
	    {
		PIX_INT phase = (PIX_INT)( ( fval[ 0 ] / M_PI ) * 256.0F * 32768.0F );
		PIX_INT amp = (PIX_INT)fval[ 1 ];
		PIX_FLOAT amp_f = fval[ 1 ];
		PIX_INT delta1 = (PIX_INT)( ( fval[ 2 ] / M_PI ) * 256.0F * 32768.0F );
		PIX_INT delta2 = (PIX_INT)( ( fval[ 3 ] / M_PI ) * 256.0F * 32768.0F );
		switch( c->type )
		{
		    case PIX_CONTAINER_TYPE_INT8: GENERATOR_OP_SIN8( int8_t, 1 ); break;
		    case PIX_CONTAINER_TYPE_INT16: GENERATOR_OP_SIN8( int16_t, 1 ); break;
		    case PIX_CONTAINER_TYPE_INT32: GENERATOR_OP_SIN8( int32_t, 1 ); break;
#ifdef PIX_INT64_ENABLED
		    case PIX_CONTAINER_TYPE_INT64: GENERATOR_OP_SIN8( int64_t, 1 ); break;
#endif
		    case PIX_CONTAINER_TYPE_FLOAT32: GENERATOR_OP_SIN8( float, 0 ); break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64: GENERATOR_OP_SIN8( double, 0 ); break;
#endif
		    default: rv = -1; break;
		}
	    }
	    break;
	case PIX_DATA_OPCODE_RAND:
	    {
		PIX_INT amp = (PIX_INT)fval[ 1 ];
		PIX_FLOAT amp_f = fval[ 1 ];
		switch( c->type )
		{
		    case PIX_CONTAINER_TYPE_INT8: GENERATOR_OP_RAND( int8_t, 1 ); break;
		    case PIX_CONTAINER_TYPE_INT16: GENERATOR_OP_RAND( int16_t, 1 ); break;
		    case PIX_CONTAINER_TYPE_INT32: GENERATOR_OP_RAND( int32_t, 1 ); break;
#ifdef PIX_INT64_ENABLED
		    case PIX_CONTAINER_TYPE_INT64: GENERATOR_OP_RAND( int64_t, 1 ); break;
#endif
		    case PIX_CONTAINER_TYPE_FLOAT32: GENERATOR_OP_RAND( float, 0 ); break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64: GENERATOR_OP_RAND( double, 0 ); break;
#endif
		    default: rv = -1; break;
		}
	    }
	    break;
	default:
	    PIX_VM_LOG( "generator: unknown opcode %d\n", opcode );
	    rv = -1;
	    break;
    }
    
    return rv;
}

PIX_INT pix_vm_wavetable_generator( 
    PIX_CID dest_cnum, 
    PIX_INT dest_off, 
    PIX_INT dest_len, 
    PIX_CID table_cnum, 
    PIX_CID amp_cnum, 
    PIX_CID amp_delta_cnum, 
    PIX_CID pos_cnum, 
    PIX_CID pos_delta_cnum,
    PIX_INT gen_offset,
    PIX_INT gen_step,
    PIX_INT gen_count,
    pix_vm* vm )
{
    PIX_INT rv = -1;

    pix_vm_container* dest_cont = pix_vm_get_container( dest_cnum, vm ); if( !dest_cont ) return -1;
    pix_vm_container* table_cont = pix_vm_get_container( table_cnum, vm ); if( !table_cont ) return -1;
    pix_vm_container* amp_cont = pix_vm_get_container( amp_cnum, vm ); if( !amp_cont ) return -1;
    pix_vm_container* amp_delta_cont = pix_vm_get_container( amp_delta_cnum, vm ); if( !amp_delta_cont ) return -1;
    pix_vm_container* pos_cont = pix_vm_get_container( pos_cnum, vm ); if( !pos_cont ) return -1;
    pix_vm_container* pos_delta_cont = pix_vm_get_container( pos_delta_cnum, vm ); if( !pos_delta_cont ) return -1;

    if( dest_off < 0 ) return -1;
    if( dest_len <= 0 ) return -1;
    if( dest_off >= dest_cont->size ) return -1;
    if( dest_off + dest_len > dest_cont->size ) return -1;

    if( amp_cont->type != PIX_CONTAINER_TYPE_INT32 )
    {
	PIX_VM_LOG( "wavetable_generator: amplitude container must be of type INT32\n" );
	return -1;
    }
    if( amp_delta_cont->type != PIX_CONTAINER_TYPE_INT32 )
    {
	PIX_VM_LOG( "wavetable_generator: amplitude delta container must be of type INT32\n" );
	return -1;
    }
    if( pos_cont->type != PIX_CONTAINER_TYPE_INT32 )
    {
	PIX_VM_LOG( "wavetable_generator: position container must be of type INT32\n" );
	return -1;
    }
    if( pos_delta_cont->type != PIX_CONTAINER_TYPE_INT32 )
    {
	PIX_VM_LOG( "wavetable_generator: position delta container must be of type INT32\n" );
	return -1;
    }

    int* RESTRICT amp = (int*)amp_cont->data; // << 16
    int* RESTRICT amp_delta = (int*)amp_delta_cont->data; // << 16
    int* RESTRICT pos = (int*)pos_cont->data; // << 16
    int* RESTRICT pos_delta = (int*)pos_delta_cont->data; // << 16
    int16_t* RESTRICT amp2 = ( (int16_t*)amp ) + 1; //max = 32767; 32768 -> overflow; should we use unsigned amp and amp2?
    int16_t* RESTRICT pos2 = ( (int16_t*)pos ) + 1;

    if( gen_offset + gen_count * gen_step > amp_cont->size ) //Bounds check
	gen_count = ( amp_cont->size - gen_offset ) / gen_step;

    if( dest_cont->type == PIX_CONTAINER_TYPE_INT16 )
    {
	int16_t* dest = (int16_t*)dest_cont->data + dest_off;
	int16_t* dest_end = dest + dest_len;
	switch( table_cont->type )
	{
	    case PIX_CONTAINER_TYPE_INT16:
		{
		    if( table_cont->size < 32768 )
		    {
			PIX_VM_LOG( "wavetable_generator: required size of wavetable container (type %s) is 32768\n", g_pix_container_type_names[ table_cont->type ] );
			break;
		    }
		    int16_t* table = (int16_t*)table_cont->data;
		    if( gen_step == 1 )
		    {
			for( ; dest != dest_end; dest++ )
			{
			    int val = 0;
			    for( int gen_num = gen_offset; gen_num < gen_offset + gen_count; gen_num++ )
			    {
				int amp_val = amp2[ gen_num * 2 ];
				int table_val = table[ pos2[ gen_num * 2 ] & 32767 ];
				val += ( table_val * amp_val ) >> 15;
				amp[ gen_num ] += amp_delta[ gen_num ];
				pos[ gen_num ] += pos_delta[ gen_num ];
			    }
			    if( val < -32768 ) val = -32768;
			    if( val > 32767 ) val = 32767;
			    *dest = (int16_t)val;
			}
		    }
		    else
		    {
			for( ; dest != dest_end; dest++ )
			{
			    int val = 0;
			    for( int gen_num = gen_offset; gen_num < gen_offset + gen_count * gen_step; gen_num += gen_step )
			    {
				int amp_val = amp2[ gen_num * 2 ];
				int table_val = table[ pos2[ gen_num * 2 ] & 32767 ];
				val += ( table_val * amp_val ) >> 15;
				amp[ gen_num ] += amp_delta[ gen_num ];
				pos[ gen_num ] += pos_delta[ gen_num ];
			    }
			    if( val < -32768 ) val = -32768;
			    if( val > 32767 ) val = 32767;
			    *dest = (int16_t)val;
			}
		    }
		    rv = 0;
		}
		break;
	    default:
		PIX_VM_LOG( "wavetable_generator: wavetable container of type %s is not supported for INT16 destination\n", g_pix_container_type_names[ table_cont->type ]  );
		break;
	}
    }

    if( dest_cont->type == PIX_CONTAINER_TYPE_INT32 )
    {
	int* dest = (int*)dest_cont->data + dest_off;
	int* dest_end = dest + dest_len;
	switch( table_cont->type )
	{
	    case PIX_CONTAINER_TYPE_INT16:
		{
		    if( table_cont->size < 32768 )
		    {
			PIX_VM_LOG( "wavetable_generator: required size of wavetable container (type %s) is 32768\n", g_pix_container_type_names[ table_cont->type ] );
			break;
		    }
		    int16_t* table = (int16_t*)table_cont->data;
		    if( gen_step == 1 )
		    {
			for( ; dest != dest_end; dest++ )
			{
			    int val = 0;
			    for( int gen_num = gen_offset; gen_num < gen_offset + gen_count; gen_num++ )
			    {
				int amp_val = amp2[ gen_num * 2 ];
				int table_val = table[ pos2[ gen_num * 2 ] & 32767 ];
				val += ( table_val * amp_val ) >> 15;
				amp[ gen_num ] += amp_delta[ gen_num ];
				pos[ gen_num ] += pos_delta[ gen_num ];
			    }
			    *dest = val;
			}
		    }
		    else
		    {
			for( ; dest != dest_end; dest++ )
			{
			    int val = 0;
			    for( int gen_num = gen_offset; gen_num < gen_offset + gen_count * gen_step; gen_num += gen_step )
			    {
				int amp_val = amp2[ gen_num * 2 ];
				int table_val = table[ pos2[ gen_num * 2 ] & 32767 ];
				val += ( table_val * amp_val ) >> 15;
				amp[ gen_num ] += amp_delta[ gen_num ];
				pos[ gen_num ] += pos_delta[ gen_num ];
			    }
			    *dest = val;
			}
		    }
		    rv = 0;
		}
		break;
	    default:
		PIX_VM_LOG( "wavetable_generator: wavetable container of type %s is not supported for INT32 destination\n", g_pix_container_type_names[ table_cont->type ] );
		break;
	}
    }

    if( dest_cont->type == PIX_CONTAINER_TYPE_FLOAT32 )
    {
	float* dest = (float*)dest_cont->data + dest_off;
	float* dest_end = dest + dest_len;
	switch( table_cont->type )
	{
	    case PIX_CONTAINER_TYPE_FLOAT32:
		{
		    if( table_cont->size < 32768 )
		    {
			PIX_VM_LOG( "wavetable_generator: required size of wavetable container (type %s) is 32768\n", g_pix_container_type_names[ table_cont->type ] );
			break;
		    }
		    float* table = (float*)table_cont->data;
		    for( ; dest != dest_end; dest++ )
		    {
			float val = 0;
			for( int gen_num = gen_offset; gen_num < gen_offset + gen_count * gen_step; gen_num += gen_step )
			{
			    int amp_val = amp2[ gen_num * 2 ];
			    float c = ( pos[ gen_num ] & 65535 ) / (float)65536;
			    float table_val =
				table[ pos2[ gen_num * 2 ] & 32767 ] * ( 1 - c )
				+
				table[ ( pos2[ gen_num * 2 ] + 1 ) & 32767 ] * c;
			    val += ( table_val * amp_val ) / 32768;
			    amp[ gen_num ] += amp_delta[ gen_num ];
			    pos[ gen_num ] += pos_delta[ gen_num ];
			}
			*dest = val;
		    }
		    rv = 0;
		}
		break;
	    default:
		PIX_VM_LOG( "wavetable_generator: wavetable container of type %s is not supported for FLOAT32 destination\n", g_pix_container_type_names[ table_cont->type ]  );
		break;
	}
    }

    return rv;
}

//Sample pointer (fixed point) precision:
#define SMP_PREC (int)16
#define SMP_INTERP_PREC ( SMP_PREC - 1 )
#define SMP_SIGNED_SUB64( v1, v1_p, v2, v2_p ) \
{ \
    v1 -= v2; v1_p -= v2_p; \
    if( v1_p < 0 ) { v1--; v1_p += ( (int)1 << SMP_PREC ); } \
}
#define SMP_SIGNED_ADD64( v1, v1_p, v2, v2_p ) \
{ \
    v1 += v2; v1_p += v2_p; \
    if( v1_p > ( (int)1 << SMP_PREC ) - 1 ) { v1++; v1_p -= ( (int)1 << SMP_PREC ); } \
}

PIX_INT pix_vm_sampler( pix_vm_container* pars_cont, pix_vm* vm )
{
    if( !pars_cont ) return 1;
    if( pars_cont->type != PIX_CONTAINER_TYPE_INT32 && pars_cont->type != PIX_CONTAINER_TYPE_INT64 ) return 1;
    if( pars_cont->size < PIX_SAMPLER_PARAMETERS ) return 1;

    PIX_CID dest;
    PIX_INT dest_off;
    PIX_INT dest_len;
    PIX_CID src;
    PIX_INT src_off_h;
    PIX_INT src_off_l;
    PIX_INT src_size;
    PIX_INT loop;
    PIX_INT loop_len;
    PIX_INT vol_i[ 2 ];
    PIX_FLOAT vol_f[ 2 ];
    bool normal = 0;
    PIX_INT delta;
    uint flags;

    if( pars_cont->type == PIX_CONTAINER_TYPE_INT32 )
    {
	int32_t* p = (int32_t*)pars_cont->data;
	dest = p[ PIX_SAMPLER_DEST ];
	dest_off = p[ PIX_SAMPLER_DEST_OFF ];
	dest_len = p[ PIX_SAMPLER_DEST_LEN ];
	src = p[ PIX_SAMPLER_SRC ];
	src_off_h = p[ PIX_SAMPLER_SRC_OFF_H ];
	src_off_l = p[ PIX_SAMPLER_SRC_OFF_L ];
	src_size = p[ PIX_SAMPLER_SRC_SIZE ];
	loop = p[ PIX_SAMPLER_LOOP ];
	loop_len = p[ PIX_SAMPLER_LOOP_LEN ];
	vol_i[ 0 ] = p[ PIX_SAMPLER_VOL1 ] / 32;
	vol_f[ 0 ] = (PIX_FLOAT)p[ PIX_SAMPLER_VOL1 ] / (PIX_FLOAT)32768;
	vol_i[ 1 ] = p[ PIX_SAMPLER_VOL2 ] / 32;
	vol_f[ 1 ] = (PIX_FLOAT)p[ PIX_SAMPLER_VOL2 ] / (PIX_FLOAT)32768;
	delta = p[ PIX_SAMPLER_DELTA ];
	flags = p[ PIX_SAMPLER_FLAGS ];
    }
    else
    {
#ifdef PIX_INT64_ENABLED
	int64_t* p = (int64_t*)pars_cont->data;
	dest = p[ PIX_SAMPLER_DEST ];
	dest_off = p[ PIX_SAMPLER_DEST_OFF ];
	dest_len = p[ PIX_SAMPLER_DEST_LEN ];
	src = p[ PIX_SAMPLER_SRC ];
	src_off_h = p[ PIX_SAMPLER_SRC_OFF_H ];
	src_off_l = p[ PIX_SAMPLER_SRC_OFF_L ];
	src_size = p[ PIX_SAMPLER_SRC_SIZE ];
	loop = p[ PIX_SAMPLER_LOOP ];
	loop_len = p[ PIX_SAMPLER_LOOP_LEN ];
	vol_i[ 0 ] = p[ PIX_SAMPLER_VOL1 ] / 32;
	vol_f[ 0 ] = (PIX_FLOAT)p[ PIX_SAMPLER_VOL1 ] / (PIX_FLOAT)32768;
	vol_i[ 1 ] = p[ PIX_SAMPLER_VOL2 ] / 32;
	vol_f[ 1 ] = (PIX_FLOAT)p[ PIX_SAMPLER_VOL2 ] / (PIX_FLOAT)32768;
	delta = p[ PIX_SAMPLER_DELTA ];
	flags = p[ PIX_SAMPLER_FLAGS ];
#else
	return 1;
#endif
    }
    
    if( (unsigned)dest >= (unsigned)vm->c_num ) return 1;
    if( (unsigned)src >= (unsigned)vm->c_num ) return 1;
    pix_vm_container* dest_cont = vm->c[ dest ];
    pix_vm_container* src_cont = vm->c[ src ];
    if( !dest_cont ) return 1;
    if( !src_cont ) return 1;
    
    if( dest_off >= dest_cont->size ) return 1;
    if( dest_off + dest_len > dest_cont->size )
	dest_len = (PIX_INT)dest_cont->size - dest_off;

    if( vol_i[ 0 ] == 32768 && vol_i [ 1 ] == 32768 )
    {
	normal = 1;
    }
    
    if( src_size == 0 ) src_size = src_cont->size;
    if( src_size > src_cont->size ) src_size = src_cont->size;

    PIX_INT loop_end = loop + loop_len;
    if( loop_end > src_size ) loop_end = src_size;
    
    PIX_INT cur_vol_i = vol_i[ 0 ] * 1024;
    PIX_FLOAT cur_vol_f = vol_f[ 0 ] * 1024;
    PIX_INT vol_delta_i = ( ( vol_i[ 1 ] - vol_i[ 0 ] ) * 1024 ) / dest_len;
    PIX_FLOAT vol_delta_f = ( vol_f[ 1 ] - vol_f[ 0 ] ) / (PIX_FLOAT)dest_len;
    
    int8_t* src_c = (int8_t*)src_cont->data;
    int16_t* src_s = (int16_t*)src_cont->data;
    int32_t* src_i = (int32_t*)src_cont->data;
#ifdef PIX_INT64_ENABLED
    int64_t* src_l = (int64_t*)src_cont->data;
#endif
    float* src_f = (float*)src_cont->data;
#ifdef PIX_FLOAT64_ENABLED
    double* src_d = (double*)src_cont->data;
#endif
    PIX_INT i;
    for( i = dest_off; i < dest_off + dest_len; i++ )
    {
	if( (unsigned)src_off_h >= (unsigned)src_size ) break; //End of sample
	PIX_INT v_i;
	PIX_FLOAT v_f;
	int interp = flags & PIX_SAMPLER_FLAG_INTERP_MASK;
	if( interp == 0 )
	{
	    //No interpolation:
	    switch( src_cont->type )
	    {
		case PIX_CONTAINER_TYPE_INT8:
		    v_i = src_c[ src_off_h ];
		    if( normal == 0 )
		    {
			v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			cur_vol_i += vol_delta_i;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT16:
		    v_i = src_s[ src_off_h ];
		    if( normal == 0 )
		    {
			v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			cur_vol_i += vol_delta_i;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT32:
		    v_i = src_i[ src_off_h ];
		    if( normal == 0 )
		    {
			v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			cur_vol_i += vol_delta_i;
		    }
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
		    v_i = src_l[ src_off_h ];
		    if( normal == 0 )
		    {
			v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			cur_vol_i += vol_delta_i;
		    }
		    break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32:
		    v_f = src_f[ src_off_h ];
		    if( normal == 0 )
		    {
			v_f = v_f * cur_vol_f;
			cur_vol_f += vol_delta_f;
		    }
		    break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64:
		    v_f = src_d[ src_off_h ];
		    if( normal == 0 )
		    {
			v_f = v_f * cur_vol_f;
			cur_vol_f += vol_delta_f;
		    }
		    break;
#endif
		default:
		    break;
	    }
	}
	else 
	{
	    //Interpolation:
	    PIX_INT s_offset0;
	    PIX_INT s_offset1 = src_off_h;
	    PIX_INT s_offset2 = src_off_h + 1;
	    PIX_INT s_offset3;
	    switch( interp )
	    {
		case PIX_SAMPLER_FLAG_INTERP2:
		    //Linear interpolation:
		    if( loop_len )
		    {
			//Loop correction:
			if( flags & PIX_SAMPLER_FLAG_PINGPONG )
			{
			    if( s_offset2 >= loop_end ) s_offset2 = s_offset1;
			}
			else 
			{
			    if( s_offset2 >= loop_end ) s_offset2 = loop;
			}
		    }
		    else 
		    {
			if( s_offset2 >= src_size ) s_offset2 = (PIX_INT)src_size - 1;
		    }
		    break;
		case PIX_SAMPLER_FLAG_INTERP4:
		    //Cubic interpolation:
		    s_offset3 = s_offset1 + 2;
		    s_offset0 = s_offset1 - 1;
		    if( loop_len )
		    {
			//Loop correction:
			if( flags & PIX_SAMPLER_FLAG_PINGPONG )
			{
			    if( ( flags & PIX_SAMPLER_FLAG_INLOOP ) && s_offset0 < loop ) s_offset0 = loop;
			    if( s_offset2 >= loop_end ) { s_offset2 = ( loop_end - 1 ) - ( s_offset2 - loop_end ); }
			    if( s_offset3 >= loop_end ) { s_offset3 = ( loop_end - 1 ) - ( s_offset3 - loop_end ); flags |= PIX_SAMPLER_FLAG_INLOOP; if( s_offset3 >= src_size ) s_offset3 = (PIX_INT)src_size - 1; }
			    if( s_offset3 < 0 ) s_offset3 = 0;
			}
			else 
			{
			    if( ( flags & PIX_SAMPLER_FLAG_INLOOP ) && s_offset0 < loop ) s_offset0 = loop_end - 1;
			    if( s_offset2 >= loop_end ) { s_offset2 -= loop_len; }
			    if( s_offset3 >= loop_end ) { s_offset3 -= loop_len; flags |= PIX_SAMPLER_FLAG_INLOOP; if( s_offset3 >= src_size ) s_offset3 = (PIX_INT)src_size - 1; }
			}
		    }
		    else 
		    {
			if( s_offset2 >= src_size ) s_offset2 = (PIX_INT)src_size - 1;
			if( s_offset3 >= src_size ) s_offset3 = (PIX_INT)src_size - 1;
		    }
		    if( s_offset0 < 0 ) s_offset0 = 0;
		    break;
	    }
	    int intr = src_off_l >> ( SMP_PREC - SMP_INTERP_PREC ); 
	    int intr2 = ( 1 << SMP_INTERP_PREC ) - 1 - intr;
	    if( interp == PIX_SAMPLER_FLAG_INTERP2 )
	    {
		//Linear interpolation:
		switch( src_cont->type )
		{
		    case PIX_CONTAINER_TYPE_INT8:
			v_i = (PIX_INT)src_c[ s_offset1 ] * intr2;
			v_i += (PIX_INT)src_c[ s_offset2 ] * intr;
			v_i >>= SMP_INTERP_PREC;
			if( normal == 0 )
			{
			    v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			    cur_vol_i += vol_delta_i;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT16:
			v_i = (PIX_INT)src_s[ s_offset1 ] * intr2;
			v_i += (PIX_INT)src_s[ s_offset2 ] * intr;
			v_i >>= SMP_INTERP_PREC;
			if( normal == 0 )
			{
			    v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			    cur_vol_i += vol_delta_i;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT32:
			v_i = (PIX_INT)src_i[ s_offset1 ] * intr2;
			v_i += (PIX_INT)src_i[ s_offset2 ] * intr;
			v_i >>= SMP_INTERP_PREC;
			if( normal == 0 )
			{
			    v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			    cur_vol_i += vol_delta_i;
			}
			break;
#ifdef PIX_INT64_ENABLED
		    case PIX_CONTAINER_TYPE_INT64:
			v_i = (PIX_INT)src_l[ s_offset1 ] * intr2;
			v_i += (PIX_INT)src_l[ s_offset2 ] * intr;
			v_i >>= SMP_INTERP_PREC;
			if( normal == 0 )
			{
			    v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			    cur_vol_i += vol_delta_i;
			}
			break;
#endif
		    case PIX_CONTAINER_TYPE_FLOAT32:
			v_f = src_f[ s_offset1 ] * ( (float)intr2 / 32768.0F );
			v_f += src_f[ s_offset2 ] * ( (float)intr / 32768.0F );
			if( normal == 0 )
			{
			    v_f = v_f * cur_vol_f;
			    cur_vol_f += vol_delta_f;
			}
			break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64:
			v_f = src_d[ s_offset1 ] * ( (double)intr2 / 32768.0F );
			v_f += src_d[ s_offset2 ] * ( (double)intr / 32768.0F );
			if( normal == 0 )
			{
			    v_f = v_f * cur_vol_f;
			    cur_vol_f += vol_delta_f;
			}
			break;
#endif
		    default:
			break;
		}
	    }
	    else if( interp == PIX_SAMPLER_FLAG_INTERP4 )
	    {
		//Cubic spline interpolation:
		switch( src_cont->type )
		{
		    case PIX_CONTAINER_TYPE_INT8:
			{
			    PIX_FLOAT y0 = src_c[ s_offset0 ];
			    PIX_FLOAT y1 = src_c[ s_offset1 ];
			    PIX_FLOAT y2 = src_c[ s_offset2 ];
			    PIX_FLOAT y3 = src_c[ s_offset3 ];
			    PIX_FLOAT mu = (PIX_FLOAT)src_off_l / (PIX_FLOAT)( 1 << SMP_PREC );
			    PIX_FLOAT a = ( 3 * ( y1-y2 ) - y0 + y3 ) / 2;
			    PIX_FLOAT b = 2 * y2 + y0 - ( 5 * y1 + y3 ) / 2;
			    PIX_FLOAT c2 = ( y2 - y0 ) / 2;
			    v_i = (PIX_INT)( ( ( ( a * mu ) + b ) * mu + c2 ) * mu + y1 );
			}
			if( normal == 0 )
			{
			    v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			    cur_vol_i += vol_delta_i;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT16:
			{
			    PIX_FLOAT y0 = src_s[ s_offset0 ];
			    PIX_FLOAT y1 = src_s[ s_offset1 ];
			    PIX_FLOAT y2 = src_s[ s_offset2 ];
			    PIX_FLOAT y3 = src_s[ s_offset3 ];
			    PIX_FLOAT mu = (PIX_FLOAT)src_off_l / (PIX_FLOAT)( 1 << SMP_PREC );
			    PIX_FLOAT a = ( 3 * ( y1-y2 ) - y0 + y3 ) / 2;
			    PIX_FLOAT b = 2 * y2 + y0 - ( 5 * y1 + y3 ) / 2;
			    PIX_FLOAT c2 = ( y2 - y0 ) / 2;
			    v_i = (PIX_INT)( ( ( ( a * mu ) + b ) * mu + c2 ) * mu + y1 );
			}
			if( normal == 0 )
			{
			    v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			    cur_vol_i += vol_delta_i;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT32:
			{
			    PIX_FLOAT y0 = src_i[ s_offset0 ];
			    PIX_FLOAT y1 = src_i[ s_offset1 ];
			    PIX_FLOAT y2 = src_i[ s_offset2 ];
			    PIX_FLOAT y3 = src_i[ s_offset3 ];
			    PIX_FLOAT mu = (PIX_FLOAT)src_off_l / (PIX_FLOAT)( 1 << SMP_PREC );
			    PIX_FLOAT a = ( 3 * ( y1-y2 ) - y0 + y3 ) / 2;
			    PIX_FLOAT b = 2 * y2 + y0 - ( 5 * y1 + y3 ) / 2;
			    PIX_FLOAT c2 = ( y2 - y0 ) / 2;
			    v_i = (PIX_INT)( ( ( ( a * mu ) + b ) * mu + c2 ) * mu + y1 );
			}
			if( normal == 0 )
			{
			    v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			    cur_vol_i += vol_delta_i;
			}
			break;
#ifdef PIX_INT64_ENABLED
		    case PIX_CONTAINER_TYPE_INT64:
			{
			    PIX_FLOAT y0 = src_l[ s_offset0 ];
			    PIX_FLOAT y1 = src_l[ s_offset1 ];
			    PIX_FLOAT y2 = src_l[ s_offset2 ];
			    PIX_FLOAT y3 = src_l[ s_offset3 ];
			    PIX_FLOAT mu = (PIX_FLOAT)src_off_l / (PIX_FLOAT)( 1 << SMP_PREC );
			    PIX_FLOAT a = ( 3 * ( y1-y2 ) - y0 + y3 ) / 2;
			    PIX_FLOAT b = 2 * y2 + y0 - ( 5 * y1 + y3 ) / 2;
			    PIX_FLOAT c2 = ( y2 - y0 ) / 2;
			    v_i = (PIX_INT)( ( ( ( a * mu ) + b ) * mu + c2 ) * mu + y1 );
			}
			if( normal == 0 )
			{
			    v_i = ( v_i * ( cur_vol_i / 1024 ) ) / 1024;
			    cur_vol_i += vol_delta_i;
			}
			break;
#endif
		    case PIX_CONTAINER_TYPE_FLOAT32:
			{
			    PIX_FLOAT y0 = src_f[ s_offset0 ];
			    PIX_FLOAT y1 = src_f[ s_offset1 ];
			    PIX_FLOAT y2 = src_f[ s_offset2 ];
			    PIX_FLOAT y3 = src_f[ s_offset3 ];
			    PIX_FLOAT mu = (PIX_FLOAT)src_off_l / (PIX_FLOAT)( 1 << SMP_PREC );
			    PIX_FLOAT a = ( 3 * ( y1-y2 ) - y0 + y3 ) / 2;
			    PIX_FLOAT b = 2 * y2 + y0 - ( 5 * y1 + y3 ) / 2;
			    PIX_FLOAT c2 = ( y2 - y0 ) / 2;
			    v_f = ( ( ( a * mu ) + b ) * mu + c2 ) * mu + y1;
			}
			if( normal == 0 )
			{
			    v_f = v_f * cur_vol_f;
			    cur_vol_f += vol_delta_f;
			}
			break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64:
			{
			    PIX_FLOAT y0 = src_d[ s_offset0 ];
			    PIX_FLOAT y1 = src_d[ s_offset1 ];
			    PIX_FLOAT y2 = src_d[ s_offset2 ];
			    PIX_FLOAT y3 = src_d[ s_offset3 ];
			    PIX_FLOAT mu = (PIX_FLOAT)src_off_l / (PIX_FLOAT)( 1 << SMP_PREC );
			    PIX_FLOAT a = ( 3 * ( y1-y2 ) - y0 + y3 ) / 2;
			    PIX_FLOAT b = 2 * y2 + y0 - ( 5 * y1 + y3 ) / 2;
			    PIX_FLOAT c2 = ( y2 - y0 ) / 2;
			    v_f = ( ( ( a * mu ) + b ) * mu + c2 ) * mu + y1;
			}
			if( normal == 0 )
			{
			    v_f = v_f * cur_vol_f;
			    cur_vol_f += vol_delta_f;
			}
			break;
#endif
		    default:
			break;
		}
	    }
	} // end of Interpolation block
	//Save the sample value:
	switch( dest_cont->type )
	{
	    case PIX_CONTAINER_TYPE_INT8:
		((int8_t*)dest_cont->data)[ i ] = v_i;
		break;
	    case PIX_CONTAINER_TYPE_INT16:
		((int16_t*)dest_cont->data)[ i ] = v_i;
		break;
	    case PIX_CONTAINER_TYPE_INT32:
		((int32_t*)dest_cont->data)[ i ] = v_i;
		break;
#ifdef PIX_INT64_ENABLED
	    case PIX_CONTAINER_TYPE_INT64:
		((int64_t*)dest_cont->data)[ i ] = v_i;
		break;
#endif
	    case PIX_CONTAINER_TYPE_FLOAT32:
		((float*)dest_cont->data)[ i ] = v_f;
		break;
#ifdef PIX_FLOAT64_ENABLED
	    case PIX_CONTAINER_TYPE_FLOAT64:
		((double*)dest_cont->data)[ i ] = v_f;
		break;
#endif
	    default:
		break;
	}
	//Go to the next sample:
	if( flags & PIX_SAMPLER_FLAG_REVERSE )
	{
	    src_off_h -= delta >> 16;
	    src_off_l -= delta & 0xFFFF;
	    if( src_off_l < 0 ) { src_off_h--; src_off_l = (unsigned)src_off_l & (unsigned)0xFFFF; }
	}
	else
	{
	    src_off_h += delta >> 16;
	    src_off_l += delta & 0xFFFF;
	    if( src_off_l > 0xFFFF ) { src_off_h++; src_off_l &= 0xFFFF; }
	}
	//Loop control:
	if( loop_len )
	{
	    if( src_off_h >= loop_end )
	    {
		if( ( flags & PIX_SAMPLER_FLAG_REVERSE ) == 0 )
		{
		    //Forward direction:
		    if( flags & PIX_SAMPLER_FLAG_PINGPONG )
		    {
			//Pingpong loop:
			PIX_INT rep_part = ( src_off_h - loop ) / loop_len;
			if( rep_part & 1 )
			{
			    flags |= PIX_SAMPLER_FLAG_REVERSE;
			    PIX_INT temp_off_h = src_off_h;
			    PIX_INT temp_off_l = src_off_l;
			    src_off_h = loop_end * ( rep_part + 1 );
			    src_off_l = 0;
			    SMP_SIGNED_SUB64( src_off_h, src_off_l, temp_off_h, temp_off_l );
			    src_off_h += loop;
			    if( src_off_h == loop_end && src_off_h > 0 )
                    	    {
                    	        src_off_h = loop_end - 1;
                        	src_off_l = ( 1 << SMP_PREC ) - 1;
                    	    }
			}
			else
			{
		    	    flags &= ~PIX_SAMPLER_FLAG_REVERSE;
		    	    src_off_h -= loop_len * rep_part;
			}
		    }
		    else
		    {
			//Normal loop:
			src_off_h = ( src_off_h - loop ) % loop_len + loop;
		    }
		} // end of Forward direction
	    }
	    else 
	    {
		if( src_off_h < loop )
		{
		    if( flags & PIX_SAMPLER_FLAG_REVERSE )
		    {
			//Backward direction:
			if( flags & PIX_SAMPLER_FLAG_PINGPONG )
			{
			    //Pingpong loop:
	    		    PIX_INT temp_off_h2 = src_off_h;
			    PIX_INT temp_off_l2 = src_off_l;
			    src_off_h = loop;
			    src_off_l = 0;
			    SMP_SIGNED_SUB64( src_off_h, src_off_l, temp_off_h2, temp_off_l2 );
			    PIX_INT rep_part = src_off_h / loop_len;
			    src_off_h += loop;
			    if( rep_part & 1 )
			    {
				flags |= PIX_SAMPLER_FLAG_REVERSE;
				PIX_INT temp_off_h = src_off_h;
				PIX_INT temp_off_l = src_off_l;
				src_off_h = loop_end * ( rep_part + 1 );
				src_off_l = 0;
				SMP_SIGNED_SUB64( src_off_h, src_off_l, temp_off_h, temp_off_l );
				src_off_h += loop;
			    }
			    else
			    {
				flags &= ~PIX_SAMPLER_FLAG_REVERSE;
				src_off_h -= loop_len * rep_part;
			    }
			}
			else
			{
			    //Normal loop:
			    src_off_h += ( ( ( loop - src_off_h - 1 ) / loop_len ) + 1 ) * loop_len;
			}
		    } // end of Backward direction
		}
	    }
	} // end of the Loop Control
    }
    //Clear empty area:
    if( i < dest_off + dest_len )
    {
	smem_clear( (int8_t*)dest_cont->data + i * g_pix_container_type_sizes[ dest_cont->type ], ( dest_off + dest_len - i ) * g_pix_container_type_sizes[ dest_cont->type ] );
    }

    //Save the Sampler variables back:
    if( pars_cont->type == PIX_CONTAINER_TYPE_INT32 )
    {
	int32_t* p = (int32_t*)pars_cont->data;
	p[ PIX_SAMPLER_SRC_OFF_H ] = src_off_h;
	p[ PIX_SAMPLER_SRC_OFF_L ] = src_off_l;
	p[ PIX_SAMPLER_FLAGS ] = flags;
    }
    else
    {
#ifdef PIX_INT64_ENABLED
	int64_t* p = (int64_t*)pars_cont->data;
	p[ PIX_SAMPLER_SRC_OFF_H ] = src_off_h;
	p[ PIX_SAMPLER_SRC_OFF_L ] = src_off_l;
	p[ PIX_SAMPLER_FLAGS ] = flags;
#endif
    }
    
    return 0;
}

PIX_INT pix_vm_envelope2p( PIX_CID cnum, PIX_INT v1, PIX_INT v2, PIX_INT offset, PIX_INT size, int8_t dc_off1_type, PIX_VAL dc_off1, int8_t dc_off2_type, PIX_VAL dc_off2, pix_vm* vm )
{
    pix_vm_container* c = pix_vm_get_container( cnum, vm );
    if( c == 0 ) return -1;
    
    if( size <= 0 ) size = c->size;
    if( offset + size <= 0 ) return -1;
    if( offset + size > c->size ) return -1;
    
    if( c->type == PIX_CONTAINER_TYPE_FLOAT32 || c->type == PIX_CONTAINER_TYPE_FLOAT64 )
    {
	PIX_FLOAT v1_f = (PIX_FLOAT)v1 / 32768;
	PIX_FLOAT v2_f = (PIX_FLOAT)v2 / 32768;
	PIX_FLOAT v_delta = ( v2_f - v1_f ) / size;
	PIX_FLOAT v = v1_f;
	
	PIX_FLOAT dc_off1_f;
	PIX_FLOAT dc_off2_f;
	if( dc_off1_type == 0 )
	    dc_off1_f = dc_off1.i;
	else
	    dc_off1_f = dc_off1.f;
	if( dc_off2_type == 0 )
	    dc_off2_f = dc_off2.i;
	else
	    dc_off2_f = dc_off2.f;
	PIX_FLOAT dc_off_delta = ( dc_off2_f - dc_off1_f ) / size;
	PIX_FLOAT dc_off = dc_off1_f;
	
	if( offset < 0 )
	{
	    v += v_delta * -offset;
	    dc_off += dc_off_delta * -offset;
	    size -= -offset;
	    offset = 0;
	}
	if( offset + size > c->size )
	{
	    size = offset + size - c->size;
	}
	if( size <= 0 ) return -1;
	
	switch( c->type )
	{
	    case PIX_CONTAINER_TYPE_FLOAT32:
		{
		    float* data = (float*)c->data;
		    for( PIX_INT i = offset; i < offset + size; i++ )
		    {
			float val = data[ i ];
			val = val * v + dc_off;
			data[ i ] = val;
			v += v_delta;
			dc_off += dc_off_delta;
		    }
		}
		break;
#ifdef PIX_FLOAT64_ENABLED
	    case PIX_CONTAINER_TYPE_FLOAT64:
		{
		    double* data = (double*)c->data;
		    for( PIX_INT i = offset; i < offset + size; i++ )
		    {
			double val = data[ i ];
			val = val * v + dc_off;
			data[ i ] = val;
			v += v_delta;
			dc_off += dc_off_delta;
		    }
		}
		break;
#endif
	    default:
		break;
	}
    }
    else
    {
	PIX_INT v_delta = ( ( v2 - v1 ) * 1024 ) / size;
	PIX_INT v = v1 * 1024;
	
	PIX_INT o1;
	PIX_INT o2;
	if( dc_off1_type == 0 )
	    o1 = dc_off1.i;
	else
	    o1 = dc_off1.f;
	if( dc_off2_type == 0 )
	    o2 = dc_off2.i;
	else
	    o2 = dc_off2.f;
	PIX_INT dc_off_delta = ( ( o2 - o1 ) * 1024 ) / size;
	PIX_INT dc_off = o1 * 1024;
	
	if( offset < 0 )
	{
	    v += v_delta * -offset;
	    dc_off += dc_off_delta * -offset;
	    size -= -offset;
	    offset = 0;
	}
	if( offset + size > c->size )
	{
	    size = offset + size - c->size;
	}
	if( size <= 0 ) return -1;

	bool overflow = 0;
	if( v1 > 32768 || ( v1 != 0 && o1 != 0 ) || v2 > 32768 || ( v2 != 0 && o2 != 0 ) )
	    overflow = 1;
	
	switch( c->type )
	{
	    case PIX_CONTAINER_TYPE_INT8:
		{
		    int8_t* data = (int8_t*)c->data;
		    if( overflow )
			for( PIX_INT i = offset; i < offset + size; i++ )
			{
			    int val = data[ i ];
			    val = ( val * ( v / 1024 ) ) / 32768 + dc_off / 1024;
			    if( val > 127 ) val = 127;
			    if( val < -128 ) val = -128;
			    data[ i ] = (int8_t)val;
			    v += v_delta;
			    dc_off += dc_off_delta;
			}
		    else
			for( PIX_INT i = offset; i < offset + size; i++ )
			{
			    int val = data[ i ];
			    val = ( val * ( v / 1024 ) ) / 32768 + dc_off / 1024;
			    data[ i ] = (int8_t)val;
			    v += v_delta;
			    dc_off += dc_off_delta;
			}
		}
		break;
	    case PIX_CONTAINER_TYPE_INT16:
		{
		    int16_t* data = (int16_t*)c->data;
		    if( overflow )
			for( PIX_INT i = offset; i < offset + size; i++ )
			{
			    int val = data[ i ];
			    val = ( val * ( v / 1024 ) ) / 32768 + dc_off / 1024;
			    if( val > 32767 ) val = 32767;
			    if( val < -32768 ) val = -32768;
			    data[ i ] = (int16_t)val;
			    v += v_delta;
			    dc_off += dc_off_delta;
			}
		    else
			for( PIX_INT i = offset; i < offset + size; i++ )
			{
			    int val = data[ i ];
			    val = ( val * ( v / 1024 ) ) / 32768 + dc_off / 1024;
			    data[ i ] = (int16_t)val;
			    v += v_delta;
			    dc_off += dc_off_delta;
			}
		}
		break;
	    case PIX_CONTAINER_TYPE_INT32:
		{
		    int* data = (int*)c->data;
		    for( PIX_INT i = offset; i < offset + size; i++ )
		    {
			int val = data[ i ];
			val = ( val * ( v / 1024 ) ) / 32768 + dc_off / 1024;
			data[ i ] = val;
			v += v_delta;
			dc_off += dc_off_delta;
		    }
		}
		break;
#ifdef PIX_INT64_ENABLED
	    case PIX_CONTAINER_TYPE_INT64:
		{
		    int64_t* data = (int64_t*)c->data;
		    for( PIX_INT i = offset; i < offset + size; i++ )
		    {
			int64_t val = data[ i ];
			val = ( val * ( v / 1024 ) ) / 32768 + dc_off / 1024;
			data[ i ] = val;
			v += v_delta;
			dc_off += dc_off_delta;
		    }
		}
		break;
#endif
	    default:
		break;
	}
    }
    
    return 0;
}

PIX_CID pix_vm_new_filter( uint flags, pix_vm* vm )
{	
    PIX_CID rv = pix_vm_new_container( -1, sizeof( pix_vm_filter ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    if( rv < 0 ) return -1;
    
    pix_vm_filter* f = (pix_vm_filter*)pix_vm_get_container_data( rv, vm );
    if( f == 0 ) return -1;
    smem_clear( f, sizeof( pix_vm_filter ) );
    
    return rv;
}

void pix_vm_remove_filter( PIX_CID f_c, pix_vm* vm )
{
    pix_vm_filter* f = (pix_vm_filter*)pix_vm_get_container_data( f_c, vm );
    if( f )
    {
	smem_free( f->a_i );
	smem_free( f->a_f );
	smem_free( f->b_i );
	smem_free( f->b_f );
	smem_free( f->input_state );
	smem_free( f->output_state );
	pix_vm_remove_container( f_c, vm );
    }
}

PIX_INT pix_vm_init_filter( PIX_CID f_c, PIX_CID a_c, PIX_CID b_c, int rshift, uint flags, pix_vm* vm )
{
    pix_vm_filter* f = (pix_vm_filter*)pix_vm_get_container_data( f_c, vm );
    if( f == 0 ) return -1;

    int a_count = 0;
    int b_count = 0;

    pix_vm_container* a_cc = pix_vm_get_container( a_c, vm );
    pix_vm_container* b_cc = pix_vm_get_container( b_c, vm );
    if( a_cc == 0 ) return -1;
    a_count = (int)a_cc->size;
    if( a_count == 0 ) return -1;
    if( b_cc ) b_count = (int)b_cc->size;

    if( f->a_count != a_count || f->b_count != b_count )
    {
	f->a_count = a_count;
	f->b_count = b_count;
	smem_free( f->input_state ); f->input_state = 0;
	smem_free( f->output_state ); f->output_state = 0;
	f->input_state_size = round_to_power_of_two( a_count );
	f->output_state_size = round_to_power_of_two( b_count );
	f->input_state_ptr = 0;
	f->output_state_ptr = 0;
    }
	
    int integer = 0;
    if( a_cc->type < PIX_CONTAINER_TYPE_FLOAT32 ) integer++;
    if( b_cc )
    {
        if( b_cc->type < PIX_CONTAINER_TYPE_FLOAT32 ) integer++;
    }
    else integer *= 2;
    if( integer >= 2 )
        f->int_coefs = 1;
    else
        f->int_coefs = 0;
        
    f->rshift = rshift;
	
    if( smem_get_size( f->a_i ) / sizeof( PIX_INT ) < a_count )
    {
        smem_free( f->a_i );
        smem_free( f->a_f );
    	f->a_i = (PIX_INT*)smem_new( a_count * sizeof( PIX_INT ) );
	f->a_f = (PIX_FLOAT*)smem_new( a_count * sizeof( PIX_FLOAT ) );
    }
    for( size_t i = 0; i < a_count; i++ )
    {
	switch( a_cc->type )
	{
	    case PIX_CONTAINER_TYPE_INT8:
	        f->a_i[ i ] = ((int8_t*)a_cc->data)[ i ];
	        f->a_f[ i ] = f->a_i[ i ];
	        break;
	    case PIX_CONTAINER_TYPE_INT16:
	        f->a_i[ i ] = ((int16_t*)a_cc->data)[ i ];
	        f->a_f[ i ] = f->a_i[ i ];
	        break;
	    case PIX_CONTAINER_TYPE_INT32:
	        f->a_i[ i ] = ((int32_t*)a_cc->data)[ i ];
	        f->a_f[ i ] = f->a_i[ i ];
	        break;
#ifdef PIX_INT64_ENABLED
	    case PIX_CONTAINER_TYPE_INT64:
	        f->a_i[ i ] = ((int64_t*)a_cc->data)[ i ];
	        f->a_f[ i ] = f->a_i[ i ];
	        break;
#endif
	    case PIX_CONTAINER_TYPE_FLOAT32:
	        f->a_f[ i ] = ((float*)a_cc->data)[ i ];
	        f->a_i[ i ] = f->a_f[ i ];
	        break;
#ifdef PIX_FLOAT64_ENABLED
	    case PIX_CONTAINER_TYPE_FLOAT64:
	        f->a_f[ i ] = ((double*)a_cc->data)[ i ];
	        f->a_i[ i ] = f->a_f[ i ];
	        break;
#endif
	}
    }
	
    if( b_count > 0 )
    {
    	if( smem_get_size( f->b_i ) / sizeof( PIX_INT ) < b_count )
        {
            smem_free( f->b_i );
            smem_free( f->b_f );
            f->b_i = (PIX_INT*)smem_new( b_count * sizeof( PIX_INT ) );
	    f->b_f = (PIX_FLOAT*)smem_new( b_count * sizeof( PIX_FLOAT ) );
	}
	for( size_t i = 0; i < b_count; i++ )
	{
	    switch( b_cc->type )
	    {
	        case PIX_CONTAINER_TYPE_INT8:
	    	    f->b_i[ i ] = ((int8_t*)b_cc->data)[ i ];
	    	    f->b_f[ i ] = f->b_i[ i ];
		    break;
		case PIX_CONTAINER_TYPE_INT16:
		    f->b_i[ i ] = ((int16_t*)b_cc->data)[ i ];
		    f->b_f[ i ] = f->b_i[ i ];
		    break;
		case PIX_CONTAINER_TYPE_INT32:
		    f->b_i[ i ] = ((int32_t*)b_cc->data)[ i ];
		    f->b_f[ i ] = f->b_i[ i ];
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
		    f->b_i[ i ] = ((int64_t*)b_cc->data)[ i ];
		    f->b_f[ i ] = f->b_i[ i ];
		    break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32:
		    f->b_f[ i ] = ((float*)b_cc->data)[ i ];
		    f->b_i[ i ] = f->b_f[ i ];
		    break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64:
		    f->b_f[ i ] = ((double*)b_cc->data)[ i ];
		    f->b_i[ i ] = f->b_f[ i ];
		    break;
#endif
	    }
	}
    }
    
    return 0;
}

void pix_vm_reset_filter( PIX_CID f_c, pix_vm* vm )
{
    pix_vm_filter* f = (pix_vm_filter*)pix_vm_get_container_data( f_c, vm );
    if( f )
    {
	smem_zero( f->input_state );
	smem_zero( f->output_state );
    }
}

#define APPLY_FILTER( TYPE, CALC_TYPE, FLOAT, SHIFT_CODE ) \
{ \
    TYPE* output = (TYPE*)output_cc->data; \
    TYPE* input = (TYPE*)input_cc->data; \
    TYPE* input_state = (TYPE*)f->input_state; \
    TYPE* output_state = (TYPE*)f->output_state; \
    if( f->int_coefs && FLOAT == 0 ) \
    { \
	/* Int coefficients */ \
	if( b_count > 0 ) \
        { \
    	    /* Recursive */ \
	    for( PIX_INT n = offset; n < offset + size; n++ ) \
	    { \
		input_state_ptr++; \
		input_state[ input_state_ptr & input_state_size ] = input[ n ]; \
		CALC_TYPE val = f->a_i[ 0 ] * input_state[ input_state_ptr & input_state_size ]; \
		for( int i = 1; i < a_count; i++ ) \
		{ \
	    	    val += f->a_i[ i ] * input_state[ ( input_state_ptr - i ) & input_state_size ]; \
    		} \
		for( int i = 0; i < b_count; i++ ) \
		{ \
		    val += f->b_i[ i ] * output_state[ ( output_state_ptr - i ) & output_state_size ]; \
		} \
		SHIFT_CODE; \
		output[ n ] = (TYPE)val; \
		output_state_ptr++; \
		output_state[ output_state_ptr & output_state_size ] = val; \
	    } \
	} \
	else \
	{ \
	    for( PIX_INT n = offset; n < offset + size; n++ ) \
	    { \
		input_state_ptr++; \
		input_state[ input_state_ptr & input_state_size ] = input[ n ]; \
		CALC_TYPE val = f->a_i[ 0 ] * input_state[ input_state_ptr & input_state_size ]; \
		for( int i = 1; i < a_count; i++ ) \
		{ \
		    val += f->a_i[ i ] * input_state[ ( input_state_ptr - i ) & input_state_size ]; \
		} \
		SHIFT_CODE; \
		output[ n ] = (TYPE)val; \
	    } \
	} \
    } \
    else \
    { \
	/* Float coefficients */ \
	if( b_count > 0 ) \
        { \
    	    /* Recursive */ \
    	    while( 1 ) \
    	    { \
    		if( a_count == 3 && b_count == 2 ) \
    		{ \
		    for( PIX_INT n = offset; n < offset + size; n++ ) \
    		    { \
    			static int input_state_mask = 3; \
    			static int output_state_mask = 1; \
			input_state_ptr++; \
			input_state[ input_state_ptr & input_state_mask ] = input[ n ]; \
			CALC_TYPE val = f->a_f[ 0 ] * input_state[ input_state_ptr & input_state_mask ]; \
			for( int i = 1; i < 3; i++ ) \
			{ \
	    	    	    val += f->a_f[ i ] * input_state[ ( input_state_ptr - i ) & input_state_mask ]; \
    			} \
			for( int i = 0; i < 2; i++ ) \
			{ \
			    val += f->b_f[ i ] * output_state[ ( output_state_ptr - i ) & output_state_mask ]; \
			} \
			output[ n ] = (TYPE)val; \
			output_state_ptr++; \
			output_state[ output_state_ptr & output_state_mask ] = val; \
		    } \
    		    break; \
    		} \
		for( PIX_INT n = offset; n < offset + size; n++ ) \
    		{ \
		    input_state_ptr++; \
		    input_state[ input_state_ptr & input_state_size ] = input[ n ]; \
		    CALC_TYPE val = f->a_f[ 0 ] * input_state[ input_state_ptr & input_state_size ]; \
		    for( int i = 1; i < a_count; i++ ) \
		    { \
	    	        val += f->a_f[ i ] * input_state[ ( input_state_ptr - i ) & input_state_size ]; \
    		    } \
		    for( int i = 0; i < b_count; i++ ) \
		    { \
			val += f->b_f[ i ] * output_state[ ( output_state_ptr - i ) & output_state_size ]; \
		    } \
		    output[ n ] = (TYPE)val; \
		    output_state_ptr++; \
		    output_state[ output_state_ptr & output_state_size ] = val; \
		} \
		break; \
	    } \
	} \
	else \
	{ \
	    for( PIX_INT n = offset; n < offset + size; n++ ) \
	    { \
		input_state_ptr++; \
		input_state[ input_state_ptr & input_state_size ] = input[ n ]; \
		CALC_TYPE val = f->a_f[ 0 ] * input_state[ input_state_ptr & input_state_size ]; \
		for( int i = 1; i < a_count; i++ ) \
		{ \
		    val += f->a_f[ i ] * input_state[ ( input_state_ptr - i ) & input_state_size ]; \
		} \
		output[ n ] = (TYPE)val; \
	    } \
	} \
    } \
}

PIX_INT pix_vm_apply_filter( PIX_CID f_c, PIX_CID output_c, PIX_CID input_c, uint flags, PIX_INT offset, PIX_INT size, pix_vm* vm )
{
    PIX_INT rv = -1;
    
    while( 1 )
    {
	pix_vm_filter* f = (pix_vm_filter*)pix_vm_get_container_data( f_c, vm );
	if( f == 0 ) break;
	
	if( f->a_count == 0 || f->a_i == 0 ) break;

	//Input and Output:	
	pix_vm_container* output_cc = pix_vm_get_container( output_c, vm );
	if( output_cc == 0 ) break;
	pix_vm_container* input_cc = pix_vm_get_container( input_c, vm );
	if( input_cc == 0 ) break;
	if( output_cc->type != input_cc->type )
	{
	    PIX_VM_LOG( "filter: output.type(%s) != input.type(%s); output type must be equal to input type\n", g_pix_container_type_names[ output_cc->type ], g_pix_container_type_names[ input_cc->type ] );
	    break;
	}
	
	//State buffers:
	int a_count = f->a_count;
	int b_count = f->b_count;
	int input_state_size = f->input_state_size;
	int output_state_size = f->output_state_size;
	int type_size = g_pix_container_type_sizes[ output_cc->type ];
	if( smem_get_size( f->input_state ) < input_state_size * type_size )
	{
	    smem_free( f->input_state );
	    f->input_state = smem_new( input_state_size * type_size );
	    smem_zero( f->input_state );
	    f->input_state_ptr = 0;
	}
	if( b_count > 0 && smem_get_size( f->output_state ) < output_state_size * type_size )
	{
	    smem_free( f->output_state );
	    f->output_state = smem_new( output_state_size * type_size );
	    smem_zero( f->output_state );
	    f->output_state_ptr = 0;
	}
	uint input_state_ptr = f->input_state_ptr;
	uint output_state_ptr = f->output_state_ptr;
	input_state_size--; //Make mask
	output_state_size--; //Make mask
	
	//Size control:
	if( offset >= (PIX_INT)output_cc->size ) break;
	if( offset >= (PIX_INT)input_cc->size ) break;
	if( size == -1 ) size = output_cc->size;
	if( offset + size > (PIX_INT)output_cc->size ) { size = (PIX_INT)output_cc->size - offset; }
	if( offset + size > (PIX_INT)input_cc->size ) { size = (PIX_INT)input_cc->size - offset; }
	if( offset < 0 ) { size += offset; offset = 0; }
	if( size <= 0 ) break;
	
	switch( output_cc->type )
	{
	    case PIX_CONTAINER_TYPE_INT8:
    		APPLY_FILTER( int8_t, int32_t, 0, val >>= f->rshift );
		break;
	    case PIX_CONTAINER_TYPE_INT16:
    		APPLY_FILTER( int16_t, int32_t, 0, val >>= f->rshift );
		break;
	    case PIX_CONTAINER_TYPE_INT32:
    		APPLY_FILTER( int32_t, int32_t, 0, val >>= f->rshift );
		break;
#ifdef PIX_INT64_ENABLED
	    case PIX_CONTAINER_TYPE_INT64:
    		APPLY_FILTER( int64_t, int64_t, 0, val >>= f->rshift );
		break;
#endif
	    case PIX_CONTAINER_TYPE_FLOAT32:
    		APPLY_FILTER( float, float, 1, /**/ );
		break;
#ifdef PIX_FLOAT64_ENABLED
	    case PIX_CONTAINER_TYPE_FLOAT64:
    		APPLY_FILTER( double, double, 1, /**/ );
		break;
#endif
	}
	rv = 0;

	f->input_state_ptr = input_state_ptr;
	f->output_state_ptr = output_state_ptr;

	break;
    }

    return rv;
}

inline PIX_INT blend8( PIX_INT val1, PIX_INT val2, int pos )
{
    return ( val1 * ( 255 - pos ) + val2 * pos ) >> 8;
}

inline PIX_INT blend15( PIX_INT val1, PIX_INT val2, int pos )
{
    return ( val1 * ( 32767 - pos ) + val2 * pos ) >> 15;
}

inline float fblend30( float val1, float val2, int pos )
{
    float fpos = (float)pos / (float)( 1 << 30 );
    return val1 * ( 1 - fpos ) + val2 * fpos;
}

inline double dblend30( double val1, double val2, int pos )
{
    double fpos = (double)pos / (double)( 1 << 30 );
    return val1 * ( 1 - fpos ) + val2 * fpos;
}

#define PIX_VM_RESIZE_INTERP( cont_type, pos_bits, blend_func ) \
{ \
    PIX_INT cur_y = 0; \
    cont_type* dest = (cont_type*)pars->dest + pars->dest_y * pars->dest_xsize + pars->dest_x; \
    int interp = 0; \
    if( PIX_RESIZE_INTERP_TYPE( pars->resize_flags ) == PIX_RESIZE_INTERP2 ) interp = 1; \
    for( PIX_INT y = 0; y < pars->dest_rect_ysize; y++ ) \
    { \
        cont_type* src = (cont_type*)pars->src + ( ( cur_y >> yprec ) + pars->src_y ) * pars->src_xsize + pars->src_x; \
        PIX_INT cur_x = 0; \
        if( interp == 0 ) \
        { \
            for( PIX_INT x = 0; x < pars->dest_rect_xsize; x++ ) \
            { \
                *dest = src[ cur_x >> xprec ]; \
                cur_x += xd; \
                dest++; \
            } \
        } \
        else \
        { \
            cont_type* src2; \
            if( ( cur_y >> yprec ) + 1 < pars->src_rect_ysize ) \
                src2 = src + pars->src_xsize; \
            else \
                src2 = src; \
            int ypos_shift = yprec - pos_bits; \
            int xpos_shift = xprec - pos_bits; \
            int blend_ypos; \
            if( ypos_shift >= 0 ) \
                blend_ypos = ( cur_y & ( ( 1 << yprec ) - 1 ) ) >> ypos_shift; \
            else \
                blend_ypos = ( cur_y & ( ( 1 << yprec ) - 1 ) ) << -ypos_shift; \
            if( pars->src_rect_xsize > 1 ) \
            { \
                for( PIX_INT x = 0; x < pars->dest_rect_xsize; x++ ) \
                { \
                    int blend_xpos; \
                    if( xpos_shift >= 0 ) \
                        blend_xpos = ( cur_x & ( ( 1 << xprec ) - 1 ) ) >> xpos_shift; \
                    else \
                        blend_xpos = ( cur_x & ( ( 1 << xprec ) - 1 ) ) << -xpos_shift; \
                    PIX_INT cur_x1 = cur_x >> xprec; \
                    PIX_INT cur_x2 = ( cur_x >> xprec ) + 1; if( cur_x2 >= pars->src_rect_xsize ) cur_x2--; \
                    cont_type c1 = blend_func( src[ cur_x1 ], src[ cur_x2 ], blend_xpos ); \
                    cont_type c2 = blend_func( src2[ cur_x1 ], src2[ cur_x2 ], blend_xpos ); \
                    *dest = blend_func( c1, c2, blend_ypos ); \
                    cur_x += xd; \
                    dest++; \
                } \
            } \
            else \
            { \
                for( PIX_INT x = 0; x < pars->dest_rect_xsize; x++ ) \
                { \
                    *dest = blend_func( src[ cur_x >> xprec ], src2[ cur_x >> xprec ], blend_ypos ); \
                    dest++; \
                } \
            } \
        } \
        cur_y += yd; \
        dest += pars->dest_xsize - pars->dest_rect_xsize; \
    } \
}

void pix_vm_copy_and_resize( pix_vm_resize_pars* pars )
{
    if( pars->dest_rect_xsize == 0 && pars->dest_rect_ysize == 0 )
    {
	//Whole container:
	pars->dest_x = 0;
	pars->dest_y = 0;
	pars->dest_rect_xsize = pars->dest_xsize;
	pars->dest_rect_ysize = pars->dest_ysize;
    }
    else
    {
	if( pars->dest_x >= pars->dest_xsize ) return;
	if( pars->dest_y >= pars->dest_ysize ) return;
	if( pars->src_x >= pars->src_xsize ) return;
	if( pars->src_y >= pars->src_ysize ) return;
    }
    if( pars->dest_x < 0 && pars->dest_rect_xsize ) 
    { 
	PIX_INT tv = ( pars->dest_x * pars->src_rect_xsize ) / pars->dest_rect_xsize;
	pars->dest_rect_xsize += pars->dest_x;
	pars->src_x -= tv;
	pars->src_rect_xsize += tv;
	pars->dest_x = 0; 
    }
    if( pars->dest_y < 0 && pars->dest_rect_ysize ) 
    { 
	PIX_INT tv = ( pars->dest_y * pars->src_rect_ysize ) / pars->dest_rect_ysize;
	pars->dest_rect_ysize += pars->dest_y; 
	pars->src_y -= tv;
	pars->src_rect_ysize += tv;
	pars->dest_y = 0; 
    }
    if( pars->dest_rect_xsize <= 0 ) return;
    if( pars->dest_rect_ysize <= 0 ) return;
    if( pars->dest_x + pars->dest_rect_xsize > pars->dest_xsize && pars->dest_rect_xsize ) 
    {
	pars->src_rect_xsize -= ( ( pars->dest_x + pars->dest_rect_xsize - pars->dest_xsize ) * pars->src_rect_xsize ) / pars->dest_rect_xsize;	
	pars->dest_rect_xsize = pars->dest_xsize - pars->dest_x;
    }
    if( pars->dest_y + pars->dest_rect_ysize > pars->dest_ysize )
    {
	pars->src_rect_ysize -= ( ( pars->dest_y + pars->dest_rect_ysize - pars->dest_ysize ) * pars->src_rect_ysize ) / pars->dest_rect_ysize;	
	pars->dest_rect_ysize = pars->dest_ysize - pars->dest_y;
    }

    if( pars->src_rect_xsize == 0 && pars->src_rect_ysize == 0 )
    {
	//Whole container:
	pars->src_x = 0;
	pars->src_y = 0;
	pars->src_rect_xsize = pars->src_xsize;
	pars->src_rect_ysize = pars->src_ysize;
    }
    if( pars->src_x < 0 ) { pars->src_rect_xsize += pars->src_x; pars->src_x = 0; }
    if( pars->src_y < 0 ) { pars->src_rect_ysize += pars->src_y; pars->src_y = 0; }
    if( pars->src_rect_xsize <= 0 ) return;
    if( pars->src_rect_ysize <= 0 ) return;
    if( pars->src_x + pars->src_rect_xsize > pars->src_xsize ) pars->src_rect_xsize = pars->src_xsize - pars->src_x;
    if( pars->src_y + pars->src_rect_ysize > pars->src_ysize ) pars->src_rect_ysize = pars->src_ysize - pars->src_y;

    int xsize_bits = 0;
    int ysize_bits = 0;
    for( int b = 0; b < sizeof( PIX_INT ) * 8 - 1; b++ )
    {
	if( ( 1 << b ) >= pars->src_rect_xsize )
        {
            xsize_bits = b + 1;
            break;
        }
    }
    for( int b = 0; b < sizeof( PIX_INT ) * 8 - 1; b++ )
    {
        if( ( 1 << b ) >= pars->src_rect_ysize )
        {
            ysize_bits = b + 1;
            break;
        }
    }
    int xprec = sizeof( PIX_INT ) * 8 - 1 - xsize_bits;
    int yprec = sizeof( PIX_INT ) * 8 - 1 - ysize_bits;
    PIX_INT xd = 0, yd = 0;
    if( PIX_RESIZE_INTERP_TYPE( pars->resize_flags ) == PIX_RESIZE_INTERP2 )
    {
        if( pars->dest_rect_xsize > 1 && pars->src_rect_xsize > 1 ) xd = ( ( pars->src_rect_xsize - 1 ) << xprec ) / ( pars->dest_rect_xsize - 1 );
        if( pars->dest_rect_ysize > 1 && pars->src_rect_ysize > 1 ) yd = ( ( pars->src_rect_ysize - 1 ) << yprec ) / ( pars->dest_rect_ysize - 1 );
    }
    else
    {
        if( pars->dest_rect_xsize > 0 ) xd = ( pars->src_rect_xsize << xprec ) / pars->dest_rect_xsize;
        if( pars->dest_rect_ysize > 0 ) yd = ( pars->src_rect_ysize << yprec ) / pars->dest_rect_ysize;
    }
    if( PIX_RESIZE_INTERP_OPTIONS( pars->resize_flags ) == PIX_RESIZE_INTERP_COLOR )
    {
        //Color interpolation:
        PIX_VM_RESIZE_INTERP( COLOR, 8, blend );
    }
    else
    {
	//Data interpolation:
        if( ( PIX_RESIZE_INTERP_OPTIONS( pars->resize_flags ) == PIX_RESIZE_INTERP_UNSIGNED ) && ( pars->type < PIX_CONTAINER_TYPE_FLOAT32 ) )
        {
            //Unsigned:
            switch( pars->type )
            {
                case PIX_CONTAINER_TYPE_INT8:
                    PIX_VM_RESIZE_INTERP( uint8_t, 8, blend8 );
                    break;
                case PIX_CONTAINER_TYPE_INT16:
                    PIX_VM_RESIZE_INTERP( uint16_t, 15, blend15 );
                    break;
                case PIX_CONTAINER_TYPE_INT32:
                    PIX_VM_RESIZE_INTERP( uint, 15, blend15 );
                    break;
#ifdef PIX_INT64_ENABLED
                case PIX_CONTAINER_TYPE_INT64:
                    PIX_VM_RESIZE_INTERP( uint64_t, 15, blend15 );
                    break;
#endif
            }
        }
        else
        {
            //Signed:
            switch( pars->type )
            {
                case PIX_CONTAINER_TYPE_INT8:
                    PIX_VM_RESIZE_INTERP( int8_t, 8, blend8 );
                    break;
                case PIX_CONTAINER_TYPE_INT16:
                    PIX_VM_RESIZE_INTERP( int16_t, 15, blend15 );
                    break;
                case PIX_CONTAINER_TYPE_INT32:
                    PIX_VM_RESIZE_INTERP( int, 15, blend15 );
                    break;
#ifdef PIX_INT64_ENABLED
                case PIX_CONTAINER_TYPE_INT64:
                    PIX_VM_RESIZE_INTERP( int64_t, 15, blend15 );
                    break;
#endif
                case PIX_CONTAINER_TYPE_FLOAT32:
                    PIX_VM_RESIZE_INTERP( float, 30, fblend30 );
                    break;
#ifdef PIX_FLOAT64_ENABLED
                case PIX_CONTAINER_TYPE_FLOAT64:
                    PIX_VM_RESIZE_INTERP( double, 30, dblend30 );
                    break;
#endif
            }
        }
    }
}

//MODE: 0 - int; 1 - float; 2 - color;
#define CONV_FILTER_LINE( TYPE, UTYPE, KERNEL_TYPE, MODE, MIN, MAX, EDGE_HANDLING, UNSIGNED ) \
{ \
    KERNEL_TYPE div, offset; \
    if( MODE == 1 ) \
	{ div = pars->div.f; offset = pars->offset.f; } \
    else \
	{ div = pars->div.i; offset = pars->offset.i; } \
    TYPE* d = (TYPE*)dest_ptr; \
    TYPE* s = (TYPE*)src_ptr; \
    int asize = 1; if( MODE == 2 ) asize = 3; \
    for( PIX_INT x = 0; x < xsize; x += pars->xstep, d += pars->xstep, s += pars->xstep ) \
    { \
	KERNEL_TYPE a[ asize ]; \
        for( int i = 0; i < asize; i++ ) a[ i ] = 0; \
	KERNEL_TYPE* k = (KERNEL_TYPE*)pars->kernel->data; \
	for( PIX_INT yy = 0; yy < pars->kernel->ysize; yy++ ) \
	{ \
	    PIX_INT yyy = yy - pars->kernel_ycenter; \
	    if( EDGE_HANDLING ) \
	    { \
		if( yyy + y + src_y < 0 ) { yyy = -src_y - y; } \
		if( yyy + y + src_y >= pars->src->ysize ) { yyy = pars->src->ysize - 1 - y - src_y; } \
	    } \
	    TYPE* ss = s + yyy * pars->src->xsize - pars->kernel_xcenter; \
	    for( PIX_INT xx = 0; xx < pars->kernel->xsize; xx++ ) \
	    { \
		PIX_INT xx2 = xx; \
		if( EDGE_HANDLING ) \
		{ \
		    PIX_INT xxx = xx - pars->kernel_xcenter + x + src_x; \
		    if( xxx < 0 ) xx2 = pars->kernel_xcenter - x - src_x; \
		    if( xxx >= pars->src->xsize ) xx2 = pars->src->xsize - 1 + pars->kernel_xcenter - x - src_x; \
		} \
		if( MODE == 2 ) \
		{ \
		    KERNEL_TYPE kk = k[ 0 ]; \
		    a[ 0 ] += red( ss[ xx2 ] ) * kk; \
		    a[ 1 ] += green( ss[ xx2 ] ) * kk; \
		    a[ 2 ] += blue( ss[ xx2 ] ) * kk; \
		} \
		else \
		{ \
		    if( UNSIGNED ) \
			a[ 0 ] += ((UTYPE*)ss)[ xx2 ] * k[ 0 ]; \
		    else \
			a[ 0 ] += ss[ xx2 ] * k[ 0 ]; \
		} \
		k++; \
	    } \
	} \
	if( MODE == 1 ) \
	{ \
	    a[ 0 ] = a[ 0 ] * div + offset; \
	} \
	else \
	{ \
	    for( int i = 0; i < asize; i++ ) \
	    { \
		if( div == 1 ) { a[ i ] = (PIX_INT)a[ i ] >> pars->div_rshift; } else a[ i ] = a[ i ] / div; \
		a[ i ] += offset; \
		if( MAX != 0 ) LIMIT_NUM( a[ i ], MIN, MAX ); \
	    } \
	} \
	if( MODE == 0 || MODE == 1 ) \
	{ \
	    d[ 0 ] = a[ 0 ]; \
	} \
	if( MODE == 2 ) \
	{ \
	    d[ 0 ] = get_color( a[ 0 ], a[ 1 ], a[ 2 ] ); \
	} \
    } \
}

//Kernel, Convolution matrix, Convolution filter, Mask
//Kernel must be flipped for correct convolution? Otherwise it is the correlation?
PIX_INT pix_vm_conv_filter( pix_vm* vm, pix_vm_conv_filter_pars* pars )
{
    PIX_INT rv = -1;
    while( 1 )
    {
	if( pars->dest->type != pars->src->type )
	{
	    PIX_VM_LOG( "conv_filter: dest.type(%s) != src.type(%s); dest/src types must be the same\n", g_pix_container_type_names[ pars->dest->type ], g_pix_container_type_names[ pars->src->type ] );
    	    break;
	}
	int type = pars->dest->type;
	int el_size = g_pix_container_type_sizes[ type ];
	if( type < PIX_CONTAINER_TYPE_FLOAT32 )
	{
	    if( ( pars->kernel->type >= PIX_CONTAINER_TYPE_FLOAT32 ) || g_pix_container_type_sizes[ pars->kernel->type ] != sizeof( PIX_INT ) )
	    {
		PIX_VM_LOG( "conv_filter: kernel type must be INT for integer dest/src\n" );
		break;
	    }
	    if( pars->div_type != 0 )
	    {
		pars->div.i = pars->div.f;
		pars->div_type = 0;
	    }
	    if( pars->offset_type != 0 )
	    {
		pars->offset.i = pars->offset.f;
		pars->offset_type = 0;
	    }
	    if( pars->div.i == 0 ) pars->div.i = 1;
	    bool div_sign = 0;
	    if( pars->div.i < 0 ) { pars->div.i = -pars->div.i; div_sign = 1; }
	    pars->div_rshift = 0;
	    int v = 1;
	    while( v < pars->div.i )
	    {
		pars->div_rshift++;
		v <<= 1;
	    }
	    if( ( 1 << pars->div_rshift ) == pars->div.i )
	    {
		//Shift instead of division:
		pars->div.i = 1;
	    }
	    else
	    {
		//Division:
		pars->div_rshift = 0;
		if( div_sign ) pars->div.i = -pars->div.i;
	    }
	}
	else
	{
	    if( ( pars->kernel->type < PIX_CONTAINER_TYPE_FLOAT32 ) || g_pix_container_type_sizes[ pars->kernel->type ] != sizeof( PIX_FLOAT ) )
	    {
		PIX_VM_LOG( "conv_filter: kernel type must be FLOAT for floating point dest/src\n" );
		break;
	    }
	    if( pars->div_type != 1 )
	    {
		pars->div.f = pars->div.i;
		pars->div_type = 1;
	    }
	    if( pars->offset_type != 1 )
	    {
		pars->offset.f = pars->offset.i;
		pars->offset_type = 1;
	    }
	    if( pars->div.f == 0 ) pars->div.f = 1;
	    pars->div.f = 1 / pars->div.f;
	}
	if( pars->xstep == 0 ) pars->xstep = 1;
	if( pars->ystep == 0 ) pars->ystep = 1;
	if( pars->xsize <= 0 ) pars->xsize = pars->dest->xsize;
	if( pars->ysize <= 0 ) pars->ysize = pars->dest->ysize;
	if( pars->dest_x < 0 ) { pars->xsize += pars->dest_x; pars->src_x -= pars->dest_x; pars->dest_x = 0; }
	if( pars->dest_y < 0 ) { pars->ysize += pars->dest_y; pars->src_y -= pars->dest_y; pars->dest_y = 0; }
	if( pars->src_x < 0 ) { pars->xsize += pars->src_x; pars->dest_x -= pars->src_x; pars->src_x = 0; }
	if( pars->src_y < 0 ) { pars->ysize += pars->src_y; pars->dest_y -= pars->src_y; pars->src_y = 0; }
	if( pars->dest_x + pars->xsize > pars->dest->xsize ) pars->xsize = pars->dest->xsize - pars->dest_x;
	if( pars->dest_y + pars->ysize > pars->dest->ysize ) pars->ysize = pars->dest->ysize - pars->dest_y;
	if( pars->src_x + pars->xsize > pars->src->xsize ) pars->xsize = pars->src->xsize - pars->src_x;
	if( pars->src_y + pars->ysize > pars->src->ysize ) pars->ysize = pars->src->ysize - pars->src_y;
	if( (unsigned)pars->kernel_xcenter >= (unsigned)pars->kernel->xsize ) pars->kernel_xcenter = 0;
	if( (unsigned)pars->kernel_ycenter >= (unsigned)pars->kernel->ysize ) pars->kernel_ycenter = 0;
	
	PIX_INT dest_x = pars->dest_x;
	PIX_INT dest_y = pars->dest_y;
	PIX_INT src_x = pars->src_x;
	PIX_INT src_y = pars->src_y;
	PIX_INT xsize = pars->xsize;
	PIX_INT ysize = pars->ysize;
	PIX_INT cut[ 4 ];
	cut[ 0 ] = -( src_x - pars->kernel_xcenter ); //xcut1
	cut[ 1 ] = -( src_y - pars->kernel_ycenter ); //ycut1
	cut[ 2 ] = ( src_x + xsize + ( pars->kernel->xsize - ( pars->kernel_xcenter + 1 ) ) ) - pars->src->xsize; //xcut2
	cut[ 3 ] = ( src_y + ysize + ( pars->kernel->ysize - ( pars->kernel_ycenter + 1 ) ) ) - pars->src->ysize; //ycut2
	if( cut[ 0 ] > 0 ) { xsize -= cut[ 0 ]; dest_x += cut[ 0 ]; src_x += cut[ 0 ]; } else cut[ 0 ] = 0;
	if( cut[ 1 ] > 0 ) { ysize -= cut[ 1 ]; dest_y += cut[ 1 ]; src_y += cut[ 1 ]; } else cut[ 1 ] = 0;
	if( cut[ 2 ] > 0 ) xsize -= cut[ 2 ]; else cut[ 2 ] = 0;
	if( cut[ 3 ] > 0 ) ysize -= cut[ 3 ]; else cut[ 3 ] = 0;
	
	int8_t* dest_ptr = (int8_t*)pars->dest->data + ( dest_y * pars->dest->xsize + dest_x ) * el_size;
	int8_t* src_ptr = (int8_t*)pars->src->data + ( src_y * pars->src->xsize + src_x ) * el_size;
	for( PIX_INT y = 0; y < ysize; y += pars->ystep )
	{
	    if( PIX_CONV_FILTER_TYPE( pars->flags ) == PIX_CONV_FILTER_TYPE_COLOR )
    	    {
		CONV_FILTER_LINE( COLOR, COLOR, PIX_INT, 2, 0, 255, false, false );
    	    }
    	    else
    	    switch( type )
    	    {
        	case PIX_CONTAINER_TYPE_INT8:
        	    if( pars->flags & PIX_CONV_FILTER_UNSIGNED )
			CONV_FILTER_LINE( int8_t, uint8_t, PIX_INT, 0, 0, 255, false, true )
		    else
			CONV_FILTER_LINE( int8_t, uint8_t, PIX_INT, 0, -128, 127, false, false )
            	    break;
        	case PIX_CONTAINER_TYPE_INT16:
        	    if( pars->flags & PIX_CONV_FILTER_UNSIGNED )
			CONV_FILTER_LINE( int16_t, uint16_t, PIX_INT, 0, 0, 65535, false, true )
		    else
			CONV_FILTER_LINE( int16_t, uint16_t, PIX_INT, 0, -32768, 32767, false, false )
            	    break;
        	case PIX_CONTAINER_TYPE_INT32:
		    CONV_FILTER_LINE( int32_t, uint32_t, PIX_INT, 0, 0, 0, false, false );
            	    break;
#ifdef PIX_INT64_ENABLED
        	case PIX_CONTAINER_TYPE_INT64:
		    CONV_FILTER_LINE( int64_t, uint64_t, PIX_INT, 0, 0, 0, false, false );
        	    break;
#endif
                case PIX_CONTAINER_TYPE_FLOAT32:
		    CONV_FILTER_LINE( float, float, PIX_FLOAT, 1, 0, 0, false, false );
            	    break;
#ifdef PIX_FLOAT64_ENABLED
                case PIX_CONTAINER_TYPE_FLOAT64:
		    CONV_FILTER_LINE( double, double, PIX_FLOAT, 1, 0, 0, false, false );
            	    break;
#endif
		default: break;
    	    }
    	    dest_ptr += pars->dest->xsize * el_size * pars->ystep;
    	    src_ptr += pars->src->xsize * el_size * pars->ystep;
	}

	if( PIX_CONV_FILTER_BORDER( pars->flags ) != PIX_CONV_FILTER_BORDER_SKIP )
	for( int i = 0; i < 4; i++ )
	{
	    if( cut[ i ] <= 0 ) continue;
	    switch( i )
	    {
		case 0:
		    dest_x = pars->dest_x;
		    dest_y = pars->dest_y;
		    src_x = pars->src_x;
		    src_y = pars->src_y;
		    xsize = cut[ i ];
		    ysize = pars->ysize;
		    break;
		case 1:
		    dest_x = pars->dest_x + cut[ 0 ];
		    dest_y = pars->dest_y;
		    src_x = pars->src_x + cut[ 0 ];
		    src_y = pars->src_y;
		    xsize = pars->xsize - cut[ 0 ] - cut[ 2 ];
		    ysize = cut[ i ];
		    break;
		case 2:
		    dest_x = pars->dest_x + pars->xsize - cut[ i ];
		    dest_y = pars->dest_y;
		    src_x = pars->src_x + pars->xsize - cut[ i ];
		    src_y = pars->src_y;
		    xsize = cut[ i ];
		    ysize = pars->ysize;
		    break;
		case 3:
		    dest_x = pars->dest_x + cut[ 0 ];
		    dest_y = pars->dest_y + pars->ysize - cut[ i ];
		    src_x = pars->src_x + cut[ 0 ];
		    src_y = pars->src_y + pars->ysize - cut[ i ];
		    xsize = pars->xsize - cut[ 0 ] - cut[ 2 ];
		    ysize = cut[ i ];
		    break;
	    }
	    //printf( "cut %d = %d; %d %d; %d %d; %d %d;\n", i, cut[ i ], dest_x, dest_y, src_x, src_y, xsize, ysize );
	    dest_ptr = (int8_t*)pars->dest->data + ( dest_y * pars->dest->xsize + dest_x ) * el_size;
	    src_ptr = (int8_t*)pars->src->data + ( src_y * pars->src->xsize + src_x ) * el_size;
	    for( PIX_INT y = 0; y < ysize; y += pars->ystep )
	    {
    		if( PIX_CONV_FILTER_TYPE( pars->flags ) == PIX_CONV_FILTER_TYPE_COLOR )
    		{
		    CONV_FILTER_LINE( COLOR, COLOR, PIX_INT, 2, 0, 255, true, false );
    		}
    		else
    		switch( type )
    		{
        	    case PIX_CONTAINER_TYPE_INT8:
        		{
        		    PIX_INT min = -128;
        		    PIX_INT max = 127;
        		    bool u = 0;
        		    if( pars->flags & PIX_CONV_FILTER_UNSIGNED )
        		    {
        			u = 1;
        			min = 0;
        			max = 255;
        		    }
			    CONV_FILTER_LINE( int8_t, uint8_t, PIX_INT, 0, min, max, true, u );
			}
            		break;
        	    case PIX_CONTAINER_TYPE_INT16:
        		{
        		    PIX_INT min = -32768;
        		    PIX_INT max = 32767;
        		    bool u = 0;
        		    if( pars->flags & PIX_CONV_FILTER_UNSIGNED )
        		    {
        			u = 1;
        			min = 0;
        			max = 65535;
        		    }
			    CONV_FILTER_LINE( int16_t, uint16_t, PIX_INT, 0, min, max, true, u );
			}
            		break;
        	    case PIX_CONTAINER_TYPE_INT32:
			CONV_FILTER_LINE( int32_t, uint32_t, PIX_INT, 0, 0, 0, true, false );
            		break;
#ifdef PIX_INT64_ENABLED
        	    case PIX_CONTAINER_TYPE_INT64:
			CONV_FILTER_LINE( int64_t, uint64_t, PIX_INT, 0, 0, 0, true, false );
        		break;
#endif
            	    case PIX_CONTAINER_TYPE_FLOAT32:
		        CONV_FILTER_LINE( float, float, PIX_FLOAT, 1, 0, 0, true, false );
            		break;
#ifdef PIX_FLOAT64_ENABLED
            	    case PIX_CONTAINER_TYPE_FLOAT64:
		        CONV_FILTER_LINE( double, double, PIX_FLOAT, 1, 0, 0, true, false );
            		break;
#endif
		    default: break;
		}
    		dest_ptr += pars->dest->xsize * el_size * pars->ystep;
    		src_ptr += pars->src->xsize * el_size * pars->ystep;
	    }
	}

	rv = 0;
	break;
    }
    return rv;
}
