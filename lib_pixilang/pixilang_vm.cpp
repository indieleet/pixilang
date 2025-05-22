/*
    pixilang_vm.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"

#include <errno.h>

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) slog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif

//#define VM_DEBUGGER

#define FIRST_CODE_PAGE	    1024
#define NEXT_CODE_PAGE	    1024

const char* g_pix_container_type_names[] =
{
    "INT8",
    "INT16",
    "INT32",
    "INT64",
    "FLOAT32",
    "FLOAT64"
};

const int g_pix_container_type_sizes[] =
{
    1,
    2,
    4,
    8,
    4,
    8,
    sizeof( PIX_VAL )
};

int pix_vm_init( pix_vm* vm, WINDOWPTR win )
{
    int rv = 1;
    while( 1 )
    {
	smem_clear( vm, sizeof( pix_vm ) );

	vm->win = win;
	vm->wm = win->wm;

	//Containers:
	{
	    size_t c_num = 8192;
#ifdef PIXI_CONTAINERS_NUM
	    c_num = PIXI_CONTAINERS_NUM;
#endif
	    for( int i = 0; g_app_options[ i ].n != NULL; i++ )
	    {
		if( smem_strcmp( g_app_options[ i ].n, "pixi_containers_num" ) == 0 )
		{
		    c_num = g_app_options[ i ].v;
		    break;
		}
	    }
	    vm->c_num = sconfig_get_int_value( "pixi_containers_num", c_num, 0 );
	    vm->c = (pix_vm_container**)smem_znew( sizeof( pix_vm_container* ) * vm->c_num );
	    if( !vm->c )
	    {
		slog( "Memory allocation error (containers)\n" );
		break;
    	    }
    	    smutex_init( &vm->c_mutex, 0 );
	    vm->c_counter = 0;
	}

	//Timers:
	{
	    int timers_num = 16;
#ifdef PIXI_TIMERS_NUM
	    timers_num = PIXI_TIMERS_NUM;
#endif
	    for( int i = 0; g_app_options[ i ].n != NULL; i++ )
	    {
		if( smem_strcmp( g_app_options[ i ].n, "pixi_timers_num" ) == 0 )
		{
		    timers_num = g_app_options[ i ].v;
		    break;
		}
	    }
	    vm->timers_num = sconfig_get_int_value( "pixi_timers_num", timers_num, 0 );
	    vm->timers = (uint*)smem_znew( sizeof( uint ) * vm->timers_num );
	    if( !vm->timers )
	    {
		slog( "Memory allocation error (timers)\n" );
		break;
    	    }
    	}

	//Fonts:
	{
	    int fonts_num = 8;
#ifdef PIXI_FONTS_NUM
	    fonts_num = PIXI_FONTS_NUM;
#endif
	    for( int i = 0; g_app_options[ i ].n != NULL; i++ )
	    {
		if( smem_strcmp( g_app_options[ i ].n, "pixi_fonts_num" ) == 0 )
		{
		    fonts_num = g_app_options[ i ].v;
		    break;
		}
	    }
	    vm->fonts_num = sconfig_get_int_value( "pixi_fonts_num", fonts_num, 0 );
	    vm->fonts = (pix_vm_font*)smem_znew( sizeof( pix_vm_font ) * vm->fonts_num );
	    if( !vm->fonts )
	    {
		slog( "Memory allocation error (fonts)\n" );
		break;
    	    }
    	}
    
        //Events:
	smutex_init( &vm->events_mutex, 0 );
    
        //Threads:
	smutex_init( &vm->th_mutex, 0 );
    
        //Log:
	vm->log_buffer = (char*)smem_new( 4 * 1024 );
        smem_zero( vm->log_buffer );
	vm->log_prev_msg = (char*)smem_new( 4 );
        vm->log_prev_msg[ 0 ] = 0;
	smutex_init( &vm->log_mutex, 0 );

        //Some initial values:
        vm->zbuf = -1;
        vm->pixel_size = 1;
        vm->transp = 255;
	pix_vm_gfx_matrix_reset( vm );
	vm->gl_callback = -1;
	vm->audio_callback = -1;
	vm->quit_action = 1;
	vm->audio_channels_cont = -1;
        vm->audio_input_channels_cont = -1;
	for( int i = 0; i < PIX_VM_AUDIO_CHANNELS; i++ )
        {
    	    vm->audio_buffers_conts[ i ] = -1;
	    vm->audio_input_buffers_conts[ i ] = -1;
        }
	vm->event = -1;
	vm->current_path = -1;
        vm->user_path = -1;
	vm->temp_path = -1;
        vm->os_name = -1;
	vm->arch_name = -1;
        vm->lang_name = -1;

#ifdef OPENGL
	if( pix_vm_gl_init( vm ) )
	{
	    slog( "OpenGL init error\n" );
	    break;
	}
#endif
        
    	COMPILER_MEMORY_BARRIER();
        vm->ready = 1;
        rv = 0;
        break;
    }
    
    return rv;
}

int pix_vm_deinit( pix_vm* vm )
{
    int rv = 0;

    if( !vm ) return 2;

    vm->ready = 0;

    //Stop separate threads created by Pixilang:
    int sleep_counter = 0;
    int sleep_counter_deadline = 2000;
    for( int i = 0; i < PIX_VM_THREADS; i++ )
    {
	smutex_lock( &vm->th_mutex );
	if( vm->th[ i ] )
	{
	    if( vm->th[ i ]->thread_open )
	    {
		int step_ms = 25;
		bool closed = 0;
		while( sleep_counter < sleep_counter_deadline )
		{
		    int r = sthread_destroy( &vm->th[ i ]->th, -step_ms );
		    smutex_unlock( &vm->th_mutex );
		    stime_sleep( 10 );
		    smutex_lock( &vm->th_mutex );
		    if( r == 0 )
		    {
			closed = 1;
			break;
		    }
		    sleep_counter += step_ms;
		}
		if( closed == 0 )
		{
		    vm->th[ i ]->active = 0;
		    if( sthread_destroy( &vm->th[ i ]->th, 100 ) )
		    {
			PIX_VM_LOG( "Thread %d is not responding. Forcibly removed.\n", i );
		    }
		    else 
		    {
			PIX_VM_LOG( "Thread %d is not responding. Forcibly removed (via halt).\n", i );
		    }
		}
	    }
	}
	smutex_unlock( &vm->th_mutex );
    }

    //Close audio stream:
    pix_stack_val u;
    u.i = 0;
    u.t = 0;
    pix_vm_set_audio_callback( -1, u, 0, PIX_CONTAINER_TYPE_INT16, 0, 0, vm );
#ifndef PIX_NOSUNVOX
    for( int i = 0; i < PIX_VM_SUNVOX_STREAMS; i++ )
    {
	if( !vm->sv[ i ] ) continue;
	pix_vm_sv_remove( i, vm );
    }
#endif

    //Remove all threads data:
    for( int i = 0; i < PIX_VM_THREADS; i++ )
    {
	smutex_lock( &vm->th_mutex );
	if( vm->th[ i ] )
	{
	    smem_free( vm->th[ i ] );
	    vm->th[ i ] = NULL;
	}
	smutex_unlock( &vm->th_mutex );
    }
    smutex_destroy( &vm->th_mutex );

    //Log:
    smutex_lock( &vm->log_mutex );
    smem_free( vm->log_buffer );
    smem_free( vm->log_prev_msg );
    vm->log_buffer = NULL;
    vm->log_prev_msg = NULL;
    smutex_unlock( &vm->log_mutex );

    //Compiler errors:
    smem_free( vm->compiler_errors );
    vm->compiler_errors = NULL;

    //Events:
    smutex_destroy( &vm->events_mutex );

    //Variables:
    if( vm->var_names )
    {
	for( size_t i = 0; i < vm->vars_num; i++ )
	{
	    smem_free( vm->var_names[ i ] );
	}
    }
    smem_free( vm->vars );
    smem_free( vm->var_names );
    vm->vars = NULL;
    vm->var_names = NULL;
    vm->vars_num = 0;
    
    //Containers:
    vm->c_ignore_mutex = 1;
    if( vm->c )
    {
	for( PIX_CID i = 0; i < (PIX_CID)( smem_get_size( vm->c ) / sizeof( pix_vm_container* ) ); i++ )
	{
	    pix_vm_remove_container( i, vm );
	}
	smem_free( vm->c );
	vm->c = NULL;
    }
    smutex_destroy( &vm->c_mutex );
    
    //Timers:
    smem_free( vm->timers );
    vm->timers = NULL;

    //Fonts:
    smem_free( vm->fonts );
    vm->fonts = NULL;
    
    //Text:
    smem_free( vm->text );
    smem_free( vm->text_lines );
    vm->text = NULL;
    vm->text_lines = NULL;
    
    //Effector:
    smem_free( vm->effector_colors_r );
    smem_free( vm->effector_colors_g );
    smem_free( vm->effector_colors_b );
    
    //Code:
    smem_free( vm->code );
    vm->code = NULL;
    vm->code_ptr = 0;
    vm->code_size = 0;

    //Virtual disk0:
    sfs_close( vm->virt_disk0 );
    
    //Base path:
    smem_free( vm->base_path );
    vm->base_path = NULL;

    //Final log deinit:
    smutex_destroy( &vm->log_mutex );
    
    //SunDog requests:
    smem_free( vm->sd_filedialog );
    smem_free( vm->sd_textinput );
    smem_free( vm->sd_vcap );
    vm->sd_filedialog = NULL;
    vm->sd_textinput = NULL;
    vm->sd_vcap = NULL;
    
#ifdef OPENGL
    pix_vm_gl_deinit( vm );
#endif

    return rv;
}

void pix_vm_log( char* message, pix_vm* vm )
{
    if( !vm->log_buffer )
    {
	slog( "%s", message );
	return;
    }
    smutex_lock( &vm->log_mutex );
    while( 1 )
    {
	size_t log_size = smem_get_size( vm->log_buffer );
	size_t msg_size = smem_strlen( message );
	if( log_size == 0 || msg_size == 0 ) break;
	if( message[ msg_size - 1 ] == 0xA && smem_strcmp( message, vm->log_prev_msg ) == 0 )
	{
	    //the same message (with "new line" char):
	    if( vm->log_prev_msg_repeat_cnt >= 256 ) break;
	    vm->log_prev_msg_repeat_cnt++;
	}
	else vm->log_prev_msg_repeat_cnt = 0;

	slog( "%s", message );

	if( vm->log_prev_msg && vm->log_prev_msg_len && vm->log_prev_msg[ vm->log_prev_msg_len - 1 ] == 0xA )
	{
	    char date[ 10 ];
	    sprintf( date, "%02d:%02d:%02d ", stime_hours(), stime_minutes(), stime_seconds() );
	    for( int i = 0; i < 9; i++ )
	    {
    		vm->log_buffer[ vm->log_ptr ] = date[ i ];
    		vm->log_ptr++;
    		if( vm->log_ptr >= log_size ) vm->log_ptr = 0;
	    }
	    vm->log_filled += 9;
	}

	for( size_t i = 0; i < msg_size; i++ )
	{
    	    vm->log_buffer[ vm->log_ptr ] = message[ i ];
    	    vm->log_ptr++;
    	    if( vm->log_ptr >= log_size ) vm->log_ptr = 0;
	}
	vm->log_filled += msg_size;
	if( vm->log_filled > log_size ) vm->log_filled = log_size;

	if( smem_get_size( vm->log_prev_msg ) < msg_size + 1 )
	    vm->log_prev_msg = (char*)smem_resize( vm->log_prev_msg, msg_size + 1 );
	if( vm->log_prev_msg )
	{
	    smem_copy( vm->log_prev_msg, message, msg_size + 1 );
	    vm->log_prev_msg_len = msg_size;
	}

	break;
    }
    smutex_unlock( &vm->log_mutex );
}

void pix_vm_put_opcode( PIX_OPCODE opcode, pix_vm* vm )
{
    if( vm->code == 0 )
    {
	vm->code = (PIX_OPCODE*)smem_new( FIRST_CODE_PAGE * sizeof( PIX_OPCODE ) );
	vm->code_size = 1;
    }
    
    if( vm->code_ptr >= vm->code_size )
    {
	vm->code_size = vm->code_ptr + 1;
	if( vm->code_size > smem_get_size( vm->code ) / sizeof( PIX_OPCODE ) )
	{
	    vm->code = (PIX_OPCODE*)smem_resize( vm->code, ( vm->code_size + NEXT_CODE_PAGE ) * sizeof( PIX_OPCODE ) );
	}
    }
    
    vm->code[ vm->code_ptr++ ] = opcode;
}

void pix_vm_put_int( PIX_INT v, pix_vm* vm )
{
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_INT ) )
    {
	pix_vm_put_opcode( (PIX_OPCODE)v, vm );
    }
    else 
    {
	for( uint i = 0; i < sizeof( PIX_INT ); i += sizeof( PIX_OPCODE ) )
	{
	    PIX_OPCODE opcode;
	    uint size = sizeof( PIX_INT ) - i;
	    if( size > sizeof( PIX_OPCODE ) ) size = sizeof( PIX_OPCODE );
	    smem_copy( &opcode, (char*)&v + i, size );
	    pix_vm_put_opcode( opcode, vm );
	}
    }
}

void pix_vm_put_float( PIX_FLOAT v, pix_vm* vm )
{
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_FLOAT ) )
    {
	volatile PIX_OPCODE opcode;
	volatile PIX_FLOAT* ptr = (PIX_FLOAT*)&opcode;
	*ptr = v;
	pix_vm_put_opcode( opcode, vm );
    }
    else 
    {
	for( uint i = 0; i < sizeof( PIX_FLOAT ); i += sizeof( PIX_OPCODE ) )
	{
	    PIX_OPCODE opcode;
	    uint size = sizeof( PIX_FLOAT ) - i;
	    if( size > sizeof( PIX_OPCODE ) ) size = sizeof( PIX_OPCODE );
	    smem_copy( &opcode, (char*)&v + i, size );
	    pix_vm_put_opcode( opcode, vm );
	}
    }
}

char* pix_vm_get_variable_name( pix_vm* vm, size_t vnum )
{
    char* name = 0;
    if( vm->var_names && vnum < vm->vars_num )
    {
	name = vm->var_names[ vnum ];
	if( name == 0 )
	{
	    if( vnum < 128 )
	    {
		vm->var_names[ vnum ] = (char*)smem_new( 2 );
		name = vm->var_names[ vnum ];
		name[ 0 ] = (char)vnum;
		name[ 1 ] = 0;
	    }
	}
    }
    return name;
}

void pix_vm_resize_variables( pix_vm* vm )
{
    if( vm->vars == 0 )
    {
	vm->vars = (pix_stack_val*)smem_znew( vm->vars_num * sizeof( pix_stack_val ) );
	vm->var_names = (char**)smem_znew( vm->vars_num * sizeof( char* ) );
    }
    else
    {
	size_t prev_size = smem_get_size( vm->vars ) / sizeof( pix_stack_val );
	if( vm->vars_num > prev_size ) 
	{
	    size_t new_size = vm->vars_num + 64;
	    vm->vars = (pix_stack_val*)smem_resize2( vm->vars, new_size * sizeof( pix_stack_val ) );
	    vm->var_names = (char**)smem_resize2( vm->var_names, new_size * sizeof( char* ) );
	}
    }
}

int pix_vm_send_event(
    int16_t type,
    int16_t flags,
    int16_t x,
    int16_t y,
    int16_t key,
    int16_t scancode,
    int16_t pressure,
    pix_vm* vm )
{
    int rv = 0;
    
    if( !vm ) return 2;
    
    smutex_lock( &vm->events_mutex );
    
    if( vm->events_count + 1 <= PIX_VM_EVENTS )
    {
	//Get pointer to a new event:
	int new_ptr = ( vm->current_event_num + vm->events_count ) & ( PIX_VM_EVENTS - 1 );
	
	//Save new event to FIFO buffer:
	vm->events[ new_ptr ].type = (uint16_t)type;
	vm->events[ new_ptr ].time = stime_ms();
	vm->events[ new_ptr ].flags = (uint16_t)flags & EVT_FLAGS_MASK;
	vm->events[ new_ptr ].x = (int16_t)x / vm->pixel_size;
	vm->events[ new_ptr ].y = (int16_t)y / vm->pixel_size;
	vm->events[ new_ptr ].key = (uint16_t)key;
	vm->events[ new_ptr ].scancode = (uint16_t)scancode;
	vm->events[ new_ptr ].pressure = (uint16_t)pressure;
	
	//Increment number of unhandled events:
	vm->events_count = vm->events_count + 1;
    }
    else
    {
	rv = 1;
    }
    
    smutex_unlock( &vm->events_mutex );
    
    return rv;
}

int pix_vm_send_event(
    int16_t type,
    int16_t flags,
    pix_vm* vm )
{
    return pix_vm_send_event( type, flags, 0, 0, 0, 0, 0, vm );
}

int pix_vm_get_event( pix_vm* vm )
{
    if( vm->events_count )
    {
	smutex_lock( &vm->events_mutex );
	
	//There are unhandled events:
	//Copy current event (prepare it for handling):
	pix_vm_event* evt = &vm->events[ vm->current_event_num ];
	int current_event = vm->event;
	if( (unsigned)current_event < (unsigned)vm->c_num )
	{
	    pix_vm_container* c = vm->c[ current_event ];
	    if( c && c->data && c->type == PIX_CONTAINER_TYPE_INT32 && c->size >= 16 )
	    {
		int* fields = (int*)c->data;
		fields[ 0 ] = evt->type;
		fields[ 1 ] = evt->flags;
		fields[ 2 ] = evt->time;
		fields[ 3 ] = evt->x;
		fields[ 4 ] = evt->y;
		fields[ 5 ] = evt->key;
		fields[ 6 ] = evt->scancode;
		fields[ 7 ] = evt->pressure;
	    }
	}
	//This event will be handled. So decrement count of events:
	vm->events_count--;
	//And increment FIFO pointer:
	vm->current_event_num = ( vm->current_event_num + 1 ) & ( PIX_VM_EVENTS - 1 );
	
	smutex_unlock( &vm->events_mutex );
	
	return 1;
    }
    else 
    {
	return 0;
    }
}

#define CONTROL_PC { }
#define LOAD_OPCODE( v ) { v = code[ pc++ ]; CONTROL_PC; }
#define LOAD_INT( v ) \
{ \
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_INT ) ) \
    { \
	v = (PIX_INT)code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 2 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 4 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 2 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 3 ] = code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 8 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 2 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 3 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 4 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 5 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 6 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 7 ] = code[ pc++ ]; CONTROL_PC; \
    } \
}
#define LOAD_FLOAT( v ) \
{ \
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_INT ) ) \
    { \
	v = *( (PIX_FLOAT*)&code[ pc++ ] ); CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 2 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 4 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 2 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 3 ] = code[ pc++ ]; CONTROL_PC; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 8 ) \
    { \
	PIX_OPCODE* v_ptr = (PIX_OPCODE*)&v; \
	v_ptr[ 0 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 1 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 2 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 3 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 4 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 5 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 6 ] = code[ pc++ ]; CONTROL_PC; \
	v_ptr[ 7 ] = code[ pc++ ]; CONTROL_PC; \
    } \
}

void* pix_vm_thread_body( void* user_data )
{
    pix_vm_thread* th = (pix_vm_thread*)user_data;
    int thread_num = th->thread_num;
    
    DPRINT( "Thread %d begin.\n", thread_num );
    
    pix_vm_run( th->thread_num, 0, 0, PIX_VM_CONTINUE, th->vm );

    DPRINT( "Thread %d end.\n", thread_num );
    
    return 0;
}

int pix_vm_create_active_thread( int thread_num, pix_vm* vm )
{
    smutex_lock( &vm->th_mutex );
    if( (unsigned)thread_num >= PIX_VM_THREADS )
    {
	for( int i = 0; i < PIX_VM_THREADS - PIX_VM_SYSTEM_THREADS; i++ )
	{
	    if( vm->th[ i ] )
	    {
		if( vm->th[ i ]->active == 0 && ( vm->th[ i ]->flags & PIX_THREAD_FLAG_AUTO_DESTROY ) )
		{
		    pix_vm_destroy_thread( i, 200, vm );
		}
	    }
	    if( vm->th[ i ] == 0 )
	    {
		thread_num = i;
		break;
	    }
	}
	if( (unsigned)thread_num >= PIX_VM_THREADS )
	{
	    //No thread
	    PIX_VM_LOG( "Can't create a new thread. No free slots.\n" );
	    smutex_unlock( &vm->th_mutex );
	    return -1;
	}
    }
    if( vm->th[ thread_num ] == 0 )
    {
	vm->th[ thread_num ] = (pix_vm_thread*)smem_new( sizeof( pix_vm_thread ) );
	smem_zero( vm->th[ thread_num ] );
    }
    vm->th[ thread_num ]->active = 1;
    smutex_unlock( &vm->th_mutex );
    return thread_num;
}

int pix_vm_destroy_thread( int thread_num, PIX_INT timeout, pix_vm* vm )
{
    int rv = -1;
    if( (unsigned)thread_num < (unsigned)PIX_VM_THREADS )
    {
        if( timeout == PIX_INT_MAX_POSITIVE ) timeout = 0x7FFFFFFF;
        pix_vm_thread* t = vm->th[ thread_num ];
        if( t && t->thread_open )
        {
            rv = sthread_destroy( &t->th, (int)timeout );
            if( rv == 1 && timeout < 0 )
            {
                //Don't touch
            }
            else
            {
                smutex_lock( &vm->th_mutex );
                smem_free( t );
                vm->th[ thread_num ] = 0;
                smutex_unlock( &vm->th_mutex );
            }
        }
    }
    return rv;
}

int pix_vm_get_thread_retval( int thread_num, pix_vm* vm, pix_stack_val* retval )
{
    if( (unsigned)thread_num < PIX_VM_THREADS )
    {
	pix_vm_thread* th = vm->th[ thread_num ];
	if( th->sp < PIX_VM_STACK_SIZE )
        {
            *retval = th->stack[ th->sp ];
            return 0;
        }
    }
    retval[ 0 ].i = 0;
    retval[ 0 ].t = 0;
    return -1;
}

int pix_vm_run(
    int thread_num, 
    bool open_new_thread, 
    pix_vm_function* fun, 
    pix_vm_run_mode mode, 
    pix_vm* vm )
{
    pix_vm_thread* th;

    thread_num = pix_vm_create_active_thread( thread_num, vm );
    if( thread_num < 0 ) return thread_num;
    th = vm->th[ thread_num ];

    if( mode == PIX_VM_CONTINUE )
    {
    }
    else
    {
	//Reset thread:
	th->sp = PIX_VM_STACK_SIZE;
	//Push the parameters:
	if( fun && fun->p_num )
	{
	    for( int pnum = fun->p_num - 1; pnum >= 0; pnum-- )
	    {
		th->sp--;
		th->stack[ th->sp ] = fun->p[ pnum ];
	    }
	    th->sp--;
	    th->fp = th->sp;
	    th->stack[ th->sp ].i = fun->p_num;
	    th->stack[ th->sp ].t = 0;
	}
	else 
	{
	    //No parameters:
	    th->sp--;
	    th->fp = th->sp;
	    th->stack[ th->sp ].i = 0;
	    th->stack[ th->sp ].t = 0;
	}
	//Push previous FP:
	th->sp--;
	th->stack[ th->sp ].i = PIX_VM_STACK_SIZE - 1;
	//Push previous PC:
	th->sp--;
	th->stack[ th->sp ].i = (PIX_INT)vm->halt_addr;
	//Set function pointer:
	switch( mode )
	{
	    case PIX_VM_CALL_FUNCTION: th->pc = fun->addr; break;
	    case PIX_VM_CALL_MAIN: th->pc = 1; break;
	    default: break;
	}
    }
    
    if( open_new_thread )
    {
	//Open new thread for code execution:
	th->thread_num = thread_num;
	th->vm = vm;
	th->thread_open = 1;
	sthread_create( &th->th, vm->wm->sd, &pix_vm_thread_body, (void*)th, 0 );
	return thread_num;
    }
    else
    {
	//Execute code in the current thread (blocking mode):
	PIX_PC pc = th->pc;
	PIX_SP sp = th->sp;
	PIX_SP fp = th->fp;
	PIX_OPCODE* code = vm->code;
	PIX_PC code_size = vm->code_size;
	pix_stack_val* vars = vm->vars;
	pix_stack_val* stack = th->stack;
	PIX_OPCODE val;
	PIX_INT val_i;
	PIX_FLOAT val_f;
	if( pc >= code_size )
	{
	    PIX_VM_LOG( "Error. Incorrect PC (program counter) value %u\n", (unsigned int)pc );
	}
	else
	while( th->active )
	{
#ifdef VM_DEBUGGER
	    PIX_VM_LOG( "PC:%d; SP:%d; FP:%d. q - exit.\n", (int)pc, (int)sp, (int)fp );
	    int key = getchar();
	    if( key == 'q' ) { th->active = 0; break; }
#endif
	    PIX_OPCODE c = code[ pc ];
	    pc++;
	    switch( (pix_vm_opcode)( c & PIX_OPCODE_MASK ) )
	    {
		case OPCODE_NOP:
		    break;

		case OPCODE_HALT:
 		    th->active = 0;
		    break;

		//case OPCODE_TMP: break;

		case OPCODE_PUSH_I:
		    {
			LOAD_INT( val_i );
			sp--;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			stack[ sp2 ].t = 0;
			stack[ sp2 ].i = val_i;
		    }
		    break;
		case OPCODE_PUSH_i:
		    {
			sp--;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			stack[ sp2 ].t = 0;
			stack[ sp2 ].i = (signed)c >> PIX_OPCODE_BITS;
		    }
		    break;
                case OPCODE_PUSH_F:
            	    {
			LOAD_FLOAT( val_f );
			sp--;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			stack[ sp2 ].t = 1;
			stack[ sp2 ].f = val_f;
		    }
		    break;
		case OPCODE_PUSH_v:
		    {
			val = c >> PIX_OPCODE_BITS;
			sp--;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			stack[ sp2 ] = vars[ val ];
		    }
		    break;

		case OPCODE_GO:
		    {
			PIX_ADDR addr;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			if( stack[ sp2 ].t == 0 )
			    addr = stack[ sp2 ].i;
			else
			    addr = stack[ sp2 ].f;
			if( IS_ADDRESS_CORRECT( addr ) )
			    pc = addr & PIX_INT_ADDRESS_MASK;
			else
			    PIX_VM_LOG( "Pixilang VM Error. %u: OPCODE_GO. Address %u is incorrect\n", (unsigned int)pc, (unsigned int)stack[ sp2 ].i );
			sp++;
		    }
		    break;
		case OPCODE_JMP_i:
		    pc += ( (signed)c >> PIX_OPCODE_BITS ) - 1;
		    break;
		case OPCODE_JMP_IF_FALSE_i:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			if( ( stack[ sp2 ].t == 0 && stack[ sp2 ].i == 0 ) ||
		    	    ( stack[ sp2 ].t == 1 && stack[ sp2 ].f == 0 ) )
			{
			    pc += ( (signed)c >> PIX_OPCODE_BITS ) - 1;
			}
			sp++;
		    }
		    break;

		case OPCODE_SAVE_TO_VAR_v:
		    {
			val = c >> PIX_OPCODE_BITS;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			vars[ val ] = stack[ sp2 ];
			sp++;
		    }
		    break;

		case OPCODE_SAVE_TO_PROP_I:
		    {
			LOAD_INT( val_i );
			PIX_CID cnum;
			int prop_hash;
			char* prop_name;
	                cnum = (PIX_CID)stack[ PIX_CHECK_SP( sp + 1 ) ].i;
	            	prop_name = pix_vm_get_variable_name( vm, val_i );
	            	if( prop_name )
	            	{
	            	    prop_hash = (int)vm->vars[ val_i ].i;
	            	    pix_vm_set_container_property( cnum, prop_name + 1, prop_hash, stack[ PIX_CHECK_SP( sp ) ].t, stack[ PIX_CHECK_SP( sp ) ].v, vm );
	            	}
	                sp += 2;
		    }
		    break;
		case OPCODE_LOAD_FROM_PROP_I:
		    {
			LOAD_INT( val_i );
			PIX_CID cnum;
			size_t prop_var;
			char* prop_name;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
	                cnum = (PIX_CID)stack[ sp2 ].i;
	            	prop_name = pix_vm_get_variable_name( vm, val_i );
	            	bool loaded = 0;
	            	if( prop_name )
	            	{
	            	    pix_sym* sym = pix_vm_get_container_property( 
	            		cnum, 
	            		prop_name + 1, //Property name
	            		(int)vm->vars[ val_i ].i, //Property hash
	            		vm );
	            	    if( sym )
	            	    {
	            		stack[ sp2 ].v = sym->val;
	            		if( sym->type == SYMTYPE_NUM_F )
	            		    stack[ sp2 ].t = 1;
	            		else
	            		    stack[ sp2 ].t = 0;
	            		loaded = 1;
	            	    }
	            	}
	            	if( loaded == 0 )
	            	{
	            	    stack[ sp2 ].i = 0;
	            	    stack[ sp2 ].t = 0;
			}
		    }
		    break;

		case OPCODE_SAVE_TO_MEM:
		    {
			PIX_CID cnum;
	                cnum = (PIX_CID)stack[ PIX_CHECK_SP( sp + 2 ) ].i;
			if( (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
			{
			    pix_vm_container* cont = vm->c[ cnum ];
	                    PIX_INT offset;
			    if( stack[ PIX_CHECK_SP( sp + 1 ) ].t == 0 )
				offset = stack[ PIX_CHECK_SP( sp + 1 ) ].i;
			    else
				offset = (PIX_INT)stack[ PIX_CHECK_SP( sp + 1 ) ].f;
			    if( (unsigned)offset < cont->size )
			    {
				PIX_SP sp2 = PIX_CHECK_SP( sp );
				if( stack[ sp2 ].t == 0 )
				{
				    //Integer value:
				    switch( cont->type )
				    {
					case PIX_CONTAINER_TYPE_INT8: ((int8_t*)cont->data)[ offset ] = (int8_t)stack[ sp2 ].i; break;
					case PIX_CONTAINER_TYPE_INT16: ((int16_t*)cont->data)[ offset ] = (int16_t)stack[ sp2 ].i; break;
					case PIX_CONTAINER_TYPE_INT32: ((int32_t*)cont->data)[ offset ] = (int32_t)stack[ sp2 ].i; break;
#ifdef PIX_INT64_ENABLED
					case PIX_CONTAINER_TYPE_INT64: ((int64_t*)cont->data)[ offset ] = (int64_t)stack[ sp2 ].i; break;
#endif
					case PIX_CONTAINER_TYPE_FLOAT32: ((float*)cont->data)[ offset ] = (float)stack[ sp2 ].i; break;
#ifdef PIX_FLOAT64_ENABLED
					case PIX_CONTAINER_TYPE_FLOAT64: ((double*)cont->data)[ offset ] = (double)stack[ sp2 ].i; break;
#endif
					default: break;
				    }
				}
				else 
				{
				    //Floating point value:
				    switch( cont->type )
				    {
					case PIX_CONTAINER_TYPE_INT8: ((int8_t*)cont->data)[ offset ] = (int8_t)stack[ sp2 ].f; break;
					case PIX_CONTAINER_TYPE_INT16: ((int16_t*)cont->data)[ offset ] = (int16_t)stack[ sp2 ].f; break;
					case PIX_CONTAINER_TYPE_INT32: ((int32_t*)cont->data)[ offset ] = (int32_t)stack[ sp2 ].f; break;
#ifdef PIX_INT64_ENABLED
					case PIX_CONTAINER_TYPE_INT64: ((int64_t*)cont->data)[ offset ] = (int64_t)stack[ sp2 ].f; break;
#endif
					case PIX_CONTAINER_TYPE_FLOAT32: ((float*)cont->data)[ offset ] = (float)stack[ sp2 ].f; break;
#ifdef PIX_FLOAT64_ENABLED
					case PIX_CONTAINER_TYPE_FLOAT64: ((double*)cont->data)[ offset ] = (double)stack[ sp2 ].f; break;
#endif
					default: break;
				    }
				}
			    }
			}
		    }
		    sp += 3;
		    break;
		case OPCODE_SAVE_TO_SMEM_2D:
		    {
			PIX_CID cnum;
	                cnum = (PIX_CID)stack[ PIX_CHECK_SP( sp + 3 ) ].i;
			if( (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
			{
			    pix_vm_container* cont = vm->c[ cnum ];
	                    PIX_INT offset;
			    if( stack[ PIX_CHECK_SP( sp + 2 ) ].t == 0 )
				offset = stack[ PIX_CHECK_SP( sp + 2 ) ].i;
			    else
				offset = (PIX_INT)stack[ PIX_CHECK_SP( sp + 2 ) ].f;
			    if( stack[ PIX_CHECK_SP( sp + 1 ) ].t == 0 )
				offset += stack[ PIX_CHECK_SP( sp + 1 ) ].i * cont->xsize;
			    else
				offset += (PIX_INT)stack[ PIX_CHECK_SP( sp + 1 ) ].f * cont->xsize;
			    if( (unsigned)offset < cont->size )
			    {
				PIX_SP sp2 = PIX_CHECK_SP( sp );
				if( stack[ sp2 ].t == 0 )
				{
				    //Integer value:
				    switch( cont->type )
				    {
					case PIX_CONTAINER_TYPE_INT8: ((int8_t*)cont->data)[ offset ] = (int8_t)stack[ sp2 ].i; break;
					case PIX_CONTAINER_TYPE_INT16: ((int16_t*)cont->data)[ offset ] = (int16_t)stack[ sp2 ].i; break;
					case PIX_CONTAINER_TYPE_INT32: ((int32_t*)cont->data)[ offset ] = (int32_t)stack[ sp2 ].i; break;
#ifdef PIX_INT64_ENABLED
					case PIX_CONTAINER_TYPE_INT64: ((int64_t*)cont->data)[ offset ] = (int64_t)stack[ sp2 ].i; break;
#endif
					case PIX_CONTAINER_TYPE_FLOAT32: ((float*)cont->data)[ offset ] = (float)stack[ sp2 ].i; break;
#ifdef PIX_FLOAT64_ENABLED
					case PIX_CONTAINER_TYPE_FLOAT64: ((double*)cont->data)[ offset ] = (double)stack[ sp2 ].i; break;
#endif
					default: break;
				    }
				}
				else 
				{
				    //Floating point value:
				    switch( cont->type )
				    {
					case PIX_CONTAINER_TYPE_INT8: ((int8_t*)cont->data)[ offset ] = (int8_t)stack[ sp2 ].f; break;
					case PIX_CONTAINER_TYPE_INT16: ((int16_t*)cont->data)[ offset ] = (int16_t)stack[ sp2 ].f; break;
					case PIX_CONTAINER_TYPE_INT32: ((int32_t*)cont->data)[ offset ] = (int32_t)stack[ sp2 ].f; break;
#ifdef PIX_INT64_ENABLED
					case PIX_CONTAINER_TYPE_INT64: ((int64_t*)cont->data)[ offset ] = (int64_t)stack[ sp2 ].f; break;
#endif
					case PIX_CONTAINER_TYPE_FLOAT32: ((float*)cont->data)[ offset ] = (float)stack[ sp2 ].f; break;
#ifdef PIX_FLOAT64_ENABLED
					case PIX_CONTAINER_TYPE_FLOAT64: ((double*)cont->data)[ offset ] = (double)stack[ sp2 ].f; break;
#endif
					default: break;
				    }
				}
			    }
			}
		    }
		    sp += 4;
		    break;
		case OPCODE_LOAD_FROM_MEM:
		    {
			sp++;
			PIX_CID cnum;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			cnum = (PIX_CID)stack[ sp2 ].i;
			if( (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
			{
			    pix_vm_container* cont = vm->c[ cnum ];
			    PIX_INT offset;
			    if( stack[ PIX_CHECK_SP( sp - 1 ) ].t == 0 )
				offset = stack[ PIX_CHECK_SP( sp - 1 ) ].i;
			    else
				offset = (PIX_INT)stack[ PIX_CHECK_SP( sp - 1 ) ].f;
			    if( (unsigned)offset < cont->size )
			    {
				switch( cont->type )
				{
				    case PIX_CONTAINER_TYPE_INT8: stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)( ((int8_t*)cont->data)[ offset ] ); break;
				    case PIX_CONTAINER_TYPE_INT16: stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)( ((int16_t*)cont->data)[ offset ] ); break;
				    case PIX_CONTAINER_TYPE_INT32: stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)( ((int32_t*)cont->data)[ offset ] ); break;
#ifdef PIX_INT64_ENABLED
				    case PIX_CONTAINER_TYPE_INT64: stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)( ((int64_t*)cont->data)[ offset ] ); break;
#endif
				    case PIX_CONTAINER_TYPE_FLOAT32: stack[ sp2 ].t = 1; stack[ sp2 ].f = (PIX_FLOAT)( ((float*)cont->data)[ offset ] ); break;
#ifdef PIX_FLOAT64_ENABLED
				    case PIX_CONTAINER_TYPE_FLOAT64: stack[ sp2 ].t = 1; stack[ sp2 ].f = (PIX_FLOAT)( ((double*)cont->data)[ offset ] ); break;
#endif
				    default: break;
				}
			    }
			    else
			    {
				stack[ sp2 ].t = 0;
				stack[ sp2 ].i = 0;
			    }
			}
			else
			{
			    stack[ sp2 ].t = 0;
			    stack[ sp2 ].i = 0;
			}
	   	    }
		    break;
		case OPCODE_LOAD_FROM_SMEM_2D:
		    {
			sp += 2;
			PIX_CID cnum;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			cnum = (PIX_CID)stack[ sp2 ].i;
			if( (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
			{
			    pix_vm_container* cont = vm->c[ cnum ];
			    PIX_INT offset;
			    if( stack[ PIX_CHECK_SP( sp - 1 ) ].t == 0 )
				offset = stack[ PIX_CHECK_SP( sp - 1 ) ].i;
			    else
				offset = (PIX_INT)stack[ PIX_CHECK_SP( sp - 1 ) ].f;
			    if( stack[ PIX_CHECK_SP( sp - 2 ) ].t == 0 )
				offset += stack[ PIX_CHECK_SP( sp - 2 ) ].i * cont->xsize;
			    else
				offset += (PIX_INT)stack[ PIX_CHECK_SP( sp - 2 ) ].f * cont->xsize;
			    if( (unsigned)offset < cont->size )
			    {
				switch( cont->type )
				{
				    case PIX_CONTAINER_TYPE_INT8: stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)( ((int8_t*)cont->data)[ offset ] ); break;
				    case PIX_CONTAINER_TYPE_INT16: stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)( ((int16_t*)cont->data)[ offset ] ); break;
				    case PIX_CONTAINER_TYPE_INT32: stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)( ((int32_t*)cont->data)[ offset ] ); break;
#ifdef PIX_INT64_ENABLED
				    case PIX_CONTAINER_TYPE_INT64: stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)( ((int64_t*)cont->data)[ offset ] ); break;
#endif
				    case PIX_CONTAINER_TYPE_FLOAT32: stack[ sp2 ].t = 1; stack[ sp2 ].f = (PIX_FLOAT)( ((float*)cont->data)[ offset ] ); break;
#ifdef PIX_FLOAT64_ENABLED
				    case PIX_CONTAINER_TYPE_FLOAT64: stack[ sp2 ].t = 1; stack[ sp2 ].f = (PIX_FLOAT)( ((double*)cont->data)[ offset ] ); break;
#endif
				    default: break;
				}
			    }
			    else
			    {
				stack[ sp2 ].t = 0;
				stack[ sp2 ].i = 0;
			    }
			}
			else
			{
			    stack[ sp2 ].t = 0;
			    stack[ sp2 ].i = 0;
			}
	   	    }
		    break;

		case OPCODE_SAVE_TO_STACKFRAME_i:
		    {
			val = (signed)c >> PIX_OPCODE_BITS;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			PIX_SP fp2 = PIX_CHECK_SP( fp + (signed)val );
			stack[ fp2 ] = stack[ sp2 ];
			sp++;
		    }
		    break;
		case OPCODE_LOAD_FROM_STACKFRAME_i:
		    {
			val = (signed)c >> PIX_OPCODE_BITS;
			sp--;
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			PIX_SP fp2 = PIX_CHECK_SP( fp + (signed)val );
			stack[ sp2 ] = stack[ fp2 ];
		    }
		    break;

		case OPCODE_SUB:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i -= stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].t = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i - stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].f = stack[ sp2 ].f - (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].f -= stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_ADD:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i += stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].t = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i + stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].f = stack[ sp2 ].f + (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].f += stack[ sp3 ].f; break;
			}
		        sp++;
		    }
		    break;
		case OPCODE_MUL:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
		        {
		    	    case 0: /*II*/ stack[ sp2 ].i *= stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].t = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i * stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].f = stack[ sp2 ].f * (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].f *= stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_IDIV:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			pix_stack_val* v2 = &stack[ sp2 ];
			pix_stack_val* v3 = &stack[ sp3 ];
#ifdef ARCH_ARM64
			//28 feb 2024:
			//...здесь отметились шаманы с бубнами...
			//arm64 clang (-O3) в исходном алгоритме (switch case) вместо одной ASM операции деления для case 0 может добавить JMP, чтобы совместить несколько case-ов,
			//и делает он это не всегда, а в зависимости от выравнивания case OPCODE_IDIV, т.е. в зависимости от предшествующего кода:
			//можно что-нибудь незначительное поменять и получить двукратное (!!) замедление OPCODE_IDIV...
			//Поэтому пришлось переписать этот кусок кода (в таком виде он, вроде бы, работает даже быстрее):
			PIX_INT i2 = v2->i;
			PIX_INT i3 = v3->i;
			if( stack[ sp2 ].t ) i2 = v2->f;
			if( stack[ sp3 ].t ) i3 = v3->f;
			v2->i = i2 / i3;
			v2->t = 0;
#else
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*IDIV II*/ v2->i /= v3->i; break;
			    case 1: /*IDIV IF*/ v2->i = v2->i / (PIX_INT)v3->f; break;
			    case 2: /*IDIV FI*/ v2->t = 0; v2->i = (PIX_INT)v2->f / v3->i; break;
			    case 3: /*IDIV FF*/ v2->t = 0; v2->i = (PIX_INT)v2->f / (PIX_INT)v3->f; break;
			}
#endif
			sp++;
		    }
		    break;
		case OPCODE_DIV:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
		        {
		    	    case 0: /*II*/ stack[ sp2 ].t = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i / (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].t = 1; stack[ sp2 ].f = (PIX_FLOAT)stack[ sp2 ].i / stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].f = stack[ sp2 ].f / (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].f /= stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_MOD:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i %= stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i % (PIX_INT)stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f % stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f % (PIX_INT)stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_AND:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i &= stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i & (PIX_INT)stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f & stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f & (PIX_INT)stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_OR:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i |= stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i | (PIX_INT)stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f | stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f | (PIX_INT)stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_XOR:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i ^= stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i ^ (PIX_INT)stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f ^ stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f ^ (PIX_INT)stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_ANDAND:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i && stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i && stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f && stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f && stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_OROR:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
		        {
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i || stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i || stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f || stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f || stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_EQ:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i == stack[ sp3 ].i; break;
		    	    case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i == stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f == (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f == stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_NEQ:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i != stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i != stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f != (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f != stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_LESS:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i < stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i < stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f < (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f < stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_LEQ:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i <= stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i <= stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f <= (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f <= stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_GREATER:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i > stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i > stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f > (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f > stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_GEQ:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i >= stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = (PIX_FLOAT)stack[ sp2 ].i >= stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f >= (PIX_FLOAT)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = stack[ sp2 ].f >= stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;
		case OPCODE_LSHIFT:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i << (unsigned)stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i << (unsigned)(PIX_INT)stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f << (unsigned)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f << (unsigned)(PIX_INT)stack[ sp3 ].f; break;
		        }
			sp++;
		    }
		    break;
		case OPCODE_RSHIFT:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp + 1 );
			PIX_SP sp3 = PIX_CHECK_SP( sp );
			switch( ( stack[ sp2 ].t << 1 ) + stack[ sp3 ].t )
			{
			    case 0: /*II*/ stack[ sp2 ].i = stack[ sp2 ].i >> (unsigned)stack[ sp3 ].i; break;
			    case 1: /*IF*/ stack[ sp2 ].i = stack[ sp2 ].i >> (unsigned)(PIX_INT)stack[ sp3 ].f; break;
			    case 2: /*FI*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f >> (unsigned)stack[ sp3 ].i; break;
			    case 3: /*FF*/ stack[ sp2 ].t = 0; stack[ sp2 ].i = (PIX_INT)stack[ sp2 ].f >> (unsigned)(PIX_INT)stack[ sp3 ].f; break;
			}
			sp++;
		    }
		    break;

		case OPCODE_NEG:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			if( stack[ sp2 ].t == 0 )
			    stack[ sp2 ].i = -stack[ sp2 ].i;
			else
			    stack[ sp2 ].f = -stack[ sp2 ].f;
		    }
		    break;
		case OPCODE_LOGICAL_NOT:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			if( stack[ sp2 ].t == 0 )
			    stack[ sp2 ].i = !(stack[ sp2 ].i);
			else
			    stack[ sp2 ].f = !((PIX_INT)stack[ sp2 ].f);
		    }
		    break;
		case OPCODE_BITWISE_NOT:
		    {
			PIX_SP sp2 = PIX_CHECK_SP( sp );
			if( stack[ sp2 ].t == 0 )
			    stack[ sp2 ].i = ~stack[ sp2 ].i;
			else
			    stack[ sp2 ].f = ~(PIX_INT)stack[ sp2 ].f;
		    }
		    break;

		case OPCODE_CALL_BUILTIN_FN:
		    {
			int pars_num = c >> ( PIX_OPCODE_BITS + PIX_FN_BITS );
			int fn_num = ( c >> PIX_OPCODE_BITS ) & PIX_FN_MASK;
			g_pix_fns[ fn_num ]( fn_num, pars_num, sp, th, vm );
		        sp += pars_num - 1;
		    }
		    break;
		case OPCODE_CALL_BUILTIN_FN_VOID:
		    {
			int pars_num = c >> ( PIX_OPCODE_BITS + PIX_FN_BITS );
			int fn_num = ( c >> PIX_OPCODE_BITS ) & PIX_FN_MASK;
			g_pix_fns[ fn_num ]( fn_num, pars_num, sp, th, vm );
		        sp += pars_num;
		    }
		    break;
		case OPCODE_CALL_i:
		    {
			PIX_ADDR addr;
		        if( stack[ PIX_CHECK_SP( sp ) ].t == 0 )
			    addr = stack[ PIX_CHECK_SP( sp ) ].i;
		        else
			    addr = stack[ PIX_CHECK_SP( sp ) ].f;
			if( IS_ADDRESS_CORRECT( addr ) )
                        {
		    	    //Set new PC:
			    PIX_INT old_pc = (PIX_INT)pc;
			    pc = addr & PIX_INT_ADDRESS_MASK;
		    	    //Save number of parameters:
		    	    stack[ PIX_CHECK_SP( sp ) ].t = 0;
		    	    stack[ PIX_CHECK_SP( sp ) ].i = (PIX_INT)( c >> PIX_OPCODE_BITS );
			    //Save FP:
    			    sp--;
		    	    //Dont touch the type of stack item, because user can't access this item directly
		    	    stack[ PIX_CHECK_SP( sp ) ].i = (PIX_INT)fp;
			    //Save PC:
			    sp--;
		    	    stack[ PIX_CHECK_SP( sp ) ].i = old_pc;
		    	    //Set new FP:
		    	    fp = sp + 2;
		    	}
		    	else
		    	{
		    	    int pars = (int)( c >> PIX_OPCODE_BITS );
			    PIX_VM_LOG( "Pixilang VM Error. %u: call function i(%d). Address %u is incorrect\n", (unsigned int)pc, pars, (unsigned int)addr );
			    sp += pars - 1;
			    stack[ PIX_CHECK_SP( sp ) ].t = 0;
                            stack[ PIX_CHECK_SP( sp ) ].i = 0;
		    	}
		    }
		    break;
		case OPCODE_INC_SP_i:
		    sp += (signed)c >> PIX_OPCODE_BITS;
		    break;
		case OPCODE_RET_i:
		    {
			PIX_INT pars_num;
			if( stack[ PIX_CHECK_SP( fp ) ].t == 0 )
			    pars_num = stack[ PIX_CHECK_SP( fp ) ].i;
			else
			    pars_num = (PIX_INT)stack[ PIX_CHECK_SP( fp ) ].f;
			sp = fp + pars_num;
			stack[ PIX_CHECK_SP( fp + pars_num ) ].t = 0;
			stack[ PIX_CHECK_SP( fp + pars_num ) ].i = (PIX_INT)( (signed)c >> PIX_OPCODE_BITS );
			pc = stack[ PIX_CHECK_SP( fp - 2 ) ].i;
			fp = stack[ PIX_CHECK_SP( fp - 1 ) ].i;
		    }
		    break;
		case OPCODE_RET_I:
		    {
			LOAD_INT( val_i );
			PIX_INT pars_num;
			if( stack[ PIX_CHECK_SP( fp ) ].t == 0 )
			    pars_num = stack[ PIX_CHECK_SP( fp ) ].i;
			else
			    pars_num = (PIX_INT)stack[ PIX_CHECK_SP( fp ) ].f;
			sp = fp + pars_num;
			stack[ PIX_CHECK_SP( fp + pars_num ) ].t = 0;
			stack[ PIX_CHECK_SP( fp + pars_num ) ].i = val_i;
			pc = stack[ PIX_CHECK_SP( fp - 2 ) ].i;
			fp = stack[ PIX_CHECK_SP( fp - 1 ) ].i;
		    }
		    break;
		case OPCODE_RET:
		    {
			PIX_INT pars_num;
			if( stack[ PIX_CHECK_SP( fp ) ].t == 0 )
			    pars_num = stack[ PIX_CHECK_SP( fp ) ].i;
			else
			    pars_num = (PIX_INT)stack[ PIX_CHECK_SP( fp ) ].f;
			stack[ PIX_CHECK_SP( fp + pars_num ) ] = stack[ PIX_CHECK_SP( sp ) ];
			sp = fp + pars_num;
			pc = stack[ PIX_CHECK_SP( fp - 2 ) ].i;
			fp = stack[ PIX_CHECK_SP( fp - 1 ) ].i;
		    }
		    break;

		case NUMBER_OF_OPCODES:
		    break;
	    }
	}
	DPRINT( "Thread %d halted. PC: %d; SP: %d; FP: %d.\n", thread_num, (int)pc, (int)sp, (int)fp );
	//Correct values after return from the main function:
	//PC = 1 (next instruction after HALT);
	//SP = PIX_VM_STACK_SIZE - 1 (points to the returned value);
	//FP = PIX_VM_STACK_SIZE - 1;
	th->pc = pc;
	th->sp = sp;
	th->fp = fp;
    }

    return thread_num;
}

int pix_vm_save_code( const char* name, pix_vm* vm )
{
    int rv = 0;

    sfs_file f = sfs_open( name, "wb" );
    if( f )
    {
	const char* signature = "PIXICODE";
	uint32_t version = PIXILANG_VERSION;
	sfs_write( (void*)signature, 1, 8, f );
	sfs_write( &version, 1, 4, f );
	for( int i = 0; i < 4; i++ ) sfs_putc( 0, f );

	sfs_write( &vm->code_size, 1, 4, f );
	sfs_write( vm->code, sizeof( PIX_OPCODE ), vm->code_size, f );
	sfs_write( &vm->code_ptr, 1, 4, f );
	sfs_write( &vm->halt_addr, 1, 4, f );

	sfs_write( &vm->vars_num, 1, 4, f );
	for( int i = 0; i < (int)vm->vars_num; i++ )
	{
	    sfs_write( &vm->vars[ i ].t, 1, 1, f );
	    sfs_write( &vm->vars[ i ].v, 1, sizeof( PIX_VAL ), f );
	    char* var_name = vm->var_names[ i ];
	    int name_len = 0;
	    if( var_name ) name_len = smem_strlen( var_name );
	    sfs_write( &name_len, 1, 4, f );
	    if( name_len > 0 )
		sfs_write( var_name, 1, name_len, f );
	}
	
	sfs_write( &vm->screen, 1, 4, f );
	
	for( int i = 0; i < vm->fonts_num; i++ )
	{
	    pix_vm_font* font = &vm->fonts[ i ];
	    sfs_write( &font->font, 1, sizeof( PIX_CID ), f );
	    sfs_write( &font->first, 1, 4, f );
	    sfs_write( &font->last, 1, 4, f );
	    uint32_t v;
	    v = font->char_xsize; sfs_write( &v, 1, 4, f );
	    v = font->char_ysize; sfs_write( &v, 1, 4, f );
	    v = font->char_xsize2; sfs_write( &v, 1, 4, f );
	    v = font->char_ysize2; sfs_write( &v, 1, 4, f );
	    v = font->grid_xoffset; sfs_write( &v, 1, 4, f );
	    v = font->grid_yoffset; sfs_write( &v, 1, 4, f );
	    v = font->grid_cell_xsize; sfs_write( &v, 1, 4, f );
	    v = font->grid_cell_ysize; sfs_write( &v, 1, 4, f );
	    v = font->xchars; sfs_write( &v, 1, 4, f );
	    v = font->ychars; sfs_write( &v, 1, 4, f );
	}
	
	sfs_write( &vm->event, 1, sizeof( PIX_CID ), f );
	
	sfs_write( &vm->current_path, 1, sizeof( PIX_CID ), f );
	sfs_write( &vm->user_path, 1, sizeof( PIX_CID ), f );
	sfs_write( &vm->temp_path, 1, sizeof( PIX_CID ), f );
	sfs_write( &vm->os_name, 1, sizeof( PIX_CID ), f );
	sfs_write( &vm->arch_name, 1, sizeof( PIX_CID ), f );
	sfs_write( &vm->lang_name, 1, sizeof( PIX_CID ), f );
	
	for( PIX_CID i = 0; i < (PIX_CID)vm->c_num; i++ )
	{
	    pix_vm_container* c = vm->c[ i ];
	    if( c )
	    {
		sfs_write( &i, 1, sizeof( PIX_CID ), f );
		sfs_write( &c->type, 1, 1, f );
		sfs_write( &c->flags, 1, 4, f );
		sfs_write( &c->xsize, 1, 4, f );
		sfs_write( &c->ysize, 1, 4, f );
		uint8_t key_color[ 3 ];
		key_color[ 0 ] = red( c->key );
		key_color[ 1 ] = green( c->key );
		key_color[ 2 ] = blue( c->key );
		sfs_write( &key_color, 1, 3, f );
		sfs_write( &c->alpha, 1, sizeof( PIX_CID ), f );
		sfs_write( c->data, g_pix_container_type_sizes[ c->type ], c->xsize * c->ysize, f );
		/*printf( "%d: %d x %d\n", i, c->xsize, c->ysize );
		for( size_t p = 0; p < c->xsize * c->ysize * g_pix_container_type_sizes[ c->type ]; p++ ) printf( "%c", ((char*)c->data)[ p ] );
		printf("\n\n");*/
	    }
	}
	
	sfs_close( f );
    }
    else
    {
	PIX_VM_LOG( "Can't open %s for writing.\n", name );
	rv = 1;
    }
    
    return rv;
}

void pix_vm_set_systeminfo_containers( pix_vm* vm )
{
    //System name (container):
    {
        size_t slen = smem_strlen( OS_NAME );
        if( vm->os_name == -1 )
    	    vm->os_name = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->os_name, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->os_name, pix_vm_get_container_flags( vm->os_name, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        smem_copy( pix_vm_get_container_data( vm->os_name, vm ), OS_NAME, slen );
    }
    //Architecture name (container):
    {
        const char* name = ARCH_NAME;
        size_t slen = smem_strlen( name );
        if( vm->arch_name == -1 )
    	    vm->arch_name = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->arch_name, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->arch_name, pix_vm_get_container_flags( vm->arch_name, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        smem_copy( pix_vm_get_container_data( vm->arch_name, vm ), name, slen );
    }
    //Language name (container):
    {
        const char* name = slocale_get_lang();
        size_t slen = smem_strlen( name );
        if( vm->lang_name == -1 )
    	    vm->lang_name = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->lang_name, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->lang_name, pix_vm_get_container_flags( vm->lang_name, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        smem_copy( pix_vm_get_container_data( vm->lang_name, vm ), name, slen );
    }
    //Current working path (container):
    {
        char* p = sfs_make_filename( vm->wm->sd, vm->base_path, true );
        size_t slen = smem_strlen( p );
        if( slen == 0 )
        {
    	    slen = 1;
    	    smem_free( p );
    	    p = (char*)smem_new( 1 );
    	    p[ 0 ] = 0;
        }
        if( vm->current_path == -1 )
    	    vm->current_path = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->current_path, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->current_path, pix_vm_get_container_flags( vm->current_path, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        smem_copy( pix_vm_get_container_data( vm->current_path, vm ), p, slen );
        smem_free( p );
    }
    //User data path (container):
    {
        char* p = sfs_make_filename( vm->wm->sd, sfs_get_conf_path(), true );
        size_t slen = smem_strlen( p );
        if( slen == 0 )
        {
    	    slen = 1;
    	    smem_free( p );
    	    p = (char*)smem_new( 1 );
    	    p[ 0 ] = 0;
        }
        if( vm->user_path == -1 )
    	    vm->user_path = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
	    pix_vm_resize_container( vm->user_path, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->user_path, pix_vm_get_container_flags( vm->user_path, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        smem_copy( pix_vm_get_container_data( vm->user_path, vm ), p, slen );
        smem_free( p );
    }
    //Temp path (container):
    {
        char* p = sfs_make_filename( vm->wm->sd, sfs_get_temp_path(), true );
        size_t slen = smem_strlen( p );
        if( slen == 0 )
        {
    	    slen = 1;
    	    smem_free( p );
    	    p = (char*)smem_new( 1 );
    	    p[ 0 ] = 0;
        }
        if( vm->temp_path == -1 )
    	    vm->temp_path = pix_vm_new_container( -1, slen, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	else
    	    pix_vm_resize_container( vm->temp_path, slen, 1, -1, 0, vm );
    	pix_vm_set_container_flags( vm->temp_path, pix_vm_get_container_flags( vm->temp_path, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
        smem_copy( pix_vm_get_container_data( vm->temp_path, vm ), p, slen );
        smem_free( p );
    }
}

void pix_vm_set_pixiinfo( pix_vm* vm )
{
    PIX_INT info = 0;
#ifdef SUNDOG_MODULE
    info |= PIX_INFO_MODULE;
#endif
#ifdef MULTITOUCH
    info |= PIX_INFO_MULTITOUCH;
#endif
    if( vm->wm->flags & WIN_INIT_FLAG_TOUCHCONTROL )
	info |= PIX_INFO_TOUCHCONTROL;
    if( vm->wm->flags & WIN_INIT_FLAG_NOWINDOW )
	info |= PIX_INFO_NOWINDOW;
#ifndef NOMIDI
    info |= PIX_INFO_MIDIIN;
    info |= PIX_INFO_MIDIOUT;
#ifdef OS_IOS
    info |= PIX_INFO_MIDIOPTIONS;
#endif
#endif
    if( g_webserver_available )
	info |= PIX_INFO_WEBSERVER;
#ifdef CAN_COPYPASTE
    info |= PIX_INFO_CLIPBOARD;
#endif
#ifdef CAN_COPYPASTE_AV
    info |= PIX_INFO_CLIPBOARD_AV;
#endif
#ifdef CAN_SEND_TO_GALLERY
    info |= PIX_INFO_GALLERY;
#endif
#ifdef CAN_EXPORT
    info |= PIX_INFO_EXPORT;
#endif
#ifdef CAN_EXPORT2
    info |= PIX_INFO_EXPORT2;
#endif
#ifdef CAN_IMPORT
    info |= PIX_INFO_IMPORT;
#endif
#ifdef CAN_SEND_TO_EMAIL
    info |= PIX_INFO_EMAIL;
#endif
    if( video_capture_supported( vm->wm ) )
	info |= PIX_INFO_VIDEOCAPTURE;
#ifdef OPENGL
    if( !vm->wm->fb )
	info |= PIX_INFO_OPENGL;
#endif
    vm->vars[ PIX_GVAR_PIXILANG_INFO ].t = 0;
    vm->vars[ PIX_GVAR_PIXILANG_INFO ].i = info;
}

int pix_vm_load_code( const char* name, char* base_path, pix_vm* vm )
{
    int rv = 0;

    sfs_file f = sfs_open( name, "rb" );
    if( f )
    {
	char sign[ 16 ];
	sfs_read( sign, 1, 16, f );
	if( smem_cmp( sign, "PIXICODE", 8 ) )
	{
	    PIX_VM_LOG( "pix_vm_load_code(): wrong signature!\n" );
	    rv = 1;
	    goto load_code_end;
	}
	uint32_t file_version = 0;
	memmove( &file_version, &sign[ 8 ], 4 );
	uint32_t version = PIXILANG_VERSION;
	if( file_version != version )
	{
	    bool version_err = false;
	    if( ( version & 0xFFFFFF00 ) != ( file_version & 0xFFFFFF00 ) ) //X.Y.Z.* != X.Y.Z.* (ex: 3.8.3 != 3.8.2)
	    {
		version_err = true;
	    }
	    if( version_err )
	    {
		PIX_VM_LOG( "pix_vm_load_code(): can't load pixicode v%d.%d.%d.%d in Pixilang v%d.%d.%d.%d\n", 
		    ( file_version >> 24 ) & 255,
		    ( file_version >> 16 ) & 255,
		    ( file_version >> 8 ) & 255,
		    ( file_version >> 0 ) & 255,
		    ( version >> 24 ) & 255,
		    ( version >> 16 ) & 255,
		    ( version >> 8 ) & 255,
		    ( version >> 0 ) & 255 );
		rv = 2;
		goto load_code_end;
	    }
	}

	sfs_read( &vm->code_size, 1, 4, f );
	vm->code = (PIX_OPCODE*)smem_new( sizeof( PIX_OPCODE ) * vm->code_size );
	sfs_read( vm->code, sizeof( PIX_OPCODE ), vm->code_size, f );
	sfs_read( &vm->code_ptr, 1, 4, f );
	sfs_read( &vm->halt_addr, 1, 4, f );

	sfs_read( &vm->vars_num, 1, 4, f );
	pix_vm_resize_variables( vm );
	for( int i = 0; i < (int)vm->vars_num; i++ )
    	{
	    sfs_read( &vm->vars[ i ].t, 1, 1, f );
	    sfs_read( &vm->vars[ i ].v, 1, sizeof( PIX_VAL ), f );
    	    int name_len;
    	    sfs_read( &name_len, 1, 4, f );
    	    if( name_len > 0 )
    	    {
    		char* var_name = (char*)smem_new( name_len + 1 );
    		sfs_read( var_name, 1, name_len, f );
    		var_name[ name_len ] = 0;
    		vm->var_names[ i ] = var_name;
    	    }
	}
	
	sfs_read( &vm->screen, 1, 4, f );
	
	for( int i = 0; i < vm->fonts_num; i++ )
	{
	    pix_vm_font* font = &vm->fonts[ i ];
	    sfs_read( &font->font, 1, sizeof( PIX_CID ), f );
	    sfs_read( &font->first, 1, 4, f );
	    sfs_read( &font->last, 1, 4, f );
	    uint32_t v;
	    sfs_read( &v, 1, 4, f ); font->char_xsize = v;
	    sfs_read( &v, 1, 4, f ); font->char_ysize = v;
	    sfs_read( &v, 1, 4, f ); font->char_xsize2 = v;
	    sfs_read( &v, 1, 4, f ); font->char_ysize2 = v;
	    sfs_read( &v, 1, 4, f ); font->grid_xoffset = v;
	    sfs_read( &v, 1, 4, f ); font->grid_yoffset = v;
	    sfs_read( &v, 1, 4, f ); font->grid_cell_xsize = v;
	    sfs_read( &v, 1, 4, f ); font->grid_cell_ysize = v;
	    sfs_read( &v, 1, 4, f ); font->xchars = v;
	    sfs_read( &v, 1, 4, f ); font->ychars = v;
	}
	
	sfs_read( &vm->event, 1, sizeof( PIX_CID ), f );

	sfs_read( &vm->current_path, 1, sizeof( PIX_CID ), f );
	sfs_read( &vm->user_path, 1, sizeof( PIX_CID ), f );
	sfs_read( &vm->temp_path, 1, sizeof( PIX_CID ), f );
	sfs_read( &vm->os_name, 1, sizeof( PIX_CID ), f );
	sfs_read( &vm->arch_name, 1, sizeof( PIX_CID ), f );
	sfs_read( &vm->lang_name, 1, sizeof( PIX_CID ), f );

	while( 1 )
	{
	    PIX_CID cnum;
	    if( sfs_read( &cnum, 1, sizeof( PIX_CID ), f ) != 4 ) break;
	    int type = 0;
	    uint flags = 0;
	    int xsize = 0;
	    int ysize = 0;
	    uint8_t key_color[ 3 ];
	    COLOR color;
	    PIX_CID alpha = 0;
	    sfs_read( &type, 1, 1, f );
	    sfs_read( &flags, 1, 4, f );
	    sfs_read( &xsize, 1, 4, f );
	    sfs_read( &ysize, 1, 4, f );
	    sfs_read( &key_color, 1, 3, f );
	    color = get_color( key_color[ 0 ], key_color[ 1 ], key_color[ 2 ] );
	    sfs_read( &alpha, 1, sizeof( PIX_CID ), f );
	    size_t size = g_pix_container_type_sizes[ type ] * xsize * ysize;
	    void* data = smem_new( size );
	    sfs_read( data, 1, size, f ); 
	    pix_vm_new_container( cnum, xsize, ysize, type, data, vm );
	    pix_vm_container* c = vm->c[ cnum ];
	    if( c )
	    {
		pix_vm_set_container_flags( cnum, flags, vm );
		pix_vm_set_container_key_color( cnum, color, vm );
		pix_vm_set_container_alpha( cnum, alpha, vm );
	    }
	}

	//Set base VM path:
	smem_free( vm->base_path );
	vm->base_path = (char*)smem_new( smem_strlen( base_path ) + 1 );
	vm->base_path[ 0 ] = 0;
	smem_strcat_resize( vm->base_path, base_path );

        //Set system info containers:
	pix_vm_set_systeminfo_containers( vm );

	//Set Pixilang VM info (features/modes):
	pix_vm_set_pixiinfo( vm );

    }
    else
    {
	PIX_VM_LOG( "pix_vm_load_code(): can't open %s for reading.\n", name );
	rv = 3;
    }

load_code_end:

    sfs_close( f );

    return rv;
}
