/*
    pixilang_vm_builtin_fns.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "pixilang_vm_builtin_fns.h"

#include "video/video.h"
#include "dsp.h"
#ifndef PIX_NOSUNVOX
    #include "sunvox_engine.h"
#else
    #define PSYNTH_FREQ_TABLE_BODY
    #include "../lib_sunvox/psynth/psynth_freq_table.h" //g_linear_freq_tab[]
#endif

#include <errno.h>

const char* g_pix_fn_names[ FN_NUM ] = 
{ //FN names

    //Containers (memory management):

    "new",
    "remove",
    "remove_with_alpha",
    "resize",
    "rotate",
    "clean",
    "clone",
    "copy",
    "get_size",
    "get_xsize",
    "get_ysize",
    "get_esize",
    "get_type",
    "get_flags",
    "set_flags",
    "reset_flags",
    "get_prop",
    "set_prop",
    "remove_prop",
    "remove_props",
    "get_proplist",
    "remove_proplist",
    "convert_type",
    "show_memory_debug_messages",
    "zlib_pack",
    "zlib_unpack",

    //Working with strings:

    "num_to_str",
    "str_to_num",

    //Working with strings (posix):

    "strcat",
    "strcmp",
    "strlen",
    "strstr",
    "sprintf",
    "sprintf2",
    "printf",
    "fprintf",

    //Log management:

    "logf",
    "get_log",
    "get_system_log",

    //Files:

    "load",
    "fload",
    "save",
    "fsave",
    "get_real_path",
    "new_flist",
    "remove_flist",
    "get_flist_name",
    "get_flist_type",
    "flist_next",
    "get_file_size",
    "get_file_format",
    "get_fformat_mime",
    "get_fformat_ext",
    "remove_file",
    "rename_file",
    "copy_file",
    "create_directory",
    "set_disk0",
    "get_disk0",

    //Files (posix):

    "fopen",
    "fopen_mem",
    "fclose",
    "fputc",
    "fputs",
    "fwrite",
    "fgetc",
    "fgets",
    "fread",
    "feof",
    "fflush",
    "fseek",
    "ftell",
    "setxattr",

    //Graphics:

    "frame",
    "vsync",
    "set_pixel_size",
    "get_pixel_size",
    "set_screen",
    "get_screen",
    "set_zbuf",
    "get_zbuf",
    "clear_zbuf",
    "get_color",
    "get_red",
    "get_green",
    "get_blue",
    "get_blend",
    "transp",
    "get_transp",
    "clear",
    "dot",
    "dot3d",
    "get_dot",
    "get_dot3d",
    "line",
    "line3d",
    "box",
    "fbox",
    "pixi",
    "triangles3d",
    "sort_triangles3d",
    "set_key_color",
    "get_key_color",
    "set_alpha",
    "get_alpha",
    "print",
    "get_text_xsize",
    "get_text_ysize",
    "get_text_xysize",
    "set_font",
    "get_font",
    "effector",
    "color_gradient",
    "split_rgb",
    "split_ycbcr",

    //OpenGL:

    "set_gl_callback",
    "remove_gl_data",
    "update_gl_data",
    "gl_draw_arrays",
    "gl_blend_func",
    "gl_bind_framebuffer",
    "gl_bind_texture",
    "gl_get_int",
    "gl_get_float",
    "gl_new_prog",
    "gl_use_prog",
    "gl_uniform",
    "gl_uniform_matrix",

    //Animation:

    "unpack_frame",
    "pack_frame",
    "create_anim",
    "remove_anim",
    "clone_frame",
    "remove_frame",
    "play",
    "stop",

    //Video (not finished):

    "video_open",
    "video_close",
    "video_start",
    "video_stop",
    "video_set_props",
    "video_get_props",
    "video_capture_frame",

    //Transformation:

    "t_reset",
    "t_rotate",
    "t_translate",
    "t_scale",
    "t_push_matrix",
    "t_pop_matrix",
    "t_get_matrix",
    "t_set_matrix",
    "t_mul_matrix",
    "t_point",

    //Audio:

    "set_audio_callback",
    "enable_audio_input",
    "get_audio_sample_rate",
    "get_note_freq",

    //MIDI:

    "midi_open_client",
    "midi_close_client",
    "midi_get_device",
    "midi_open_port",
    "midi_reopen_port",
    "midi_close_port",
    "midi_get_event",
    "midi_get_event_time",
    "midi_next_event",
    "midi_send_event",

    //SunVox:

    "sv_new",
    "sv_remove",
    "sv_get_sample_rate",
    "sv_render",
    "sv_lock",
    "sv_unlock",
    "sv_load",
    "sv_fload",
    "sv_save",
    "sv_fsave",
    "sv_play",
    "sv_stop",
    "sv_pause",
    "sv_resume",
    "sv_sync_resume",
    "sv_set_autostop",
    "sv_get_autostop",
    "sv_get_status",
    "sv_rewind",
    "sv_volume",
    "sv_set_event_t",
    "sv_send_event",
    "sv_get_current_line",
    "sv_get_current_line2",
    "sv_get_current_signal_level",
    "sv_get_name",
    "sv_set_name",
    "sv_get_bpm",
    "sv_get_tpl",
    "sv_get_length_frames",
    "sv_get_length_lines",
    "sv_get_time_map",
    "sv_new_module",
    "sv_remove_module",
    "sv_connect_module",
    "sv_disconnect_module",
    "sv_load_module",
    "sv_fload_module",
    "sv_sampler_load",
    "sv_sampler_fload",
    "sv_sampler_par",
    "sv_metamodule_load",
    "sv_metamodule_fload",
    "sv_vplayer_load",
    "sv_vplayer_fload",
    "sv_get_number_of_modules",
    "sv_find_module",
    "sv_selected_module",
    "sv_get_module_flags",
    "sv_get_module_inputs",
    "sv_get_module_outputs",
    "sv_get_module_type",
    "sv_get_module_name",
    "sv_set_module_name",
    "sv_get_module_xy",
    "sv_set_module_xy",
    "sv_get_module_color",
    "sv_set_module_color",
    "sv_get_module_finetune",
    "sv_set_module_finetune",
    "sv_set_module_relnote",
    "sv_get_module_scope",
    "sv_module_curve",
    "sv_get_number_of_module_ctls",
    "sv_get_module_ctl_name",
    "sv_get_module_ctl_value",
    "sv_set_module_ctl_value",
    "sv_get_module_ctl_min",
    "sv_get_module_ctl_max",
    "sv_get_module_ctl_offset",
    "sv_get_module_ctl_type",
    "sv_get_module_ctl_group",
    "sv_new_pattern",
    "sv_remove_pattern",
    "sv_get_number_of_patterns",
    "sv_find_pattern",
    "sv_get_pattern_x",
    "sv_get_pattern_y",
    "sv_set_pattern_xy",
    "sv_get_pattern_tracks",
    "sv_get_pattern_lines",
    "sv_set_pattern_size",
    "sv_get_pattern_name",
    "sv_set_pattern_name",
    "sv_get_pattern_data",
    "sv_set_pattern_event",
    "sv_get_pattern_event",
    "sv_pattern_mute",

    //Time:

    "start_timer",
    "get_timer",
    "get_year",
    "get_month",
    "get_day",
    "get_hours",
    "get_minutes",
    "get_seconds",
    "get_ticks",
    "get_tps",
    "sleep",

    //Events:

    "get_event",
    "set_quit_action",

    //Threads:

    "thread_create",
    "thread_destroy",
    "mutex_create",
    "mutex_destroy",
    "mutex_lock",
    "mutex_trylock",
    "mutex_unlock",

    //Mathematical functions:

    "acos",
    "acosh",
    "asin",
    "asinh",
    "atan",
    "atanh",
    "atan2",
    "ceil",
    "cos",
    "cosh",
    "exp",
    "exp2",
    "expm1",
    "abs",
    "floor",
    "mod",
    "log",
    "log2",
    "log10",
    "pow",
    "sin",
    "sinh",
    "sqrt",
    "tan",
    "tanh",
    "rand",
    "rand_seed",

    //Type punning:

    "reinterpret_type",

    //Data processing:

    "op_cn",
    "op_cc",
    "op_ccn",
    "generator",
    "wavetable_generator",
    "sampler",
    "envelope2p",
    "gradient",
    "fft",
    "new_filter",
    "remove_filter",
    "init_filter",
    "reset_filter",
    "apply_filter",
    "replace_values",
    "copy_and_resize",
    "conv_filter",

    //Dialogs:

    "file_dialog",
    "prefs_dialog",
    "textinput_dialog",

    //Network:

    "open_url",

    //Native code:

    "dlopen",
    "dlclose",
    "dlsym",
    "dlcall",

    //Posix compatibility:

    "system",
    "argc",
    "argv",
    "exit",

    //Experimental API:

    "webserver_dialog",
    "midiopt_dialog",
    "system_copy",
    "system_paste",
    "send_file_to_gallery",
    "export_import_file",
    "set_audio_play_status",
    "get_audio_event",
    "open_app_state",
    "close_app_state",
    "wm_video_capture_start",
    "wm_video_capture_stop",
    "wm_video_capture_get_ext",
    "wm_video_capture_encode",
}; //FN names

pix_builtin_fn g_pix_fns[ FN_NUM ] = 
{ //FNs

    //Containers (memory management):

    fn_new_pixi,
    fn_remove_pixi,
    fn_remove_pixi_with_alpha,
    fn_resize_pixi,
    fn_rotate_pixi,
    fn_clean_pixi,
    fn_clone_pixi,
    fn_copy_pixi,
    fn_get_pixi_info,
    fn_get_pixi_info,
    fn_get_pixi_info,
    fn_get_pixi_info,
    fn_get_pixi_info,
    fn_get_pixi_flags,
    fn_set_pixi_flags,
    fn_reset_pixi_flags,
    fn_get_pixi_prop_OR_set_pixi_prop,
    fn_get_pixi_prop_OR_set_pixi_prop,
    fn_remove_pixi_prop,
    fn_remove_pixi_props,
    fn_get_pixi_proplist,
    fn_remove_pixi_proplist,
    fn_convert_pixi_type,
    fn_show_smem_debug_messages,
    fn_zlib_pack,
    fn_zlib_unpack,

    //Working with strings:

    fn_num_to_string,
    fn_string_to_num,

    //Working with strings (posix):

    fn_strcat,
    fn_strcmp_OR_strstr,
    fn_strlen,
    fn_strcmp_OR_strstr,
    fn_sprintf,
    fn_sprintf,
    fn_sprintf,
    fn_sprintf,

    //Log management:

    fn_sprintf,
    fn_get_log,
    fn_get_system_log,

    //Files:

    fn_load,
    fn_fload,
    fn_save,
    fn_fsave,
    fn_get_real_path,
    fn_new_flist,
    fn_remove_flist,
    fn_get_flist_name,
    fn_get_flist_type,
    fn_flist_next,
    fn_get_file_size,
    fn_get_file_format,
    fn_get_fformat_mime_OR_ext,
    fn_get_fformat_mime_OR_ext,
    fn_remove_file,
    fn_rename_file,
    fn_copy_file,
    fn_create_directory,
    fn_set_disk0,
    fn_get_disk0,

    //Files (posix):

    fn_fopen,
    fn_fopen_mem,
    fn_fclose,
    fn_fputc,
    fn_fputs,
    fn_fgets_OR_fwrite_OR_fread,
    fn_fgetc,
    fn_fgets_OR_fwrite_OR_fread,
    fn_fgets_OR_fwrite_OR_fread,
    fn_feof_OF_fflush_OR_ftell,
    fn_feof_OF_fflush_OR_ftell,
    fn_fseek,
    fn_feof_OF_fflush_OR_ftell,
    fn_setxattr,

    //Graphics:

    fn_frame,
    fn_vsync,
    fn_set_pixel_size,
    fn_get_pixel_size,
    fn_set_screen,
    fn_get_screen,
    fn_set_zbuf,
    fn_get_zbuf,
    fn_clear_zbuf,
    fn_get_color,
    fn_get_red,
    fn_get_green,
    fn_get_blue,
    fn_get_blend,
    fn_transp,
    fn_get_transp,
    fn_clear,
    fn_dot,
    fn_dot,
    fn_get_dot,
    fn_get_dot,
    fn_line,
    fn_line,
    fn_box,
    fn_box,
    fn_pixi,
    fn_triangles3d,
    fn_sort_triangles3d,
    fn_set_key_color,
    fn_get_key_color,
    fn_set_alpha,
    fn_get_alpha,
    fn_print,
    fn_get_text_xysize,
    fn_get_text_xysize,
    fn_get_text_xysize,
    fn_set_font,
    fn_get_font,
    fn_effector,
    fn_color_gradient,
    fn_split_rgb,
    fn_split_rgb,

    //OpenGL:

    fn_set_gl_callback,
    fn_remove_gl_data,
    fn_update_gl_data,
    fn_gl_draw_arrays,
    fn_gl_blend_func,
    fn_gl_bind_framebuffer,
    fn_gl_bind_texture,
    fn_gl_get_int,
    fn_gl_get_float,
    fn_gl_new_prog,
    fn_gl_use_prog,
    fn_gl_uniform,
    fn_gl_uniform_matrix,

    //Animation:

    fn_pixi_unpack_frame,
    fn_pixi_pack_frame,
    fn_pixi_create_anim,
    fn_pixi_remove_anim,
    fn_pixi_clone_frame,
    fn_pixi_remove_frame,
    fn_pixi_play,
    fn_pixi_stop,

    //Video (not finished):

    fn_video_open,
    fn_video_close,
    fn_video_start,
    fn_video_stop,
    fn_video_props,
    fn_video_props,
    fn_video_capture_frame,

    //Transformation:

    fn_t_reset,
    fn_t_rotate,
    fn_t_translate,
    fn_t_scale,
    fn_t_push_matrix,
    fn_t_pop_matrix,
    fn_t_get_matrix,
    fn_t_set_matrix,
    fn_t_mul_matrix,
    fn_t_point,

    //Audio:

    fn_set_audio_callback,
    fn_enable_audio_input,
    fn_get_audio_sample_rate,
    fn_get_note_freq,

    //MIDI:

    fn_midi_open_client,
    fn_midi_close_client,
    fn_midi_get_device,
    fn_midi_open_port,
    fn_midi_reopen_port,
    fn_midi_close_port,
    fn_midi_get_event,
    fn_midi_get_event_time,
    fn_midi_next_event,
    fn_midi_send_event,

    //SunVox:

    fn_sv_new,
    fn_sv_remove,
    fn_sv_get_sample_rate,
    fn_sv_render,
    fn_sv_lock_unlock,
    fn_sv_lock_unlock,
    fn_sv_load_fload,
    fn_sv_load_fload,
    fn_sv_save_fsave,
    fn_sv_save_fsave,
    fn_sv_play,
    fn_sv_stop,
    fn_sv_pause,
    fn_sv_resume,
    fn_sv_sync_resume,
    fn_sv_set_autostop,
    fn_sv_get_autostop,
    fn_sv_get_status,
    fn_sv_rewind,
    fn_sv_volume,
    fn_sv_set_event_t,
    fn_sv_send_event,
    fn_sv_get_current_line,
    fn_sv_get_current_line,
    fn_sv_get_current_signal_level,
    fn_sv_get_name,
    fn_sv_set_name,
    fn_sv_get_bpm,
    fn_sv_get_bpm,
    fn_sv_get_len,
    fn_sv_get_len,
    fn_sv_get_time_map,
    fn_sv_new_module,
    fn_sv_remove_module,
    fn_sv_connect_disconnect_module,
    fn_sv_connect_disconnect_module,
    fn_sv_fload_module,
    fn_sv_fload_module,
    fn_sv_mod_fload,
    fn_sv_mod_fload,
    fn_sv_sampler_par,
    fn_sv_mod_fload,
    fn_sv_mod_fload,
    fn_sv_mod_fload,
    fn_sv_mod_fload,
    fn_sv_get_number_of_modules,
    fn_sv_find_module,
    fn_sv_selected_module,
    fn_sv_get_module_flags,
    fn_sv_get_module_inputs,
    fn_sv_get_module_inputs,
    fn_sv_get_module_type,
    fn_sv_get_module_name,
    fn_sv_set_module_name,
    fn_sv_get_module_xy,
    fn_sv_set_module_xy,
    fn_sv_get_module_color,
    fn_sv_set_module_color,
    fn_sv_get_module_finetune,
    fn_sv_set_module_finetune,
    fn_sv_set_module_relnote,
    fn_sv_get_module_scope,
    fn_sv_module_curve,
    fn_sv_get_module_ctl_cnt,
    fn_sv_get_module_ctl_name,
    fn_sv_get_module_ctl_value,
    fn_sv_set_module_ctl_value,
    fn_sv_get_module_ctl_par,
    fn_sv_get_module_ctl_par,
    fn_sv_get_module_ctl_par,
    fn_sv_get_module_ctl_par,
    fn_sv_get_module_ctl_par,
    fn_sv_new_pat,
    fn_sv_remove_pat,
    fn_sv_get_number_of_pats,
    fn_sv_find_pattern,
    fn_sv_get_pat,
    fn_sv_get_pat,
    fn_sv_set_pat_xy,
    fn_sv_get_pat,
    fn_sv_get_pat,
    fn_sv_set_pat_size,
    fn_sv_get_pat,
    fn_sv_set_pat_name,
    fn_sv_get_pat,
    fn_sv_set_pat_event,
    fn_sv_get_pat_event,
    fn_sv_pat_mute,

    //Time:

    fn_start_timer,
    fn_get_timer,
    fn_get_year,
    fn_get_month,
    fn_get_day,
    fn_get_hours,
    fn_get_minutes,
    fn_get_seconds,
    fn_get_ticks,
    fn_get_tps,
    fn_sleep,

    //Events:

    fn_get_event,
    fn_set_quit_action,

    //Threads:

    fn_thread_create,
    fn_thread_destroy,
    fn_mutex_create,
    fn_mutex_destroy,
    fn_mutex_lock,
    fn_mutex_trylock,
    fn_mutex_unlock,

    //Mathematical functions:

    fn_acos,
    fn_acosh,
    fn_asin,
    fn_asinh,
    fn_atan,
    fn_atanh,
    fn_atan2,
    fn_ceil,
    fn_cos,
    fn_cosh,
    fn_exp,
    fn_exp2,
    fn_expm1,
    fn_abs,
    fn_floor,
    fn_mod,
    fn_log,
    fn_log2,
    fn_log10,
    fn_pow,
    fn_sin,
    fn_sinh,
    fn_sqrt,
    fn_tan,
    fn_tanh,
    fn_rand,
    fn_rand_seed,

    //Type punning:

    fn_reinterpret_type,

    //Data processing:

    fn_op_cn,
    fn_op_cc,
    fn_op_ccn,
    fn_generator,
    fn_wavetable_generator,
    fn_sampler,
    fn_envelope2p,
    fn_gradient,
    fn_fft,
    fn_new_filter,
    fn_remove_filter,
    fn_init_filter,
    fn_reset_filter,
    fn_apply_filter,
    fn_replace_values,
    fn_copy_and_resize,
    fn_conv_filter,

    //Dialogs:

    fn_file_dialog,
    fn_prefs_dialog,
    fn_textinput_dialog,

    //Network:

    fn_system_copy_OR_open_url,

    //Native code:

    fn_dlopen,
    fn_dlclose,
    fn_dlsym,
    fn_dlcall,

    //Posix compatibility:

    fn_system,
    fn_argc,
    fn_argv,
    fn_exit,

    //Experimental API:

    fn_webserver_dialog,
    fn_midiopt_dialog,
    fn_system_copy_OR_open_url,
    fn_system_paste,
    fn_send_file_to,
    fn_export_import_file,
    fn_set_audio_play_status,
    fn_get_audio_event,
    fn_openclose_app_state,
    fn_openclose_app_state,
    fn_wm_video_capture_start_OR_stop,
    fn_wm_video_capture_start_OR_stop,
    fn_wm_video_capture_get_ext,
    fn_wm_video_capture_encode,
}; //FNs

//
// Containers (memory management)
//

void fn_new_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_INT xsize = 1;
    PIX_INT ysize = 1;
    int type = 32;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( xsize, 0, PIX_INT );
    if( pars_num >= 2 ) GET_VAL_FROM_STACK( ysize, 1, PIX_INT );
    if( pars_num >= 3 ) GET_VAL_FROM_STACK( type, 2, int );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_new_container( -1, xsize, ysize, type, 0, vm );
}

void fn_remove_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_remove_container( cnum, vm );
    }
}

void fn_remove_pixi_with_alpha( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	PIX_CID a = pix_vm_get_container_alpha( cnum, vm );
	pix_vm_remove_container( a, vm );
	pix_vm_remove_container( cnum, vm );
    }
}

void fn_resize_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 2 ) return;
    PIX_CID cnum;
    PIX_INT xsize = -1;
    PIX_INT ysize = -1;
    int type = -1;
    uint flags = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    GET_VAL_FROM_STACK( xsize, 1, PIX_INT );
    if( pars_num >= 3 ) GET_VAL_FROM_STACK( ysize, 2, PIX_INT );
    if( pars_num >= 4 ) GET_VAL_FROM_STACK( type, 3, int );
    if( pars_num >= 5 ) GET_VAL_FROM_STACK( flags, 4, int );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_resize_container( cnum, xsize, ysize, type, flags, vm );
}

void fn_rotate_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 2 ) return;
    PIX_CID cnum;
    int angle = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    GET_VAL_FROM_STACK( angle, 1, int );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_rotate_container( cnum, angle, vm );
}

void fn_clean_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    if( pars_num > 0 ) 
    {
	PIX_INT offset = 0;
	PIX_INT size = -1;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( pars_num > 1 )
	{
	    if( pars_num > 2 ) { GET_VAL_FROM_STACK( offset, 2, PIX_INT ); }
	    if( pars_num > 3 ) { GET_VAL_FROM_STACK( size, 3, PIX_INT ); }
	    pix_vm_clean_container( cnum, stack[ PIX_CHECK_SP( sp + 1 ) ].t, stack[ PIX_CHECK_SP( sp + 1 ) ].v, offset, size, vm );
	}
	else 
	{
	    PIX_VAL v;
	    v.i = 0;
	    pix_vm_clean_container( cnum, 0, v, 0, -1, vm );
	}
    }
}

void fn_clone_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	stack[ sp2 ].i = pix_vm_clone_container( cnum, vm );
    }
    else
    {
	stack[ sp2 ].i = -1;
    }
}

void fn_copy_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID cnum1; //destination
	PIX_CID cnum2; //source
	uint flags = 0;
	GET_VAL_FROM_STACK( cnum1, 0, PIX_CID );
	GET_VAL_FROM_STACK( cnum2, 1, PIX_CID );
	if( pars_num >= 8 ) GET_VAL_FROM_STACK( flags, 7, size_t );
	if( (unsigned)cnum1 >= (unsigned)vm->c_num ) goto copy_end;
	pix_vm_container* cont1 = vm->c[ cnum1 ];
#ifdef OPENGL
	if( cnum2 == PIX_GL_SCREEN )
	{
	    if( g_pix_container_type_sizes[ cont1->type ] == COLORLEN )
	    {
		int format = -1;
		int type = -1;
		if( COLORLEN == 4 ) { format = GL_RGBA; type = GL_UNSIGNED_BYTE; }
		if( COLORLEN == 2 ) { format = GL_RGB; type = GL_UNSIGNED_SHORT_5_6_5; }
		if( format >= 0 && type >= 0 )
		{
		    glReadPixels( 
			0, 
			0, 
			cont1->xsize, 
			cont1->ysize,
			format, type, cont1->data );
		    if( ( flags & PIX_COPY_NO_AUTOROTATE ) == 0 )
		    {
			COLORPTR src1 = (COLORPTR)cont1->data;
			COLORPTR src2 = src1 + cont1->xsize * ( cont1->ysize - 1 );
			for( int y = 0; y < cont1->ysize / 2; y++ )
			{
			    for( int x = 0; x < cont1->xsize; x++ )
			    {
				COLOR temp = *src2;
				*src2 = *src1;
				*src1 = temp;
				src1++;
				src2++;
			    }
			    src2 -= cont1->xsize * 2;
			}
		    }
		    rv = 0;
		}
		goto copy_end;
	    }
	}
#endif
	if( (unsigned)cnum2 >= (unsigned)vm->c_num ) goto copy_end;
	pix_vm_container* cont2 = vm->c[ cnum2 ];
	if( !cont1 || !cont2 ) goto copy_end;

	size_t dest_offset;
	size_t src_offset;
	size_t count;
	size_t dest_step;
	size_t src_step;
	if( pars_num >= 3 ) { GET_VAL_FROM_STACK( dest_offset, 2, size_t ); } else { dest_offset = 0; }
	if( pars_num >= 4 ) { GET_VAL_FROM_STACK( src_offset, 3, size_t ); } else { src_offset = 0; }
	if( pars_num >= 5 ) { GET_VAL_FROM_STACK( count, 4, size_t ); } else { count = cont1->size; }
	if( pars_num >= 6 ) { GET_VAL_FROM_STACK( dest_step, 5, size_t ); } else { dest_step = 1; }
	if( pars_num >= 7 ) { GET_VAL_FROM_STACK( src_step, 6, size_t ); } else { src_step = 1; }
	if( dest_offset >= cont1->size ) goto copy_end;
	if( src_offset >= cont2->size ) goto copy_end;
	if( count == 0 ) goto copy_end;
	size_t last_element_ptr = dest_offset + ( count - 1 ) * dest_step;
	if( last_element_ptr >= cont1->size )
	{
	    count = ( cont1->size - dest_offset ) / dest_step;
	    if( ( cont1->size - dest_offset ) % dest_step ) count++;
	}
	last_element_ptr = src_offset + ( count - 1 ) * src_step;
	if( last_element_ptr >= cont2->size )
	{
	    count = ( cont2->size - src_offset ) / src_step;
	    if( ( cont2->size - src_offset ) % src_step ) count++;
	}

	rv = (PIX_INT)count;

	if( cont1->type == cont2->type && dest_step == 1 && src_step == 1 )
	{
	    smem_copy( 
		     (int8_t*)cont1->data + dest_offset * g_pix_container_type_sizes[ cont1->type ], 
		     (int8_t*)cont2->data + src_offset * g_pix_container_type_sizes[ cont1->type ], 
		     count * g_pix_container_type_sizes[ cont1->type ] 
		    );
	    rv = 0;
	}
	else
	{
	    if( ( flags & PIX_COPY_CLIPPING ) && cont1->type < PIX_CONTAINER_TYPE_INT32 )
	    {
		for( PIX_INT i = dest_offset, i2 = src_offset; i < dest_offset + count * dest_step; i += dest_step, i2 += src_step )
		{
		    PIX_INT val;
		    switch( cont2->type )
		    {
		        case PIX_CONTAINER_TYPE_INT8: val = ( (int8_t*)cont2->data )[ i2 ]; break;
		        case PIX_CONTAINER_TYPE_INT16: val = ( (int16_t*)cont2->data )[ i2 ]; break;
		        case PIX_CONTAINER_TYPE_INT32: val = ( (int32_t*)cont2->data )[ i2 ]; break;
#ifdef PIX_INT64_ENABLED
		        case PIX_CONTAINER_TYPE_INT64: val = ( (int64_t*)cont2->data )[ i2 ]; break;
#endif
		        case PIX_CONTAINER_TYPE_FLOAT32: val = ( (float*)cont2->data )[ i2 ]; break;
#ifdef PIX_FLOAT64_ENABLED
		        case PIX_CONTAINER_TYPE_FLOAT64: val = ( (double*)cont2->data )[ i2 ]; break;
#endif
		        default: val = 0; break;
		    }
		    if( cont1->type == PIX_CONTAINER_TYPE_INT8 )
		    {
		        if( val < -128 ) 
		    	    val = -128;
			else if( val > 127 ) 
			    val = 127;
			( (int8_t*)cont1->data )[ i ] = val;
		    }
		    else
		    {
		        if( val < -32768 )
			    val = -32768;
			else if( val > 32767 ) 
			    val = 32767;
			( (int16_t*)cont1->data )[ i ] = val;
		    }
		}
		rv = 0;
	    }
	    else
	    {
		if( cont2->type < PIX_CONTAINER_TYPE_FLOAT32 )
		{
		    //INT source:
		    for( PIX_INT i = dest_offset, i2 = src_offset; i < dest_offset + count * dest_step; i += dest_step, i2 += src_step )
		    {
			PIX_INT val;
			switch( cont2->type )
			{
			    case PIX_CONTAINER_TYPE_INT8: val = ( (int8_t*)cont2->data )[ i2 ]; break;
			    case PIX_CONTAINER_TYPE_INT16: val = ( (int16_t*)cont2->data )[ i2 ]; break;
			    case PIX_CONTAINER_TYPE_INT32: val = ( (int32_t*)cont2->data )[ i2 ]; break;
#ifdef PIX_INT64_ENABLED
			    case PIX_CONTAINER_TYPE_INT64: val = ( (int64_t*)cont2->data )[ i2 ]; break;
#endif
			    default: val = 0; break;
			}
			switch( cont1->type )
			{
			    case PIX_CONTAINER_TYPE_INT8: ( (int8_t*)cont1->data )[ i ] = val; break;
			    case PIX_CONTAINER_TYPE_INT16: ( (int16_t*)cont1->data )[ i ] = val; break;
			    case PIX_CONTAINER_TYPE_INT32: ( (int32_t*)cont1->data )[ i ] = val; break;
#ifdef PIX_INT64_ENABLED
			    case PIX_CONTAINER_TYPE_INT64: ( (int64_t*)cont1->data )[ i ] = val; break;
#endif
			    case PIX_CONTAINER_TYPE_FLOAT32: ( (float*)cont1->data )[ i ] = val; break;
#ifdef PIX_FLOAT64_ENABLED
			    case PIX_CONTAINER_TYPE_FLOAT64: ( (double*)cont1->data )[ i ] = val; break;
#endif
			    default: break;
			}
		    }
		}
		else
		{
		    //FLOAT source:
		    for( PIX_INT i = dest_offset, i2 = src_offset; i < dest_offset + count * dest_step; i += dest_step, i2 += src_step )
		    {
			PIX_FLOAT val;
			switch( cont2->type )
			{
			    case PIX_CONTAINER_TYPE_FLOAT32: val = ( (float*)cont2->data )[ i2 ]; break;
#ifdef PIX_FLOAT64_ENABLED
			    case PIX_CONTAINER_TYPE_FLOAT64: val = ( (double*)cont2->data )[ i2 ]; break;
#endif
			    default: val = 0; break;
			}
			switch( cont1->type )
			{
			    case PIX_CONTAINER_TYPE_INT8: ( (int8_t*)cont1->data )[ i ] = val; break;
			    case PIX_CONTAINER_TYPE_INT16: ( (int16_t*)cont1->data )[ i ] = val; break;
			    case PIX_CONTAINER_TYPE_INT32: ( (int32_t*)cont1->data )[ i ] = val; break;
#ifdef PIX_INT64_ENABLED
			    case PIX_CONTAINER_TYPE_INT64: ( (int64_t*)cont1->data )[ i ] = val; break;
#endif
			    case PIX_CONTAINER_TYPE_FLOAT32: ( (float*)cont1->data )[ i ] = val; break;
#ifdef PIX_FLOAT64_ENABLED
			    case PIX_CONTAINER_TYPE_FLOAT64: ( (double*)cont1->data )[ i ] = val; break;
#endif
			    default: break;
			}
		    }
		}
		rv = 0;
	    }
	}
    }

copy_end:

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_get_pixi_info( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID cnum;
    PIX_INT rv = 0;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
#ifdef OPENGL
	if( cnum == PIX_GL_SCREEN )
    	{
    	    switch( fn_num )
    	    {
	        case FN_GET_PIXI_SIZE: rv = vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i * vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i; break;
	        case FN_GET_PIXI_XSIZE: rv = vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i; break;
	        case FN_GET_PIXI_YSIZE: rv = vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i; break;
		case FN_GET_PIXI_ESIZE: rv = COLORLEN; break;
		case FN_GET_PIXI_TYPE: 
		    if( COLORLEN == 2 ) rv = PIX_CONTAINER_TYPE_INT16; 
		    if( COLORLEN == 4 ) rv = PIX_CONTAINER_TYPE_INT32; 
		    break;
		default: break;
    	    }
    	    goto get_info_end;
    	}
#endif
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    switch( fn_num )
	    {
	        case FN_GET_PIXI_SIZE: rv = c->xsize * c->ysize; break;
	        case FN_GET_PIXI_XSIZE: rv = c->xsize; break;
	        case FN_GET_PIXI_YSIZE: rv = c->ysize; break;
	        case FN_GET_PIXI_ESIZE: rv = g_pix_container_type_sizes[ c->type ]; break;
	        case FN_GET_PIXI_TYPE: rv = c->type; break;
		default: break;
	    }
	}
    }
get_info_end:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_get_pixi_flags( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    uint flags = 0;
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    flags = c->flags;
	}
    }
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = flags;
}

void fn_set_pixi_flags( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    uint flags;
    if( pars_num >= 2 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( flags, 1, int );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    volatile uint new_flags = c->flags | flags;
	    c->flags = new_flags;
	}
    }
}

void fn_reset_pixi_flags( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID cnum;
    uint flags;
    if( pars_num >= 2 ) 
    {
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( flags, 1, int );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    volatile uint new_flags = c->flags & ~flags;
	    c->flags = new_flags;
	}
    }
}

void fn_get_pixi_prop_OR_set_pixi_prop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	PIX_CID prop_name;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( prop_name, 1, PIX_CID );
	bool prop_name_str_ = false;
	char* prop_name_str = pix_vm_make_cstring_from_container( prop_name, &prop_name_str_, vm );
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	if( fn_num == FN_GET_PIXI_PROP )
	{
	    //Get:
	    if( pars_num > 2 )
	    {
		stack[ sp2 ] = stack[ PIX_CHECK_SP( sp + 2 ) ];
	    }
	    else
	    {
		stack[ sp2 ].t = 0;
		stack[ sp2 ].i = 0;
	    }

	    pix_sym* sym = pix_vm_get_container_property( cnum, prop_name_str, -1, vm );

	    if( sym && sym->type != SYMTYPE_DELETED )
	    {
		if( sym->type == SYMTYPE_NUM_F )
		    stack[ sp2 ].t = 1;
		stack[ sp2 ].v = sym->val;
	    }
	}
	else
	{
	    //Set:
	    pix_vm_set_container_property( cnum, prop_name_str, -1, stack[ PIX_CHECK_SP( sp + 2 ) ].t, stack[ PIX_CHECK_SP( sp + 2 ) ].v, vm );
	}
	if( prop_name_str_ ) smem_free( prop_name_str );
    }
}

void fn_remove_pixi_prop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	PIX_CID prop_name;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( prop_name, 1, PIX_CID );
	bool prop_name_str_ = false;
	char* prop_name_str = pix_vm_make_cstring_from_container( prop_name, &prop_name_str_, vm );
	pix_sym* sym = pix_vm_get_container_property( cnum, prop_name_str, -1, vm );
	if( sym )
	{
	    sym->type = SYMTYPE_DELETED;
	    sym->val.i = 0;
        }
	if( prop_name_str_ ) smem_free( prop_name_str );
    }
}

void fn_remove_pixi_props( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && c->opt_data )
	{
	    pix_symtab_deinit( &c->opt_data->props );
	}
    }
}

void fn_get_pixi_proplist( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;
    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && c->opt_data )
	{
	    pix_sym* ss = pix_symtab_get_list( &c->opt_data->props );
	    if( ss )
	    {
		int cnt = smem_get_size( ss ) / sizeof( pix_sym );
		rv = pix_vm_new_container( -1, cnt, 1, PIX_CONTAINER_TYPE_INT32, NULL, vm );
		if( rv )
		{
		    for( int i = 0; i < cnt; i++ )
		    {
			PIX_CID name = pix_vm_make_container_from_cstring( ss[ i ].name, vm );
			pix_vm_set_container_int_element( rv, i, name, vm );
		    }
		}
		smem_free( ss );
	    }
	}
    }
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_remove_pixi_proplist( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && c->opt_data )
	{
	    int cnt = c->xsize * c->ysize;
	    for( int i = 0; i < cnt; i++ )
	    {
		PIX_CID name = pix_vm_get_container_int_element( cnum, i, vm );
		pix_vm_remove_container( name, vm );
	    }
	    pix_vm_remove_container( cnum, vm );
	}
    }
}

void fn_convert_pixi_type( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = 1;
    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int type;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( type, 1, int );

	rv = pix_vm_convert_container_type( cnum, type, vm );
    }
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_show_smem_debug_messages( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER; 
   
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( vm->c_show_debug_messages, 0, bool );
    }
}

void fn_zlib_pack( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;
    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	int level = -1;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( pars_num >= 2 ) GET_VAL_FROM_STACK( level, 1, int );
	
	rv = pix_vm_zlib_pack_container( cnum, level, vm );
    }
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_zlib_unpack( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;
    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	
	rv = pix_vm_zlib_unpack_container( cnum, vm );
    }
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//
// Working with strings
//

void fn_num_to_string( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int radix = 10;
	PIX_INT str_offset = 0;
	bool no_null_term = false;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( pars_num >= 3 ) GET_VAL_FROM_STACK( radix, 2, int );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( str_offset, 3, PIX_INT );
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( no_null_term, 4, bool );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num && vm->c[ cnum ] )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    char ts[ 128 ];
	    if( stack[ PIX_CHECK_SP( sp + 1 ) ].t == 0 )
	    {
		//int:
		switch( radix )
		{
		    case 16: hex_int_to_string( stack[ PIX_CHECK_SP( sp + 1 ) ].i, ts ); break;
		    default: int_to_string( stack[ PIX_CHECK_SP( sp + 1 ) ].i, ts ); break;
		}
	    }
	    else
	    {
		//float:
		snprintf( ts, sizeof( ts ), "%f", (float)stack[ PIX_CHECK_SP( sp + 1 ) ].f );
	    }
	    char* ts2 = ts;
	    PIX_INT ts_len = (PIX_INT)smem_strlen( ts );
	    PIX_INT size = c->size * g_pix_container_type_sizes[ c->type ];
	    if( str_offset + ts_len > size )
	    {
		if( pix_vm_resize_container( cnum, str_offset + ts_len, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) return;
		for( PIX_INT i = size; i < str_offset; i++ ) ((char*)c->data)[ i ] = ' ';
		size = str_offset + ts_len;
	    }
	    if( str_offset < 0 )
	    {
		ts_len += str_offset;
		ts2 -= str_offset;
		str_offset = 0;
	    }
	    if( ts_len > 0 )
	    {
	        smem_copy( (char*)c->data + str_offset, ts2, ts_len );
		if( !no_null_term )
		    if( str_offset + ts_len < size )
			((char*)c->data)[ str_offset + ts_len ] = 0;
	    }
	}
    }
}

void fn_string_to_num( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_VAL rv;
    int8_t rv_t;
    rv.i = 0;
    rv_t = 0;

    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	if( vm->c && (unsigned)cnum < (unsigned)vm->c_num )
	{
	    pix_vm_container* c = vm->c[ cnum ];
	    if( c && c->data )
	    {
		const char* str = (const char*)c->data;
		PIX_INT size = c->size * g_pix_container_type_sizes[ c->type ];

		PIX_INT str_offset = 0;
		PIX_INT str_len = size;
		if( pars_num >= 2 ) GET_VAL_FROM_STACK( str_offset, 1, PIX_INT );
		if( pars_num >= 3 ) GET_VAL_FROM_STACK( str_len, 2, PIX_INT );

		if( str_offset )
		{
		    if( str_offset < 0 ) str_offset = 0;
		    str += str_offset;
		}
		if( str_offset + str_len > size ) str_len = size - str_offset;
		if( str_len > 0 )
		{
		    PIX_INT l;
		    for( l = 0; l < str_len; l++ )
			if( str[ l ] == 0 ) break;
		    pix_str_to_num( str, l, &rv, &rv_t, vm );
		}
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].v = rv;
    stack[ sp2 ].t = rv_t;
}

//
// Working with strings (posix)
//

//Appends a copy of the source string to the destination string:
void fn_strcat( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID s1;
    PIX_CID s2;
    pix_vm_container* s1_cont;
    pix_vm_container* s2_cont;
    PIX_INT off1 = 0;
    PIX_INT off2 = 0;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( s1, 0, PIX_CID );
	if( pars_num >= 3 )
	{
	    GET_VAL_FROM_STACK( off1, 1, PIX_INT );
	    GET_VAL_FROM_STACK( s2, 2, PIX_CID );
	    if( pars_num >= 4 )
	    {
		GET_VAL_FROM_STACK( off2, 3, PIX_INT );
	    }
	}
	else
	{
	    GET_VAL_FROM_STACK( s2, 1, PIX_CID );
	}
	if( (unsigned)s1 >= (unsigned)vm->c_num ) { err = 1; break; }
	s1_cont = vm->c[ s1 ];
	if( (unsigned)s2 >= (unsigned)vm->c_num ) { err = 1; break; }
	s2_cont = vm->c[ s2 ];
	if( s1_cont == 0 ) { err = 1; break; }
	if( s2_cont == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    while( err == 0 )
    {
	size_t s1_size = s1_cont->size * g_pix_container_type_sizes[ s1_cont->type ];
	size_t s2_size = s2_cont->size * g_pix_container_type_sizes[ s2_cont->type ];
	if( off2 >= s2_size ) break;
	s2_size -= off2;
	size_t s1_len;
	size_t s2_len;
	char* s1_ptr = (char*)s1_cont->data;
	char* s2_ptr = (char*)s2_cont->data;
	s2_ptr += off2;
	for( s2_len = 0; s2_len < s2_size; s2_len++ )
	    if( s2_ptr[ s2_len ] == 0 ) break;
	if( s2_len == 0 ) break;
	if( off1 >= s1_size )
	{
	    s1_size = off1 + s2_len;
	    if( pix_vm_resize_container( s1, (PIX_INT)( s1_size ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) break;
	    s1_size -= off1;
	    s1_ptr = (char*)s1_cont->data;
	    s1_ptr += off1;
	    smem_copy( s1_ptr, s2_ptr, s2_len );
	}
	else
	{
	    s1_size -= off1;
	    s1_ptr += off1;
	    for( s1_len = 0; s1_len < s1_size; s1_len++ )
		if( s1_ptr[ s1_len ] == 0 ) break;
	    if( s1_len + s2_len > s1_size )
	    {
		if( pix_vm_resize_container( s1, (PIX_INT)( off1 + s1_len + s2_len ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) break;
		s1_ptr = (char*)s1_cont->data;
		s1_ptr += off1;
		s1_size = s1_len + s2_len;
	    }
	    else 
	    {
		if( s1_len + s2_len < s1_size )
		    s1_ptr[ s1_len + s2_len ] = 0;
	    }
	    smem_copy( s1_ptr + s1_len, s2_ptr, s2_len );
	}
	break;
    }
}

//Compare two strings /
//Returns a offset of the first occurrence of str2 in str1, or -1 if str2 is not part of str1:
void fn_strcmp_OR_strstr( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID s1;
    PIX_CID s2;
    pix_vm_container* s1_cont;
    pix_vm_container* s2_cont;
    PIX_INT off1 = 0;
    PIX_INT off2 = 0;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( s1, 0, PIX_CID );
	if( pars_num >= 3 )
	{
	    GET_VAL_FROM_STACK( off1, 1, PIX_INT );
	    GET_VAL_FROM_STACK( s2, 2, PIX_CID );
	    if( pars_num >= 4 )
	    {
		GET_VAL_FROM_STACK( off2, 3, PIX_INT );
	    }
	}
	else
	{
	    GET_VAL_FROM_STACK( s2, 1, PIX_CID );
	}
	if( (unsigned)s1 >= (unsigned)vm->c_num ) { err = 1; break; }
	s1_cont = vm->c[ s1 ];
	if( (unsigned)s2 >= (unsigned)vm->c_num ) { err = 1; break; }
	s2_cont = vm->c[ s2 ];
	if( s1_cont == 0 ) { err = 1; break; }
	if( s2_cont == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    if( err == 0 )
    {
	size_t s1_size = s1_cont->size * g_pix_container_type_sizes[ s1_cont->type ];
	size_t s2_size = s2_cont->size * g_pix_container_type_sizes[ s2_cont->type ];
	if( off1 > s1_size ) { s1_size = 0; off1 = 0; }
	if( off2 > s2_size ) { s2_size = 0; off2 = 0; }
	s1_size -= off1;
	s2_size -= off2;
	size_t s1_len;
	size_t s2_len;
	char* s1_ptr = (char*)s1_cont->data;
	char* s2_ptr = (char*)s2_cont->data;
	s1_ptr += off1;
	s2_ptr += off2;
	for( s1_len = 0; s1_len < s1_size; s1_len++ )
	    if( s1_ptr[ s1_len ] == 0 ) break;
	for( s2_len = 0; s2_len < s2_size; s2_len++ )
	    if( s2_ptr[ s2_len ] == 0 ) break;
	char* s1_str = 0;
	char* s2_str = 0;
	if( s1_len == s1_size )
	{
	    s1_str = (char*)smem_new( s1_len + 1 );
	    smem_copy( s1_str, s1_ptr, s1_len );
	    s1_str[ s1_len ] = 0;
	}
	else 
	{
	    s1_str = s1_ptr;
	}
	if( s2_len == s2_size )
	{
	    s2_str = (char*)smem_new( s2_len + 1 );
	    smem_copy( s2_str, s2_ptr, s2_len );
	    s2_str[ s2_len ] = 0;
	}
	else 
	{
	    s2_str = s2_ptr;
	}
	if( fn_num == FN_STRCMP )
	{
	    stack[ sp2 ].i = smem_strcmp( s1_str, s2_str );
	}
	else 
	{
	    char* substr = strstr( s1_str, s2_str );
	    if( substr == 0 )
		stack[ sp2 ].i = -1;
	    else
		stack[ sp2 ].i = (PIX_INT)( substr - s1_str ) + off1;
	}
	if( s1_len == s1_size ) smem_free( s1_str );
	if( s2_len == s2_size ) smem_free( s2_str );
    }
    else 
    {
	stack[ sp2 ].i = -1;
    }
}

//Returns the length of the string:
void fn_strlen( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID str;
    PIX_INT off = 0;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );

    //Get parameters:
    if( pars_num < 1 ) 
    { 
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = 0;
	return;
    }
    
    GET_VAL_FROM_STACK( str, 0, PIX_CID );
    if( pars_num >= 2 )
    {
	GET_VAL_FROM_STACK( off, 1, PIX_INT );
    }
    
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (PIX_INT)pix_vm_get_container_strlen( str, (size_t)off, vm );
}

//Writes into the array pointed by str a C string consisting on a sequence of data formatted as the format argument specifies:
void fn_sprintf( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    bool err = 0;
    int i2;

    sfs_file dest_stream = 0;
    PIX_CID str = -1;
    PIX_CID format = -1;
    pix_vm_container* str_cont = NULL;
    pix_vm_container* format_cont = NULL;
    bool no_null_term = false;
    PIX_INT str_ptr = 0;
    int args_off;

    //Get parameters:
    if( fn_num == FN_SPRINTF || fn_num == FN_SPRINTF2 )
    {
	//sprintf:
	while( 1 )
	{
	    GET_VAL_FROM_STACK( str, 0, PIX_CID );
	    if( fn_num == FN_SPRINTF )
	    {
		if( pars_num < 2 ) { err = 1; break; }
		GET_VAL_FROM_STACK( format, 1, PIX_CID );
		args_off = 2;
	    }
	    else
	    {
		//sprintf2:
		if( pars_num < 4 ) { err = 1; break; }
		GET_VAL_FROM_STACK( str_ptr, 1, PIX_INT );
		GET_VAL_FROM_STACK( no_null_term, 2, bool );
		GET_VAL_FROM_STACK( format, 3, PIX_CID );
		if( str_ptr < 0 ) str_ptr = 0;
		args_off = 4;
		fn_num = FN_SPRINTF;
	    }
	    if( (unsigned)str >= (unsigned)vm->c_num ) { err = 1; break; }
	    if( (unsigned)format >= (unsigned)vm->c_num ) { err = 1; break; }
	    str_cont = vm->c[ str ];
	    format_cont = vm->c[ format ];
	    if( !str_cont ) { err = 1; break; }
	    if( !format_cont ) { err = 1; break; }
	    break;
	}
    }
    else
    {
	//printf or fprintf
	while( 1 )
	{
	    if( fn_num == FN_PRINTF || fn_num == FN_LOGF )
	    {
		if( pars_num < 1 ) { err = 1; break; }
		GET_VAL_FROM_STACK( format, 0, PIX_CID );
		args_off = 1;
	    }
	    else
	    {
		//fprintf
		if( pars_num < 2 ) { err = 1; break; }
		GET_VAL_FROM_STACK( dest_stream, 0, sfs_file );
		GET_VAL_FROM_STACK( format, 1, PIX_CID );
		args_off = 2;
	    }
	    if( (unsigned)format >= (unsigned)vm->c_num ) { err = 1; break; }
	    format_cont = vm->c[ format ];
	    if( !format_cont ) { err = 1; break; }
	    break;
	}
    }

    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	PIX_INT str_size;
	char* cstr = NULL;
	if( fn_num == FN_SPRINTF )
	{
	    //sprintf:
	    str_size = str_cont->size * g_pix_container_type_sizes[ format_cont->type ];
	    if( str_ptr >= str_size )
	    {
		PIX_INT new_size = str_ptr + 8;
		if( pix_vm_resize_container( str, new_size, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) goto sprintf_error;
		for( PIX_INT i = str_size; i < new_size; i++ )
		    ((char*)str_cont->data)[ i ] = ' ';
		str_size = new_size;
	    }
	}
	else
	{
	    //printf:
	    str_size = 256;
	    cstr = (char*)smem_new( str_size );
	    if( !cstr ) goto sprintf_error;
	}
	int arg_num = 0;
	char* format_str = (char*)format_cont->data;
	PIX_INT format_size = format_cont->size * g_pix_container_type_sizes[ format_cont->type ];
	for( PIX_INT i = 0; i < format_size; i++ )
	{
	    if( format_str[ i ] == 0 )
	    {
		format_size = i;
		break;
	    }
	}
	for( PIX_INT i = 0; i < format_size; i++ )
	{
	    char c = format_str[ i ];
	    bool c_to_output = 0;
	    if( c == '%' )
	    {
		i++;
		if( i >= format_size ) break;
		c = format_str[ i ];

		//Parse format:

		char flags[ 2 ];
		flags[ 0 ] = 0;
		flags[ 1 ] = 0;
		switch( c )
		{
		    case '-':
		    case '+':
		    case ' ':
		    case '#':
		    case '0':
			flags[ 0 ] = c;
			break;
		}
		if( flags[ 0 ] )
		{
		    i++;
		    if( i >= format_size ) break;
		    c = format_str[ i ];
		}

		char width[ 4 ];
		i2 = 0;
		while( 1 )
		{
		    if( ( c >= '0' && c <= '9' ) || c == '*' )
		    {
			if( i2 < 3 )
			    width[ i2 ] = c;
			i2++;
			i++;
			if( i >= format_size ) break;
			c = format_str[ i ];
		    }
		    else break;
		}
		if( i >= format_size ) break;
		if( i2 < 4 ) width[ i2 ] = 0;

		char prec[ 5 ];
		i2 = 0;
		while( 1 )
		{
		    if( ( c >= '0' && c <= '9' ) || c == '*' || c == '.' )
		    {
			if( i2 < 4 )
			    prec[ i2 ] = c;
			i2++;
			i++;
			if( i >= format_size ) break;
			c = format_str[ i ];
		    }
		    else break;
		}
		if( i >= format_size ) break;
		if( i2 < 5 ) prec[ i2 ] = 0;

		char len[ 2 ];
		len[ 0 ] = 0;
		len[ 1 ] = 0;
		switch( c )
		{
		    case 'h':
		    case 'l':
		    case 'L':
			len[ 0 ] = c;
			break;
		}
		if( len[ 0 ] )
		{
		    i++;
		    if( i >= format_size ) break;
		    c = format_str[ i ];
		}

		char specifier[ 2 ];
		specifier[ 0 ] = c;
		specifier[ 1 ] = 0;

		if( specifier[ 0 ] == '%' )
		{
		    c_to_output = 1;
		}
		else 
		{
		    char arg_format[ 24 ];
		    arg_format[ 0 ] = 0;
		    strcat( arg_format, "%" );
		    strcat( arg_format, flags );
		    strcat( arg_format, width );
		    strcat( arg_format, prec );
		    strcat( arg_format, len );
		    strcat( arg_format, specifier );
		    switch( specifier[ 0 ] )
		    {
			case 's': //String of characters:
			    {
				PIX_CID arg_str;
				GET_VAL_FROM_STACK( arg_str, args_off + arg_num, PIX_CID );
				arg_num++;
				if( (unsigned)arg_str >= (unsigned)vm->c_num ) break;
				pix_vm_container* arg_str_cont = vm->c[ arg_str ];
				if( arg_str_cont == 0 ) break;
				PIX_INT arg_str_size = pix_vm_get_container_strlen( arg_str, 0, vm );
				if( str_ptr + arg_str_size > str_size )
				{
				    if( fn_num == FN_SPRINTF )
				    {
					//sprintf:
					if( pix_vm_resize_container( str, str_ptr + arg_str_size + 8, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) 
					    break;
				    }
				    else
				    {
					//printf:
					cstr = (char*)smem_resize2( cstr, str_ptr + arg_str_size + 8 );
					if( cstr == 0 )
					    break;
				    }
				    str_size = str_ptr + arg_str_size + 8;
				}
				for( i2 = 0; i2 < arg_str_size; i2++ )
				{
				    if( fn_num == FN_SPRINTF )
				    {
					//sprintf:
					((char*)str_cont->data)[ str_ptr ] = ((char*)arg_str_cont->data)[ i2 ];
				    }
				    else
				    {
					//printf:
					cstr[ str_ptr ] = ((char*)arg_str_cont->data)[ i2 ];
				    }
				    str_ptr++;
				}
			    }
			    break;
			case 'c': //Character:
			case 'd': //Signed decimal integer:
			case 'i': //Signed decimal integer:
			case 'o': //Signed octal:
			case 'u': //Unsigned decimal integer:
			case 'x': //Unsigned hexadecimal integer:
			case 'X': //Unsigned hexadecimal integer (capital letters):
			    {
				PIX_INT arg_int;
				GET_VAL_FROM_STACK( arg_int, args_off + arg_num, PIX_INT );
				arg_num++;
				char ts[ 32 ];
				if( specifier[ 0 ] == 'c' )
				{
				    if( len[ 0 ] )
				    {
					//unicode char:
					uint32_t ts2[ 2 ];
					ts2[ 0 ] = arg_int;
					ts2[ 1 ] = 0;
					utf32_to_utf8( ts, sizeof( ts ), ts2 );
				    }
				    else
				    {
					//ascii char (byte):
					ts[ 0 ] = (char)arg_int;
					ts[ 1 ] = 0;
				    }
				}
				else
				{
				    snprintf( ts, sizeof( ts ), arg_format, (int)arg_int );
				}
				for( i2 = 0; i2 < 32; i2++ )
				{
				    if( ts[ i2 ] == 0 ) break;
				    if( str_ptr >= str_size )
				    {
					if( fn_num == FN_SPRINTF )
					{
					    //sprintf:
					    if( pix_vm_resize_container( str, str_ptr + 8, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) 
						break;
					}
					else
					{
					    //printf:
					    cstr = (char*)smem_resize2( cstr, str_ptr + 8 );
					    if( cstr == 0 ) break;
					}
					str_size = str_ptr + 8;
				    }
				    if( fn_num == FN_SPRINTF )
				    {
					//sprintf:
					((char*)str_cont->data)[ str_ptr ] = ts[ i2 ];
				    }
				    else
				    {
					//printf:
					cstr[ str_ptr ] = ts[ i2 ];
				    }
				    str_ptr++;
				}
			    }
			    break;
			case 'e': //Scientific notation (mantise/exponent) using e character:
			case 'E': //Scientific notation (mantise/exponent) using E character:
			case 'f': //Decimal floating point:
			case 'g': //Use the shorter of %e or %f:
			case 'G': //Use the shorter of %E or %f:
			case 'a': //Hexadecimal notation, starting with 0x
			case 'A': // ... 0X
			    {
				PIX_FLOAT arg_float;
				GET_VAL_FROM_STACK( arg_float, args_off + arg_num, PIX_FLOAT );
				arg_num++;
				char ts[ 32 ];
				snprintf( ts, 32, arg_format, (float)arg_float );
				for( i2 = 0; i2 < 32; i2++ )
				{
				    if( ts[ i2 ] == 0 ) break;
				    if( str_ptr >= str_size )
				    {
					if( fn_num == FN_SPRINTF )
					{
					    //sprintf:
					    if( pix_vm_resize_container( str, str_ptr + 8, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) 
						break;
					}
					else
					{
					    //printf:
					    cstr = (char*)smem_resize2( cstr, str_ptr + 8 );
					    if( cstr == 0 ) break;
					}
					str_size = str_ptr + 8;
				    }
				    if( fn_num == FN_SPRINTF )
				    {
					//sprintf:
					((char*)str_cont->data)[ str_ptr ] = ts[ i2 ];
				    }
				    else
				    {
					//printf:
					cstr[ str_ptr ] = ts[ i2 ];
				    }
				    str_ptr++;
				}
			    }
			    break;
		    }
		}
	    }
	    else
	    {
		c_to_output = 1;
	    }
	    if( c_to_output )
	    {
		if( str_ptr >= str_size )
		{
		    if( fn_num == FN_SPRINTF )
		    {
			//sprintf:
			if( pix_vm_resize_container( str, str_ptr + 8, 1, PIX_CONTAINER_TYPE_INT8, 0, vm ) ) 
			    break;
		    }
		    else
		    {
			//printf:
			cstr = (char*)smem_resize2( cstr, str_ptr + 8 );
			if( !cstr ) break;
		    }
		    str_size = str_ptr + 8;
		}
		if( fn_num == FN_SPRINTF )
		{
		    //sprintf:
		    ((char*)str_cont->data)[ str_ptr ] = c;
		}
		else
		{
		    //printf:
		    cstr[ str_ptr ] = c;
		}
		str_ptr++;
	    }
	}
	if( fn_num == FN_SPRINTF )
	{
	    //sprintf:
	    if( !no_null_term )
		if( str_ptr < str_size ) ((char*)str_cont->data)[ str_ptr ] = 0;
	}
	else
	{
	    //printf / fprintf / logf:
	    if( cstr )
	    {
		if( str_ptr + 1 >= str_size )
		{
		    cstr = (char*)smem_resize2( cstr, str_ptr + 1 );
		}
		if( cstr )
		{
		    if( fn_num == FN_PRINTF || fn_num == FN_LOGF )
		    {
			cstr[ str_ptr ] = 0;
			if( fn_num == FN_LOGF )
			{
			    //Add message to the log buffer:
			    pix_vm_log( cstr, vm );
			}
			else
			{
			    //Printf:
			    printf( "%s", cstr );
			}
		    }
		    else
		    {
			//fprintf:
			str_ptr = sfs_write( cstr, 1, str_ptr, dest_stream );
		    }
		}
	    }
	}
	smem_free( cstr );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = str_ptr;
    }
    else 
    {
	//Some error occured:
sprintf_error:
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }
}

//
// Log mamagement
//

void fn_get_log( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;
    
    if( vm->log_filled > 0 )
    {
	smutex_lock( &vm->log_mutex );
	size_t log_size = smem_get_size( vm->log_buffer );
	rv = pix_vm_new_container( -1, vm->log_filled + 1, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
	char* buf = (char*)pix_vm_get_container_data( rv, vm );
	if( buf )
	{
	    buf[ vm->log_filled ] = 0;
	    for( int i = (int)vm->log_filled - 1, i2 = (int)vm->log_ptr; i >= 0; i-- )
	    {
		i2--; if( i2 < 0 ) i2 = (int)log_size - 1;
	        buf[ i ] = vm->log_buffer[ i2 ];
	    }
	}
	smutex_unlock( &vm->log_mutex );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_get_system_log( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;
    
    const char* fname = slog_get_file();
    if( fname )
    {
	size_t log_size = sfs_get_file_size( fname );
	if( log_size > 0 )
	{
	    PIX_INT cnum = pix_vm_new_container( -1, log_size, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
	    char* buf = (char*)pix_vm_get_container_data( cnum, vm );
	    if( buf )
	    {
		sfs_file f = sfs_open( fname, "rb" );
		if( f )
		{
		    sfs_read( buf, 1, log_size, f );
		    sfs_close( f );
		    rv = cnum;
		}
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//
// Working with files
//

void fn_load( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num == 0 ) return;

    PIX_CID name;
    int par1 = 0;
    GET_VAL_FROM_STACK( name, 0, PIX_CID );
    if( pars_num >= 2 )
    {
	GET_VAL_FROM_STACK( par1, 1, int );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;

    bool need_to_free = 0;
    char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
    if( !ts ) return;

    char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
    if( full_path )
    {
	stack[ sp2 ].i = pix_vm_load( (const char*)full_path, 0, par1, vm );
	smem_free( full_path );
    }

    if( need_to_free ) smem_free( ts );
}

void fn_fload( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num == 0 ) return;

    sfs_file stream;
    int par1 = 0;
    GET_VAL_FROM_STACK( stream, 0, sfs_file );
    if( pars_num >= 2 )
    {
	GET_VAL_FROM_STACK( par1, 1, int );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_load( 0, stream, par1, vm );
}

void fn_save( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 3 ) return;
    
    PIX_CID cnum;
    PIX_CID name;
    int format;
    int par1 = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    GET_VAL_FROM_STACK( name, 1, PIX_CID );
    GET_VAL_FROM_STACK( format, 2, int );
    if( pars_num > 3 ) GET_VAL_FROM_STACK( par1, 3, int );

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;

    bool need_to_free = 0;
    char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
    if( !ts ) return;
    
    char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
    if( full_path )
    {
	stack[ sp2 ].i = pix_vm_save( cnum, (const char*)full_path, 0, format, par1, vm );
	smem_free( full_path );
    }
    
    if( need_to_free ) smem_free( ts );
}

void fn_fsave( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 3 ) return;
    
    PIX_CID cnum;
    sfs_file stream;
    int format;
    int par1 = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    GET_VAL_FROM_STACK( stream, 1, sfs_file );
    GET_VAL_FROM_STACK( format, 2, int );
    if( pars_num > 3 ) GET_VAL_FROM_STACK( par1, 3, int );

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_save( cnum, 0, stream, format, par1, vm );
}

void fn_get_real_path( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID name;
    pix_vm_container* name_cont;

    bool err = 0;

    //Get parameters:
    while( 1 )
    {
        if( pars_num < 1 ) { err = 1; break; }
        GET_VAL_FROM_STACK( name, 0, PIX_CID );
        if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
        name_cont = vm->c[ name ];
        break;
    }

    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
        bool need_to_free = 0;
        char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
        if( ts )
        {
	    char* path = pix_compose_full_path( vm->base_path, ts, vm ); //make SunDog filename: file -> base_path/file; 0:/file -> vfs0:/file;
	    char* path2 = sfs_make_filename( vm->wm->sd, (const char*)path, true ); //make system filename (can be used in std C file functions): 1:/.../file -> /home/user/.../file;
	    if( !path2 )
	    {
		err = 1;
	    }
	    else
	    {
		int path2_len = (int)smem_strlen( path2 );
        	PIX_CID path2_cnum = pix_vm_new_container( -1, path2_len, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
        	if( path2_cnum >= 0 )
        	{
            	    pix_vm_container* path2_cont = vm->c[ path2_cnum ];
            	    smem_copy( path2_cont->data, path2, path2_len );
        	}
    		stack[ sp2 ].t = 0;
    		stack[ sp2 ].i = path2_cnum;
    	    }
    	    smem_free( path );
    	    smem_free( path2 );
    	    if( need_to_free ) smem_free( ts );
    	}
    }

    if( err != 0 )
    {
        stack[ sp2 ].t = 0;
        stack[ sp2 ].i = -1;
    }
}

struct flist_data
{
    char* path;
    char* mask;
    sfs_find_struct fs;
    char* cur_file;
    int cur_type;
};

void fn_new_flist( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    flist_data* f = 0;    
    char* path = 0;
    char* mask = 0;
    PIX_CID path_cnum = -1;
    PIX_CID mask_cnum = -1;
    
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( path_cnum, 0, PIX_CID );
    if( pars_num >= 2 ) GET_VAL_FROM_STACK( mask_cnum, 1, PIX_CID );
    
    bool need_to_free1 = 0;
    path = pix_vm_make_cstring_from_container( path_cnum, &need_to_free1, vm );
    bool need_to_free2 = 0;
    mask = pix_vm_make_cstring_from_container( mask_cnum, &need_to_free2, vm );
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;
    
    if( path )
    {
	f = (flist_data*)smem_new( sizeof( flist_data ) );
	smem_zero( f );
	f->path = (char*)smem_new( smem_strlen( path ) + 1 ); f->path[ 0 ] = 0; smem_strcat_resize( f->path, path );
	if( mask )
	{
	    f->mask = (char*)smem_new( smem_strlen( mask ) + 1 ); f->mask[ 0 ] = 0; smem_strcat_resize( f->mask, mask );
	}
	f->fs.start_dir = (const char*)f->path;
	f->fs.mask = (const char*)f->mask;
	if( sfs_find_first( &f->fs ) )
	{
	    f->cur_file = f->fs.name;
	    f->cur_type = f->fs.type;
	}
	stack[ sp2 ].i = pix_vm_new_container( -1, smem_get_size( f ), 1, PIX_CONTAINER_TYPE_INT8, f, vm );
    }
    
    if( need_to_free1 ) smem_free( path );
    if( need_to_free2 ) smem_free( mask );
}

void fn_remove_flist( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;    
    PIX_CID flist_cnum = -1;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flist_cnum, 0, PIX_CID );
    flist_data* f = (flist_data*)pix_vm_get_container_data( flist_cnum, vm );
    if( f )
    {
	sfs_find_close( &f->fs );
	smem_free( f->path );
	smem_free( f->mask );
	pix_vm_remove_container( flist_cnum, vm );
    }
}

void fn_get_flist_name( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID flist_cnum = -1;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flist_cnum, 0, PIX_CID );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;
    flist_data* f = (flist_data*)pix_vm_get_container_data( flist_cnum, vm );
    if( f && f->cur_file )
    {
	int name_len = (int)smem_strlen( f->cur_file );
        PIX_CID name_cnum = pix_vm_new_container( -1, name_len, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
        if( name_cnum >= 0 )
        {
    	    pix_vm_container* name_cont = vm->c[ name_cnum ];
            smem_copy( name_cont->data, f->cur_file, name_len );
	    stack[ sp2 ].i = name_cnum;
        }
    }
}

void fn_get_flist_type( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID flist_cnum = -1;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flist_cnum, 0, PIX_CID );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;
    flist_data* f = (flist_data*)pix_vm_get_container_data( flist_cnum, vm );
    if( f )
    {
	stack[ sp2 ].i = f->cur_type;
    }
}

void fn_flist_next( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID flist_cnum = -1;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flist_cnum, 0, PIX_CID );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = 0;
    flist_data* f = (flist_data*)pix_vm_get_container_data( flist_cnum, vm );
    if( f )
    {
	if( sfs_find_next( &f->fs ) )
	{
	    f->cur_file = f->fs.name;
	    f->cur_type = f->fs.type;
	    stack[ sp2 ].i = 1;
	}
    }
}

void fn_get_file_size( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	bool need_to_free = 0;
	char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );

	char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)sfs_get_file_size( full_path );
	smem_free( full_path );

	if( need_to_free ) smem_free( ts );
    }
    else 
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = 0;
    }
}

void fn_get_file_format( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;
    PIX_CID name = -1;
    sfs_file f = 0;

    //Get parameters:
    while( 1 )
    {
	if( pars_num >= 1 ) { GET_VAL_FROM_STACK( name, 0, PIX_CID ); }
	if( pars_num >= 2 ) { GET_VAL_FROM_STACK( f, 1, sfs_file ); }
	break;
    }

    char* full_path = NULL;
    bool ts_ = false;
    char* ts = pix_vm_make_cstring_from_container( name, &ts_, vm );
    if( ts && ts[ 0 ] )
    {
	full_path = pix_compose_full_path( vm->base_path, ts, vm );
    }
    if( full_path || f ) rv = (PIX_INT)sfs_get_file_format( full_path, f );
    smem_free( full_path );
    if( ts_ ) smem_free( ts );

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_get_fformat_mime_OR_ext( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;
    int fmt = -1;

    if( pars_num >= 1 ) { GET_VAL_FROM_STACK( fmt, 0, int ); }

    if( fmt >= 0 && fmt < SFS_FILE_FMTS )
    {
	const char* s = NULL;
	if( fn_num == FN_GET_FFORMAT_MIME )
	    s = sfs_get_mime_type( (sfs_file_fmt)fmt );
	else
	    s = sfs_get_extension( (sfs_file_fmt)fmt );
	rv = pix_vm_make_container_from_cstring( s, vm );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//Remove a file:
void fn_remove_file( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	bool need_to_free = 0;
	char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	
	char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)sfs_remove_file( full_path );
	smem_free( full_path );
	
	if( need_to_free ) smem_free( ts );
    }
    else
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }
}

//Rename a file:
void fn_rename_file( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name1;
    PIX_CID name2;
    pix_vm_container* name1_cont;
    pix_vm_container* name2_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name1, 0, PIX_CID );
	GET_VAL_FROM_STACK( name2, 1, PIX_CID );
	if( (unsigned)name1 >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name1 ] == 0 ) { err = 1; break; }
	name1_cont = vm->c[ name1 ];
	if( (unsigned)name2 >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name2 ] == 0 ) { err = 1; break; }
	name2_cont = vm->c[ name2 ];
	if( name1_cont->size == 0 ) { err = 1; break; }
	if( name2_cont->size == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	char* ts1 = pix_vm_make_cstring_from_container( name1, &need_to_free1, vm );
	char* ts2 = pix_vm_make_cstring_from_container( name2, &need_to_free2, vm );
	
	char* full_path1 = pix_compose_full_path( vm->base_path, ts1, vm );
	char* full_path2 = pix_compose_full_path( vm->base_path, ts2, vm );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)sfs_rename( full_path1, full_path2 );
	smem_free( full_path1 );
	smem_free( full_path2 );
	
	if( need_to_free1 ) smem_free( ts1 );
	if( need_to_free2 ) smem_free( ts2 );
    }
    else 
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }
}

//Copy a file:
void fn_copy_file( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name1;
    PIX_CID name2;
    pix_vm_container* name1_cont;
    pix_vm_container* name2_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name1, 0, PIX_CID );
	GET_VAL_FROM_STACK( name2, 1, PIX_CID );
	if( (unsigned)name1 >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name1 ] == 0 ) { err = 1; break; }
	name1_cont = vm->c[ name1 ];
	if( (unsigned)name2 >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name2 ] == 0 ) { err = 1; break; }
	name2_cont = vm->c[ name2 ];
	if( name1_cont->size == 0 ) { err = 1; break; }
	if( name2_cont->size == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	char* ts1 = pix_vm_make_cstring_from_container( name1, &need_to_free1, vm );
	char* ts2 = pix_vm_make_cstring_from_container( name2, &need_to_free2, vm );
	
	char* full_path1 = pix_compose_full_path( vm->base_path, ts1, vm );
	char* full_path2 = pix_compose_full_path( vm->base_path, ts2, vm );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = sfs_copy_file( (const char*)full_path2, (const char*)full_path1 );
	smem_free( full_path1 );
	smem_free( full_path2 );
	
	if( need_to_free1 ) smem_free( ts1 );
	if( need_to_free2 ) smem_free( ts2 );
    }
    else
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }
}

void fn_create_directory( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    uint mode = 0;
#ifdef OS_UNIX
    mode = S_IRWXU | S_IRWXG | S_IRWXO;
#endif
    pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
        if( pars_num > 1 ) GET_VAL_FROM_STACK( mode, 1, uint );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	if( name_cont->size == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	bool need_to_free = 0;
	char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	
	char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = sfs_mkdir( (const char*)full_path, mode );
	smem_free( full_path );
	
	if( need_to_free ) smem_free( ts );
    }
    else
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }
}

//Set virtual disk 0:
void fn_set_disk0( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    sfs_file stream;    
    
    if( pars_num < 1 ) return;
    GET_VAL_FROM_STACK( stream, 0, sfs_file );
    vm->virt_disk0 = stream;
}

//Get virtual disk 0:
void fn_get_disk0( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = vm->virt_disk0;
}

//
// Working with files (posix)
//

//Open a stream:
void fn_fopen( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    PIX_CID mode;
    pix_vm_container* name_cont;
    pix_vm_container* mode_cont;
    
    bool err = 0;

    //Get parameters:
    while( 1 )
    {
	if( pars_num < 2 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	GET_VAL_FROM_STACK( mode, 1, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	if( (unsigned)mode >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ mode ] == 0 ) { err = 1; break; }
	mode_cont = vm->c[ mode ];
	if( name_cont->size == 0 ) { err = 1; break; }
	if( mode_cont->size == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	char* ts1 = pix_vm_make_cstring_from_container( name, &need_to_free1, vm );
	char* ts2 = pix_vm_make_cstring_from_container( mode, &need_to_free2, vm );
	
	char* full_path = pix_compose_full_path( vm->base_path, ts1, vm );
	sfs_file f = sfs_open( full_path, ts2 );
	smem_free( full_path );
	
	if( need_to_free1 ) smem_free( ts1 );
	if( need_to_free2 ) smem_free( ts2 );
	
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)f;
    }
    else
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = 0;
    }
}

//Open a stream in memory:
void fn_fopen_mem( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID data;
    pix_vm_container* data_cont = 0;
    
    bool err = 0;

    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( data, 0, PIX_CID );
	data_cont = pix_vm_get_container( data, vm );
	if( data_cont == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	sfs_file f = sfs_open_in_memory( data_cont->data, data_cont->size * g_pix_container_type_sizes[ data_cont->type ] );
	sfs_set_user_data( f, data );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)f;
    }
    else
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = 0;
    }
}

//Close a stream:
void fn_fclose( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( pars_num >= 1 )
    {
	sfs_file f;
	GET_VAL_FROM_STACK( f, 0, sfs_file );
	if( sfs_get_type( f ) == SFS_FILE_IN_MEMORY )
	{
	    PIX_CID data_cnum = sfs_get_user_data( f );
	    if( data_cnum )
	    {
		void* data_ptr = sfs_get_data( f );
		if( data_ptr )
		{
		    pix_vm_container* data_cont = pix_vm_get_container( data_cnum, vm );
		    if( data_cont )
		    {
			size_t data_size = sfs_get_data_size( f );
			if( data_cont->data == data_ptr && data_cont->size * g_pix_container_type_sizes[ data_cont->type ] == data_size )
			{
			    //No changes
			}
			else
			{
			    data_cont->data = data_ptr;
			    data_cont->size = data_size / g_pix_container_type_sizes[ data_cont->type ];
			    data_cont->xsize = data_cont->size;
			    data_cont->ysize = 1;
			}
		    }
		}
	    }
	}
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = sfs_close( f );
    }
    else
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }
}

//Put a byte on a stream:
void fn_fputc( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int c;
    sfs_file f;
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( pars_num >= 2 ) 
    {
	GET_VAL_FROM_STACK( c, 0, int );
	GET_VAL_FROM_STACK( f, 1, sfs_file );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = sfs_putc( c, f );
    }
    else if( pars_num == 1 )
    {
	GET_VAL_FROM_STACK( c, 0, int );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = sfs_putc( c, SFS_STDOUT );
    }
}

//Put a string on a stream:
void fn_fputs( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_INT rv = -1;
    PIX_CID s = -1;
    sfs_file f;
    if( pars_num >= 2 ) 
    {
	GET_VAL_FROM_STACK( s, 0, PIX_CID );
	GET_VAL_FROM_STACK( f, 1, sfs_file );
    }
    else if( pars_num == 1 )
    {
	GET_VAL_FROM_STACK( s, 0, PIX_CID );
	f = SFS_STDOUT;
    }
    if( (unsigned)s < (unsigned)vm->c_num )
    {
	pix_vm_container* cont = vm->c[ s ];
	if( cont )
	{
	    size_t str_len;
	    for( str_len = 0; str_len < cont->size; str_len++ )
		if( ((char*)cont->data)[ str_len ] == 0 )
		    break;
	    rv = (PIX_INT)sfs_write( cont->data, 1, str_len, f );
	}
    }
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_fgets_OR_fwrite_OR_fread( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = 0;
    PIX_CID s = -1;
    PIX_INT offset = 0;
    PIX_INT size;
    sfs_file f;
    if( pars_num >= 3 ) 
    {
	GET_VAL_FROM_STACK( s, 0, PIX_CID );
	GET_VAL_FROM_STACK( size, 1, PIX_INT );
	GET_VAL_FROM_STACK( f, 2, sfs_file );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( offset, 3, PIX_INT );
    }
    if( (unsigned)s < (unsigned)vm->c_num )
    {
	pix_vm_container* cont = vm->c[ s ];
	if( cont )
	{
	    size_t real_size = cont->size * g_pix_container_type_sizes[ cont->type ];
	    if( offset + size > real_size )
		size = (PIX_INT)real_size - offset;
	    char* data = (char*)cont->data + offset;
	    switch( fn_num )
	    {
		case FN_FGETS: 
		    {
			//Get a string from a stream:
			char* str = (char*)data;
			size_t p;
			bool eof = false;
			for( p = 0; p < real_size - 1; p++ )
			{
			    int c = sfs_getc( f );
			    if( c == -1 ) { eof = true; break; }
			    if( c == 0xD ) break;
			    if( c == 0xA ) break;
			    str[ p ] = (char)c;
			}
			str[ p ] = 0;
			rv = (PIX_INT)p;
			if( p == 0 && eof ) rv = -1;
		    }
		    break;
		case FN_FWRITE: 
		    rv = (PIX_INT)sfs_write( data, 1, size, f ); 
		    break;
		case FN_FREAD: 
		    rv = (PIX_INT)sfs_read( data, 1, size, f ); 
		    break;
	    }
	}
    }
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//Get a byte from a stream:
void fn_fgetc( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    sfs_file f;
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( pars_num >= 1 ) 
    {
	GET_VAL_FROM_STACK( f, 0, sfs_file );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = sfs_getc( f );
    }
    else if( pars_num == 0 )
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = sfs_getc( SFS_STDIN );
    }
}

void fn_feof_OF_fflush_OR_ftell( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( pars_num >= 1 )
    {
	sfs_file f;
	GET_VAL_FROM_STACK( f, 0, sfs_file );
	stack[ sp2 ].t = 0;
	switch( fn_num )
	{
	    case FN_FEOF: stack[ sp2 ].i = sfs_eof( f ); break;
	    case FN_FFLUSH: stack[ sp2 ].i = sfs_flush( f ); break;
	    case FN_FTELL: stack[ sp2 ].i = (PIX_INT)sfs_tell( f ); break;
	}
    }
}

void fn_fseek( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	sfs_file f;
	PIX_INT offset;
	PIX_INT mode;
	GET_VAL_FROM_STACK( f, 0, sfs_file );
	GET_VAL_FROM_STACK( offset, 1, PIX_INT );
	GET_VAL_FROM_STACK( mode, 2, PIX_INT );
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = sfs_seek( f, offset, mode );
    }
}

void fn_setxattr( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

#if defined(OS_UNIX) && !defined(NO_SETXATTR)
    
    PIX_CID path;
    PIX_CID name;
    PIX_CID value;
    size_t size;
    uint flags;
    pix_vm_container* path_cont;
    pix_vm_container* name_cont;
    void* value_data;
    
    bool err = 0;

    //Get parameters:
    while( 1 )
    {
	if( pars_num < 5 ) { err = 1; break; }
	GET_VAL_FROM_STACK( path, 0, PIX_CID );
	GET_VAL_FROM_STACK( name, 1, PIX_CID );
	GET_VAL_FROM_STACK( value, 2, PIX_CID );
	GET_VAL_FROM_STACK( size, 3, size_t );
	GET_VAL_FROM_STACK( flags, 4, int );
	path_cont = pix_vm_get_container( path, vm ); if( path_cont == 0 ) { err = 1; break; }
	name_cont = pix_vm_get_container( name, vm ); if( name_cont == 0 ) { err = 1; break; }
	value_data = pix_vm_get_container_data( value, vm ); if( value_data == 0 ) { err = 1; break; }
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	char* ts1 = pix_vm_make_cstring_from_container( path, &need_to_free1, vm );
	char* ts2 = pix_vm_make_cstring_from_container( name, &need_to_free2, vm );
	
	char* full_path = pix_compose_full_path( vm->base_path, ts1, vm );
	char* full_path2 = sfs_make_filename( vm->wm->sd, full_path, true );
	smem_free( full_path );
#if defined(OS_FREEBSD) || defined(OS_APPLE)
	int res = setxattr( full_path2, ts2, value_data, size, 0, flags );
#else
	int res = setxattr( full_path2, ts2, value_data, size, flags );
#endif
	smem_free( full_path2 );
	
	if( need_to_free1 ) smem_free( ts1 );
	if( need_to_free2 ) smem_free( ts2 );
	
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)res;
    }
    else
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }

#else

    //Not *NIX compatible system:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;

#endif

}

//
// Graphics
//

void fn_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = 0;
    while( 1 )
    {
	if( !vm->ready ) { stime_sleep( 50 ); break; }
	int delay = 0;
	if( pars_num >= 1 ) GET_VAL_FROM_STACK( delay, 0, int );
	PIX_CID scr = vm->screen;
	if( (unsigned)scr < (unsigned)vm->c_num && vm->c[ scr ] && vm->screen_ptr )
	{
	    pix_vm_container* c = vm->c[ scr ];
	    vm->screen_change_x = 0;
	    vm->screen_change_y = 0;
	    vm->screen_change_xsize = c->xsize;
	    vm->screen_change_ysize = c->ysize;
	    if( pars_num >= 2 ) GET_VAL_FROM_STACK( vm->screen_change_x, 1, int );
	    if( pars_num >= 3 ) GET_VAL_FROM_STACK( vm->screen_change_y, 2, int );
	    if( pars_num >= 4 ) GET_VAL_FROM_STACK( vm->screen_change_xsize, 3, int );
	    if( pars_num >= 5 ) GET_VAL_FROM_STACK( vm->screen_change_ysize, 4, int );
	    volatile int prev_counter = vm->screen_redraw_counter;
	    vm->screen_redraw_request++;
	    if( stime_ms() - vm->fps_time > 1000 )
	    {
		vm->fps = vm->fps_counter;
		vm->vars[ PIX_GVAR_FPS ].i = vm->fps;
		vm->vars[ PIX_GVAR_FPS ].t = 0;
		vm->fps_time = stime_ms();
		vm->fps_counter = 0;
	    }
	    vm->fps_counter++;
	    if( delay ) stime_sleep( delay );
	    stime_ms_t t = stime_ms();
	    stime_ms_t t_timeout = 1000 / 2;
	    int sleep2 = 1000 / vm->wm->max_fps / 2; LIMIT_NUM( sleep2, 1, 8 );
	    bool no_timeout = 0;
	    while( vm->screen_redraw_request != vm->screen_redraw_answer )
	    {
		if( vm->wm->suspended )
		{
		    if( vm->prev_frame_res == -2 ) stime_sleep( 1000 );
		    rv = -2;
		    break;
		}
		if( !vm->ready ) break;
		stime_sleep( sleep2 );
		if( no_timeout == 0 && stime_ms() - t >= t_timeout )
		{
		    if( prev_counter != vm->screen_redraw_counter )
		    {
			//Screen drawing callback is still active. So just waiting...
			no_timeout = 1;
		    }
		    else
		    {
			//UI thread is not active for some reason
			rv = -2;
			break;
		    }
		}
	    }
	}
	else 
	{
	    //No screen.
	    if( delay == 0 ) delay = 50; 
	    stime_sleep( delay );
	    rv = -1;
	}
	break;
    }

    vm->prev_frame_res = rv;
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_vsync( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    bool vsync = false;
    if( pars_num >= 1 ) { GET_VAL_FROM_STACK( vsync, 0, bool ); }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = 0;

    sundog_event evt;
    smem_clear_struct( evt );
    evt.win = vm->win;
    evt.type = EVT_PIXICMD;
    evt.x = pix_sundog_req_vsync;
    evt.y = vsync;
    send_events( &evt, 1, vm->wm );
}

void fn_set_pixel_size( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	int size;
	GET_VAL_FROM_STACK( size, 0, int );
	if( size < 1 ) size = 1;
	int prev_size = vm->pixel_size;
	if( prev_size != size )
	{
	    float scale = (float)prev_size / (float)size;
	    vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i *= scale;
	    vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i *= scale;
	    vm->vars[ PIX_GVAR_PPI ].i *= scale;
	    vm->pixel_size = (uint16_t)size;
	}
    }
}

void fn_get_pixel_size( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = vm->pixel_size;
}

void fn_set_screen( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_CID scr;
	GET_VAL_FROM_STACK( scr, 0, PIX_CID );
	pix_vm_gfx_set_screen( scr, vm );
    }
}

void fn_get_screen( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = vm->screen;
}

void fn_set_zbuf( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_CID z;
	GET_VAL_FROM_STACK( z, 0, PIX_CID );
#ifdef OPENGL
	if( vm->screen == PIX_GL_SCREEN )
	{
	    if( vm->zbuf != PIX_GL_ZBUF && z == PIX_GL_ZBUF )
	    {
		glDepthFunc( GL_LESS );
		glEnable( GL_DEPTH_TEST );
	    }
	    if( vm->zbuf == PIX_GL_ZBUF && z == -1 )
	    {
		glDisable( GL_DEPTH_TEST );
	    }
	}
#endif
	vm->zbuf = z;
    }
}

void fn_get_zbuf( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = vm->zbuf;
}

void fn_clear_zbuf( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_VAL v;
    v.i = 0x80000000;
    pix_vm_clean_container( vm->zbuf, 0, v, 0, -1, vm );

#ifdef OPENGL
    if( vm->screen == PIX_GL_SCREEN )
    {
	if( vm->zbuf == PIX_GL_ZBUF )
	{
	    glClear( GL_DEPTH_BUFFER_BIT );
	}
    }
#endif
}

void fn_get_color( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 3 )
    {
	int r, g, b;
	GET_VAL_FROM_STACK( r, 0, int );
	GET_VAL_FROM_STACK( g, 1, int );
	GET_VAL_FROM_STACK( b, 2, int );
	if( r < 0 ) r = 0;
	if( g < 0 ) g = 0;
	if( b < 0 ) b = 0;
	if( r > 255 ) r = 255;
	if( g > 255 ) g = 255;
	if( b > 255 ) b = 255;
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (COLORSIGNED)get_color( r, g, b );
    }
}

void fn_get_red( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_INT c;
	GET_VAL_FROM_STACK( c, 0, PIX_INT );
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)red( c );
    }
}

void fn_get_green( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_INT c;
	GET_VAL_FROM_STACK( c, 0, PIX_INT );
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)green( c );
    }
}

void fn_get_blue( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_INT c;
	GET_VAL_FROM_STACK( c, 0, PIX_INT );
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)blue( c );
    }
}

void fn_get_blend( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	PIX_INT c1, c2, v;
	GET_VAL_FROM_STACK( c1, 0, PIX_INT );
	GET_VAL_FROM_STACK( c2, 1, PIX_INT );
	GET_VAL_FROM_STACK( v, 2, PIX_INT );
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (COLORSIGNED)blend( c1, c2, (int)v );
    }
}

void fn_transp( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_INT t;
	GET_VAL_FROM_STACK( t, 0, PIX_INT );
	if( t < 0 ) t = 0;
	if( t > 255 ) t = 255;
	vm->transp = (uint8_t)t;
    }
}

void fn_get_transp( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = vm->transp;
}

void fn_clear( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int t = vm->transp;
    if( t == 0 ) return;

#ifdef OPENGL
    if( vm->screen == PIX_GL_SCREEN )
    {
	COLOR c = CLEARCOLOR;
	if( pars_num >= 1 )
	{
	    GET_VAL_FROM_STACK( c, 0, COLOR );
	}
	if( t >= 255 ) 
	{
	    glClearColor( (float)red( c ) / 255, (float)green( c ) / 255, (float)blue( c ) / 255, 1 );
	    glClear( GL_COLOR_BUFFER_BIT );
	}
	else
	{
	    bool prev_t_enabled = vm->t_enabled;
	    if( prev_t_enabled )
	    {
		vm->t_enabled = 0;
		pix_vm_gl_program_reset( vm );
	    }
	    gl_program_struct* p = vm->gl_prog_solid;
    	    if( vm->gl_current_prog != p )
	    {
		pix_vm_gl_use_prog( p, vm );
		gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) );
	    }
	    float v[ 8 ];
	    float xsize = (float)vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i / 2.0F;
	    float ysize = (float)vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i / 2.0F;
	    v[ 0 ] = -xsize; v[ 1 ] = -ysize;
	    v[ 2 ] = xsize; v[ 3 ] = -ysize;
	    v[ 4 ] = -xsize; v[ 5 ] = ysize;
	    v[ 6 ] = xsize; v[ 7 ] = ysize;
	    GL_CHANGE_PROG_COLOR( p, c, t );
	    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 2, GL_FLOAT, false, 0, v );
	    glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	    if( prev_t_enabled )
	    {
		vm->t_enabled = 1;
		pix_vm_gl_program_reset( vm );
	    }
	}
	return;
    }
#endif

    if( !vm->screen_ptr ) return;
    COLOR c = CLEARCOLOR;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( c, 0, COLOR );
    }
    COLORPTR p = vm->screen_ptr;
    COLORPTR p_end = vm->screen_ptr + vm->screen_xsize * vm->screen_ysize;
    if( t == 255 )
    {
	for( ; p < p_end; p++ )
	{
	    *p = c;
	}
    }
    else 
    {
	for( ; p < p_end; p++ )
	{
	    *p = fast_blend( *p, c, t );
	}
    }
}

void fn_dot( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( vm->screen_ptr == 0 ) return;
    if( vm->transp == 0 ) return;

    COLOR c = COLORMASK;
    PIX_INT x, y, z;

    if( fn_num == FN_DOT )
    {
	if( pars_num < 2 ) return;
	if( pars_num >= 3 )
	{
	    GET_VAL_FROM_STACK( c, 2, COLOR );
	}
    }
    else 
    {
	if( pars_num < 3 ) return;
	if( pars_num >= 4 )
	{
	    GET_VAL_FROM_STACK( c, 3, COLOR );
	}
    }

#ifdef OPENGL
    if( vm->screen == PIX_GL_SCREEN )
    {
	float v[ 3 ];
	if( fn_num == FN_DOT )
	{
	    GET_VAL_FROM_STACK( v[ 0 ], 0, float );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, float );
	    v[ 2 ] = 0;
	}
	else
	{
	    GET_VAL_FROM_STACK( v[ 0 ], 0, float );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, float );
	    GET_VAL_FROM_STACK( v[ 2 ], 2, float );
	}
	if( !vm->gl_no_2d_line_shift )
	{
	    //Remove it in future major update?
	    v[ 0 ] += GL_2D_LINE_SHIFT;
	    v[ 1 ] += GL_2D_LINE_SHIFT;
	}
	gl_program_struct* p = vm->gl_prog_solid;
	if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
	if( vm->gl_current_prog != p )
        {
            pix_vm_gl_use_prog( p, vm );
            gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) );
        }
        GL_CHANGE_PROG_COLOR( p, c, vm->transp );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, v );
	glDrawArrays( GL_POINTS, 0, 1 );
	return;
    }
#endif

    if( vm->t_enabled )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT v[ 3 ];
	GET_VAL_FROM_STACK( v[ 0 ], 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( v[ 1 ], 1, PIX_FLOAT );
	if( fn_num == FN_DOT )
	    v[ 2 ] = 0;
	else 
	    GET_VAL_FROM_STACK( v[ 2 ], 2, PIX_FLOAT );
	pix_vm_gfx_vertex_transform( v, m );
	x = v[ 0 ];
	y = v[ 1 ];
	z = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
    }
    else 
    {
	GET_VAL_FROM_STACK( x, 0, PIX_INT );
	GET_VAL_FROM_STACK( y, 1, PIX_INT );
	if( fn_num == FN_GET_DOT )
	    z = 0;
	else 
	{
	    PIX_FLOAT zf;
	    GET_VAL_FROM_STACK( zf, 1, PIX_FLOAT );
	    z = zf * ( 1 << PIX_FIXED_MATH_PREC );
	}
    }
    x += vm->screen_xsize / 2;
    y += vm->screen_ysize / 2;
    if( (unsigned)x < (unsigned)vm->screen_xsize &&
	(unsigned)y < (unsigned)vm->screen_ysize )
    {
	int* zbuf = pix_vm_gfx_get_zbuf( vm );
	if( zbuf )
	{
	    int p = y * vm->screen_xsize + x;
	    if( z > zbuf[ p ] ) 
	    {
		if( vm->transp == 255 )
		{
		    vm->screen_ptr[ p ] = c;
		}
		else
		{
		    vm->screen_ptr[ p ] = fast_blend( vm->screen_ptr[ p ], c, vm->transp );
		}
		zbuf[ p ] = z;
	    }
	}
	else 
	{
	    COLORPTR p = vm->screen_ptr + y * vm->screen_xsize + x;
	    if( vm->transp == 255 )
	    {
		*p = c;
	    }
	    else
	    {
		*p = fast_blend( *p, c, vm->transp );
	    }
	}
    }
}

void fn_get_dot( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( vm->screen_ptr == 0 ) return;
    
    PIX_INT x, y;
    
    if( fn_num == FN_GET_DOT )
    {
	if( pars_num < 2 ) return;
    }
    else 
    {
	if( pars_num < 3 ) return;
    }
    
    if( vm->t_enabled )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT v[ 3 ];
	GET_VAL_FROM_STACK( v[ 0 ], 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( v[ 1 ], 1, PIX_FLOAT );
	if( fn_num == FN_GET_DOT )
	    v[ 2 ] = 0;
	else 
	    GET_VAL_FROM_STACK( v[ 2 ], 2, PIX_FLOAT );
	pix_vm_gfx_vertex_transform( v, m );
	x = v[ 0 ];
	y = v[ 1 ];
    }
    else 
    {
	GET_VAL_FROM_STACK( x, 0, PIX_INT );
	GET_VAL_FROM_STACK( y, 1, PIX_INT );
    }
    x += vm->screen_xsize / 2;
    y += vm->screen_ysize / 2;
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    if( (unsigned)x < (unsigned)vm->screen_xsize &&
	(unsigned)y < (unsigned)vm->screen_ysize )
    {
	COLORPTR p = vm->screen_ptr + y * vm->screen_xsize + x;
	stack[ sp2 ].i = (PIX_INT)*p;
    }
    else 
    {
	stack[ sp2 ].i = 0;
    }
}

void fn_line( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( vm->screen_ptr == 0 ) return;
    if( vm->transp == 0 ) return;

    COLOR c = COLORMASK;
    PIX_INT x1, y1, z1, x2, y2, z2;

    if( fn_num == FN_LINE )
    {
	if( pars_num < 4 ) return;
	if( pars_num >= 5 )
	{
	    GET_VAL_FROM_STACK( c, 4, COLOR );
	}
    }
    else
    {
	if( pars_num < 6 ) return;
	if( pars_num >= 7 )
	{
	    GET_VAL_FROM_STACK( c, 6, COLOR );
	}
    }

#ifdef OPENGL
    if( vm->screen == PIX_GL_SCREEN )
    {
	float v[ 6 ];
	if( fn_num == FN_LINE )
	{
	    GET_VAL_FROM_STACK( v[ 0 ], 0, float );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, float );
	    v[ 2 ] = 0;
	    GET_VAL_FROM_STACK( v[ 3 ], 2, float );
	    GET_VAL_FROM_STACK( v[ 4 ], 3, float );
	    v[ 5 ] = 0;
	}
	else
	{
	    GET_VAL_FROM_STACK( v[ 0 ], 0, float );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, float );
	    GET_VAL_FROM_STACK( v[ 2 ], 2, float );
	    GET_VAL_FROM_STACK( v[ 3 ], 3, float );
	    GET_VAL_FROM_STACK( v[ 4 ], 4, float );
	    GET_VAL_FROM_STACK( v[ 5 ], 5, float );
	}
	if( !vm->gl_no_2d_line_shift )
        {
    	    //Remove it in future major update?
            v[ 0 ] += GL_2D_LINE_SHIFT;
            v[ 1 ] += GL_2D_LINE_SHIFT;
            v[ 3 ] += GL_2D_LINE_SHIFT;
            v[ 4 ] += GL_2D_LINE_SHIFT;
        }
	gl_program_struct* p = vm->gl_prog_solid;
	if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
	if( vm->gl_current_prog != p )
        {
            pix_vm_gl_use_prog( p, vm );
            gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) );
        }
        GL_CHANGE_PROG_COLOR( p, c, vm->transp );
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, v );
	glDrawArrays( GL_LINES, 0, 2 );
	return;
    }
#endif
    
    int* zbuf = pix_vm_gfx_get_zbuf( vm );

    if( fn_num == FN_LINE )
    {
	if( vm->t_enabled )
	{
	    PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	    PIX_FLOAT v[ 3 ];
	    
	    GET_VAL_FROM_STACK( v[ 0 ], 0, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, PIX_FLOAT );
	    v[ 2 ] = 0;
	    pix_vm_gfx_vertex_transform( v, m );
	    x1 = v[ 0 ];
	    y1 = v[ 1 ];
	    if( zbuf ) z1 = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );

	    GET_VAL_FROM_STACK( v[ 0 ], 2, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 1 ], 3, PIX_FLOAT );
	    v[ 2 ] = 0;
	    pix_vm_gfx_vertex_transform( v, m );
	    x2 = v[ 0 ];
	    y2 = v[ 1 ];
	    if( zbuf ) z2 = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	}
	else 
	{
	    GET_VAL_FROM_STACK( x1, 0, PIX_INT );
	    GET_VAL_FROM_STACK( y1, 1, PIX_INT );
	    GET_VAL_FROM_STACK( x2, 2, PIX_INT );
	    GET_VAL_FROM_STACK( y2, 3, PIX_INT );
	    if( zbuf )
	    {
		z1 = 0;
		z2 = 0;
	    }
	}
    }
    else 
    {
	if( vm->t_enabled )
	{
	    PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	    PIX_FLOAT v[ 3 ];

	    GET_VAL_FROM_STACK( v[ 0 ], 0, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 1 ], 1, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 2 ], 2, PIX_FLOAT );
	    pix_vm_gfx_vertex_transform( v, m );
	    x1 = v[ 0 ];
	    y1 = v[ 1 ];
	    if( zbuf ) z1 = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );

	    GET_VAL_FROM_STACK( v[ 0 ], 3, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 1 ], 4, PIX_FLOAT );
	    GET_VAL_FROM_STACK( v[ 2 ], 5, PIX_FLOAT );
	    pix_vm_gfx_vertex_transform( v, m );
	    x2 = v[ 0 ];
	    y2 = v[ 1 ];
	    if( zbuf ) z2 = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	}
	else 
	{
	    PIX_FLOAT zz;
	    GET_VAL_FROM_STACK( x1, 0, PIX_INT );
	    GET_VAL_FROM_STACK( y1, 1, PIX_INT );
	    GET_VAL_FROM_STACK( x2, 3, PIX_INT );
	    GET_VAL_FROM_STACK( y2, 4, PIX_INT );
	    if( zbuf ) 
	    {
		GET_VAL_FROM_STACK( zz, 2, PIX_FLOAT );
		z1 = zz * ( 1 << PIX_FIXED_MATH_PREC );
		GET_VAL_FROM_STACK( zz, 5, PIX_FLOAT );
		z2 = zz * ( 1 << PIX_FIXED_MATH_PREC );
	    }
	}
    }
    
    int screen_hxsize = vm->screen_xsize / 2;
    int screen_hysize = vm->screen_ysize / 2;
    
    x1 += screen_hxsize;
    y1 += screen_hysize;
    x2 += screen_hxsize;
    y2 += screen_hysize;
    if( zbuf )
    {
	pix_vm_gfx_draw_line_zbuf( x1, y1, z1, x2, y2, z2, c, zbuf, vm );
    }
    else 
    {
	pix_vm_gfx_draw_line( x1, y1, x2, y2, c, vm );
    }
}

void fn_box( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( vm->screen_ptr == 0 ) return;
    if( vm->transp == 0 ) return;

    COLOR c = COLORMASK;
    if( pars_num < 4 ) return;
    if( pars_num >= 5 )
    {
	GET_VAL_FROM_STACK( c, 4, COLOR );
    }

    int screen_hxsize = vm->screen_xsize / 2;
    int screen_hysize = vm->screen_ysize / 2;
    
    if( vm->t_enabled || vm->zbuf >= 0 || vm->screen == PIX_GL_SCREEN )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT x, y, xsize, ysize;
	
	GET_VAL_FROM_STACK( x, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( y, 1, PIX_FLOAT );
	GET_VAL_FROM_STACK( xsize, 2, PIX_FLOAT );
	GET_VAL_FROM_STACK( ysize, 3, PIX_FLOAT );
	
	PIX_FLOAT v[ 12 ];
	v[ 0 ] = x;
	v[ 1 ] = y;
	v[ 2 ] = 0;
	v[ 3 ] = x + xsize;
	v[ 4 ] = y;
	v[ 5 ] = 0;
	v[ 6 ] = x;
	v[ 7 ] = y + ysize;
	v[ 8 ] = 0;
	v[ 9 ] = x + xsize;
	v[ 10 ] = y + ysize;
	v[ 11 ] = 0;
#ifdef OPENGL
	if( vm->screen == PIX_GL_SCREEN )
	{
	    gl_program_struct* p = vm->gl_prog_solid;
	    if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
	    if( vm->gl_current_prog != p )
    	    {
    		pix_vm_gl_use_prog( p, vm );
    		gl_enable_attributes( p, ( 1 << GL_PROG_ATT_POSITION ) );
    	    }
    	    GL_CHANGE_PROG_COLOR( p, c, vm->transp );
	    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, v );
	    float vvv[ 15 ];
	    float* vv;
	    if( fn_num == FN_BOX )
	    {
		vvv[ 0 ] = v[ 0 ];
		vvv[ 1 ] = v[ 1 ];
		vvv[ 2 ] = v[ 2 ];
		vvv[ 3 ] = v[ 3 ];
		vvv[ 4 ] = v[ 4 ];
		vvv[ 5 ] = v[ 5 ];
		vvv[ 6 ] = v[ 9 ];
		vvv[ 7 ] = v[ 10 ];
		vvv[ 8 ] = v[ 11 ];
		vvv[ 9 ] = v[ 6 ];
		vvv[ 10 ] = v[ 7 ];
		vvv[ 11 ] = v[ 8 ];
		vvv[ 12 ] = v[ 0 ];
		vvv[ 13 ] = v[ 1 ];
		vvv[ 14 ] = v[ 2 ];
		glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, vvv );
		glDrawArrays( GL_LINE_STRIP, 0, 5 );
	    }
	    else
	    {
		if( sizeof( float ) == sizeof( PIX_FLOAT ) )
	    	    vv = (float*)v;
		else
		{
		    vvv[ 0 ] = v[ 0 ];
		    vvv[ 1 ] = v[ 1 ];
		    vvv[ 2 ] = v[ 2 ];
		    vvv[ 3 ] = v[ 3 ];
		    vvv[ 4 ] = v[ 4 ];
		    vvv[ 5 ] = v[ 5 ];
		    vvv[ 6 ] = v[ 6 ];
		    vvv[ 7 ] = v[ 7 ];
		    vvv[ 8 ] = v[ 8 ];
		    vvv[ 9 ] = v[ 9 ];
		    vvv[ 10 ] = v[ 10 ];
		    vvv[ 11 ] = v[ 11 ];
		    vv = vvv;
		}
		glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, vv );
		glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );
	    }
	    return;
	}
#endif
	if( vm->t_enabled )
	{
	    pix_vm_gfx_vertex_transform( v, m );
	    pix_vm_gfx_vertex_transform( v + 3, m );
	    pix_vm_gfx_vertex_transform( v + 6, m );
	    pix_vm_gfx_vertex_transform( v + 9, m );
	}
	v[ 0 ] += screen_hxsize;
	v[ 1 ] += screen_hysize;
	v[ 3 ] += screen_hxsize;
	v[ 4 ] += screen_hysize;
	v[ 6 ] += screen_hxsize;
	v[ 7 ] += screen_hysize;
	v[ 9 ] += screen_hxsize;
	v[ 10 ] += screen_hysize;
	if( fn_num == FN_BOX )
	{
	    pix_vm_gfx_draw_line( v[ 0 ], v[ 1 ], v[ 3 ], v[ 4 ], c, vm );
	    pix_vm_gfx_draw_line( v[ 3 ], v[ 4 ], v[ 9 ], v[ 10 ], c, vm );
	    pix_vm_gfx_draw_line( v[ 9 ], v[ 10 ], v[ 6 ], v[ 7 ], c, vm );
	    pix_vm_gfx_draw_line( v[ 6 ], v[ 7 ], v[ 0 ], v[ 1 ], c, vm );
	}
	else 
	{
	    pix_vm_ivertex iv1;
	    pix_vm_ivertex iv2;
	    pix_vm_ivertex iv3;
	    iv1.x = v[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.y = v[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.z = v[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.x = v[ 3 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.y = v[ 4 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.z = v[ 5 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.x = v[ 6 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.y = v[ 7 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.z = v[ 8 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    if( vm->zbuf >= 0 )
		pix_vm_gfx_draw_triangle_zbuf( &iv1, &iv2, &iv3, c, vm );
	    else 
		pix_vm_gfx_draw_triangle( &iv1, &iv2, &iv3, c, vm );
	    iv1.x = v[ 9 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.y = v[ 10 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.z = v[ 11 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    if( vm->zbuf >= 0 )
		pix_vm_gfx_draw_triangle_zbuf( &iv3, &iv2, &iv1, c, vm );
	    else 
		pix_vm_gfx_draw_triangle( &iv3, &iv2, &iv1, c, vm );
	}
    }
    else 
    {
	PIX_INT x, y, xsize, ysize;
	
	GET_VAL_FROM_STACK( x, 0, PIX_INT );
	GET_VAL_FROM_STACK( y, 1, PIX_INT );
	GET_VAL_FROM_STACK( xsize, 2, PIX_INT );
	GET_VAL_FROM_STACK( ysize, 3, PIX_INT );
	
	x += screen_hxsize;
	y += screen_hysize;
	
	if( fn_num == FN_BOX )
	    pix_vm_gfx_draw_box( x, y, xsize, ysize, c, vm );
	else 
	    pix_vm_gfx_draw_fbox( x, y, xsize, ysize, c, vm );
    }
}

void fn_pixi( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( vm->screen_ptr == 0 ) return;

    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    PIX_FLOAT x, y, xscale, yscale;
    PIX_INT tx, ty, txsize, tysize;
    COLOR c = get_color( 255, 255, 255 );
    x = 0;
    y = 0;
    xscale = 1;
    yscale = 1;
    tx = 0;
    ty = 0;
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );

    if( (unsigned)cnum >= (unsigned)vm->c_num ) return;
    pix_vm_container* cont = vm->c[ cnum ];
    if( cont == 0 ) return;
    
    if( cont->opt_data && cont->opt_data->hdata )
    {
	//Try to auto-play animation:
	int f = pix_vm_container_hdata_autoplay_control( cnum, vm );
	if( f >= 0 ) pix_vm_container_hdata_unpack_frame( cnum, vm );
    }

    txsize = cont->xsize;
    tysize = cont->ysize;

    while( 1 )
    {
	if( pars_num >= 2 ) GET_VAL_FROM_STACK( x, 1, PIX_FLOAT ) else break;
        if( pars_num >= 3 ) GET_VAL_FROM_STACK( y, 2, PIX_FLOAT ) else break;
        if( pars_num >= 4 ) GET_VAL_FROM_STACK( c, 3, COLOR ) else break;
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( xscale, 4, PIX_FLOAT ) else break;
        if( pars_num >= 6 ) GET_VAL_FROM_STACK( yscale, 5, PIX_FLOAT ) else break;
	if( xscale == 0 ) return;
	if( yscale == 0 ) return;
        if( pars_num >= 7 ) GET_VAL_FROM_STACK( tx, 6, PIX_INT ) else break;
        if( pars_num >= 8 ) GET_VAL_FROM_STACK( ty, 7, PIX_INT ) else break;
        if( pars_num >= 9 ) GET_VAL_FROM_STACK( txsize, 8, PIX_INT ) else break;
        if( pars_num >= 10 ) GET_VAL_FROM_STACK( tysize, 9, PIX_INT ) else break;
        break;
    }
    
    PIX_FLOAT xsize = (PIX_FLOAT)txsize * xscale;
    PIX_FLOAT ysize = (PIX_FLOAT)tysize * yscale;
        
    pix_vm_gfx_draw_container( cnum, x - xsize / 2, y - ysize / 2, xsize, ysize, tx, ty, txsize, tysize, c, vm );
}

void fn_triangles3d( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 2 ) return;
    
    PIX_CID vertices;
    PIX_CID triangles;
    PIX_INT tnum2 = -1;
    
    GET_VAL_FROM_STACK( vertices, 0, PIX_CID );
    GET_VAL_FROM_STACK( triangles, 1, PIX_CID );
    while( 1 )
    {
        if( pars_num >= 3 ) GET_VAL_FROM_STACK( tnum2, 2, PIX_INT ) else break;
        break;
    }

    if( tnum2 == 0 ) return;
    
    pix_vm_container* v_cont = pix_vm_get_container( vertices, vm );
    pix_vm_container* t_cont = pix_vm_get_container( triangles, vm );
    if( v_cont == 0 ) return;
    if( t_cont == 0 ) return;
    if( v_cont->data == 0 ) return;
    if( t_cont->data == 0 ) return;
    if( v_cont->size < 8 ) return;
    if( t_cont->size < 8 ) return;

    int screen_hxsize = vm->screen_xsize / 2;
    int screen_hysize = vm->screen_ysize / 2;

    size_t vnum = v_cont->size / 8;
    size_t tnum = t_cont->size / 8;
    if( tnum2 > 0 )
    {
	if( tnum2 < tnum )
	    tnum = tnum2;
    }
    for( size_t t = 0; t < tnum; t++ )
    {
	size_t ts = pix_vm_get_container_int_element( triangles, t * 8 + 7, vm );
	PIX_INT v1num = pix_vm_get_container_int_element( triangles, ts * 8 + 0, vm );
	PIX_INT v2num = pix_vm_get_container_int_element( triangles, ts * 8 + 1, vm );
	PIX_INT v3num = pix_vm_get_container_int_element( triangles, ts * 8 + 2, vm );
	COLOR color = (COLOR)pix_vm_get_container_int_element( triangles, ts * 8 + 3, vm );
	PIX_INT texture = pix_vm_get_container_int_element( triangles, ts * 8 + 4, vm );
	PIX_INT transp = pix_vm_get_container_int_element( triangles, ts * 8 + 5, vm );
	if( transp <= 0 ) continue;
	if( transp >= 255 ) transp = 255;
	uint8_t prev_transp = vm->transp;
	uint8_t new_transp = ( (int)transp * (int)vm->transp ) / 256;
	if( new_transp == 0 ) continue;
	vm->transp = new_transp;
	if( (unsigned)v1num >= vnum ) continue;
	if( (unsigned)v2num >= vnum ) continue;
	if( (unsigned)v3num >= vnum ) continue;
	PIX_FLOAT v1[ 5 ];
	PIX_FLOAT v2[ 5 ];
	PIX_FLOAT v3[ 5 ];
	v1[ 0 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 0, vm );
	v1[ 1 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 1, vm );
	v1[ 2 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 2, vm );
	v2[ 0 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 0, vm );
	v2[ 1 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 1, vm );
	v2[ 2 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 2, vm );
	v3[ 0 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 0, vm );
	v3[ 1 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 1, vm );
	v3[ 2 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 2, vm );
	if( texture >= 0 )
	{
	    v1[ 3 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 3, vm );
	    v1[ 4 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 4, vm );
	    v2[ 3 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 3, vm );
	    v2[ 4 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 4, vm );
	    v3[ 3 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 3, vm );
	    v3[ 4 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 4, vm );
	}
#ifdef OPENGL
	if( vm->screen == PIX_GL_SCREEN )
	{
	    pix_vm_container_gl_data* gl = 0;
	    gl_program_struct* p;
	    float v[ 3 * 3 ];
	    v[ 0 ] = v1[ 0 ]; v[ 1 ] = v1[ 1 ]; v[ 2 ] = v1[ 2 ];
	    v[ 3 ] = v2[ 0 ]; v[ 4 ] = v2[ 1 ]; v[ 5 ] = v2[ 2 ];
	    v[ 6 ] = v3[ 0 ]; v[ 7 ] = v3[ 1 ]; v[ 8 ] = v3[ 2 ];
	    if( texture == -1 )
	    {
	        p = vm->gl_prog_solid;
	    }
	    else
	    {
		gl = pix_vm_create_container_gl_data( texture, vm );
    		if( gl == 0 ) goto triangle_draw_end;
    		if( gl->texture_format == GL_ALPHA )
	    	    p = vm->gl_prog_tex_alpha_solid;
        	else
    	    	    p = vm->gl_prog_tex_rgba_solid;
    	    }
    	    if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
	    if( vm->gl_current_prog != p )
            {
    	        pix_vm_gl_use_prog( p, vm );
    	        uint attr = 1 << GL_PROG_ATT_POSITION;
    		if( texture != -1 )
    		{
	    	    attr |= 1 << GL_PROG_ATT_TEX_COORD;
            	    glActiveTexture( GL_TEXTURE0 );
        	    glUniform1i( p->uniforms[ GL_PROG_UNI_TEXTURE ], 0 );
        	}
    	        gl_enable_attributes( p, attr );
    	    }
    	    GL_CHANGE_PROG_COLOR( p, color, vm->transp );
    	    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], 3, GL_FLOAT, false, 0, v );
	    if( texture != -1 )
	    {
    		float t[ 2 * 3 ];
    		t[ 0 ] = v1[ 3 ] / gl->xsize; t[ 1 ] = v1[ 4 ] / gl->ysize;
    		t[ 2 ] = v2[ 3 ] / gl->xsize; t[ 3 ] = v2[ 4 ] / gl->ysize;
    		t[ 4 ] = v3[ 3 ] / gl->xsize; t[ 5 ] = v3[ 4 ] / gl->ysize;
		GL_BIND_TEXTURE( vm->wm, gl->texture_id );
		GL_CHANGE_PROG_COLOR( p, color, vm->transp );
		glVertexAttribPointer( p->attributes[ GL_PROG_ATT_TEX_COORD ], 2, GL_FLOAT, false, 0, t );
    	    }
    	    glDrawArrays( GL_TRIANGLES, 0, 3 );
	    goto triangle_draw_end;
	}
#endif
	if( vm->t_enabled )
	{
	    pix_vm_gfx_vertex_transform( v1, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	    pix_vm_gfx_vertex_transform( v2, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	    pix_vm_gfx_vertex_transform( v3, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	}
	v1[ 0 ] += screen_hxsize;
	v1[ 1 ] += screen_hysize;
	v2[ 0 ] += screen_hxsize;
	v2[ 1 ] += screen_hysize;
	v3[ 0 ] += screen_hxsize;
	v3[ 1 ] += screen_hysize;
	if( texture == -1 )
	{
	    //No texture:
	    pix_vm_ivertex iv1;
	    pix_vm_ivertex iv2;
	    pix_vm_ivertex iv3;
	    iv1.x = v1[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.y = v1[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv1.z = v1[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.x = v2[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.y = v2[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv2.z = v2[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.x = v3[ 0 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.y = v3[ 1 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    iv3.z = v3[ 2 ] * ( 1 << PIX_FIXED_MATH_PREC );
	    if( vm->zbuf >= 0 )
		pix_vm_gfx_draw_triangle_zbuf( &iv1, &iv2, &iv3, color, vm );
	    else 
		pix_vm_gfx_draw_triangle( &iv1, &iv2, &iv3, color, vm );
	}
	else
	{
	    pix_vm_gfx_draw_triangle_t( v1, v2, v3, texture, color, vm );
	}
triangle_draw_end:
	vm->transp = prev_transp;
    }
}

void fn_sort_triangles3d( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 2 ) return;
    
    PIX_CID vertices;
    PIX_CID triangles;
    PIX_INT tnum2 = -1;
    
    GET_VAL_FROM_STACK( vertices, 0, PIX_CID );
    GET_VAL_FROM_STACK( triangles, 1, PIX_CID );
    while( 1 )
    {
        if( pars_num >= 3 ) GET_VAL_FROM_STACK( tnum2, 2, PIX_INT ) else break;
        break;
    }

    if( tnum2 == 0 ) return;

    pix_vm_container* v_cont = pix_vm_get_container( vertices, vm );
    pix_vm_container* t_cont = pix_vm_get_container( triangles, vm );
    if( v_cont == 0 ) return;
    if( t_cont == 0 ) return;
    if( v_cont->data == 0 ) return;
    if( t_cont->data == 0 ) return;
    if( v_cont->size < 8 ) return;
    if( t_cont->size < 8 ) return;
    
    PIX_INT vnum = v_cont->size / 8;
    PIX_INT tnum = t_cont->size / 8;
    if( tnum2 > 0 )
    {
	if( tnum2 < tnum )
	    tnum = tnum2;
    }
    PIX_FLOAT* zz = (PIX_FLOAT*)smem_new( tnum * sizeof( PIX_FLOAT ) );
    int* order = (int*)smem_new( tnum * sizeof( int ) );
    if( zz == 0 ) return;
    if( order == 0 ) return;
    for( PIX_INT t = 0; t < tnum; t++ )
    {
	order[ t ] = t;
	zz[ t ] = 0;
	PIX_INT v1num = pix_vm_get_container_int_element( triangles, t * 8 + 0, vm );
	PIX_INT v2num = pix_vm_get_container_int_element( triangles, t * 8 + 1, vm );
	PIX_INT v3num = pix_vm_get_container_int_element( triangles, t * 8 + 2, vm );
	if( (unsigned)v1num >= vnum ) continue;
	if( (unsigned)v2num >= vnum ) continue;
	if( (unsigned)v3num >= vnum ) continue;
	PIX_FLOAT v1[ 3 ];
	PIX_FLOAT v2[ 3 ];
	PIX_FLOAT v3[ 3 ];
	v1[ 0 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 0, vm );
	v1[ 1 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 1, vm );
	v1[ 2 ] = pix_vm_get_container_float_element( vertices, v1num * 8 + 2, vm );
	v2[ 0 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 0, vm );
	v2[ 1 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 1, vm );
	v2[ 2 ] = pix_vm_get_container_float_element( vertices, v2num * 8 + 2, vm );
	v3[ 0 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 0, vm );
	v3[ 1 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 1, vm );
	v3[ 2 ] = pix_vm_get_container_float_element( vertices, v3num * 8 + 2, vm );
	if( vm->t_enabled )
	{
	    pix_vm_gfx_vertex_transform( v1, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	    pix_vm_gfx_vertex_transform( v2, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	    pix_vm_gfx_vertex_transform( v3, vm->t_matrix + ( vm->t_matrix_sp * 16 ) );
	}
	zz[ t ] = ( v1[ 2 ] + v2[ 2 ] + v3[ 2 ] ) / 3;
    }
    //Sort (insertion alg.):
    for( PIX_INT i = 1; i < tnum; i++ )
    {
	PIX_INT j = i;
	PIX_FLOAT z = zz[ i ];
	int o = order[ i ];
	while( j > 0 && zz[ j - 1 ] > z )
	{
	    zz[ j ] = zz[ j - 1 ];
	    order[ j ] = order[ j - 1 ];
	    j--;
	}
	if( j != i )
	{
	    zz[ j ] = z;
	    order[ j ] = o;
	}
    }
    /*while( 1 )
    {
	bool sort = 0;
	for( size_t t = 0; t < tnum - 1; t++ )
	{
	    if( zz[ t ] > zz[ t + 1 ] )
	    {
		PIX_FLOAT tz = zz[ t ];
		zz[ t ] = zz[ t + 1 ];
		zz[ t + 1 ] = tz;
		int to = order[ t ];
		order[ t ] = order[ t + 1 ];
		order[ t + 1 ] = to;
		sort = 1;
	    }
	}
	if( sort == 0 ) break;
    }*/
    for( size_t t = 0; t < tnum; t++ )
    {
	pix_vm_set_container_int_element( triangles, t * 8 + 7, order[ t ], vm );
    }
    smem_free( zz );
    smem_free( order );
}

void fn_set_key_color( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    bool key_color = 0;
    PIX_INT key;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    if( pars_num > 1 )
    {
	key_color = 1;
	GET_VAL_FROM_STACK( key, 1, PIX_INT );
    }
    
    if( key_color )
    {
	pix_vm_set_container_key_color( cnum, key, vm );
    }
    else 
    {
	pix_vm_remove_container_key_color( cnum, vm );
    }
}

void fn_get_key_color( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_get_container_key_color( cnum, vm );
}

void fn_set_alpha( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    PIX_CID alpha_cnum = -1;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    if( pars_num > 1 )
    {
	GET_VAL_FROM_STACK( alpha_cnum, 1, PIX_CID );
    }
    
    pix_vm_set_container_alpha( cnum, alpha_cnum, vm );
}

void fn_get_alpha( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_get_container_alpha( cnum, vm );
}

void fn_print( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    PIX_CID cnum;
    PIX_FLOAT x = 0, y = 0;
    COLOR c = get_color( 255, 255, 255 );
    int align = 0;
    int max_xsize = 0;
    PIX_INT str_offset = 0;
    PIX_INT str_size = 0;
    
    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    while( 1 )
    {
	if( pars_num > 1 ) { GET_VAL_FROM_STACK( x, 1, PIX_FLOAT ); } else break;
	if( pars_num > 2 ) { GET_VAL_FROM_STACK( y, 2, PIX_FLOAT ); } else break;
	if( pars_num > 3 ) { GET_VAL_FROM_STACK( c, 3, COLOR ); } else break;
	if( pars_num > 4 ) { GET_VAL_FROM_STACK( align, 4, int ); } else break;
	if( pars_num > 5 ) { GET_VAL_FROM_STACK( max_xsize, 5, int ); } else break;
	if( pars_num > 6 ) { GET_VAL_FROM_STACK( str_offset, 6, PIX_INT ); } else break;
	if( pars_num > 7 ) { GET_VAL_FROM_STACK( str_size, 7, PIX_INT ); } else break;
	break;
    }
    
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* cont = vm->c[ cnum ];
	if( cont && cont->data && cont->size )
	{
	    PIX_INT real_size = cont->size * g_pix_container_type_sizes[ cont->type ];
	    if( str_offset < 0 ) str_offset = 0;
	    if( str_size <= 0 ) str_size = real_size;
	    if( str_offset + str_size > real_size ) str_size = real_size - str_offset;
	    if( str_size > 0 )
	    {
		pix_vm_gfx_draw_text( (char*)cont->data + str_offset, str_size, x, y, align, c, max_xsize, 0, 0, 0, vm );
	    }
	}
    }
}

void fn_get_text_xysize( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 1 ) return;

    PIX_CID cnum;
    int align = 0;
    int max_xsize = 0;
    PIX_INT str_offset = 0;
    PIX_INT str_size = 0;

    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
    if( pars_num > 1 ) GET_VAL_FROM_STACK( align, 1, int );
    if( pars_num > 2 ) GET_VAL_FROM_STACK( max_xsize, 2, int );
    if( pars_num > 3 ) GET_VAL_FROM_STACK( str_offset, 3, PIX_INT );
    if( pars_num > 4 ) GET_VAL_FROM_STACK( str_size, 4, PIX_INT );

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = 0;

    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
	pix_vm_container* cont = vm->c[ cnum ];
	if( cont && cont->data && cont->size )
	{
	    PIX_INT real_size = cont->size * g_pix_container_type_sizes[ cont->type ];
	    if( str_offset < 0 ) str_offset = 0;
	    if( str_size <= 0 ) str_size = real_size;
	    if( str_offset + str_size > real_size ) str_size = real_size - str_offset;
	    if( str_size > 0 )
	    {
		int xsize;
		int ysize;
		pix_vm_gfx_draw_text( (char*)cont->data + str_offset, str_size, 0, 0, align, 1, max_xsize, &xsize, &ysize, 1, vm );
		PIX_INT rv;
		switch( fn_num )
		{
		    case FN_GET_TEXT_XSIZE: rv = xsize; break;
		    case FN_GET_TEXT_YSIZE: rv = ysize; break;
		    default: rv = ( xsize & 0xFFFF ) | ( ( ysize & 0xFFFF ) << 16 ); break;
		}
		stack[ sp2 ].i = rv;
	    }
	}
    }
}

void fn_set_font( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num < 2 ) return;

    uint32_t first_char;
    PIX_CID cnum;
    int xchars = 0;
    int ychars = 0;

    uint32_t last_char = 0;
    int char_xsize = 0;
    int char_ysize = 0;
    int char_xsize2 = 0;
    int char_ysize2 = 0;
    int grid_xoffset = 0;
    int grid_yoffset = 0;
    int grid_cell_xsize = 0;
    int grid_cell_ysize = 0;

    GET_VAL_FROM_STACK( first_char, 0, uint32_t );
    GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
    if( pars_num >= 3 )	GET_VAL_FROM_STACK( xchars, 2, int );
    if( pars_num >= 4 )	GET_VAL_FROM_STACK( ychars, 3, int );

    if( pars_num >= 5 ) GET_VAL_FROM_STACK( last_char, 4, uint32_t );
    if( pars_num >= 6 ) GET_VAL_FROM_STACK( char_xsize, 5, int );
    if( pars_num >= 7 ) GET_VAL_FROM_STACK( char_ysize, 6, int );
    if( pars_num >= 8 ) GET_VAL_FROM_STACK( char_xsize2, 7, int );
    if( pars_num >= 9 ) GET_VAL_FROM_STACK( char_ysize2, 8, int );
    if( pars_num >= 10 ) GET_VAL_FROM_STACK( grid_xoffset, 9, int );
    if( pars_num >= 11 ) GET_VAL_FROM_STACK( grid_yoffset, 10, int );
    if( pars_num >= 12 ) GET_VAL_FROM_STACK( grid_cell_xsize, 11, int );
    if( pars_num >= 13 ) GET_VAL_FROM_STACK( grid_cell_ysize, 12, int );

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_set_font(
	first_char,
	last_char,
	cnum,
        xchars,
	ychars,
	char_xsize,
	char_ysize,
	char_xsize2,
	char_ysize2,
	grid_xoffset,
	grid_yoffset,
	grid_cell_xsize,
	grid_cell_ysize,
	vm );
}

void fn_get_font( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 1 ) return;
    
    uint32_t char_code;
    GET_VAL_FROM_STACK( char_code, 0, uint32_t );
    
    pix_vm_font* font = pix_vm_get_font_for_char( char_code, vm );

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    if( font )
	stack[ sp2 ].i = font->font;
    else
	stack[ sp2 ].i = -1;
}

void fn_effector( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 2 ) return;
    
    int transp = vm->transp;
    if( transp == 0 ) return;
    
    int type;
    int power;
    COLOR color = get_color( 255, 255, 255 );
    int sx = -vm->screen_xsize / 2;
    int sy = -vm->screen_ysize / 2;
    int xsize = vm->screen_xsize;
    int ysize = vm->screen_ysize;
    int x_step = 1;
    int y_step = 1;
    
    int pnum = 0;
    GET_VAL_FROM_STACK( type, 0, int );
    GET_VAL_FROM_STACK( power, 1, int );
    pnum = 2;
    while( 1 )
    {
	if( pars_num > pnum ) { GET_VAL_FROM_STACK( color, pnum, COLOR ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( sx, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( sy, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( xsize, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( ysize, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( x_step, pnum, int ); } else break; pnum++;
	if( pars_num > pnum ) { GET_VAL_FROM_STACK( y_step, pnum, int ); } else break; pnum++;
	break;
    }

    sx += vm->screen_xsize / 2;
    sy += vm->screen_ysize / 2;
    
    if( sx + xsize < 0 ) return;
    if( sy + ysize < 0 ) return;
    if( sx < 0 ) { xsize -= -sx; sx = 0; }
    if( sy < 0 ) { ysize -= -sy; sy = 0; }
    if( sx + xsize > vm->screen_xsize ) xsize = vm->screen_xsize - sx;
    if( sy + ysize > vm->screen_ysize ) ysize = vm->screen_ysize - sy;
    if( xsize <= 0 ) return;
    if( ysize <= 0 ) return;
    if( x_step <= 0 ) x_step = 1;
    if( y_step <= 0 ) y_step = 1;

    int* cols_r = 0;
    int* cols_g = 0;
    int* cols_b = 0;
    switch( type )
    {
	case PIX_EFFECT_HBLUR:
	case PIX_EFFECT_VBLUR:
	    if( vm->effector_colors_r == 0 ) vm->effector_colors_r = (int*)smem_new( 512 * sizeof( int ) );
	    if( vm->effector_colors_g == 0 ) vm->effector_colors_g = (int*)smem_new( 512 * sizeof( int ) );
	    if( vm->effector_colors_b == 0 ) vm->effector_colors_b = (int*)smem_new( 512 * sizeof( int ) );
	    cols_r = vm->effector_colors_r;
	    cols_g = vm->effector_colors_g;
	    cols_b = vm->effector_colors_b;
	    if( cols_r == 0 ) return;
	    if( cols_g == 0 ) return;
	    if( cols_b == 0 ) return;
	    break;
    }
    
    switch( type )
    {
	case PIX_EFFECT_NOISE:
	    {
		transp *= power;
		transp /= 256;
		if( transp <= 0 ) break;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
		    for( int cx = 0; cx < xsize; cx += x_step )
		    {
			*scr = blend( *scr, color, ( transp * ( pseudo_random() & 255 ) ) / 256 );
			scr += x_step;
		    }
		}
	    }
	    break;
	case PIX_EFFECT_SPREAD_LEFT:
	    {
		power /= 2;
		if( power <= 0 ) break;
		power++;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize;
		    for( int cx = sx; cx < sx + xsize; cx += x_step )
		    {
		        int cx2;
		        int rnd = ( ( pseudo_random() & 4095 ) * power ) / 4096;
		        cx2 = cx + rnd;
		        if( cx2 >= vm->screen_xsize ) cx2 = vm->screen_xsize - 1;
		        scr[ cx ] = blend( scr[ cx ], scr[ cx2 ], transp );
		    }
		}
	    }
	    break;
	case PIX_EFFECT_SPREAD_RIGHT:
	    {
		power /= 2;
		if( power <= 0 ) break;
		power++;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize;
		    for( int cx = sx + ( ( xsize - 1 ) / x_step ) * x_step; cx >= sx; cx -= x_step )
		    {
		        int cx2;
		        int rnd = ( ( pseudo_random() & 4095 ) * power ) / 4096;
		        cx2 = cx - rnd;
		        if( cx2 < 0 ) cx2 = 0;
		        scr[ cx ] = blend( scr[ cx ], scr[ cx2 ], transp );
		    }
		}
	    }
	    break;
	case PIX_EFFECT_SPREAD_UP:
	    {
		power /= 2;
		if( power <= 0 ) break;
		power++;
		for( int cx = 0; cx < xsize; cx += x_step )
		{
		    COLORPTR scr = vm->screen_ptr + sx + cx;
		    for( int cy = sy; cy < sy + ysize; cy += y_step )
		    {
		        int cy2;
		        int rnd = ( ( pseudo_random() & 4095 ) * power ) / 4096;
		        cy2 = cy + rnd;
		        if( cy2 >= vm->screen_ysize ) cy2 = vm->screen_ysize - 1;
		        COLORPTR s = &scr[ cy * vm->screen_xsize ];
		        *s = blend( *s, scr[ cy2 * vm->screen_xsize ], transp );
		    }
		}
	    }
	    break;
	case PIX_EFFECT_SPREAD_DOWN:
	    {
		power /= 2;
		if( power <= 0 ) break;
		power++;
		for( int cx = 0; cx < xsize; cx += x_step )
		{
		    COLORPTR scr = vm->screen_ptr + sx + cx;
		    for( int cy = sy + ( ( ysize - 1 ) / y_step ) * y_step; cy >= sy; cy -= y_step )
		    {
		        int cy2;
		        int rnd = ( ( pseudo_random() & 4095 ) * power ) / 4096;
		        cy2 = cy - rnd;
		        if( cy2 < 0 ) cy2 = 0;
		        COLORPTR s = &scr[ cy * vm->screen_xsize ];
		        *s = blend( *s, scr[ cy2 * vm->screen_xsize ], transp );
		    }
		}
	    }
	    break;
	case PIX_EFFECT_HBLUR:
	    {
		power /= 2;
		if( power > 255 ) power = 255;
		if( power <= 0 ) break;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
		    int col_ptr = 0;
		    COLOR pcol = *scr;
		    int start_r = red( pcol );
		    int start_g = green( pcol );
		    int start_b = blue( pcol );
		    cols_r[ 0 ] = 0;
		    cols_g[ 0 ] = 0;
		    cols_b[ 0 ] = 0;
		    for( int i = 1; i < power + 1; i++ ) 
		    { 
			cols_r[ i ] = start_r * i; 
			cols_g[ i ] = start_g * i; 
			cols_b[ i ] = start_b * i; 
		    }
		    col_ptr = power + 1;
		    for( int i = 0; ( i < power ) && ( i < xsize ); i++ ) 
		    {
			pcol = scr[ i ];
			cols_r[ col_ptr ] = cols_r[ col_ptr - 1 ] + red( pcol );
			cols_g[ col_ptr ] = cols_g[ col_ptr - 1 ] + green( pcol );
			cols_b[ col_ptr ] = cols_b[ col_ptr - 1 ] + blue( pcol );
			col_ptr++;
		    }
		    if( power > xsize )
		    {
			pcol = scr[ xsize - 1 ];
			int rr = red( pcol );
			int gg = green( pcol );
			int bb = blue( pcol );
			for( int i = 0; i < power - xsize; i++ ) 
			{
			    cols_r[ col_ptr ] = cols_r[ col_ptr - 1 ] + rr;
			    cols_g[ col_ptr ] = cols_g[ col_ptr - 1 ] + gg;
			    cols_b[ col_ptr ] = cols_b[ col_ptr - 1 ] + bb;
			    col_ptr++;
			}
		    }
		    for( int cx = 0; cx < xsize; cx += x_step )
		    {
			int prev_ptr = ( col_ptr - 1 ) & 511;
			int new_ptr = cx + power; if( new_ptr > xsize - 1 ) new_ptr = xsize - 1;
			pcol = scr[ new_ptr ];
			cols_r[ col_ptr ] = cols_r[ prev_ptr ] + red( pcol );
			cols_g[ col_ptr ] = cols_g[ prev_ptr ] + green( pcol );
			cols_b[ col_ptr ] = cols_b[ prev_ptr ] + blue( pcol );
			COLOR res = get_color(
					      ( cols_r[ col_ptr ] - cols_r[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 ),
					      ( cols_g[ col_ptr ] - cols_g[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 ),
					      ( cols_b[ col_ptr ] - cols_b[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 )
					      );
			scr[ cx ] = fast_blend( scr[ cx ], res, transp );
			col_ptr ++;
			col_ptr &= 511;
		    }
		}
	    }
	    break;
	case PIX_EFFECT_VBLUR:
	    {
		power /= 2;
		if( power > 255 ) power = 255;
		if( power <= 0 ) break;
		for( int cx = 0; cx < xsize; cx += x_step )
		{
		    COLORPTR scr = vm->screen_ptr + sy * vm->screen_xsize + sx + cx;
		    int col_ptr = 0;
		    COLOR pcol = *scr;
		    int start_r = red( pcol );
		    int start_g = green( pcol );
		    int start_b = blue( pcol );
		    cols_r[ 0 ] = 0;
		    cols_g[ 0 ] = 0;
		    cols_b[ 0 ] = 0;
		    int i;
		    for( i = 1; i < power + 1; i++ ) 
		    { 
			cols_r[ i ] = start_r * i; 
			cols_g[ i ] = start_g * i; 
			cols_b[ i ] = start_b * i; 
		    }
		    col_ptr = power + 1;
		    for( int i = 0, i2 = 0; ( i < power ) && ( i < ysize ); i++ ) 
		    {
			pcol = scr[ i2 ];
			cols_r[ col_ptr ] = cols_r[ col_ptr - 1 ] + red( pcol );
			cols_g[ col_ptr ] = cols_g[ col_ptr - 1 ] + green( pcol );
			cols_b[ col_ptr ] = cols_b[ col_ptr - 1 ] + blue( pcol );
			i2 += vm->screen_xsize;
			col_ptr++;
		    }
		    if( power > ysize )
		    {
			pcol = scr[ ( ysize - 1 ) * vm->screen_xsize ];
			int rr = red( pcol );
			int gg = green( pcol );
			int bb = blue( pcol );
			for( int i = 0; i < power - ysize; i++ )
			{ 
			    cols_r[ col_ptr ] = cols_r[ col_ptr - 1 ] + rr;
			    cols_g[ col_ptr ] = cols_g[ col_ptr - 1 ] + gg;
			    cols_b[ col_ptr ] = cols_b[ col_ptr - 1 ] + bb;
			    col_ptr++;
			}
		    }
		    int power2 = power * vm->screen_xsize;
		    int end_ptr = ( ysize - 1 ) * vm->screen_xsize;
		    for( int cy = 0; cy < ysize * vm->screen_xsize; cy += y_step * vm->screen_xsize )
		    {
			int prev_ptr = ( col_ptr - 1 ) & 511;
			int new_ptr = cy + power2; if( new_ptr > end_ptr ) new_ptr = end_ptr;
			pcol = scr[ new_ptr ];
			cols_r[ col_ptr ] = cols_r[ prev_ptr ] + red( pcol );
			cols_g[ col_ptr ] = cols_g[ prev_ptr ] + green( pcol );
			cols_b[ col_ptr ] = cols_b[ prev_ptr ] + blue( pcol );
			COLOR res = get_color(
					      ( cols_r[ col_ptr ] - cols_r[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 ),
					      ( cols_g[ col_ptr ] - cols_g[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 ),
					      ( cols_b[ col_ptr ] - cols_b[ (col_ptr-(power*2+1)) & 511 ] ) / ( power * 2 + 1 )
					      );
			scr[ cy ] = fast_blend( scr[ cy ], res, transp );
			col_ptr ++;
			col_ptr &= 511;
		    }
		}
	    }
	    break;
	case PIX_EFFECT_COLOR:
	    {
		transp *= power;
		transp /= 256;
		if( transp <= 0 ) break;
		if( transp > 255 ) transp = 255;
		for( int cy = 0; cy < ysize; cy += y_step )
		{
		    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
		    for( int cx = 0; cx < xsize; cx += x_step )
		    {
			*scr = blend( *scr, color, transp );
			scr += x_step;
		    }
		}
	    }
    }
}

void fn_color_gradient( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 8 ) return;
    
    int transp = vm->transp;
    if( transp == 0 ) return;

    COLOR c[ 4 ];
    int t[ 4 ];
    int sx = -vm->screen_xsize / 2;
    int sy = -vm->screen_ysize / 2;
    int xsize = vm->screen_xsize;
    int ysize = vm->screen_ysize;
    int x_step = 1;
    int y_step = 1;

    bool zero_transp = 1;
    bool no_transp = 1;
    for( int i = 0; i < 4; i++ )
    {
	GET_VAL_FROM_STACK( c[ i ], i * 2, COLOR );
	GET_VAL_FROM_STACK( t[ i ], i * 2 + 1, int );
	if( transp < 255 ) t[ i ] = ( t[ i ] * transp ) / 255;
	if( t[ i ] < 0 ) t[ i ] = 0;
	if( t[ i ] > 255 ) t[ i ] = 255;
	if( t[ i ] != 0 ) zero_transp = 0;
	if( t[ i ] != 255 ) no_transp = 0;
    }
    if( zero_transp ) return;
    int pnum = 8;
    while( 1 )
    {
	if( pars_num > pnum ) { GET_VAL_FROM_STACK( sx, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( sy, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( xsize, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( ysize, pnum, int ); } else break; pnum++;
        if( pars_num > pnum ) { GET_VAL_FROM_STACK( x_step, pnum, int ); } else break; pnum++;
	if( pars_num > pnum ) { GET_VAL_FROM_STACK( y_step, pnum, int ); } else break; pnum++;
	break;
    }

    sx += vm->screen_xsize / 2;
    sy += vm->screen_ysize / 2;

    if( sx + xsize < 0 ) return;
    if( sy + ysize < 0 ) return;
    if( sx >= vm->screen_xsize ) return;
    if( sy >= vm->screen_ysize ) return;
    if( xsize <= 0 ) return;
    if( ysize <= 0 ) return;
    if( x_step <= 0 ) x_step = 1;
    if( y_step <= 0 ) y_step = 1;

    int xd = ( 256 << 8 ) / xsize;
    int xstart = 0;
    int yd = ( 256 << 8 ) / ysize;
    int ystart = 0;

    if( sx < 0 ) { xsize -= -sx; xstart = -sx * xd; sx = 0; }
    if( sy < 0 ) { ysize -= -sy; ystart = -sy * yd; sy = 0; }
    if( sx + xsize > vm->screen_xsize ) xsize = vm->screen_xsize - sx;
    if( sy + ysize > vm->screen_ysize ) ysize = vm->screen_ysize - sy;
    
    xd *= x_step;
    yd *= y_step;

    int yy = ystart;
    if( no_transp )
    {
	for( int cy = 0; cy < ysize; cy += y_step )
	{
	    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
	    int xx = xstart;
	    int v = yy >> 8;
	    COLOR c1 = blend( c[ 0 ], c[ 2 ], v );
	    COLOR c2 = blend( c[ 1 ], c[ 3 ], v );
	    for( int cx = 0; cx < xsize; cx += x_step )
	    {
		*scr = blend( c1, c2, xx >> 8 );
		scr += x_step;
		xx += xd;
	    }
	    yy += yd;
	}
    }
    else
    {
	for( int cy = 0; cy < ysize; cy += y_step )
	{
	    COLORPTR scr = vm->screen_ptr + ( sy + cy ) * vm->screen_xsize + sx;
	    int xx = xstart;
	    int v = yy >> 8;
	    int vv = 256 - v;
	    COLOR c1 = blend( c[ 0 ], c[ 2 ], v );
	    COLOR c2 = blend( c[ 1 ], c[ 3 ], v );
	    int t1 = ( t[ 0 ] * vv + t[ 2 ] * v ) >> 8;
	    int t2 = ( t[ 1 ] * vv + t[ 3 ] * v ) >> 8;
	    for( int cx = 0; cx < xsize; cx += x_step )
	    {
		v = xx >> 8;
		vv = 256 - v;
		int t = ( t1 * vv + t2 * v ) >> 8;
		COLOR pixel = blend( c1, c2, v );
		*scr = blend( *scr, pixel, t );
		scr += x_step;
		xx += xd;
	    }
	    yy += yd;
	}
    }
}

const int YR = 19595, YG = 38470, YB = 7471, CB_R = -11059, CB_G = -21709, CB_B = 32768, CR_R = 32768, CR_G = -27439, CR_B = -5329;
inline uint8_t clamp( int i ) { if( i < 0 ) i = 0; else if( i > 255 ) i = 255; return (uint8_t)i; }
inline float clamp_f( float i ) { if( i < 0 ) i = 0; else if( i > 1 ) i = 1; return i; }
#define SPLIT_PRECISION 9
#define SPLIT_CONV( num ) (int)( (float)num * (float)( 1 << SPLIT_PRECISION ) )
#define SPLIT_AMUL( num ) ( num >> SPLIT_PRECISION )

void fn_split_rgb( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num < 3 ) return;
    
    int rv = -1;

    int direction; //0 - from image to RGB; 1 - from RGB to image;   
    PIX_CID image;
    PIX_CID channels[ 3 ] = { -1, -1, -1 };
    size_t image_offset = 0;
    size_t channel_offset = 0;
    size_t size = -1;
    
    const char* fn_name;
    bool rgb;
    if( fn_num == FN_SPLIT_RGB )
    {
	fn_name = "split_rgb";
	rgb = 1;
    }
    else
    {
	fn_name = "split_ycbcr";
	rgb = 0;
    }
    
    GET_VAL_FROM_STACK( direction, 0, int );
    GET_VAL_FROM_STACK( image, 1, PIX_CID );
    GET_VAL_FROM_STACK( channels[ 0 ], 2, PIX_CID );
    while( 1 )
    {
	if( pars_num > 3 ) { GET_VAL_FROM_STACK( channels[ 1 ], 3, PIX_CID ); } else break;
	if( pars_num > 4 ) { GET_VAL_FROM_STACK( channels[ 2 ], 4, PIX_CID ); } else break;
	if( pars_num > 5 ) { GET_VAL_FROM_STACK( image_offset, 5, size_t ); } else break;
	if( pars_num > 6 ) { GET_VAL_FROM_STACK( channel_offset, 6, size_t ); } else break;
	if( pars_num > 7 ) { GET_VAL_FROM_STACK( size, 7, size_t ); } else break;
	break;
    }
    
    while( 1 )
    {
	pix_vm_container* image_cont = pix_vm_get_container( image, vm );
	if( image_cont == 0 ) break;
	if( g_pix_container_type_sizes[ image_cont->type ] != COLORLEN )
	{
	    PIX_VM_LOG( "%s: image container must be of PIXEL type\n", fn_name );
	    break;
	}
	pix_vm_container* channels_cont[ 3 ];
	int channel_type = -1;
	for( int i = 0; i < 3; i++ )
	{
	    channels_cont[ i ] = pix_vm_get_container( channels[ i ], vm );
	    if( channels_cont[ i ] )
	    {
		if( channel_type == -1 ) 
		    channel_type = channels_cont[ i ]->type;
		else
		{
		    if( channels_cont[ i ]->type != channel_type )
		    {
			PIX_VM_LOG( "%s: all channels must have the same type\n", fn_name );
			channel_type = -1;
			break;
		    }
		}
	    }
	}
	if( channel_type == -1 )
	{
	    PIX_VM_LOG( "%s: can't split the image (no channels selected)\n", fn_name );
	    break;
	}
	
	if( size == -1 ) size = image_cont->size;
	if( image_offset >= image_cont->size ) break;
	if( image_offset + size > image_cont->size )
	{
	    size = image_cont->size - image_offset;
	}
	
	COLORPTR image_ptr = (COLORPTR)image_cont->data + image_offset;
	void* ch[ 3 ];
	for( int i = 0; i < 3; i++ )
	{
	    if( channels_cont[ i ] && channels_cont[ i ]->data ) 
	    {
		if( channel_offset >= channels_cont[ i ]->size ) size = 0;
		if( channel_offset + size > channels_cont[ i ]->size )
		{
		    size = channels_cont[ i ]->size - channel_offset;
		}
		ch[ i ] = (void*)( (uint8_t*)channels_cont[ i ]->data + channel_offset * g_pix_container_type_sizes[ channel_type ] );
	    }
	    else ch[ i ] = 0;
	}
	if( direction == 0 )
	{
	    //From image to RGB / YCbCr:
	    switch( channel_type )
	    {
		case PIX_CONTAINER_TYPE_INT8:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((uint8_t*)ch[ 0 ])[ p ] = red( pixel );
				if( ch[ 1 ] ) ((uint8_t*)ch[ 1 ])[ p ] = green( pixel );
				if( ch[ 2 ] ) ((uint8_t*)ch[ 2 ])[ p ] = blue( pixel );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				COLOR pixel = image_ptr[ p ];
				int r = red( pixel );
				int g = green( pixel );
				int b = blue( pixel );
				if( ch[ 0 ] ) ((uint8_t*)ch[ 0 ])[ p ] = (uint8_t)( ( r * YR + g * YG + b * YB + 32768 ) >> 16 );
				if( ch[ 1 ] ) ((uint8_t*)ch[ 1 ])[ p ] = clamp( 128 + ( ( r * CB_R + g * CB_G + b * CB_B + 32768 ) >> 16 ) );
				if( ch[ 2 ] ) ((uint8_t*)ch[ 2 ])[ p ] = clamp( 128 + ( ( r * CR_R + g * CR_G + b * CR_B + 32768 ) >> 16 ) );
			    }
			rv = 0;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT16:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((int16_t*)ch[ 0 ])[ p ] = red( pixel );
				if( ch[ 1 ] ) ((int16_t*)ch[ 1 ])[ p ] = green( pixel );
				if( ch[ 2 ] ) ((int16_t*)ch[ 2 ])[ p ] = blue( pixel );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				COLOR pixel = image_ptr[ p ];
				int r = red( pixel );
				int g = green( pixel );
				int b = blue( pixel );
				if( ch[ 0 ] ) ((uint16_t*)ch[ 0 ])[ p ] = (uint16_t)( ( r * YR + g * YG + b * YB + 32768 ) >> 16 );
				if( ch[ 1 ] ) ((uint16_t*)ch[ 1 ])[ p ] = clamp( 128 + ( ( r * CB_R + g * CB_G + b * CB_B + 32768 ) >> 16 ) );
				if( ch[ 2 ] ) ((uint16_t*)ch[ 2 ])[ p ] = clamp( 128 + ( ( r * CR_R + g * CR_G + b * CR_B + 32768 ) >> 16 ) );
			    }
			rv = 0;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT32:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((int*)ch[ 0 ])[ p ] = red( pixel );
				if( ch[ 1 ] ) ((int*)ch[ 1 ])[ p ] = green( pixel );
				if( ch[ 2 ] ) ((int*)ch[ 2 ])[ p ] = blue( pixel );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				int r = red( pixel );
				int g = green( pixel );
				int b = blue( pixel );
				if( ch[ 0 ] ) ((int*)ch[ 0 ])[ p ] = ( r * YR + g * YG + b * YB + 32768 ) >> 16;
				if( ch[ 1 ] ) ((int*)ch[ 1 ])[ p ] = clamp( 128 + ( ( r * CB_R + g * CB_G + b * CB_B + 32768 ) >> 16 ) );
				if( ch[ 2 ] ) ((int*)ch[ 2 ])[ p ] = clamp( 128 + ( ( r * CR_R + g * CR_G + b * CR_B + 32768 ) >> 16 ) );
			    }
			rv = 0;
		    }
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((int64_t*)ch[ 0 ])[ p ] = red( pixel );
				if( ch[ 1 ] ) ((int64_t*)ch[ 1 ])[ p ] = green( pixel );
				if( ch[ 2 ] ) ((int64_t*)ch[ 2 ])[ p ] = blue( pixel );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				int r = red( pixel );
				int g = green( pixel );
				int b = blue( pixel );
				if( ch[ 0 ] ) ((int64_t*)ch[ 0 ])[ p ] = ( r * YR + g * YG + b * YB + 32768 ) >> 16;
				if( ch[ 1 ] ) ((int64_t*)ch[ 1 ])[ p ] = clamp( 128 + ( ( r * CB_R + g * CB_G + b * CB_B + 32768 ) >> 16 ) );
				if( ch[ 2 ] ) ((int64_t*)ch[ 2 ])[ p ] = clamp( 128 + ( ( r * CR_R + g * CR_G + b * CR_B + 32768 ) >> 16 ) );
			    }
			rv = 0;
		    }
		    break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((float*)ch[ 0 ])[ p ] = (float)red( pixel ) / 255.0F;
				if( ch[ 1 ] ) ((float*)ch[ 1 ])[ p ] = (float)green( pixel ) / 255.0F;
				if( ch[ 2 ] ) ((float*)ch[ 2 ])[ p ] = (float)blue( pixel ) / 255.0F;
			    }
			else
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				float r = red( pixel ) / 255.0F;
				float g = green( pixel ) / 255.0F;
				float b = blue( pixel ) / 255.0F;
				if( ch[ 0 ] ) ((float*)ch[ 0 ])[ p ] = 0.299 * r + 0.587 * g + 0.114 * b;
				if( ch[ 1 ] ) ((float*)ch[ 1 ])[ p ] = - 0.168736 * r - 0.331264 * g + 0.5 * b;
				if( ch[ 2 ] ) ((float*)ch[ 2 ])[ p ] = 0.5 * r - 0.418688 * g - 0.081312 * b;
                    	    }
			rv = 0;
		    }
		    break;
#ifdef PIX_FLOAT32_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				if( ch[ 0 ] ) ((double*)ch[ 0 ])[ p ] = (float)red( pixel ) / 255.0F;
				if( ch[ 1 ] ) ((double*)ch[ 1 ])[ p ] = (float)green( pixel ) / 255.0F;
				if( ch[ 2 ] ) ((double*)ch[ 2 ])[ p ] = (float)blue( pixel ) / 255.0F;
			    }
			else
			    for( size_t p = 0; p < size; p++ )
                    	    {
                    		COLOR pixel = image_ptr[ p ];
				float r = red( pixel ) / 255.0F;
				float g = green( pixel ) / 255.0F;
				float b = blue( pixel ) / 255.0F;
				if( ch[ 0 ] ) ((double*)ch[ 0 ])[ p ] = 0.299 * r + 0.587 * g + 0.114 * b;
				if( ch[ 1 ] ) ((double*)ch[ 1 ])[ p ] = - 0.168736 * r - 0.331264 * g + 0.5 * b;
				if( ch[ 2 ] ) ((double*)ch[ 2 ])[ p ] = 0.5 * r - 0.418688 * g - 0.081312 * b;
                    	    }
			rv = 0;
		    }
		    break;
#endif
	    }
	}
	else
	{
	    //From RGB / YCbCr to image:
	    switch( channel_type )
	    {
		case PIX_CONTAINER_TYPE_INT8:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = ((uint8_t*)ch[ 0 ])[ p ]; else r = 0;
				if( ch[ 1 ] ) g = ((uint8_t*)ch[ 1 ])[ p ]; else g = 0;
				if( ch[ 2 ] ) b = ((uint8_t*)ch[ 2 ])[ p ]; else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				int Y, Cb, Cr;
				int r, g, b;
				if( ch[ 0 ] ) Y = ((uint8_t*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((uint8_t*)ch[ 1 ])[ p ] - 128; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((uint8_t*)ch[ 2 ])[ p ] - 128; else Cr = 0;
				r = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.40200 ) * Cr ) );
    				g = clamp( Y - SPLIT_AMUL( SPLIT_CONV( 0.34414 ) * Cb ) - SPLIT_AMUL( SPLIT_CONV( 0.71414 ) * Cr ) );
    				b = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.77200 ) * Cb ) );
				image_ptr[ p ] = get_color( r, g, b );
			    }
			rv = 0;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT16:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = clamp( ((int16_t*)ch[ 0 ])[ p ] ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((int16_t*)ch[ 1 ])[ p ] ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((int16_t*)ch[ 2 ])[ p ] ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				int Y, Cb, Cr;
				int r, g, b;
				if( ch[ 0 ] ) Y = ((int16_t*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((int16_t*)ch[ 1 ])[ p ] - 128; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((int16_t*)ch[ 2 ])[ p ] - 128; else Cr = 0;
				r = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.40200 ) * Cr ) );
    				g = clamp( Y - SPLIT_AMUL( SPLIT_CONV( 0.34414 ) * Cb ) - SPLIT_AMUL( SPLIT_CONV( 0.71414 ) * Cr ) );
    				b = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.77200 ) * Cb ) );
				image_ptr[ p ] = get_color( r, g, b );
			    }
			rv = 0;
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT32:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = clamp( ((int*)ch[ 0 ])[ p ] ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((int*)ch[ 1 ])[ p ] ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((int*)ch[ 2 ])[ p ] ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				int Y, Cb, Cr;
				int r, g, b;
				if( ch[ 0 ] ) Y = ((int*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((int*)ch[ 1 ])[ p ] - 128; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((int*)ch[ 2 ])[ p ] - 128; else Cr = 0;
				r = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.40200 ) * Cr ) );
    				g = clamp( Y - SPLIT_AMUL( SPLIT_CONV( 0.34414 ) * Cb ) - SPLIT_AMUL( SPLIT_CONV( 0.71414 ) * Cr ) );
    				b = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.77200 ) * Cb ) );
				image_ptr[ p ] = get_color( r, g, b );
			    }
			rv = 0;
		    }
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int64_t r, g, b;
				if( ch[ 0 ] ) r = clamp( ((int64_t*)ch[ 0 ])[ p ] ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((int64_t*)ch[ 1 ])[ p ] ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((int64_t*)ch[ 2 ])[ p ] ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				int Y, Cb, Cr;
				int r, g, b;
				if( ch[ 0 ] ) Y = ((int64_t*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((int64_t*)ch[ 1 ])[ p ] - 128; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((int64_t*)ch[ 2 ])[ p ] - 128; else Cr = 0;
				r = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.40200 ) * Cr ) );
    				g = clamp( Y - SPLIT_AMUL( SPLIT_CONV( 0.34414 ) * Cb ) - SPLIT_AMUL( SPLIT_CONV( 0.71414 ) * Cr ) );
    				b = clamp( Y + SPLIT_AMUL( SPLIT_CONV( 1.77200 ) * Cb ) );
				image_ptr[ p ] = get_color( r, g, b );
			    }
			rv = 0;
		    }
		    break;
#endif
		case PIX_CONTAINER_TYPE_FLOAT32:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = clamp( ((float*)ch[ 0 ])[ p ] * 255 ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((float*)ch[ 1 ])[ p ] * 255 ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((float*)ch[ 2 ])[ p ] * 255 ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				float Y, Cb, Cr;
				float r, g, b;
				if( ch[ 0 ] ) Y = ((float*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((float*)ch[ 1 ])[ p ]; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((float*)ch[ 2 ])[ p ]; else Cr = 0;
				r = clamp_f( Y + 1.40200 * Cr );
    				g = clamp_f( Y - 0.34414 * Cb - 0.71414 * Cr );
    				b = clamp_f( Y + 1.77200 * Cb );
				image_ptr[ p ] = get_color( r * 255, g * 255, b * 255 );
			    }
			rv = 0;
		    }
		    break;
#ifdef PIX_FLOAT64_ENABLED
		case PIX_CONTAINER_TYPE_FLOAT64:
		    {
			if( rgb )
			    for( size_t p = 0; p < size; p++ )
			    {
				int r, g, b;
				if( ch[ 0 ] ) r = clamp( ((double*)ch[ 0 ])[ p ] * 255 ); else r = 0;
				if( ch[ 1 ] ) g = clamp( ((double*)ch[ 1 ])[ p ] * 255 ); else g = 0;
				if( ch[ 2 ] ) b = clamp( ((double*)ch[ 2 ])[ p ] * 255 ); else b = 0;
				image_ptr[ p ] = get_color( r, g, b );
			    }
			else
			    for( size_t p = 0; p < size; p++ )
			    {
				float Y, Cb, Cr;
				float r, g, b;
				if( ch[ 0 ] ) Y = ((double*)ch[ 0 ])[ p ]; else Y = 0;
				if( ch[ 1 ] ) Cb = ((double*)ch[ 1 ])[ p ]; else Cb = 0;
				if( ch[ 2 ] ) Cr = ((double*)ch[ 2 ])[ p ]; else Cr = 0;
				r = clamp_f( Y + 1.40200 * Cr );
    				g = clamp_f( Y - 0.34414 * Cb - 0.71414 * Cr );
    				b = clamp_f( Y + 1.77200 * Cb );
				image_ptr[ p ] = get_color( r * 255, g * 255, b * 255 );
			    }
			rv = 0;
		    }
		    break;
#endif
	    }
	}
	
	break;
    }
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//
// OpenGL graphics
//

void fn_set_gl_callback( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
#ifdef OPENGL
    if( pars_num > 0 )
    {
	gl_lock( vm->wm );

	PIX_ADDR callback;
	pix_stack_val userdata;
	userdata.i = 0;
	userdata.t = 0;
	GET_VAL_FROM_STACK( callback, 0, PIX_ADDR );
	if( callback == -1 || IS_ADDRESS_CORRECT( callback ) )
	{
	    if( callback != -1 )
		callback &= PIX_INT_ADDRESS_MASK;
	    if( pars_num > 1 ) 
	    {
		userdata = stack[ PIX_CHECK_SP( sp + 1 ) ];
	    }
	    vm->gl_userdata = userdata;
	    vm->gl_callback = callback;
	    stack[ sp2 ].i = 0;
	    stack[ sp2 ].t = 0;
	}
	else
	{
	    stack[ sp2 ].i = -1;
	    stack[ sp2 ].t = 0;
	    PIX_VM_LOG( "set_gl_callback() error: wrong callback address %d\n", (int)callback );
	}

	gl_unlock( vm->wm );
    }
#else
    stack[ sp2 ].i = -1;
    stack[ sp2 ].t = 0;
    PIX_VM_LOG( "set_gl_callback() error: no OpenGL. Please use the special Pixilang version with OpenGL support.\n" );
#endif
}

void fn_remove_gl_data( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
#ifdef OPENGL
    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_remove_container_gl_data( cnum, vm );
    }
#endif
}

void fn_update_gl_data( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
#ifdef OPENGL
    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_update_gl_texture_data( cnum, vm );
    }
#endif
}

void fn_gl_draw_arrays( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;
    
    GLenum mode;
    GLint first;
    GLsizei count;
    int r, g, b, a;
    PIX_CID texture;
    PIX_CID vertex_array;
    PIX_CID color_array = -1;
    PIX_CID texcoord_array = -1;
    
    if( pars_num >= 9 )
    {
	GET_VAL_FROM_STACK( mode, 0, GLenum );
	GET_VAL_FROM_STACK( first, 1, GLint );
	GET_VAL_FROM_STACK( count, 2, GLsizei );
	GET_VAL_FROM_STACK( r, 3, int );
	GET_VAL_FROM_STACK( g, 4, int );
	GET_VAL_FROM_STACK( b, 5, int );
	GET_VAL_FROM_STACK( a, 6, int );
	GET_VAL_FROM_STACK( texture, 7, PIX_CID );
	GET_VAL_FROM_STACK( vertex_array, 8, PIX_CID );
	if( pars_num >= 10 ) GET_VAL_FROM_STACK( color_array, 9, PIX_CID );
	if( pars_num >= 11 ) GET_VAL_FROM_STACK( texcoord_array, 10, PIX_CID );
    }

    pix_vm_container* vertex_array_cont = pix_vm_get_container( vertex_array, vm );
    GLenum vertex_type;
    if( vertex_array_cont )
    {
	switch( vertex_array_cont->type )
	{
	    case PIX_CONTAINER_TYPE_INT8: vertex_type = GL_BYTE; break;
	    case PIX_CONTAINER_TYPE_INT16: vertex_type = GL_SHORT; break;
	    case PIX_CONTAINER_TYPE_FLOAT32: vertex_type = GL_FLOAT; break;
	    default:
		PIX_VM_LOG( "gl_draw_arrays() error: vertex_array can be INT8, INT16 or FLOAT32 only\n" );
		return;
		break;
	}
    }
    else return;

    pix_vm_container* color_array_cont = pix_vm_get_container( color_array, vm );
    GLenum color_type;
    bool color_normalize = false;
    if( color_array_cont )
    {
	switch( color_array_cont->type )
	{
	    case PIX_CONTAINER_TYPE_INT8: color_type = GL_UNSIGNED_BYTE; color_normalize = true; break;
	    case PIX_CONTAINER_TYPE_FLOAT32: color_type = GL_FLOAT; break;
	    default:
		PIX_VM_LOG( "gl_draw_arrays() error: color_array can be INT8 or FLOAT32 only\n" );
		color_array = -1;
		break;
	}
    }

    pix_vm_container_gl_data* texture_gl = 0;
    pix_vm_container* texcoord_array_cont = 0;
    if( texture >= 0 )
    {
	texture_gl = pix_vm_create_container_gl_data( texture, vm );
    }
    GLenum texcoord_type;
    bool texcoord_normalize = false;
    texcoord_array_cont = pix_vm_get_container( texcoord_array, vm );
    if( texcoord_array_cont )
    {
        switch( texcoord_array_cont->type )
        {
    	    case PIX_CONTAINER_TYPE_INT8: texcoord_type = GL_BYTE; texcoord_normalize = true; break;
	    case PIX_CONTAINER_TYPE_INT16: texcoord_type = GL_SHORT; texcoord_normalize = true; break;
	    case PIX_CONTAINER_TYPE_FLOAT32: texcoord_type = GL_FLOAT; break;
	    default:
		PIX_VM_LOG( "gl_draw_arrays() error: texcoord_array can be INT8, INT16 or FLOAT32 only\n" );
		texcoord_array_cont = 0;
		break;
	}
    }

    gl_program_struct* p;
    if( texture_gl )
    {
	if( texture_gl->texture_format == GL_ALPHA )
	{
	    if( color_array >= 0 )
    		p = vm->gl_prog_tex_alpha_gradient;
    	    else
    		p = vm->gl_prog_tex_alpha_solid;
    	}
        else
        {
	    if( color_array >= 0 )
		p = vm->gl_prog_tex_rgba_gradient;
	    else
		p = vm->gl_prog_tex_rgba_solid;
	}
    }
    else
    {
	if( color_array >= 0 )
    	    p = vm->gl_prog_gradient;
    	else
    	    p = vm->gl_prog_solid;
    }
    if( vm->gl_user_defined_prog ) p = vm->gl_user_defined_prog;
    if( vm->gl_current_prog != p )
    {
        pix_vm_gl_use_prog( p, vm );
        uint attr = 1 << GL_PROG_ATT_POSITION;
        if( texture_gl )
        {
    	    glActiveTexture( GL_TEXTURE0 );
    	    glUniform1i( p->uniforms[ GL_PROG_UNI_TEXTURE ], 0 );
    	}
	if( texcoord_array_cont )
	{
    	    attr |= 1 << GL_PROG_ATT_TEX_COORD;
	}
	if( color_array_cont )
	{
    	    attr |= 1 << GL_PROG_ATT_COLOR;
	}
	gl_enable_attributes( p, attr );
    }
    if( texture_gl )
    {
	GL_BIND_TEXTURE( vm->wm, texture_gl->texture_id );
    }
    if( texcoord_array_cont )
    {
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_TEX_COORD ], texcoord_array_cont->xsize, texcoord_type, texcoord_normalize, 0, texcoord_array_cont->data );
    }
    if( color_array_cont )
    {
	glVertexAttribPointer( p->attributes[ GL_PROG_ATT_COLOR ], color_array_cont->xsize, color_type, color_normalize, 0, color_array_cont->data );
    }
    else
    {
	if( r < 0 ) r = 0; else if( r > 255 ) r = 255;
	if( g < 0 ) g = 0; else if( g > 255 ) g = 255;
	if( b < 0 ) b = 0; else if( b > 255 ) b = 255;
	if( a < 0 ) a = 0; else if( a > 255 ) a = 255;
	GL_CHANGE_PROG_COLOR_RGBA( p, r, g, b, a );
    }
    glVertexAttribPointer( p->attributes[ GL_PROG_ATT_POSITION ], vertex_array_cont->xsize, vertex_type, false, 0, vertex_array_cont->data );
    glDrawArrays( mode, first, count );

#endif //OPENGL
}

void fn_gl_blend_func( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;

#ifdef OPENGL
    if( pars_num < 2 )
    {
	//Set defaults:
	gl_set_default_blend_func( vm->wm );
    }
    else
    {
	GLenum sfactor;
	GLenum dfactor;
	GLenum sfactor_alpha;
	GLenum dfactor_alpha;
	GET_VAL_FROM_STACK( sfactor, 0, GLenum );
	GET_VAL_FROM_STACK( dfactor, 1, GLenum );
	if( pars_num == 4 )
	{
	    GET_VAL_FROM_STACK( sfactor_alpha, 2, GLenum );
	    GET_VAL_FROM_STACK( dfactor_alpha, 3, GLenum );
	    glBlendFuncSeparate( sfactor, dfactor, sfactor_alpha, dfactor_alpha );
	}
	else
	{
	    glBlendFunc( sfactor, dfactor );
	}
    }
#endif
}

//Convert selected pixel container to the OpenGL framebuffer (with attached texture) and bind it.
//All rendering operations will be redirected to this framebuffer.
//To unbind - just call this function without parameters.
//Pixel size does not affect the framebuffer.
//The framebuffer is flipped along the Y-axis when shown with pixi(). (native OpenGL framebuffer coordinates are using)
void fn_gl_bind_framebuffer( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;

    if( pars_num < 1 )
    {
	//Set defaults:
	gl_bind_framebuffer( 0, vm->wm );

	//Get previous viewport and WM transformation:
	gl_set_default_viewport( vm->wm );
	smem_copy( vm->gl_wm_transform, vm->gl_wm_transform_prev, sizeof( vm->gl_wm_transform ) );
	pix_vm_gl_matrix_set( vm );
    }
    else
    {
	while( 1 )
	{
	    PIX_CID cnum;
	    int flags = 0;
	    int vx, vy, vw, vh = 0;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    if( pars_num >= 2 ) GET_VAL_FROM_STACK( flags, 1, int );
	    if( pars_num >= 6 )
	    {
		GET_VAL_FROM_STACK( vx, 2, int );
		GET_VAL_FROM_STACK( vy, 3, int );
    		GET_VAL_FROM_STACK( vh, 4, int );
		GET_VAL_FROM_STACK( vw, 5, int );
	    }

	    pix_vm_container* c = pix_vm_get_container( cnum, vm );
	    if( !c ) break;
	    c->flags |= PIX_CONTAINER_FLAG_GL_FRAMEBUFFER;

	    pix_vm_container_gl_data* gl = pix_vm_create_container_gl_data( cnum, vm );
    	    if( !gl ) break;

	    gl_bind_framebuffer( gl->framebuffer_id, vm->wm );

	    //Set new viewport and WM transformation:
	    smem_copy( vm->gl_wm_transform_prev, vm->gl_wm_transform, sizeof( vm->gl_wm_transform ) );
	    if( vh )
		glViewport( vx, vy, vw, vh );
	    else
		glViewport( 0, 0, c->xsize, c->ysize );
	    matrix_4x4_reset( vm->gl_wm_transform );
	    //matrix_4x4_ortho( -1, 1, -1, 1, -1, 1, vm->gl_wm_transform );
	    vm->gl_no_2d_line_shift = true;
	    if( ( flags & GL_BFB_IDENTITY_MATRIX ) == 0 )
	    {
		/*
		    //0.375 shift has been removed on 15 dec 2022 (see details in lib_sundog/wm/wm_opengl.hpp)
		    matrix_4x4_ortho( -(float)c->xsize / 2 + 0.375, (float)c->xsize / 2 + 0.375, (float)c->ysize / 2 + 0.375, -(float)c->ysize / 2 + 0.375, -GL_ORTHO_MAX_DEPTH, GL_ORTHO_MAX_DEPTH, vm->gl_wm_transform );
		*/
		int hxsize = c->xsize / 2;
		int hysize = c->ysize / 2;
		matrix_4x4_ortho( -hxsize, c->xsize - hxsize, c->ysize - hysize, -hysize, -GL_ORTHO_MAX_DEPTH, GL_ORTHO_MAX_DEPTH, vm->gl_wm_transform );
		vm->gl_no_2d_line_shift = true;
	    }
	    pix_vm_gl_matrix_set( vm );

    	    break;
    	}
    }
#endif
}

//Bind the container to the specified texture image unit:
void fn_gl_bind_texture( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;

    while( pars_num >= 2 )
    {
        PIX_CID cnum;
        int texture_unit;
        GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
        GET_VAL_FROM_STACK( texture_unit, 1, int );

	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( !c ) break;

	pix_vm_container_gl_data* gl = pix_vm_create_container_gl_data( cnum, vm );
    	if( !gl ) break;

	glActiveTexture( GL_TEXTURE0 + texture_unit );
        if( texture_unit == 0 )
        {
    	    GL_BIND_TEXTURE( vm->wm, gl->texture_id );
    	}
    	else
    	{
    	    glBindTexture( GL_TEXTURE_2D, gl->texture_id );
    	}
	glActiveTexture( GL_TEXTURE0 );

        break;
    }
#endif
}

void fn_gl_get_int( PIX_BUILTIN_FN_PARAMETERS )
{
    //Querying GL State:
#ifdef OPENGL
    FN_HEADER;

    int value = 0;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( value, 0, int );
    }

    //Default retval:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = 0;
    stack[ sp2 ].t = 0;

    if( vm->screen != PIX_GL_SCREEN ) return;
    
    int gl_state_v = 0;
    glGetIntegerv( value, &gl_state_v );
    stack[ sp2 ].i = gl_state_v;
#endif
}

void fn_gl_get_float( PIX_BUILTIN_FN_PARAMETERS )
{
    //Querying GL State:
#ifdef OPENGL
    FN_HEADER;

    int value = 0;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( value, 0, int );
    }

    //Default retval:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].f = 0;
    stack[ sp2 ].t = 1;

    if( vm->screen != PIX_GL_SCREEN ) return;
    
    float gl_state_v = 0;
    glGetFloatv( value, &gl_state_v );
    stack[ sp2 ].f = gl_state_v;
#endif
}

void fn_gl_new_prog( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;
    
    PIX_CID rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID vshader;
	PIX_CID fshader;
	GET_VAL_FROM_STACK( vshader, 0, PIX_CID );
	GET_VAL_FROM_STACK( fshader, 1, PIX_CID );

	bool vshader_m = false;	
	bool fshader_m = false;	
	char* vshader_str = 0;
	char* fshader_str = 0;
	char ts1[ 2 ] = { 0, 0 };
	char ts2[ 2 ] = { 0, 0 };
	if( vshader < 0 )
	{
	    vshader = -vshader - 1;
	    if( vshader < GL_SHADER_MAX )
	    {
		ts1[ 0 ] = '0' + vshader;
		vshader_str = ts1;
	    }
	}
	else
	{
	    vshader_str = pix_vm_make_cstring_from_container( vshader, &vshader_m, vm );
	}
	if( fshader < 0 )
	{
	    fshader = -fshader - 1;
	    if( fshader < GL_SHADER_MAX )
	    {
		ts2[ 0 ] = '0' + fshader;
		fshader_str = ts2;
	    }
	}
	else
	{
	    fshader_str = pix_vm_make_cstring_from_container( fshader, &fshader_m, vm );
	}
	if( vshader_str && fshader_str )
	{
	    size_t len1 = smem_strlen( vshader_str );
	    size_t len2 = smem_strlen( fshader_str );
	    char* res = (char*)smem_new( len1 + 1 + len2 + 1 );
	    smem_copy( res, vshader_str, len1 + 1 );
	    smem_copy( res + len1 + 1, fshader_str, len2 + 1 );
	    rv = pix_vm_new_container( -1, len1 + 1 + len2 + 1, 1, PIX_CONTAINER_TYPE_INT8, res, vm );
	    pix_vm_mix_container_flags( rv, PIX_CONTAINER_FLAG_GL_PROG, vm );
	}
	if( vshader_m ) smem_free( vshader_str );
	if( fshader_m ) smem_free( fshader_str );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
#endif
}

void fn_gl_use_prog( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;

    if( pars_num >= 1 )
    {
	//Use user-defined GLSL program:
	PIX_CID p;
	GET_VAL_FROM_STACK( p, 0, PIX_CID );
	pix_vm_container_gl_data* gl = pix_vm_create_container_gl_data( p, vm );
	if( gl && gl->prog )
	{
	    vm->gl_user_defined_prog = gl->prog;
	    glUseProgram( gl->prog->program );
	}
    }
    else
    {
	//Use default GLSL program:
	vm->gl_user_defined_prog = 0;
    }
#endif
}

void fn_gl_uniform( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;

    bool float_vals = false;
    if( pars_num >= 2 )
    {
	GLint uniform_loc = 0;
	GET_VAL_FROM_STACK( uniform_loc, 0, GLint );
	uniform_loc--;
	for( int i = 1; i < pars_num; i++ )
	{
	    if( stack[ PIX_CHECK_SP( sp + i ) ].t != 0 )
	    {
		float_vals = true;
		break;
	    }
	}
	if( float_vals )
	{
	    GLfloat v0, v1, v2, v3;
	    while( 1 )
	    {
		GET_VAL_FROM_STACK( v0, 1, GLfloat );
		if( pars_num == 2 )
		{
		    glUniform1f( uniform_loc, v0 );
		    break;
		}
		GET_VAL_FROM_STACK( v1, 2, GLfloat );
		if( pars_num == 3 )
		{
		    glUniform2f( uniform_loc, v0, v1 );
		    break;
		}
		GET_VAL_FROM_STACK( v2, 3, GLfloat );
		if( pars_num == 4 )
		{
		    glUniform3f( uniform_loc, v0, v1, v2 );
		    break;
		}
		GET_VAL_FROM_STACK( v3, 4, GLfloat );
		if( pars_num == 5 )
		{
		    glUniform4f( uniform_loc, v0, v1, v2, v3 );
		    break;
		}
		break;
	    }
	}
	else
	{
	    bool not_array = true;
	    if( pars_num == 5 )
	    {
		//Perhaps this is an array of vectors?
		//number of the first element in the container = first_vec * vec_size;
		//recommended container types: INT32 or FLOAT32;
		PIX_CID cnum;
		int vec_size;
		PIX_INT first_vec;
		PIX_INT count;
		GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
		GET_VAL_FROM_STACK( vec_size, 2, int );
		GET_VAL_FROM_STACK( first_vec, 3, PIX_INT );
		GET_VAL_FROM_STACK( count, 4, PIX_INT );
		while( 1 )
		{
		    pix_vm_container* c = pix_vm_get_container( cnum, vm );
		    if( c == 0 ) break;
		    int el_size = g_pix_container_type_sizes[ c->type ];
		    if( vec_size < 1 || vec_size > 4 ) break;
		    if( first_vec < 0 ) break;
		    if( count < 0 ) break;
		    if( (unsigned)( ( first_vec + count ) * vec_size ) > c->size ) break;
		    void* cdata = (uint8_t*)c->data + first_vec * vec_size * el_size;
		    void* temp_buf = 0;
		    if( c->type >= PIX_CONTAINER_TYPE_FLOAT32 )
		    {
			//Floating point:
			GLfloat* vv = 0;
			if( el_size == sizeof( GLfloat ) )
			{
			    vv = (GLfloat*)cdata;
			}
			else
			{
			    temp_buf = smem_new( sizeof( GLfloat ) * vec_size * count );
			    if( temp_buf )
			    {
				vv = (GLfloat*)temp_buf;
				for( PIX_INT i = 0; i < count * vec_size; i++ )
				    vv[ i ] = pix_vm_get_container_float_element( cnum, i + first_vec * vec_size, vm );
			    }
			}
			switch( vec_size )
			{
			    case 1: glUniform1fv( uniform_loc, count, vv ); break;
			    case 2: glUniform2fv( uniform_loc, count, vv ); break;
			    case 3: glUniform3fv( uniform_loc, count, vv ); break;
			    case 4: glUniform4fv( uniform_loc, count, vv ); break;
			    default: break;
			}
		    }
		    else
		    {
			//Integer:
			GLint* ii = 0;
			if( el_size == sizeof( GLint ) )
			{
			    ii = (GLint*)cdata;
			}
			else
			{
			    temp_buf = smem_new( sizeof( GLint ) * vec_size * count );
			    if( temp_buf )
			    {
				ii = (GLint*)temp_buf;
				for( PIX_INT i = 0; i < count * vec_size; i++ )
				    ii[ i ] = pix_vm_get_container_int_element( cnum, i + first_vec * vec_size, vm );
			    }
			}
			switch( vec_size )
			{
			    case 1: glUniform1iv( uniform_loc, count, ii ); break;
			    case 2: glUniform2iv( uniform_loc, count, ii ); break;
			    case 3: glUniform3iv( uniform_loc, count, ii ); break;
			    case 4: glUniform4iv( uniform_loc, count, ii ); break;
			    default: break;
			}
		    }
		    if( temp_buf ) smem_free( temp_buf );
		    not_array = false;
		    break;
		}
	    }
	    if( not_array )
	    {
		GLint v0, v1, v2, v3;
		while( 1 )
		{
		    GET_VAL_FROM_STACK( v0, 1, GLint );
		    if( pars_num == 2 )
		    {
			glUniform1i( uniform_loc, v0 );
			break;
		    }
		    GET_VAL_FROM_STACK( v1, 2, GLint );
		    if( pars_num == 3 )
		    {
			glUniform2i( uniform_loc, v0, v1 );
			break;
		    }
		    GET_VAL_FROM_STACK( v2, 3, GLint );
		    if( pars_num == 4 )
		    {
			glUniform3i( uniform_loc, v0, v1, v2 );
			break;
	    	    }
		    GET_VAL_FROM_STACK( v3, 4, GLint );
		    if( pars_num == 5 )
		    {
			glUniform4i( uniform_loc, v0, v1, v2, v3 );
			break;
		    }
		    break;
		}
	    }
	}
    }
#endif
}

void fn_gl_uniform_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
#ifdef OPENGL
    FN_HEADER;

    if( vm->screen != PIX_GL_SCREEN ) return;

    if( pars_num >= 3 )
    {
	int size;
	GLint uniform_loc = 0;
	GLboolean transpose;
	PIX_CID cnum;
	GET_VAL_FROM_STACK( size, 0, int );
	GET_VAL_FROM_STACK( uniform_loc, 1, GLint );
	uniform_loc--;
	GET_VAL_FROM_STACK( transpose, 2, GLboolean );
	GET_VAL_FROM_STACK( cnum, 3, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && size > 1 && size <= 4 )
	{
	    GLfloat temp[ 4 * 4 ];
	    GLfloat* p = 0;
	    if( c->type == PIX_CONTAINER_TYPE_FLOAT32 && sizeof( float ) == sizeof( GLfloat ) )
	    {
		p = (GLfloat*)c->data;
	    }
	    else
	    {
		p = temp;
		for( size_t i = 0; i < size * size; i++ )
		{
		    p[ i ] = pix_vm_get_container_float_element( cnum, i, vm );
		}
	    }
	    switch( size )
	    {
		case 2: glUniformMatrix2fv( uniform_loc, 1, transpose, p ); break;
		case 3: glUniformMatrix3fv( uniform_loc, 1, transpose, p ); break;
		case 4: glUniformMatrix4fv( uniform_loc, 1, transpose, p ); break;
	    }
	}
    }
#endif
}

//
// Animation
//

void fn_pixi_unpack_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    if( pars_num >= 1 ) 
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	rv = pix_vm_container_hdata_unpack_frame( cnum, vm );
    }
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_pixi_pack_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    if( pars_num >= 1 ) 
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	rv = pix_vm_container_hdata_pack_frame( cnum, vm );
    }
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_pixi_create_anim( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    while( 1 )
    {
	if( pars_num >= 1 ) 
	{
	    PIX_CID cnum;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    pix_vm_container* c = pix_vm_get_container( cnum, vm );
	    if( c == 0 ) break;
	    uint8_t* hdata_ptr = (uint8_t*)pix_vm_get_container_hdata( cnum, vm );
	    if( hdata_ptr )
	    {
		if( hdata_ptr[ 0 ] != pix_vm_container_hdata_type_anim )
		{
		    pix_vm_remove_container_hdata( cnum, vm );
		    hdata_ptr = 0;
		}
	    }
	    if( hdata_ptr == 0 )
	    {
		pix_vm_create_container_hdata( cnum, pix_vm_container_hdata_type_anim, sizeof( pix_vm_container_hdata_anim ), vm );
		hdata_ptr = (uint8_t*)pix_vm_get_container_hdata( cnum, vm );
		if( hdata_ptr )
		{
		    pix_vm_container_hdata_anim* hdata = (pix_vm_container_hdata_anim*)hdata_ptr;
		    hdata->frame_count = 1;
		    PIX_VAL prop_val;
		    prop_val.i = 0; pix_vm_set_container_property( cnum, "frame", -1, 0, prop_val, vm );
		    prop_val.i = 10; pix_vm_set_container_property( cnum, "fps", -1, 0, prop_val, vm );
		    prop_val.i = 1; pix_vm_set_container_property( cnum, "frames", -1, 0, prop_val, vm );
		    prop_val.i = -1; pix_vm_set_container_property( cnum, "repeat", -1, 0, prop_val, vm );
		    hdata->frames = (pix_vm_anim_frame*)smem_new( sizeof( pix_vm_anim_frame ) );
		    smem_zero( hdata->frames );
		    if( hdata->frames )
		    {
			rv = pix_vm_container_hdata_pack_frame_from_buf( cnum, 0, (COLORPTR)c->data, c->type, c->xsize, c->ysize, vm );
		    }
		}
	    }
	}
	break;
    }
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_pixi_remove_anim( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    while( 1 )
    {
	if( pars_num >= 1 ) 
	{
	    PIX_CID cnum;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    pix_vm_container* c = pix_vm_get_container( cnum, vm );
	    if( c == 0 ) break;
	    pix_vm_remove_container_hdata( cnum, vm );
	    rv = 0;
	}
	break;
    }
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_pixi_clone_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    while( 1 )
    {
	if( pars_num >= 1 ) 
	{
	    PIX_CID cnum;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    rv = pix_vm_container_hdata_clone_frame( cnum, vm );
	}
	break;
    }
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_pixi_remove_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    while( 1 )
    {
	if( pars_num >= 1 ) 
	{
	    PIX_CID cnum;
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    rv = pix_vm_container_hdata_remove_frame( cnum, vm );
	}
	break;
    }
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_pixi_play( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 ) 
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && c->opt_data && c->opt_data->hdata )
	{
	    PIX_VAL prop_val;
	    prop_val.i = 1; pix_vm_set_container_property( cnum, "play", -1, 0, prop_val, vm );
	    prop_val.i = (uint)stime_ticks(); pix_vm_set_container_property( cnum, "start_time", -1, 0, prop_val, vm );
	    prop_val.i = pix_vm_get_container_property_i( cnum, "frame", -1, vm ); pix_vm_set_container_property( cnum, "start_frame", -1, 0, prop_val, vm );
	    //Unpack first frame:
	    pix_vm_container_hdata_unpack_frame( cnum, vm );
	}
    }
}

void fn_pixi_stop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 ) 
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c && c->opt_data && c->opt_data->hdata )
	{
	    PIX_VAL prop_val;
	    prop_val.i = 0; pix_vm_set_container_property( cnum, "play", -1, 0, prop_val, vm );
	}
    }
}

//
// Video
//

struct pix_video_struct
{
    svideo_struct vid;
    PIX_CID svideo_container;
    pix_vm* vm;
    PIX_ADDR capture_callback;
    pix_stack_val capture_user_data;
    bool capture_callback_working;
};

void pix_video_capture_callback( svideo_struct* vid )
{
    pix_video_struct* pix_vid = (pix_video_struct*)vid->capture_user_data;
    if( pix_vid->capture_callback != -1 )
    {
	pix_vm_function fun;
        pix_stack_val pp[ 2 ];
        fun.p = pp;
        fun.addr = pix_vid->capture_callback;
        fun.p[ 0 ].i = pix_vid->svideo_container;
        fun.p[ 0 ].t = 0;
        fun.p[ 1 ] = pix_vid->capture_user_data;
        fun.p_num = 2;
        pix_vid->capture_callback_working = 1;
        pix_vm_run( PIX_VM_THREADS - 3, 0, &fun, PIX_VM_CALL_FUNCTION, pix_vid->vm );
        pix_vid->capture_callback_working = 0;
    }
}

void fn_video_open( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;

    if( pars_num >= 2 ) 
    {
	PIX_CID name_cnum;
	uint flags;
	PIX_ADDR capture_callback = -1;
	pix_stack_val capture_user_data;
	capture_user_data.t = 0;
	capture_user_data.i = 0;

	GET_VAL_FROM_STACK( name_cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( flags, 1, int );

	bool need_to_free = 0;
        char* name = pix_vm_make_cstring_from_container( name_cnum, &need_to_free, vm );

	if( pars_num >= 3 ) GET_VAL_FROM_STACK( capture_callback, 2, PIX_ADDR );
	if( pars_num >= 4 ) capture_user_data = stack[ PIX_CHECK_SP( sp + 3 ) ];

        rv = pix_vm_new_container( -1, sizeof( pix_video_struct ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
        if( rv >= 0 )
        {
    	    pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( rv, vm );
    	    smem_clear( pix_vid, sizeof( pix_video_struct ) );
    	    svideo_struct* vid = &pix_vid->vid;
    	    pix_vid->svideo_container = rv;
    	    pix_vid->vm = vm;
    	    pix_vid->capture_callback = -1;
    	    if( IS_ADDRESS_CORRECT( capture_callback ) )
    	    {
    		pix_vid->capture_callback = capture_callback & PIX_INT_ADDRESS_MASK;
    	    }
    	    pix_vid->capture_user_data = capture_user_data;
    	    if( svideo_open( vid, vm->wm->sd, (const char*)name, flags, pix_video_capture_callback, pix_vid ) != 0 )
    	    {
    		pix_vm_remove_container( rv, vm );
    		rv = -1;
    	    }
        }

        if( need_to_free ) smem_free( name );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_video_close( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( cnum, vm );
	if( pix_vid )
	{
    	    svideo_struct* vid = &pix_vid->vid;
    	    rv = svideo_close( vid );
    	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_video_start( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( cnum, vm );
	if( pix_vid )
	{
    	    svideo_struct* vid = &pix_vid->vid;
    	    rv = svideo_start( vid );
    	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_video_stop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 1 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( cnum, vm );
	if( pix_vid )
	{
    	    svideo_struct* vid = &pix_vid->vid;
    	    rv = svideo_stop( vid );
    	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_video_props( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID vid_cnum;
	PIX_CID props_cnum;
	GET_VAL_FROM_STACK( vid_cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( props_cnum, 1, PIX_CID );
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( vid_cnum, vm );
	if( pix_vid )
	{
    	    svideo_struct* vid = &pix_vid->vid;
    	    pix_vm_container* props_cont = pix_vm_get_container( props_cnum, vm );
    	    if( props_cont && props_cont->size >= 2 )
    	    {
    		svideo_prop* props = (svideo_prop*)smem_new( sizeof( svideo_prop ) * ( props_cont->size / 2 + 1 ) );
    		smem_zero( props );
    		if( props )
    		{
		    for( int i = 0; i < props_cont->size; i += 2 )
		    {
			PIX_INT prop_id = pix_vm_get_container_int_element( props_cnum, i + 0, vm );
			PIX_INT prop_val = pix_vm_get_container_int_element( props_cnum, i + 1, vm );
			props[ i / 2 ].id = (int)prop_id;
			if( fn_num == FN_VIDEO_SET_PROPS )
			    props[ i / 2 ].val.i = prop_val;
		    }
		    if( fn_num == FN_VIDEO_SET_PROPS )
		    {
			//Set:
    			rv = svideo_set_props( vid, props );
    		    }
    		    else
    		    {
    			//Get:
    			rv = svideo_get_props( vid, props );
    			if( rv == 0 )
    			{
    		    	    for( int i = 0; i < props_cont->size; i += 2 )
			    {
				if( props[ i / 2 ].id == SVIDEO_PROP_ORIENTATION_I )
				{
				    props[ i / 2 ].val.i = ( vid->orientation - vm->wm->screen_angle ) & 3;
				}
				pix_vm_set_container_int_element( props_cnum, i + 1, props[ i / 2 ].val.i, vm );
			    }
			}
    		    }
		    smem_free( props );
		}
	    }
    	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_video_capture_frame( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	PIX_CID dest_cnum;
	int pixel_format = 0; //0 - normal; 1 - grayscale8
	uint flags = 0;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( dest_cnum, 1, PIX_CID );
	while( 1 )
	{
	    if( pars_num >= 3 ) { GET_VAL_FROM_STACK( pixel_format, 2, PIX_CID ); } else break;
	    if( pars_num >= 4 ) { GET_VAL_FROM_STACK( flags, 3, PIX_CID ); } else break;
	    break;
	}
	
	pix_video_struct* pix_vid = (pix_video_struct*)pix_vm_get_container_data( cnum, vm );
	while( pix_vid )
	{
    	    svideo_struct* vid = &pix_vid->vid;
    	    if( pix_vid->capture_callback_working == 0 )
    	    {
    		PIX_VM_LOG( "video_capture_frame() can't be called outsize of the capture callback\n" );
    		break;
    	    }
    	    int frame_xsize = vid->frame_width;
    	    int frame_ysize = vid->frame_height;
    	    int dest_pixel_format;
    	    int dest_pixel_size;
    	    int type;
    	    switch( pixel_format )
    	    {
    		case 1: dest_pixel_format = SVIDEO_PIXEL_FORMAT_GRAYSCALE8; dest_pixel_size = 1; type = PIX_CONTAINER_TYPE_INT8; break;
    		default: dest_pixel_format = SVIDEO_PIXEL_FORMAT_COLOR; dest_pixel_size = COLORLEN; type = 32; break;
    	    }
    	    pix_vm_container* dest_cont = pix_vm_get_container( dest_cnum, vm );
    	    if( dest_cont == 0 ) break;
    	    size_t frame_size = frame_xsize * frame_ysize;
    	    if( frame_size * dest_pixel_size > dest_cont->size * g_pix_container_type_sizes[ dest_cont->type ] )
    	    {
    		const char* type_str = 0;
    		if( type == 32 )
    		    type_str = "PIXEL";
    		else
    		    type_str = g_pix_container_type_names[ type ];
    		PIX_VM_LOG( "video_capture_frame(): wrong container size; expected: %dx%d %s\n", frame_xsize, frame_ysize, type_str );
    		break;
    	    }
    	    int orient = vid->orientation;
    	    if( orient == vm->wm->screen_angle || ( flags & 1 ) ) //no autorotate
    	    {
    		svideo_pixel_convert( vid->capture_plans, vid->capture_plans_cnt, vid->pixel_format, dest_cont->data, dest_pixel_format, frame_xsize, frame_ysize );
    	    }
    	    else
    	    {
    		//Autorotate:
    		//MUST BE REMOVED IN FUTURE UPDATES!
    		//USE NOAUTOROTATE+VIDEO_PROP_ORIENTATION!
    		PIX_INT xsize = frame_xsize;
    		PIX_INT ysize = frame_ysize;
    		int rotate = ( vm->wm->screen_angle - orient ) & 3;
    		void* temp_buf = smem_new( frame_size * dest_pixel_size );
    		if( temp_buf )
    		{
    		    svideo_pixel_convert( vid->capture_plans, vid->capture_plans_cnt, vid->pixel_format, temp_buf, dest_pixel_format, frame_xsize, frame_ysize );
    		    if( rotate == 2 || xsize == ysize )
    		    {
    			pix_vm_rotate_block( &temp_buf, &xsize, &ysize, dest_cont->type, rotate, dest_cont->data );
    		    }
    		    else
    		    {
    			PIX_INT dest_p;
    			PIX_INT dest_add_x;
    			PIX_INT dest_add_y;
    			PIX_INT src_p;
    			PIX_INT src_add_x;
    			PIX_INT src_add_y;
    			PIX_INT smallest_size;
    			if( xsize > ysize )
    			{
    			    dest_p = ( xsize - ysize ) / 2;
    			    dest_add_x = 1;
    			    dest_add_y = xsize - ysize;
    			    src_p = xsize * ( ysize - 1 ) + ( xsize - ysize ) / 2; 
    			    src_add_x = -xsize;
    			    src_add_y = xsize * ysize + 1;
    			    smallest_size = ysize;
    			}
    			else
    			{
    			    dest_p = xsize * ( ( ysize - xsize ) / 2 );
    			    dest_add_x = 1;
    			    dest_add_y = 0;
    			    src_p = xsize * ( ( ysize + xsize ) / 2 );
    			    src_add_x = -xsize;
    			    src_add_y = xsize * xsize + 1;
    			    smallest_size = xsize;
    			}
    			if( rotate == 3 )
    			{
    			    dest_p = xsize * ysize - 1 - dest_p;
    			    dest_add_x = -dest_add_x;
    			    dest_add_y = -dest_add_y;
    			}
    			dest_p *= dest_pixel_size;
    			dest_add_x *= dest_pixel_size;
    			dest_add_y *= dest_pixel_size;
    			src_p *= dest_pixel_size;
    			src_add_x *= dest_pixel_size;
    			src_add_y *= dest_pixel_size;
    			for( PIX_INT y = 0; y < smallest_size; y++ )
    			{
    			    for( PIX_INT x = 0; x < smallest_size; x++ )
    			    {
    				for( int b = 0; b < dest_pixel_size; b++ )
    				{
    				    ((uint8_t*)dest_cont->data)[ dest_p + b ] = ((uint8_t*)temp_buf)[ src_p + b ];
    				}
    				dest_p += dest_add_x;
    				src_p += src_add_x;
    			    }
    			    dest_p += dest_add_y;
    			    src_p += src_add_y;
    			}
		    }
        	    smem_free( temp_buf );
        	}
    	    }
	    rv = 0;
    	    break;
    	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

//
// Transformation
//

void fn_t_reset( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    pix_vm_gfx_matrix_reset( vm );
    
#ifdef OPENGL
    pix_vm_gl_matrix_set( vm );
#endif
}

void fn_t_rotate( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 4 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT angle, x, y, z;
	GET_VAL_FROM_STACK( angle, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( x, 1, PIX_FLOAT );
	GET_VAL_FROM_STACK( y, 2, PIX_FLOAT );
	GET_VAL_FROM_STACK( z, 3, PIX_FLOAT );
	angle /= 180;
	angle *= M_PI;
	
	//Normalize vector:
	PIX_FLOAT inv_length = 1.0f / sqrt( x * x + y * y + z * z );
	x *= inv_length;
	y *= inv_length;
	z *= inv_length;
	
	//Create the matrix:
	PIX_FLOAT c = cos( angle );
	PIX_FLOAT s = sin( angle );
	PIX_FLOAT cc = 1 - c;
	PIX_FLOAT r[ 4 * 4 ];
	PIX_FLOAT res[ 4 * 4 ];
	r[ 0 + 0 ] = x * x * cc + c;
	r[ 4 + 0 ] = x * y * cc - z * s;
	r[ 8 + 0 ] = x * z * cc + y * s;
	r[ 12 + 0 ] = 0;
	r[ 0 + 1 ] = y * x * cc + z * s;
	r[ 4 + 1 ] = y * y * cc + c;
	r[ 8 + 1 ] = y * z * cc - x * s;
	r[ 12 + 1 ] = 0;
	r[ 0 + 2 ] = x * z * cc - y * s;
	r[ 4 + 2 ] = y * z * cc + x * s;
	r[ 8 + 2 ] = z * z * cc + c;
	r[ 12 + 2 ] = 0;
	r[ 0 + 3 ] = 0;
	r[ 4 + 3 ] = 0;
	r[ 8 + 3 ] = 0;
	r[ 12 + 3 ] = 1;
	pix_vm_gfx_matrix_mul( res, m, r );
	smem_copy( m, res, sizeof( PIX_FLOAT ) * 4 * 4 );

	vm->t_enabled = 1;

#ifdef OPENGL
	pix_vm_gl_matrix_set( vm );
#endif
    }
}

void fn_t_translate( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT x, y, z;
	GET_VAL_FROM_STACK( x, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( y, 1, PIX_FLOAT );
	GET_VAL_FROM_STACK( z, 2, PIX_FLOAT );
	
	PIX_FLOAT m2[ 4 * 4 ];
	PIX_FLOAT res[ 4 * 4 ];
	smem_clear( m2, sizeof( PIX_FLOAT ) * 4 * 4 );
	m2[ 0 ] = 1;
	m2[ 4 + 1 ] = 1;
	m2[ 8 + 2 ] = 1;
	m2[ 12 + 0 ] = x;
	m2[ 12 + 1 ] = y;
	m2[ 12 + 2 ] = z;
	m2[ 12 + 3 ] = 1;

	pix_vm_gfx_matrix_mul( res, m, m2 );
	smem_copy( m, res, sizeof( PIX_FLOAT ) * 4 * 4 );
	
	vm->t_enabled = 1;

#ifdef OPENGL
	pix_vm_gl_matrix_set( vm );
#endif
    }
}

void fn_t_scale( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 3 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	PIX_FLOAT x, y, z;
	GET_VAL_FROM_STACK( x, 0, PIX_FLOAT );
	GET_VAL_FROM_STACK( y, 1, PIX_FLOAT );
	GET_VAL_FROM_STACK( z, 2, PIX_FLOAT );

	PIX_FLOAT m2[ 4 * 4 ];
	PIX_FLOAT res[ 4 * 4 ];
	smem_clear( m2, sizeof( PIX_FLOAT ) * 4 * 4 );
	m2[ 0 ] = x;
	m2[ 4 + 1 ] = y;
	m2[ 8 + 2 ] = z;
	m2[ 12 + 3 ] = 1;

	pix_vm_gfx_matrix_mul( res, m, m2 );
	smem_copy( m, res, sizeof( PIX_FLOAT ) * 4 * 4 );

	vm->t_enabled = 1;

#ifdef OPENGL
        pix_vm_gl_matrix_set( vm );
#endif
    }
}

void fn_t_push_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( vm->t_matrix_sp >= PIX_T_MATRIX_STACK_SIZE - 1 )
    {
	PIX_VM_LOG( "t_push_matrix(): stack overflow\n" );
    }
    else
    {
	smem_copy( vm->t_matrix + ( vm->t_matrix_sp + 1 ) * 16, vm->t_matrix + vm->t_matrix_sp * 16, 4 * 4 * sizeof( PIX_FLOAT ) );
	vm->t_matrix_sp++;
    }
}

void fn_t_pop_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( vm->t_matrix_sp == 0 )
    {
	PIX_VM_LOG( "t_pop_matrix(): nothing to pop up froms stack\n" );
    }
    else
    {
	vm->t_matrix_sp--;
#ifdef OPENGL
	pix_vm_gl_matrix_set( vm );
#endif
    }
}

void fn_t_get_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	
	PIX_CID m2;
	GET_VAL_FROM_STACK( m2, 0, PIX_CID );
	
	if( (unsigned)m2 < (unsigned)vm->c_num && vm->c[ m2 ] )
	{
	    pix_vm_container* c = vm->c[ m2 ];
	    if( sizeof( PIX_FLOAT ) == 32 && c->type != PIX_CONTAINER_TYPE_FLOAT32 ) return;
	    if( sizeof( PIX_FLOAT ) == 64 && c->type != PIX_CONTAINER_TYPE_FLOAT64 ) return;
	    if( c->size < 4 * 4 ) return;
	    smem_copy( c->data, m, sizeof( PIX_FLOAT ) * 4 * 4 );
	}
    }
}

void fn_t_set_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	
	PIX_CID m2;
	GET_VAL_FROM_STACK( m2, 0, PIX_CID );
	
	if( (unsigned)m2 < (unsigned)vm->c_num && vm->c[ m2 ] )
	{
	    pix_vm_container* c = vm->c[ m2 ];
	    if( sizeof( PIX_FLOAT ) == 32 && c->type != PIX_CONTAINER_TYPE_FLOAT32 ) return;
	    if( sizeof( PIX_FLOAT ) == 64 && c->type != PIX_CONTAINER_TYPE_FLOAT64 ) return;
	    if( c->size < 4 * 4 ) return;
	    smem_copy( m, c->data, sizeof( PIX_FLOAT ) * 4 * 4 );
	    
	    vm->t_enabled = 1;

#ifdef OPENGL
	    pix_vm_gl_matrix_set( vm );
#endif
	}
    }
}

void fn_t_mul_matrix( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	
	PIX_CID m2;
	GET_VAL_FROM_STACK( m2, 0, PIX_CID );
	
	if( (unsigned)m2 < (unsigned)vm->c_num && vm->c[ m2 ] )
	{
	    pix_vm_container* c = vm->c[ m2 ];
	    if( sizeof( PIX_FLOAT ) == 32 && c->type != PIX_CONTAINER_TYPE_FLOAT32 ) return;
	    if( sizeof( PIX_FLOAT ) == 64 && c->type != PIX_CONTAINER_TYPE_FLOAT64 ) return;
	    if( c->size < 4 * 4 ) return;
	    PIX_FLOAT res_m[ 4 * 4 ];
	    pix_vm_gfx_matrix_mul( res_m, m, (PIX_FLOAT*)c->data );
	    smem_copy( m, res_m, sizeof( PIX_FLOAT ) * 4 * 4 );
	    
	    vm->t_enabled = 1;

#ifdef OPENGL
	    pix_vm_gl_matrix_set( vm );
#endif
	}
    }
}

void fn_t_point( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	PIX_FLOAT* m = vm->t_matrix + ( vm->t_matrix_sp * 16 );
	
	PIX_CID p;
	GET_VAL_FROM_STACK( p, 0, PIX_CID );
	
	if( (unsigned)p < (unsigned)vm->c_num && vm->c[ p ] )
	{
	    pix_vm_container* c = vm->c[ p ];
	    if( sizeof( PIX_FLOAT ) == 32 && c->type != PIX_CONTAINER_TYPE_FLOAT32 ) return;
	    if( sizeof( PIX_FLOAT ) == 64 && c->type != PIX_CONTAINER_TYPE_FLOAT64 ) return;
	    if( c->size < 3 ) return;
	    while( pars_num >= 2 )
	    {
		PIX_CID mc;
		GET_VAL_FROM_STACK( mc, 1, PIX_CID );
		if( (unsigned)mc < (unsigned)vm->c_num && vm->c[ mc ] )
		{
		    pix_vm_container* c2 = vm->c[ mc ];
		    if( sizeof( PIX_FLOAT ) == 32 && c2->type != PIX_CONTAINER_TYPE_FLOAT32 ) break;
		    if( sizeof( PIX_FLOAT ) == 64 && c2->type != PIX_CONTAINER_TYPE_FLOAT64 ) break;
		    if( c2->size < 4 * 4 ) break;
		    m = (PIX_FLOAT*)c2->data;
		}
		break;
	    }
	    pix_vm_gfx_vertex_transform( (PIX_FLOAT*)c->data, m );
	}
    }
}

//
// Audio
//

void fn_set_audio_callback( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num > 0 )
    {
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	PIX_ADDR callback;
	pix_stack_val userdata;
	userdata.i = 0;
	userdata.t = 0;
	int freq = 0;
	int format = PIX_CONTAINER_TYPE_INT16;
	int channels = 1;
	uint flags = 0;
	GET_VAL_FROM_STACK( callback, 0, PIX_ADDR );
	if( callback == -1 || IS_ADDRESS_CORRECT( callback ) )
	{
	    if( callback != -1 )
		callback &= PIX_INT_ADDRESS_MASK;
	    if( pars_num > 1 ) 
	    {
		userdata = stack[ PIX_CHECK_SP( sp + 1 ) ];
	    }
	    if( pars_num > 2 ) { GET_VAL_FROM_STACK( freq, 2, int ); }
	    if( pars_num > 3 ) { GET_VAL_FROM_STACK( format, 3, int ); }
	    if( pars_num > 4 ) { GET_VAL_FROM_STACK( channels, 4, int ); }
	    if( pars_num > 5 ) { GET_VAL_FROM_STACK( flags, 5, int ); }
	    stack[ sp2 ].i = pix_vm_set_audio_callback( callback, userdata, freq, (pix_container_type)format, channels, flags, vm );
	    stack[ sp2 ].t = 0;
	}
	else
	{
	    stack[ sp2 ].i = -1;
            stack[ sp2 ].t = 0;
            PIX_VM_LOG( "set_audio_callback() error: wrong callback address %d\n", (int)callback );
	}
    }
}

void fn_enable_audio_input( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;

    if( pars_num > 0 )
    {
	int enable = 0;
	GET_VAL_FROM_STACK( enable, 0, int );
	if( vm->audio )
	{
	    sundog_sound_input( vm->audio, enable );
	    if( enable ) vm->audio_input_enabled++; else vm->audio_input_enabled--;
	}
    }
}

void fn_get_audio_sample_rate( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;

    if( pars_num > 0 )
    {
	int source = 0;
	GET_VAL_FROM_STACK( source, 0, int );
	rv = pix_vm_get_audio_sample_rate( source, vm );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].i = rv;
    stack[ sp2 ].t = 0;
}

void fn_get_note_freq( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = 0;

    if( pars_num > 0 )
    {
	int note = 0;
	int fine = 0;
	GET_VAL_FROM_STACK( note, 0, int );
	if( pars_num > 1 ) { GET_VAL_FROM_STACK( fine, 1, int ); }
	int p = 7680 - note * 64 - fine;
	if( p >= 0 )
	    rv = ( g_linear_freq_tab[ p % 768 ] >> ( p / 768 ) );
	else
	    rv = ( g_linear_freq_tab[ (7680*4+p) % 768 ] << -( ( (7680*4+p) / 768 ) - (7680*4)/768 ) ); //if pitch is negative
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//
// MIDI
//

void fn_midi_open_client( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;

    if( pars_num > 0 )
    {
	PIX_CID client_name;
	GET_VAL_FROM_STACK( client_name, 0, PIX_CID );
	bool need_to_free = 0;
	char* name = pix_vm_make_cstring_from_container( client_name, &need_to_free, vm );
	if( !name ) name = (char*)"Pixilang MIDI Client";

	rv = pix_vm_new_container( -1, sizeof( sundog_midi_client ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
	sundog_midi_client* c = (sundog_midi_client*)pix_vm_get_container_data( rv, vm );
	if( c )
	{
	    if( sundog_midi_client_open( c, vm->wm->sd, vm->audio, name, 0 ) )
	    {
		//Error:
		pix_vm_remove_container( rv, vm );
		rv = -1;
	    }
	}

	if( need_to_free ) smem_free( name );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_midi_close_client( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );

	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont && cont->type == PIX_CONTAINER_TYPE_INT8 && cont->size == sizeof( sundog_midi_client ) )
	{
	    sundog_midi_client* c = (sundog_midi_client*)cont->data;
	    if( sundog_midi_client_close( c ) == 0 )
	    {
	        pix_vm_remove_container( cnum, vm );
	        rv = 0;
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_midi_get_device( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;
    
    if( pars_num >= 3 )
    {
	PIX_CID cnum;
	int dev_num;
	uint flags;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( dev_num, 1, int );
	GET_VAL_FROM_STACK( flags, 2, int );

	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont && cont->type == PIX_CONTAINER_TYPE_INT8 && cont->size == sizeof( sundog_midi_client ) )
	{
	    sundog_midi_client* c = (sundog_midi_client*)cont->data;
	    char** devices = NULL;
	    int devs = sundog_midi_client_get_devices( c, &devices, flags );
	    if( devs > 0 && devices )
	    {
	        if( dev_num < devs )
	        {
	    	    char* name = devices[ dev_num ];
		    if( name )
		    {
		        rv = pix_vm_new_container( -1, smem_strlen( name ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
		        smem_copy( pix_vm_get_container_data( rv, vm ), name, smem_strlen( name ) );
		    }
		}
		for( int i = 0; i < devs; i++ )
		{
		    smem_free( devices[ i ] );
		}
		smem_free( devices );
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_midi_open_port( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 4 )
    {
	PIX_CID cnum;
	PIX_CID port_name_cont;
	PIX_CID dev_name_cont;
	uint flags;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port_name_cont, 1, PIX_CID );
	GET_VAL_FROM_STACK( dev_name_cont, 2, PIX_CID );
	GET_VAL_FROM_STACK( flags, 3, int );
	bool need_to_free1 = 0;
	bool need_to_free2 = 0;
	char* port_name = pix_vm_make_cstring_from_container( port_name_cont, &need_to_free1, vm );
	char* dev_name = pix_vm_make_cstring_from_container( dev_name_cont, &need_to_free2, vm );

	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont && cont->type == PIX_CONTAINER_TYPE_INT8 && cont->size == sizeof( sundog_midi_client ) )
	{
	    sundog_midi_client* c = (sundog_midi_client*)cont->data;
	    rv = sundog_midi_client_open_port( c, (const char*)port_name, (const char*)dev_name, flags );
	}

	if( need_to_free1 ) smem_free( port_name );
	if( need_to_free2 ) smem_free( dev_name );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_midi_reopen_port( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );

	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont && cont->type == PIX_CONTAINER_TYPE_INT8 && cont->size == sizeof( sundog_midi_client ) )
	{
	    sundog_midi_client* c = (sundog_midi_client*)cont->data;
	    rv = sundog_midi_client_reopen_port( c, port );
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_midi_close_port( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );

	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont && cont->type == PIX_CONTAINER_TYPE_INT8 && cont->size == sizeof( sundog_midi_client ) )
	{
	    sundog_midi_client* c = (sundog_midi_client*)cont->data;
	    rv = sundog_midi_client_close_port( c, port );
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_midi_get_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = 0;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	PIX_CID data;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );
	GET_VAL_FROM_STACK( data, 2, PIX_CID );

	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont && cont->type == PIX_CONTAINER_TYPE_INT8 && cont->size == sizeof( sundog_midi_client ) )
	{
	    sundog_midi_client* c = (sundog_midi_client*)cont->data;
	    sundog_midi_event* evt = sundog_midi_client_get_event( c, port );
	    if( evt && evt->size > 0 && evt->data )
	    {
		pix_vm_container* data_cont = pix_vm_get_container( data, vm );
		if( data_cont )
		{
		    if( data_cont->size * g_pix_container_type_sizes[ data_cont->type ] < evt->size )
		    {
			pix_vm_resize_container( data, evt->size, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
		    }
		    smem_copy( data_cont->data, evt->data, evt->size );
		}
		rv = evt->size;
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_midi_get_event_time( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );

	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont && cont->type == PIX_CONTAINER_TYPE_INT8 && cont->size == sizeof( sundog_midi_client ) )
	{
	    sundog_midi_client* c = (sundog_midi_client*)cont->data;
	    sundog_midi_event* evt = sundog_midi_client_get_event( c, port );
	    if( evt )
	    {
		rv = evt->t;
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_midi_next_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 2 )
    {
	PIX_CID cnum;
	int port;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );

	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont && cont->type == PIX_CONTAINER_TYPE_INT8 && cont->size == sizeof( sundog_midi_client ) )
	{
	    sundog_midi_client* c = (sundog_midi_client*)cont->data;
	    rv = sundog_midi_client_next_event( c, port );
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_midi_send_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    if( pars_num >= 5 )
    {
	PIX_CID cnum;
	int port;
	PIX_CID data;
	PIX_INT size;
	PIX_INT t;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( port, 1, int );
	GET_VAL_FROM_STACK( data, 2, PIX_CID );
	GET_VAL_FROM_STACK( size, 3, PIX_INT );
	GET_VAL_FROM_STACK( t, 4, PIX_INT );

	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont && cont->type == PIX_CONTAINER_TYPE_INT8 && cont->size == sizeof( sundog_midi_client ) )
	{
	    sundog_midi_client* c = (sundog_midi_client*)cont->data;
	    pix_vm_container* data_cont = pix_vm_get_container( data, vm );
	    if( data_cont && data_cont->data && data_cont->size * g_pix_container_type_sizes[ data_cont->type ] >= size )
	    {
		sundog_midi_event evt;
		evt.t = (stime_ticks_t)t;
		evt.size = (size_t)size;
		evt.data = (uint8_t*)data_cont->data;
	        rv = sundog_midi_client_send_event( c, port, &evt );
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//
// Timers
//

void fn_start_timer( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int tnum = 0;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( tnum, 0, int );
    }
    if( (unsigned)tnum < (unsigned)( vm->timers_num ) )
    {
	vm->timers[ tnum ] = stime_ms();
    }
}

void fn_get_timer( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int tnum = 0;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( tnum, 0, int );
    }
    if( (unsigned)tnum < (unsigned)( vm->timers_num ) )
    {
	uint t = stime_ms() - vm->timers[ tnum ];
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)t;
    }
}

void fn_get_year( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (PIX_INT)stime_year();
}

void fn_get_month( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (PIX_INT)stime_month();
}

void fn_get_day( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (PIX_INT)stime_day();
}

void fn_get_hours( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (PIX_INT)stime_hours();
}

void fn_get_minutes( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (PIX_INT)stime_minutes();
}

void fn_get_seconds( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (PIX_INT)stime_seconds();
}

void fn_get_ticks( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (uint32_t)stime_ticks();
}

void fn_get_tps( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (uint)stime_ticks_per_second();
}

void fn_sleep( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int ms = 0;
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( ms, 0, int );
	stime_sleep( ms );
    }
}

//
// Events
//

void fn_get_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_get_event( vm );
}

void fn_set_quit_action( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( vm->quit_action, 0, int8_t );
    }
}

//
// Threads
//

void fn_thread_create( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;
    if( pars_num >= 2 )
    {
	rv = pix_vm_create_active_thread( -1, vm );
	if( rv >= 0 )
	{
	    PIX_ADDR function;
	    uint flags = 0;
	    GET_VAL_FROM_STACK( function, 0, PIX_ADDR );
	    if( pars_num >= 3 ) 
	    {
		GET_VAL_FROM_STACK( flags, 2, int );
		pix_vm_thread* th = vm->th[ rv ];
		th->flags = flags;
	    }
	    if( IS_ADDRESS_CORRECT( function ) )
	    {
		function &= PIX_INT_ADDRESS_MASK;
		pix_vm_function fun;
		pix_stack_val pp[ 2 ];
		fun.p = pp;
    		fun.addr = function;
		fun.p[ 0 ].i = rv;
		fun.p[ 0 ].t = 0;
		fun.p[ 1 ] = stack[ PIX_CHECK_SP( sp + 1 ) ];
		fun.p_num = 2;
		pix_vm_run( rv, 1, &fun, PIX_VM_CALL_FUNCTION, vm );
	    }
	    else
	    {
		PIX_VM_LOG( "thread_create() error: wrong thread address %d\n", (int)function );
	    }
	}
    }
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_thread_destroy( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;
    if( pars_num >= 1 )
    {
	int thread_num;
	int timeout = STHREAD_TIMEOUT_INFINITE;
	GET_VAL_FROM_STACK( thread_num, 0, int );
	if( pars_num >= 2 ) GET_VAL_FROM_STACK( timeout, 1, int );
	rv = pix_vm_destroy_thread( thread_num, timeout, vm );
    }
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_mutex_create( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_CID rv = -1;

    rv = pix_vm_new_container( -1, sizeof( smutex ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    if( rv >= 0 )
    {
	smutex* m = (smutex*)pix_vm_get_container_data( rv, vm );
	if( m == NULL || smutex_init( m, 0 ) )
	{
	    pix_vm_remove_container( rv, vm );
	    rv = -1;
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_mutex_destroy( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = 0;

    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c )
	{
	    if( c->size == sizeof( smutex ) && c->data )
	    {
		rv = smutex_destroy( (smutex*)c->data );
		pix_vm_remove_container( cnum, vm );
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_mutex_lock( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = 0;

    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c )
	{
	    if( c->size == sizeof( smutex ) && c->data )
	    {
		rv = smutex_lock( (smutex*)c->data );
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_mutex_trylock( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = 0;

    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c )
	{
	    if( c->size == sizeof( smutex ) && c->data )
	    {
		rv = smutex_trylock( (smutex*)c->data );
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_mutex_unlock( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = 0;

    if( pars_num > 0 )
    {
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* c = pix_vm_get_container( cnum, vm );
	if( c )
	{
	    if( c->size == sizeof( smutex ) && c->data )
	    {
		rv = smutex_unlock( (smutex*)c->data );
	    }
	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//
// Mathematical functions
//

void fn_acos( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = acos( v );
}

void fn_acosh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = acosh( v );
}

void fn_asin( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = asin( v );
}

void fn_asinh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = asinh( v );
}

void fn_atan( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = atan( v );
}

void fn_atanh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = atanh( v );
}

void fn_atan2( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v1;
    PIX_FLOAT v2;
    GET_VAL_FROM_STACK( v1, 0, PIX_FLOAT );
    GET_VAL_FROM_STACK( v2, 1, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = atan2( v1, v2 );
}

void fn_ceil( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = ceil( v );
}

void fn_cos( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = cos( v );
}

void fn_cosh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = cosh( v );
}

void fn_exp( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = exp( v );
}

void fn_exp2( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = pow( 2.0, v );
}

void fn_expm1( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = expm1( v );
}

void fn_abs( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int type = stack[ PIX_CHECK_SP( sp + 0 ) ].t;
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = type;
    if( type == 0 )
    {
        PIX_INT v;
        GET_VAL_FROM_STACK( v, 0, PIX_INT );
        stack[ sp2 ].i = abs( v );
    }
    else
    {
        PIX_FLOAT v;
        GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
        stack[ sp2 ].f = fabs( v );
    }
}

void fn_floor( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = floor( v );
}

void fn_mod( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v1, v2;
    GET_VAL_FROM_STACK( v1, 0, PIX_FLOAT );
    GET_VAL_FROM_STACK( v2, 1, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = fmod( v1, v2 );
}

void fn_log( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = log( v );
}

void fn_log2( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = LOG2( v );
}

void fn_log10( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = log10( v );
}

void fn_pow( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v1, v2;
    GET_VAL_FROM_STACK( v1, 0, PIX_FLOAT );
    GET_VAL_FROM_STACK( v2, 1, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = pow( v1, v2 );
}

void fn_sin( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = sin( v );
}

void fn_sinh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = sinh( v );
}

void fn_sqrt( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = sqrt( v );
}

void fn_tan( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = tan( v );
}

void fn_tanh( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_FLOAT v;
    GET_VAL_FROM_STACK( v, 0, PIX_FLOAT );
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 1;
    stack[ sp2 ].f = tanh( v );
}

void fn_rand( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pseudo_random( &vm->random );
}

void fn_rand_seed( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_INT v;
	GET_VAL_FROM_STACK( v, 0, PIX_INT );
	vm->random = v;
    }
}

//
// Type punning
//

void fn_reinterpret_type( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num != 3 ) return;

    int to_float;
    int bits;
    GET_VAL_FROM_STACK( to_float, 1, int );
    GET_VAL_FROM_STACK( bits, 2, int );

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( to_float )
    {
	//INT -> X-bit FLOAT -> FLOAT
	PIX_FLOAT res;
	if( bits > 32 )
	{
	    union { uint64_t i; double f; } v;
	    v.i = stack[ PIX_CHECK_SP( sp ) ].i;
	    res = v.f;
	}
	else
	{
	    union { uint32_t i; float f; } v;
	    v.i = stack[ PIX_CHECK_SP( sp ) ].i;
	    res = v.f;
	}
	stack[ sp2 ].f = res;
	stack[ sp2 ].t = 1;
    }
    else
    {
	//FLOAT -> X-bit FLOAT -> INT
	PIX_INT res;
	if( bits > 32 )
	{
	    union { uint64_t i; double f; } v;
	    v.f = stack[ PIX_CHECK_SP( sp ) ].f;
	    res = v.i;
	}
	else
	{
	    union { uint32_t i; float f; } v;
	    v.f = stack[ PIX_CHECK_SP( sp ) ].f;
	    res = v.i;
	}
	stack[ sp2 ].i = res;
	stack[ sp2 ].t = 0;
    }
}

//
// Data processing
//

void fn_op_cn( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_VAL retval;
    retval.i = 0;
    int8_t retval_type = 0;

    if( pars_num >= 2 )
    {
	int opcode;
	PIX_CID cnum;
	int8_t val_type = 0;
	PIX_VAL val;
	val.i = 0;
	PIX_INT x = 0;
	PIX_INT y = 0;
	PIX_INT xsize = 0;
	PIX_INT ysize = 0;

	GET_VAL_FROM_STACK( opcode, 0, int );
	GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
	if( pars_num >= 3 )
	{
	    val_type = stack[ PIX_CHECK_SP( sp + 2 ) ].t;
	    val = stack[ PIX_CHECK_SP( sp + 2 ) ].v;
	}

	if( pars_num == 5 )
	{
	    //1D:
	    GET_VAL_FROM_STACK( x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 4, PIX_INT );
	    if( xsize <= 0 ) return;
	}
	if( pars_num == 7 )
	{
	    //2D:
	    GET_VAL_FROM_STACK( x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( y, 4, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 5, PIX_INT );
	    GET_VAL_FROM_STACK( ysize, 6, PIX_INT );
	    if( xsize <= 0 ) return;
	    if( ysize <= 0 ) return;
	}

	pix_vm_op_cn( opcode, cnum, val_type, val, x, y, xsize, ysize, &retval, &retval_type, vm );
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = retval_type;
    stack[ sp2 ].v = retval;
}

void fn_op_cc( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 3 )
    {
	int opcode;
	PIX_CID cnum1;
	PIX_CID cnum2;
	PIX_INT dest_x = 0;
	PIX_INT dest_y = 0;
	PIX_INT src_x = 0;
	PIX_INT src_y = 0;
	PIX_INT xsize = 0;
	PIX_INT ysize = 0;

	GET_VAL_FROM_STACK( opcode, 0, int );
	GET_VAL_FROM_STACK( cnum1, 1, PIX_CID );
	GET_VAL_FROM_STACK( cnum2, 2, PIX_CID );

	if( pars_num == 6 )
	{
	    //1D:
	    GET_VAL_FROM_STACK( dest_x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( src_x, 4, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 5, PIX_INT );
	    if( xsize <= 0 ) return;
	}
	if( pars_num == 9 )
	{
	    //2D:
	    GET_VAL_FROM_STACK( dest_x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( dest_y, 4, PIX_INT );
	    GET_VAL_FROM_STACK( src_x, 5, PIX_INT );
	    GET_VAL_FROM_STACK( src_y, 6, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 7, PIX_INT );
	    GET_VAL_FROM_STACK( ysize, 8, PIX_INT );
	    if( xsize <= 0 ) return;
	    if( ysize <= 0 ) return;
	}

	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
        stack[ sp2 ].i = pix_vm_op_cc( opcode, cnum1, cnum2, dest_x, dest_y, src_x, src_y, xsize, ysize, vm );
    }
}

void fn_op_ccn( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 4 )
    {
	int opcode;
	PIX_CID cnum1;
	PIX_CID cnum2;
	int8_t val_type;
	PIX_VAL val;
	PIX_INT dest_x = 0;
	PIX_INT dest_y = 0;
	PIX_INT src_x = 0;
	PIX_INT src_y = 0;
	PIX_INT xsize = 0;
	PIX_INT ysize = 0;
	
	GET_VAL_FROM_STACK( opcode, 0, int );
	GET_VAL_FROM_STACK( cnum1, 1, PIX_CID );
	GET_VAL_FROM_STACK( cnum2, 2, PIX_CID );
	val_type = stack[ PIX_CHECK_SP( sp + 3 ) ].t;
	val = stack[ PIX_CHECK_SP( sp + 3 ) ].v;
	
	if( pars_num == 7 )
	{
	    //1D:
	    GET_VAL_FROM_STACK( dest_x, 4, PIX_INT );
	    GET_VAL_FROM_STACK( src_x, 5, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 6, PIX_INT );
	    if( xsize <= 0 ) return;
	}
	if( pars_num == 10 )
	{
	    //2D:
	    GET_VAL_FROM_STACK( dest_x, 4, PIX_INT );
	    GET_VAL_FROM_STACK( dest_y, 5, PIX_INT );
	    GET_VAL_FROM_STACK( src_x, 6, PIX_INT );
	    GET_VAL_FROM_STACK( src_y, 7, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 8, PIX_INT );
	    GET_VAL_FROM_STACK( ysize, 9, PIX_INT );
	    if( xsize <= 0 ) return;
	    if( ysize <= 0 ) return;
	}
	
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
        stack[ sp2 ].i = pix_vm_op_ccn( opcode, cnum1, cnum2, val_type, val, dest_x, dest_y, src_x, src_y, xsize, ysize, vm );
    }
}

void fn_generator( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 2 )
    {
	int opcode;
	PIX_CID cnum;
	PIX_FLOAT fval[ 4 ];
	fval[ 0 ] = 0; //Phase
	fval[ 1 ] = 1; //Amplitude
	fval[ 2 ] = 0; //Delta X
	fval[ 3 ] = 0; //Delta Y
	PIX_INT x = 0;
	PIX_INT y = 0;
	PIX_INT xsize = 0;
	PIX_INT ysize = 0;
	
	GET_VAL_FROM_STACK( opcode, 0, int );
	GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
	if( pars_num >= 3 ) GET_VAL_FROM_STACK( fval[ 0 ], 2, PIX_FLOAT );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( fval[ 1 ], 3, PIX_FLOAT );
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( fval[ 2 ], 4, PIX_FLOAT );
	if( pars_num >= 6 ) GET_VAL_FROM_STACK( fval[ 3 ], 5, PIX_FLOAT );
	
	if( pars_num == 8 )
	{
	    //1D:
	    GET_VAL_FROM_STACK( x, 6, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 7, PIX_INT );
	    if( xsize <= 0 ) return;
	}
	if( pars_num == 10 )
	{
	    //2D:
	    GET_VAL_FROM_STACK( x, 6, PIX_INT );
	    GET_VAL_FROM_STACK( y, 7, PIX_INT );
	    GET_VAL_FROM_STACK( xsize, 8, PIX_INT );
	    GET_VAL_FROM_STACK( ysize, 9, PIX_INT );
	    if( xsize <= 0 ) return;
	    if( ysize <= 0 ) return;
	}
	
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
        stack[ sp2 ].i = pix_vm_generator( opcode, cnum, fval, x, y, xsize, ysize, vm );
    }
}

void fn_wavetable_generator( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 8 )
    {
	PIX_CID dest; //Audio destination (INT16 or FLOAT32)
	PIX_INT dest_offset;
	PIX_INT dest_length;
	PIX_CID table; //Table with waveform (supported formats: 32768 x INT16, 32768 x FLOAT32)
	PIX_CID amp; //INT32 array of amplitudes (fixed point 16.16)
	PIX_CID amp_delta; //INT32 array of amplitude delta values (fixed point 16.16)
	PIX_CID pos; //INT32 array of wavetable positions (fixed point 16.16)
	PIX_CID pos_delta; //INT32 array of wavetable position delta values (fixed point 16.16)
	PIX_INT gen_offset; //Number of the first generator
	PIX_INT gen_step; //Play every gen_step generator
	PIX_INT gen_count; //Total number of generators to play
	GET_VAL_FROM_STACK( dest, 0, PIX_CID );
	GET_VAL_FROM_STACK( dest_offset, 1, PIX_INT );
	GET_VAL_FROM_STACK( dest_length, 2, PIX_INT );
	GET_VAL_FROM_STACK( table, 3, PIX_CID );
	GET_VAL_FROM_STACK( amp, 4, PIX_CID );
	GET_VAL_FROM_STACK( amp_delta, 5, PIX_CID );
	GET_VAL_FROM_STACK( pos, 6, PIX_CID );
	GET_VAL_FROM_STACK( pos_delta, 7, PIX_CID );
	GET_VAL_FROM_STACK( gen_offset, 8, PIX_INT );
	GET_VAL_FROM_STACK( gen_step, 9, PIX_INT );
	GET_VAL_FROM_STACK( gen_count, 10, PIX_INT );

	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
        stack[ sp2 ].i = pix_vm_wavetable_generator( 
    	    dest, dest_offset, dest_length, 
    	    table, 
    	    amp, amp_delta, 
    	    pos, pos_delta, 
    	    gen_offset, gen_step, gen_count, 
    	    vm );
    }
}

void fn_sampler( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	int pars;
	GET_VAL_FROM_STACK( pars, 0, int );
	if( (unsigned)pars >= (unsigned)vm->c_num ) return;
	pix_vm_container* pars_cont = vm->c[ pars ];
	PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
        stack[ sp2 ].i = pix_vm_sampler( pars_cont, vm );
    }
}

void fn_envelope2p( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 3 )
    {
	PIX_CID cnum;
	PIX_INT v1;
	PIX_INT v2;
	PIX_INT offset = 0;
	PIX_INT size = -1;
	int8_t dc_off1_type = 0;
	int8_t dc_off2_type = 0; 
	PIX_VAL dc_off1; dc_off1.i = 0;
	PIX_VAL dc_off2; dc_off2.i = 0;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( v1, 1, PIX_INT );
	GET_VAL_FROM_STACK( v2, 2, PIX_INT );
	if( pars_num > 3 ) { GET_VAL_FROM_STACK( offset, 3, PIX_INT ); }
	if( pars_num > 4 ) { GET_VAL_FROM_STACK( size, 4, PIX_INT ); }
	if( pars_num > 5 ) { dc_off1_type = stack[ PIX_CHECK_SP( sp + 5 ) ].t; dc_off1 = stack[ PIX_CHECK_SP( sp + 5 ) ].v; }
	if( pars_num > 6 ) { dc_off2_type = stack[ PIX_CHECK_SP( sp + 6 ) ].t; dc_off2 = stack[ PIX_CHECK_SP( sp + 6 ) ].v; }
        PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
	stack[ sp2 ].t = 0;
        stack[ sp2 ].i = pix_vm_envelope2p( cnum, v1, v2, offset, size, dc_off1_type, dc_off1, dc_off2_type, dc_off2, vm );
    }
}

void fn_gradient( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = -1;
    while( 1 )
    {
	if( pars_num < 5 ) break;
    
	PIX_VAL v[ 4 ];
	int8_t v_types[ 4 ];
	PIX_CID cnum;
	PIX_INT x = 0;
	PIX_INT y = 0;
	PIX_INT x_step = 1;
	PIX_INT y_step = 1;

	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	pix_vm_container* cont = pix_vm_get_container( cnum, vm );
	if( cont == 0 ) break;
	PIX_INT xsize = cont->xsize;
	PIX_INT ysize = cont->ysize;

	for( int i = 0; i < 4; i++ )
	{
	    v[ i ] = stack[ PIX_CHECK_SP( sp + i + 1 ) ].v;
	    v_types[ i ] = stack[ PIX_CHECK_SP( sp + i + 1 ) ].t;
	    if( cont->type < PIX_CONTAINER_TYPE_FLOAT32 )
	    {
		if( v_types[ i ] == 1 )
		{
		    //Float to Int:
		    v_types[ i ] = 0;
		    v[ i ].i = v[ i ].f;
		}
	    }
	    else
	    {
		if( v_types[ i ] == 0 )
		{
		    //Int to Float:
		    v_types[ i ] = 1;
		    v[ i ].f = v[ i ].i;
		}
	    }
	}
	int pnum = 5;
	while( 1 )
	{
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( x, pnum, PIX_INT ); } else break; pnum++;
    	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( y, pnum, PIX_INT ); } else break; pnum++;
    	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( xsize, pnum, PIX_INT ); } else break; pnum++;
    	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( ysize, pnum, PIX_INT ); } else break; pnum++;
    	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( x_step, pnum, PIX_INT ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( y_step, pnum, PIX_INT ); } else break; pnum++;
	    break;
	}

	if( x + xsize < 0 ) break;
	if( y + ysize < 0 ) break;
	if( x >= cont->xsize ) break;
	if( y >= cont->ysize ) break;
	if( xsize <= 0 ) break;
	if( ysize <= 0 ) break;
	if( x_step <= 0 ) x_step = 1;
	if( y_step <= 0 ) y_step = 1;

	PIX_INT xd, yd, xstart, ystart;
	PIX_FLOAT xd_f, yd_f, xstart_f, ystart_f;
	if( cont->type < PIX_CONTAINER_TYPE_FLOAT32 )
	{
	    xd = ( 32768 << 15 ) / xsize;
	    xstart = 0;
	    yd = ( 32768 << 15 ) / ysize;
	    ystart = 0;
	    if( x < 0 ) { xsize -= -x; xstart = -x * xd; x = 0; }
	    if( y < 0 ) { ysize -= -y; ystart = -y * yd; y = 0; }
	    xd *= x_step;
	    yd *= y_step;
	}
	else
	{
	    xd_f = 1.0F / (PIX_FLOAT)xsize;
	    xstart_f = 0;
	    yd_f = 1.0F / (PIX_FLOAT)ysize;
	    ystart_f = 0;
	    if( x < 0 ) { xsize -= -x; xstart_f = -x * xd_f; x = 0; }
	    if( y < 0 ) { ysize -= -y; ystart_f = -y * yd_f; y = 0; }
	    xd_f *= x_step;
	    yd_f *= y_step;
	}
	
	if( x + xsize > cont->xsize ) xsize = cont->xsize - x;
	if( y + ysize > cont->ysize ) ysize = cont->ysize - y;

	if( cont->type < PIX_CONTAINER_TYPE_FLOAT32 )
	{
	    //Int:
	    PIX_INT yy = ystart;
	    for( PIX_INT cy = 0; cy < ysize; cy += y_step )
	    {
		PIX_INT xx = xstart;
		PIX_INT n = yy >> 15;
		PIX_INT nn = 32768 - n;
		switch( cont->type )
		{
		    case PIX_CONTAINER_TYPE_INT8:
			{
			    int8_t* ptr = (int8_t*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_INT v1 = ( v[ 0 ].i * nn + v[ 2 ].i * n ) >> 15;
			    PIX_INT v2 = ( v[ 1 ].i * nn + v[ 3 ].i * n ) >> 15;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx >> 15;
				nn = 32768 - n;
				*ptr = ( v1 * nn + v2 * n ) >> 15;
				ptr += x_step;
				xx += xd;
			    }
			}
			break;
		    case PIX_CONTAINER_TYPE_INT16:
			{
			    int16_t* ptr = (int16_t*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_INT v1 = ( v[ 0 ].i * nn + v[ 2 ].i * n ) >> 15;
			    PIX_INT v2 = ( v[ 1 ].i * nn + v[ 3 ].i * n ) >> 15;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx >> 15;
				nn = 32768 - n;
				*ptr = ( v1 * nn + v2 * n ) >> 15;
				ptr += x_step;
				xx += xd;
			    }
			}
			break;
		    case PIX_CONTAINER_TYPE_INT32:
			{
			    int* ptr = (int*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_INT v1 = ( v[ 0 ].i * nn + v[ 2 ].i * n ) >> 15;
			    PIX_INT v2 = ( v[ 1 ].i * nn + v[ 3 ].i * n ) >> 15;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx >> 15;
				nn = 32768 - n;
				*ptr = ( v1 * nn + v2 * n ) >> 15;
				ptr += x_step;
				xx += xd;
			    }
			}
			break;
#ifdef PIX_INT64_ENABLED
		    case PIX_CONTAINER_TYPE_INT64:
			{
			    int64_t* ptr = (int64_t*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_INT v1 = ( v[ 0 ].i * nn + v[ 2 ].i * n ) >> 15;
			    PIX_INT v2 = ( v[ 1 ].i * nn + v[ 3 ].i * n ) >> 15;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx >> 15;
				nn = 32768 - n;
				*ptr = ( v1 * nn + v2 * n ) >> 15;
				ptr += x_step;
				xx += xd;
			    }
			}
			break;
#endif
		}
		yy += yd;
	    }
	}
	else
	{
	    //Float:
	    PIX_FLOAT yy = ystart_f;
	    for( PIX_INT cy = 0; cy < ysize; cy += y_step )
	    {
		PIX_FLOAT xx = xstart_f;
		PIX_FLOAT n = yy;
		PIX_FLOAT nn = 1 - n;
		switch( cont->type )
		{
		    case PIX_CONTAINER_TYPE_FLOAT32:
			{
			    float* ptr = (float*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_FLOAT v1 = v[ 0 ].f * nn + v[ 2 ].f * n;
			    PIX_FLOAT v2 = v[ 1 ].f * nn + v[ 3 ].f * n;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx;
				nn = 1 - n;
				*ptr = v1 * nn + v2 * n;
				ptr += x_step;
				xx += xd_f;
			    }
			}
			break;
#ifdef PIX_FLOAT64_ENABLED
		    case PIX_CONTAINER_TYPE_FLOAT64:
			{
			    double* ptr = (double*)cont->data + ( y + cy ) * cont->xsize + x;
			    PIX_FLOAT v1 = v[ 0 ].f * nn + v[ 2 ].f * n;
			    PIX_FLOAT v2 = v[ 1 ].f * nn + v[ 3 ].f * n;
			    for( int cx = 0; cx < xsize; cx += x_step )
			    {
				n = xx;
				nn = 1 - n;
				*ptr = v1 * nn + v2 * n;
				ptr += x_step;
				xx += xd_f;
			    }
			}
			break;
#endif
		}
		yy += yd_f;
	    }
	}
	
	rv = 0;
	
	break;
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_fft( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 3 )
    {
	uint flags;
	PIX_CID cnum1;
	PIX_CID cnum2;
	int size = 0;
	GET_VAL_FROM_STACK( flags, 0, int );
	GET_VAL_FROM_STACK( cnum1, 1, PIX_CID );
	GET_VAL_FROM_STACK( cnum2, 2, PIX_CID );
	if( pars_num >= 4 )
	    GET_VAL_FROM_STACK( size, 3, int );
	if( (unsigned)cnum1 >= (unsigned)vm->c_num ) return;
	if( (unsigned)cnum2 >= (unsigned)vm->c_num ) return;
	if( size < 0 ) return;
	pix_vm_container* cont1 = vm->c[ cnum1 ];
	pix_vm_container* cont2 = vm->c[ cnum2 ];
	if( cont1 && cont2 && cont1->data && cont2->data )
	{
	    if( cont1->type != cont2->type ) return;
	    if( size == 0 ) size = (int)cont1->size;
	    if( cont1->type == PIX_CONTAINER_TYPE_FLOAT32 )
	    {
		fft( flags, (float*)cont1->data, (float*)cont2->data, size );
	    }
	    if( cont1->type == PIX_CONTAINER_TYPE_FLOAT64 )
	    {
		fft( flags, (double*)cont1->data, (double*)cont2->data, size );
	    }
	}
    }
}

void fn_new_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    uint flags = 0; //optional;

    if( pars_num >= 1 ) GET_VAL_FROM_STACK( flags, 0, int );

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = pix_vm_new_filter( flags, vm );
}

void fn_remove_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_CID f;
	GET_VAL_FROM_STACK( f, 0, PIX_CID );
	pix_vm_remove_filter( f, vm );
    }
}

void fn_init_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( pars_num >= 2 )
    {
	PIX_CID f;
	PIX_CID a; //feedforward filter coefficients;
	PIX_CID b = -1; //feedback filter coefficients; optional; can be -1;
	int rshift = 0; //bitwise right shift for fixed point computations; optional;
	uint flags = 0; //optional;

	GET_VAL_FROM_STACK( f, 0, PIX_CID );
	GET_VAL_FROM_STACK( a, 1, PIX_CID );
	while( 1 )
	{
	    if( pars_num > 2 ) { GET_VAL_FROM_STACK( b, 2, PIX_CID ); } else break;
	    if( pars_num > 3 ) { GET_VAL_FROM_STACK( rshift, 3, int ); } else break;
	    if( pars_num > 4 ) { GET_VAL_FROM_STACK( flags, 4, int ); } else break;
	    break;
	}

	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = pix_vm_init_filter( f, a, b, rshift, flags, vm );
    }
    else
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }
}

void fn_reset_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    if( pars_num >= 1 )
    {
	PIX_CID f;
	GET_VAL_FROM_STACK( f, 0, PIX_CID );
	pix_vm_reset_filter( f, vm );
    }
}

void fn_apply_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    // output[ n ] = ( a[ 0 ] * input[ n ] + a[ 1 ] * input[ n - 1 ] + ... + a[ a_count - 1 ] * input[ n - a_count - 1 ]
    //                                     + b[ 0 ] * output[ n - 1 ] + ... + b[ b_count - 1 ] * output[ n - b_count - 1 ] ) >> rshift;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( pars_num >= 3 )
    {
	PIX_CID f; //filter (created with new_filter());
	PIX_CID output;
	PIX_CID input;
	uint flags = 0; //optional;
	PIX_INT offset = 0; //optional;
	PIX_INT size = -1; //optional;

	GET_VAL_FROM_STACK( f, 0, PIX_CID );
	GET_VAL_FROM_STACK( output, 1, PIX_CID );
	GET_VAL_FROM_STACK( input, 2, PIX_CID );
	while( 1 )
	{
	    if( pars_num > 3 ) { GET_VAL_FROM_STACK( flags, 3, int ); } else break;
	    if( pars_num > 4 ) { GET_VAL_FROM_STACK( offset, 4, PIX_INT ); } else break;
	    if( pars_num > 5 ) { GET_VAL_FROM_STACK( size, 5, PIX_INT ); } else break;
	    break;
	}

	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = pix_vm_apply_filter( f, output, input, flags, offset, size, vm );
    }
    else
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }
}

void fn_replace_values( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    
    while( pars_num >= 3 )
    {
	PIX_CID dest;
	PIX_CID src;
	PIX_CID values;
	size_t dest_offset = 0;
	size_t src_offset = 0;
	size_t size = -1;
	GET_VAL_FROM_STACK( dest, 0, PIX_CID );
	GET_VAL_FROM_STACK( src, 1, PIX_CID );
	GET_VAL_FROM_STACK( values, 2, PIX_CID );
	while( 1 )
	{
	    if( pars_num >= 4 ) GET_VAL_FROM_STACK( dest_offset, 3, size_t ) else break;
	    if( pars_num >= 5 ) GET_VAL_FROM_STACK( src_offset, 4, size_t ) else break;
	    if( pars_num >= 6 ) GET_VAL_FROM_STACK( size, 5, size_t ) else break;
	    break;
	}
	if( (unsigned)dest >= (unsigned)vm->c_num ) break;
	if( (unsigned)src >= (unsigned)vm->c_num ) break;
	if( (unsigned)values >= (unsigned)vm->c_num ) break;
	pix_vm_container* dest_cont = vm->c[ dest ];
	pix_vm_container* src_cont = vm->c[ src ];
	pix_vm_container* values_cont = vm->c[ values ];
	if( dest_cont == 0 ) break;
	if( src_cont == 0 ) break;
	if( values_cont == 0 ) break;
	size_t values_num = values_cont->size;
	
	if( dest_cont->type != values_cont->type )
	{
	    PIX_VM_LOG( "replace_values(): destination type must be = values type\n" );
	    break;
	}

	if( size == -1 ) size = dest_cont->size;
	if( dest_offset >= dest_cont->size ) break;
	if( src_offset >= src_cont->size ) break;
	if( dest_offset + size > dest_cont->size )
	{
	    size = dest_cont->size - dest_offset;
	}
	if( src_offset + size > src_cont->size )
	{
	    size = src_cont->size - src_offset;
	}
	
	if( dest_cont->type == values_cont->type )
	{
	    switch( src_cont->type )
	    {
		case PIX_CONTAINER_TYPE_INT8:
		    {
			uint8_t* s = (uint8_t*)src_cont->data + src_offset;
			switch( dest_cont->type )
			{
		    	    case PIX_CONTAINER_TYPE_INT8: { uint8_t* d = (uint8_t*)dest_cont->data + dest_offset; uint8_t* v = (uint8_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT16: { uint16_t* d = (uint16_t*)dest_cont->data + dest_offset; uint16_t* v = (uint16_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break; 
		    	    case PIX_CONTAINER_TYPE_INT32:
		    	    case PIX_CONTAINER_TYPE_FLOAT32:
		    		{ uint* d = (uint*)dest_cont->data + dest_offset; uint* v = (uint*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
			    case PIX_CONTAINER_TYPE_INT64:
			    case PIX_CONTAINER_TYPE_FLOAT64:
		    		{ uint64_t* d = (uint64_t*)dest_cont->data + dest_offset; uint64_t* v = (uint64_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#endif
			    default:
				PIX_VM_LOG( "replace_values(): unsupported type of destination container\n" );
				break;
			}
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT16:
		    {
			uint16_t* s = (uint16_t*)src_cont->data + src_offset;
			switch( dest_cont->type )
			{
		    	    case PIX_CONTAINER_TYPE_INT8: { uint8_t* d = (uint8_t*)dest_cont->data + dest_offset; uint8_t* v = (uint8_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT16: { uint16_t* d = (uint16_t*)dest_cont->data + dest_offset; uint16_t* v = (uint16_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT32:
		    	    case PIX_CONTAINER_TYPE_FLOAT32:
		    		{ uint* d = (uint*)dest_cont->data + dest_offset; uint* v = (uint*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
			    case PIX_CONTAINER_TYPE_INT64:
			    case PIX_CONTAINER_TYPE_FLOAT64:
		    		{ uint64_t* d = (uint64_t*)dest_cont->data + dest_offset; uint64_t* v = (uint64_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#endif
			    default:
				PIX_VM_LOG( "replace_values(): unsupported type of destination container\n" );
				break;
			}
		    }
		    break;
		case PIX_CONTAINER_TYPE_INT32:
		    {
			uint* s = (uint*)src_cont->data + src_offset;
			switch( dest_cont->type )
			{
		    	    case PIX_CONTAINER_TYPE_INT8: { uint8_t* d = (uint8_t*)dest_cont->data + dest_offset; uint8_t* v = (uint8_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT16: { uint16_t* d = (uint16_t*)dest_cont->data + dest_offset; uint16_t* v = (uint16_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT32:
		    	    case PIX_CONTAINER_TYPE_FLOAT32:
		    		{ uint* d = (uint*)dest_cont->data + dest_offset; uint* v = (uint*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
			    case PIX_CONTAINER_TYPE_INT64:
			    case PIX_CONTAINER_TYPE_FLOAT64:
		    		{ uint64_t* d = (uint64_t*)dest_cont->data + dest_offset; uint64_t* v = (uint64_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#endif
			    default:
				PIX_VM_LOG( "replace_values(): unsupported type of destination container\n" );
				break;
			}
		    }
		    break;
#ifdef PIX_INT64_ENABLED
		case PIX_CONTAINER_TYPE_INT64:
		    {
			uint64_t* s = (uint64_t*)src_cont->data + src_offset;
			switch( dest_cont->type )
			{
		    	    case PIX_CONTAINER_TYPE_INT8: { uint8_t* d = (uint8_t*)dest_cont->data + dest_offset; uint8_t* v = (uint8_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT16: { uint16_t* d = (uint16_t*)dest_cont->data + dest_offset; uint16_t* v = (uint16_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
		    	    case PIX_CONTAINER_TYPE_INT32:
		    	    case PIX_CONTAINER_TYPE_FLOAT32:
		    		{ uint* d = (uint*)dest_cont->data + dest_offset; uint* v = (uint*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#if defined(PIX_INT64_ENABLED) || defined(PIX_FLOAT64_ENABLED)
			    case PIX_CONTAINER_TYPE_INT64:
			    case PIX_CONTAINER_TYPE_FLOAT64:
		    		{ uint64_t* d = (uint64_t*)dest_cont->data + dest_offset; uint64_t* v = (uint64_t*)values_cont->data; for( size_t i = 0; i < size; i++ ) d[ i ] = v[ s[ i ] ]; rv = 0; } break;
#endif
			    default:
				PIX_VM_LOG( "replace_values(): unsupported type of destination container\n" );
				break;
			}
		    }
		    break;
#endif
		default:
		    PIX_VM_LOG( "replace_values(): unsupported type of source container\n" );
		    break;
	    }
	}
	break;
    }
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_copy_and_resize( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    
    while( pars_num >= 2 )
    {
	pix_vm_resize_pars pars;
	smem_clear( &pars, sizeof( pars ) );
	
	PIX_CID dest_cnum;
	PIX_CID src_cnum;
	
	GET_VAL_FROM_STACK( dest_cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( src_cnum, 1, PIX_CID );
	
	pix_vm_container* dest = pix_vm_get_container( dest_cnum, vm );
	pix_vm_container* src = pix_vm_get_container( src_cnum, vm );
	if( dest == 0 || src == 0 ) break;
	
	if( dest->type != src->type )
	{
	    PIX_VM_LOG( "copy_and_resize(): destination type must be = src type\n" );
	    break;
	}
	
	pars.dest = dest->data;
	pars.src = src->data;
	pars.type = dest->type;
	pars.dest_xsize = dest->xsize;
	pars.dest_ysize = dest->ysize;
	pars.src_xsize = src->xsize;
	pars.src_ysize = src->ysize;
	
	if( pars_num >= 3 ) { GET_VAL_FROM_STACK( pars.resize_flags, 2, int ); } else pars.resize_flags = PIX_RESIZE_INTERP1;
	if( pars_num == 11 )
	{
	    GET_VAL_FROM_STACK( pars.dest_x, 3, PIX_INT );
	    GET_VAL_FROM_STACK( pars.dest_y, 4, PIX_INT );
	    GET_VAL_FROM_STACK( pars.dest_rect_xsize, 5, PIX_INT );
	    GET_VAL_FROM_STACK( pars.dest_rect_ysize, 6, PIX_INT );
	    GET_VAL_FROM_STACK( pars.src_x, 7, PIX_INT );
	    GET_VAL_FROM_STACK( pars.src_y, 8, PIX_INT );
	    GET_VAL_FROM_STACK( pars.src_rect_xsize, 9, PIX_INT );
	    GET_VAL_FROM_STACK( pars.src_rect_ysize, 10, PIX_INT );
	}
	
	pix_vm_copy_and_resize( &pars );
	
	rv = 0;
	
	break;
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_conv_filter( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_INT rv = -1;
    
    while( pars_num >= 3 )
    {
	pix_vm_conv_filter_pars pars;
	smem_clear( &pars, sizeof( pars ) );
	
	PIX_CID dest_cnum;
	PIX_CID src_cnum;
	PIX_CID kernel_cnum;
	
	GET_VAL_FROM_STACK( dest_cnum, 0, PIX_CID );
	GET_VAL_FROM_STACK( src_cnum, 1, PIX_CID );
	GET_VAL_FROM_STACK( kernel_cnum, 2, PIX_CID );
	
	pix_vm_container* dest = pix_vm_get_container( dest_cnum, vm );
	pix_vm_container* src = pix_vm_get_container( src_cnum, vm );
	pix_vm_container* kernel = pix_vm_get_container( kernel_cnum, vm );
	if( dest == 0 || src == 0 || kernel == 0 ) break;
	pars.dest = dest;
	pars.src = src;
	pars.kernel = kernel;
	
	pars.kernel_xcenter = kernel->xsize / 2;
	pars.kernel_ycenter = kernel->ysize / 2;

	int pnum = 3;
	while( 1 )
	{
	    if( pars_num > pnum ) { pars.div = stack[ PIX_CHECK_SP( sp + pnum ) ].v; pars.div_type = stack[ PIX_CHECK_SP( sp + pnum ) ].t; } else break; pnum++;
    	    if( pars_num > pnum ) { pars.offset = stack[ PIX_CHECK_SP( sp + pnum ) ].v; pars.offset_type = stack[ PIX_CHECK_SP( sp + pnum ) ].t; } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.flags, pnum, uint ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.kernel_xcenter, pnum, int ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.kernel_ycenter, pnum, int ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.dest_x, pnum, PIX_INT ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.dest_y, pnum, PIX_INT ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.src_x, pnum, PIX_INT ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.src_y, pnum, PIX_INT ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.xsize, pnum, PIX_INT ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.ysize, pnum, PIX_INT ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.xstep, pnum, int ); } else break; pnum++;
	    if( pars_num > pnum ) { GET_VAL_FROM_STACK( pars.ystep, pnum, int ); } else break; pnum++;
	    break;
	}
	
	rv = pix_vm_conv_filter( vm, &pars );
	
	break;
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//
// Dialogs
//

void fn_file_dialog( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    bool name_ = 0;
    bool mask_ = 0;
    bool id_ = 0;
    bool defname_ = 0;
    char* name = NULL;
    char* mask = NULL;
    char* id = NULL;
    char* defname = NULL;
    uint32_t flags = 0;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 3 ) { err = 1; break; }
	PIX_CID cnum;
	GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	name = pix_vm_make_cstring_from_container( cnum, &name_, vm );
	if( !name ) { err = 1; break; }
	GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
	mask = pix_vm_make_cstring_from_container( cnum, &mask_, vm );
	GET_VAL_FROM_STACK( cnum, 2, PIX_CID );
	id = pix_vm_make_cstring_from_container( cnum, &id_, vm );
	if( !id ) { err = 1; break; }
	if( pars_num > 3 )
	{
	    GET_VAL_FROM_STACK( cnum, 3, PIX_CID );
	    defname = pix_vm_make_cstring_from_container( cnum, &defname_, vm );
	}
	if( pars_num > 4 )
	{
	    uint32_t ff = 0;
	    GET_VAL_FROM_STACK( ff, 4, uint32_t );
	    flags |= ff;
	}
	break;
    }

    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;
    if( err == 0 )
    {
	pix_sundog_filedialog* req = vm->sd_filedialog;
	if( req == NULL )
	{
	    req = smem_new_struct( pix_sundog_filedialog );
	    vm->sd_filedialog = req;
	}
	if( req )
	{
	    smem_zero( req );

	    req->name = name;
	    if( mask && mask[ 0 ] == 0 )
		req->mask = NULL;
	    else
		req->mask = mask;
	    req->id = id;
	    req->def_name = defname;
	    req->flags = flags;

	    sundog_event evt;
	    smem_clear_struct( evt );
	    evt.win = vm->win;
	    evt.type = EVT_PIXICMD;
	    evt.x = pix_sundog_req_filedialog;
	    if( send_events( &evt, 1, vm->wm ) == 0 )
	    {
		while( 1 )
		{
		    if( req->handled ) break;
		    if( !vm->ready ) break;
		    stime_sleep( 100 );
		}
		stack[ sp2 ].i = pix_vm_make_container_from_cstring( (const char*)req->result, vm );
	    }
	}
	if( name_ ) smem_free( name );
	if( mask_ ) smem_free( mask );
	if( id_ ) smem_free( id );
	if( defname_ ) smem_free( defname );
    }
}

void fn_prefs_dialog( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = 0;

    sundog_event evt;
    smem_clear_struct( evt );
    evt.win = vm->win;
    evt.type = EVT_PIXICMD;
    evt.x = pix_sundog_req_preferences;
    send_events( &evt, 1, vm->wm );
}

void fn_textinput_dialog( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    bool def_str_ = 0;
    bool name_ = 0;
    char* def_str = NULL;
    char* name = NULL;

    //Get parameters:
    while( 1 )
    {
	PIX_CID cnum;
	if( pars_num > 0 )
	{
	    GET_VAL_FROM_STACK( cnum, 0, PIX_CID );
	    def_str = pix_vm_make_cstring_from_container( cnum, &def_str_, vm );
	}
	if( pars_num > 1 )
	{
	    GET_VAL_FROM_STACK( cnum, 1, PIX_CID );
	    name = pix_vm_make_cstring_from_container( cnum, &name_, vm );
	}
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;

    pix_sundog_textinput* req = vm->sd_textinput;
    if( req == NULL )
    {
        req = smem_new_struct( pix_sundog_textinput );
        vm->sd_textinput = req;
    }
    if( req )
    {
        smem_zero( req );
    
	req->name = name;
	req->def_str = def_str;

	sundog_event evt;
	smem_clear_struct( evt );
	evt.win = vm->win;
	evt.type = EVT_PIXICMD;
	evt.x = pix_sundog_req_textinput;
	if( send_events( &evt, 1, vm->wm ) == 0 )
	{
	    while( 1 )
	    {
    		if( req->handled ) break;
    		if( !vm->ready ) break;
    		stime_sleep( 100 );
	    }
	    stack[ sp2 ].i = pix_vm_make_container_from_cstring( (const char*)req->result, vm );
	    smem_free( (void*)req->result );
	}
    }
    
    if( def_str_ ) smem_free( def_str );
    if( name_ ) smem_free( name );
}

//
// Posix compatibility
//

//Issue a command:
void fn_system( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
#if !defined(OS_WINCE) && !defined(OS_IOS)
    PIX_CID name;
    pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	name_cont = vm->c[ name ];
	break;
    }
    
    //Execute:
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( err == 0 )
    {
	bool need_to_free = 0;
	char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );

	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)system( ts );

	if( need_to_free ) smem_free( ts );
    }
    else 
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = (PIX_INT)system( NULL );
    }
#else
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = 0;
#endif
}

void fn_argc( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = (PIX_INT)vm->wm->sd->argc;
}

void fn_argv( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    bool err = 1;
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    if( pars_num >= 1 )
    {
	PIX_INT arg_num;
	GET_VAL_FROM_STACK( arg_num, 0, PIX_INT );
	if( (unsigned)arg_num < vm->wm->sd->argc && vm->wm->sd->argv && vm->wm->sd->argv[ arg_num ] )
	{
	    int arg_len = (int)smem_strlen( vm->wm->sd->argv[ arg_num ] );
	    PIX_CID arg = pix_vm_new_container( -1, arg_len, 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
	    if( arg >= 0 )
	    {
		pix_vm_container* arg_cont = vm->c[ arg ];
		smem_copy( arg_cont->data, vm->wm->sd->argv[ arg_num ], arg_len );
		stack[ sp2 ].t = 0;
		stack[ sp2 ].i = arg;
		err = 0;
	    }
	}
    }
    if( err )
    {
	stack[ sp2 ].t = 0;
	stack[ sp2 ].i = -1;
    }
}

void fn_exit( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    if( pars_num >= 1 )
    {
	GET_VAL_FROM_STACK( vm->wm->sd->exit_code, 0, int );
    }
    th->active = 0;
    vm->wm->exit_request = 1;
}

//
// Experimental API
//

void fn_webserver_dialog( PIX_BUILTIN_FN_PARAMETERS )
{
    sundog_event evt;
    smem_clear_struct( evt );
    evt.win = vm->win;
    evt.type = EVT_PIXICMD;
    evt.x = pix_sundog_req_webserver;
    vm->sd_webserver_closed = 0;
    if( send_events( &evt, 1, vm->wm ) == 0 )
    {
	while( vm->sd_webserver_closed == 0 )
	{
	    if( !vm->ready ) break;
    	    stime_sleep( 100 );
	}
	vm->sd_webserver_closed = 0;
    }
}

void fn_midiopt_dialog( PIX_BUILTIN_FN_PARAMETERS )
{
    sundog_event evt;
    smem_clear_struct( evt );
    evt.win = vm->win;
    evt.type = EVT_PIXICMD;
    evt.x = pix_sundog_req_midiopt;
    send_events( &evt, 1, vm->wm );
}

void fn_system_copy_OR_open_url( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    PIX_CID name;
    //pix_vm_container* name_cont;
    
    bool err = 0;
    
    //Get parameters:
    while( 1 )
    {
	if( pars_num < 1 ) { err = 1; break; }
	GET_VAL_FROM_STACK( name, 0, PIX_CID );
	if( (unsigned)name >= (unsigned)vm->c_num ) { err = 1; break; }
	if( vm->c[ name ] == 0 ) { err = 1; break; }
	//name_cont = vm->c[ name ];
	break;
    }
    
    //Execute:
    if( err == 0 )
    {
	bool ts_ = 0;
	char* ts = pix_vm_make_cstring_from_container( name, &ts_, vm );
	if( ts )
	{
	    if( fn_num == FN_SYSTEM_COPY )
	    {
		char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
		if( full_path )
		{
		    sclipboard_copy( vm->wm->sd, ts, 0 );
		    smem_free( full_path );
		}
	    }
	    if( fn_num == FN_OPEN_URL )
		open_url( vm->wm->sd, ts );
	    if( ts_ ) smem_free( ts );
	}
    }
}

void fn_system_paste( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int type = 0;

    if( pars_num > 0 ) { GET_VAL_FROM_STACK( type, 0, int ); }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;

    char* fname = sclipboard_paste( vm->wm->sd, type, 0 );
    if( fname )
    {
        PIX_CID name = pix_vm_new_container( -1, smem_strlen( fname ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
        if( name >= 0 )
        {
            pix_vm_container* name_cont = vm->c[ name ];
            smem_copy( name_cont->data, fname, smem_strlen( fname ) );
            stack[ sp2 ].t = 0;
            stack[ sp2 ].i = name;
        }
        smem_free( fname );
    }
}

void fn_send_file_to( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    while( pars_num >= 1 )
    {
	PIX_CID file_path;

	GET_VAL_FROM_STACK( file_path, 0, PIX_CID );

	bool ts_ = 0;
	char* ts = pix_vm_make_cstring_from_container( file_path, &ts_, vm );
	if( ts )
	{
	    char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	    if( full_path )
	    {
		switch( fn_num )
		{
		    case FN_SEND_FILE_TO_GALLERY: rv = send_file_to_gallery( vm->wm->sd, full_path ); break;
		    default: break;
		}
                smem_free( full_path );
	    }
	    if( ts_ ) smem_free( ts );
	}

	break;
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_export_import_file( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    while( pars_num >= 1 )
    {
	PIX_CID file_path;
	uint32_t flags = 0; //EIFILE_xx

	GET_VAL_FROM_STACK( file_path, 0, PIX_CID );
	if( pars_num >= 2 ) GET_VAL_FROM_STACK( flags, 1, uint32_t );

	bool ts_ = 0;
	char* ts = pix_vm_make_cstring_from_container( file_path, &ts_, vm );
	char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	rv = export_import_file( vm->wm->sd, full_path, flags );
        smem_free( full_path );
	if( ts_ ) smem_free( ts );

	break;
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_set_audio_play_status( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int status;
    GET_VAL_FROM_STACK( status, 0, int );
#ifndef SUNDOG_MODULE
    g_snd_play_status = status;
#endif
}

void fn_get_audio_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = 0;
#ifndef SUNDOG_MODULE
    while( 1 )
    {
	if( g_snd_play_request )
	{
	    rv = 1;
	    g_snd_play_request = 0;
	    break;
	}
	if( g_snd_stop_request )
	{
	    rv = 2;
	    g_snd_stop_request = 0;
	    break;
	}
	if( g_snd_rewind_request )
	{
	    rv = 3;
	    g_snd_rewind_request = 0;
	    break;
	}
	break;
    }
#endif
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_openclose_app_state( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_INT rv = 0;

#ifdef SUNDOG_STATE

    int io = 0;
    GET_VAL_FROM_STACK( io, 0, int );

    if( fn_num == FN_OPEN_APP_STATE )
    {
	//Open:
	if( io == 0 )
	{
	    //Input:
	    sundog_state* state = sundog_state_get( vm->wm->sd, 0 );
	    if( state )
	    {
		vm->in_state = state;
		if( state->fname )
		{
		    vm->in_state_f = sfs_open( state->fname, "rb" );
		}
		else
		{
		    if( state->data )
		    {
			vm->in_state_f = sfs_open_in_memory( (int8_t*)state->data + state->data_offset, state->data_size );
		    }
		}
		rv = vm->in_state_f;
	    }
	}
	else
	{
	    //Output:
	    vm->out_state_f = sfs_open_in_memory( smem_new( 4 ), 4 );
	    rv = vm->out_state_f;
	}
    }
    else
    {
	//Close:
	if( io == 0 )
	{
	    //Input:
	    sundog_state* state = vm->in_state;
	    if( state )
	    {
		if( vm->in_state_f ) sfs_close( vm->in_state_f );
		if( state->flags & SUNDOG_STATE_TEMP )
            	    sfs_remove_file( state->fname );
		sundog_state_remove( state );
		vm->in_state = NULL;
		vm->in_state_f = 0;
	    }
	}
	else
	{
	    //Output:
	    sfs_file f = vm->out_state_f;
	    if( f )
	    {
		void* d = sfs_get_data( f );
		size_t dsize = sfs_tell( f );
		sfs_close( f );
		vm->out_state_f = 0;
		if( d )
		{
		    d = smem_resize( d, dsize );
		    size_t doffset = 0;
		    d = smem_get_stdc_ptr( d, &doffset );
		    sundog_state* state = sundog_state_new( d, doffset, dsize, 0 );
		    if( state )
		    {
			sundog_state_set( vm->wm->sd, 1, state );
		    }
		}
	    }
	}
    }

#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_wm_video_capture_start_OR_stop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    int rv = -1;

    pix_sundog_vcap* req = vm->sd_vcap;
    if( req == NULL )
    {
        req = smem_new_struct( pix_sundog_vcap );
        vm->sd_vcap = req;
    }
    if( req )
    {
        smem_zero( req );

	if( fn_num == FN_WM_VIDEO_CAPTURE_START )
	{
	    if( pars_num >= 1 ) GET_VAL_FROM_STACK( req->fps, 0, int ) else req->fps = 30;
	    if( pars_num >= 2 ) GET_VAL_FROM_STACK( req->bitrate_kb, 1, int ) else req->bitrate_kb = 1000;
	    if( pars_num >= 3 ) GET_VAL_FROM_STACK( req->flags, 2, int ) else req->flags = 0;
	}

	sundog_event evt;
	smem_clear_struct( evt );
	evt.win = vm->win;
	evt.type = EVT_PIXICMD;
	evt.x = pix_sundog_req_vcap;
	if( fn_num == FN_WM_VIDEO_CAPTURE_START )
	    evt.y = 0;
	else
	    evt.y = 1;
	if( send_events( &evt, 1, vm->wm ) == 0 )
	{
	    while( !req->handled )
	    {
	        if( !vm->ready ) break;
		    stime_sleep( 10 );
	    }
	    rv = req->err;
    	}
    }

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_wm_video_capture_get_ext( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = -1;

    const char* ext = video_capture_get_file_ext( vm->wm );
    if( ext )
    {
	PIX_CID str = pix_vm_new_container( -1, smem_strlen( ext ), 1, PIX_CONTAINER_TYPE_INT8, 0, vm );
    	if( str >= 0 )
    	{
    	    pix_vm_container* str_cont = vm->c[ str ];
    	    smem_copy( str_cont->data, ext, smem_strlen( ext ) );
    	    stack[ sp2 ].t = 0;
    	    stack[ sp2 ].i = str;
    	}
    }
}

void fn_wm_video_capture_encode( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    
    int rv = -1;
    
    while( pars_num >= 1 )
    {
	bool name_need_to_free = 0;
	char* name = 0;
	PIX_CID name_cont;
	
	GET_VAL_FROM_STACK( name_cont, 0, PIX_CID );
	name = pix_vm_make_cstring_from_container( name_cont, &name_need_to_free, vm );
	if( name == 0 ) break;

	video_capture_set_in_name( name, vm->wm );
        rv = video_capture_encode( vm->wm );

	if( name_need_to_free ) smem_free( name );
	
	break;
    }
    
    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

//
// SunVox
//

void fn_sv_new( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sample_rate = 0;
    uint32_t flags = 0;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( sample_rate, 0, int );
    if( pars_num >= 2 ) GET_VAL_FROM_STACK( flags, 1, uint32_t );
    rv = pix_vm_sv_new( sample_rate, flags, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_remove( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_remove( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_sample_rate( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_get_sample_rate( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_render( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = 0;

#ifndef PIX_NOSUNVOX
    int out_channels = 2;

    int sv_id = -1;
    PIX_CID out_cid = -1;
    int out_frames = -1;
    int out_latency = 0;
    stime_ticks_t out_time = 0;
    PIX_CID in_cid = -1;
    int in_channels = out_channels;
    while( 1 )
    {
	if( pars_num >= 1 ) { GET_VAL_FROM_STACK( sv_id, 0, int ); } else break;
	if( pars_num >= 2 ) { GET_VAL_FROM_STACK( out_cid, 1, PIX_CID ); } else break;
	if( pars_num >= 3 ) { GET_VAL_FROM_STACK( out_frames, 2, int ); } else break;
	if( pars_num >= 4 ) { GET_VAL_FROM_STACK( out_latency, 3, int ); } else break;
	if( pars_num >= 5 ) { GET_VAL_FROM_STACK( out_time, 4, stime_ticks_t ); } else break;
	if( pars_num >= 6 ) { GET_VAL_FROM_STACK( in_cid, 5, PIX_CID ); } else break;
	if( pars_num >= 7 ) { GET_VAL_FROM_STACK( in_channels, 6, int ); } else break;
	break;
    }

    pix_vm_container* out_cont = pix_vm_get_container( out_cid, vm );
    if( out_cont )
    {
	sunvox_render_data rdata;
	smem_clear_struct( rdata );

	if( out_cont->type >= PIX_CONTAINER_TYPE_FLOAT32 )
    	    rdata.buffer_type = sound_buffer_float32;
    	else
    	    rdata.buffer_type = sound_buffer_int16;

	int max_frames = out_cont->size * g_pix_container_type_sizes[ out_cont->type ] / ( g_sample_size[ rdata.buffer_type ] * out_channels );
	if( out_frames < 0 ) out_frames = max_frames;
	if( out_frames > max_frames ) out_frames = max_frames;

	if( out_time == 0 ) out_time = stime_ticks();

	rdata.buffer = out_cont->data;
        rdata.frames = out_frames;
	rdata.channels = out_channels;
        rdata.out_latency = out_latency;
        rdata.out_latency2 = out_latency;
        rdata.out_time = out_time;

	pix_vm_container* in_cont = pix_vm_get_container( in_cid, vm );
	if( in_cont )
	{
	    if( in_cont->type >= PIX_CONTAINER_TYPE_FLOAT32 )
    		rdata.in_type = sound_buffer_float32;
    	    else
    		rdata.in_type = sound_buffer_int16;

	    int max_in_frames = in_cont->size * g_pix_container_type_sizes[ in_cont->type ] / ( g_sample_size[ rdata.in_type ] * in_channels );
	    if( max_in_frames >= out_frames )
	    {
    		rdata.in_buffer = in_cont->data;
    		rdata.in_channels = in_channels;
    	    }
    	}

        rv = pix_vm_sv_render( sv_id, &rdata, vm );
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_lock_unlock( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    sunvox_stream_command cmd;
    if( fn_num == FN_SV_LOCK )
        cmd = SUNVOX_STREAM_LOCK;
    else
        cmd = SUNVOX_STREAM_UNLOCK;
    rv = pix_vm_sv_stream_control( sv_id, cmd, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_load_fload( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	sfs_file f = 0;
	bool close_f = false;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	if( fn_num == FN_SV_LOAD )
	{
	    PIX_CID name;
	    GET_VAL_FROM_STACK( name, 1, PIX_CID );

	    bool need_to_free = 0;
	    char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	    if( ts == 0 ) break;

	    char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	    if( full_path )
	    {
		f = sfs_open( full_path, "rb" );
		close_f = true;
		smem_free( full_path );
	    }

	    if( need_to_free ) smem_free( ts );
	}
	else
	{
	    GET_VAL_FROM_STACK( f, 1, sfs_file );
	}
	if( f )
	{
	    rv = pix_vm_sv_fload( sv_id, f, vm );
	    if( close_f ) sfs_close( f );
	}
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_save_fsave( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	sfs_file f = 0;
	bool close_f = false;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	if( fn_num == FN_SV_SAVE )
	{
	    PIX_CID name;
	    GET_VAL_FROM_STACK( name, 1, PIX_CID );

	    bool need_to_free = 0;
	    char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	    if( ts == 0 ) break;

	    char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	    if( full_path )
	    {
		f = sfs_open( full_path, "wb" );
		close_f = true;
		smem_free( full_path );
	    }

	    if( need_to_free ) smem_free( ts );
	}
	else
	{
	    GET_VAL_FROM_STACK( f, 1, sfs_file );
	}
	if( f )
	{
	    rv = pix_vm_sv_fsave( sv_id, f, vm );
	    if( close_f ) sfs_close( f );
	}
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_play( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    int pos = 0;
    bool jump_to_pos = false;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( sv_id, 0, int );
    if( pars_num >= 2 ) { GET_VAL_FROM_STACK( pos, 1, int ); jump_to_pos = true; }
    rv = pix_vm_sv_play( sv_id, pos, jump_to_pos, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_stop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_stop( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_pause( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_pause( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_resume( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_resume( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_sync_resume( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_sync_resume( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_autostop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	bool autostop;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( autostop, 1, bool );
	rv = pix_vm_sv_set_autostop( sv_id, autostop, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_autostop( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_get_autostop( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_status( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_get_status( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_rewind( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int pos;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( pos, 1, int );
	rv = pix_vm_sv_rewind( sv_id, pos, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_volume( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    int vol = -1;
    if( pars_num >= 1 ) GET_VAL_FROM_STACK( sv_id, 0, int );
    if( pars_num >= 2 ) GET_VAL_FROM_STACK( vol, 1, int );
    rv = pix_vm_sv_volume( sv_id, vol, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_event_t( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int set;
	int t = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( set, 1, int );
	if( pars_num >= 3 ) GET_VAL_FROM_STACK( t, 2, int );
	rv = pix_vm_sv_set_event_t( sv_id, set, t, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_send_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int track;
	int note;
	int vel = 0;
	int mod = -1;
	int ctl = 0;
	int ctl_val = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( track, 1, int );
	GET_VAL_FROM_STACK( note, 2, int );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( vel, 3, int );
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( mod, 4, int );
	if( pars_num >= 6 ) GET_VAL_FROM_STACK( ctl, 5, int );
	if( pars_num >= 7 ) GET_VAL_FROM_STACK( ctl_val, 6, int );
	rv = pix_vm_sv_send_event( sv_id, track, note, vel, mod, ctl, ctl_val, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_current_line( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = 0;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    if( fn_num == FN_SV_GET_CURRENT_LINE )
        rv = pix_vm_sv_get_current_line( sv_id, vm ) / 32;
    else
        rv = pix_vm_sv_get_current_line( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_current_signal_level( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = 0;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    int ch = 0;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    if( pars_num >= 2 )	GET_VAL_FROM_STACK( ch, 1, int );
    rv = pix_vm_sv_get_current_signal_level( sv_id, ch, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_name( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_make_container_from_cstring( pix_vm_sv_get_name( sv_id, vm ), vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_name( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    PIX_CID name_cid = -1;
    if( pars_num >= 2 )
    {
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( name_cid, 1, PIX_CID );
	bool name_str_ = false;
        char* name_str = pix_vm_make_cstring_from_container( name_cid, &name_str_, vm );
        rv = pix_vm_sv_set_name( sv_id, name_str, vm );
        if( name_str_ ) smem_free( name_str );
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_bpm( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    if( fn_num == FN_SV_GET_BPM )
	rv = pix_vm_sv_get_proj_par( sv_id, 0, vm );
    else
	rv = pix_vm_sv_get_proj_par( sv_id, 1, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_len( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    if( fn_num == FN_SV_GET_LEN_FRAMES )
	rv = pix_vm_sv_get_proj_len( sv_id, 0, vm );
    else
	rv = pix_vm_sv_get_proj_len( sv_id, 1, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_time_map( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 5 )
    {
	int sv_id;
	int start_line;
	int len;
	PIX_CID dest_cid;
	int flags;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( start_line, 1, int );
	GET_VAL_FROM_STACK( len, 2, int );
	GET_VAL_FROM_STACK( dest_cid, 3, PIX_CID );
	GET_VAL_FROM_STACK( flags, 4, int );
	pix_vm_container* dest_cont = pix_vm_get_container( dest_cid, vm );
	if( dest_cont && dest_cont->size * g_pix_container_type_sizes[ dest_cont->type ] >= len * sizeof(uint32_t) )
	{
	    rv = pix_vm_sv_get_time_map( sv_id, start_line, len, (uint32_t*)dest_cont->data, flags, vm );
	}
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_new_module( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    PIX_CID type_cid;
    PIX_CID name_cid;
    int x = 0;
    int y = 0;
    int z = 0;
    if( pars_num >= 2 )
    {
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( type_cid, 1, PIX_CID );
	if( pars_num >= 3 ) GET_VAL_FROM_STACK( name_cid, 2, PIX_CID ) else name_cid = type_cid;
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( x, 3, int );
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( y, 4, int );
	if( pars_num >= 6 ) GET_VAL_FROM_STACK( z, 5, int );
	bool type_str_ = false;
	bool name_str_ = false;
        char* type_str = pix_vm_make_cstring_from_container( type_cid, &type_str_, vm );
        char* name_str = pix_vm_make_cstring_from_container( name_cid, &name_str_, vm );
        rv = pix_vm_sv_new_module( sv_id, name_str, type_str, x, y, z, vm );
        if( type_str_ ) smem_free( type_str );
        if( name_str_ ) smem_free( name_str );
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_remove_module( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int mod;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	rv = pix_vm_sv_remove_module( sv_id, mod, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_connect_disconnect_module( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int src;
	int dst;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( src, 1, int );
	GET_VAL_FROM_STACK( dst, 2, int );
	rv = pix_vm_sv_connect_module( sv_id, src, dst, fn_num == FN_SV_DISCONNECT_MODULE, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_fload_module( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	sfs_file f = 0;
	bool close_f = false;
	int x = 0;
	int y = 0;
	int z = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	if( pars_num >= 3 ) GET_VAL_FROM_STACK( x, 2, int );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( y, 3, int );
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( z, 4, int );
	if( fn_num == FN_SV_LOAD_MODULE )
	{
	    PIX_CID name;
	    GET_VAL_FROM_STACK( name, 1, PIX_CID );

	    bool need_to_free = 0;
	    char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	    if( !ts ) break;

	    char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	    if( full_path )
	    {
		f = sfs_open( full_path, "rb" );
		close_f = true;
		smem_free( full_path );
	    }

	    if( need_to_free ) smem_free( ts );
	}
	else
	{
	    GET_VAL_FROM_STACK( f, 1, sfs_file );
	}
	if( f )
	{
	    rv = pix_vm_sv_fload_module( sv_id, f, x, y, z, vm );
	    if( close_f ) sfs_close( f );
	}
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_mod_fload( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int mod = -1;
	sfs_file f = 0;
	bool close_f = false;
	int slot = -1;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( slot, 3, int );
	int modtype = 0;
	bool with_filename = 0;
	switch( fn_num )
	{
	    case FN_SV_SAMPLER_LOAD: modtype = 0; with_filename = 1; break;
	    case FN_SV_SAMPLER_FLOAD: modtype = 0; break;
	    case FN_SV_METAMODULE_LOAD: modtype = 1; with_filename = 1; break;
	    case FN_SV_METAMODULE_FLOAD: modtype = 1; break;
	    case FN_SV_VPLAYER_LOAD: modtype = 2; with_filename = 1; break;
	    case FN_SV_VPLAYER_FLOAD: modtype = 2; break;
	};
	if( with_filename )
	{
	    PIX_CID name;
	    GET_VAL_FROM_STACK( name, 2, PIX_CID );

	    bool need_to_free = 0;
	    char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	    if( !ts ) break;

	    char* full_path = pix_compose_full_path( vm->base_path, ts, vm );
	    if( full_path )
	    {
		f = sfs_open( full_path, "rb" );
		close_f = true;
		smem_free( full_path );
	    }

	    if( need_to_free ) smem_free( ts );
	}
	else
	{
	    GET_VAL_FROM_STACK( f, 2, sfs_file );
	}
	if( f )
	{
	    rv = pix_vm_sv_mod_fload( sv_id, modtype, mod, slot, f, vm );
	    if( close_f ) sfs_close( f );
	}
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_sampler_par( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int mod = -1;
	int slot = -1;
	int par = -1;
	int par_val = 0;
	int set = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( slot, 2, int );
	GET_VAL_FROM_STACK( par, 3, int );
	GET_VAL_FROM_STACK( par_val, 4, int );
	GET_VAL_FROM_STACK( set, 5, int );
	rv = pix_vm_sv_sampler_par( sv_id, mod, slot, par, par_val, set, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_number_of_modules( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = 0;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_get_number_of_modules( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_selected_module( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = 0;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    int mod = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    if( pars_num >= 2 )	GET_VAL_FROM_STACK( mod, 1, int );
    rv = pix_vm_sv_selected_module( sv_id, mod, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_find_module( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	PIX_CID name;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( name, 1, PIX_CID );

	bool need_to_free = 0;
	char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	if( !ts ) break;

	rv = pix_vm_sv_find_module( sv_id, ts, vm );

	if( need_to_free ) smem_free( ts );

	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_flags( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int mod;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	rv = pix_vm_sv_get_module_flags( sv_id, mod, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_inputs( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int mod;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	int num = 0;
	int* links = pix_vm_sv_get_module_inouts( sv_id, mod, fn_num == FN_SV_GET_MODULE_OUTPUTS, &num, vm );
	if( num > 0 && links )
	{
	    rv = pix_vm_new_container( -1, num, 1, PIX_CONTAINER_TYPE_INT32, links, vm );
            pix_vm_set_container_flags( rv, pix_vm_get_container_flags( rv, vm ) | PIX_CONTAINER_FLAG_STATIC_DATA, vm );
	}
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_type( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int mod;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	rv = pix_vm_make_container_from_cstring( pix_vm_sv_get_module_type( sv_id, mod, vm ), vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_name( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int mod;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	rv = pix_vm_make_container_from_cstring( pix_vm_sv_get_module_name( sv_id, mod, vm ), vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_module_name( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int mod;
	PIX_CID name_cid;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( name_cid, 2, PIX_CID );
	bool name_str_ = false;
        char* name_str = pix_vm_make_cstring_from_container( name_cid, &name_str_, vm );
	rv = pix_vm_sv_set_module_name( sv_id, mod, name_str, vm );
        if( name_str_ ) smem_free( name_str );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_xy( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int mod;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	rv = pix_vm_sv_get_module_xy( sv_id, mod, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_module_xy( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 4 )
    {
	int sv_id;
	int mod;
	int x, y;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( x, 2, int );
	GET_VAL_FROM_STACK( y, 3, int );
	rv = pix_vm_sv_set_module_xy( sv_id, mod, x, y, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_color( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    COLORSIGNED rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int mod;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	rv = pix_vm_sv_get_module_color( sv_id, mod, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_module_color( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int mod;
	COLOR color;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( color, 2, COLOR );
	rv = pix_vm_sv_set_module_color( sv_id, mod, color, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_finetune( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int mod;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	rv = pix_vm_sv_get_module_finetune( sv_id, mod, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_module_finetune( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int mod;
	int finetune;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( finetune, 2, int );
	rv = pix_vm_sv_set_module_finetune( sv_id, mod, finetune, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_module_relnote( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int mod;
	int relative_note;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( relative_note, 2, int );
	rv = pix_vm_sv_set_module_relnote( sv_id, mod, relative_note, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_scope( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 4 )
    {
	int sv_id;
	int mod;
	int ch;
	PIX_CID dest_cid;
	int samples_to_read = -1;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( ch, 2, int );
	GET_VAL_FROM_STACK( dest_cid, 3, PIX_CID );
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( samples_to_read, 4, int );
	pix_vm_container* dest_cont = pix_vm_get_container( dest_cid, vm );
	if( dest_cont )
	{
	    rv = pix_vm_sv_get_module_scope( sv_id, mod, ch, dest_cont, samples_to_read, vm );
	}
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_module_curve( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 6 )
    {
	int sv_id;
	int mod;
	int curve_num;
	PIX_CID data_cid;
	int len;
	int w;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( curve_num, 2, int );
	GET_VAL_FROM_STACK( data_cid, 3, PIX_CID );
	GET_VAL_FROM_STACK( len, 4, int );
	GET_VAL_FROM_STACK( w, 5, int );
	pix_vm_container* data_cont = pix_vm_get_container( data_cid, vm );
	if( data_cont )
	{
	    rv = pix_vm_sv_module_curve( sv_id, mod, curve_num, data_cont, len, w, vm );
	}
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_ctl_cnt( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	int mod;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	rv = pix_vm_sv_get_module_ctl_cnt( sv_id, mod, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_ctl_name( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_CID rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int mod;
	int ctl;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( ctl, 2, int );
	rv = pix_vm_make_container_from_cstring( pix_vm_sv_get_module_ctl_name( sv_id, mod, ctl, vm ), vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_ctl_value( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int mod;
	int ctl;
	int scaled = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( ctl, 2, int );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( scaled, 3, int );
	rv = pix_vm_sv_get_module_ctl_value( sv_id, mod, ctl, scaled, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_module_ctl_value( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 4 )
    {
	int sv_id;
	int mod;
	int ctl;
	int val;
	int scaled = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( ctl, 2, int );
	GET_VAL_FROM_STACK( val, 3, int );
	if( pars_num >= 5 ) GET_VAL_FROM_STACK( scaled, 4, int );
	rv = pix_vm_sv_set_module_ctl_value( sv_id, mod, ctl, val, scaled, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_module_ctl_par( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = 0;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 3 )
    {
	int sv_id;
	int mod;
	int ctl;
	int scaled = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( mod, 1, int );
	GET_VAL_FROM_STACK( ctl, 2, int );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( scaled, 3, int );
	rv = pix_vm_sv_get_module_ctl_par( sv_id, mod, ctl, scaled, fn_num - FN_SV_GET_MODULE_CTL_MIN, vm );
	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_new_pat( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 4 )
    {
        int sv_id;
        int clone;
        int x, y;
        int tracks = 16;
        int lines = 64;
        static int icon_seed_cnt = 0;
        int icon_seed = icon_seed_cnt++;
        PIX_CID name_cid;
        GET_VAL_FROM_STACK( sv_id, 0, int );
        GET_VAL_FROM_STACK( clone, 1, int );
        GET_VAL_FROM_STACK( x, 2, int );
        GET_VAL_FROM_STACK( y, 3, int );
        if( pars_num >= 5 ) GET_VAL_FROM_STACK( tracks, 4, int );
        if( pars_num >= 6 ) GET_VAL_FROM_STACK( lines, 5, int );
        if( pars_num >= 7 ) GET_VAL_FROM_STACK( icon_seed, 6, int );
        if( pars_num >= 8 ) GET_VAL_FROM_STACK( name_cid, 7, PIX_CID );
        bool name_str_ = false;
        char* name_str = pix_vm_make_cstring_from_container( name_cid, &name_str_, vm );
        rv = pix_vm_sv_new_pat( sv_id, clone, x, y, tracks, lines, icon_seed, name_str, vm );
        if( name_str_ ) smem_free( name_str );
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_remove_pat( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 2 )
    {
	int sv_id = -1;
	int pat = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( pat, 1, int );
	rv = pix_vm_sv_remove_pat( sv_id, pat, vm );
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_number_of_pats( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    int sv_id = -1;
    if( pars_num >= 1 )	GET_VAL_FROM_STACK( sv_id, 0, int );
    rv = pix_vm_sv_get_number_of_pats( sv_id, vm );
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_find_pattern( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    int rv = -1;

#ifndef PIX_NOSUNVOX
    while( pars_num >= 2 )
    {
	int sv_id;
	PIX_CID name;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( name, 1, PIX_CID );

	bool need_to_free = 0;
	char* ts = pix_vm_make_cstring_from_container( name, &need_to_free, vm );
	if( !ts ) break;

	rv = pix_vm_sv_find_pattern( sv_id, ts, vm );

	if( need_to_free ) smem_free( ts );

	break;
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_pat( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 2 )
    {
	int sv_id = -1;
	int pat = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( pat, 1, int );
	sunvox_pattern* pat_data = NULL;
	sunvox_pattern_info* pat_info = NULL;
	int rv2 = pix_vm_sv_get_pat( sv_id, pat, &pat_data, &pat_info, vm );
	if( rv2 == 0 && pat_data && pat_info )
	{
	    switch( fn_num )
	    {
		case FN_SV_GET_PAT_X: rv = pat_info->x; break;
		case FN_SV_GET_PAT_Y: rv = pat_info->y; break;
		case FN_SV_GET_PAT_TRACKS: rv = pat_data->data_xsize; break;
		case FN_SV_GET_PAT_LINES: rv = pat_data->data_ysize; break;
		case FN_SV_GET_PAT_NAME: rv = pix_vm_make_container_from_cstring( pat_data->name, vm ); break;
		case FN_SV_GET_PAT_DATA:
	    	    rv = pix_vm_new_container( -1, pat_data->data_xsize * 8, pat_data->data_ysize, PIX_CONTAINER_TYPE_INT8, pat_data->data, vm );
        	    pix_vm_set_container_flags( rv, pix_vm_get_container_flags( rv, vm ) | PIX_CONTAINER_FLAG_STATIC_DATA, vm );
		    break;
		default: break;
	    }
	}
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_pat_xy( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 4 )
    {
	int sv_id = -1;
	int pat = 0;
	int x = 0;
	int y = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( pat, 1, int );
	GET_VAL_FROM_STACK( x, 2, int );
	GET_VAL_FROM_STACK( y, 3, int );
	rv = pix_vm_sv_set_pat_xy( sv_id, pat, x, y, vm );
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_pat_size( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 3 )
    {
	int sv_id = -1;
	int pat = 0;
	int tracks = -1;
	int lines = -1;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( pat, 1, int );
	GET_VAL_FROM_STACK( tracks, 2, int );
	if( pars_num >= 4 ) GET_VAL_FROM_STACK( lines, 3, int );
	rv = pix_vm_sv_set_pat_size( sv_id, pat, tracks, lines, vm );
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_pat_name( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 3 )
    {
        int sv_id;
        int pat;
        PIX_CID name_cid;
        GET_VAL_FROM_STACK( sv_id, 0, int );
        GET_VAL_FROM_STACK( pat, 1, int );
        GET_VAL_FROM_STACK( name_cid, 2, PIX_CID );
        bool name_str_ = false;
        char* name_str = pix_vm_make_cstring_from_container( name_cid, &name_str_, vm );
        rv = pix_vm_sv_set_pat_name( sv_id, pat, name_str, vm );
        if( name_str_ ) smem_free( name_str );
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_set_pat_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 5 )
    {
	int sv_id = -1;
	int pat = 0;
	int track = 0;
	int line = 0;
	int nn = -1;
	int vv = -1;
	int mm = -1;
	int ccee = -1;
	int xxyy = -1;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( pat, 1, int );
	GET_VAL_FROM_STACK( track, 2, int );
	GET_VAL_FROM_STACK( line, 3, int );
	GET_VAL_FROM_STACK( nn, 4, int );
	if( pars_num >= 6 ) GET_VAL_FROM_STACK( vv, 5, int );
	if( pars_num >= 7 ) GET_VAL_FROM_STACK( mm, 6, int );
	if( pars_num >= 8 ) GET_VAL_FROM_STACK( ccee, 7, int );
	if( pars_num >= 9 ) GET_VAL_FROM_STACK( xxyy, 8, int );
	sunvox_pattern* pat_data = NULL;
	int rv2 = pix_vm_sv_get_pat( sv_id, pat, &pat_data, NULL, vm );
	if( rv2 == 0 && pat_data )
	{
	    if( (unsigned)track < (unsigned)pat_data->channels && (unsigned)line < (unsigned)pat_data->lines )
	    {
		sunvox_note* p = &pat_data->data[ line * pat_data->data_xsize + track ];
		if( nn >= 0 ) p->note = nn;
		if( vv >= 0 ) p->vel = vv;
		if( mm >= 0 ) p->mod = mm;
		if( ccee >= 0 ) p->ctl = ccee;
		if( xxyy >= 0 ) p->ctl_val = xxyy;
		rv = 0;
	    }
	}
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_get_pat_event( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 5 )
    {
	int sv_id = -1;
	int pat = 0;
	int track = 0;
	int line = 0;
	int column = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( pat, 1, int );
	GET_VAL_FROM_STACK( track, 2, int );
	GET_VAL_FROM_STACK( line, 3, int );
	GET_VAL_FROM_STACK( column, 4, int );
	sunvox_pattern* pat_data = NULL;
	int rv2 = pix_vm_sv_get_pat( sv_id, pat, &pat_data, NULL, vm );
	if( rv2 == 0 && pat_data )
	{
	    if( (unsigned)track < (unsigned)pat_data->channels && (unsigned)line < (unsigned)pat_data->lines )
	    {
		sunvox_note* p = &pat_data->data[ line * pat_data->data_xsize + track ];
		switch( column )
		{
	            case 0: rv = p->note; break;
	            case 1: rv = p->vel; break;
	            case 2: rv = p->mod; break;
	            case 3: rv = p->ctl; break;
	            case 4: rv = p->ctl_val; break;
	        }
	    }
	}
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}

void fn_sv_pat_mute( PIX_BUILTIN_FN_PARAMETERS )
{
    FN_HEADER;
    PIX_INT rv = -1;

#ifndef PIX_NOSUNVOX
    if( pars_num >= 3 )
    {
	int sv_id = -1;
	int pat = 0;
	int mute = 0;
	GET_VAL_FROM_STACK( sv_id, 0, int );
	GET_VAL_FROM_STACK( pat, 1, int );
	GET_VAL_FROM_STACK( mute, 2, int );
	rv = pix_vm_sv_pat_mute( sv_id, pat, mute, vm );
    }
#endif

    PIX_SP sp2 = PIX_CHECK_SP( sp + ( pars_num - 1 ) );
    stack[ sp2 ].t = 0;
    stack[ sp2 ].i = rv;
}
