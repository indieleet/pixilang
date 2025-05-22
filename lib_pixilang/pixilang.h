#pragma once

//********************************************************************
// Main Pixilang Virtual Machine Configuration: **********************
//********************************************************************

#define PIXILANG_VERSION ( ( 3 << 24 ) | ( 8 << 16 ) | ( 5 << 8 ) | ( 1 << 0 ) )
#define PIXILANG_VERSION_STR "v3.8.5b"

#define PIX_VM_THREADS		    32
#define PIX_VM_SYSTEM_THREADS	    3 /* threads-1 == audio; threads-2 == opengl; threads-3 == video capture callback */
#define PIX_VM_STACK_SIZE	    8192 /* number of PIX_VALs */
#define PIX_VM_EVENTS		    64

#define PIX_VM_AUDIO_CHANNELS       2
#define PIX_VM_SUNVOX_STREAMS       8

typedef int32_t PIX_INT; //Pixilang integer type
typedef float PIX_FLOAT; //Pixilang floating point type
typedef uint32_t PIX_OPCODE; //VM opcode type
typedef int32_t PIX_CID; //Container ID
typedef int32_t PIX_ADDR; //Pixilang code address (instruction offset)
typedef uint32_t PIX_PC; //Program Counter
typedef uint32_t PIX_SP; //Stack Pointer
#define PIX_INT_BITS ( sizeof( PIX_INT ) * 8 )
#define PIX_FLOAT_BITS ( sizeof( PIX_FLOAT ) * 8 )

//#define PIX_INT64_ENABLED
//#define PIX_FLOAT64_ENABLED

#define PIX_OPCODE_BITS		    6
#define PIX_OPCODE_MASK		    ( ( 1 << PIX_OPCODE_BITS ) - 1 )

#define PIX_FN_BITS		    9
#define PIX_FN_MASK		    ( ( 1 << PIX_FN_BITS ) - 1 )

#define PIX_INT_MAX_POSITIVE	    ( (unsigned)( (PIX_INT)(-1) ) >> 1 )
#define PIX_INT_ADDRESS_MARKER	    (unsigned int)0xA0000000
#define PIX_INT_ADDRESS_MASK	    (unsigned int)0x0FFFFFFF
#define IS_ADDRESS_CORRECT( v )     ( ( ( v ) & ~PIX_INT_ADDRESS_MASK ) == PIX_INT_ADDRESS_MARKER )

//Fixed point precision for math operations:
#define PIX_FIXED_MATH_PREC	    12
#define PIX_TEX_FIXED_MATH_PREC	    15
#define PIX_AUDIO_PTR_PREC	    14
#define PIX_MAX_FIXED_MATH 	    ( 1 << ( PIX_INT_BITS - PIX_FIXED_MATH_PREC - 1 ) )

#define PIX_T_MATRIX_STACK_SIZE	    16

//#define PIX_SAFE_VM //More checks to prevent segfaults; required if Pixilang is running as a script interpreter inside some other app
#ifdef PIX_SAFE_VM
    #define PIX_CHECK_SP( SP )	( ( SP ) & ( PIX_VM_STACK_SIZE - 1 ) )
#else
    #define PIX_CHECK_SP( SP )	( SP )
#endif

//********************************************************************
//********************************************************************
//********************************************************************

//Pixilang program format:
// address | instruction
// 0       | HALT
// 1       | start of the main user function 
// ...     | ...
// ...     | RET_i - return from the main function

//Pixilang stack structure:
// [ top of the stack ]
// ...
// function parameter X;
// ...
// function parameter 1;
// number of parameters; <============= current FP
// previous FP (frame pointer);
// previous PC (program counter);
// local variables;
// ...
// [ bottom of the stack - zero offset ]

//Pixilang transformation matrix:
// | 0  4  8  12 |
// | 1  5  9  13 |
// | 2  6  10 14 |
// | 3  7  11 15 |

union PIX_VAL
{
    PIX_INT i;
    PIX_FLOAT f;
};

//Pixilang VM opcodes:
// small letter - number is in the opcode body;
// V - variable (type = PIX_OPCODE);
// N - short number (type = PIX_OPCODE);
// I - PIX_INT (type = one or several PIX_OPCODEs);
// F - PIX_FLOAT (type = one or several PIX_OPCODEs).
enum pix_vm_opcode
{
    OPCODE_NOP = 0,

    OPCODE_HALT,

    //OPCODE_TMP,

    OPCODE_PUSH_I,
    OPCODE_PUSH_i,
    OPCODE_PUSH_F,
    OPCODE_PUSH_v,

    //Size of these opcodes must be less or equal pcomp->statlist_header_size: ******
    OPCODE_GO,
    OPCODE_JMP_i,
    OPCODE_JMP_IF_FALSE_i,
    //*******************************************************************************

    OPCODE_SAVE_TO_VAR_v,

    OPCODE_SAVE_TO_PROP_I,
    OPCODE_LOAD_FROM_PROP_I,
    
    OPCODE_SAVE_TO_MEM,
    OPCODE_SAVE_TO_SMEM_2D,
    OPCODE_LOAD_FROM_MEM,
    OPCODE_LOAD_FROM_SMEM_2D,
    
    OPCODE_SAVE_TO_STACKFRAME_i,
    OPCODE_LOAD_FROM_STACKFRAME_i,

    OPCODE_SUB,
    OPCODE_ADD,
    OPCODE_MUL,
    OPCODE_IDIV,
    OPCODE_DIV,
    OPCODE_MOD,
    OPCODE_AND,
    OPCODE_OR,
    OPCODE_XOR,
    OPCODE_ANDAND,
    OPCODE_OROR,
    OPCODE_EQ,
    OPCODE_NEQ,
    OPCODE_LESS,
    OPCODE_LEQ,
    OPCODE_GREATER,
    OPCODE_GEQ,
    OPCODE_LSHIFT,
    OPCODE_RSHIFT,

    OPCODE_NEG,
    OPCODE_LOGICAL_NOT,
    OPCODE_BITWISE_NOT,

    OPCODE_CALL_BUILTIN_FN,
    OPCODE_CALL_BUILTIN_FN_VOID,
    OPCODE_CALL_i,
    OPCODE_INC_SP_i,
    OPCODE_RET_i,
    OPCODE_RET_I,
    OPCODE_RET,

    NUMBER_OF_OPCODES
};

//Built-in functions:
enum
{ //FN IDs

    //Containers (memory management):

    FN_NEW_PIXI,
    FN_REMOVE_PIXI,
    FN_REMOVE_PIXI_WITH_ALPHA,
    FN_RESIZE_PIXI,
    FN_ROTATE_PIXI,
    FN_CLEAN_PIXI,
    FN_CLONE_PIXI,
    FN_COPY_PIXI,
    FN_GET_PIXI_SIZE,
    FN_GET_PIXI_XSIZE,
    FN_GET_PIXI_YSIZE,
    FN_GET_PIXI_ESIZE,
    FN_GET_PIXI_TYPE,
    FN_GET_PIXI_FLAGS,
    FN_SET_PIXI_FLAGS,
    FN_RESET_PIXI_FLAGS,
    FN_GET_PIXI_PROP,
    FN_SET_PIXI_PROP,
    FN_REMOVE_PIXI_PROP,
    FN_REMOVE_PIXI_PROPS,
    FN_GET_PIXI_PROPLIST,
    FN_REMOVE_PIXI_PROPLIST,
    FN_CONVERT_PIXI_TYPE,
    FN_SHOW_SMEM_DEBUG_MESSAGES,
    FN_ZLIB_PACK,
    FN_ZLIB_UNPACK,

    //Working with strings:

    FN_NUM_TO_STRING,
    FN_STRING_TO_NUM,

    //Working with strings (posix):

    FN_STRCAT,
    FN_STRCMP,
    FN_STRLEN,
    FN_STRSTR,
    FN_SPRINTF,
    FN_SPRINTF2,
    FN_PRINTF,
    FN_FPRINTF,

    //Log management:

    FN_LOGF,
    FN_GET_LOG,
    FN_GET_SYSTEM_LOG,

    //Files:

    FN_LOAD,
    FN_FLOAD,
    FN_SAVE,
    FN_FSAVE,
    FN_GET_REAL_PATH,
    FN_NEW_FLIST,
    FN_REMOVE_FLIST,
    FN_GET_FLIST_NAME,
    FN_GET_FLIST_TYPE,
    FN_FLIST_NEXT,
    FN_GET_FILE_SIZE,
    FN_GET_FILE_FORMAT,
    FN_GET_FFORMAT_MIME,
    FN_GET_FFORMAT_EXT,
    FN_REMOVE_FILE,
    FN_RENAME_FILE,
    FN_COPY_FILE,
    FN_CREATE_DIRECTORY,
    FN_SET_DISK0,
    FN_GET_DISK0,

    //Files (posix):

    FN_FOPEN,
    FN_FOPEN_MEM,
    FN_FCLOSE,
    FN_FPUTC,
    FN_FPUTS,
    FN_FWRITE,
    FN_FGETC,
    FN_FGETS,
    FN_FREAD,
    FN_FEOF,
    FN_FFLUSH,
    FN_FSEEK,
    FN_FTELL,
    FN_SETXATTR,

    //Graphics:

    FN_FRAME,
    FN_VSYNC,
    FN_SET_PIXEL_SIZE,
    FN_GET_PIXEL_SIZE,
    FN_SET_SCREEN,
    FN_GET_SCREEN,
    FN_SET_ZBUF,
    FN_GET_ZBUF,
    FN_CLEAR_ZBUF,
    FN_GET_COLOR,
    FN_GET_RED,
    FN_GET_GREEN,
    FN_GET_BLUE,
    FN_GET_BLEND,
    FN_TRANSP,
    FN_GET_TRANSP,
    FN_CLEAR,
    FN_DOT,
    FN_DOT3D,
    FN_GET_DOT,
    FN_GET_DOT3D,
    FN_LINE,
    FN_LINE3D,
    FN_BOX,
    FN_FBOX,
    FN_PIXI,
    FN_TRIANGLES,
    FN_SORT_TRIANGLES,
    FN_SET_KEY_COLOR,
    FN_GET_KEY_COLOR,
    FN_SET_ALPHA,
    FN_GET_ALPHA,
    FN_PRINT,
    FN_GET_TEXT_XSIZE,
    FN_GET_TEXT_YSIZE,
    FN_GET_TEXT_XYSIZE,
    FN_SET_FONT,
    FN_GET_FONT,
    FN_EFFECTOR,
    FN_COLOR_GRADIENT,
    FN_SPLIT_RGB,
    FN_SPLIT_YCBCR,

    //OpenGL:

    FN_SET_GL_CALLBACK,
    FN_REMOVE_GL_DATA,
    FN_UPDATE_GL_DATA,
    FN_GL_DRAW_ARRAYS,
    FN_GL_BLEND_FUNC,
    FN_GL_BIND_FRAMEBUFFER,
    FN_GL_BIND_TEXTURE,
    FN_GL_GET_INT,
    FN_GL_GET_FLOAT,
    FN_GL_NEW_PROG,
    FN_GL_USE_PROG,
    FN_GL_UNIFORM,
    FN_GL_UNIFORM_MATRIX,

    //Animation:

    FN_PIXI_UNPACK_FRAME,
    FN_PIXI_PACK_FRAME,
    FN_PIXI_CREATE_ANIM,
    FN_PIXI_REMOVE_ANIM,
    FN_PIXI_CLONE_FRAME,
    FN_PIXI_REMOVE_FRAME,
    FN_PIXI_PLAY,
    FN_PIXI_STOP,

    //Video (not finished):

    FN_VIDEO_OPEN,
    FN_VIDEO_CLOSE,
    FN_VIDEO_START,
    FN_VIDEO_STOP,
    FN_VIDEO_SET_PROPS,
    FN_VIDEO_GET_PROPS,
    FN_VIDEO_CAPTURE_FRAME,

    //Transformation:

    FN_T_RESET,
    FN_T_ROTATE,
    FN_T_TRANSLATE,
    FN_T_SCALE,
    FN_T_PUSH_MATRIX,
    FN_T_POP_MATRIX,
    FN_T_GET_MATRIX,
    FN_T_SET_MATRIX,
    FN_T_MUL_MATRIX,
    FN_T_POINT,

    //Audio:

    FN_SET_AUDIO_CALLBACK,
    FN_ENABLE_AUDIO_INPUT,
    FN_GET_AUDIO_SAMPLE_RATE,
    FN_GET_NOTE_FREQ,

    //MIDI:

    FN_MIDI_OPEN_CLIENT,
    FN_MIDI_CLOSE_CLIENT,
    FN_MIDI_GET_DEVICE,
    FN_MIDI_OPEN_PORT,
    FN_MIDI_REOPEN_PORT,
    FN_MIDI_CLOSE_PORT,
    FN_MIDI_GET_EVENT,
    FN_MIDI_GET_EVENT_TIME,
    FN_MIDI_NEXT_EVENT,
    FN_MIDI_SEND_EVENT,

    //SunVox:

    FN_SV_NEW,
    FN_SV_REMOVE,
    FN_SV_GET_SAMPLE_RATE,
    FN_SV_RENDER,
    FN_SV_LOCK,
    FN_SV_UNLOCK,
    FN_SV_LOAD,
    FN_SV_FLOAD,
    FN_SV_SAVE,
    FN_SV_FSAVE,
    FN_SV_PLAY,
    FN_SV_STOP,
    FN_SV_PAUSE,
    FN_SV_RESUME,
    FN_SV_SYNC_RESUME,
    FN_SV_SET_AUTOSTOP,
    FN_SV_GET_AUTOSTOP,
    FN_SV_GET_STATUS,
    FN_SV_REWIND,
    FN_SV_VOLUME,
    FN_SV_SET_EVENT_T,
    FN_SV_SEND_EVENT,
    FN_SV_GET_CURRENT_LINE,
    FN_SV_GET_CURRENT_LINE2,
    FN_SV_GET_CURRENT_SIGLEVEL,
    FN_SV_GET_NAME,
    FN_SV_SET_NAME,
    FN_SV_GET_BPM,
    FN_SV_GET_TPL,
    FN_SV_GET_LEN_FRAMES,
    FN_SV_GET_LEN_LINES,
    FN_SV_GET_TIME_MAP,
    FN_SV_NEW_MODULE,
    FN_SV_REMOVE_MODULE,
    FN_SV_CONNECT_MODULE,
    FN_SV_DISCONNECT_MODULE,
    FN_SV_LOAD_MODULE,
    FN_SV_FLOAD_MODULE,
    FN_SV_SAMPLER_LOAD,
    FN_SV_SAMPLER_FLOAD,
    FN_SV_SAMPLER_PAR,
    FN_SV_METAMODULE_LOAD,
    FN_SV_METAMODULE_FLOAD,
    FN_SV_VPLAYER_LOAD,
    FN_SV_VPLAYER_FLOAD,
    FN_SV_GET_NUMBER_OF_MODULES,
    FN_SV_FIND_MODULE,
    FN_SV_SELECTED_MODULE,
    FN_SV_GET_MODULE_FLAGS,
    FN_SV_GET_MODULE_INPUTS,
    FN_SV_GET_MODULE_OUTPUTS,
    FN_SV_GET_MODULE_TYPE,
    FN_SV_GET_MODULE_NAME,
    FN_SV_SET_MODULE_NAME,
    FN_SV_GET_MODULE_XY,
    FN_SV_SET_MODULE_XY,
    FN_SV_GET_MODULE_COLOR,
    FN_SV_SET_MODULE_COLOR,
    FN_SV_GET_MODULE_FINETUNE,
    FN_SV_SET_MODULE_FINETUNE,
    FN_SV_SET_MODULE_RELNOTE,
    FN_SV_GET_MODULE_SCOPE,
    FN_SV_MODULE_CURVE,
    FN_SV_GET_MODULE_CTL_CNT,
    FN_SV_GET_MODULE_CTL_NAME,
    FN_SV_GET_MODULE_CTL_VALUE,
    FN_SV_SET_MODULE_CTL_VALUE,
    FN_SV_GET_MODULE_CTL_MIN,
    FN_SV_GET_MODULE_CTL_MAX,
    FN_SV_GET_MODULE_CTL_OFFSET,
    FN_SV_GET_MODULE_CTL_TYPE,
    FN_SV_GET_MODULE_CTL_GROUP,
    FN_SV_NEW_PAT,
    FN_SV_REMOVE_PAT,
    FN_SV_GET_NUMBER_OF_PATS,
    FN_SV_FIND_PATTERN,
    FN_SV_GET_PAT_X,
    FN_SV_GET_PAT_Y,
    FN_SV_SET_PAT_XY,
    FN_SV_GET_PAT_TRACKS,
    FN_SV_GET_PAT_LINES,
    FN_SV_SET_PAT_SIZE,
    FN_SV_GET_PAT_NAME,
    FN_SV_SET_PAT_NAME,
    FN_SV_GET_PAT_DATA,
    FN_SV_SET_PAT_EVENT,
    FN_SV_GET_PAT_EVENT,
    FN_SV_PAT_MUTE,

    //Time:

    FN_START_TIMER,
    FN_GET_TIMER,
    FN_GET_YEAR,
    FN_GET_MONTH,
    FN_GET_DAY,
    FN_GET_HOURS,
    FN_GET_MINUTES,
    FN_GET_SECONDS,
    FN_GET_TICKS,
    FN_GET_TPS,
    FN_SLEEP,

    //Events:

    FN_GET_EVENT,
    FN_SET_QUIT_ACTION,

    //Threads:

    FN_THREAD_CREATE,
    FN_THREAD_DESTROY,
    FN_MUTEX_CREATE,
    FN_MUTEX_DESTROY,
    FN_MUTEX_LOCK,
    FN_MUTEX_TRYLOCK,
    FN_MUTEX_UNLOCK,

    //Mathematical functions:

    FN_ACOS,
    FN_ACOSH,
    FN_ASIN,
    FN_ASINH,
    FN_ATAN,
    FN_ATANH,
    FN_ATAN2,
    FN_CEIL,
    FN_COS,
    FN_COSH,
    FN_EXP,
    FN_EXP2,
    FN_EXPM1,
    FN_ABS,
    FN_FLOOR,
    FN_MOD,
    FN_LOG,
    FN_LOG2,
    FN_LOG10,
    FN_POW,
    FN_SIN,
    FN_SINH,
    FN_SQRT,
    FN_TAN,
    FN_TANH,
    FN_RAND,
    FN_RAND_SEED,

    //Type punning:

    FN_REINTERPRET_TYPE,

    //Data processing:

    FN_OP_CN,
    FN_OP_CC,
    FN_OP_CCN,
    FN_GENERATOR,
    FN_WAVETABLE_GENERATOR,
    FN_SAMPLER,
    FN_ENVELOPE2P,
    FN_GRADIENT,
    FN_FFT,
    FN_NEW_FILTER,
    FN_REMOVE_FILTER,
    FN_INIT_FILTER,
    FN_RESET_FILTER,
    FN_APPLY_FILTER,
    FN_REPLACE_VALUES,
    FN_COPY_AND_RESIZE,
    FN_CONV_FILTER,

    //Dialogs:

    FN_FILE_DIALOG,
    FN_PREFS_DIALOG,
    FN_TEXTINPUT_DIALOG,

    //Network:

    FN_OPEN_URL,

    //Native code:

    FN_DLOPEN,
    FN_DLCLOSE,
    FN_DLSYM,
    FN_DLCALL,

    //Posix compatibility:

    FN_SYSTEM,
    FN_ARGC,
    FN_ARGV,
    FN_EXIT,

    //Experimental API:

    FN_WEBSERVER_DIALOG,
    FN_MIDIOPT_DIALOG,
    FN_SYSTEM_COPY,
    FN_SYSTEM_PASTE,
    FN_SEND_FILE_TO_GALLERY,
    FN_EXPORT_IMPORT_FILE,
    FN_SET_AUDIO_PLAY_STATUS,
    FN_GET_AUDIO_EVENT,
    FN_OPEN_APP_STATE,
    FN_CLOSE_APP_STATE,
    FN_WM_VIDEO_CAPTURE_START,
    FN_WM_VIDEO_CAPTURE_STOP,
    FN_WM_VIDEO_CAPTURE_GET_EXT,
    FN_WM_VIDEO_CAPTURE_ENCODE,

    FN_NUM
}; //FN IDs

//pix_vm_run() modes:
enum pix_vm_run_mode
{
    PIX_VM_CONTINUE,
    PIX_VM_CALL_FUNCTION,
    PIX_VM_CALL_MAIN,
};

enum pix_sym_type
{
    SYMTYPE_LVAR,
    SYMTYPE_GVAR,
    SYMTYPE_NUM_I,
    SYMTYPE_NUM_F,
    SYMTYPE_WHILE,
    SYMTYPE_FOR,
    SYMTYPE_BREAK,
    SYMTYPE_BREAK2,
    SYMTYPE_BREAK3,
    SYMTYPE_BREAK4,
    SYMTYPE_BREAKALL,
    SYMTYPE_CONTINUE,
    SYMTYPE_IF,
    SYMTYPE_ELSE,
    SYMTYPE_GO,
    SYMTYPE_RET,
    SYMTYPE_IDIV,
    SYMTYPE_FNNUM,
    SYMTYPE_FNDEF,
    SYMTYPE_INCLUDE,
    SYMTYPE_HALT,
    SYMTYPE_DELETED
};

enum pix_container_type
{
    PIX_CONTAINER_TYPE_INT8 = 0,
    PIX_CONTAINER_TYPE_INT16,
    PIX_CONTAINER_TYPE_INT32,
    PIX_CONTAINER_TYPE_INT64,
    PIX_CONTAINER_TYPE_FLOAT32,
    PIX_CONTAINER_TYPE_FLOAT64,
    PIX_CONTAINER_TYPES
};

enum
{
    PIX_EFFECT_NOISE,
    PIX_EFFECT_SPREAD_LEFT,
    PIX_EFFECT_SPREAD_RIGHT,
    PIX_EFFECT_SPREAD_UP,
    PIX_EFFECT_SPREAD_DOWN,
    PIX_EFFECT_HBLUR,
    PIX_EFFECT_VBLUR,
    PIX_EFFECT_COLOR
};

enum
{
    GL_SHADER_SOLID = 0,
    GL_SHADER_GRAD,
    GL_SHADER_TEX_ALPHA_SOLID,
    GL_SHADER_TEX_ALPHA_GRAD,
    GL_SHADER_TEX_RGBA_SOLID,
    GL_SHADER_TEX_RGBA_GRAD,
    GL_SHADER_MAX
};

//gl_bind_framebuffer() flags:
#define GL_BFB_IDENTITY_MATRIX 			( 1 << 0 )

#define PIX_CONTAINER_FLAG_USES_KEY	 	( 1 << 0 )
#define PIX_CONTAINER_FLAG_STATIC_DATA	 	( 1 << 1 )
#define PIX_CONTAINER_FLAG_SYSTEM_MANAGED 	( 1 << 2 )
#define PIX_CONTAINER_FLAG_GL_MIN_LINEAR	( 1 << 3 )
#define PIX_CONTAINER_FLAG_GL_MAG_LINEAR	( 1 << 4 )
#define PIX_CONTAINER_FLAG_GL_NO_XREPEAT	( 1 << 5 )
#define PIX_CONTAINER_FLAG_GL_NO_YREPEAT	( 1 << 6 )
#define PIX_CONTAINER_FLAG_GL_NICEST		( 1 << 7 )
#define PIX_CONTAINER_FLAG_GL_NO_ALPHA		( 1 << 8 )
#define PIX_CONTAINER_FLAG_GL_FRAMEBUFFER	( 1 << 9 )
#define PIX_CONTAINER_FLAG_GL_FRAMEBUFFER_WITH_DEPTH	( 1 << 10 )
#define PIX_CONTAINER_FLAG_GL_PROG		( 1 << 11 )
#define PIX_CONTAINER_FLAG_GL_NPOT		( 1 << 12 )
#define PIX_CONTAINER_FLAG_INTERP		( 1 << 13 )

enum pix_data_opcode
{
    //op_cn():
    PIX_DATA_OPCODE_MIN,
    PIX_DATA_OPCODE_MAX,
    PIX_DATA_OPCODE_MAXABS,
    PIX_DATA_OPCODE_SUM,
    PIX_DATA_OPCODE_LIMIT_TOP,
    PIX_DATA_OPCODE_LIMIT_BOTTOM,
    PIX_DATA_OPCODE_ABS,
    PIX_DATA_OPCODE_SUB2,
    PIX_DATA_OPCODE_COLOR_SUB2,
    PIX_DATA_OPCODE_DIV2,
    PIX_DATA_OPCODE_H_INTEGRAL,
    PIX_DATA_OPCODE_V_INTEGRAL,
    PIX_DATA_OPCODE_H_DERIVATIVE,
    PIX_DATA_OPCODE_V_DERIVATIVE,
    PIX_DATA_OPCODE_H_FLIP,
    PIX_DATA_OPCODE_V_FLIP,
    //op_cn(), op_cc():
    PIX_DATA_OPCODE_ADD,
    PIX_DATA_OPCODE_SADD,
    PIX_DATA_OPCODE_COLOR_ADD,
    PIX_DATA_OPCODE_SUB,
    PIX_DATA_OPCODE_SSUB,
    PIX_DATA_OPCODE_COLOR_SUB,
    PIX_DATA_OPCODE_MUL,
    PIX_DATA_OPCODE_SMUL,
    PIX_DATA_OPCODE_MUL_RSHIFT15,
    PIX_DATA_OPCODE_COLOR_MUL,
    PIX_DATA_OPCODE_DIV,
    PIX_DATA_OPCODE_COLOR_DIV,
    PIX_DATA_OPCODE_AND,
    PIX_DATA_OPCODE_OR,
    PIX_DATA_OPCODE_XOR,
    PIX_DATA_OPCODE_LSHIFT,
    PIX_DATA_OPCODE_RSHIFT,
    PIX_DATA_OPCODE_EQUAL,
    PIX_DATA_OPCODE_LESS,
    PIX_DATA_OPCODE_GREATER,
    PIX_DATA_OPCODE_COPY,
    PIX_DATA_OPCODE_COPY_LESS,
    PIX_DATA_OPCODE_COPY_GREATER,
    //op_cc():
    PIX_DATA_OPCODE_BMUL,
    PIX_DATA_OPCODE_EXCHANGE,
    PIX_DATA_OPCODE_COMPARE,
    //op_ccn():
    PIX_DATA_OPCODE_MUL_DIV, //container1 = ( container1 * container2 ) / number
    PIX_DATA_OPCODE_MUL_RSHIFT, //container1 = ( container1 * container2 ) >> number
    //generator():
    PIX_DATA_OPCODE_SIN,
    PIX_DATA_OPCODE_SIN8,
    PIX_DATA_OPCODE_RAND,
};

enum
{
    PIX_SAMPLER_DEST = 0,
    PIX_SAMPLER_DEST_OFF,
    PIX_SAMPLER_DEST_LEN,
    PIX_SAMPLER_SRC,
    PIX_SAMPLER_SRC_OFF_H,
    PIX_SAMPLER_SRC_OFF_L,
    PIX_SAMPLER_SRC_SIZE,
    PIX_SAMPLER_LOOP,
    PIX_SAMPLER_LOOP_LEN,
    PIX_SAMPLER_VOL1,
    PIX_SAMPLER_VOL2,
    PIX_SAMPLER_DELTA,
    PIX_SAMPLER_FLAGS,
    PIX_SAMPLER_PARAMETERS,
};

#define PIX_SAMPLER_FLAG_INTERP0	0
#define PIX_SAMPLER_FLAG_INTERP2	1
#define PIX_SAMPLER_FLAG_INTERP4	2
#define PIX_SAMPLER_FLAG_INTERP_MASK	7
#define PIX_SAMPLER_FLAG_PINGPONG	( 1 << 3 )
#define PIX_SAMPLER_FLAG_REVERSE	( 1 << 4 )
#define PIX_SAMPLER_FLAG_INLOOP		( 1 << 5 )

#define PIX_AUDIO_FLAG_INTERP2		1

#define PIX_COPY_NO_AUTOROTATE		1
#define PIX_COPY_CLIPPING		2

#define PIX_RESIZE_INTERP1			1
#define PIX_RESIZE_INTERP2			2
#define PIX_RESIZE_INTERP_TYPE( flags )		( flags & 15 )
#define PIX_RESIZE_INTERP_COLOR			( 1 << 4 )
#define PIX_RESIZE_INTERP_UNSIGNED		( 2 << 4 )
#define PIX_RESIZE_INTERP_OPTIONS( flags )	( flags & ( 15 << 4 ) )
#define PIX_RESIZE_MASK_INTERPOLATION		255

#define PIX_CONV_FILTER_TYPE_NUM		0
#define PIX_CONV_FILTER_TYPE_COLOR		1	//only for PIXEL rgb containers
#define PIX_CONV_FILTER_TYPE( flags )		( flags & 15 )
#define PIX_CONV_FILTER_BORDER_EXTEND		( 0 << 4 )
#define PIX_CONV_FILTER_BORDER_SKIP		( 1 << 4 )
#define PIX_CONV_FILTER_BORDER( flags )		( flags & ( 15 << 4 ) )
#define PIX_CONV_FILTER_UNSIGNED		( 1 << 8 )	//only for INT8 and INT16 containers

#define PIX_THREAD_FLAG_AUTO_DESTROY		( 1 << 0 )

enum 
{
    PIX_EVT_NULL = 0,
    PIX_EVT_MOUSEBUTTONDOWN,
    PIX_EVT_MOUSEBUTTONUP,
    PIX_EVT_MOUSEMOVE,
    PIX_EVT_TOUCHBEGIN,
    PIX_EVT_TOUCHEND,
    PIX_EVT_TOUCHMOVE,
    PIX_EVT_BUTTONDOWN,
    PIX_EVT_BUTTONUP,
    PIX_EVT_SCREENRESIZE,
    PIX_EVT_LOADSTATE,
    PIX_EVT_SAVESTATE,
    PIX_EVT_QUIT,
};

//Load/Save options (flags):
#define PIX_GIF_GRAYSCALE		1
#define PIX_GIF_PALETTE_MASK		3
#define PIX_GIF_DITHER			4
#define PIX_JPEG_QUALITY( opt )		( opt & 127 )
#define PIX_JPEG_H1V1			( 1 << 7 )
#define PIX_JPEG_H2V1			( 2 << 7 )
#define PIX_JPEG_H2V2			( 3 << 7 )
#define PIX_JPEG_SUBSAMPLING_MASK	( 3 << 7 )
#define PIX_JPEG_TWOPASS		( 1 << ( 7 + 3 ) )
#define PIX_FLAC_QUALITY( opt )		( opt & 15 )
#define PIX_OGG_QUALITY( opt )		( opt & 127 )
#define PIX_LOAD_FIRST_FRAME		( 1 << 0 )
#define PIX_LOAD_AS_RAW			( 1 << 1 )

#define PIX_INFO_MODULE			( 1 << 0 )
#define PIX_INFO_MULTITOUCH		( 1 << 1 )
#define PIX_INFO_TOUCHCONTROL		( 1 << 2 )
#define PIX_INFO_NOWINDOW		( 1 << 3 )
#define PIX_INFO_MIDIIN			( 1 << 4 )
#define PIX_INFO_MIDIOUT		( 1 << 5 )
#define PIX_INFO_MIDIOPTIONS		( 1 << 6 )
#define PIX_INFO_WEBSERVER		( 1 << 7 )
#define PIX_INFO_CLIPBOARD		( 1 << 8 )
#define PIX_INFO_CLIPBOARD_AV		( 1 << 9 )
#define PIX_INFO_GALLERY		( 1 << 10 )
#define PIX_INFO_EMAIL			( 1 << 11 )
#define PIX_INFO_EXPORT			( 1 << 12 )
#define PIX_INFO_EXPORT2		( 1 << 13 )
#define PIX_INFO_IMPORT			( 1 << 14 )
#define PIX_INFO_VIDEOCAPTURE		( 1 << 15 )
#define PIX_INFO_OPENGL			( 1 << 16 )

enum
{
    PIX_GVAR_WINDOW_XSIZE = 128,
    PIX_GVAR_WINDOW_YSIZE,
    PIX_GVAR_WINDOW_SAFE_AREA_X,
    PIX_GVAR_WINDOW_SAFE_AREA_Y,
    PIX_GVAR_WINDOW_SAFE_AREA_W,
    PIX_GVAR_WINDOW_SAFE_AREA_H,
    PIX_GVAR_FPS,
    PIX_GVAR_PPI,
    PIX_GVAR_SCALE,
    PIX_GVAR_FONT_SCALE,
    PIX_GVAR_PIXILANG_INFO,
    PIX_GVARS
};

#define PIX_COMPILER_SYMTAB_SIZE	6151	//prime number
#define PIX_CONTAINER_SYMTAB_SIZE	53	//prime number

enum
{
    pix_vm_container_hdata_type_anim = 0,
};

#define PIX_CCONV_DEFAULT		0
#define PIX_CCONV_CDECL			1
#define PIX_CCONV_STDCALL		2
#define PIX_CCONV_UNIX_AMD64		3
#define PIX_CCONV_WIN64			4

#define PIX_GL_SCREEN			-2
#define PIX_GL_ZBUF			-3

#define PIX_CODE_ANALYZER_SHOW_OPCODES	( 1 << 0 )
#define PIX_CODE_ANALYZER_SHOW_ADDRESS	( 1 << 1 )
#define PIX_CODE_ANALYZER_SHOW_STATS	( 1 << 2 )

#define PIX_SV_INIT_FLAG_OFFLINE	( 1 << 0 )
#define PIX_SV_INIT_FLAG_ONE_THREAD	( 1 << 1 )
#define PIX_SV_TIME_MAP_SPEED       	0
#define PIX_SV_TIME_MAP_FRAMECNT   	1
#define PIX_SV_TIME_MAP_TYPE_MASK   	3
#define PIX_SV_MODULE_FLAG_EXISTS 	( 1 << 0 )
#define PIX_SV_MODULE_FLAG_GENERATOR 	( 1 << 1 )
#define PIX_SV_MODULE_FLAG_EFFECT 	( 1 << 2 )
#define PIX_SV_MODULE_FLAG_MUTE 	( 1 << 3 )
#define PIX_SV_MODULE_FLAG_SOLO 	( 1 << 4 )
#define PIX_SV_MODULE_FLAG_BYPASS 	( 1 << 5 )
#define PIX_SV_MODULE_INPUTS_OFF 	16
#define PIX_SV_MODULE_INPUTS_MASK 	( 255 << PIX_SV_MODULE_INPUTS_OFF )
#define PIX_SV_MODULE_OUTPUTS_OFF 	( 16 + 8 )
#define PIX_SV_MODULE_OUTPUTS_MASK 	( 255 << PIX_SV_MODULE_OUTPUTS_OFF )

struct pix_vm;
struct sunvox_engine;
struct sunvox_pattern;
struct sunvox_pattern_info;
struct sunvox_render_data;

struct pix_sym //Symbol
{
    char*		name;
    pix_sym_type	type;
    PIX_VAL		val;
    pix_sym*		next;
};

struct pix_symtab //Symbol table
{
    int			size;
    pix_sym**		symtab;
};

struct pix_stack_val
{
    union
    {
	PIX_VAL v;
	PIX_INT i;
	PIX_FLOAT f;
    };
    uint8_t t; //type
};

struct pix_vm_thread
{
    volatile bool	active;

    uint		flags;
    sthread		th;
    bool		thread_open; //TRUE if it is a separate non-blocking thread
    int			thread_num;
    pix_vm*		vm;

    PIX_PC		pc; //Program counter
    PIX_SP		sp; //Stack pointer (grows down)
    PIX_SP		fp; //Stack frame pointer

    pix_stack_val	stack[ PIX_VM_STACK_SIZE ];
};

struct pix_vm_function
{
    PIX_ADDR		addr;
    pix_stack_val*	p;
    int			p_num;
};

struct pix_vm_anim_frame
{
    pix_container_type	type;
    PIX_INT		xsize;
    PIX_INT		ysize;
    COLORPTR		pixels;
};

struct pix_vm_container_hdata_anim
{
    uint8_t		type;
    uint		frame_count;
    pix_vm_anim_frame*	frames;
};

#ifdef OPENGL
#define PIX_GL_DATA_FLAG_UPDATE_REQ	( 1 << 0 )
#define PIX_GL_DATA_FLAG_ALPHA_FF	( 1 << 1 ) //when internal GL texture format = GL_RGBA, but A is not used
struct pix_vm_container_gl_data
{
    volatile uint32_t	flags;
    //int			src_xsize; //may be != texture xsize
    //int			src_ysize; //may be != texture ysize
    int			xsize; //texture width (power of two)
    int			ysize; //texture height (power of two)
    uint		texture_id;
    uint		texture_format;
    uint		depth_texture_id;
    uint		framebuffer_id;
    gl_program_struct*	prog;
};
#endif

struct pix_vm_container_opt_data
{
    pix_symtab		props; //Properties
    void*		hdata; //Hidden data
#ifdef OPENGL
    pix_vm_container_gl_data* gl;
#endif
};

struct pix_vm_container //Universal container - base component of Pixilang
{
    pix_container_type	type;
    uint		flags;
    PIX_INT		xsize;
    PIX_INT		ysize;
    size_t		size;
    void*		data;
    COLOR		key;
    PIX_CID		alpha; //Container with alpha-channel
    pix_vm_container_opt_data* opt_data; //Optional data (properties, animation etc)
};

struct pix_vm_font
{
    PIX_CID 		font; //container with the font texture (glyph atlas)
    uint32_t		first; //first unicode character
    uint32_t		last; //last unicode character
    uint16_t		char_xsize; //visible char size
    uint16_t		char_ysize; //...
    uint16_t		char_xsize2; //char size on the font texture
    uint16_t		char_ysize2; //...
    //grid inside the font texture:
    uint16_t		grid_xoffset;
    uint16_t		grid_yoffset;
    uint16_t		grid_cell_xsize;
    uint16_t		grid_cell_ysize;
    uint16_t		xchars;
    uint16_t		ychars;
};

struct pix_vm_text_line
{
    size_t		offset;
    size_t		end;
    int			xsize;
    int			ysize;
};

struct pix_vm_event
{
    uint16_t          	type; //event type
    uint16_t	    	flags;
    stime_ms_t	    	time;
    int16_t           	x;
    int16_t           	y;
    uint16_t	    	key; //virtual key code: standart ASCII (0..127) and additional (see KEY_* defines)
    uint16_t	    	scancode; //device dependent
    uint16_t	    	pressure; //key pressure (0..1024)
};

struct pix_vm_ivertex
{
    PIX_INT		x; //fixed point (PIX_FIXED_MATH_PREC)
    PIX_INT		y; //...
    PIX_INT		z; //...
};

struct pix_vm_ivertex_t
{
    PIX_INT		x; //fixed point (PIX_FIXED_MATH_PREC)
    PIX_INT		y; //... 
    PIX_INT		z; //...
    PIX_INT		tx; //...
    PIX_INT		ty; //...
};

struct pix_vm_filter
{
    int a_count; //number of feedforward filter coefficients
    int b_count; //number of feedback filter coefficients
    bool int_coefs;
    int rshift;
    PIX_INT* a_i; //feedforward filter coefficients
    PIX_INT* b_i; //feedback filter coefficients
    PIX_FLOAT* a_f;
    PIX_FLOAT* b_f;
    int input_state_size;
    int output_state_size;
    uint input_state_ptr;
    uint output_state_ptr;
    void* input_state;
    void* output_state;
};

struct pix_vm_resize_pars
{
    void* dest;
    void* src;
    int type;
    uint resize_flags;
    PIX_INT dest_xsize;
    PIX_INT dest_ysize;
    PIX_INT src_xsize;
    PIX_INT src_ysize;
    PIX_INT dest_x;
    PIX_INT dest_y;
    PIX_INT dest_rect_xsize;
    PIX_INT dest_rect_ysize;
    PIX_INT src_x;
    PIX_INT src_y;
    PIX_INT src_rect_xsize;
    PIX_INT src_rect_ysize;
};

struct pix_vm_conv_filter_pars
{
    pix_vm_container* dest;
    pix_vm_container* src;
    pix_vm_container* kernel;
    PIX_INT dest_x;
    PIX_INT dest_y;
    PIX_INT src_x;
    PIX_INT src_y;
    PIX_INT xsize;
    PIX_INT ysize;
    int xstep;
    int ystep;
    int kernel_xcenter;
    int kernel_ycenter;
    PIX_VAL div;
    uint8_t div_type;
    int div_rshift;
    PIX_VAL offset;
    uint8_t offset_type;
    uint flags;
};

#ifndef PIX_NOSUNVOX
struct pix_vm_sunvox
{
    pix_vm* vm;
    sunvox_engine* s;
    int slot;
    int lock_count;
    int sample_rate;
    bool suspended;
    int last_frames;
    int last_latency; //frames
    stime_ticks_t last_out_t;
    stime_ticks_t evt_t;
    bool evt_t_set;
    smutex mutex;
    uint32_t flags; //PIX_SV_INIT_FLAG_*
};
#endif

enum //EVT_PIXICMD + evt->x:
{
    pix_sundog_req_filedialog = 0, //modal (thread is blocked until this dialog is closed)
    pix_sundog_req_preferences,
    pix_sundog_req_vsync, //evt->y: 0 - disable; 1 - enable;
    pix_sundog_req_webserver, //modal
    pix_sundog_req_textinput, //modal
    pix_sundog_req_vcap, //blocking; evt->y: 0 - start; 1 - stop;
    pix_sundog_req_midiopt,
};
struct pix_sundog_filedialog
{
    char* name;
    char* mask;
    char* id;
    char* def_name;
    uint32_t flags;
    volatile char* result;
    volatile int handled;
};
struct pix_sundog_textinput
{
    char* name;
    char* def_str;
    volatile char* result;
    volatile int handled;
};
struct pix_sundog_vcap
{
    int fps;
    int bitrate_kb;
    uint32_t flags;
    volatile int err;
    volatile int handled;
};

struct pix_vm //Pixilang virtual machine
{
    bool		ready;
    
    PIX_OPCODE*		code;
    PIX_PC		code_ptr;
    PIX_PC		code_size;
    PIX_PC		halt_addr; //Address of HALT instruction; functions must returns to this addr.

    pix_stack_val*	vars; //Global variables
    char**		var_names; //Global variable names
    size_t		vars_num; //Number of global variables
    
    pix_vm_container**	c; //Containers
    size_t		c_num;
    smutex		c_mutex;
    PIX_CID		c_counter;
    bool		c_show_debug_messages;
    bool		c_ignore_mutex;
    
    int			prev_frame_res;
    int			fps;
    int			fps_counter;
    stime_ms_t		fps_time;
    PIX_CID		screen; //Container with the main Pixilang screen
    COLORPTR		screen_ptr;
    volatile int	screen_redraw_request;
    volatile int	screen_redraw_answer;
    volatile int	screen_redraw_counter;
    int			screen_change_x;
    int			screen_change_y;
    int			screen_change_xsize;
    int			screen_change_ysize;
    int			screen_xsize;
    int			screen_ysize;
    uint16_t		pixel_size;
    PIX_CID		zbuf;
    
    uint8_t		transp; //Opacity
    
    pix_vm_font*	fonts;
    int			fonts_num;
    uint32_t*		text;
    pix_vm_text_line*	text_lines;

    int*		effector_colors_r;
    int*		effector_colors_g;
    int*		effector_colors_b;

    //Coordinate transformation:
    bool		t_enabled;
    PIX_FLOAT		t_matrix[ 4 * 4 * PIX_T_MATRIX_STACK_SIZE ];
    int			t_matrix_sp;
    
    PIX_ADDR		gl_callback;
    pix_stack_val	gl_userdata;
    COLOR		gl_temp_screen;
    
    //Audio stream:
    sundog_sound*	audio;
    bool		audio_external; //true = audio object was created outside the Pixilang
    int			audio_retain_count; //0 = audio object is not used by anyone inside the Pixilang
    int			audio_slot;
    PIX_ADDR		audio_callback;
    pix_stack_val	audio_userdata;
    int			audio_freq;
    pix_container_type	audio_format;
    int			audio_channels;
    uint		audio_flags;
    int			audio_input_enabled;
    uint		audio_src_ptr; //infinite; fixed point
    uint		audio_src_rendered; //infinite
    void*		audio_src_buffers[ PIX_VM_AUDIO_CHANNELS ];
    uint		audio_src_buffer_size;
    uint		audio_src_buffer_ptr; //fixed point;
    void*		audio_input_buffers[ PIX_VM_AUDIO_CHANNELS ];
    PIX_CID		audio_channels_cont; //Container with audio output channels
    PIX_CID		audio_input_channels_cont; //Container with audio input channels
    PIX_CID		audio_buffers_conts[ PIX_VM_AUDIO_CHANNELS ]; //Containers with audio output buffers;
    PIX_CID		audio_input_buffers_conts[ PIX_VM_AUDIO_CHANNELS ]; //Containers with audio input buffers;
    
#ifndef PIX_NOSUNVOX
    //SunVox:
    pix_vm_sunvox*	sv[ PIX_VM_SUNVOX_STREAMS ];
#endif
    
    uint*		timers; //User defined timers
    int			timers_num;
    
    int16_t            	events_count; //Number of events to execute
    int16_t	       	current_event_num;
    pix_vm_event    	events[ PIX_VM_EVENTS ];
    smutex    		events_mutex;
    PIX_CID		event; //(container)
    int8_t		quit_action; //Action on the QUIT event: 0 - none; 1 - close virtual machine (default).

    uint32_t		random;
    
    char*		log_buffer; //Cyclic buffer for the log messages
    size_t		log_filled; //Number of filled bytes
    size_t		log_ptr;
    char*		log_prev_msg; //Previous message
    size_t		log_prev_msg_len;
    int			log_prev_msg_repeat_cnt;
    char		log_temp_str[ 4096 ];
    smutex		log_mutex;
    
    char*		compiler_errors;
    
    sfs_file		virt_disk0;
    
    char*		base_path;
    
    PIX_CID		current_path; //Container
    PIX_CID		user_path; //Container
    PIX_CID		temp_path; //Container
    
    PIX_CID		os_name; //Container
    PIX_CID		arch_name; //...
    PIX_CID		lang_name; //...
    
    pix_vm_thread*	th[ PIX_VM_THREADS ]; //Threads
    smutex		th_mutex;
    
    sundog_state*	in_state;
    sfs_file		in_state_f;
    sfs_file		out_state_f;

    window_manager*		wm; //SunDog Window Manager
    WINDOWPTR			win; //SunDog window with Pixilang VM
    //Data of requests for some SunDog-based UI functions:
    pix_sundog_filedialog*	sd_filedialog;
    volatile int		sd_webserver_closed;
    pix_sundog_textinput*	sd_textinput;
    pix_sundog_vcap*		sd_vcap;
    
#ifdef OPENGL
    smutex		gl_unused_data_mutex; //to be able to remove GL data for objs A and B at the same time; or remove A while drawing B;
    uint*		gl_unused_textures;
    volatile int	gl_unused_textures_count;
    uint*		gl_unused_framebuffers;
    volatile int	gl_unused_framebuffers_count;
    gl_program_struct**	gl_unused_progs;
    volatile int	gl_unused_progs_count;
    int                 gl_transform_counter;
    float		gl_wm_transform[ 4 * 4 ];
    float		gl_wm_transform_prev[ 4 * 4 ];
    bool		gl_no_2d_line_shift; //don't shift the lines and dots for per-pixel accuracy
    GLuint              gl_vshader_solid;
    GLuint              gl_vshader_gradient;
    GLuint              gl_vshader_tex_solid;
    GLuint              gl_vshader_tex_gradient;
    GLuint              gl_fshader_solid;
    GLuint              gl_fshader_gradient;
    GLuint              gl_fshader_tex_alpha_solid;
    GLuint              gl_fshader_tex_alpha_gradient;
    GLuint              gl_fshader_tex_rgba_solid;
    GLuint              gl_fshader_tex_rgba_gradient;
    gl_program_struct*  gl_current_prog;
    gl_program_struct*  gl_user_defined_prog;
    gl_program_struct*  gl_prog_solid;
    gl_program_struct*  gl_prog_gradient;
    gl_program_struct*  gl_prog_tex_alpha_solid;
    gl_program_struct*  gl_prog_tex_alpha_gradient;
    gl_program_struct*  gl_prog_tex_rgba_solid;
    gl_program_struct*  gl_prog_tex_rgba_gradient;
#endif
};

#define PIX_BUILTIN_FN_PARAMETERS int fn_num, int pars_num, PIX_SP sp, pix_vm_thread* th, pix_vm* vm
typedef void (*pix_builtin_fn)( PIX_BUILTIN_FN_PARAMETERS );

extern const char* g_pix_fn_names[];
extern pix_builtin_fn g_pix_fns[];

//
// LEVEL 0. Pixilang main; global init/deinit
//

int pix_global_init( void );
int pix_global_deinit( void );

//
// LEVEL 1. Virtual machine (process)
//

//Thread-safe functions:
//  pix_vm_new_container;
//  pix_vm_send_event;
//  pix_vm_get_event.

extern const char* g_pix_container_type_names[];
extern const int g_pix_container_type_sizes[];

int pix_vm_init( pix_vm* vm, WINDOWPTR win );
int pix_vm_deinit( pix_vm* vm );
void pix_vm_log( char* message, pix_vm* vm );
#define PIX_VM_LOG( fmt, ARGS... ) \
{ \
    bool use_mutex; if( vm->log_buffer ) use_mutex = 1; else use_mutex = 0; \
    if( use_mutex ) smutex_lock( &vm->log_mutex ); \
    sprintf( vm->log_temp_str, fmt, ## ARGS ); pix_vm_log( vm->log_temp_str, vm ); \
    if( use_mutex) smutex_unlock( &vm->log_mutex ); \
}
void pix_vm_put_opcode( PIX_OPCODE opcode, pix_vm* vm );
void pix_vm_put_int( PIX_INT v, pix_vm* vm );
void pix_vm_put_float( PIX_FLOAT v, pix_vm* vm );
char* pix_vm_get_variable_name( pix_vm* vm, size_t vnum );
void pix_vm_resize_variables( pix_vm* vm );
int pix_vm_send_event(
    int16_t type,
    int16_t flags,
    int16_t x,
    int16_t y,
    int16_t key,
    int16_t scancode,
    int16_t pressure,
    pix_vm* vm );
int pix_vm_send_event(
    int16_t type,
    int16_t flags,
    pix_vm* vm );
int pix_vm_get_event( pix_vm* vm );
int pix_vm_create_active_thread( int thread_num, pix_vm* vm );
int pix_vm_destroy_thread( int thread_num, PIX_INT timeout, pix_vm* vm );
int pix_vm_get_thread_retval( int thread_num, pix_vm* vm, pix_stack_val* retval );
int pix_vm_run( 
    int thread_num, 
    bool open_new_thread, 
    pix_vm_function* fun, 
    pix_vm_run_mode mode, 
    pix_vm* vm );
void pix_vm_set_systeminfo_containers( pix_vm* vm );
void pix_vm_set_pixiinfo( pix_vm* vm );
int pix_vm_save_code( const char* name, pix_vm* vm );
int pix_vm_load_code( const char* name, char* base_path, pix_vm* vm );

int pix_vm_set_audio_callback( PIX_ADDR callback, pix_stack_val userdata, uint freq, pix_container_type format, int channels, uint flags, pix_vm* vm );
int pix_vm_get_audio_sample_rate( int source, pix_vm* vm );

int pix_vm_sv_new( int sample_rate, uint32_t flags, pix_vm* vm );
int pix_vm_sv_remove( int sv_id, pix_vm* vm );
int pix_vm_sv_get_sample_rate( int sv_id, pix_vm* vm );
int pix_vm_sv_render( int sv_id, sunvox_render_data* rdata, pix_vm* vm );
int pix_vm_sv_stream_control( int sv_id, int cmd, pix_vm* vm );
int pix_vm_sv_fload( int sv_id, sfs_file f, pix_vm* vm );
int pix_vm_sv_fsave( int sv_id, sfs_file f, pix_vm* vm );
int pix_vm_sv_play( int sv_id, int pos, bool jump_to_pos, pix_vm* vm );
int pix_vm_sv_stop( int sv_id, pix_vm* vm );
int pix_vm_sv_pause( int sv_id, pix_vm* vm );
int pix_vm_sv_resume( int sv_id, pix_vm* vm );
int pix_vm_sv_sync_resume( int sv_id, pix_vm* vm );
int pix_vm_sv_set_autostop( int sv_id, bool autostop, pix_vm* vm );
int pix_vm_sv_get_autostop( int sv_id, pix_vm* vm );
int pix_vm_sv_get_status( int sv_id, pix_vm* vm );
int pix_vm_sv_rewind( int sv_id, int pos, pix_vm* vm );
int pix_vm_sv_volume( int sv_id, int vol, pix_vm* vm );
int pix_vm_sv_set_event_t( int sv_id, int set, int t, pix_vm* vm );
int pix_vm_sv_send_event( int sv_id, int track, int note, int vel, int mod, int ctl, int ctl_val, pix_vm* vm );
int pix_vm_sv_get_current_line( int sv_id, pix_vm* vm );
int pix_vm_sv_get_current_signal_level( int sv_id, int ch, pix_vm* vm );
const char* pix_vm_sv_get_name( int sv_id, pix_vm* vm );
int pix_vm_sv_set_name( int sv_id, char* name, pix_vm* vm );
int pix_vm_sv_get_proj_par( int sv_id, int p, pix_vm* vm ); //0 - BPM; 1 - TPL;
int pix_vm_sv_get_proj_len( int sv_id, int t, pix_vm* vm ); //0 - frames; 1 - lines;
int pix_vm_sv_get_time_map( int sv_id, int start_line, int len, uint32_t* dest, int flags, pix_vm* vm );
int pix_vm_sv_new_module( int sv_id, char* name, char* type, int x, int y, int z, pix_vm* vm );
int pix_vm_sv_remove_module( int sv_id, int mod, pix_vm* vm );
int pix_vm_sv_connect_module( int sv_id, int src, int dst, bool disconnect, pix_vm* vm );
int pix_vm_sv_fload_module( int sv_id, sfs_file f, int x, int y, int z, pix_vm* vm );
int pix_vm_sv_mod_fload( int sv_id, int modtype, int mod, int slot, sfs_file f, pix_vm* vm );
int pix_vm_sv_sampler_par( int sv_id, int mod, int slot, int par, int par_val, int set, pix_vm* vm );
int pix_vm_sv_get_number_of_modules( int sv_id, pix_vm* vm );
int pix_vm_sv_find_module( int sv_id, char* name, pix_vm* vm );
int pix_vm_sv_selected_module( int sv_id, int mod, pix_vm* vm );
int pix_vm_sv_get_module_flags( int sv_id, int mod, pix_vm* vm );
int* pix_vm_sv_get_module_inouts( int sv_id, int mod, bool out, int* num, pix_vm* vm );
const char* pix_vm_sv_get_module_type( int sv_id, int mod, pix_vm* vm );
const char* pix_vm_sv_get_module_name( int sv_id, int mod, pix_vm* vm );
int pix_vm_sv_set_module_name( int sv_id, int mod, char* name, pix_vm* vm );
uint32_t pix_vm_sv_get_module_xy( int sv_id, int mod, pix_vm* vm );
int pix_vm_sv_set_module_xy( int sv_id, int mod, int x, int y, pix_vm* vm );
COLOR pix_vm_sv_get_module_color( int sv_id, int mod, pix_vm* vm );
int pix_vm_sv_set_module_color( int sv_id, int mod, COLOR color, pix_vm* vm );
uint32_t pix_vm_sv_get_module_finetune( int sv_id, int mod, pix_vm* vm );
int pix_vm_sv_set_module_finetune( int sv_id, int mod, int finetune, pix_vm* vm );
int pix_vm_sv_set_module_relnote( int sv_id, int mod, int relative_note, pix_vm* vm );
int pix_vm_sv_get_module_scope( int sv_id, int mod, int ch, pix_vm_container* dest_cont, int samples_to_read, pix_vm* vm );
int pix_vm_sv_module_curve( int sv_id, int mod, int curve_num, pix_vm_container* data_cont, int len, int w, pix_vm* vm );
int pix_vm_sv_get_module_ctl_cnt( int sv_id, int mod, pix_vm* vm );
const char* pix_vm_sv_get_module_ctl_name( int sv_id, int mod, int ctl, pix_vm* vm );
int pix_vm_sv_get_module_ctl_value( int sv_id, int mod, int ctl, int scaled, pix_vm* vm );
int pix_vm_sv_set_module_ctl_value( int sv_id, int mod, int ctl, int val, int scaled, pix_vm* vm );
int pix_vm_sv_get_module_ctl_par( int sv_id, int mod, int ctl, int scaled, int par, pix_vm* vm );
int pix_vm_sv_new_pat( int sv_id, int clone, int x, int y, int tracks, int lines, int icon_seed, char* name, pix_vm* vm );
int pix_vm_sv_remove_pat( int sv_id, int pat, pix_vm* vm );
int pix_vm_sv_get_number_of_pats( int sv_id, pix_vm* vm );
int pix_vm_sv_find_pattern( int sv_id, char* name, pix_vm* vm );
int pix_vm_sv_get_pat( int sv_id, int pat, sunvox_pattern** out_pat_data, sunvox_pattern_info** out_pat_info, pix_vm* vm );
int pix_vm_sv_set_pat_xy( int sv_id, int pat, int x, int y, pix_vm* vm );
int pix_vm_sv_set_pat_size( int sv_id, int pat, int tracks, int lines, pix_vm* vm );
int pix_vm_sv_set_pat_name( int sv_id, int pat, char* name, pix_vm* vm );
int pix_vm_sv_pat_mute( int sv_id, int pat, int mute, pix_vm* vm );

void pix_vm_call_builtin_function( int fn_num, int pars_num, PIX_SP sp, pix_vm_thread* th, pix_vm* vm );

PIX_CID pix_vm_new_container( PIX_CID cnum, PIX_INT xsize, PIX_INT ysize, int type, void* data, pix_vm* vm );
void pix_vm_remove_container( PIX_CID cnum, pix_vm* vm );
int pix_vm_resize_container( PIX_CID cnum, PIX_INT xsize, PIX_INT ysize, int type, uint flags, pix_vm* vm );
int pix_vm_rotate_block( void** ptr, PIX_INT* xsize, PIX_INT* ysize, int type, int angle, void* save_to ); //angle*90 degrees (clockwise (по часовой стрелке))
int pix_vm_rotate_container( PIX_CID cnum, int angle, pix_vm* vm );
int pix_vm_convert_container_type( PIX_CID cnum, int type, pix_vm* vm );
void pix_vm_clean_container( PIX_CID cnum, int8_t v_type, PIX_VAL v, PIX_INT offset, PIX_INT size, pix_vm* vm );
PIX_CID pix_vm_clone_container( PIX_CID cnum, pix_vm* vm );
PIX_CID pix_vm_zlib_pack_container( PIX_CID cnum, int level, pix_vm* vm );
PIX_CID pix_vm_zlib_unpack_container( PIX_CID cnum, pix_vm* vm );
PIX_INT pix_vm_get_container_int_element( PIX_CID cnum, size_t elnum, pix_vm* vm );
PIX_FLOAT pix_vm_get_container_float_element( PIX_CID cnum, size_t elnum, pix_vm* vm );
void pix_vm_set_container_int_element( PIX_CID cnum, size_t elnum, PIX_INT val, pix_vm* vm );
void pix_vm_set_container_float_element( PIX_CID cnum, size_t elnum, PIX_FLOAT val, pix_vm* vm );
size_t pix_vm_get_container_strlen( PIX_CID cnum, size_t offset, pix_vm* vm );
char* pix_vm_make_cstring_from_container( PIX_CID cnum, bool* need_to_free, pix_vm* vm );
PIX_CID pix_vm_make_container_from_cstring( const char* str, pix_vm* vm );
pix_sym* pix_vm_get_container_property( PIX_CID cnum, const char* prop_name, int prop_hash, pix_vm* vm );
PIX_INT pix_vm_get_container_property_i( PIX_CID cnum, const char* prop_name, int prop_hash, pix_vm* vm );
void pix_vm_set_container_property( PIX_CID cnum, const char* prop_name, int prop_hash, int8_t val_type, PIX_VAL val, pix_vm* vm );
void* pix_vm_get_container_hdata( PIX_CID cnum, pix_vm* vm );
int pix_vm_create_container_hdata( PIX_CID cnum, uint8_t hdata_type, size_t hdata_size, pix_vm* vm );
void pix_vm_remove_container_hdata( PIX_CID cnum, pix_vm* vm );
size_t pix_vm_get_container_hdata_size( PIX_CID cnum, pix_vm* vm );
size_t pix_vm_save_container_hdata( PIX_CID cnum, sfs_file f, pix_vm* vm );
size_t pix_vm_load_container_hdata( PIX_CID cnum, sfs_file f, pix_vm* vm );
int pix_vm_clone_container_hdata( PIX_CID new_cnum, PIX_CID old_cnum, pix_vm* vm );
PIX_INT pix_vm_container_get_cur_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_get_frame_count( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_get_frame_size( PIX_CID cnum, int cur_frame, pix_container_type* type, int* xsize, int* ysize, pix_vm* vm );
int pix_vm_container_hdata_unpack_frame_to_buf( PIX_CID cnum, int cur_frame, COLORPTR buf, pix_vm* vm );
int pix_vm_container_hdata_unpack_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_pack_frame_from_buf( PIX_CID cnum, int cur_frame, COLORPTR buf, pix_container_type type, int xsize, int ysize, pix_vm* vm );
int pix_vm_container_hdata_pack_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_clone_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_remove_frame( PIX_CID cnum, pix_vm* vm );
int pix_vm_container_hdata_autoplay_control( PIX_CID cnum, pix_vm* vm );

inline pix_vm_container* pix_vm_get_container( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        return vm->c[ cnum ];
    }
    return 0;
}
inline void* pix_vm_get_container_data( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            return c->data;
        }
    }
    return 0;
}
inline int pix_vm_get_container_flags( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            return c->flags;
        }
    }
    return 0;
}
inline void pix_vm_set_container_flags( PIX_CID cnum, uint flags, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->flags = flags;
        }
    }
}
inline void pix_vm_mix_container_flags( PIX_CID cnum, uint flags, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->flags |= flags;
        }
    }
}
inline COLOR pix_vm_get_container_key_color( PIX_CID cnum, pix_vm* vm )
{   
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            return c->key;
        }
    }
    return 0;
}
inline void pix_vm_set_container_key_color( PIX_CID cnum, COLOR key, pix_vm* vm )
{   
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->key = key;
            c->flags |= PIX_CONTAINER_FLAG_USES_KEY;
        }
    }
}
inline void pix_vm_remove_container_key_color( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->flags &= ~PIX_CONTAINER_FLAG_USES_KEY;
        }
    }
}
inline PIX_CID pix_vm_get_container_alpha( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            return c->alpha;
        }
    }
    return 0;
}
inline void* pix_vm_get_container_alpha_data( PIX_CID cnum, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            if( (unsigned)c->alpha < (unsigned)vm->c_num )
            {
                pix_vm_container* alpha_cont = vm->c[ c->alpha ];
                if( alpha_cont )
                {
            	    return alpha_cont->data;
            	}
            }
        }
    }    
    return 0;
}
inline void pix_vm_set_container_alpha( PIX_CID cnum, PIX_CID alpha, pix_vm* vm )
{
    if( (unsigned)cnum < (unsigned)vm->c_num )
    {
        pix_vm_container* c = vm->c[ cnum ];
        if( c )
        {
            c->alpha = alpha;
        }
    }
}

void pix_vm_op_cn( int opcode, PIX_CID cnum, int8_t val_type, PIX_VAL val, PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, PIX_VAL* retval, int8_t* retval_type, pix_vm* vm );
PIX_INT pix_vm_op_cc( int opcode, PIX_CID cnum1, PIX_CID cnum2, PIX_INT dest_x, PIX_INT dest_y, PIX_INT src_x, PIX_INT src_y, PIX_INT xsize, PIX_INT ysize, pix_vm* vm );
PIX_INT pix_vm_op_ccn( int opcode, PIX_CID cnum1, PIX_CID cnum2, int8_t val_type, PIX_VAL val, PIX_INT dest_x, PIX_INT dest_y, PIX_INT src_x, PIX_INT src_y, PIX_INT xsize, PIX_INT ysize, pix_vm* vm );
PIX_INT pix_vm_generator( int opcode, PIX_CID cnum, PIX_FLOAT* fval, PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, pix_vm* vm );
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
    pix_vm* vm );
PIX_INT pix_vm_sampler( pix_vm_container* pars_cont, pix_vm* vm );
PIX_INT pix_vm_envelope2p( PIX_CID cnum, PIX_INT v1, PIX_INT v2, PIX_INT offset, PIX_INT size, int8_t dc_off1_type, PIX_VAL dc_off1, int8_t dc_off2_type, PIX_VAL dc_off2, pix_vm* vm );
PIX_CID pix_vm_new_filter( uint flags, pix_vm* vm );
void pix_vm_remove_filter( PIX_CID f_c, pix_vm* vm );
PIX_INT pix_vm_init_filter( PIX_CID f_c, PIX_CID a_c, PIX_CID b_c, int rshift, uint flags, pix_vm* vm );
void pix_vm_reset_filter( PIX_CID f_c, pix_vm* vm );
PIX_INT pix_vm_apply_filter( PIX_CID f_c, PIX_CID output_c, PIX_CID input_c, uint flags, PIX_INT offset, PIX_INT size, pix_vm* vm );
void pix_vm_copy_and_resize( pix_vm_resize_pars* pars );
PIX_INT pix_vm_conv_filter( pix_vm* vm, pix_vm_conv_filter_pars* pars );

void pix_vm_gfx_set_screen( PIX_CID cnum, pix_vm* vm );
void pix_vm_gfx_matrix_reset( pix_vm* vm );
void pix_vm_gfx_matrix_mul( PIX_FLOAT* res, PIX_FLOAT* m1, PIX_FLOAT* m2 );
void pix_vm_gfx_vertex_transform( PIX_FLOAT* v, PIX_FLOAT* m );
void pix_vm_gfx_draw_line( PIX_INT x1, PIX_INT y1, PIX_INT x2, PIX_INT y2, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_line_zbuf( PIX_INT x1, PIX_INT y1, PIX_INT z1, PIX_INT x2, PIX_INT y2, PIX_INT z2, COLOR color, int* zbuf, pix_vm* vm );
void pix_vm_gfx_draw_box( PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_fbox( PIX_INT x, PIX_INT y, PIX_INT xsize, PIX_INT ysize, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_container( PIX_CID cnum, PIX_FLOAT x, PIX_FLOAT y, PIX_FLOAT xsize, PIX_FLOAT ysize, PIX_INT tx, PIX_INT ty, PIX_INT txsize, PIX_INT tysize, COLOR color, pix_vm* vm );
int* pix_vm_gfx_get_zbuf( pix_vm* vm );
void pix_vm_gfx_draw_text( char* str, PIX_INT str_size, PIX_FLOAT x, PIX_FLOAT y, int align, COLOR color, int max_xsize, int* out_xsize, int* out_ysize, bool dont_draw, pix_vm* vm );
void pix_vm_gfx_draw_triangle( pix_vm_ivertex* v1, pix_vm_ivertex* v2, pix_vm_ivertex* v3, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_triangle_zbuf( pix_vm_ivertex* v1, pix_vm_ivertex* v2, pix_vm_ivertex* v3, COLOR color, pix_vm* vm );
void pix_vm_gfx_draw_triangle_t( PIX_FLOAT* v1f, PIX_FLOAT* v2f, PIX_FLOAT* v3f, PIX_CID cnum, COLOR color, pix_vm* vm );

inline pix_vm_font* pix_vm_get_font_for_char( uint32_t c, pix_vm* vm )
{
    for( int n = 0; n < vm->fonts_num; n++ )
    {
        pix_vm_font* font = &vm->fonts[ n ];
        if( (unsigned)font->font < (unsigned)vm->c_num )
        {
            if( c >= font->first && c <= font->last )
            {
        	return font;
            }
        }
    }
    return NULL;
}
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
    pix_vm* vm );

PIX_CID pix_vm_load( const char* filename, sfs_file f, int par1, pix_vm* vm );
int pix_vm_save( PIX_CID cnum, const char* filename, sfs_file f, int format, int par1, pix_vm* vm );

#ifdef OPENGL
int pix_vm_gl_init( pix_vm* vm );
void pix_vm_gl_deinit( pix_vm* vm );
void pix_vm_gl_matrix_set( pix_vm* vm );
void pix_vm_gl_program_reset( pix_vm* vm );
void pix_vm_gl_use_prog( gl_program_struct* p, pix_vm* vm );
pix_vm_container_gl_data* pix_vm_get_container_gl_data( PIX_CID cnum, pix_vm* vm );
pix_vm_container_gl_data* pix_vm_create_container_gl_data( PIX_CID cnum, pix_vm* vm );
void pix_vm_remove_container_gl_data( PIX_CID cnum, pix_vm* vm );
void pix_vm_update_gl_texture_data( PIX_CID cnum, pix_vm* vm );
#endif

//
// LEVEL 2. Pixilang compiler (text source -> virtual code)
//

//Load *.pixicode file or compile *.pixi source file
int pix_load( const char* name, pix_vm* vm );

//
// LEVEL X. Symbol table
//

int pix_symtab_init( int size_level, pix_symtab* st ); //size_level: 0...15 - level; 15... - real symtab size (prime number)
int pix_symtab_hash( const char* name, int size );
pix_sym* pix_symtab_lookup( const char* name, int hash, bool create, pix_sym_type type, PIX_INT ival, PIX_FLOAT fval, bool* created, pix_symtab* st );
pix_sym* pix_sym_clone( pix_sym* s );
int pix_symtab_clone( pix_symtab* dest_st, pix_symtab* src_st );
pix_sym* pix_symtab_get_list( pix_symtab* st );
int pix_symtab_deinit( pix_symtab* st );

//
// LEVEL X. Misc
//

char* pix_get_base_path( const char* src_name );
char* pix_compose_full_path( char* base_path, char* file_name, pix_vm* vm );
void pix_str_to_num( const char* str, int str_len, PIX_VAL* v, int8_t* t, pix_vm* vm );
