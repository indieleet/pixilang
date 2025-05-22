/*
    pixilang_vm_font.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) slog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif

int pix_vm_set_font(
    uint32_t first_char,
    uint32_t last_char,
    PIX_CID cnum,
    int xchars,
    int ychars,
    int char_xsize,
    int char_ysize,
    int char_xsize2,
    int char_ysize2,
    int grid_xoffset,
    int grid_yoffset,
    int grid_cell_xsize,
    int grid_cell_ysize,
    pix_vm* vm )
{
    pix_vm_font* font = NULL;
    bool create = false;
    bool change_container = false;

    for( int n = 0; n < vm->fonts_num; n++ )
    {
	font = &vm->fonts[ n ];
	if( font->first == first_char )
	{
	    if( font->font != cnum ) change_container = true;
	    break;
	}
	font = NULL;
    }
    if( font == NULL )
    {
	//No font for selected first_char. Create it:
	for( int n = 0; n < vm->fonts_num; n++ )
	{
	    font = &vm->fonts[ n ];
	    if( font->font == -1 )
	    {
		create = true;
		break;
	    }
	    font = NULL;
	}
    }
    if( font == NULL )
    {
	return 1;
    }

    if( create )
    {
	//Set defaults:
	font->first = 0;
	font->last = 16 * 16 - 1;
	font->xchars = 16;
	font->ychars = 16;
	font->char_xsize = 8;
	font->char_ysize = 8;
        font->grid_xoffset = 0;
	font->grid_yoffset = 0;
	change_container = true;
    }

    font->font = cnum;
    font->first = first_char;
    if( xchars > 0 ) font->xchars = xchars;
    if( ychars > 0 ) font->ychars = ychars;

    if( create && xchars > 0 && ychars > 0 )
    {
	font->last = font->first + font->xchars * font->ychars - 1;
    }

    if( change_container )
    {
        //Change the container:
        pix_vm_container* c = pix_vm_get_container( cnum, vm );
        if( c )
        {
    	    font->char_xsize = c->xsize / font->xchars;
	    font->char_ysize = c->ysize / font->ychars;
	}
	font->char_xsize2 = font->char_xsize;
	font->char_ysize2 = font->char_ysize;
	font->grid_cell_xsize = font->char_xsize;
    	font->grid_cell_ysize = font->char_ysize;
    }

    if( last_char ) font->last = last_char;
    if( char_xsize > 0 ) font->char_xsize = char_xsize;
    if( char_ysize > 0 ) font->char_ysize = char_ysize;
    if( char_xsize2 > 0 ) font->char_xsize2 = char_xsize2;
    if( char_ysize2 > 0 ) font->char_ysize2 = char_ysize2;
    if( grid_xoffset > 0 ) font->grid_xoffset = grid_xoffset;
    if( grid_yoffset > 0 ) font->grid_yoffset = grid_yoffset;
    if( grid_cell_xsize > 0 ) font->grid_cell_xsize = grid_cell_xsize;
    if( grid_cell_ysize > 0 ) font->grid_cell_ysize = grid_cell_ysize;

    return 0;
}
