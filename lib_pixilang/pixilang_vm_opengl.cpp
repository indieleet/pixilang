/*
    pixilang_vm_opengl.cpp
    This file is part of the Pixilang.
    Copyright (C) 2015 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

//NOT THREAD SAFE!

#include "sundog.h"
#include "pixilang.h"

#ifdef OPENGL

static const char* g_gl_vshader_solid = "\
uniform mat4 g_wm_transform; \n\
uniform mat4 g_pixi_transform; \n\
IN vec4 position; // vertex position attribute \n\
void main() \n\
{ \n\
    gl_Position = g_wm_transform * g_pixi_transform * position; \n\
    gl_PointSize = 1.0; \n\
} \n\
";

static const char* g_gl_vshader_gradient = "\
uniform mat4 g_wm_transform; \n\
uniform mat4 g_pixi_transform; \n\
IN vec4 position; // vertex position attribute \n\
IN vec4 color; \n\
OUT vec4 color_var; \n\
void main() \n\
{ \n\
    gl_Position = g_wm_transform * g_pixi_transform * position; \n\
    gl_PointSize = 1.0; \n\
    color_var = color; \n\
} \n\
";

static const char* g_gl_vshader_tex_solid = "\
uniform mat4 g_wm_transform; \n\
uniform mat4 g_pixi_transform; \n\
IN vec4 position; // vertex position attribute \n\
IN vec2 tex_coord; // vertex texture coordinate attribute \n\
OUT vec2 tex_coord_var; // vertex texture coordinate varying \n\
void main() \n\
{ \n\
    gl_Position = g_wm_transform * g_pixi_transform * position; \n\
    gl_PointSize = 1.0; \n\
    tex_coord_var = tex_coord; // assign the texture coordinate attribute to its varying \n\
} \n\
";

static const char* g_gl_vshader_tex_gradient = "\
uniform mat4 g_wm_transform; \n\
uniform mat4 g_pixi_transform; \n\
IN vec4 position; // vertex position attribute \n\
IN vec2 tex_coord; // vertex texture coordinate attribute \n\
IN vec4 color; \n\
OUT vec2 tex_coord_var; // vertex texture coordinate varying \n\
OUT vec4 color_var; \n\
void main() \n\
{ \n\
    gl_Position = g_wm_transform * g_pixi_transform * position; \n\
    gl_PointSize = 1.0; \n\
    tex_coord_var = tex_coord; // assign the texture coordinate attribute to its varying \n\
    color_var = color; \n\
} \n\
";

static const char* g_gl_fshader_solid = "\
PRECISION( LOWP, float ) \n\
uniform vec4 g_color; \n\
void main() \n\
{ \n\
    gl_FragColor = g_color; \n\
} \n\
";

static const char* g_gl_fshader_gradient = "\
PRECISION( LOWP, float ) \n\
IN vec4 color_var; \n\
void main() \n\
{ \n\
    gl_FragColor = color_var; \n\
} \n\
";

static const char* g_gl_fshader_tex_alpha_solid = "\
PRECISION( MEDIUMP, float ) \n\
uniform sampler2D g_texture; \n\
uniform vec4 g_color; \n\
IN vec2 tex_coord_var; \n\
void main() \n\
{ \n\
    gl_FragColor = vec4( 1, 1, 1, texture2D( g_texture, tex_coord_var ).a ) * g_color; \n\
} \n\
";

static const char* g_gl_fshader_tex_alpha_gradient = "\
PRECISION( MEDIUMP, float ) \n\
uniform sampler2D g_texture; \n\
IN vec2 tex_coord_var; \n\
IN vec4 color_var; \n\
void main() \n\
{ \n\
    gl_FragColor = vec4( 1, 1, 1, texture2D( g_texture, tex_coord_var ).a ) * color_var; \n\
} \n\
";

static const char* g_gl_fshader_tex_rgba_solid = "\
PRECISION( MEDIUMP, float ) \n\
uniform sampler2D g_texture; \n\
uniform vec4 g_color; \n\
IN vec2 tex_coord_var; \n\
void main() \n\
{ \n\
    gl_FragColor = texture2D( g_texture, tex_coord_var ) * g_color; \n\
} \n\
";

static const char* g_gl_fshader_tex_rgba_gradient = "\
PRECISION( MEDIUMP, float ) \n\
uniform sampler2D g_texture; \n\
IN vec2 tex_coord_var; \n\
IN vec4 color_var; \n\
void main() \n\
{ \n\
    gl_FragColor = texture2D( g_texture, tex_coord_var ) * color_var; \n\
} \n\
";

//Call this function only in the thread with OpenGL context!
//(inside the gl_lock()...gl_unlock() block)
static void pix_vm_empty_gl_trash( pix_vm* vm )
{
    if( !vm->wm->gl_initialized ) return;
    if( vm->gl_unused_textures_count > 0 || vm->gl_unused_framebuffers_count > 0 || vm->gl_unused_progs_count > 0 )
    {
	smutex_lock( &vm->gl_unused_data_mutex );
	if( vm->gl_unused_textures_count > 0 )
	{
	    if( vm->gl_unused_textures )
	    {
		GL_DELETE_TEXTURES( vm->wm, vm->gl_unused_textures_count, vm->gl_unused_textures );
		vm->gl_unused_textures_count = 0;
	    }
	}
	if( vm->gl_unused_framebuffers_count > 0 )
	{
	    if( vm->gl_unused_framebuffers )
	    {
		glDeleteFramebuffers( vm->gl_unused_framebuffers_count, vm->gl_unused_framebuffers );
		vm->gl_unused_framebuffers_count = 0;
	    }
	}
	if( vm->gl_unused_progs_count > 0 )
	{
	    if( vm->gl_unused_progs )
	    {
		for( int i = 0; i < vm->gl_unused_progs_count; i++ )
		{
		    gl_program_remove( vm->gl_unused_progs[ i ] );
		}
		vm->gl_unused_progs_count = 0;
	    }
	}
	smutex_unlock( &vm->gl_unused_data_mutex );
    }
}

int pix_vm_gl_init( pix_vm* vm )
{
    int rv = -1;
    if( !vm->wm->gl_initialized ) return -1;
    gl_lock( vm->wm );
    while( 1 )
    {
        smutex_init( &vm->gl_unused_data_mutex, 0 );

        gl_program_struct* p;

        vm->gl_transform_counter = 0;
        vm->gl_current_prog = 0;

        vm->gl_vshader_solid = gl_make_shader( g_gl_vshader_solid, GL_VERTEX_SHADER );
        if( vm->gl_vshader_solid == 0 ) break;

        vm->gl_vshader_gradient = gl_make_shader( g_gl_vshader_gradient, GL_VERTEX_SHADER );
        if( vm->gl_vshader_gradient == 0 ) break;

        vm->gl_vshader_tex_solid = gl_make_shader( g_gl_vshader_tex_solid, GL_VERTEX_SHADER );
        if( vm->gl_vshader_tex_solid == 0 ) break;

        vm->gl_vshader_tex_gradient = gl_make_shader( g_gl_vshader_tex_gradient, GL_VERTEX_SHADER );
        if( vm->gl_vshader_tex_gradient == 0 ) break;
        
        vm->gl_fshader_solid = gl_make_shader( g_gl_fshader_solid, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_solid == 0 ) break;

        vm->gl_fshader_gradient = gl_make_shader( g_gl_fshader_gradient, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_gradient == 0 ) break;
        
        vm->gl_fshader_tex_alpha_solid = gl_make_shader( g_gl_fshader_tex_alpha_solid, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_tex_alpha_solid == 0 ) break;
        
        vm->gl_fshader_tex_alpha_gradient = gl_make_shader( g_gl_fshader_tex_alpha_gradient, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_tex_alpha_gradient == 0 ) break;
        
        vm->gl_fshader_tex_rgba_solid = gl_make_shader( g_gl_fshader_tex_rgba_solid, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_tex_rgba_solid == 0 ) break;
     
        vm->gl_fshader_tex_rgba_gradient = gl_make_shader( g_gl_fshader_tex_rgba_gradient, GL_FRAGMENT_SHADER );
        if( vm->gl_fshader_tex_rgba_gradient == 0 ) break;

        vm->gl_prog_solid = gl_program_new( vm->gl_vshader_solid, vm->gl_fshader_solid );
        p = vm->gl_prog_solid; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_COLOR, "g_color" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );

        vm->gl_prog_gradient = gl_program_new( vm->gl_vshader_gradient, vm->gl_fshader_gradient );
        p = vm->gl_prog_gradient; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_COLOR, "color" );

        vm->gl_prog_tex_alpha_solid = gl_program_new( vm->gl_vshader_tex_solid, vm->gl_fshader_tex_alpha_solid );
        p = vm->gl_prog_tex_alpha_solid; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_COLOR, "g_color" );
        gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );

        vm->gl_prog_tex_alpha_gradient = gl_program_new( vm->gl_vshader_tex_gradient, vm->gl_fshader_tex_alpha_gradient );
        p = vm->gl_prog_tex_alpha_gradient; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );
        gl_init_attribute( p, GL_PROG_ATT_COLOR, "color" );

        vm->gl_prog_tex_rgba_solid = gl_program_new( vm->gl_vshader_tex_solid, vm->gl_fshader_tex_rgba_solid );
        p = vm->gl_prog_tex_rgba_solid; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_COLOR, "g_color" );
        gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );

        vm->gl_prog_tex_rgba_gradient = gl_program_new( vm->gl_vshader_tex_gradient, vm->gl_fshader_tex_rgba_gradient );
        p = vm->gl_prog_tex_rgba_gradient; if( p == 0 ) break;
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
        gl_init_uniform( p, GL_PROG_UNI_TEXTURE, "g_texture" );
        gl_init_attribute( p, GL_PROG_ATT_POSITION, "position" );
        gl_init_attribute( p, GL_PROG_ATT_TEX_COORD, "tex_coord" );
        gl_init_attribute( p, GL_PROG_ATT_COLOR, "color" );

        rv = 0;
        break;
    }
    gl_unlock( vm->wm );
    return rv;
}

void pix_vm_gl_deinit( pix_vm* vm )
{
    if( !vm->wm->gl_initialized ) return;
    
    gl_lock( vm->wm );

    pix_vm_empty_gl_trash( vm );
    glDeleteShader( vm->gl_vshader_solid );
    glDeleteShader( vm->gl_vshader_gradient );
    glDeleteShader( vm->gl_vshader_tex_solid );
    glDeleteShader( vm->gl_vshader_tex_gradient );
    glDeleteShader( vm->gl_fshader_solid );
    glDeleteShader( vm->gl_fshader_gradient );
    glDeleteShader( vm->gl_fshader_tex_alpha_solid );
    glDeleteShader( vm->gl_fshader_tex_alpha_gradient );
    glDeleteShader( vm->gl_fshader_tex_rgba_solid );
    glDeleteShader( vm->gl_fshader_tex_rgba_gradient );
    gl_program_remove( vm->gl_prog_solid );
    gl_program_remove( vm->gl_prog_gradient );
    gl_program_remove( vm->gl_prog_tex_alpha_solid );
    gl_program_remove( vm->gl_prog_tex_alpha_gradient );
    gl_program_remove( vm->gl_prog_tex_rgba_solid );
    gl_program_remove( vm->gl_prog_tex_rgba_gradient );

    gl_unlock( vm->wm );

    smem_free( vm->gl_unused_textures );
    smem_free( vm->gl_unused_framebuffers );
    smem_free( vm->gl_unused_progs );
    smutex_destroy( &vm->gl_unused_data_mutex );
}

void pix_vm_gl_matrix_set( pix_vm* vm )
{
    if( vm->screen == PIX_GL_SCREEN )
    {
	if( !vm->wm->gl_initialized ) return;
        pix_vm_gl_program_reset( vm );
    }
}

void pix_vm_gl_program_reset( pix_vm* vm )
{
    if( !vm->wm->gl_initialized ) return;
    if( vm->gl_current_prog )
    {
	gl_enable_attributes( vm->gl_current_prog, 0 );
	vm->gl_current_prog = NULL;
    }
    vm->gl_transform_counter++;
}

void pix_vm_gl_use_prog( gl_program_struct* p, pix_vm* vm )
{
    if( !vm->wm->gl_initialized ) return;
    if( vm->gl_current_prog ) gl_enable_attributes( vm->gl_current_prog, 0 ); //Disable attributes of the previous program
    glUseProgram( p->program );
    vm->gl_current_prog = p;
    if( p->transform_counter != vm->gl_transform_counter )
    {
        p->transform_counter = vm->gl_transform_counter;
        glUniformMatrix4fv( p->uniforms[ GL_PROG_UNI_TRANSFORM1 ], 1, 0, vm->gl_wm_transform );
        float mm[ 4 * 4 ];
        float* m;
        if( vm->t_enabled )
        {
            PIX_FLOAT* src = vm->t_matrix + ( vm->t_matrix_sp * 16 );
            if( sizeof( PIX_FLOAT ) == sizeof( float ) )
            {
                m = (float*)src;
            }
            else
            {
                for( int i = 0; i < 4 * 4; i++ ) mm[ i ] = src[ i ];
                m = mm;
            }
        }
        else
        {
            matrix_4x4_reset( mm );
            m = mm;
        }
        glUniformMatrix4fv( p->uniforms[ GL_PROG_UNI_TRANSFORM2 ], 1, 0, m );
    }
}

pix_vm_container_gl_data* pix_vm_get_container_gl_data( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
	if( c && c->opt_data && c->opt_data->gl )
	{
	    return c->opt_data->gl;
	}
    }

    return 0;
}

static void clear_fbox( uint8_t* img, int img_xsize, int pixel_size, int x, int y, int xsize, int ysize )
{
    img_xsize *= pixel_size;
    x *= pixel_size;
    xsize *= pixel_size;
    uint8_t* ptr = img + y * img_xsize + x;
    for( int y = 0; y < ysize; y++ )
    {
	smem_clear( ptr, xsize );
	ptr += img_xsize;
    }
}

static void pix_vm_get_gl_shader_uniforms( PIX_CID cnum, GLuint prog, const char* src, pix_vm* vm )
{
    //uniform xx xx NAME [ xx ] ;
    char ts[ 1024 ];
    while( 1 )
    {
	//Begin:
	const char* name_begin = smem_strstr( src, "uniform " );
	if( name_begin == 0 ) break;
	name_begin += 8;
	
	//End:
	const char* name_end = smem_strstr( name_begin, ";" );
	if( name_end == 0 ) break;
	src = name_end;
	
	//Skip array [] chars:
	for( const char* p = name_begin; p < name_end; p++ )
	{
	    if( *p == '[' )
	    {
		name_end = p;
		break;
	    }
	}
	
	//Cut spaces from the end:
	while( 1 )
	{
	    name_end--;
	    if( *name_end != ' ' ) break;
	}

	//Get the beginning of the name:
	name_begin = name_end;
	while( 1 )
	{
	    name_begin--;
	    if( *name_begin == ' ' ) break;
	}
	name_begin++;

	if( name_end < name_begin ) continue;
	size_t len = name_end - name_begin + 1;
	if( len > 1023 ) len = 1023;

	smem_copy( ts, name_begin, len );
	ts[ len ] = 0;
	PIX_VAL v;
	v.i = glGetUniformLocation( prog, ts ) + 1;
	pix_vm_set_container_property( cnum, ts, -1, 0, v, vm );
    }
}

pix_vm_container_gl_data* pix_vm_create_container_gl_data( PIX_CID cnum, pix_vm* vm )
{
    if( !vm->wm->gl_initialized ) return NULL;
    
    pix_vm_container_gl_data* rv = NULL;

    if( (unsigned)cnum >= (unsigned)vm->c_num ) return NULL;
    pix_vm_container* c = vm->c[ cnum ];
    if( c == NULL ) return NULL;

    if( c->opt_data == NULL )
    {
	c->opt_data = (pix_vm_container_opt_data*)smem_new( sizeof( pix_vm_container_opt_data ) );
	smem_zero( c->opt_data );
    }
    if( c->opt_data == NULL ) return NULL;

    pix_vm_container_gl_data* gl = c->opt_data->gl;
    bool update = false;
    if( gl )
    {
	//Already created:
	if( gl->flags & PIX_GL_DATA_FLAG_UPDATE_REQ )
	{
	    update = true;
	    gl->flags &= ~PIX_GL_DATA_FLAG_UPDATE_REQ;
	}
	else
	{
	    return gl;
	}
    }
    else
    {
	c->opt_data->gl = (pix_vm_container_gl_data*)smem_new( sizeof( pix_vm_container_gl_data ) );
	gl = c->opt_data->gl;
	smem_zero( gl );
    }

    //Remove all unused OpenGL objects:
    pix_vm_empty_gl_trash( vm );
    
    while( 1 )
    {
        if( c->flags & PIX_CONTAINER_FLAG_GL_PROG )
        {
    	    //GLSL program:
	    GLuint vshader = 0;
	    GLuint fshader = 0;
	    bool vshader_default = false;
	    bool fshader_default = false;
	    while( 1 )
	    {
	        if( c->size < 4 )
	        {
	    	    slog( "GLSL program error: wrong container data size %d x %d. Must be at least 4\n", (int)c->xsize, (int)c->ysize );
		    break;
		}

    		const char* vshader_str = (const char*)c->data;
		const char* fshader_str = (const char*)c->data + smem_strlen( vshader_str ) + 1;

		if( vshader_str[ 0 ] >= '0' && vshader_str[ 0 ] <= '9' )
		{
		    //Default shader:
		    vshader_default = true;
		    switch( vshader_str[ 0 ] - '0' )
		    {
		        case GL_SHADER_SOLID: vshader_str = g_gl_vshader_solid; break;
		        case GL_SHADER_GRAD: vshader_str = g_gl_vshader_gradient; break;
		        case GL_SHADER_TEX_ALPHA_SOLID: vshader_str = g_gl_vshader_tex_solid; break;
		        case GL_SHADER_TEX_ALPHA_GRAD: vshader_str = g_gl_vshader_tex_gradient; break;
		        case GL_SHADER_TEX_RGBA_SOLID: vshader_str = g_gl_vshader_tex_solid; break;
		        case GL_SHADER_TEX_RGBA_GRAD: vshader_str = g_gl_vshader_tex_gradient; break;
		        default: break;
		    }
		}
		vshader = gl_make_shader( vshader_str, GL_VERTEX_SHADER );
		if( vshader == 0 ) break;

		if( fshader_str[ 0 ] >= '0' && fshader_str[ 0 ] <= '9' )
		{
		    //Default shader:
		    fshader_default = true;
		    switch( fshader_str[ 0 ] - '0' )
		    {
		        case GL_SHADER_SOLID: fshader_str = g_gl_fshader_solid; break;
		        case GL_SHADER_GRAD: fshader_str = g_gl_fshader_gradient; break;
		        case GL_SHADER_TEX_ALPHA_SOLID: fshader_str = g_gl_fshader_tex_alpha_solid; break;
		        case GL_SHADER_TEX_ALPHA_GRAD: fshader_str = g_gl_fshader_tex_alpha_gradient; break;
		        case GL_SHADER_TEX_RGBA_SOLID: fshader_str = g_gl_fshader_tex_alpha_solid; break;
		        case GL_SHADER_TEX_RGBA_GRAD: fshader_str = g_gl_fshader_tex_alpha_gradient; break;
		        default: break;
		    }
		}
		fshader = gl_make_shader( fshader_str, GL_FRAGMENT_SHADER );
		if( fshader == 0 ) break;

        	gl->prog = gl_program_new( vshader, fshader );
    		if( gl->prog == 0 ) break;
		gl_init_uniform( gl->prog, GL_PROG_UNI_TRANSFORM1, "g_wm_transform" );
		gl_init_uniform( gl->prog, GL_PROG_UNI_TRANSFORM2, "g_pixi_transform" );
		gl_init_uniform( gl->prog, GL_PROG_UNI_TEXTURE, "g_texture" );
    		gl_init_uniform( gl->prog, GL_PROG_UNI_COLOR, "g_color" );
		gl_init_attribute( gl->prog, GL_PROG_ATT_POSITION, "position" );
		gl_init_attribute( gl->prog, GL_PROG_ATT_TEX_COORD, "tex_coord" );
		gl_init_attribute( gl->prog, GL_PROG_ATT_COLOR, "color" );

		pix_vm_get_gl_shader_uniforms( cnum, gl->prog->program, vshader_str, vm );
		pix_vm_get_gl_shader_uniforms( cnum, gl->prog->program, fshader_str, vm );

		rv = gl;
		break;
	    }
	    glDeleteShader( vshader );
	    glDeleteShader( fshader );
	    break;
	} //c->flags & PIX_CONTAINER_FLAG_GL_PROG

        //Texture:

        int texture_size_larger = 0;
	if( ( c->flags & PIX_CONTAINER_FLAG_GL_NPOT )
#ifdef OS_IOS
            && ( c->flags & PIX_CONTAINER_FLAG_GL_NO_XREPEAT ) && ( c->flags & PIX_CONTAINER_FLAG_GL_NO_YREPEAT )
            //limitation of GLES 2.0
            //(but on iOS devices only?)
#endif
           )
        {
    	    gl->xsize = c->xsize;
	    gl->ysize = c->ysize;
	}
	else
	{
    	    gl->xsize = round_to_power_of_two( c->xsize );
	    gl->ysize = round_to_power_of_two( c->ysize );
	}
        if( gl->xsize != c->xsize )
    	    texture_size_larger |= 1;
	if( gl->ysize != c->ysize )
	    texture_size_larger |= 2;

	uint8_t* temp_pixels = NULL;
	uint8_t* pixels = (uint8_t*)c->data;
	int pixels_xsize = c->xsize;
	int pixels_ysize = c->ysize;
	int base_pixel_size = g_pix_container_type_sizes[ c->type ];
	int pixel_size = base_pixel_size;
	bool alpha_ch = false;
	uint8_t* alpha_data = NULL;
	if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) || pix_vm_get_container_alpha( cnum, vm ) >= 0 )
	{
	    alpha_ch = true;
	    alpha_data = (uint8_t*)pix_vm_get_container_alpha_data( cnum, vm );
	    pixel_size = 4;
	    if( base_pixel_size == 2 )
	    {
	        if( !( c->flags & PIX_CONTAINER_FLAG_GL_NICEST ) )
		    pixel_size = 2;
	    }
	}

	if( alpha_ch || ( texture_size_larger && !update ) )
	{
	    int dest_xsize = gl->xsize;
	    int dest_ysize = gl->ysize;
	    if( update )
	    {
		dest_xsize = c->xsize;
		dest_ysize = c->ysize;
	    }
	    temp_pixels = (uint8_t*)smem_new( dest_xsize * dest_ysize * pixel_size );
	    if( texture_size_larger && !update )
	    {
	        //Make empty border:
	        if( texture_size_larger & 1 )
	        {
	    	    clear_fbox( temp_pixels, gl->xsize, pixel_size, c->xsize, 0, 1, c->ysize );
		    if( c->xsize + 1 < gl->xsize )
		        clear_fbox( temp_pixels, gl->xsize, pixel_size, gl->xsize - 1, 0, 1, gl->ysize );
		}
		if( texture_size_larger & 2 )
		{
		    clear_fbox( temp_pixels, gl->xsize, pixel_size, 0, c->ysize, c->xsize, 1 );
		    if( c->ysize + 1 < gl->ysize )
		        clear_fbox( temp_pixels, gl->xsize, pixel_size, 0, gl->ysize - 1, gl->xsize, 1 );
		}
		if( texture_size_larger == 3 )
		    clear_fbox( temp_pixels, gl->xsize, pixel_size, c->xsize, c->ysize, 1, 1 );
	    }
	    for( int y = 0; y < c->ysize; y++ )
	    {
		switch( base_pixel_size )
		{
		    case 1:
		    smem_copy( temp_pixels + y * dest_xsize, pixels + y * c->xsize, c->xsize );
		    break;
		    
		    case 2: {
		    uint16_t* src = (uint16_t*)( pixels + y * c->xsize * 2 );
		    uint8_t* alpha_src = 0;
		    if( alpha_data )
		        alpha_src = alpha_data + y * c->xsize;
		    switch( pixel_size )
		    {
		        case 2:
			//From 16bit to 16bit:
			if( alpha_ch )
			{
			    uint16_t* dest = (uint16_t*)( temp_pixels + y * dest_xsize * 2 );
			    if( alpha_src )
			    for( int x = 0; x < c->xsize; x++ )
			    {
			        //to RGBA 4444
			        uint16_t p = *src;
			        uint8_t r = ( ( p >> 11 ) << 3 ) & 0xF8; r >>= 4;
			        uint8_t g = ( ( p >> 5 ) << 2 ) & 0xFC; g >>= 4;
			        uint8_t b = ( p << 3 ) & 0xF8; b >>= 4;
			        if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
				    *dest = ( r << 12 ) | ( g << 8 ) | ( b << 4 );
				else
				    *dest = ( r << 12 ) | ( g << 8 ) | ( b << 4 ) | ( *alpha_src >> 4 );
				alpha_src++;
				src++;
				dest++;
			    }
			    else
			    for( int x = 0; x < c->xsize; x++ )
			    {
			        //to RGBA 5551
			        uint16_t p = *src;
			        uint8_t r = ( ( p >> 11 ) << 3 ) & 0xF8; r >>= 3;
			        uint8_t g = ( ( p >> 5 ) << 2 ) & 0xFC; g >>= 3;
			        uint8_t b = ( p << 3 ) & 0xF8; b >>= 3;
			        if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
			    	    *dest = ( r << 11 ) | ( g << 6 ) | ( b << 1 );
				else
				    *dest = ( r << 11 ) | ( g << 6 ) | ( b << 1 ) | 1;
				alpha_src++;
				src++;
				dest++;
			    }
			}
			else
			{
			    smem_copy( temp_pixels + y * dest_xsize * 2, src, c->xsize * 2 );
			}
			break;

			case 4:
			//From 16bit to 32bit:
			{
			    uint32_t* dest = (uint32_t*)( temp_pixels + y * dest_xsize * 4 );
			    if( alpha_src )
			    for( int x = 0; x < c->xsize; x++ )
			    {
			        uint16_t p = *src;
			        uint8_t r = ( ( p >> 11 ) << 3 ) & 0xF8; if( r ) r |= 7;
			        uint8_t g = ( ( p >> 5 ) << 2 ) & 0xFC; if( g ) g |= 3;
			        uint8_t b = ( p << 3 ) & 0xF8; if( b ) b |= 7;
			        if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
				    *dest = r | ( g << 8 ) | ( b << 16 );
				else
				    *dest = r | ( g << 8 ) | ( b << 16 ) | ( *alpha_src << 24 );
				alpha_src++;
				src++;
				dest++;
			    }
			    else
			    for( int x = 0; x < c->xsize; x++ )
			    {
			        uint16_t p = *src;
			        uint8_t r = ( ( p >> 11 ) << 3 ) & 0xF8; if( r ) r |= 7;
			        uint8_t g = ( ( p >> 5 ) << 2 ) & 0xFC; if( g ) g |= 3;
			        uint8_t b = ( p << 3 ) & 0xF8; if( b ) b |= 7;
			        if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
				    *dest = r | ( g << 8 ) | ( b << 16 );
				else
				    *dest = r | ( g << 8 ) | ( b << 16 ) | ( 255 << 24 );
				src++;
				dest++;
			    }
			}
			break;
			
			default: break;
		    } //switch( pixel_size )
		    } break;
		    
		    case 4: {
		    uint32_t* src = (uint32_t*)( pixels + y * c->xsize * 4 );
		    switch( pixel_size )
		    {
		        case 4:
		    	//From 32bit to 32bit:
			if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) || alpha_ch )
			{
			    uint8_t* alpha_src = 0;
			    if( alpha_data )
				alpha_src = alpha_data + y * c->xsize;
			    uint32_t* dest = (uint32_t*)( temp_pixels + y * dest_xsize * 4 );
			    if( alpha_src )
			    for( int x = 0; x < c->xsize; x++ )
			    {
			        uint32_t p = *src;
			        if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
				    *dest = p & 0x00FFFFFF;
				else
				    *dest = ( p & 0x00FFFFFF ) | ( *alpha_src << 24 );
				alpha_src++;
				src++;
				dest++;
			    }
			    else
			    for( int x = 0; x < c->xsize; x++ )
			    {
			        uint32_t p = *src;
			        if( ( c->flags & PIX_CONTAINER_FLAG_USES_KEY ) && (unsigned)c->key == p )
				    *dest = p & 0x00FFFFFF;
				else
				    *dest = ( p & 0x00FFFFFF ) | ( 255 << 24 );
				src++;
				dest++;
			    }
			}
			else
			{
			    smem_copy( temp_pixels + y * dest_xsize * 4, src, c->xsize * 4 );
			}
			break;

			default: break;
		    } //switch( pixel_size )
		    } break;

		    default: break;
		} //switch( base_pixel_size )
	    } //for( int y = 0...
	    pixels = temp_pixels;
	    pixels_xsize = dest_xsize;
	    pixels_ysize = dest_ysize;
	} //if( alpha_ch || texture_size_larger...

	if( !update ) glGenTextures( 1, &gl->texture_id );
    	GL_BIND_TEXTURE( vm->wm, gl->texture_id );

	int internal_format;
	int format;
	int type;
	bool no_texture = false;
	switch( pixel_size )
	{
	    case 1:
	        if( c->flags & PIX_CONTAINER_FLAG_GL_NO_ALPHA )
	        {
	    	    internal_format = GL_LUMINANCE;
		    format = GL_LUMINANCE;
		}
		else
		{
		    internal_format = GL_ALPHA;
		    format = GL_ALPHA;
		}
		type = GL_UNSIGNED_BYTE;
		break;
	    case 2:
	        internal_format = GL_RGB;
	        format = GL_RGB;
	        type = GL_UNSIGNED_SHORT_5_6_5;
	        if( alpha_ch )
	        {
	    	    internal_format = GL_RGBA;
		    format = GL_RGBA;
		    if( alpha_data )
		        type = GL_UNSIGNED_SHORT_4_4_4_4;
		    else
		        type = GL_UNSIGNED_SHORT_5_5_5_1;
		}
		else
		{
		    internal_format = GL_RGB;
		    format = GL_RGB;
		    type = GL_UNSIGNED_SHORT_5_6_5;
		}
		break;
	    case 4:
		if( alpha_ch )
		    internal_format = GL_RGBA;
		else
		    internal_format = GL_RGB;
		    format = GL_RGBA;
		    type = GL_UNSIGNED_BYTE;
		    break;
	    default:
		no_texture = true;
		break;
	}
	if( !no_texture )
	{
	    if( c->flags & PIX_CONTAINER_FLAG_GL_FRAMEBUFFER )
	    {
	        if( c->flags & PIX_CONTAINER_FLAG_GL_NO_ALPHA )
	        {
	    	    internal_format = GL_RGB;
	    	    format = GL_RGB;
		}
		else
		{
		    internal_format = GL_RGBA;
		    format = GL_RGBA;
		}
		pixels = NULL;
	    }
	    else
	    {
#ifdef OPENGLES
		if( !update )
		{
	    	    if( internal_format == GL_RGB && format == GL_RGBA )
	    	        gl->flags |= PIX_GL_DATA_FLAG_ALPHA_FF;
	    	}
#endif
	        if( gl->flags & PIX_GL_DATA_FLAG_ALPHA_FF )
	        {
	    	    //Prepare texture data (fill unused alpha values by FF):
	    	    internal_format = GL_RGBA;
		    uint32_t* p = (uint32_t*)pixels;
		    int xsize = c->xsize + 1; if( xsize > pixels_xsize ) xsize = pixels_xsize;
		    int ysize = c->ysize + 1; if( ysize > pixels_ysize ) ysize = pixels_ysize;
		    for( int y = 0; y < ysize; y++ )
		    {
		        for( int x = 0; x < xsize; x++ )
		        {
		    	    (*p) |= 0xFF000000;
			    p++;
			}
			p += pixels_xsize - xsize;
		    }
		    if( xsize < pixels_xsize )
		    {
		        p = (uint32_t*)pixels;
		        p += pixels_xsize - 1;
		        for( int y = 0; y < ysize; y++ )
		        {
		    	    (*p) |= 0xFF000000;
			    p += pixels_xsize;
			}
		    }
		    if( ysize < pixels_ysize )
		    {
		        p = (uint32_t*)pixels;
		        p += pixels_xsize * ( pixels_ysize - 1 );
		        for( int x = 0; x < xsize; x++ )
		        {
		    	    (*p) |= 0xFF000000;
			    p++;
			}
		    }
		}
	    }
	    if( update )
	    {
        	glTexSubImage2D( GL_TEXTURE_2D, 0, 0, 0, pixels_xsize, pixels_ysize, format, type, pixels );
	    }
	    else
	    {
		gl->texture_format = format;
    		glTexImage2D(
        	    GL_TEXTURE_2D,
            	    0,
		    internal_format,
            	    gl->xsize, gl->ysize,
		    0,
            	    format,
		    type,
            	    pixels );
        	GLenum glerr = glGetError();
        	if( glerr != GL_NO_ERROR )
        	{
            	    if( internal_format == GL_RGB && format == GL_RGBA )
            	    {
	    		internal_format = GL_RGBA;
			uint32_t* p = (uint32_t*)pixels;
			for( size_t i = 0; i < gl->xsize * gl->ysize; i++ ) { (*p) |= 0xFF000000; p++; }
	    	        gl->flags |= PIX_GL_DATA_FLAG_ALPHA_FF;
			glTexImage2D(
            	    	    GL_TEXTURE_2D,
            	    	    0,
			    internal_format,
            		    gl->xsize, gl->ysize,
		    	    0,
            		    format,
		    	    type,
            		    pixels );
            	    }
            	    else
            	    {
            		slog( "glTexImage2D( GL_TEXTURE_2D, 0, %d, %d, %d, 0, %d, %d, pixels ) error %d\n", internal_format, gl->xsize, gl->ysize, format, type, glerr );
            	    }
        	}
		if( c->flags & PIX_CONTAINER_FLAG_GL_FRAMEBUFFER )
		{
		    //Create framebuffer (FBO):
    		    glGenFramebuffers( 1, &gl->framebuffer_id );
    		    gl_bind_framebuffer( gl->framebuffer_id, vm->wm );
    		    //Attach 2D texture to FBO:
    		    glFramebufferTexture2D( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gl->texture_id, 0 );
    		    if( vm->zbuf == PIX_GL_ZBUF )
    		    {
    			//Attach depth texture to FBO:
			glGenTextures( 1, &gl->depth_texture_id );
    			GL_BIND_TEXTURE( vm->wm, gl->depth_texture_id );
    			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
		        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
			//glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, gl->xsize, gl->ysize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
        		//if( glGetError() != GL_NO_ERROR )
        		//{
#ifdef OPENGLES
			    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24_OES, gl->xsize, gl->ysize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL );
#else
			    glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, gl->xsize, gl->ysize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, NULL );
#endif
        		    if( glGetError() != GL_NO_ERROR )
        		    {
				glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, gl->xsize, gl->ysize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_SHORT, NULL );
        		    }
        		//}
    			glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, gl->depth_texture_id, 0 );
    			GL_BIND_TEXTURE( vm->wm, gl->texture_id );
    		    }
    		    //Unbind framebuffer:
    		    gl_bind_framebuffer( 0, vm->wm );
		}
    	    }

    	    if( c->flags & PIX_CONTAINER_FLAG_GL_MIN_LINEAR )
    	        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    	    else
    	        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
    	    if( c->flags & PIX_CONTAINER_FLAG_GL_MAG_LINEAR )
    	        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    	    else
    	        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    	    if( c->flags & PIX_CONTAINER_FLAG_GL_NO_XREPEAT )
    	        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    	    if( c->flags & PIX_CONTAINER_FLAG_GL_NO_YREPEAT )
    	        glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

            rv = gl;
        } //if( !no_texture )

	smem_free( temp_pixels );

        break;
    } //while( 1 )

    return rv;
}

static void add_unused_texture( pix_vm* vm, uint t )
{
    if( !vm->gl_unused_textures )
    {
        vm->gl_unused_textures = (uint32_t*)smem_new( 32 * sizeof( uint32_t ) );
        vm->gl_unused_textures_count = 0;
    }
    vm->gl_unused_textures_count++;
    if( vm->gl_unused_textures_count > smem_get_size( vm->gl_unused_textures ) / sizeof( uint32_t ) )
        vm->gl_unused_textures = (uint32_t*)smem_resize( vm->gl_unused_textures, smem_get_size( vm->gl_unused_textures ) + 32 * sizeof( uint32_t ) );
    vm->gl_unused_textures[ vm->gl_unused_textures_count - 1 ] = t;
}

void pix_vm_remove_container_gl_data( PIX_CID cnum, pix_vm* vm )
{
    if( !vm->wm->gl_initialized ) return;

    if( (unsigned)cnum >= (unsigned)vm->c_num ) return;
    pix_vm_container* c = vm->c[ cnum ];
    if( c == NULL ) return;
    if( c->opt_data == NULL ) return;
    if( c->opt_data->gl == NULL ) return;
    pix_vm_container_gl_data* gl = c->opt_data->gl;

    smutex_lock( &vm->gl_unused_data_mutex );

    if( c->flags & PIX_CONTAINER_FLAG_GL_PROG )
    {
	//GLSL program:
	if( gl->prog )
	{
	    if( vm->gl_unused_progs == 0 )
	    {
		vm->gl_unused_progs = (gl_program_struct**)smem_new( 32 * sizeof( void* ) );
		vm->gl_unused_progs_count = 0;
	    }
	    vm->gl_unused_progs_count++;
	    if( vm->gl_unused_progs_count > smem_get_size( vm->gl_unused_progs ) / sizeof( void* ) )
	    {
		vm->gl_unused_progs = (gl_program_struct**)smem_resize( vm->gl_unused_progs, smem_get_size( vm->gl_unused_progs ) + 32 * sizeof( void* ) );
	    }
	    vm->gl_unused_progs[ vm->gl_unused_progs_count - 1 ] = gl->prog;
	}
    }
    else
    {
	add_unused_texture( vm, gl->texture_id );
	if( c->flags & PIX_CONTAINER_FLAG_GL_FRAMEBUFFER_WITH_DEPTH )
	    add_unused_texture( vm, gl->depth_texture_id );

	if( c->flags & PIX_CONTAINER_FLAG_GL_FRAMEBUFFER )
	{
	    if( !vm->gl_unused_framebuffers )
	    {
		vm->gl_unused_framebuffers = (uint32_t*)smem_new( 32 * sizeof( uint32_t ) );
		vm->gl_unused_framebuffers_count = 0;
	    }
	    vm->gl_unused_framebuffers_count++;
	    if( vm->gl_unused_framebuffers_count > smem_get_size( vm->gl_unused_framebuffers ) / sizeof( uint32_t ) )
		vm->gl_unused_framebuffers = (uint32_t*)smem_resize( vm->gl_unused_framebuffers, smem_get_size( vm->gl_unused_framebuffers ) + 32 * sizeof( uint32_t ) );
	    vm->gl_unused_framebuffers[ vm->gl_unused_framebuffers_count - 1 ] = gl->framebuffer_id;
	}
    }

    smutex_unlock( &vm->gl_unused_data_mutex );

    //Dangerous place!
    //Make sure you don't use this object in other threads and in gl_callback()!
    c->opt_data->gl = NULL;
    smem_free( gl );
}

void pix_vm_update_gl_texture_data( PIX_CID cnum, pix_vm* vm )
{
    if( !vm->wm->gl_initialized ) return;

    if( (unsigned)cnum >= (unsigned)vm->c_num ) return;
    pix_vm_container* c = vm->c[ cnum ];
    if( c == NULL ) return;
    if( c->opt_data == NULL ) return;
    if( c->opt_data->gl == NULL ) return;
    pix_vm_container_gl_data* gl = c->opt_data->gl;

    gl->flags |= PIX_GL_DATA_FLAG_UPDATE_REQ;
}

#endif //OPENGL
