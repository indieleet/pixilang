#include "sundog.h"
#include "pixilang.h"

#if defined(OS_UNIX)
    #if ( defined(ARCH_X86) || defined(ARCH_X86_64) || defined(ARCH_ARM) ) && !defined(OS_IOS)
	#include <dlfcn.h>
	#define DYNAMIC_LIB_SUPPORT
    #endif
    
    #include <sys/stat.h>
    #include <sys/types.h>
    
    #ifdef OS_ANDROID
	#define NO_SETXATTR
    #else
	#include <sys/xattr.h>
    #endif
#endif

#ifdef OS_WIN
    #include <windows.h>
    #define DYNAMIC_LIB_SUPPORT
#endif

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
#define DPRINT( fmt, ARGS... ) slog( fmt, ## ARGS )
#else
#define DPRINT( fmt, ARGS... ) {}
#endif

#define GET_VAL_FROM_STACK( v, snum, type ) { size_t sp_ = PIX_CHECK_SP( sp + snum ); if( stack[ sp_ ].t == 0 ) v = (type)stack[ sp_ ].i; else v = (type)stack[ sp_ ].f; }

#define FN_HEADER pix_stack_val* stack = th->stack;

//FNs

//Containers (memory management):

void fn_new_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_pixi_with_alpha( PIX_BUILTIN_FN_PARAMETERS );
void fn_resize_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_rotate_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_clean_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_clone_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_copy_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_pixi_info( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_pixi_flags( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_pixi_flags( PIX_BUILTIN_FN_PARAMETERS );
void fn_reset_pixi_flags( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_pixi_prop_OR_set_pixi_prop( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_pixi_prop( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_pixi_props( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_pixi_proplist( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_pixi_proplist( PIX_BUILTIN_FN_PARAMETERS );
void fn_convert_pixi_type( PIX_BUILTIN_FN_PARAMETERS );
void fn_show_smem_debug_messages( PIX_BUILTIN_FN_PARAMETERS );
void fn_zlib_pack( PIX_BUILTIN_FN_PARAMETERS );
void fn_zlib_unpack( PIX_BUILTIN_FN_PARAMETERS );

//Working with strings:

void fn_num_to_string( PIX_BUILTIN_FN_PARAMETERS );
void fn_string_to_num( PIX_BUILTIN_FN_PARAMETERS );

//Working with strings (posix):

void fn_strcat( PIX_BUILTIN_FN_PARAMETERS );
void fn_strcmp_OR_strstr( PIX_BUILTIN_FN_PARAMETERS );
void fn_strlen( PIX_BUILTIN_FN_PARAMETERS );
void fn_sprintf( PIX_BUILTIN_FN_PARAMETERS );

//Log management:

void fn_get_log( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_system_log( PIX_BUILTIN_FN_PARAMETERS );

//Files:

void fn_load( PIX_BUILTIN_FN_PARAMETERS );
void fn_fload( PIX_BUILTIN_FN_PARAMETERS );
void fn_save( PIX_BUILTIN_FN_PARAMETERS );
void fn_fsave( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_real_path( PIX_BUILTIN_FN_PARAMETERS );
void fn_new_flist( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_flist( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_flist_name( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_flist_type( PIX_BUILTIN_FN_PARAMETERS );
void fn_flist_next( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_file_size( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_file_format( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_fformat_mime_OR_ext( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_file( PIX_BUILTIN_FN_PARAMETERS );
void fn_rename_file( PIX_BUILTIN_FN_PARAMETERS );
void fn_copy_file( PIX_BUILTIN_FN_PARAMETERS );
void fn_create_directory( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_disk0( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_disk0( PIX_BUILTIN_FN_PARAMETERS );

//Files (posix):

void fn_fopen( PIX_BUILTIN_FN_PARAMETERS );
void fn_fopen_mem( PIX_BUILTIN_FN_PARAMETERS );
void fn_fclose( PIX_BUILTIN_FN_PARAMETERS );
void fn_fputc( PIX_BUILTIN_FN_PARAMETERS );
void fn_fputs( PIX_BUILTIN_FN_PARAMETERS );
void fn_fgets_OR_fwrite_OR_fread( PIX_BUILTIN_FN_PARAMETERS );
void fn_fgetc( PIX_BUILTIN_FN_PARAMETERS );
void fn_feof_OF_fflush_OR_ftell( PIX_BUILTIN_FN_PARAMETERS );
void fn_fseek( PIX_BUILTIN_FN_PARAMETERS );
void fn_setxattr( PIX_BUILTIN_FN_PARAMETERS );

//Graphics:

void fn_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_vsync( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_pixel_size( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_pixel_size( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_screen( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_screen( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_zbuf( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_zbuf( PIX_BUILTIN_FN_PARAMETERS );
void fn_clear_zbuf( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_color( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_red( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_green( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_blue( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_blend( PIX_BUILTIN_FN_PARAMETERS );
void fn_transp( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_transp( PIX_BUILTIN_FN_PARAMETERS );
void fn_clear( PIX_BUILTIN_FN_PARAMETERS );
void fn_dot( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_dot( PIX_BUILTIN_FN_PARAMETERS );
void fn_line( PIX_BUILTIN_FN_PARAMETERS );
void fn_box( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi( PIX_BUILTIN_FN_PARAMETERS );
void fn_triangles3d( PIX_BUILTIN_FN_PARAMETERS );
void fn_sort_triangles3d( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_key_color( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_key_color( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_alpha( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_alpha( PIX_BUILTIN_FN_PARAMETERS );
void fn_print( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_text_xysize( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_font( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_font( PIX_BUILTIN_FN_PARAMETERS );
void fn_effector( PIX_BUILTIN_FN_PARAMETERS );
void fn_color_gradient( PIX_BUILTIN_FN_PARAMETERS );
void fn_split_rgb( PIX_BUILTIN_FN_PARAMETERS );

//OpenGL:

void fn_set_gl_callback( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_gl_data( PIX_BUILTIN_FN_PARAMETERS );
void fn_update_gl_data( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_draw_arrays( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_blend_func( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_bind_framebuffer( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_bind_texture( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_get_int( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_get_float( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_new_prog( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_use_prog( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_uniform( PIX_BUILTIN_FN_PARAMETERS );
void fn_gl_uniform_matrix( PIX_BUILTIN_FN_PARAMETERS );

//Animation:

void fn_pixi_unpack_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_pack_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_create_anim( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_remove_anim( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_clone_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_remove_frame( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_play( PIX_BUILTIN_FN_PARAMETERS );
void fn_pixi_stop( PIX_BUILTIN_FN_PARAMETERS );

//Video (not finished):

void fn_video_open( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_close( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_start( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_stop( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_props( PIX_BUILTIN_FN_PARAMETERS );
void fn_video_capture_frame( PIX_BUILTIN_FN_PARAMETERS );

//Transformation:

void fn_t_reset( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_rotate( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_translate( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_scale( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_push_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_pop_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_get_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_set_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_mul_matrix( PIX_BUILTIN_FN_PARAMETERS );
void fn_t_point( PIX_BUILTIN_FN_PARAMETERS );

//Audio:

void fn_set_audio_callback( PIX_BUILTIN_FN_PARAMETERS );
void fn_enable_audio_input( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_audio_sample_rate( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_note_freq( PIX_BUILTIN_FN_PARAMETERS );

//MIDI:

void fn_midi_open_client( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_close_client( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_get_device( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_open_port( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_reopen_port( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_close_port( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_get_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_get_event_time( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_next_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_midi_send_event( PIX_BUILTIN_FN_PARAMETERS );

//SunVox:

void fn_sv_new( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_remove( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_sample_rate( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_render( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_lock_unlock( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_load_fload( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_save_fsave( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_play( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_stop( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_pause( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_resume( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_sync_resume( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_autostop( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_autostop( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_status( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_rewind( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_volume( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_event_t( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_send_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_current_line( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_current_signal_level( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_name( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_name( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_bpm( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_len( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_time_map( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_new_module( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_remove_module( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_connect_disconnect_module( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_fload_module( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_mod_fload( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_sampler_par( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_number_of_modules( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_find_module( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_selected_module( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_flags( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_inputs( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_type( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_name( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_module_name( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_xy( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_module_xy( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_color( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_module_color( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_finetune( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_module_finetune( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_module_relnote( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_scope( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_module_curve( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_ctl_cnt( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_ctl_name( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_ctl_value( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_module_ctl_value( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_module_ctl_par( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_new_pat( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_remove_pat( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_number_of_pats( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_find_pattern( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_pat( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_pat_xy( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_pat_size( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_pat_name( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_set_pat_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_get_pat_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_sv_pat_mute( PIX_BUILTIN_FN_PARAMETERS );

//Time:

void fn_start_timer( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_timer( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_year( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_month( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_day( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_hours( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_minutes( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_seconds( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_ticks( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_tps( PIX_BUILTIN_FN_PARAMETERS );
void fn_sleep( PIX_BUILTIN_FN_PARAMETERS );

//Events:

void fn_get_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_quit_action( PIX_BUILTIN_FN_PARAMETERS );

//Threads:

void fn_thread_create( PIX_BUILTIN_FN_PARAMETERS );
void fn_thread_destroy( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_create( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_destroy( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_lock( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_trylock( PIX_BUILTIN_FN_PARAMETERS );
void fn_mutex_unlock( PIX_BUILTIN_FN_PARAMETERS );

//Mathematical functions:

void fn_acos( PIX_BUILTIN_FN_PARAMETERS );
void fn_acosh( PIX_BUILTIN_FN_PARAMETERS );
void fn_asin( PIX_BUILTIN_FN_PARAMETERS );
void fn_asinh( PIX_BUILTIN_FN_PARAMETERS );
void fn_atan( PIX_BUILTIN_FN_PARAMETERS );
void fn_atanh( PIX_BUILTIN_FN_PARAMETERS );
void fn_atan2( PIX_BUILTIN_FN_PARAMETERS );
void fn_ceil( PIX_BUILTIN_FN_PARAMETERS );
void fn_cos( PIX_BUILTIN_FN_PARAMETERS );
void fn_cosh( PIX_BUILTIN_FN_PARAMETERS );
void fn_exp( PIX_BUILTIN_FN_PARAMETERS );
void fn_exp2( PIX_BUILTIN_FN_PARAMETERS );
void fn_expm1( PIX_BUILTIN_FN_PARAMETERS );
void fn_abs( PIX_BUILTIN_FN_PARAMETERS );
void fn_floor( PIX_BUILTIN_FN_PARAMETERS );
void fn_mod( PIX_BUILTIN_FN_PARAMETERS );
void fn_log( PIX_BUILTIN_FN_PARAMETERS );
void fn_log2( PIX_BUILTIN_FN_PARAMETERS );
void fn_log10( PIX_BUILTIN_FN_PARAMETERS );
void fn_pow( PIX_BUILTIN_FN_PARAMETERS );
void fn_sin( PIX_BUILTIN_FN_PARAMETERS );
void fn_sinh( PIX_BUILTIN_FN_PARAMETERS );
void fn_sqrt( PIX_BUILTIN_FN_PARAMETERS );
void fn_tan( PIX_BUILTIN_FN_PARAMETERS );
void fn_tanh( PIX_BUILTIN_FN_PARAMETERS );
void fn_rand( PIX_BUILTIN_FN_PARAMETERS );
void fn_rand_seed( PIX_BUILTIN_FN_PARAMETERS );

//Type punning:

void fn_reinterpret_type( PIX_BUILTIN_FN_PARAMETERS );

//Data processing:

void fn_op_cn( PIX_BUILTIN_FN_PARAMETERS );
void fn_op_cc( PIX_BUILTIN_FN_PARAMETERS );
void fn_op_ccn( PIX_BUILTIN_FN_PARAMETERS );
void fn_generator( PIX_BUILTIN_FN_PARAMETERS );
void fn_wavetable_generator( PIX_BUILTIN_FN_PARAMETERS );
void fn_sampler( PIX_BUILTIN_FN_PARAMETERS );
void fn_envelope2p( PIX_BUILTIN_FN_PARAMETERS );
void fn_gradient( PIX_BUILTIN_FN_PARAMETERS );
void fn_fft( PIX_BUILTIN_FN_PARAMETERS );
void fn_new_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_remove_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_init_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_reset_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_apply_filter( PIX_BUILTIN_FN_PARAMETERS );
void fn_replace_values( PIX_BUILTIN_FN_PARAMETERS );
void fn_copy_and_resize( PIX_BUILTIN_FN_PARAMETERS );
void fn_conv_filter( PIX_BUILTIN_FN_PARAMETERS );

//Dialogs:

void fn_file_dialog( PIX_BUILTIN_FN_PARAMETERS );
void fn_prefs_dialog( PIX_BUILTIN_FN_PARAMETERS );
void fn_textinput_dialog( PIX_BUILTIN_FN_PARAMETERS );

//Network:

void fn_system_copy_OR_open_url( PIX_BUILTIN_FN_PARAMETERS );

//Native code:

void fn_dlopen( PIX_BUILTIN_FN_PARAMETERS );
void fn_dlclose( PIX_BUILTIN_FN_PARAMETERS );
void fn_dlsym( PIX_BUILTIN_FN_PARAMETERS );
void fn_dlcall( PIX_BUILTIN_FN_PARAMETERS );

//Posix compatibility:

void fn_system( PIX_BUILTIN_FN_PARAMETERS );
void fn_argc( PIX_BUILTIN_FN_PARAMETERS );
void fn_argv( PIX_BUILTIN_FN_PARAMETERS );
void fn_exit( PIX_BUILTIN_FN_PARAMETERS );

//Experimental API:

void fn_webserver_dialog( PIX_BUILTIN_FN_PARAMETERS );
void fn_midiopt_dialog( PIX_BUILTIN_FN_PARAMETERS );
void fn_system_paste( PIX_BUILTIN_FN_PARAMETERS );
void fn_send_file_to( PIX_BUILTIN_FN_PARAMETERS );
void fn_export_import_file( PIX_BUILTIN_FN_PARAMETERS );
void fn_set_audio_play_status( PIX_BUILTIN_FN_PARAMETERS );
void fn_get_audio_event( PIX_BUILTIN_FN_PARAMETERS );
void fn_openclose_app_state( PIX_BUILTIN_FN_PARAMETERS );
void fn_wm_video_capture_start_OR_stop( PIX_BUILTIN_FN_PARAMETERS );
void fn_wm_video_capture_get_ext( PIX_BUILTIN_FN_PARAMETERS );
void fn_wm_video_capture_encode( PIX_BUILTIN_FN_PARAMETERS );
