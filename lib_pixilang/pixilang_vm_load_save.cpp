/*
    pixilang_vm_load_save.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"

#ifndef PIX_NOPNG
#include "png.h"

static void user_png_read( png_structp png_ptr, png_bytep data, png_size_t length )
{
    sfs_file f = *(sfs_file*)png_get_io_ptr( png_ptr );
    sfs_read( data, 1, length, f );
}

static void user_png_write( png_structp png_ptr, png_bytep data, png_size_t length )
{
    sfs_file f = *(sfs_file*)png_get_io_ptr( png_ptr );
    sfs_write( data, 1, length, f );
}

static void user_png_flush( png_structp png_ptr )
{
    sfs_file f = *(sfs_file*)png_get_io_ptr( png_ptr );
    sfs_flush( f );
}

#endif

#ifndef PIX_NOGIF
#include "gif_lib.h"

int g_gif_interlaced_offset[] = { 0, 4, 2, 1 }; // The way Interlaced image should
int g_gif_interlaced_jumps[] = { 8, 8, 4, 2 };  // be read - offsets and jumps... 
GifColorType g_gif_bwcolors[ 2 ] = { {0,0,0}, {255,255,255} };
const uint8_t g_gif_netscape20ext[] = "NETSCAPE2.0";

static int user_gif_read( GifFileType* gf, GifByteType* buf, int count )
{
    sfs_file f = *(sfs_file*)gf->UserData;
    return sfs_read( buf, 1, count, f );
}

static int user_gif_write( GifFileType* gf, const GifByteType* buf, int count )
{
    sfs_file f = *(sfs_file*)gf->UserData;
    return sfs_write( (void*)buf, 1, count, f );
}

static int gif_load_create_frame( int frame_num, PIX_CID cnum, GifFileType* gf, pix_vm* vm )
{
    pix_vm_container* cont = pix_vm_get_container( cnum, vm );
    if( cont == 0 ) return -1;
    
    if( frame_num > 0 )
    {
	//Create new frame:
	if( pix_vm_get_container_hdata( cnum, vm ) == 0 )
	    pix_vm_create_container_hdata( cnum, pix_vm_container_hdata_type_anim, sizeof( pix_vm_container_hdata_anim ), vm );
	pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)pix_vm_get_container_hdata( cnum, vm );
	if( hdata == 0 ) return -1;
	hdata->frame_count = frame_num + 1;
	if( hdata->frames == 0 )
	    hdata->frames = (pix_vm_anim_frame*)smem_new( hdata->frame_count * sizeof( pix_vm_anim_frame ) );
	if( hdata->frames == 0 ) return -1;
	if( hdata->frame_count > smem_get_size( hdata->frames ) / sizeof( pix_vm_anim_frame ) )
	{
	    hdata->frames = (pix_vm_anim_frame*)smem_resize( hdata->frames, hdata->frame_count * sizeof( pix_vm_anim_frame ) );
	    if( hdata->frames == 0 ) return -1;
	}
	pix_vm_anim_frame* f = &hdata->frames[ frame_num ];
	smem_clear( f, sizeof( pix_vm_anim_frame ) );
	
	PIX_VAL prop_val;
	prop_val.i = hdata->frame_count; pix_vm_set_container_property( cnum, "frames", -1, 0, prop_val, vm );
	
	if( frame_num == 1 )
	{
	    //Create first frame:
	    smem_clear( &hdata->frames[ 0 ], sizeof( pix_vm_anim_frame ) );
	    pix_vm_container* c = pix_vm_get_container( cnum, vm );
	    if( c && c->data )
	    {
		pix_vm_container_hdata_pack_frame_from_buf( cnum, 0, (COLORPTR)c->data, c->type, c->xsize, c->ysize, vm );
	    }
	}
	
	//Copy previous frame to current:
	pix_vm_anim_frame* prev_f = &hdata->frames[ frame_num - 1 ];
	smem_copy( f, prev_f, sizeof( pix_vm_anim_frame ) );
	f->pixels = (COLORPTR)smem_new( smem_get_size( prev_f->pixels ) );
	smem_copy( f->pixels, prev_f->pixels, smem_get_size( prev_f->pixels ) );
    }
    
    return 0;
}

static COLORPTR gif_load_get_frame_ptr( int frame_num, PIX_CID cnum, GifFileType* gf, pix_vm* vm )
{
    COLORPTR dest = 0;
    if( frame_num == 0 )
	dest = (COLORPTR)pix_vm_get_container_data( cnum, vm );
    else
    {
	pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)pix_vm_get_container_hdata( cnum, vm );
	dest = (COLORPTR)smem_new( gf->SWidth * gf->SHeight * COLORLEN );
	if( dest )
	{
	    if( pix_vm_container_hdata_unpack_frame_to_buf( cnum, frame_num, dest, vm ) != 0 )
	    {
		smem_free( dest );
		dest = 0;
	    }
	}
    }
    return dest;
}

static void gif_load_release_frame_ptr( COLORPTR p, int frame_num, PIX_CID cnum, GifFileType* gf, pix_vm* vm )
{
    if( p == 0 ) return;
    if( frame_num > 0 )
    {
	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont == 0 ) return;
	pix_vm_container_hdata_pack_frame_from_buf( cnum, frame_num, p, cont->type, gf->SWidth, gf->SHeight, vm );
	smem_free( p );
    }
}

static GifColorType* gif_load_get_colormap( int frame_num, PIX_CID cnum, GifFileType* gf, pix_vm* vm )
{
    GifColorType* cmap = 0;
    if( gf->SColorMap )
    {
	cmap = gf->SColorMap->Colors;
    }
    if( gf->Image.ColorMap )
    {
	cmap = gf->Image.ColorMap->Colors;
    }
    if( cmap == 0 )
    {
	cmap = g_gif_bwcolors;
    }
    return cmap;
}

#endif

PIX_CID pix_vm_load( const char* filename, sfs_file f, int par1, pix_vm* vm )
{
    PIX_CID rv = -1;
    
    sfs_file_fmt format = SFS_FILE_FMT_UNKNOWN;
    
    if( filename && f == 0 ) f = sfs_open( filename, "rb" );
    if( f == 0 ) return -1;

    if( ( par1 & PIX_LOAD_AS_RAW ) == 0 )
    {
	sfs_file_fmt fmt = sfs_get_file_format( NULL, f );
	if( fmt != SFS_FILE_FMT_UNKNOWN )
	    format = fmt;
    }
    
    PIX_VAL prop_val;
    
    switch( format )
    {
#ifndef PIX_NOJPEG
	case SFS_FILE_FMT_JPEG:
	    {
		simage_desc img = simage_desc();
		img.format = PFMT_SUNDOG_COLOR;
		sfs_load_jpeg( vm->wm->sd, nullptr, f, &img );
		if( img.data )
		{
		    rv = pix_vm_new_container( -1, img.width, img.height, 32, img.data, vm );
		}
	    }
	    break;
#endif
#ifndef PIX_NOPNG
	case SFS_FILE_FMT_PNG:
	    {
		png_structp png_ptr = 0;
		png_infop info_ptr = 0;
		png_bytep* row_pointers = 0;
		int width = 0;
		int height = 0;
		int color_type = 0;
		int bit_depth = 0;
		int channels = 0;
		int number_of_passes = 0;
		bool color_transformation = 0;
		png_bytep trans = 0;
		int num_trans = 0;
		png_color_16p trans_values = 0;
		COLORPTR palette = 0;
		int palette_colors = 0;
		
		while( 1 )
		{
		    png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
		    if( !png_ptr )
		    {
			PIX_VM_LOG( "PNG loading: png_create_read_struct failed\n" );
			break;
		    }
		    info_ptr = png_create_info_struct( png_ptr );
		    if( !info_ptr )
		    {
			PIX_VM_LOG( "PNG loading: png_create_info_struct failed\n" );
			break;
		    }
		    if( setjmp( png_jmpbuf( png_ptr ) ) )
		    {
			PIX_VM_LOG( "PNG loading: error during read_info\n" );
			break;
		    }
		    
		    png_set_read_fn( png_ptr, &f, user_png_read );
		    png_read_info( png_ptr, info_ptr );

		    width = png_get_image_width( png_ptr, info_ptr );
		    height = png_get_image_height( png_ptr, info_ptr );
		    color_type = png_get_color_type( png_ptr, info_ptr );
		    bit_depth = png_get_bit_depth( png_ptr, info_ptr );
		    channels = png_get_channels( png_ptr, info_ptr );

		    if( 0 ) //color_type == PNG_COLOR_TYPE_PALETTE )
		    {
			//Expand paletted colors into true RGB triplets:
			// (optional)
			png_set_expand( png_ptr );
			color_transformation = 1;
		    }
		    
		    if( png_get_tRNS( png_ptr, info_ptr, &trans, &num_trans, &trans_values ) )
		    {
			if( num_trans > 1 )
			{
			    //More than one transparent colors:
			    //Expand into RGBA:
			    png_set_expand( png_ptr );
			    color_transformation = 1;
			}
		    }

		    number_of_passes = png_set_interlace_handling( png_ptr );
		    png_read_update_info( png_ptr, info_ptr );
		    
		    //Read:
		    if( setjmp( png_jmpbuf( png_ptr ) ) )
		    {
			PIX_VM_LOG( "PNG loading: error during read_image\n" );
			break;
		    }
		    row_pointers = (png_bytep*)smem_new( sizeof( png_bytep ) * height );
		    for( int y = 0; y < height; y++ )
			row_pointers[ y ] = (png_byte*)smem_new( png_get_rowbytes( png_ptr, info_ptr ) );
		    png_read_image( png_ptr, row_pointers );
		    
		    if( color_transformation )
		    {
			color_type = png_get_color_type( png_ptr, info_ptr );
			bit_depth = png_get_bit_depth( png_ptr, info_ptr );
			channels = png_get_channels( png_ptr, info_ptr );
		    }
		    
		    if( png_get_tRNS( png_ptr, info_ptr, &trans, &num_trans, &trans_values ) == 0 )
		    {
			trans_values = 0;
		    }
		    
		    if( color_type == PNG_COLOR_TYPE_PALETTE )
		    {
			png_colorp pp = 0;
			int items = 0;
			if( png_get_PLTE( png_ptr, info_ptr, &pp, &items ) )
			{
			    palette_colors = items;
			    palette = (COLORPTR)smem_new( items * sizeof( COLOR ) );
			    for( int i = 0; i < items; i++ )
			    {
				int r = pp[ i ].red;
				int g = pp[ i ].green;
				int b = pp[ i ].blue;
				palette[ i ] = get_color( r, g, b );
			    }
			}
		    }
		    
		    //Create a container:
		    PIX_CID alpha = -1;
		    uint8_t* alpha_data = 0;
		    COLORPTR dest = 0;
		    rv = pix_vm_new_container( -1, width, height, 32, 0, vm );
		    if( rv >= 0 )
		    {
			dest = (COLORPTR)pix_vm_get_container_data( rv, vm );
			if( color_type & PNG_COLOR_MASK_ALPHA )
			{
			    alpha = pix_vm_new_container( -1, width, height, PIX_CONTAINER_TYPE_INT8, 0, vm );
			    pix_vm_set_container_alpha( rv, alpha, vm );
			    alpha_data = (uint8_t*)pix_vm_get_container_data( alpha, vm );
			}
			COLOR transp_color = 0;
			int transp_r = 0;
			int transp_g = 0;
			int transp_b = 0;
			if( trans_values )
			{
			    while( 1 )
			    {
				if( palette )
				{
				    transp_color = palette[ trans_values[ 0 ].index ];
				    bool fix_colors = 0;
				    for( int i = 0; i < palette_colors; i++ )
					if( i != trans_values[ 0 ].index )
					    if( transp_color == palette[ i ] )
					    {
						fix_colors = 1;
						break;
					    }
				    if( fix_colors )
				    {
					for( int i = 0; i < palette_colors; i++ )
					    if( palette[ i ] == 1 )
						palette[ i ] = 2;
					palette[ trans_values[ 0 ].index ] = 1;
					transp_color = 1;
				    }
				    pix_vm_set_container_key_color( rv, transp_color, vm );
				    break;
				}
				if( bit_depth == 8 && channels == 1 )
				{
				    int t = trans_values[ 0 ].gray;
				    transp_r = t;
				    transp_color = get_color( t, t, t );
				    if( sizeof( COLOR ) <= 2 )
				    {
					transp_color = 1;
				    }
				    pix_vm_set_container_key_color( rv, transp_color, vm );
				    break;
				}
				if( bit_depth == 8 && channels == 3 )
				{
				    int r = trans_values[ 0 ].red;
				    int g = trans_values[ 0 ].green;
				    int b = trans_values[ 0 ].blue;
				    transp_r = r;
				    transp_g = g;
				    transp_b = b;
				    transp_color = get_color( r, g, b );
				    if( sizeof( COLOR ) <= 2 )
				    {
					transp_color = 1;
				    }
				    pix_vm_set_container_key_color( rv, transp_color, vm );
				    break;
				}
				break;
			    }
			}
			for( int y = 0; y < height; y++ )
			{
			    if( bit_depth == 8 )
			    {
				//8bit per channel:
				uint8_t* src = row_pointers[ y ];
				switch( channels )
				{
				    case 1:
					if( palette )
					{
					    //Palette:
					    for( int x = 0; x < width; x++ )
					    {
						int i = *src; src++;
						*dest = palette[ i ];
						dest++;
					    }
					}
					else
					{
					    //Grayscale:
					    for( int x = 0; x < width; x++ )
					    {
						int grey = *src; src++;
						*dest = get_color( grey, grey, grey );
						if( sizeof( COLOR ) <= 2 )
						{
						    if( transp_color == 1 )
						    {
							if( transp_r == grey )
							    *dest = 1;
							else
							    if( *dest == 1 )
								*dest = 2;
						    }
						}
						dest++;
					    }
					}
					break;
				    case 2:
					if( palette )
					{
					    //Palette:
					    for( int x = 0; x < width; x++ )
					    {
						int i = *src; src++;
						int a = *src; src++;
						*dest = palette[ i ];
						dest++;
						*alpha_data = a;
						alpha_data++;
					    }
					}
					else
					{
					    //Grayscale:
					    for( int x = 0; x < width; x++ )
					    {
						int grey = *src; src++;
						int a = *src; src++;
						*dest = get_color( grey, grey, grey );
						if( sizeof( COLOR ) <= 2 )
						{
						    if( transp_color == 1 )
						    {
							if( transp_r == grey )
							    *dest = 1;
							else
							    if( *dest == 1 )
								*dest = 2;
						    }
						}
						dest++;
						*alpha_data = a;
						alpha_data++;
					    }
					}
					break;
				    case 3:
					for( int x = 0; x < width; x++ )
					{
					    int r = *src; src++;
					    int g = *src; src++;
					    int b = *src; src++;
					    *dest = get_color( r, g, b );
					    if( sizeof( COLOR ) <= 2 )
					    {
						if( sizeof( COLOR ) <= 2 )
						{
						    if( transp_color == 1 )
						    {
							if( transp_r == r && transp_g == g && transp_b == b )
							    *dest = 1;
							else
							    if( *dest == 1 )
								*dest = 2;
						    }
						}
					    }
					    dest++;
					}
					break;
				    case 4:
					for( int x = 0; x < width; x++ )
					{
					    int r = *src; src++;
					    int g = *src; src++;
					    int b = *src; src++;
					    int a = *src; src++;
					    *dest = get_color( r, g, b );
					    dest++;
					    *alpha_data = a;
					    alpha_data++;
					}
					break;
				}
			    }
			}
		    }
		    
		    break;
		}

		//Cleanup:
		png_destroy_read_struct( &png_ptr, &info_ptr, NULL );
		if( row_pointers )
		{
		    for( int y = 0; y < height; y++ )
			smem_free( row_pointers[ y ] );
		    smem_free( row_pointers );
		}
		
		smem_free( palette );
	    }
	    break;
#endif
#ifndef PIX_NOGIF
	case SFS_FILE_FMT_GIF:
	    {
		uint8_t* screen_line = 0;
		COLORPTR next_screen = 0; //Next screen (layer 1). Next screen images will be on layer 2.
		GifFileType* gf = DGifOpen( &f, user_gif_read );
		while( gf )
		{
		    rv = pix_vm_new_container( -1, gf->SWidth, gf->SHeight, 32, 0, vm );
		    if( rv < 0 )
		    {
			PIX_VM_LOG( "GIF loading error: can't create a container\n" );
			break;
		    }
		    
		    GifRecordType rec_type = UNDEFINED_RECORD_TYPE;
		    int images_num = 0;
		    int ext_code;
		    GifByteType* extension;
		    bool transp_used = 0;
		    bool gce_transp = 0;
		    int gce_transp_index = 0;
		    int gce_delay = 0;
		    int gce_disposal = 0;
		    int anim_fps = 0;
		    int frame_num = 0;
		    size_t screen_size = gf->SWidth * gf->SHeight;
		    screen_line = (uint8_t*)smem_new( gf->SWidth );
		    next_screen = (COLORPTR)smem_new( screen_size * COLORLEN );
		    if( screen_line == 0 ) break;
		    if( next_screen == 0 ) break;
		    for( size_t i = 0; i < screen_size; i++ ) next_screen[ i ] = 1;
		    while( rec_type != TERMINATE_RECORD_TYPE )
		    {
			if( DGifGetRecordType( gf, &rec_type ) == GIF_ERROR ) break;
			switch( rec_type ) 
			{
			    case IMAGE_DESC_RECORD_TYPE:
				{
				    //printf( "IMG\n" );
				    if( DGifGetImageDesc( gf ) == GIF_ERROR ) break;
				    images_num++;
				    gif_load_create_frame( frame_num, rv, gf, vm );
				    int top = gf->Image.Top; // Image Position relative to Screen.
				    int left = gf->Image.Left;
				    int width = gf->Image.Width;
				    int height = gf->Image.Height;
				    if( gf->Image.Left + gf->Image.Width > gf->SWidth ||
					gf->Image.Top + gf->Image.Height > gf->SHeight ) 
				    {
					PIX_VM_LOG( "GIF loading error: image is not confined to screen dimension.\n" );
					break;
				    }
				    GifColorType* cmap = gif_load_get_colormap( frame_num, rv, gf, vm );
				    COLORPTR frame_pixels = gif_load_get_frame_ptr( frame_num, rv, gf, vm );
				    smem_copy( frame_pixels, next_screen, screen_size * COLORLEN );
				    if( gf->Image.Interlace ) 
				    {
					// Need to perform 4 passes on the images:
					for( int i = 0; i < 4; i++ )
					{
					    for( int j = top + g_gif_interlaced_offset[ i ]; j < top + height; j += g_gif_interlaced_jumps[ i ] )
					    {
						if( DGifGetLine( gf, screen_line, width ) == GIF_ERROR ) break;
						COLORPTR dest = frame_pixels + j * gf->SWidth + left;
						for( int p = 0; p < width; p++ )
						{
						    uint8_t pixel = screen_line[ p ];
						    if( !( gce_transp && pixel == gce_transp_index ) )
						    {
							GifColorType* c = &cmap[ pixel ];
							COLOR col = get_color( c->Red, c->Green, c->Blue );
							if( col == 1 ) col = 2;
							*dest = col;
						    }
						    dest++;
						}
					    }
					}
				    }
				    else
				    {
					COLORPTR dest = frame_pixels + top * gf->SWidth + left;
					for( int y = 0; y < height; y++ ) 
					{
					    if( DGifGetLine( gf, screen_line, width ) == GIF_ERROR ) break;
					    for( int x = 0; x < width; x++ )
					    {
						uint8_t pixel = screen_line[ x ];
						if( !( gce_transp && pixel == gce_transp_index ) )
						{
						    GifColorType* c = &cmap[ pixel ];
						    COLOR col = get_color( c->Red, c->Green, c->Blue );
						    if( col == 1 ) col = 2;
						    *dest = col;
						}
						dest++;
					    }
					    dest += gf->SWidth - width;
					}
				    }
				    switch( gce_disposal )
				    {
					case 0:
					    //No disposal specified:
					case 1:
					    //Do not dispose:
					    smem_copy( next_screen, frame_pixels, screen_size * COLORLEN );
					    break;
					case 2:
					    //Restore to background color:
					    {
						COLORPTR dest = next_screen + top * gf->SWidth + left;
						for( int y = 0; y < height; y++ )
						{
						    for( int x = 0; x < width; x++ )
						    {
							*dest = 1;
							dest++;
						    }
						    dest += gf->SWidth - width;
						}
					    }
					    break;
				    }
				    gif_load_release_frame_ptr( frame_pixels, frame_num, rv, gf, vm );
				    gce_disposal = 0;
				    if( par1 & PIX_LOAD_FIRST_FRAME )
					rec_type = TERMINATE_RECORD_TYPE;
				}
				break;
			    case EXTENSION_RECORD_TYPE:
				if( DGifGetExtension( gf, &ext_code, &extension ) == GIF_ERROR ) break;
				if( ext_code == GRAPHICS_EXT_FUNC_CODE ) 
				{
				    // Graphic Control Extension
				    //printf( "EXT\n" );
				    GifByteType* ptr = extension + 1;
				    if( *ptr & 0x1 ) 
				    {
					gce_transp_index = *( ptr + 3 );
					gce_transp = 1;
					transp_used = 1;
				    }
				    else
				    {
					gce_transp = 0;
				    }
				    
				    gce_delay = *( ptr + 1 ) | ( *( ptr + 2 ) << 8 );
				    if( anim_fps == 0 )
				    {
					if( gce_delay == 0 ) gce_delay = 1;
					anim_fps = 100 / gce_delay;
					if( anim_fps == 0 ) anim_fps = 1;
				    }
				    gce_disposal = ( (*ptr) >> 2 ) & 0x7;
				    if( images_num > 0 )
				    {
					frame_num++;
				    }
				    //printf( "FRAME %d. disposal: %d. transp: %d %d\n", frame_num, gce_disposal, gce_transp, gce_transp_index );
				}
				while( extension != NULL ) 
				{
				    if( DGifGetExtensionNext( gf, &extension ) == GIF_ERROR ) break;
				}
				break;
			    case TERMINATE_RECORD_TYPE:
				break;
			    default:
				break;
			}
		    }
		    
		    if( frame_num > 0 )
		    {
			prop_val.i = anim_fps; pix_vm_set_container_property( rv, "fps", -1, 0, prop_val, vm );
			prop_val.i = 0; pix_vm_set_container_property( rv, "frame", -1, 0, prop_val, vm );
			prop_val.i = -1; pix_vm_set_container_property( rv, "repeat", -1, 0, prop_val, vm );
		    }

		    if( transp_used )
			pix_vm_set_container_key_color( rv, 1, vm );
		    
		    break;
		}
		if( gf ) DGifCloseFile( gf );
		smem_free( screen_line );
		smem_free( next_screen );
	    }
	    break;
#endif
	case SFS_FILE_FMT_OGG:
	case SFS_FILE_FMT_MP3:
	case SFS_FILE_FMT_FLAC:
	case SFS_FILE_FMT_AIFF:
	case SFS_FILE_FMT_WAVE:
	    {
#ifdef PIX_NOOGG
		if( format == SFS_FILE_FMT_OGG ) break;
#endif
#ifdef PIX_NOMP3
		if( format == SFS_FILE_FMT_MP3 ) break;
#endif
#ifdef PIX_NOFLAC
		if( format == SFS_FILE_FMT_FLAC ) break;
#endif
		sfs_sound_decoder_data d = sfs_sound_decoder_data();
		int rv2 = sfs_sound_decoder_init( vm->wm->sd, nullptr, f, format, SFS_SDEC_CONVERT_INT24_TO_FLOAT32 | SFS_SDEC_CONVERT_INT32_TO_FLOAT32 | SFS_SDEC_CONVERT_FLOAT64_TO_FLOAT32, &d );
		if( rv2 )
		{
		    PIX_VM_LOG( "sfs_sound_decoder_init() error %d\n", rv2 );
		    break;
		}
		pix_container_type ctype = PIX_CONTAINER_TYPE_INT8;
		switch( d.sample_format2 )
		{
		    case SFMT_INT8: ctype = PIX_CONTAINER_TYPE_INT8; break;
		    case SFMT_INT16: ctype = PIX_CONTAINER_TYPE_INT16; break;
		    case SFMT_FLOAT32: ctype = PIX_CONTAINER_TYPE_FLOAT32; break;
		    default: break;
		}
		int new_frame_size = d.channels * g_sfs_sample_format_sizes[ d.sample_format2 ];
		rv = pix_vm_new_container( -1, d.len * d.channels, 1, ctype, 0, vm );
		if( rv >= 0 )
		{
		    void* wave_data = pix_vm_get_container_data( rv, vm );
		    prop_val.i = d.rate;
		    pix_vm_set_container_property( rv, "sample_rate", -1, 0, prop_val, vm );
		    prop_val.i = d.channels;
		    pix_vm_set_container_property( rv, "channels", -1, 0, prop_val, vm );
		    sfs_sound_decoder_read2( &d, wave_data, d.len );
		    if( d.loop_type )
            	    {
                	if( d.loop_start < d.len && d.loop_start + d.loop_len <= d.len )
                	{
                    	    prop_val.i = d.loop_start;
                    	    pix_vm_set_container_property( rv, "loop_start", -1, 0, prop_val, vm );
                    	    prop_val.i = d.loop_len;
                    	    pix_vm_set_container_property( rv, "loop_len", -1, 0, prop_val, vm );
                    	    prop_val.i = d.loop_type;
                    	    pix_vm_set_container_property( rv, "loop_type", -1, 0, prop_val, vm );
            	        }
            	    }
		}
		sfs_sound_decoder_deinit( &d );
	    }
	    break;
	case SFS_FILE_FMT_PIXICONTAINER:
	    {
		char chunk_name[ 64 ];
		uint64_t chunk_size;
		
		int colorlen = COLORLEN;
		int intlen = sizeof( PIX_INT );
		int floatlen = sizeof( PIX_FLOAT );

		pix_container_type cont_type = PIX_CONTAINER_TYPE_INT8;
		uint cont_flags = 0;
		PIX_INT cont_xsize = 0;
		PIX_INT cont_ysize = 0;
		void* cont_data = 0;
		COLOR cont_key = 0;
		
		bool eof = 0;
		while( eof == 0 )
		{
		    //Read chunk name:
		    for( int i = 0; i < sizeof( chunk_name ); i++ )
		    {
			int c = sfs_getc( f );
			if( c < 0 ) { chunk_name[ i ] = 0; break; }
			chunk_name[ i ] = c;
			if( c == 0 ) break;
		    }
		    if( eof ) break;
		    chunk_name[ sizeof( chunk_name ) - 1 ] = 0;
		    
		    //Read chunk size:
		    if( sfs_read( &chunk_size, 1, sizeof( chunk_size ), f ) != sizeof( chunk_size ) )
			break;
			
		    if( chunk_name[ 0 ] == 'P' )
		    {
			//Property:
			if( rv )
			{
			    PIX_VAL val;
			    char type = 0;
			    smem_clear( &val, sizeof( val ) );
			    type = sfs_getc( f );
			    if( type < 0 ) break;
			    chunk_size -= 1;
			    if( type == 0 || ( type == 1 && floatlen == sizeof( PIX_FLOAT ) ) )
			    {
				size_t size = chunk_size;
				if( chunk_size > sizeof( val ) ) size = sizeof( val );
				if( sfs_read( &val, 1, size, f ) != size ) break;
				chunk_size -= size;
			    }
			    if( type == 1 && floatlen != sizeof( PIX_FLOAT ) )
			    {
				uint8_t ff[ 16 ];
				if( sfs_read( ff, 1, chunk_size, f ) != chunk_size ) break;
				if( floatlen == 4 ) val.f = ((float*)&ff)[ 0 ];
				if( floatlen == 8 ) val.f = ((double*)&ff)[ 0 ];
				chunk_size = 0;
			    }
			    pix_vm_set_container_property( rv, chunk_name + 1, -1, type, val, vm );
			}
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "colorlen" ) == 0 )
		    {
			colorlen = sfs_getc( f );
			if( colorlen < 0 ) break;
			chunk_size -= 1;
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "intlen" ) == 0 )
		    {
			intlen = sfs_getc( f );
			if( intlen < 0 ) break;
			chunk_size -= 1;
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "floatlen" ) == 0 )
		    {
			floatlen = sfs_getc( f );
			if( floatlen < 0 ) break;
			chunk_size -= 1;
			goto next_pc_chunk;
		    }
			
		    if( smem_strcmp( (const char*)chunk_name, "type" ) == 0 )
		    {
			size_t size = chunk_size;
			if( chunk_size > sizeof( cont_type ) ) size = sizeof( cont_type );
			if( sfs_read( &cont_type, 1, size, f ) != size ) break;
			chunk_size -= size;
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "flags" ) == 0 )
		    {
			size_t size = chunk_size;
			if( chunk_size > sizeof( cont_flags ) ) size = sizeof( cont_flags );
			if( sfs_read( &cont_flags, 1, size, f ) != size ) break;
			chunk_size -= size;
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "xsize" ) == 0 )
		    {
			size_t size = chunk_size;
			if( chunk_size > sizeof( cont_xsize ) ) size = sizeof( cont_xsize );
			if( sfs_read( &cont_xsize, 1, size, f ) != size ) break;
			chunk_size -= size;
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "ysize" ) == 0 )
		    {
			size_t size = chunk_size;
			if( chunk_size > sizeof( cont_ysize ) ) size = sizeof( cont_ysize );
			if( sfs_read( &cont_ysize, 1, size, f ) != size ) break;
			chunk_size -= size;
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "data" ) == 0 )
		    {
			cont_data = smem_new( chunk_size );
			if( cont_data == 0 ) break;
			if( sfs_read( cont_data, 1, chunk_size, f ) != chunk_size ) break;
			chunk_size = 0;
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "key color" ) == 0 )
		    {
			uint8_t rgb[ 3 ];
			if( sfs_read( rgb, 1, 3, f ) != 3 ) break;
			cont_key = get_color( rgb[ 0 ], rgb[ 1 ], rgb[ 2 ] );
			chunk_size -= 3;
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "create" ) == 0 )
		    {
			rv = pix_vm_new_container( -1, cont_xsize, cont_ysize, cont_type, cont_data, vm );
			goto next_pc_chunk;
		    }

		    if( smem_strcmp( (const char*)chunk_name, "hdata" ) == 0 )
		    {
			if( pix_vm_load_container_hdata( rv, f, vm ) != chunk_size ) break;
			chunk_size = 0;
			goto next_pc_chunk;
		    }

next_pc_chunk:

		    if( chunk_size > 0 )
		    {
			//Skip the rest of chunk:
			if( sfs_seek( f, chunk_size, 1 ) ) break;
		    }
		}
	    }
	    break;
	default:
	    break;
    }
    
    if( rv < 0 )
    {
	//Some error or format not recognized:
	format = SFS_FILE_FMT_UNKNOWN;
	sfs_seek( f, 0, 2 );
	size_t size = sfs_tell( f );
	sfs_rewind( f );
	void* data = smem_new( size );
	sfs_read( data, 1, size, f );
	rv = pix_vm_new_container( -1, size, 1, PIX_CONTAINER_TYPE_INT8, data, vm );
    }
    
    if( rv >= 0 )
    {
	prop_val.i = format;
	pix_vm_set_container_property( rv, "file_format", -1, 0, prop_val, vm );
    }
    
    if( filename && f ) sfs_close( f );
    
    return rv;
}

#define GIF_DITHERING \
    if( ( x + y ) & 1 ) \
    { \
	if( y & 1 ) \
	{ \
	    if( ( r & 31 ) >= 10 && r < 256 - 32 ) r += 32; \
    	    if( ( g & 31 ) >= 10 && g < 256 - 32 ) g += 32; \
	    if( ( b & 63 ) >= 20 && b < 256 - 64 ) b += 64; \
	} \
	else \
	{ \
	    if( ( r & 31 ) >= 21 && r < 256 - 32 ) r += 32; \
	    if( ( g & 31 ) >= 21 && g < 256 - 32 ) g += 32; \
	    if( ( b & 63 ) >= 42 && b < 256 - 64 ) b += 64; \
	} \
    }
    
int pix_vm_save_chunk( uint64_t size, const char* name, sfs_file f )
{
    size_t len = smem_strlen( name ) + 1; 
    if( sfs_write( name, 1, len, f ) != len ) return -1;
    if( sfs_write( &size, 1, sizeof( size ), f ) != sizeof( size ) ) return -1;
    return 0;
}

int pix_vm_save( PIX_CID cnum, const char* filename, sfs_file f, int format, int par1, pix_vm* vm )
{
    int rv = -1;
    
    if( filename && f == 0 ) f = sfs_open( filename, "wb" );
    if( f == 0 ) return -1;
    
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* c = vm->c[ cnum ];
	if( c && c->data && c->size )
	{
	    switch( format )
	    {
		case SFS_FILE_FMT_UNKNOWN:
		    {
			sfs_write( c->data, 1, c->size * g_pix_container_type_sizes[ c->type ], f );
			sfs_close( f );
			rv = 0;
		    }
		    break;
#ifndef PIX_NOJPEG
		case SFS_FILE_FMT_JPEG:
		    {
			if( sizeof( COLOR ) != g_pix_container_type_sizes[ c->type ] )
			{
			    PIX_VM_LOG( "JPEG saving: type of the container != PIXEL; can't write.\n" );
			    break;
			}

			simage_desc img = simage_desc();
			sfs_jpeg_enc_params jpg_params = sfs_jpeg_enc_params();
			if( par1 ) 
			{
			    int q = PIX_JPEG_QUALITY( par1 );
			    if( q == 0 ) q = 1;
			    if( q > 100 ) q = 100;
			    jpg_params.quality = q;
			    if( ( par1 & PIX_JPEG_SUBSAMPLING_MASK ) == PIX_JPEG_H1V1 ) jpg_params.subsampling = JE_H1V1;
			    if( ( par1 & PIX_JPEG_SUBSAMPLING_MASK ) == PIX_JPEG_H2V1 ) jpg_params.subsampling = JE_H2V1;
			    if( ( par1 & PIX_JPEG_SUBSAMPLING_MASK ) == PIX_JPEG_H2V2 ) jpg_params.subsampling = JE_H2V2;
			    if( par1 & PIX_JPEG_TWOPASS )
				jpg_params.two_pass_flag = 1;
			}
			img.data = pix_vm_get_container_data( cnum, vm );
			img.format = PFMT_SUNDOG_COLOR;
			img.width = c->xsize;
			img.height = c->ysize;
			sfs_save_jpeg( vm->wm->sd, nullptr, f, &img, &jpg_params );

			rv = 0;
		    }
		    break;
#endif
#ifndef PIX_NOPNG
		case SFS_FILE_FMT_PNG:
		    {
			if( sizeof( COLOR ) != g_pix_container_type_sizes[ c->type ] )
			{
			    PIX_VM_LOG( "PNG saving: type of the container != PIXEL; can't write.\n" );
			    break;
			}
			
			uint8_t* alpha = (uint8_t*)pix_vm_get_container_alpha_data( cnum, vm );
			COLORPTR img = (COLORPTR)pix_vm_get_container_data( cnum, vm );
			
			png_structp png_ptr;
			png_infop info_ptr;
			png_bytep* row_pointers = 0;

			//Create rows:
			row_pointers = (png_bytep*)smem_new( sizeof( png_bytep* ) * c->ysize );
			for( int y = 0; y < c->ysize; y++ )
			{
			    if( alpha )
				row_pointers[ y ] = (png_bytep)smem_new( 4 * c->xsize );
			    else
				row_pointers[ y ] = (png_bytep)smem_new( 3 * c->xsize );
			    COLORPTR src = img + y * c->xsize;
			    uint8_t* src_alpha = 0;
			    if( alpha ) src_alpha = alpha + y * c->xsize;
			    uint8_t* dest = row_pointers[ y ];
			    for( int x = 0; x < c->xsize; x++ )
			    {
				COLOR pixel = *src; src++;
				*dest = red( pixel ); dest++;
				*dest = green( pixel ); dest++;
				*dest = blue( pixel ); dest++;
				if( alpha ) 
				{
				    *dest = *src_alpha; 
				    dest++;
				    src_alpha++;
				}
			    }
			}
			
			while( 1 )
			{
			    //Initialize stuff:
			    png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, NULL, NULL, NULL );
			    if( !png_ptr )
			    {
				PIX_VM_LOG( "PNG saving: png_create_write_struct failed\n" );
				break;
			    }
			    info_ptr = png_create_info_struct( png_ptr );
			    if( !info_ptr )
			    {
				PIX_VM_LOG( "PNG saving: png_create_info_struct failed\n" );
				break;
			    }
			    
			    //Write header:
			    if( setjmp( png_jmpbuf( png_ptr ) ) )
			    {
				PIX_VM_LOG( "PNG saving: error during writing header\n" );
				break;
			    }
			    if( alpha )
			    {
				png_set_IHDR( png_ptr, info_ptr, c->xsize, c->ysize, 8, 
						PNG_COLOR_TYPE_RGBA, PNG_INTERLACE_NONE,
						PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );
			    }
			    else
			    {
				png_set_IHDR( png_ptr, info_ptr, c->xsize, c->ysize, 8, 
						PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
						PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE );
			    }
			    if( pix_vm_get_container_flags( cnum, vm ) & PIX_CONTAINER_FLAG_USES_KEY )
			    {
				COLOR transp_color = pix_vm_get_container_key_color( cnum, vm );
				png_color_16 trans_values;
				trans_values.red = red( transp_color );
				trans_values.green = green( transp_color );
				trans_values.blue = blue( transp_color );
				png_set_tRNS( png_ptr, info_ptr, 0, 0, &trans_values );
			    }
			    png_set_write_fn( png_ptr, &f, user_png_write, user_png_flush );
			    png_write_info( png_ptr, info_ptr );
			    
			    //Write bytes:
			    if( setjmp( png_jmpbuf( png_ptr ) ) )
			    {
				PIX_VM_LOG( "PNG saving: error during writing bytes\n" );
				break;
			    }
			    png_write_image( png_ptr, row_pointers );
			    
			    //End write:
			    if( setjmp( png_jmpbuf( png_ptr ) ) )
			    {
				PIX_VM_LOG( "PNG saving: error during end of write\n" );
				break;
			    }
			    png_write_end( png_ptr, NULL );
			    
			    rv = 0;
			    
			    break;
			}

			png_destroy_write_struct( &png_ptr, &info_ptr );
			if( row_pointers )
			{
			    for( int y = 0; y < c->ysize; y++ )
				smem_free( row_pointers[ y ] );
			    smem_free( row_pointers );
			}
		    }
		    break;
#endif
#ifndef PIX_NOGIF
		case SFS_FILE_FMT_GIF:
		    {
			if( sizeof( COLOR ) != g_pix_container_type_sizes[ c->type ] )
			{
			    PIX_VM_LOG( "GIF saving: type of the container != PIXEL; can't write.\n" );
			    break;
			}
			
			GifColorType* pal = 0;
			uint8_t* screen_line = 0;
			GifFileType* gf = EGifOpen( &f, user_gif_write );
			while( gf )
			{
			    screen_line = (uint8_t*)smem_new( c->xsize );
			    if( screen_line == 0 ) break;
			    
			    //Create palette:
			    pal = (GifColorType*)smem_new( sizeof( GifColorType ) * 256 );
			    int pal_transp_index = 1;
			    if( pal == 0 ) break;
			    if( ( par1 & PIX_GIF_PALETTE_MASK ) == 0 )
			    {
				//256 color palette:
				for( int i = 0; i < 256; i++ )
				{
				    GifColorType* c = &pal[ i ];
				    c->Red = i & 7;
				    if( c->Red == 7 ) c->Red = 255; else c->Red <<= 5;
				    c->Green = ( i >> 3 ) & 7;
				    if( c->Green == 7 ) c->Green = 255; else c->Green <<= 5;
				    c->Blue = ( i >> 6 ) & 3;
				    if( c->Blue == 3 ) c->Blue = 255; else c->Blue <<= 6;
				}
			    }
			    if( ( par1 & PIX_GIF_PALETTE_MASK ) == PIX_GIF_GRAYSCALE )
			    {
				//256 grayscale palette:
				for( int i = 0; i < 256; i++ )
				{
				    GifColorType* c = &pal[ i ];
				    c->Red = i;
				    c->Green = i;
				    c->Blue = i;
				}
			    }
			    ColorMapObject* cmap = MakeMapObject( 256, pal );

			    if( EGifPutScreenDesc( gf, c->xsize, c->ysize, 8, 0, cmap ) == GIF_ERROR ) break;

			    int frame_count = pix_vm_container_hdata_get_frame_count( cnum, vm );
			    if( c->flags & PIX_CONTAINER_FLAG_USES_KEY || frame_count > 0 )
			    {
				//Animation or/and transparency.
				{
				    EGifPutExtensionFirst( gf, APPLICATION_EXT_FUNC_CODE, 11, (const VoidPtr)g_gif_netscape20ext );
				    int rep = pix_vm_get_container_property_i( cnum, "repeat", -1, vm );
				    if( rep < 0 ) rep = 0;
				    uint8_t netscape_ext_subblock[ 3 ];
				    netscape_ext_subblock[ 0 ] = 1;
				    netscape_ext_subblock[ 1 ] = rep & 255;
				    netscape_ext_subblock[ 2 ] = ( rep >> 8 ) & 255;
				    EGifPutExtensionLast( gf, 0, 3, (const VoidPtr)netscape_ext_subblock );
				}
				for( int fnum = 0; fnum < frame_count; fnum++ )
				{
				    pix_container_type type;
				    int xsize, ysize;
				    if( pix_vm_container_hdata_get_frame_size( cnum, fnum, &type, &xsize, &ysize, vm ) ) break;
				    void* frame_buf = smem_new( xsize * ysize * g_pix_container_type_sizes[ type ] );
				    if( frame_buf == 0 ) break;
				    if( pix_vm_container_hdata_unpack_frame_to_buf( cnum, fnum, (COLORPTR)frame_buf, vm ) ) break;
				    {
					//Graphic Control Extension:
					uint8_t gce[ 4 ];
					int fps = 100 / 20;
					pix_sym* fps_prop = pix_vm_get_container_property( cnum, "fps", -1, vm );
					if( fps_prop )
					{
    					    if( fps_prop->type == SYMTYPE_NUM_F )
    					    {
    						if( fps_prop->val.f != 0 )
        					    fps = 100.0f / fps_prop->val.f;
        				    }
    					    else
    					    {
    						if( fps_prop->val.i != 0 )
        					    fps = 100 / fps_prop->val.i;
        				    }
					}
					gce[ 0 ] = 0;
					gce[ 1 ] = fps & 255;
					gce[ 2 ] = ( fps >> 8 ) & 255;
					gce[ 3 ] = 0;
					if( c->flags & PIX_CONTAINER_FLAG_USES_KEY )
					{
					    gce[ 0 ] |= 2 << 2; //Disposal method: Restore to background color
					    gce[ 0 ] |= 1; //Transparent Color Flag
					    gce[ 3 ] = pal_transp_index; //Transparent color index
					}
					EGifPutExtension( gf, GRAPHICS_EXT_FUNC_CODE, sizeof( gce ), (const VoidPtr)gce );
				    }
				    if( EGifPutImageDesc( gf, 0, 0, xsize, ysize, 0, 0 ) == GIF_ERROR ) break;
				    COLORPTR src = (COLORPTR)frame_buf;
				    for( int y = 0; y < ysize; y++ )
				    {
					if( c->flags & PIX_CONTAINER_FLAG_USES_KEY )
					{
					    if( ( par1 & PIX_GIF_PALETTE_MASK ) == 0 )
					    {
						//Full color:
						if( ( par1 & PIX_GIF_DITHER ) == 0 )
						    //No dithering:
						    for( int x = 0; x < xsize; x++ )
						    {
							COLOR pixel = *src;
							uint8_t val;
							if( pixel == c->key )
							    val = pal_transp_index;
							else
							{
							    int r = red( pixel );
							    int g = green( pixel );
							    int b = blue( pixel );
							    val = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
							    if( val == pal_transp_index ) val = 0;
							}
							screen_line[ x ] = val;
							src++;
						    }
						else
						    //Dithering:
						    for( int x = 0; x < xsize; x++ )
						    {
							COLOR pixel = *src;
							uint8_t val;
							if( pixel == c->key )
							    val = pal_transp_index;
							else
							{
							    int r = red( pixel );
							    int g = green( pixel );
							    int b = blue( pixel );
							    GIF_DITHERING;
							    val = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
							    if( val == pal_transp_index ) val = 0;
							}
							screen_line[ x ] = val;
							src++;
						    }
					    }
					    if( ( par1 & PIX_GIF_PALETTE_MASK ) == PIX_GIF_GRAYSCALE )
						//Grayscale:
						for( int x = 0; x < xsize; x++ )
						{
						    COLOR pixel = *src;
						    uint8_t val;
						    if( pixel == c->key )
							val = pal_transp_index;
						    else
						    {
							int g = ( red( pixel ) + green( pixel ) + blue( pixel ) ) / 3;
							val = g;
							if( val == pal_transp_index ) val = 0;
						    }
						    screen_line[ x ] = val;
						    src++;
						}
					}
					else
					{
					    if( ( par1 & PIX_GIF_PALETTE_MASK ) == 0 )
					    {
						//Full color:
						if( ( par1 & PIX_GIF_DITHER ) == 0 )
						    //No dithering:
						    for( int x = 0; x < xsize; x++ )
						    {
							COLOR pixel = *src;
							int r = red( pixel );
							int g = green( pixel );
							int b = blue( pixel );
							screen_line[ x ] = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
							src++;
						    }
						else
						    //Dithering:
						    for( int x = 0; x < xsize; x++ )
						    {
							COLOR pixel = *src;
							int r = red( pixel );
							int g = green( pixel );
							int b = blue( pixel );
							GIF_DITHERING;
							screen_line[ x ] = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
							src++;
						    }
					    }
					    if( ( par1 & PIX_GIF_PALETTE_MASK ) == PIX_GIF_GRAYSCALE )
						//Grayscale:
						for( int x = 0; x < xsize; x++ )
						{
						    COLOR pixel = *src;
						    int g = ( red( pixel ) + green( pixel ) + blue( pixel ) ) / 3;
						    screen_line[ x ] = (uint8_t)g;
						    src++;
						}
					}
					EGifPutLine( gf, screen_line, xsize );    
				    }
				    smem_free( frame_buf );
				}
			    }
			    else
			    {
				//Just an image. No animation.
				if( EGifPutImageDesc( gf, 0, 0, c->xsize, c->ysize, 0, 0 ) == GIF_ERROR ) break;
				COLORPTR src = (COLORPTR)c->data;
				for( int y = 0; y < c->ysize; y++ )
				{
				    if( ( par1 & PIX_GIF_PALETTE_MASK ) == 0 )
				    {
					if( ( par1 & PIX_GIF_DITHER ) == 0 )
					    for( int x = 0; x < c->xsize; x++ )
					    {
						COLOR pixel = *src;
						int r = red( pixel );
						int g = green( pixel );
						int b = blue( pixel );
						screen_line[ x ] = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
						src++;
					    }
					else
					    for( int x = 0; x < c->xsize; x++ )
					    {
						COLOR pixel = *src;
						int r = red( pixel );
						int g = green( pixel );
						int b = blue( pixel );
						GIF_DITHERING;
						screen_line[ x ] = ( r >> 5 ) | ( ( g >> 5 ) << 3 ) | ( ( b >> 6 ) << 6 );
						src++;
					    }
				    }
				    if( ( par1 & PIX_GIF_PALETTE_MASK ) == PIX_GIF_GRAYSCALE )
					for( int x = 0; x < c->xsize; x++ )
					{
					    COLOR pixel = *src;
					    int g = ( red( pixel ) + green( pixel ) + blue( pixel ) ) / 3;
					    screen_line[ x ] = (uint8_t)g;
					    src++;
					}
				    EGifPutLine( gf, screen_line, c->xsize );
				}
			    }
			    
			    FreeMapObject( cmap );
			    
			    rv = 0;

			    break;
			}
			if( gf ) EGifCloseFile( gf );
			smem_free( pal );
			smem_free( screen_line );
		    }
		    break;
#endif
		case SFS_FILE_FMT_FLAC:
		case SFS_FILE_FMT_OGG:
		case SFS_FILE_FMT_WAVE:
#ifdef PIX_NOFLAC
		    if( format == SFS_FILE_FMT_FLAC ) break;
#endif
#ifdef PIX_NOOGG
		    if( format == SFS_FILE_FMT_OGG ) break;
#endif
		    {
			sfs_sound_encoder_data e = sfs_sound_encoder_data();

			e.sample_format = SFMT_INT8;
			switch( c->type )
			{
			    case PIX_CONTAINER_TYPE_INT8: e.sample_format = SFMT_INT8; break;
			    case PIX_CONTAINER_TYPE_INT16: e.sample_format = SFMT_INT16; break;
			    case PIX_CONTAINER_TYPE_FLOAT32: e.sample_format = SFMT_FLOAT32; break;
			    default: break;
			}
			e.rate = pix_vm_get_container_property_i( cnum, "sample_rate", -1, vm );
			e.channels = pix_vm_get_container_property_i( cnum, "channels", -1, vm );
			e.loop_start = pix_vm_get_container_property_i( cnum, "loop_start", -1, vm );
			e.loop_len = pix_vm_get_container_property_i( cnum, "loop_len", -1, vm );
			e.loop_type = pix_vm_get_container_property_i( cnum, "loop_type", -1, vm );
			if( e.rate == 0 ) e.rate = 44100;
			if( e.channels == 0 ) e.channels = 1;
			size_t len = c->size / e.channels; //number of frames

			if( format == SFS_FILE_FMT_FLAC )
			{
			    if( par1 )
			    {
				e.compression_level = PIX_FLAC_QUALITY( par1 );
			    }
			}
			if( format == SFS_FILE_FMT_OGG )
			{
			    if( par1 )
			    {
				e.compression_level = PIX_OGG_QUALITY( par1 );
			    }
			}

			int rv2 = sfs_sound_encoder_init(
        		    vm->wm->sd,
			    nullptr, f,
        		    (sfs_file_fmt)format,
        		    e.sample_format,
        		    e.rate, //Hz
        		    e.channels,
        		    len, //number of frames
        		    0,
        		    &e );
		        if( rv2 )
		        {
		            slog( "sfs_sound_encoder_init() error %d\n", rv2 );
		            break;
		        }
		        sfs_sound_encoder_write( &e, c->data, len );
		        sfs_sound_encoder_deinit( &e );

			rv = 0;
		    }
		    break;
		case SFS_FILE_FMT_PIXICONTAINER:
		    {
                        while( 1 )
                        {
                    	    const char* str;
                    	    size_t len;
                    	    uint64_t v;
                    	    
                    	    if( pix_vm_save_chunk( 0, "pixiCONT", f ) ) break;

                    	    if( pix_vm_save_chunk( 1, "colorlen", f ) ) break;
                	    if( sfs_putc( COLORLEN, f ) == EOF ) break;

                    	    if( pix_vm_save_chunk( 1, "intlen", f ) ) break;
                	    if( sfs_putc( sizeof( PIX_INT ), f ) == EOF ) break;

                    	    if( pix_vm_save_chunk( 1, "floatlen", f ) ) break;
                	    if( sfs_putc( sizeof( PIX_FLOAT ), f ) == EOF ) break;

                    	    if( pix_vm_save_chunk( 1, "type", f ) ) break;
                	    sfs_putc( c->type, f );

                    	    if( pix_vm_save_chunk( sizeof( c->flags ), "flags", f ) ) break;
                	    if( sfs_write( &c->flags, 1, sizeof( c->flags ), f ) != sizeof( c->flags ) ) break;

                    	    if( pix_vm_save_chunk( sizeof( c->xsize ), "xsize", f ) ) break;
                	    if( sfs_write( &c->xsize, 1, sizeof( c->xsize ), f ) != sizeof( c->xsize ) ) break;

                    	    if( pix_vm_save_chunk( sizeof( c->ysize ), "ysize", f ) ) break;
                	    if( sfs_write( &c->ysize, 1, sizeof( c->ysize ), f ) != sizeof( c->ysize ) ) break;

			    if( c->data )
			    {
				size_t size = c->size * g_pix_container_type_sizes[ c->type ];
                    		if( pix_vm_save_chunk( size, "data", f ) ) break;
                		if( sfs_write( c->data, 1, size, f ) != size ) break;
                	    }

                    	    if( pix_vm_save_chunk( 3, "key color", f ) ) break;
                	    if( sfs_putc( red( c->key ), f ) == EOF ) break;
                	    if( sfs_putc( green( c->key ), f ) == EOF ) break;
                	    if( sfs_putc( blue( c->key ), f ) == EOF ) break;

                    	    if( pix_vm_save_chunk( 0, "create", f ) ) break;
                	    
                	    if( c->opt_data )
                	    {
                		pix_sym* props = pix_symtab_get_list( &c->opt_data->props );
                		if( props )
                		{
                		    //Save properties:
                		    bool err = 0;
                		    size_t size = smem_get_size( props ) / sizeof( pix_sym );
                		    for( size_t p = 0; p < size; p++ )
                		    {
                			pix_sym* sym = &props[ p ];
                			if( sfs_putc( 'P', f ) == EOF ) { err = 1; break; }
                			if( pix_vm_save_chunk( 1 + sizeof( PIX_VAL ), sym->name, f ) ) { err = 1; break; }
                			int type = 0;
                			if( sym->type == SYMTYPE_NUM_F ) type = 1;
                			if( sfs_putc( type, f ) == EOF ) { err = 1; break; }
                			if( type == 0 )
                			    for( int i = sizeof( PIX_INT ); i < sizeof( PIX_VAL ); i++ ) ((uint8_t*)&sym->val)[ i ] = 0;
                			else
                			    for( int i = sizeof( PIX_FLOAT ); i < sizeof( PIX_VAL ); i++ ) ((uint8_t*)&sym->val)[ i ] = 0;
                			if( sfs_write( &sym->val, 1, sizeof( PIX_VAL ), f ) != sizeof( PIX_VAL ) ) { err = 1; break; }
                		    }
                		    smem_free( props );
                		    if( err ) break;
                		}
                		
                		size_t hdata_size = pix_vm_get_container_hdata_size( cnum, vm );
                		if( hdata_size )
                		{
                		    //Save hidden data:
                		    if( pix_vm_save_chunk( hdata_size, "hdata", f ) ) break;
                		    if( pix_vm_save_container_hdata( cnum, f, vm ) != hdata_size ) break;
                		}
                	    }
                        
                    	    rv = 0;
                    	    break;
                        }
		    }
		default:
		    break;
	    }
	}
    }
    
    if( filename && f ) 
    {
	sfs_close( f );
	if( rv != 0 ) sfs_remove_file( filename );
    }
    
    return rv;
}
