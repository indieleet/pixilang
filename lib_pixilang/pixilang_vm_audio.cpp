/*
    pixilang_vm_audio.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"
#ifndef PIX_NOSUNVOX
    #define SVH_INLINES
    #include "sunvox_engine_helper.h"
    #include "psynth/psynths_sampler.h"
    #include "psynth/psynths_metamodule.h"
    #include "psynth/psynths_vorbis_player.h"
#endif

#include <errno.h>

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) slog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif

static void pix_vm_fill_input_buffer( sundog_sound* ss, sundog_sound_slot* slot, pix_vm* vm, int chnum, int frames )
{
    int src_offset = 0;
    int src_step = ( slot->frames << 15 ) / frames;
    switch( vm->audio_format )
    {
        default: break;
	case PIX_CONTAINER_TYPE_INT8:
	    {
		int8_t* dest_ptr = (int8_t*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    int16_t* src_ptr = (int16_t*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] >> 8;
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				float v = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] * 128;
				if( v > 127 ) v = 127;
				if( v < -127 ) v = -127;
				dest_ptr[ i ] = v;
				src_offset += src_step;
			    }
			}
			break;
		    default:
			break;
		}
	    }
	    break;
	case PIX_CONTAINER_TYPE_INT16:
	    {
		int16_t* dest_ptr = (int16_t*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    int16_t* src_ptr = (int16_t*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ];
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				float v = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] * 32768;
				if( v > 32767 ) v = 32767;
				if( v < -32767 ) v = -32767;
				dest_ptr[ i ] = v;
				src_offset += src_step;
			    }
			}
			break;
		    default:
			break;
		}
	    }
	    break;
	case PIX_CONTAINER_TYPE_INT32:
	    {
		int* dest_ptr = (int*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    int16_t* src_ptr = (int16_t*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ];
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				float v = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] * 32768;
				if( v > 32767 ) v = 32767;
				if( v < -32767 ) v = -32767;
				dest_ptr[ i ] = v;
				src_offset += src_step;
			    }
			}
			break;
		    default:
			break;
		}
	    }
	    break;
	case PIX_CONTAINER_TYPE_FLOAT32:
	    {
		float* dest_ptr = (float*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    int16_t* src_ptr = (int16_t*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = (float)src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] / 32768;
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ];
				src_offset += src_step;
			    }
			}
			break;
		    default:
			break;
		}
	    }
	    break;
#ifdef PIX_FLOAT64_ENABLED
	case PIX_CONTAINER_TYPE_FLOAT64:
	    {
		double* dest_ptr = (double*)vm->audio_input_buffers[ chnum ];
		switch( ss->in_type )
		{
		    case sound_buffer_int16:
			{
			    int16_t* src_ptr = (int16_t*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = (double)src_ptr[ ( src_offset >> 15 ) * ss->in_channels ] / 32768;
				src_offset += src_step;
			    }
			}
			break;
		    case sound_buffer_float32:
			{
			    float* src_ptr = (float*)slot->in_buffer;
			    src_ptr += chnum;
			    for( int i = 0; i < frames; i++ )
			    {
				dest_ptr[ i ] = src_ptr[ ( src_offset >> 15 ) * ss->in_channels ];
				src_offset += src_step;
			    }
			}
			break;
		    default:
			break;
		}
	    }
	    break;
#endif
    }
}

static int pix_vm_render_piece_of_sound( sundog_sound* ss, int slot_num )
{
    int handled = 0;

    if( ss )
    {
	sundog_sound_slot* slot = &ss->slots[ slot_num ];
	int frames = slot->frames;

	pix_vm* vm = (pix_vm*)slot->user_data;
	if( vm->audio_callback < 0 ) return 0;

	uint freq_ratio = 1 << PIX_AUDIO_PTR_PREC;
	if( vm->audio_freq != ss->freq )
	    freq_ratio = ( (uint64_t)vm->audio_freq << PIX_AUDIO_PTR_PREC ) / ss->freq;

	vm->audio_src_ptr += frames * freq_ratio;
	uint interpolation_frames = 1;
	uint src_frames = ( ( ( vm->audio_src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 + interpolation_frames ) - vm->audio_src_rendered ) & ((1<<(32-PIX_AUDIO_PTR_PREC))-1);
	vm->audio_src_rendered += src_frames;

	if( src_frames ) 
	{
	    //Need a new piece of sound:
	    uint src_buffer_offset;
	    if( vm->audio_src_buffers[ 0 ] == 0 )
	    {
		//Buffers are empty. Create it:
		for( int i = 0; i < vm->audio_channels; i++ )
		{
		    smem_free( vm->audio_src_buffers[ i ] );
		    vm->audio_src_buffers[ i ] = smem_new( g_pix_container_type_sizes[ vm->audio_format ] * src_frames );
		}
		vm->audio_src_buffer_size = src_frames;
		vm->audio_src_buffer_ptr = 0;
		src_buffer_offset = 0;
	    }
	    else
	    {
		//Buffer already has some data. We need to save last 16 frames from it - for interpolation:
		if( vm->audio_src_buffer_size > 16 )
		{
		    for( int i = 0; i < vm->audio_channels; i++ )
			smem_copy( vm->audio_src_buffers[ i ], (char*)vm->audio_src_buffers[ i ] + ( vm->audio_src_buffer_size - 16 ) * g_pix_container_type_sizes[ vm->audio_format ], 16 * g_pix_container_type_sizes[ vm->audio_format ] );
		    vm->audio_src_buffer_ptr -= ( vm->audio_src_buffer_size - 16 ) << PIX_AUDIO_PTR_PREC;
		    vm->audio_src_buffer_size = 16;
		}
		src_buffer_offset = vm->audio_src_buffer_size;
		vm->audio_src_buffer_size += src_frames;
		size_t new_buffer_bytes = vm->audio_src_buffer_size * g_pix_container_type_sizes[ vm->audio_format ];
		if( new_buffer_bytes > smem_get_size( vm->audio_src_buffers[ 0 ] ) )
		    for( int i = 0; i < vm->audio_channels; i++ )
			vm->audio_src_buffers[ i ] = smem_resize( vm->audio_src_buffers[ i ], new_buffer_bytes );
	    }
	    //Render a new piece of sound:
	    {
		//Output:
		pix_vm_container* c = vm->c[ vm->audio_channels_cont ];
		int* cdata = (int*)c->data;
		for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
		{
		    if( i < vm->audio_channels )
			cdata[ i ] = vm->audio_buffers_conts[ i ];
		    else
			cdata[ i ] = -1;
		}
		for( int i = 0; i < vm->audio_channels; i++ )
		{
		    c = vm->c[ vm->audio_buffers_conts[ i ] ];
		    c->type = vm->audio_format;
		    c->data = (void*)( (char*)vm->audio_src_buffers[ i ] + src_buffer_offset * g_pix_container_type_sizes[ vm->audio_format ] );
		    c->xsize = src_frames;
		    c->size = src_frames;
		}

		//Input:
		if( vm->audio_input_enabled && slot->in_buffer )
		{
		    c = vm->c[ vm->audio_input_channels_cont ];
		    cdata = (int*)c->data;
		    for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
		    {
			if( i < vm->audio_channels )
			    cdata[ i ] = vm->audio_input_buffers_conts[ i ];
			else
			    cdata[ i ] = -1;
		    }
		    for( int i = 0; i < vm->audio_channels; i++ )
		    {
			if( smem_get_size( vm->audio_input_buffers[ i ] ) < src_frames * g_pix_container_type_sizes[ vm->audio_format ] )
			    vm->audio_input_buffers[ i ] = smem_resize( vm->audio_input_buffers[ i ], src_frames * g_pix_container_type_sizes[ vm->audio_format ] );
			c = vm->c[ vm->audio_input_buffers_conts[ i ] ];
			c->type = vm->audio_format;
			if( i < ss->in_channels )
			{
			    if( smem_get_size( vm->audio_input_buffers[ i ] ) < src_frames * g_pix_container_type_sizes[ vm->audio_format ] )
				vm->audio_input_buffers[ i ] = smem_resize( vm->audio_input_buffers[ i ], src_frames * g_pix_container_type_sizes[ vm->audio_format ] );
			    c->data = vm->audio_input_buffers[ i ];
			    pix_vm_fill_input_buffer( ss, slot, vm, i, src_frames );
			}
			else
			{
			    c->data = vm->audio_input_buffers[ 0 ];
			}
			c->xsize = src_frames;
			c->size = src_frames;
		    }
		}
	    }
	    pix_vm_function fun;
	    pix_stack_val pp[ 7 ];
	    fun.p = pp;
	    fun.addr = vm->audio_callback;
	    fun.p[ 0 ].i = 0;
	    fun.p[ 0 ].t = 0;
	    fun.p[ 1 ] = vm->audio_userdata;
	    fun.p[ 2 ].i = vm->audio_channels_cont;
	    fun.p[ 2 ].t = 0;
	    fun.p[ 3 ].i = src_frames;
	    fun.p[ 3 ].t = 0;
	    fun.p[ 4 ].i = slot->time;
	    fun.p[ 4 ].t = 0;
	    if( vm->audio_input_enabled && slot->in_buffer )
		fun.p[ 5 ].i = vm->audio_input_channels_cont;
	    else
		fun.p[ 5 ].i = -1;
	    fun.p[ 5 ].t = 0;
	    fun.p[ 6 ].i = ( ss->out_latency * freq_ratio ) >> PIX_AUDIO_PTR_PREC;
	    fun.p[ 6 ].t = 0;
	    fun.p_num = 7;
	    pix_vm_run( PIX_VM_THREADS - 1, 0, &fun, PIX_VM_CALL_FUNCTION, vm );
	    pix_stack_val retval;
	    pix_vm_get_thread_retval( PIX_VM_THREADS - 1, vm, &retval );
	    if( retval.t == 0 )
		handled = retval.i;
	    else
		handled = retval.f;
	    for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
	    {
	        pix_vm_container* c = vm->c[ vm->audio_buffers_conts[ i ] ];
	        c->data = NULL;
	        c->xsize = 0;
	        c->size = 0;
	    }
	}

	//Final interpolation:
	if( handled )
	{
	    uint src_ptr = vm->audio_src_buffer_ptr;
	    if( ss->out_type == sound_buffer_int16 )
	    {
		int16_t* dest = (int16_t*)slot->buffer;
		switch( vm->audio_format )
		{
		    case PIX_CONTAINER_TYPE_INT8:
			{
			    int8_t* src1 = (int8_t*)vm->audio_src_buffers[ 0 ];
			    int8_t* src2 = (int8_t*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v1 = ( (int)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    int v2 = ( (int)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (int16_t)v1 << 8;
				    dest[ i * 2 + 1 ] = (int16_t)v2 << 8;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = (int16_t)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] << 8;
				    dest[ i * 2 + 1 ] = (int16_t)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] << 8;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT16:
			{
			    int16_t* src1 = (int16_t*)vm->audio_src_buffers[ 0 ];
			    int16_t* src2 = (int16_t*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v1 = ( (int)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    int v2 = ( (int)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (int16_t)v1;
				    dest[ i * 2 + 1 ] = (int16_t)v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    dest[ i * 2 + 1 ] = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT32:
			{
			    int32_t* src1 = (int32_t*)vm->audio_src_buffers[ 0 ];
			    int32_t* src2 = (int32_t*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v_1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    if( v_1 > 32767 ) v_1 = 32767;
				    if( v_1 < -32767 ) v_1 = -32767;
				    int v_2 = src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ];
				    if( v_2 > 32767 ) v_2 = 32767;
				    if( v_2 < -32767 ) v_2 = -32767;
				    int v1 = ( v_1 * c2 + v_2 * c1 ) >> PIX_AUDIO_PTR_PREC;
				    v_1 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    if( v_1 > 32767 ) v_1 = 32767;
				    if( v_1 < -32767 ) v_1 = -32767;
				    v_2 = src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ];
				    if( v_2 > 32767 ) v_2 = 32767;
				    if( v_2 < -32767 ) v_2 = -32767;
				    int v2 = ( v_1 * c2 + v_2 * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (int16_t)v1;
				    dest[ i * 2 + 1 ] = (int16_t)v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int v = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 0 ] = (int16_t)v;
				    v = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 1 ] = (int16_t)v;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_FLOAT32:
			{
			    float* src1 = (float*)vm->audio_src_buffers[ 0 ];
			    float* src2 = (float*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    float c1 = (float)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (float)( 1 << PIX_AUDIO_PTR_PREC );
				    float c2 = 1 - c1;
				    float v1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    float v2 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    v1 *= 32768.0;
				    v2 *= 32768.0;
				    if( v1 > 32767 ) v1 = 32767;
				    if( v1 < -32767 ) v1 = -32767;
				    if( v2 > 32767 ) v2 = 32767;
				    if( v2 < -32767 ) v2 = -32767;
				    dest[ i * 2 + 0 ] = (int16_t)v1;
				    dest[ i * 2 + 1 ] = (int16_t)v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int v = (int)( src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * 32767.0F );
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 0 ] = (int16_t)v;
				    v = (int)( src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * 32767.0F );
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 1 ] = (int16_t)v;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64:
			{
			    double* src1 = (double*)vm->audio_src_buffers[ 0 ];
			    double* src2 = (double*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    double c1 = (double)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (double)( 1 << PIX_AUDIO_PTR_PREC );
				    double c2 = 1 - c1;
				    double v1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    double v2 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    v1 *= 32768.0;
				    v2 *= 32768.0;
				    if( v1 > 32767 ) v1 = 32767;
				    if( v1 < -32767 ) v1 = -32767;
				    if( v2 > 32767 ) v2 = 32767;
				    if( v2 < -32767 ) v2 = -32767;
				    dest[ i * 2 + 0 ] = (int16_t)v1;
				    dest[ i * 2 + 1 ] = (int16_t)v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int v = (int)( src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * 32767.0 );
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 0 ] = (int16_t)v;
				    v = (int)( src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * 32767.0 );
				    if( v > 32767 ) v = 32767;
				    if( v < -32767 ) v = -32767;
				    dest[ i * 2 + 1 ] = (int16_t)v;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
#endif
		    default:
			break;
		}
	    }
	    if( ss->out_type == sound_buffer_float32 )
	    {
		float* dest = (float*)slot->buffer;
		switch( vm->audio_format )
		{
		    case PIX_CONTAINER_TYPE_INT8:
			{
			    int8_t* src1 = (int8_t*)vm->audio_src_buffers[ 0 ];
			    int8_t* src2 = (int8_t*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v1 = ( (int)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    int v2 = ( (int)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (float)v1 / 128.0F;
				    dest[ i * 2 + 1 ] = (float)v2 / 128.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = (float)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 128.0F;
				    dest[ i * 2 + 1 ] = (float)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 128.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT16:
			{
			    int16_t* src1 = (int16_t*)vm->audio_src_buffers[ 0 ];
			    int16_t* src2 = (int16_t*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    int c1 = src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 );
				    int c2 = ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1;
				    int v1 = ( (int)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    int v2 = ( (int)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (int)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1 ) >> PIX_AUDIO_PTR_PREC;
				    dest[ i * 2 + 0 ] = (float)v1 / 32768.0F;
				    dest[ i * 2 + 1 ] = (float)v2 / 32768.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = (float)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 32768.0F;
				    dest[ i * 2 + 1 ] = (float)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 32768.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_INT32:
			{
			    int32_t* src1 = (int32_t*)vm->audio_src_buffers[ 0 ];
			    int32_t* src2 = (int32_t*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    float c1 = (float)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (float)( 1 << PIX_AUDIO_PTR_PREC );
				    float c2 = (float)( ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) - c1 ) / (float)( 1 << PIX_AUDIO_PTR_PREC );
				    float v1 = (float)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (float)src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    float v2 = (float)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + (float)src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    dest[ i * 2 + 0 ] = v1 / 32768.0F;
				    dest[ i * 2 + 1 ] = v2 / 32768.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = (float)src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 32768.0F;
				    dest[ i * 2 + 1 ] = (float)src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] / 32768.0F;
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
		    case PIX_CONTAINER_TYPE_FLOAT32:
			{
			    float* src1 = (float*)vm->audio_src_buffers[ 0 ];
			    float* src2 = (float*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    float c1 = (float)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (float)( 1 << PIX_AUDIO_PTR_PREC );
				    float c2 = 1 - c1;
				    float v1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    float v2 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    dest[ i * 2 + 0 ] = v1;
				    dest[ i * 2 + 1 ] = v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    dest[ i * 2 + 1 ] = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64:
			{
			    double* src1 = (double*)vm->audio_src_buffers[ 0 ];
			    double* src2 = (double*)vm->audio_src_buffers[ 1 ];
			    if( src2 == 0 ) src2 = src1;
			    if( vm->audio_flags & PIX_AUDIO_FLAG_INTERP2 )
			    {
				for( int i = 0; i < frames; i++ )
				{
				    double c1 = (double)( src_ptr & ( ( 1 << PIX_AUDIO_PTR_PREC ) - 1 ) ) / (double)( 1 << PIX_AUDIO_PTR_PREC );
				    double c2 = 1 - c1;
				    double v1 = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src1[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    double v2 = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ] * c2 + src2[ ( src_ptr >> PIX_AUDIO_PTR_PREC ) + 1 ] * c1;
				    dest[ i * 2 + 0 ] = v1;
				    dest[ i * 2 + 1 ] = v2;
				    src_ptr += freq_ratio;
				}
			    }
			    else
			    {
				for( int i = 0; i < frames; i++ )
				{
				    dest[ i * 2 + 0 ] = src1[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    dest[ i * 2 + 1 ] = src2[ src_ptr >> PIX_AUDIO_PTR_PREC ];
				    src_ptr += freq_ratio;
				}
			    }
			    vm->audio_src_buffer_ptr = src_ptr;
			}
			break;
#endif
		    default:
			break;
		}
	    }
	}
	else
	{
	    uint src_ptr = vm->audio_src_buffer_ptr;
	    src_ptr += freq_ratio * frames;
	    vm->audio_src_buffer_ptr = src_ptr;
	}
    }

    return handled;
}

static int pix_vm_audio_init( pix_vm* vm ) //Open SunDog sound stream
{
    int rv = -1;
    while( 1 )
    {
	if( vm->audio )
	{
	    //Already open:
	    rv = 0;
	    break;
	}
	vm->audio = (sundog_sound*)smem_new( sizeof( sundog_sound ) ); 
	smem_zero( vm->audio );
	if( sundog_sound_init( vm->audio, vm->wm->sd, sound_buffer_default, -1, -1, 0 ) )
	{
	    sconfig_remove_key( APP_CFG_SND_DRIVER, 0 );
    	    sconfig_remove_key( APP_CFG_SND_DEV, 0 );
    	    sconfig_remove_key( APP_CFG_SND_DEV_IN, 0 );
    	    sconfig_remove_key( APP_CFG_SND_BUF, 0 );
    	    sconfig_remove_key( APP_CFG_SND_FREQ, 0 );
    	    sconfig_save( 0 );
#ifdef OS_IOS
	    slog_show_error_report( vm->wm->sd );
#endif
	    rv = -2;
	    break;
	}
	rv = 0;
	break;
    }
    if( rv == 0 ) vm->audio_retain_count++;
    return rv;
}

static int pix_vm_audio_deinit( pix_vm* vm ) //Close SunDog sound stream
{
    bool deinit = false;
    if( vm->audio_retain_count > 0 )
    {
	vm->audio_retain_count--;
	if( vm->audio_retain_count == 0 ) deinit = true;
    }
    if( deinit && vm->audio && vm->audio_external == false )
    {
	sundog_sound_deinit( vm->audio );
	smem_free( vm->audio );
	vm->audio = 0;
    }
    return 0;
}

int pix_vm_set_audio_callback( PIX_ADDR callback, pix_stack_val userdata, uint freq, pix_container_type format, int channels, uint flags, pix_vm* vm )
{
    bool set_slot = false;
    bool remove_slot = false;

    if( callback == -1 && vm->audio_callback != -1 )
    {
	//Close:
	pix_vm_audio_deinit( vm );
	remove_slot = true;
    }

    if( callback != -1 && vm->audio_callback == -1 )
    {
	//Open:
	if( pix_vm_audio_init( vm ) != 0 ) return -1;
	set_slot = true;
    }

    //Set Pixilang audio parameters:
    if( freq == 0 )
    {
	if( vm->audio )
	    freq = vm->audio->freq;
	else
	    freq = sconfig_get_int_value( APP_CFG_SND_FREQ, 44100, 0 );
    }
    void* trash[ PIX_VM_AUDIO_CHANNELS * 2 ]; smem_clear_struct( trash );
    sundog_sound_lock( vm->audio ); //LOCK
    vm->audio_callback = callback;
    vm->audio_userdata = userdata;
    vm->audio_freq = freq;
    vm->audio_format = format;
    vm->audio_channels = channels;
    vm->audio_flags = flags;
    vm->audio_src_ptr = 0;
    vm->audio_src_rendered = 0;
    vm->audio_src_buffer_size = 0;
    vm->audio_src_buffer_ptr = 0;
    for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
    {
	if( vm->audio_src_buffers[ i ] )
	{
	    trash[ i ] = vm->audio_src_buffers[ i ];
	    vm->audio_src_buffers[ i ] = NULL;
	}
	if( vm->audio_input_buffers[ i ] )
	{
	    trash[ PIX_VM_AUDIO_CHANNELS + i ] = vm->audio_input_buffers[ i ];
	    vm->audio_input_buffers[ i ] = NULL;
	}
    }
    if( vm->audio_channels_cont == -1 )
    {
        //Create audio channels (container):
        vm->audio_channels_cont = pix_vm_new_container( -1, PIX_VM_AUDIO_CHANNELS, 1, PIX_CONTAINER_TYPE_INT32, 0, vm );
        pix_vm_set_container_flags( vm->audio_channels_cont, pix_vm_get_container_flags( vm->audio_channels_cont, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        vm->audio_input_channels_cont = pix_vm_new_container( -1, PIX_VM_AUDIO_CHANNELS, 1, PIX_CONTAINER_TYPE_INT32, 0, vm );
        pix_vm_set_container_flags( vm->audio_input_channels_cont, pix_vm_get_container_flags( vm->audio_input_channels_cont, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        //Create audio buffers (containers):
        for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
        {
    	    vm->audio_buffers_conts[ i ] = pix_vm_new_container( -1, 1, 1, PIX_CONTAINER_TYPE_INT32, &vm->audio_channels_cont, vm );
    	    vm->audio_input_buffers_conts[ i ] = pix_vm_new_container( -1, 1, 1, PIX_CONTAINER_TYPE_INT32, &vm->audio_input_channels_cont, vm );
    	    pix_vm_set_container_flags( vm->audio_buffers_conts[ i ], pix_vm_get_container_flags( vm->audio_buffers_conts[ i ], vm ) | PIX_CONTAINER_FLAG_STATIC_DATA | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
    	    pix_vm_set_container_flags( vm->audio_input_buffers_conts[ i ], pix_vm_get_container_flags( vm->audio_input_buffers_conts[ i ], vm ) | PIX_CONTAINER_FLAG_STATIC_DATA | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
	}
    }
    sundog_sound_unlock( vm->audio ); //UNLOCK
    for( int i = 0; i < (int)( sizeof( trash ) / sizeof( void* ) ); i++ ) smem_free( trash[ i ] );

    if( set_slot )
    {
	vm->audio_slot = sundog_sound_get_free_slot( vm->audio );
    	sundog_sound_set_slot_callback( vm->audio, vm->audio_slot, &pix_vm_render_piece_of_sound, vm );
	sundog_sound_play( vm->audio, vm->audio_slot );
    }

    if( remove_slot )
    {
    	sundog_sound_remove_slot_callback( vm->audio, vm->audio_slot );
    }

    return 0;
}

int pix_vm_get_audio_sample_rate( int source, pix_vm* vm )
{
    int rv = 0;
    if( source == 0 )
    {
	//Local sample rate (may be resampled):
	if( vm->audio_callback != -1 )
	{
	    rv = vm->audio_freq;
	}
    }
    else
    {
	//Global sample rate:
	if( vm->audio )
	    rv = vm->audio->freq;
	else
	    rv = sconfig_get_int_value( APP_CFG_SND_FREQ, 44100, 0 );
    }
    return rv;
}

#ifndef PIX_NOSUNVOX

static int pix_vm_sv_render_piece_of_sound( sundog_sound* ss, int slot_num )
{
    int handled = 0;

    sundog_sound_slot* slot = &ss->slots[ slot_num ];
    sunvox_engine* s = (sunvox_engine*)slot->user_data;

    if( !s ) return 0;
    if( !s->initialized ) return 0;

    sunvox_render_data rdata;
    smem_clear( &rdata, sizeof( sunvox_render_data ) );
    rdata.buffer_type = ss->out_type;
    rdata.buffer = slot->buffer;
    rdata.frames = slot->frames;
    rdata.channels = ss->out_channels;
    rdata.out_latency = ss->out_latency;
    rdata.out_latency2 = ss->out_latency2;
    rdata.out_time = slot->time;

    handled = sunvox_render_piece_of_sound( &rdata, s );

    if( handled && rdata.silence )
        handled = 2;

    return handled;
}

static int pix_vm_sv_sound_stream_control( sunvox_stream_command cmd, void* user_data, sunvox_engine* s )
{
    pix_vm_sunvox* sv = (pix_vm_sunvox*)user_data;
    pix_vm* vm = sv->vm;
    if( !vm || !sv->s ) return 0;
    int rv = 0;
    switch( cmd )
    {
        case SUNVOX_STREAM_LOCK:
    	    sv->lock_count++;
	    if( sv->flags & PIX_SV_INIT_FLAG_OFFLINE )
	    {
		if( !( sv->flags & PIX_SV_INIT_FLAG_ONE_THREAD ) )
		    smutex_lock( &sv->mutex );
	    }
	    else
	    {
    		if( sv->slot >= 0 ) sundog_sound_lock( vm->audio );
    	    }
            break;
        case SUNVOX_STREAM_UNLOCK:
	    if( sv->flags & PIX_SV_INIT_FLAG_OFFLINE )
	    {
		if( !( sv->flags & PIX_SV_INIT_FLAG_ONE_THREAD ) )
		    smutex_unlock( &sv->mutex );
	    }
	    else
	    {
    		if( sv->slot >= 0 ) sundog_sound_unlock( vm->audio );
    	    }
    	    sv->lock_count--;
            break;
        case SUNVOX_STREAM_STOP:
	    if( sv->flags & PIX_SV_INIT_FLAG_OFFLINE )
	    {
		if( !sv->suspended )
		{
		    if( sv->flags & PIX_SV_INIT_FLAG_ONE_THREAD )
			sv->suspended = true;
		    else
		    {
			smutex_lock( &sv->mutex );
			sv->suspended = true;
			smutex_unlock( &sv->mutex );
		    }
		}
	    }
	    else
	    {
    		if( sv->slot >= 0 ) sundog_sound_stop( vm->audio, sv->slot );
    	    }
            break;
        case SUNVOX_STREAM_PLAY:
	    if( sv->flags & PIX_SV_INIT_FLAG_OFFLINE )
	    {
		if( sv->suspended )
		{
		    if( sv->flags & PIX_SV_INIT_FLAG_ONE_THREAD )
			sv->suspended = false;
		    else
		    {
			smutex_lock( &sv->mutex );
			sv->suspended = false;
			smutex_unlock( &sv->mutex );
		    }
		}
	    }
	    else
	    {
    		if( sv->slot >= 0 ) sundog_sound_play( vm->audio, sv->slot );
    	    }
            break;
        case SUNVOX_STREAM_SYNC_PLAY:
            sundog_sound_sync_play( vm->audio, sv->slot, true );
            break;
        case SUNVOX_STREAM_SYNC:
            sundog_sound_slot_sync( vm->audio, sv->slot, sv->s->stream_control_par_sync );
            break;
        case SUNVOX_STREAM_ENABLE_INPUT:
    	    if( sv->slot >= 0 ) sundog_sound_input_request( vm->audio, true );
            break;
        case SUNVOX_STREAM_DISABLE_INPUT:
    	    if( sv->slot >= 0 ) sundog_sound_input_request( vm->audio, false );
    	    break;
    	case SUNVOX_STREAM_IS_SUSPENDED:
	    if( sv->flags & PIX_SV_INIT_FLAG_OFFLINE )
		rv = sv->suspended;
	    else
        	rv = sundog_sound_is_slot_suspended( vm->audio, sv->slot );
            break;
    }
    return rv;
}

int pix_vm_sv_new( int sample_rate, uint32_t flags, pix_vm* vm )
{
    int rv = -1;

    int err = 0;
    bool audio_init = false;
    int slot = -1;
    sunvox_engine* s = NULL;
    pix_vm_sunvox* sv = NULL;
    while( 1 )
    {
	for( rv = 0; rv < PIX_VM_SUNVOX_STREAMS; rv++ ) if( !vm->sv[ rv ] ) break;
	if( rv >= PIX_VM_SUNVOX_STREAMS ) { err = 1; break; }

	sv = (pix_vm_sunvox*)smem_new( sizeof( pix_vm_sunvox ) );
	if( !sv ) { err = 2; break; }
	smem_zero( sv );

	s = (sunvox_engine*)smem_new( sizeof( sunvox_engine ) );
	if( !s ) { err = 3; break; }

	if( flags & PIX_SV_INIT_FLAG_OFFLINE )
	{
	    smutex_init( &sv->mutex, 0 );
	}
	else
	{
	    if( pix_vm_audio_init( vm ) != 0 ) { err = 4; break; }
	    audio_init = true;
	    sample_rate = vm->audio->freq;
	}

	uint32_t sv_flags = SUNVOX_FLAG_MAIN;
        sv_flags |= SUNVOX_FLAG_NO_MIDI; //to prevent duplication of the public MIDI IN port (in Pixi app and in SunVox engine)
	if( flags & PIX_SV_INIT_FLAG_ONE_THREAD )
	    sv_flags |= SUNVOX_FLAG_ONE_THREAD;
	sunvox_engine_init(
	    sv_flags,
    	    sample_rate,
    	    NULL,
    	    vm->audio,
    	    pix_vm_sv_sound_stream_control, sv,
    	    s );

	if( !( flags & PIX_SV_INIT_FLAG_OFFLINE ) )
	{
	    slot = sundog_sound_get_free_slot( vm->audio );
	    if( slot < 0 ) { err = 5; break; }
    	    sundog_sound_set_slot_callback( vm->audio, slot, &pix_vm_sv_render_piece_of_sound, s );
	}

        COMPILER_MEMORY_BARRIER();
	sv->vm = vm;
	sv->s = s;
	sv->slot = slot;
	sv->flags = flags;
	sv->sample_rate = sample_rate;
        COMPILER_MEMORY_BARRIER();
    	vm->sv[ rv ] = sv;
        COMPILER_MEMORY_BARRIER();

	if( !( flags & PIX_SV_INIT_FLAG_OFFLINE ) )
	{
    	    sundog_sound_play( vm->audio, slot );
	}

	break;
    }

    if( err )
    {
	slog( "pix_vm_sv_new() error %d\n", err );
	if( slot >= 0 )
	{
    	    sundog_sound_stop( vm->audio, slot );
	    sundog_sound_remove_slot_callback( vm->audio, slot );
	}
	if( audio_init )
	{
	    pix_vm_audio_deinit( vm );
	}
	if( s )
	{
	    smem_zero( sv );
	    sunvox_engine_close( s );
	    smem_free( s );
	}
	if( rv >= 0 ) vm->sv[ rv ] = NULL;
	smem_free( sv );
	rv = -1;
    }

    return rv;
}

static bool is_sv_locked( int sv_id, pix_vm* vm, const char* fn_name )
{
    if( (unsigned)sv_id >= PIX_VM_SUNVOX_STREAMS ) return false;
    pix_vm_sunvox* sv = vm->sv[ sv_id ];
    if( !( sv->flags & PIX_SV_INIT_FLAG_ONE_THREAD ) && sv->lock_count <= 0 )
    {
        printf( "%s error: use it within { sv_lock() ... sv_unlock() } block only!\n", fn_name );
        return false;
    }
    return true;
}

static int pix_vm_sv_get_id( pix_vm* vm ) //Get priority slot with SunVox
{
    int sv_id = -1;
    for( int i = 0; i < SUNVOX_STREAM_LOCK; i++ )
    {
        if( vm->sv[ i ] )
        {
    	    sv_id = i;
        }
    }
    return sv_id;
}

#define GET_SV() \
    pix_vm_sunvox* sv = NULL; \
    if( sv_id < 0 ) sv_id = pix_vm_sv_get_id( vm ); \
    if( (unsigned)sv_id < PIX_VM_SUNVOX_STREAMS ) sv = vm->sv[ sv_id ];

int pix_vm_sv_remove( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !( sv->flags & PIX_SV_INIT_FLAG_OFFLINE ) )
    {
	if( vm->audio ) sundog_sound_remove_slot_callback( vm->audio, sv->slot );
	pix_vm_audio_deinit( vm );
	sv->slot = -1;
    }
    sv->vm = NULL;
    sunvox_engine_close( sv->s );
    if( sv->flags & PIX_SV_INIT_FLAG_OFFLINE )
    {
        smutex_destroy( &sv->mutex );
    }
    smem_free( sv->s );
    smem_free( sv );
    vm->sv[ sv_id ] = NULL;
    return 0;
}

int pix_vm_sv_get_sample_rate( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    return sv->sample_rate;
}

//Return value:
// 0 - silence, buffer is not filled;
// 1 - buffer is filled;
// 2 - silence, buffer is filled;
int pix_vm_sv_render( int sv_id, sunvox_render_data* rdata, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    if( !sv->s ) return 0;
    int rv = 0;

    if( !( sv->flags & PIX_SV_INIT_FLAG_ONE_THREAD ) ) smutex_lock( &sv->mutex );
    if( sv->suspended || !sv->s->initialized )
    {
	int frame_size = g_sample_size[ rdata->buffer_type ] * rdata->channels;
	smem_clear( rdata->buffer, rdata->frames * frame_size );
	rv = 2;
    }
    else
    {
        rv = sunvox_render_piece_of_sound( rdata, sv->s );
        if( rv && rdata->silence )
            rv = 2;
    }
    sv->last_frames = rdata->frames;
    sv->last_latency = rdata->out_latency;
    sv->last_out_t = rdata->out_time;
    if( !( sv->flags & PIX_SV_INIT_FLAG_ONE_THREAD ) ) smutex_unlock( &sv->mutex );

    return rv;
}

int pix_vm_sv_stream_control( int sv_id, int cmd, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    SUNVOX_SOUND_STREAM_CONTROL( sv->s, (sunvox_stream_command)cmd );
    return 0;
}

int pix_vm_sv_fload( int sv_id, sfs_file f, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1001;
    return sunvox_load_proj_from_fd( f, 0, sv->s );
}

int pix_vm_sv_fsave( int sv_id, sfs_file f, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1001;
    return sunvox_save_proj_to_fd( f, 0, sv->s );
}

int pix_vm_sv_play( int sv_id, int pos, bool jump_to_pos, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    sunvox_play( pos, jump_to_pos, -1, sv->s );
    return 0;
}

int pix_vm_sv_stop( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    sunvox_stop( sv->s );
    return 0;
}

int pix_vm_sv_pause( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    SUNVOX_SOUND_STREAM_CONTROL( sv->s, SUNVOX_STREAM_STOP );
    return 0;
}

int pix_vm_sv_resume( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    SUNVOX_SOUND_STREAM_CONTROL( sv->s, SUNVOX_STREAM_PLAY );
    return 0;
}

int pix_vm_sv_sync_resume( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    SUNVOX_SOUND_STREAM_CONTROL( sv->s, SUNVOX_STREAM_SYNC_PLAY );
    return 0;
}

int pix_vm_sv_set_autostop( int sv_id, bool autostop, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    sv->s->stop_at_the_end_of_proj = autostop;
    return 0;
}

int pix_vm_sv_get_autostop( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    return sv->s->stop_at_the_end_of_proj;
}

int pix_vm_sv_get_status( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( sv->s->playing )
	return 1;
    else
	return 0;
}

int pix_vm_sv_rewind( int sv_id, int pos, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    sunvox_rewind( pos, -1, sv->s );
    return 0;
}

int pix_vm_sv_volume( int sv_id, int vol, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    int prev_vol = sv->s->net->global_volume;
    if( vol >= 0 ) sv->s->net->global_volume = vol;
    return prev_vol;
}

int pix_vm_sv_set_event_t( int sv_id, int set, int t, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    sv->evt_t_set = set;
    sv->evt_t = t;
    return 0;
}

int pix_vm_sv_send_event( int sv_id, int track, int note, int vel, int mod, int ctl, int ctl_val, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    stime_ticks_t t;
    if( sv->evt_t_set )
	t = sv->evt_t;
    else
	t = stime_ticks();
    return svh_send_event( sv->s, t, track, note, vel, mod, ctl, ctl_val );
}

int pix_vm_sv_get_current_line( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    stime_ticks_t t;
    if( ( sv->flags & PIX_SV_INIT_FLAG_OFFLINE ) && sv->last_latency == 0 )
        t = sv->last_out_t;
    else
	t = stime_ticks();
    return sunvox_frames_get_value( SUNVOX_VF_CHAN_LINENUM, t, sv->s );
}

int pix_vm_sv_get_current_signal_level( int sv_id, int ch, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    stime_ticks_t t;
    if( ( sv->flags & PIX_SV_INIT_FLAG_OFFLINE ) && sv->last_latency == 0 )
        t = sv->last_out_t;
    else
	t = stime_ticks();
    return sunvox_frames_get_value( SUNVOX_VF_CHAN_VOL0 + ch, t, sv->s );
}

const char* pix_vm_sv_get_name( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return NULL;
    return sv->s->proj_name;
}

int pix_vm_sv_set_name( int sv_id, char* name, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    sunvox_rename( sv->s, name );
    return 0;
}

int pix_vm_sv_get_proj_par( int sv_id, int p, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    int rv = 0;
    switch( p )
    {
	case 0: rv = sv->s->bpm; break;
	case 1: rv = sv->s->speed; break;
	default: break;
    }
    return rv;
}

int pix_vm_sv_get_proj_len( int sv_id, int t, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    int rv = 0;
    switch( t )
    {
	case 0: rv = sunvox_get_proj_frames( sv->s ); break;
	case 1: rv = sunvox_get_proj_lines( sv->s ); break;
	default: break;
    }
    return rv;
}

int pix_vm_sv_get_time_map( int sv_id, int start_line, int len, uint32_t* dest, int flags, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( len <= 0 ) return -1;
    if( !dest ) return -1;
    int rv = -1;
    int map_type = flags & PIX_SV_TIME_MAP_TYPE_MASK;
    sunvox_time_map_item* map = (sunvox_time_map_item*)smem_new( sizeof( sunvox_time_map_item ) * len );
    if( map )
    {
        uint32_t* frame_map = NULL;
        if( map_type == PIX_SV_TIME_MAP_FRAMECNT ) frame_map = dest;
        sunvox_get_time_map( map, frame_map, start_line, len, sv->s );
        if( map_type == PIX_SV_TIME_MAP_SPEED ) for( int i = 0; i < len; i++ ) dest[ i ] = map[ i ].bpm | ( map[ i ].tpl << 16 );
        rv = 0;
        smem_free( map );
    }
    return rv;
}

int pix_vm_sv_new_module( int sv_id, char* name, char* type, int x, int y, int z, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !is_sv_locked( sv_id, vm, __FUNCTION__ ) ) return -1;
    PS_RETTYPE (*mod_hnd)( PSYNTH_MODULE_HANDLER_PARAMETERS ) = get_module_handler_by_name( type, sv->s );
    if( mod_hnd == psynth_empty ) return -1;
    if( !name ) name = type;
    int rv = psynth_add_module(
        -1,
        mod_hnd,
        name,
        0,
        x, y, z,
        sv->s->bpm,
        sv->s->speed,
        sv->s->net );
    if( rv > 0 )
    {
        psynth_do_command( rv, PS_CMD_SETUP_FINISHED, sv->s->net );
    }
    return rv;
}

int pix_vm_sv_remove_module( int sv_id, int mod, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !is_sv_locked( sv_id, vm, __FUNCTION__ ) ) return -1;
    psynth_remove_module( mod, sv->s->net );
    return 0;
}

int pix_vm_sv_connect_module( int sv_id, int src, int dst, bool disconnect, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !is_sv_locked( sv_id, vm, __FUNCTION__ ) ) return -1;
    if( !disconnect )
	psynth_make_link( dst, src, sv->s->net );
    else
	psynth_remove_link( dst, src, sv->s->net );
    return 0;
}

int pix_vm_sv_fload_module( int sv_id, sfs_file f, int x, int y, int z, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    int rv = sunvox_load_module_from_fd( -1, x, y, z, f, 0, sv->s );
    if( rv < 0 )
    {
        rv = psynth_add_module(
            -1,
            get_module_handler_by_name( "Sampler", sv->s ),
            "Sampler",
            0, x, y, z,
            sv->s->bpm,
            sv->s->speed,
            sv->s->net );
        if( rv > 0 )
        {
            psynth_do_command( rv, PS_CMD_SETUP_FINISHED, sv->s->net );
            sfs_rewind( f );
            sampler_load( NULL, f, rv, sv->s->net, -1, 0 );
        }
    }
    return rv;
}

const char* g_pix_vm_sv_mod_load_types[] = { "Sampler", "MetaModule", "Vorbis player" };
static int pix_vm_sv_mod_load_check( int sv_id, int modtype, int mod, pix_vm* vm )
{
    if( (unsigned)modtype > (unsigned)2 ) return -1;
    const char* modtype_str1 = pix_vm_sv_get_module_type( sv_id, mod, vm );
    const char* modtype_str2 = g_pix_vm_sv_mod_load_types[ modtype ];
    if( strcmp( modtype_str1, modtype_str2 ) )
    {
        slog( "Can't load data into the %s module. Expected type - %s\n", modtype_str1, modtype_str2 );
        return -1;
    }
    return 0;
}
int pix_vm_sv_mod_fload( int sv_id, int modtype, int mod, int slot, sfs_file f, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( pix_vm_sv_mod_load_check( sv_id, modtype, mod, vm ) ) return -1;
    int rv = -1;
    switch( modtype )
    {
	case 0: rv = sampler_load( NULL, f, mod, sv->s->net, slot, 0 ); break;
	case 1: rv = metamodule_load( NULL, f, mod, sv->s->net ); break;
	case 2: rv = vplayer_load_file( mod, NULL, f, sv->s->net ); break;
    }
    return rv;
}

int pix_vm_sv_sampler_par( int sv_id, int mod, int slot, int par, int par_val, int set, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    const char* modtype_str = pix_vm_sv_get_module_type( sv_id, mod, vm );
    if( strcmp( modtype_str, "Sampler" ) )
    {
        slog( "sv_sampler_par(): module %d is not a Sampler!\n", mod );
        return 0;
    }
    return sampler_par( sv->s->net, mod, slot, par, par_val, set );
}

int pix_vm_sv_get_number_of_modules( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    return sv->s->net->mods_num;
}

int pix_vm_sv_find_module( int sv_id, char* name, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    return psynth_get_module_by_name( name, sv->s->net );
}

int pix_vm_sv_selected_module( int sv_id, int mod, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    int rv = sv->s->selected_module;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
	sv->s->selected_module = mod;
	if( m->flags & PSYNTH_FLAG_GENERATOR )
    	    sv->s->last_selected_generator = mod;
    }
    return rv;
}

int pix_vm_sv_get_module_flags( int sv_id, int mod, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    int rv = 0;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
        rv |= PIX_SV_MODULE_FLAG_EXISTS;
        if( m->flags & PSYNTH_FLAG_GENERATOR ) rv |= PIX_SV_MODULE_FLAG_GENERATOR;
        if( m->flags & PSYNTH_FLAG_EFFECT ) rv |= PIX_SV_MODULE_FLAG_EFFECT;
        if( m->flags & PSYNTH_FLAG_MUTE ) rv |= PIX_SV_MODULE_FLAG_MUTE;
        if( m->flags & PSYNTH_FLAG_SOLO ) rv |= PIX_SV_MODULE_FLAG_SOLO;
        if( m->flags & PSYNTH_FLAG_BYPASS ) rv |= PIX_SV_MODULE_FLAG_BYPASS;
        rv |= m->input_links_num << PIX_SV_MODULE_INPUTS_OFF;
        rv |= m->output_links_num << PIX_SV_MODULE_OUTPUTS_OFF;
    }
    return rv;
}

int* pix_vm_sv_get_module_inouts( int sv_id, int mod, bool out, int* num, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return NULL;
    int* rv = NULL;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
        if( out )
        {
    	    rv = m->output_links;
    	    *num = m->output_links_num;
    	}
    	else
    	{
    	    rv = m->input_links;
    	    *num = m->input_links_num;
        }
    }
    return rv;
}

const char* pix_vm_sv_get_module_type( int sv_id, int mod, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return NULL;
    const char* rv = NULL;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
	psynth_event mod_evt = {};
        mod_evt.command = PS_CMD_GET_NAME;
        rv = (const char*)m->handler( mod, &mod_evt, sv->s->net );
        if( !rv ) rv = "";
        if( mod == 0 ) rv = "Output";
    }
    return rv;
}

const char* pix_vm_sv_get_module_name( int sv_id, int mod, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return NULL;
    const char* rv = NULL;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
        rv = (const char*)m->name;
    }
    return rv;
}

int pix_vm_sv_set_module_name( int sv_id, int mod, char* name, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    psynth_rename( mod, name, sv->s->net );
    return 0;
}

uint32_t pix_vm_sv_get_module_xy( int sv_id, int mod, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    uint32_t rv = 0;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
        uint32_t x = m->x;
        uint32_t y = m->y;
        rv = ( x & 0xFFFF ) | ( ( y & 0xFFFF ) << 16 );
    }
    return rv;
}

int pix_vm_sv_set_module_xy( int sv_id, int mod, int x, int y, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
        m->x = x;
        m->y = y;
        return 0;
    }
    return -1;
}

COLOR pix_vm_sv_get_module_color( int sv_id, int mod, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    COLOR rv = 0;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
        rv = get_color( m->color[ 0 ], m->color[ 1 ], m->color[ 2 ] );
    }
    return rv;
}

int pix_vm_sv_set_module_color( int sv_id, int mod, COLOR color, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
	m->color[ 0 ] = red( color );
	m->color[ 1 ] = green( color );
	m->color[ 2 ] = blue( color );
        return 0;
    }
    return -1;
}

uint32_t pix_vm_sv_get_module_finetune( int sv_id, int mod, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    uint32_t rv = 0;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
        uint32_t x = m->finetune;
        uint32_t y = m->relative_note;
        rv = ( x & 0xFFFF ) | ( ( y & 0xFFFF ) << 16 );
    }
    return rv;
}

int pix_vm_sv_set_module_finetune( int sv_id, int mod, int finetune, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
	m->finetune = finetune;
        return 0;
    }
    return -1;
}

int pix_vm_sv_set_module_relnote( int sv_id, int mod, int relative_note, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
        m->relative_note = relative_note;
        return 0;
    }
    return -1;
}

int pix_vm_sv_get_module_scope( int sv_id, int mod, int ch, pix_vm_container* dest_cont, int samples_to_read, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    int rv = 0;
    psynth_module* m = psynth_get_module( mod, sv->s->net );
    if( m )
    {
	if( (unsigned)samples_to_read > (unsigned)dest_cont->size ) samples_to_read = dest_cont->size;
        int size = 0;
        int offset = 0;
        stime_ticks_t t;
	if( ( sv->flags & PIX_SV_INIT_FLAG_OFFLINE ) && sv->last_latency == 0 )
	    t = sv->last_out_t + 1; //required for offline rendering with animation
	else
    	    t = stime_ticks();
        void* scope = psynth_get_scope_buffer( ch, &offset, &size, mod, t, sv->s->net );
        if( !scope || !size ) return 0;
        size--; //make mask
        rv = samples_to_read;
#ifdef PSYNTH_SCOPE_MODE_SLOW_HQ
        offset = ( offset - samples_to_read ) & size;
#else
        //Buffer is not cyclic. We cet get only last g_sound->out_frames:
        int last_frames = 0;
	if( !( sv->flags & PIX_SV_INIT_FLAG_OFFLINE ) )
	{
	    if( vm->audio ) last_frames = vm->audio->out_frames;
	}
	else
	{
	    last_frames = sv->last_frames;
	}
	if( last_frames < samples_to_read )
    	    rv = last_frames;
#endif
	switch( dest_cont->type )
        {
            case PIX_CONTAINER_TYPE_INT8:
    		for( int i = 0; i < rv; i++ )
	        {
	            PS_STYPE v = ((PS_STYPE*)scope)[ ( offset + i ) & size ];
	            int vv = v * 128 / PS_STYPE_ONE;
	            LIMIT_NUM( vv, -128, 127 );
	            ((int8_t*)dest_cont->data)[ i ] = vv;
	        }
                break;
            case PIX_CONTAINER_TYPE_INT16:
    		for( int i = 0; i < rv; i++ )
	        {
	            PS_STYPE v = ((PS_STYPE*)scope)[ ( offset + i ) & size ];
	            PS_STYPE_TO_INT16( ((int16_t*)dest_cont->data)[ i ], v );
	        }
                break;
            case PIX_CONTAINER_TYPE_INT32:
    		for( int i = 0; i < rv; i++ )
	        {
	            PS_STYPE v = ((PS_STYPE*)scope)[ ( offset + i ) & size ];
	            ((int32_t*)dest_cont->data)[ i ] = v * 32768 / PS_STYPE_ONE;
	        }
                break;
#ifdef PIX_INT64_ENABLED
            case PIX_CONTAINER_TYPE_INT64:
    		for( int i = 0; i < rv; i++ )
	        {
	            PS_STYPE v = ((PS_STYPE*)scope)[ ( offset + i ) & size ];
	            ((int64_t*)dest_cont->data)[ i ] = v * 32768 / PS_STYPE_ONE;
	        }
                break;
#endif
            case PIX_CONTAINER_TYPE_FLOAT32:
    		for( int i = 0; i < rv; i++ )
	        {
	            PS_STYPE v = ((PS_STYPE*)scope)[ ( offset + i ) & size ];
	            PS_STYPE_TO_FLOAT( ((float*)dest_cont->data)[ i ], v );
	        }
                break;
#ifdef PIX_FLOAT64_ENABLED
            case PIX_CONTAINER_TYPE_FLOAT64:
    		for( int i = 0; i < rv; i++ )
	        {
	            PS_STYPE v = ((PS_STYPE*)scope)[ ( offset + i ) & size ];
	            PS_STYPE_TO_FLOAT( ((double*)dest_cont->data)[ i ], v );
	        }
                break;
#endif
        }
    }
    return rv;
}

int pix_vm_sv_module_curve( int sv_id, int mod, int curve_num, pix_vm_container* data_cont, int len, int w, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    if( data_cont->type != PIX_CONTAINER_TYPE_FLOAT32 )
    {
	slog( "sv_module_curve(): data container type must be FLOAT32\n" );
	return 0;
    }
    if( len > (signed)data_cont->size ) len = data_cont->size;
    return psynth_curve( mod, curve_num, (float*)data_cont->data, len, w, sv->s->net );
}

int pix_vm_sv_get_module_ctl_cnt( int sv_id, int mod, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    return svh_get_number_of_module_ctls( sv->s, mod );
}

const char* pix_vm_sv_get_module_ctl_name( int sv_id, int mod, int ctl, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return NULL;
    return svh_get_module_ctl_name( sv->s, mod, ctl );
}

int pix_vm_sv_get_module_ctl_value( int sv_id, int mod, int ctl, int scaled, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    return svh_get_module_ctl_value( sv->s, mod, ctl, scaled );
}

int pix_vm_sv_set_module_ctl_value( int sv_id, int mod, int ctl, int val, int scaled, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    stime_ticks_t t;
    if( sv->evt_t_set )
	t = sv->evt_t;
    else
	t = stime_ticks();
    return svh_set_module_ctl_value( sv->s, t, mod, ctl, val, scaled );
}

int pix_vm_sv_get_module_ctl_par( int sv_id, int mod, int ctl, int scaled, int par, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    return svh_get_module_ctl_par( sv->s, mod, ctl, scaled, par );
}

int pix_vm_sv_new_pat( int sv_id, int clone, int x, int y, int tracks, int lines, int icon_seed, char* name, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !is_sv_locked( sv_id, vm, __FUNCTION__ ) ) return -1;
    sunvox_engine* s = sv->s;
    int rv = -1;
    if( clone >= 0 )
        rv = sunvox_new_pattern_clone( clone, x, y, s );
    else
    {
        rv = sunvox_new_pattern( lines, tracks, x, y, icon_seed, s );
        sunvox_rename_pattern( rv, name, s );
    }
    return rv;
}

int pix_vm_sv_remove_pat( int sv_id, int pat, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !is_sv_locked( sv_id, vm, __FUNCTION__ ) ) return -1;
    sunvox_engine* s = sv->s;
    sunvox_remove_pattern( pat, s );
    return 0;
}

int pix_vm_sv_get_number_of_pats( int sv_id, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return 0;
    return sv->s->pats_num;
}

int pix_vm_sv_find_pattern( int sv_id, char* name, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    return sunvox_get_pattern_num_by_name( name, sv->s );
}

int pix_vm_sv_get_pat( int sv_id, int pat, sunvox_pattern** out_pat_data, sunvox_pattern_info** out_pat_info, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    sunvox_engine* s = sv->s;
    if( (unsigned)pat >= (unsigned)s->pats_num ) return -1;
    if( !s->pats[ pat ] ) return -1;
    if( out_pat_data ) *out_pat_data = s->pats[ pat ];
    if( out_pat_info ) *out_pat_info = &s->pats_info[ pat ];
    return 0;
}

int pix_vm_sv_set_pat_xy( int sv_id, int pat, int x, int y, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !is_sv_locked( sv_id, vm, __FUNCTION__ ) ) return -1;
    sunvox_engine* s = sv->s;
    if( (unsigned)pat >= (unsigned)s->pats_num ) return -1;
    sunvox_pattern* pat_data = s->pats[ pat ];
    sunvox_pattern_info* pat_info = &s->pats_info[ pat ];
    if( !pat_data ) return -1;
    pat_info->x = x;
    pat_info->y = y;
    return 0;
}

int pix_vm_sv_set_pat_size( int sv_id, int pat, int tracks, int lines, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !is_sv_locked( sv_id, vm, __FUNCTION__ ) ) return -1;
    sunvox_engine* s = sv->s;
    if( (unsigned)pat >= (unsigned)s->pats_num ) return -1;
    if( !s->pats[ pat ] ) return -1;
    sunvox_pattern* pat_data = s->pats[ pat ];
    sunvox_pattern_info* pat_info = &s->pats_info[ pat ];
    if( !pat_data ) return -1;
    if( pat_data->data_xsize != tracks && tracks > 0 )
        sunvox_pattern_set_number_of_channels( pat, tracks, s );
    if( pat_data->data_ysize != lines && lines > 0 )
        sunvox_pattern_set_number_of_lines( pat, lines, 0, s );
    return 0;
}

int pix_vm_sv_set_pat_name( int sv_id, int pat, char* name, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !is_sv_locked( sv_id, vm, __FUNCTION__ ) ) return -1;
    sunvox_engine* s = sv->s;
    if( (unsigned)pat >= (unsigned)s->pats_num ) return -1;
    sunvox_pattern* pat_data = s->pats[ pat ];
    sunvox_pattern_info* pat_info = &s->pats_info[ pat ];
    if( !pat_data ) return -1;
    sunvox_rename_pattern( pat, name, s );
    return 0;
}

int pix_vm_sv_pat_mute( int sv_id, int pat, int mute, pix_vm* vm )
{
    GET_SV();
    if( !sv ) return -1;
    if( !is_sv_locked( sv_id, vm, __FUNCTION__ ) ) return -1;
    sunvox_engine* s = sv->s;
    if( (unsigned)pat >= (unsigned)s->pats_num ) return -1;
    if( !s->pats[ pat ] ) return -1;
    int prev_val = 0;
    if( s->pats_info[ pat ].flags & SUNVOX_PATTERN_INFO_FLAG_MUTE ) prev_val = 1;
    if( mute == 1 ) s->pats_info[ pat ].flags |= SUNVOX_PATTERN_INFO_FLAG_MUTE;
    if( mute == 0 ) s->pats_info[ pat ].flags &= ~SUNVOX_PATTERN_INFO_FLAG_MUTE;
    return prev_val;
}

#endif
