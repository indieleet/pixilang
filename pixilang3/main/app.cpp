/*
    app.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2023 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"

#ifndef PIX_NOSUNVOX
    #include "sunvox_engine.h"
#endif

enum
{
    pix_status_none = 0,
    pix_status_loading,
    pix_status_working,
    pix_status_disabled
};

#ifdef OS_IOS
    #include "main/ios/sundog_bridge.h"
    extern char* g_apple_resources_path;
    bool g_pix_save_code_and_exit = 0;
#else
    bool g_pix_save_code_and_exit = 1;
#endif

#ifdef OS_ANDROID
    #ifdef FREE_VERSION
	#include "main/android/android_resources.hpp"
    #else
	#include "../../misc/android_resources.hpp"
    #endif
    #include "main/android/sundog_bridge.h"
#endif

struct pixilang_window_data
{
    WINDOWPTR this_window;
    pix_vm* vm; //Pixilang virtual machine
    int status;
    sdwm_image* screen_image;
    int timer;
    bool sundog_ui_request_handling;
    
    char* prog_name;
    bool prog_name_alloc;
    bool no_program_selection_dialog;
    int open_count;
    bool save_code_request;
};

enum
{
    STR_PIX_UI_PREFS,
    STR_PIX_UI_COMPILE,
    STR_PIX_UI_SELECT,
    STR_PIX_UI_MENU,
    STR_PIX_UI_MENU_NAME,
    STR_PIX_UI_LOADING,
};

const char* pix_ui_get_string( int str_id )
{
    const char* str = 0;
    const char* lang = slocale_get_lang();
    while( 1 )
    {
        if( smem_strstr( lang, "ru_" ) )
        {
            switch( str_id )
            {
                case STR_PIX_UI_PREFS: str = "Настр."; break;
    		case STR_PIX_UI_COMPILE: str = "Компил."; break;
    		case STR_PIX_UI_SELECT: str = "Укажите pixi-программу для запуска"; break;
    		case STR_PIX_UI_MENU: str = "Выход"; break;
    		case STR_PIX_UI_MENU_NAME: str = "Меню Pixilang"; break;
    		case STR_PIX_UI_LOADING: str = "Загрузка..."; break;
            }
            if( str ) break;
        }
        //Default:
        switch( str_id )
        {
    	    case STR_PIX_UI_PREFS: str = "Prefs"; break;
    	    case STR_PIX_UI_COMPILE: str = "Compile"; break;
    	    case STR_PIX_UI_SELECT: str = "Select a program"; break;
    	    case STR_PIX_UI_MENU: str = "Exit"; break;
    	    case STR_PIX_UI_MENU_NAME: str = "Pixilang Menu"; break;
    	    case STR_PIX_UI_LOADING: str = "Loading..."; break;
        }
        break;
    }
    return str;
}

void pix_vm_draw_screen( WINDOWPTR win, bool draw_changes )
{
    pixilang_window_data* data = (pixilang_window_data*)win->data;
    window_manager* wm = win->wm;
    pix_vm* vm = data->vm;
    if( !vm ) return;

    win_draw_lock( win, wm );

    vm->screen_redraw_counter++;

    while( 1 )
    {
	if( data->sundog_ui_request_handling )
	{
	    win_draw_frect( win, 0, 0, win->xsize, win->ysize, win->color, wm );
	    break;
	}

        vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i = win->xsize / vm->pixel_size;
	vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i = win->ysize / vm->pixel_size;
	vm->vars[ PIX_GVAR_PPI ].i = wm->screen_ppi / vm->pixel_size;
#ifdef SCREEN_SAFE_AREA_SUPPORTED
	if( win->xsize == wm->screen_xsize && win->ysize == wm->screen_ysize )
	{
	    //Fullscreen mode:
	    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_X ].i = wm->screen_safe_area.x;
	    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_Y ].i = wm->screen_safe_area.y;
	    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_W ].i = wm->screen_safe_area.w;
	    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_H ].i = wm->screen_safe_area.h;
	}
#endif

        int x, y;
        int sxsize, sysize;
	int screen_change_x;
        int screen_change_y;
        int screen_change_xsize;
        int screen_change_ysize;
        pix_vm_container* c = 0;
        bool ready_for_answer = false;
	bool new_data_available = false;
        bool screen_image_can_be_redrawn = false;
#ifdef OPENGL
	screen_image_can_be_redrawn = true;
#endif

#ifdef OPENGL
        if( vm->gl_callback != -1 && wm->gl_initialized )
	{
	    //OpenGL graphics:

	    bool gl_draw = false;
    	    if( vm->gl_callback != -1 ) //Double check
	    {
		ready_for_answer = true;
		gl_draw = true;

		gl_program_reset( wm );
		smem_copy( vm->gl_wm_transform, wm->gl_projection_matrix, sizeof( vm->gl_wm_transform ) );
    		matrix_4x4_translate( (int)( vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i * vm->pixel_size / 2 + win->screen_x ), (int)( vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i * vm->pixel_size / 2 + win->screen_y ), 0, vm->gl_wm_transform );
    		//matrix_4x4_translate( (float)vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i * vm->pixel_size / 2 + win->screen_x, (float)vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i * vm->pixel_size / 2 + win->screen_y, 0, vm->gl_wm_transform );
		if( vm->pixel_size != 1 )
    		    matrix_4x4_scale( vm->pixel_size, vm->pixel_size, 1, vm->gl_wm_transform );
    		pix_vm_gl_program_reset( vm );

    		pix_vm_function fun;
    		pix_stack_val pp[ 1 ];
		fun.p = pp;
    		fun.addr = vm->gl_callback;
    		fun.p[ 0 ] = vm->gl_userdata;
    		fun.p_num = 1;
    		pix_vm_run( PIX_VM_THREADS - 2, 0, &fun, PIX_VM_CALL_FUNCTION, vm );

    		pix_vm_gl_program_reset( vm );

		//In future this code may be replaced by the viewport change,
		//OR you can remove it, if you need OpenGL content larger than win rectangle (for shadow, glow, etc.):
		int left = win->screen_x;
		int right = win->screen_x + win->xsize;
		int top = win->screen_y;
		int bottom = win->screen_y + win->ysize;
		COLOR c = win->color;
		if( left > 0 ) wm->device_draw_frect( 0, 0, left, wm->screen_ysize, c, wm );
		if( right < wm->screen_xsize ) wm->device_draw_frect( right, 0, wm->screen_xsize - right, wm->screen_ysize, c, wm );
		if( top > 0 ) wm->device_draw_frect( left, 0, right, top, c, wm );
		if( bottom < wm->screen_ysize ) wm->device_draw_frect( left, bottom, right, wm->screen_ysize - bottom, c, wm );
	    }

	    if( gl_draw )
	    {
		screen_changed( wm );
		goto skip_frame_drawing;
    	    }
	}
#endif

        //Check for changes:
        if( vm->screen_redraw_request != vm->screen_redraw_answer )
        {
    	    ready_for_answer = 1;
	    if( (unsigned)vm->screen < (unsigned)vm->c_num && vm->c[ vm->screen ] )
    	    {
	        c = vm->c[ vm->screen ];
		screen_change_x = vm->screen_change_x;
		screen_change_y = vm->screen_change_y;
		screen_change_xsize = vm->screen_change_xsize;
		screen_change_ysize = vm->screen_change_ysize;
	    }
	}
	if( c && c->data )
	{
	    new_data_available = true;
	    bool update = false;
	    if( data->screen_image )
	    {
		if( c->xsize == data->screen_image->xsize && c->ysize == data->screen_image->ysize && c->data == data->screen_image->data )
		    update = true;
	    }
	    if( update )
	    {
		update_image( data->screen_image );
	    }
	    else
	    {
		remove_image( data->screen_image );
		data->screen_image = new_image( c->xsize, c->ysize, c->data, c->xsize, c->ysize, IMAGE_NATIVE_RGB | IMAGE_STATIC_SOURCE, wm );
	    }
	    //You can redraw this image later only if:
	    // 1) OpenGL available (GPU texture created);
	    //   or
	    // 2) c && c->data, so the new image is created at the moment - image data pointer is valid.
	    screen_image_can_be_redrawn = true;
	}

	//Empty data control:
	if( draw_changes )
	{
	    if( !new_data_available )
	    {
		//Nothing to draw:
		goto skip_frame_drawing;
	    }
	}
	else
	{
	    if( data->screen_image == 0 )
	    {
		//Nothing to draw:
		win_draw_frect( win, 0, 0, win->xsize, win->ysize, win->color, wm );
		goto skip_frame_drawing;
	    }
	}

	sxsize = data->screen_image->xsize * vm->pixel_size;
	sysize = data->screen_image->ysize * vm->pixel_size;
        x = ( win->xsize - sxsize ) / 2;
        y = ( win->ysize - sysize ) / 2;

	if( draw_changes )
	{
	    //Draw changed screen region only:

	    if( vm->pixel_size == 1 )
	    {
	        win_draw_image_ext( 
	    	    win,
    		    x + screen_change_x, y + screen_change_y, //dest XY
		    screen_change_xsize, screen_change_ysize, //dest size
		    screen_change_x, screen_change_y, //src XY
		    data->screen_image, 
		    wm );
	    }
	    else
	    {
	        wbd_lock( win );
	        sdwm_image_scaled img;
	        img.img = data->screen_image;
	        img.src_x = screen_change_x << IMG_PREC;
	        img.src_y = screen_change_y << IMG_PREC;
	        img.src_xsize = screen_change_xsize << IMG_PREC;
	        img.src_ysize = screen_change_ysize << IMG_PREC;
		img.dest_xsize = screen_change_xsize * vm->pixel_size;
		img.dest_ysize = screen_change_ysize * vm->pixel_size;
	        draw_image_scaled( x + screen_change_x * vm->pixel_size, y + screen_change_y * vm->pixel_size, &img, wm );
		wbd_draw( wm );
		wbd_unlock( wm );
	    }
	}
	else
	{
	    //Force last available screen redraw + border redraw:

    	    if( vm->pixel_size == 1 )
	    {
	        win_draw_frect( win, 0, 0, x, win->ysize, win->color, wm );
	        win_draw_frect( win, x + sxsize, 0, win->xsize - ( x + sxsize ), win->ysize, win->color, wm );
	        win_draw_frect( win, x, 0, sxsize, y, win->color, wm );
    	        win_draw_frect( win, x, y + sysize, sxsize, win->ysize - ( y + sysize ), win->color, wm );
    	        if( screen_image_can_be_redrawn )
    	        {
    		    win_draw_image( win, x, y, data->screen_image, wm );
            	}
	    }
	    else
	    {
	        wbd_lock( win );
	        draw_frect( 0, 0, x, win->ysize, win->color, wm );
	        draw_frect( x + sxsize, 0, win->xsize - ( x + sxsize ), win->ysize, win->color, wm );
	        draw_frect( x, 0, sxsize, y, win->color, wm );
    	        draw_frect( x, y + sysize, sxsize, win->ysize - ( y + sysize ), win->color, wm );
	        sdwm_image_scaled img;
	        img.img = data->screen_image;
	    	img.src_x = 0;
	    	img.src_y = 0;
	    	img.src_xsize = data->screen_image->xsize << IMG_PREC;
	    	img.src_ysize = data->screen_image->ysize << IMG_PREC;
	    	img.dest_xsize = sxsize;
	    	img.dest_ysize = sysize;
	        draw_image_scaled( x, y, &img, wm );
		wbd_draw( wm );
		wbd_unlock( wm );
	    }
	}

skip_frame_drawing:

	if( ready_for_answer )
	{
	    if( vm->screen_redraw_request != vm->screen_redraw_answer )
		vm->screen_redraw_answer = vm->screen_redraw_request;
	}

	break;
    }

    win_draw_unlock( win, wm );
}

int pixilang_menu_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    switch( win->action_result )
    {
	case 0:
	    //EXIT:
	    send_event( 0, EVT_BUTTONDOWN, 0, 0, 0, KEY_ESCAPE, 0, 1024, wm );
	    break;
    }
    return 0;
}

int pixilang_prefs_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    prefs_clear( wm );
    wm->prefs_flags |= PREFS_FLAG_NO_COLOR_THEME | PREFS_FLAG_NO_FONTS | PREFS_FLAG_NO_CONTROL_TYPE | PREFS_FLAG_NO_KEYMAP;
    prefs_add_default_sections( wm );
    prefs_open( 0, wm );
    return 0;
}

int pixilang_compile_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    pixilang_window_data* data = (pixilang_window_data*)user_data;
    data->save_code_request ^= 1;
    g_pix_save_code_and_exit = 0;
    if( data->save_code_request )
	win->color = BUTTON_HIGHLIGHT_COLOR;
    else
	win->color = wm->button_color;
    draw_window( win, wm );
    return 0;
}

static void pixilang_remove_prog_name( pixilang_window_data* data )
{
    if( data->prog_name_alloc ) smem_free( data->prog_name );
    data->prog_name = NULL;
    data->prog_name_alloc = false;
}

void pixilang_timer( void* user_data, sundog_timer* t, window_manager* wm );

int pixilang_error_dialog_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    pixilang_window_data* data = (pixilang_window_data*)user_data;
    if( data->no_program_selection_dialog )
    {
	//Exit:
	wm->exit_request = 1;
    }
    else
    {
	//Restart:
	data->timer = add_timer( pixilang_timer, data, 0, wm );
    }
    return 1;
}

void pixilang_show_error( pixilang_window_data* data, int err_code, window_manager* wm )
{
    remove_timer( data->timer, wm );
    data->timer = -1;
    WINDOWPTR win = NULL;
    char* str = data->vm->compiler_errors;
    if( str )
    {
        win = dialog_open( NULL, str, wm_get_string( STR_WM_CLOSE ), 0, wm );
        if( win )
        {
    	    set_handler( win->childs[ 0 ], pixilang_error_dialog_handler, data, wm );
	}
    }
    if( !win )
    {
	slog( "pix_load() error %d\n", err_code );
	pixilang_error_dialog_handler( data, NULL, wm );
    }
}

void pixilang_timer( void* user_data, sundog_timer* t, window_manager* wm )
{
    pixilang_window_data* data = (pixilang_window_data*)user_data;
    WINDOWPTR win = data->this_window;
    pix_vm* vm = data->vm;

    switch( data->status )
    {
	case pix_status_none:
	    {
		data->status = pix_status_loading;

		if( data->no_program_selection_dialog && data->open_count > 0 )
		{
		    wm->exit_request = 1;
		    return;
		}

		if( data->prog_name == NULL )
		{
		    wm->opt_fdialog_user_button[ 0 ] = pix_ui_get_string( STR_PIX_UI_PREFS );
		    wm->opt_fdialog_user_button_handler[ 0 ] = pixilang_prefs_handler;
		    wm->opt_fdialog_user_button_data[ 0 ] = data;
		    wm->opt_fdialog_user_button[ 1 ] = pix_ui_get_string( STR_PIX_UI_COMPILE );
		    wm->opt_fdialog_user_button_handler[ 1 ] = pixilang_compile_handler;
		    wm->opt_fdialog_user_button_data[ 1 ] = data;
		    data->prog_name = fdialog( pix_ui_get_string( STR_PIX_UI_SELECT ), 0, ".pixilang_f", 0, FDIALOG_FLAG_LOAD | FDIALOG_FLAG_FULLSCREEN, wm );
		    data->prog_name_alloc = false;
		}

		draw_window( win, wm );
                wm->device_redraw_framebuffer( wm );

		if( data->prog_name == NULL )
		{
		    wm->exit_request = 1;
		}
		else 
		{
		    data->open_count++;
		    if( data->vm )
		    {
			pix_vm_deinit( data->vm );
			smem_free( data->vm );
		    }
		    data->vm = (pix_vm*)smem_new( sizeof( pix_vm ) );
		    smem_zero( data->vm );
		    pix_vm_init( data->vm, win );
		    int cres = pix_load( data->prog_name, data->vm );
		    if( cres == 0 )
		    {
			if( data->save_code_request )
			{
			    char* code_name = (char*)smem_new( smem_strlen( data->prog_name ) + 10 );
			    code_name[ 0 ] = 0; smem_strcat_resize( code_name, data->prog_name );
			    int dot_ptr = smem_strlen( code_name );
			    for( int i = smem_strlen( code_name ); i >= 0; i-- )
			    {
				if( code_name[ i ] == '.' )
				{
				    dot_ptr = i;
				    break;
				}
			    }
			    code_name[ dot_ptr ] = 0;
			    smem_strcat_resize( code_name, ".pixicode" );
#ifdef OS_IOS
			    pix_vm_save_code( "1:/boot.pixicode", data->vm );
#else
			    pix_vm_save_code( code_name, data->vm );
			    cres = -1;
#endif
			    if( g_pix_save_code_and_exit ) wm->exit_request = 1;
			    data->save_code_request = false;
			    smem_free( code_name );
			}
		    }
		    data->status = pix_status_none;
		    draw_window( win, wm );
            	    wm->device_redraw_framebuffer( wm );
		    if( cres == 0 )
		    {
			data->vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i = win->xsize;
			data->vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i = win->ysize;
#ifdef SCREEN_SAFE_AREA_SUPPORTED
			if( win->xsize == wm->screen_xsize && win->ysize == wm->screen_ysize )
			{
			    //Fullscreen mode:
			    data->vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_X ].i = wm->screen_safe_area.x;
			    data->vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_Y ].i = wm->screen_safe_area.y;
			    data->vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_W ].i = wm->screen_safe_area.w;
			    data->vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_H ].i = wm->screen_safe_area.h;
			}
#endif
			data->vm->vars[ PIX_GVAR_PPI ].i = wm->screen_ppi;
			data->vm->vars[ PIX_GVAR_SCALE ].f = wm->screen_scale;
			data->vm->vars[ PIX_GVAR_FONT_SCALE ].f = wm->screen_font_scale;
			pix_vm_resize_container( data->vm->screen, win->xsize, win->ysize, -1, 0, data->vm );
			pix_vm_gfx_set_screen( data->vm->screen, data->vm );
			PIX_VAL v;
			v.i = 0;
			set_focus_win( win, wm );
			pix_vm_clean_container( data->vm->screen, 0, v, 0, -1, data->vm );
			pix_vm_run( 0, 1, 0, PIX_VM_CALL_MAIN, data->vm );
			data->status = pix_status_working;
		    }
		    if( cres > 0 )
		    {
			pixilang_show_error( data, cres, wm );
		    }
		}

		pixilang_remove_prog_name( data );
	    }
	    break;
	case pix_status_loading:
	    break;
	case pix_status_working:
	{
	    pix_vm* vm = data->vm;
	    if( vm == NULL ) break;
	    if( vm->th[ 0 ] && vm->th[ 0 ]->active == 0 )
	    {
		data->status = pix_status_none;
	    }
	    else 
	    {
		if( vm->screen_redraw_request != vm->screen_redraw_answer )
		{
		    if( wm->screen_buffer_preserved )
			pix_vm_draw_screen( win, 1 );
		    else
			screen_changed( wm ); //... and all windows will be redrawn by SunDog engine before device_redraw_framebuffer
		}
	    }
	    break;
	}
    }
}

int pixilang_window_handler( sundog_event* evt, window_manager* wm )
{
    int retval = 0;
    WINDOWPTR win = evt->win;
    pixilang_window_data* data = (pixilang_window_data*)win->data;
    switch( evt->type )
    {
	case EVT_GETDATASIZE:
	    retval = sizeof( pixilang_window_data );
	    break;
	case EVT_AFTERCREATE:
	    {
		data->this_window = win;
		data->vm = NULL;
		data->status = pix_status_none;
		data->screen_image = NULL;
#ifdef SHOW_PIXILANG_MENU
		int but_size = wm->scrollbar_size;
		wm->opt_button_flags = BUTTON_FLAG_FLAT;
		WINDOWPTR menu = new_window( pix_ui_get_string( STR_PIX_UI_MENU_NAME ), 0, 0, but_size, but_size, wm->color0, win, button_handler, wm );
		button_set_menu( menu, pix_ui_get_string( STR_PIX_UI_MENU ) );
		button_set_text( menu, (char*)g_str_down );
		set_window_controller( menu, 0, wm, CPERC, 100, CSUB, but_size, CEND );
		set_window_controller( menu, 2, wm, CPERC, 100, CEND );
		set_handler( menu, pixilang_menu_handler, 0, wm );
#endif
		data->timer = add_timer( pixilang_timer, (void*)data, 0, wm );
	    }
	    retval = 1;
	    break;
	case EVT_BEFORECLOSE:
	    {
		if( data->vm )
		{
		    if( data->status == pix_status_working )
			pix_vm_send_event( PIX_EVT_QUIT, 0, data->vm );
		    pix_vm_deinit( data->vm );
		    smem_free( data->vm );
		    data->vm = NULL;
		}
		remove_image( data->screen_image );
		data->screen_image = NULL;
		remove_timer( data->timer, wm );
		data->timer = -1;
		video_capture_stop( wm );

		pixilang_remove_prog_name( data );
	    }
	    retval = 1;
	    break;
	case EVT_DRAW:
	    switch( data->status )
	    {
		case pix_status_none:
		    win_draw_frect( win, 0, 0, win->xsize, win->ysize, win->color, wm );
		    break;
		case pix_status_loading:
		    wbd_lock( win );
		    wm->cur_font_color = wm->color3;
		    draw_frect( 0, 0, win->xsize, win->ysize, win->color, wm );
		    draw_string( pix_ui_get_string( STR_PIX_UI_LOADING ), 0, 0, wm );
		    wbd_draw( wm );
		    wbd_unlock( wm );
		    break;
		case pix_status_working:
		    pix_vm_draw_screen( win, 0 );
		    break;
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONDOWN:
	    if( data->vm )
	    {
		int x = evt->x - win->screen_x - win->xsize / 2;
		int y = evt->y - win->screen_y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_MOUSEBUTTONDOWN, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEBUTTONUP:
	    if( data->vm )
	    {
		int x = evt->x - win->screen_x - win->xsize / 2;
		int y = evt->y - win->screen_y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_MOUSEBUTTONUP, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_MOUSEMOVE:
	    if( data->vm )
	    {
		int x = evt->x - win->screen_x - win->xsize / 2;
		int y = evt->y - win->screen_y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_MOUSEMOVE, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHBEGIN:
	    if( data->vm )
	    {
		int x = evt->x - win->screen_x - win->xsize / 2;
		int y = evt->y - win->screen_y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_TOUCHBEGIN, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHEND:
	    if( data->vm )
	    {
		int x = evt->x - win->screen_x - win->xsize / 2;
		int y = evt->y - win->screen_y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_TOUCHEND, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_TOUCHMOVE:
	    if( data->vm )
	    {
		int x = evt->x - win->screen_x - win->xsize / 2;
		int y = evt->y - win->screen_y - win->ysize / 2;
		pix_vm_send_event( PIX_EVT_TOUCHMOVE, evt->flags, x, y, evt->key, evt->scancode, evt->pressure, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_BUTTONDOWN:
	    if( data->vm )
	    {
		pix_vm_send_event( PIX_EVT_BUTTONDOWN, evt->flags, 0, 0, evt->key, evt->scancode, evt->pressure, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_BUTTONUP:
	    if( data->vm )
	    {
		pix_vm_send_event( PIX_EVT_BUTTONUP, evt->flags, 0, 0, evt->key, evt->scancode, evt->pressure, data->vm );
	    }
	    retval = 1;
	    break;
	case EVT_SCREENRESIZE:
            if( data->vm )
	    {
		pix_vm_send_event( PIX_EVT_SCREENRESIZE, 0, data->vm );
	    }
	    break;
	case EVT_PIXICMD:
	    if( !data->vm ) break;
	    //Requests for some SunDog-based UI functions:
	    switch( evt->x )
	    {
	        case pix_sundog_req_filedialog:
	        {
	    	    pix_sundog_filedialog* req = data->vm->sd_filedialog;
		    if( req )
		    {
		        data->sundog_ui_request_handling = true;
		        req->result = fdialog( 
    		    	    (const char*)req->name, 
			    (const char*)req->mask, 
			    (const char*)req->id, 
			    (const char*)req->def_name, 
			    req->flags | FDIALOG_FLAG_FULLSCREEN, wm );
			COMPILER_MEMORY_BARRIER();
			req->handled = 1;
			data->sundog_ui_request_handling = false;
		    }
		    break;
		}
	        case pix_sundog_req_preferences:
		    pixilang_prefs_handler( 0, 0, wm ); //Show global preferences
	    	    break;
	        case pix_sundog_req_vsync:
		    win_vsync( evt->y, wm );
		    break;
	        case pix_sundog_req_webserver:
	    	    if( g_webserver_available )
	    	    {
			data->sundog_ui_request_handling = true;
            		webserver_open( wm );
            		webserver_wait_for_close( wm );
			COMPILER_MEMORY_BARRIER();
            		data->vm->sd_webserver_closed = 1;
			data->sundog_ui_request_handling = false;
		    }
	    	    break;
	    	case pix_sundog_req_textinput:
	    	{
	    	    pix_sundog_textinput* req = data->vm->sd_textinput;
		    if( req )
		    {
			data->sundog_ui_request_handling = true;
			WINDOWPTR text = new_window( "textinput source", 0, 0, 1, 1, wm->text_background, win, text_handler, wm );
			text_set_text( text, (const char*)req->def_str, wm );
			show_window( text );
			show_keyboard_for_text_window( text, (const char*)req->name, wm );
			req->result = smem_strdup( text_get_text( text, wm ) );
			remove_window( text, wm );
			COMPILER_MEMORY_BARRIER();
			req->handled = 1;
			data->sundog_ui_request_handling = false;
		    }
	    	    break;
	    	}
	    	case pix_sundog_req_vcap:
	    	{
	    	    pix_sundog_vcap* req = data->vm->sd_vcap;
		    if( req )
		    {
			if( evt->y == 0 )
			{
			    wm->vcap_in_fps = req->fps;
			    wm->vcap_in_bitrate_kb = req->bitrate_kb;
			    wm->vcap_in_flags = req->flags;
			    req->err = video_capture_start( wm );
			    COMPILER_MEMORY_BARRIER();
			    req->handled = 1;
			}
			if( evt->y == 1 )
			{
			    req->err = video_capture_stop( wm );
			    COMPILER_MEMORY_BARRIER();
			    req->handled = 1;
			}
		    }
		    break;
		}
	    	case pix_sundog_req_midiopt:
#ifdef OS_IOS
	    	    ios_sundog_bluetooth_midi_settings( wm->sd );
#endif
	    	    break;
		default: break;
	    }
	    retval = 1;
	    break;
    }
    return retval;
}

#if DEMO_MODE == 2
int demo_timer_win_handler( void* user_data, WINDOWPTR win, window_manager* wm )
{
    int res = win->action_result;
    wm->exit_request = true;
    return 0;
}

void demo_timer( void* user_data, sundog_timer* t, window_manager* wm )
{
    WINDOWPTR w = dialog_open( wm_get_string( STR_WM_DEMOVERSION ), wm_get_string( STR_WM_DEMOVERSION_BUY ), wm_get_string( STR_WM_CLOSE ), DIALOG_FLAG_SINGLE, wm );
    set_handler( w->childs[ 0 ], demo_timer_win_handler, user_data, wm );
    remove_timer( t->id, wm );
}
#endif

int app_init( window_manager* wm )
{
    bool arg_save_code = false;
    char* arg_prog_name = NULL;
    for( int i = 1; i < wm->sd->argc; i++ )
    {
        char* v = wm->sd->argv[ i ];
        while( 1 )
        {
            if( smem_strcmp( v, "clearall" ) == 0 )
            {
                sfs_remove_support_files( ".pixilang_" );
                sconfig_remove_all_files();
                return -2;
            }
            if( v[ 0 ] == '-' )
            {
        	v++;
        	if( smem_strcmp( v, "id" ) == 0 ) { i++; break; } //handled in sundog_main()
                if( smem_strcmp( v, "cfg" ) == 0 ) { i++; break; } //handled in sundog_main()
		if( smem_strcmp( v, "c" ) == 0 )
		{
		    arg_save_code = true;
		    break;
		}
        	break;
            }
	    arg_prog_name = v;
	    i = 1000000;
            break;
        }
    }

    wm->root_win = new_window( 
	"Desktop", 
	0, 0, 
	wm->screen_xsize, wm->screen_ysize, 
	wm->color0, 
	0, 
	desktop_handler,
	wm );
    show_window( wm->root_win );
    recalc_regions( wm );
    draw_window( wm->root_win, wm );

#ifdef OS_ANDROID
    copy_resources( wm );
    check_resources( wm );
#endif

    WINDOWPTR pix_win = new_window(
	"Pixilang window", 
	0, 0, 
	wm->screen_xsize, wm->screen_ysize, 
	wm->color0, 
	wm->root_win, 
	pixilang_window_handler,
	wm );
    pixilang_window_data* pix_win_data = (pixilang_window_data*)pix_win->data;
    if( arg_save_code ) pix_win_data->save_code_request = true;
    if( arg_prog_name )
    {
	pix_win_data->prog_name = arg_prog_name;
	pix_win_data->prog_name_alloc = false;
    }

    if( pix_win_data->prog_name )
    {
	if( sfs_get_file_size( pix_win_data->prog_name ) == 0 )
	{
	    slog( "Error: can't open %s\n", pix_win_data->prog_name );
    	    pixilang_remove_prog_name( pix_win_data );
    	}
    }
    if( pix_win_data->save_code_request && pix_win_data->prog_name == NULL )
    {
	slog( "Error: no source file\n" );
	wm->exit_request = 1;
    }

    sfs_file f;

#ifdef OS_IOS
    if( pix_win_data->prog_name == NULL )
    {
	pix_win_data->prog_name = (char*)smem_new( smem_strlen( g_apple_resources_path ) + 256 );
	pix_win_data->prog_name_alloc = true;

	sprintf( pix_win_data->prog_name, "%s/boot.pixicode", g_apple_resources_path );
	bool found = false;
	f = sfs_open( pix_win_data->prog_name, "rb" );
	if( f )
	{
	    found = true;
	    slog( "boot.pixicode found\n" );
	    sfs_close( f );
	}

	if( !found )
	{
	    sprintf( pix_win_data->prog_name, "%s/boot.pixi", g_apple_resources_path );
	    f = sfs_open( pix_win_data->prog_name, "rb" );
	    if( f )
	    {
		found = true;
		sfs_close( f );
	    }
	}

	if( !found ) pixilang_remove_prog_name( pix_win_data );
    }
#else
    if( pix_win_data->prog_name == NULL )
    {
	pix_win_data->prog_name = (char*)"1:/boot.pixicode";
	f = sfs_open( pix_win_data->prog_name, "rb" );
	if( f )
	    sfs_close( f );
	else
	    pixilang_remove_prog_name( pix_win_data );
    }
    
    if( pix_win_data->prog_name == NULL )
    {
	pix_win_data->prog_name = (char*)"1:/boot.pixi";
	f = sfs_open( pix_win_data->prog_name, "rb" );
	if( f )
	    sfs_close( f );
	else
	    pixilang_remove_prog_name( pix_win_data );
    }

    if( pix_win_data->prog_name == NULL )
    {
	pix_win_data->prog_name = (char*)"1:/boot.txt";
	f = sfs_open( pix_win_data->prog_name, "rb" );
	if( f )
	    sfs_close( f );
	else
	    pixilang_remove_prog_name( pix_win_data );
    }
#endif

    if( pix_win_data->prog_name )
    {
        pix_win_data->no_program_selection_dialog = true;
#ifdef PIX_SAVE_BYTECODE
        pix_win_data->save_code_request = true;
#endif
    }

    set_window_controller( pix_win, 0, wm, (WCMD)0, CEND );
    set_window_controller( pix_win, 1, wm, (WCMD)0, CEND );
    set_window_controller( pix_win, 2, wm, CPERC, (WCMD)100, CEND );
    set_window_controller( pix_win, 3, wm, CPERC, (WCMD)100, CEND );
    pix_win->font = 1;
    show_window( pix_win );
    recalc_regions( wm );
    draw_window( pix_win, wm );

#if DEMO_MODE == 2
    add_timer( demo_timer, 0, 1000 * 60 * 3, wm );
#endif

    return 0;
}

int app_event_handler( sundog_event* evt, window_manager* wm )
{
    int handled = 0;

    switch( evt->type )
    {
#ifdef SUNDOG_STATE
        case EVT_LOADSTATE:
        case EVT_SAVESTATE:
        {
            WINDOWPTR pix_win = find_win_by_handler( wm->root_win, pixilang_window_handler );
    	    if( pix_win == NULL ) break;
	    pixilang_window_data* pdata = (pixilang_window_data*)pix_win->data;
	    if( pdata->vm == NULL )
	    {
		//try again later:
		send_events( evt, 1, wm );
		break;
	    }
	    if( evt->type == EVT_LOADSTATE )
	    {
		slog( " >> SEND EVT LOADSTATE...\n" );
    		pix_vm_send_event( PIX_EVT_LOADSTATE, 0, pdata->vm );
    	    }
    	    else
    	    {
		slog( " >> SEND EVT SAVESTATE...\n" );
    		pix_vm_send_event( PIX_EVT_SAVESTATE, 0, pdata->vm );
    	    }
    	    handled = 1;
    	    break;
    	}
#endif
        case EVT_SUSPEND:
	    handled = 1;
            break;
        case EVT_DEVSUSPEND:
            {
		//Main SunDog sound stream will be stopped or resumed automatically,
		//but here you can handle some additional devices/streams:
                /*WINDOWPTR pix_win = find_win_by_handler( wm->root_win, pixilang_window_handler );
		if( !pix_win ) break;
		pixilang_window_data* pdata = (pixilang_window_data*)pix_win->data;
		if( pdata->status == pix_status_working && pdata->vm && pdata->vm->audio )
		{
            	    if( wm->devices_suspended )
            	    {
                	//Devices can be suspended (app hidden, WM inactive):
                	sundog_sound_device_stop( pdata->vm->audio );
        	    }
            	    else
            	    {
                	//Exit from suspended state:
                	sundog_sound_device_play( pdata->vm->audio );
            	    }
            	}*/
                handled = 1;
            }
            break;
	case EVT_BUTTONDOWN:
	    if( evt->key == KEY_ESCAPE )
	    {
                WINDOWPTR pix_win = find_win_by_handler( wm->root_win, pixilang_window_handler );
		if( pix_win )
		{
		    pixilang_window_data* pdata = (pixilang_window_data*)pix_win->data;
		    if( pdata->sundog_ui_request_handling ) break; //some other (not Pixilang) windows must handle ESCAPE button
		}
	    }
	    else
	    {
		break;
	    }
	case EVT_QUIT:
	    {
                WINDOWPTR pix_win = find_win_by_handler( wm->root_win, pixilang_window_handler );
		if( pix_win == NULL ) break;
		pixilang_window_data* pdata = (pixilang_window_data*)pix_win->data;
		if( pdata->status == pix_status_working && pdata->vm )
		{
		    pix_vm_send_event( PIX_EVT_QUIT, evt->flags, 0, 0, 0, 1, 0, pdata->vm );
		    if( pdata->vm->quit_action == 1 )
		    {
			//Force close:
			pix_vm_deinit( pdata->vm );
			smem_free( pdata->vm );
			pdata->vm = NULL;
			pdata->status = pix_status_none; //back to the program selection dialog...
			remove_image( pdata->screen_image );
            		pdata->screen_image = NULL;
			draw_window( wm->root_win, wm );
            	    }
		}
		else
		{
		    wm->exit_request = 1;
		}
		handled = 1;
	    }
	    break;
    }
    
    return handled;
}

int app_deinit( window_manager* wm )
{
    remove_window( wm->root_win, wm );
    return 0;
}

int app_global_init( void )
{
    int rv = 0;
    if( pix_global_init() ) rv = -1;
#ifndef PIX_NOSUNVOX
    if( sunvox_global_init() ) rv = -2;
#endif
    return rv;
}

int app_global_deinit( void )
{
    int rv = 0;
    if( pix_global_deinit() ) rv = -1;
#ifndef PIX_NOSUNVOX
    if( sunvox_global_deinit() ) rv = -2;
#endif
    return rv;
}
