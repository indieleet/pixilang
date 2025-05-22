%pure-parser
%parse-param { pix_compiler* pcomp }
%lex-param { pix_compiler* pcomp }

%{
/*
    pixilang_compiler.y
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"
#include "pixilang_font.h"
#include "video/video.h"

#ifndef PIX_NOSUNVOX
    #include "sunvox_engine.h"
#endif

#include "zlib.h"

#ifdef PIX_CODE_ANALYZER_ENABLED
    #include "pixilang_vm_code_analyzer.hpp"
#endif

//#define SHOW_DEBUG_MESSAGES

#ifdef SHOW_DEBUG_MESSAGES
    #define DPRINT( fmt, ARGS... ) slog( fmt, ## ARGS )
#else
    #define DPRINT( fmt, ARGS... ) {}
#endif

#define ERROR( fmt, ARGS... ) slog( "ERROR in %s() (line %d): " fmt "\n", __FUNCTION__, __LINE__, ## ARGS )
#define PCOMP_ERROR( fmt, ARGS... ) \
{ \
    int ts_len = smem_strlen( pcomp->src_name ) + 2048; \
    char* ts = (char*)smem_new( ts_len ); \
    slog( "ERROR (line %d) in %s: " fmt "\n", pcomp->src_line + 1, pcomp->src_name, ## ARGS ); \
    snprintf( ts, ts_len, "ERROR (line %d)\nin %s:\n" fmt, pcomp->src_line + 1, pcomp->src_name, ## ARGS ); \
    if( pcomp->vm->compiler_errors ) smem_strcat_resize( pcomp->vm->compiler_errors, "\n" ); \
    smem_strcat_resize( pcomp->vm->compiler_errors, ts ); \
    smem_free( ts ); \
}

#define NUMERIC( val ) ( val >= '0' && val <= '9' )
#define ABC( val ) ( ( val >= 'a' && val <= 'z' ) || ( val >= 'A' && val <= 'Z' ) || ( (unsigned)val >= 128 && val != -1 ) )

enum
{ //sub nodes
    LNODE_WHILE_INIT_STATLIST = 0, //may be NULL
    LNODE_WHILE_COND_EXPR, //loop execution condition (expression)
    LNODE_WHILE_JMP_TO_END, //jump to end (break the loop)
    LNODE_WHILE_BODY_STATLIST,
    LNODE_WHILE_STEP_STATLIST, //may be NULL
    LNODE_WHILE_JMP_TO_START, //jump to start (condition or body)
    LNODE_WHILE_SIZE
};

enum lnode_type
{
    lnode_empty,
    
    lnode_statlist, //statements
    
    lnode_int,
    lnode_float,
    lnode_var,
    
    lnode_halt,
    
    lnode_label,
    lnode_function_label_from_node,
    
    lnode_go,
    
    lnode_jmp_to_node, //jump to node (lnode*)val->p
    lnode_jmp_to_end_of_node,
    
    lnode_if,
    lnode_if_else,
    lnode_while,
    
    lnode_save_to_var,
    
    lnode_save_to_prop,
    lnode_load_from_prop,

    lnode_save_to_mem,
    lnode_load_from_mem,
    
    lnode_save_to_stackframe,
    lnode_load_from_stackframe,
    
    lnode_sub,
    lnode_add,
    lnode_mul,
    lnode_idiv,
    lnode_div,
    lnode_mod,
    lnode_and,
    lnode_or,
    lnode_xor,
    lnode_andand,
    lnode_oror,
    lnode_eq,
    lnode_neq,
    lnode_less,
    lnode_leq,
    lnode_greater,
    lnode_geq,
    lnode_lshift,
    lnode_rshift,
    
    lnode_neg,
    lnode_lnot,
    lnode_bnot,

    lnode_exprlist,
    lnode_call_builtin_fn,
    lnode_call_builtin_fn_void,
    lnode_call,
    lnode_call_void,
    lnode_ret_int,
    lnode_ret,
    lnode_inc_sp,
};

#define LNODE_FLAG_STATLIST_AS_EXPRESSION		( 1 << 0 )
#define LNODE_FLAG_STATLIST_WITH_JMP_HEADER		( 1 << 1 ) //add "skip statlist" code (header)
#define LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER	( 1 << 2 ) //add "skip statlist if false" code (header)
#define LNODE_FLAG_STATLIST_SKIP_NEXT_HEADER		( 1 << 3 ) //option for previous flags
#define LNODE_FLAG_JMP_IF_FALSE				( 1 << 4 )

union lnode_val
{
    PIX_INT i;
    PIX_FLOAT f;
    void* p;
};

struct lnode
{
    lnode_type type;
    uint8_t flags;
    size_t code_ptr; //Start of node
    size_t code_ptr2; //End of node (start of the next node)
    lnode_val val;
    lnode** n; //Children
    uint nn; //Children count
};

#define VAR_FLAG_LABEL					1
#define VAR_FLAG_FUNCTION				2
#define VAR_FLAG_INITIALIZED				4
#define VAR_FLAG_USED					8

#define LVAR_OFFSET 3

struct pix_lsymtab
{
    pix_symtab* lsym; //Local symbol table
    uint lvars_num; //Number of local variables
    char* lvar_flags; //Flags of local variables
    char** lvar_names;
    size_t lvar_flags_size;
    int pars_num; //Number of local function parameters
};

struct pix_include
{
    char* src;
    int src_ptr;
    int src_line;
    int src_size;
    char* src_name;
    char* base_path;
};

struct pix_compiler
{
    lnode* root; //Root lexical node
    
    char* src;
    int src_ptr;
    int src_line;
    int src_size;
    char* src_name;
    
    pix_symtab sym; //Global symbol table
    pix_lsymtab* lsym; //Local symbol tables
    int lsym_num; //Number of current local symbol table
    char temp_sym_name[ 256 + 1 ];
    
    char* var_flags;
    size_t var_flags_size;

    int statlist_header_size; //Maximum length of JMP instruction
    
    bool fn_pars_mode; //Treat lvars as function parameters
    bool for_pars_mode; //Treat ';' as FOR LOOP delimiter
    
    char* base_path; //May be not equal to vm->base_path
    
    pix_include* inc; //Stack for includes
    uint inc_num;
    
    lnode** while_stack;
    uint while_stack_ptr;
    
    lnode** fixup;
    uint fixup_num;
    
    pix_vm* vm;
};

static void push_int( pix_compiler* pcomp, PIX_INT v );
static lnode* node( lnode_type type, uint nn );
static void resize_node( lnode* n, uint nn );
static lnode* clone_tree( lnode* n );
static void remove_tree( lnode* n );
static void clean_tree( lnode* n );

static lnode* make_expr_node_from_var( PIX_INT i )
{
    lnode* n = node( lnode_var, 0 ); 
    n->val.i = i;
    return n;
}

static lnode* make_expr_node_from_local_var( PIX_INT i )
{
    lnode* n = node( lnode_load_from_stackframe, 0 );
    n->val.i = i;
    return n;
}

static void create_empty_lsym_table( pix_compiler* pcomp )
{
    pcomp->lsym_num++;
    if( pcomp->lsym_num >= smem_get_size( pcomp->lsym ) / sizeof( pix_lsymtab ) )
	pcomp->lsym = (pix_lsymtab*)smem_resize( pcomp->lsym, ( pcomp->lsym_num + 8 ) * sizeof( pix_lsymtab ) );
    pix_lsymtab* l = &pcomp->lsym[ pcomp->lsym_num ];
    smem_clear( l, sizeof( pix_lsymtab ) );
    l->lvar_flags = (char*)smem_znew( 8 );
    l->lvar_names = (char**)smem_znew( 8 * sizeof( char* ) );
    l->lvar_flags_size = 8;
}

static lnode* remove_lsym_table( pix_compiler* pcomp, lnode* statlist )
{
    lnode* new_tree = statlist;
    int lvars_num = pcomp->lsym[ pcomp->lsym_num ].lvars_num;
    if( lvars_num )
    {
	new_tree = node( lnode_statlist, 2 );
	new_tree->n[ 0 ] = node( lnode_inc_sp, 0 );
	new_tree->n[ 0 ]->val.i = -lvars_num;
	new_tree->n[ 1 ] = statlist;
    }

    pix_lsymtab* l = &pcomp->lsym[ pcomp->lsym_num ];
    bool err = false;
    for( size_t n = 0; n < l->lvar_flags_size; n++ )
    {
	if( l->lvar_names[ n ] )
	{
	    if( ( l->lvar_flags[ n ] & VAR_FLAG_INITIALIZED ) == 0 )
	    {
		PCOMP_ERROR( "local variable %s is not initialized", l->lvar_names[ n ] );
		err = true;
	    }
	    smem_free( l->lvar_names[ n ] );
	}
    }
    if( err )
    {
	remove_tree( new_tree );
	new_tree = NULL;
    }
    smem_free( l->lvar_flags );
    smem_free( l->lvar_names );
    l->lvar_flags = NULL;
    l->lvar_names = NULL;
    if( l->lsym )
    {
	pix_symtab_deinit( l->lsym );
	smem_free( l->lsym );
	l->lsym = NULL;
	l->lvars_num = 0;
	l->pars_num = 0;
    }
    pcomp->lsym_num--;

    return new_tree;
}

%}

%union //Possible types for yylval and yyval:
{
    PIX_INT i;
    PIX_FLOAT f;
    lnode* n;
}

%token NUM_I NUM_F GVAR LVAR WHILE FOR BREAK CONTINUE IF ELSE GO RET FNNUM FNDEF INCLUDE HALT
%left OROR ANDAND
%left OR XOR AND
%left EQ NEQ '<' '>' LEQ GEQ
%left LSHIFT RSHIFT
%left '+' '-'
%left '*' '/' IDIV '%' HASH
%nonassoc NEG /* negation--unary minus */
%nonassoc LNOT
%nonassoc BNOT

%%
    //#######################
    //## PIXILANG RULES    ##
    //#######################

    // 2 * 3 * 4 = node2( node1( 2 * 3 ) * 4 )

input
    : /* empty */
    | input stat
        {
            DPRINT( "input stat\n" );
            resize_node( pcomp->root, pcomp->root->nn + 1 );
            pcomp->root->n[ pcomp->root->nn - 1 ] = $2.n;
        }
    ;
lvarslist
    : /* empty */
    | LVAR
    | lvarslist ',' LVAR
    ;
smem_offset
    : expr { $$.n = $1.n; }
    | expr ',' expr 
	{
	    $$.n = node( lnode_empty, 2 );
	    $$.n->n[ 0 ] = $1.n;
	    $$.n->n[ 1 ] = $3.n;
	}
    ;
stat_math_op
    : '-' { $$.i = 0; }
    | '+' { $$.i = 1; }
    | '*' { $$.i = 2; }
    | IDIV { $$.i = 3; }
    | '/' { $$.i = 4; }
    | '%' { $$.i = 5; }
    | AND { $$.i = 6; }
    | OR { $$.i = 7; }
    | XOR { $$.i = 8; }
    | ANDAND { $$.i = 9; }
    | OROR { $$.i = 10; }
    | EQ { $$.i = 11; }
    | NEQ { $$.i = 12; }
    | '<' { $$.i = 13; }
    | LEQ { $$.i = 14; }
    | '>' { $$.i = 15; }
    | GEQ { $$.i = 16; }
    | LSHIFT { $$.i = 17; }
    | RSHIFT { $$.i = 18; }
    ;
statlist
    : /* empty */ { $$.n = node( lnode_statlist, 0 ); }
    | statlist stat
        {
	    DPRINT( "statlist stat\n" );
            resize_node( $1.n, $1.n->nn + 1 );
	    $1.n->n[ $1.n->nn - 1 ] = $2.n;
	    $$.n = $1.n;
        }
    ;
stat
    : HALT { DPRINT( "HALT\n" ); $$.n = node( lnode_halt, 0 ); }
    | GVAR ':' 
	{ 
	    DPRINT( "VAR(%d) :\n", (int)$1.i ); 
	    if( pcomp->var_flags[ $1.i ] & VAR_FLAG_LABEL )
	    {
		PCOMP_ERROR( "label %s is already defined", pix_vm_get_variable_name( pcomp->vm, $1.i ) );
                YYERROR;
	    }
	    if( pcomp->var_flags[ $1.i ] & VAR_FLAG_FUNCTION )
	    {
		PCOMP_ERROR( "label %s is already defined as function", pix_vm_get_variable_name( pcomp->vm, $1.i ) );
                YYERROR;
	    }
	    $$.n = node( lnode_label, 0 );
	    $$.n->val.i = $1.i;
	    pcomp->var_flags[ $1.i ] |= VAR_FLAG_LABEL | VAR_FLAG_INITIALIZED;
	}
    | GO expr { DPRINT( "GO expr\n" ); $$.n = node( lnode_go, 1 ); $$.n->n[ 0 ] = $2.n; }
    | GVAR '=' expr 
        { 
	    DPRINT( "GVAR(%d) = expr\n", (int)$1.i ); 
            $$.n = node( lnode_save_to_var, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = $3.n;
	    pcomp->var_flags[ $1.i ] |= VAR_FLAG_INITIALIZED;
        }
    | LVAR '=' expr 
        { 
	    DPRINT( "LVAR(%d) = expr\n", (int)$1.i );
	    $$.n = node( lnode_save_to_stackframe, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = $3.n;
	    if( $1.i < 0 )
	    {
		int lvar_num = -$1.i - LVAR_OFFSET;
		pcomp->lsym[ pcomp->lsym_num ].lvar_flags[ lvar_num ] |= VAR_FLAG_INITIALIZED;
	    }
        }
    | prop_expr '=' expr 
	{
	    DPRINT( "prop_expr = expr\n" );
	    $$.n = $1.n;
	    $$.n->type = lnode_save_to_prop;
	    resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ 1 ] = $3.n;
	}
    | prop_expr stat_math_op expr 
	{
	    DPRINT( "prop_expr stat_math_op(%d) expr\n", (int)$2.i );
	    //Create math operation:
	    lnode* op = node( (lnode_type)( lnode_sub + $2.i ), 2 ); 
	    op->n[ 0 ] = $1.n;
	    op->n[ 1 ] = $3.n;
	    //Result:
	    $$.n = clone_tree( $1.n );
	    $$.n->type = lnode_save_to_prop;
	    resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ 1 ] = op;
	}
    | mem_expr '=' expr
	{
	    DPRINT( "mem_expr = expr\n" );
	    $$.n = $1.n;
	    $$.n->type = lnode_save_to_mem;
	    resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ 2 ] = $3.n;
        }
    | mem_expr stat_math_op expr
	{
	    DPRINT( "mem_expr stat_math_op(%d) expr\n", (int)$2.i );
	    //Create math operation:
	    lnode* op = node( (lnode_type)( lnode_sub + $2.i ), 2 );
	    op->n[ 0 ] = $1.n;
	    op->n[ 1 ] = $3.n;
	    //Result:
	    $$.n = clone_tree( $1.n );
	    $$.n->type = lnode_save_to_mem;
	    resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ 2 ] = op;
	}
    | GVAR stat_math_op expr 
	{
	    DPRINT( "GVAR(%d) stat_math_op(%d) expr\n", (int)$1.i, (int)$2.i );
	    //Create first operand:
	    lnode* n1 = make_expr_node_from_var( $1.i );
	    //Create math operation:
	    lnode* n2 = node( (lnode_type)( lnode_sub + $2.i ), 2 ); 
	    n2->n[ 0 ] = n1; 
	    n2->n[ 1 ] = $3.n;
	    //Save result:
            $$.n = node( lnode_save_to_var, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = n2;
	}
    | LVAR stat_math_op expr
	{
	    DPRINT( "LVAR(%d) stat_math_op(%d) expr\n", (int)$1.i, (int)$2.i );
	    //Create first operand:
	    lnode* n1 = make_expr_node_from_local_var( $1.i );
	    //Create math operation:
	    lnode* n2 = node( (lnode_type)( lnode_sub + $2.i ), 2 ); 
	    n2->n[ 0 ] = n1; 
	    n2->n[ 1 ] = $3.n;
	    //Save result:
	    $$.n = node( lnode_save_to_stackframe, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = n2;
	}
    | FNNUM '(' exprlist ')' 
        {
	    DPRINT( "FNNUM(%d) ( exprlist )\n", (int)$1.i );
	    $$.n = node( lnode_call_builtin_fn_void, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = $3.n;
        }
    | fn_expr 
	{
	    DPRINT( "fn_expr (call void function. statement)\n" );
	    $$.n = $1.n;
	    $$.n->type = lnode_call_void;
	}
    | RET 
        {
	    DPRINT( "RET\n" );
	    $$.n = node( lnode_ret_int, 0 );
	    $$.n->val.i = 0;
        }
    | RET '(' expr ')' 
        {
	    DPRINT( "RET ( expr )\n" );
	    $$.n = node( lnode_ret, 1 );
	    $$.n->n[ 0 ] = $3.n;
        }
    | IF expr '{' statlist '}'
	{ 
	    DPRINT( "IF expr statlist\n" );
	    $$.n = node( lnode_if, 2 );
	    $$.n->n[ 0 ] = $2.n;
	    $$.n->n[ 1 ] = $4.n;
	    $4.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER;
	}
    | IF expr '{' statlist '}' ELSE '{' statlist '}'
	{ 
	    DPRINT( "IF expr statlist ELSE statlist\n" );
	    $$.n = node( lnode_if_else, 3 );
	    $$.n->n[ 0 ] = $2.n;
	    $$.n->n[ 1 ] = $4.n;
	    $$.n->n[ 2 ] = $8.n;
	    $4.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER | LNODE_FLAG_STATLIST_SKIP_NEXT_HEADER;
	    $8.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_HEADER;
	}
    | WHILE expr '{'
	{
	    DPRINT( "WHILE expr {\n" );
	    $$.n = node( lnode_while, LNODE_WHILE_SIZE );
	    $$.n->n[ LNODE_WHILE_COND_EXPR ] = $2.n;
	    $$.n->n[ LNODE_WHILE_JMP_TO_START ] = node( lnode_jmp_to_node, 0 );
	    $$.n->n[ LNODE_WHILE_JMP_TO_START ]->val.p = $2.n;
	    $$.n->n[ LNODE_WHILE_JMP_TO_END ] = node( lnode_jmp_to_end_of_node, 0 );
	    $$.n->n[ LNODE_WHILE_JMP_TO_END ]->val.p = $$.n->n[ LNODE_WHILE_JMP_TO_START ];
	    $$.n->n[ LNODE_WHILE_JMP_TO_END ]->flags |= LNODE_FLAG_JMP_IF_FALSE;
	    //Push it to stack:
	    if( pcomp->while_stack_ptr + 1 >= smem_get_size( pcomp->while_stack ) / sizeof( lnode* ) )
		pcomp->while_stack = (lnode**)smem_resize2( pcomp->while_stack, smem_get_size( pcomp->while_stack ) + sizeof( lnode* ) * 8 );
	    pcomp->while_stack[ pcomp->while_stack_ptr ] = $$.n;
	    pcomp->while_stack_ptr++;
	}
      statlist '}'
	{
	    DPRINT( "statlist } (while)\n" );
	    $$.n = $4.n;
	    $$.n->n[ LNODE_WHILE_BODY_STATLIST ] = $5.n;
	    //Pop from stack:
	    pcomp->while_stack_ptr--;
	}
    | FOR
	{
	    DPRINT( "FOR begin\n" );
	    pcomp->for_pars_mode = 1;
	}
      '(' statlist ';' expr ';' statlist ')' '{' 
	{
	    DPRINT( "FOR ( statlist ; expr ; statlist ) {\n" );
	    pcomp->for_pars_mode = 0;
	    $$.n = node( lnode_while, LNODE_WHILE_SIZE );
	    $$.n->n[ LNODE_WHILE_INIT_STATLIST ] = $4.n;
	    $$.n->n[ LNODE_WHILE_COND_EXPR ] = $6.n;
	    $$.n->n[ LNODE_WHILE_STEP_STATLIST ] = $8.n;
	    $$.n->n[ LNODE_WHILE_JMP_TO_START ] = node( lnode_jmp_to_node, 0 );
	    $$.n->n[ LNODE_WHILE_JMP_TO_START ]->val.p = $$.n->n[ LNODE_WHILE_COND_EXPR ];
	    $$.n->n[ LNODE_WHILE_JMP_TO_END ] = node( lnode_jmp_to_end_of_node, 0 );
	    $$.n->n[ LNODE_WHILE_JMP_TO_END ]->val.p = $$.n->n[ LNODE_WHILE_JMP_TO_START ];
	    $$.n->n[ LNODE_WHILE_JMP_TO_END ]->flags |= LNODE_FLAG_JMP_IF_FALSE;
	    //Push it to stack:
	    if( pcomp->while_stack_ptr + 1 >= smem_get_size( pcomp->while_stack ) / sizeof( lnode* ) )
		pcomp->while_stack = (lnode**)smem_resize2( pcomp->while_stack, smem_get_size( pcomp->while_stack ) + sizeof( lnode* ) * 8 );
	    pcomp->while_stack[ pcomp->while_stack_ptr ] = $$.n;
	    pcomp->while_stack_ptr++;
	}
      statlist '}'
	{
	    DPRINT( "statlist } (for)\n" );
	    $$.n = $11.n;
	    $$.n->n[ LNODE_WHILE_BODY_STATLIST ] = $12.n;
	    //Pop from stack:
	    pcomp->while_stack_ptr--;
	}
    | BREAK 
        { 
	    DPRINT( "BREAK (level %d)\n", (int)$1.i );
	    if( pcomp->while_stack_ptr > 0 )
	    {
        	$$.n = node( lnode_jmp_to_end_of_node, 0 );
		if( $1.i == -1 )
        	    $$.n->val.p = pcomp->while_stack[ 0 ]->n[ LNODE_WHILE_JMP_TO_START ];
        	else
        	{
        	    if( (signed)( pcomp->while_stack_ptr - $1.i ) >= 0 )
        		$$.n->val.p = pcomp->while_stack[ pcomp->while_stack_ptr - $1.i ]->n[ LNODE_WHILE_JMP_TO_START ];
        	    else
        	    {
            		PCOMP_ERROR( "wrong level number %d for 'break' operator", (int)$1.i );
            		YYERROR;
        	    }
        	}
    	    }
    	    else
    	    {
                PCOMP_ERROR( "operator 'break' can't be used outside of a loop" );
                YYERROR;
            }
        }
    | CONTINUE 
        { 
	    DPRINT( "CONTINUE\n" );
	    if( pcomp->while_stack_ptr > 0 )
	    {
        	$$.n = node( lnode_jmp_to_node, 0 );
        	lnode* w = pcomp->while_stack[ pcomp->while_stack_ptr - 1 ];
        	if( w->n[ LNODE_WHILE_STEP_STATLIST ] )
        	    $$.n->val.p = w->n[ LNODE_WHILE_STEP_STATLIST ];
        	else
        	    $$.n->val.p = w->n[ LNODE_WHILE_COND_EXPR ];
    	    }
    	    else
    	    {
                PCOMP_ERROR( "operator 'continue' can't be used outside of a loop" );
                YYERROR;
            }
        }
    | FNDEF
	{
	    DPRINT( "function begin\n" );
	    //Create new empty local symbol table:
	    create_empty_lsym_table( pcomp );
	    pcomp->fn_pars_mode = 1;
	}
      GVAR '(' lvarslist ')' 
	{
	    pcomp->fn_pars_mode = 0;
	    if( pcomp->var_flags[ $3.i ] & VAR_FLAG_FUNCTION )
	    {
		PCOMP_ERROR( "function %s is already defined", pix_vm_get_variable_name( pcomp->vm, $3.i ) );
		remove_lsym_table( pcomp, NULL );
                YYERROR;
	    }
	    if( pcomp->var_flags[ $3.i ] & VAR_FLAG_LABEL )
	    {
		PCOMP_ERROR( "function %s is already defined as label", pix_vm_get_variable_name( pcomp->vm, $3.i ) );
		remove_lsym_table( pcomp, NULL );
                YYERROR;
	    }
	    pcomp->var_flags[ $3.i ] |= VAR_FLAG_FUNCTION | VAR_FLAG_INITIALIZED;
	}
      '{' statlist '}'
	{
	    DPRINT( "function\n" );
	    //Remove local symbol table:
	    $$.n = remove_lsym_table( pcomp, $9.n );
	    if( $$.n == NULL ) YYERROR;
	    //Add the header:
	    $$.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_HEADER;
            //Add ret instruction to this statlist, because it is the function now:
            resize_node( $$.n, $$.n->nn + 2 );
	    $$.n->n[ $$.n->nn - 2 ] = node( lnode_ret_int, 0 );
	    $$.n->n[ $$.n->nn - 2 ]->val.i = 0;
	    //Save address of this function to global variable:
	    $$.n->n[ $$.n->nn - 1 ] = node( lnode_function_label_from_node, 1 );
	    $$.n->n[ $$.n->nn - 1 ]->val.i = $3.i;
	    $$.n->n[ $$.n->nn - 1 ]->n[ 0 ] = node( lnode_empty, 0 );
	    $$.n->n[ $$.n->nn - 1 ]->n[ 0 ]->val.p = $$.n;
	}
    | INCLUDE NUM_I
	{
	    if( (unsigned)$2.i < (unsigned)pcomp->vm->c_num )
	    {
		int name_size = pcomp->vm->c[ $2.i ]->size;
		char* name = (char*)smem_new( name_size + 1 );
		smem_copy( name, pcomp->vm->c[ $2.i ]->data, name_size );
		name[ name_size ] = 0;
		char* new_name = pix_compose_full_path( pcomp->base_path, name, 0 );
		smem_free( name );
		pix_vm_remove_container( $2.i, pcomp->vm );
		DPRINT( "include \"%s\"\n", new_name );
		
		size_t fsize = sfs_get_file_size( new_name );
		if( fsize == 0 )
		{
		    PCOMP_ERROR( "%s not found", new_name );
		    smem_free( new_name );
		    YYERROR;
		}
		
		//Save previous compiler state:
		if( pcomp->inc == 0 )
		{
		    pcomp->inc = (pix_include*)smem_new( 2 * sizeof( pix_include ) );
		}
		if( pcomp->inc_num >= smem_get_size( pcomp->inc ) / sizeof( pix_include ) )
		{
		    pcomp->inc = (pix_include*)smem_resize( pcomp->inc, ( pcomp->inc_num + 2 ) * sizeof( pix_include ) );
		}
		pcomp->inc[ pcomp->inc_num ].src = pcomp->src;
		pcomp->inc[ pcomp->inc_num ].src_ptr = pcomp->src_ptr;
		pcomp->inc[ pcomp->inc_num ].src_line = pcomp->src_line;
		pcomp->inc[ pcomp->inc_num ].src_size = pcomp->src_size;
		pcomp->inc[ pcomp->inc_num ].src_name = pcomp->src_name;
		pcomp->inc[ pcomp->inc_num ].base_path = pcomp->base_path;
		pcomp->inc_num++;
		
		//Set new compiler state:
		pcomp->src = (char*)smem_new( fsize );
		sfs_file f = sfs_open( new_name, "rb" );
		if( fsize >= 3 )
    		{
        	    sfs_read( pcomp->src, 1, 3, f );
        	    if( (uint8_t)pcomp->src[ 0 ] == 0xEF && (uint8_t)pcomp->src[ 1 ] == 0xBB && (uint8_t)pcomp->src[ 2 ] == 0xBF )
        	    {
            		//Byte order mark found. Just ignore it:
            		fsize -= 3;
        	    }
	            else
    		    {
            		sfs_rewind( f );
	            }
    		}
		sfs_read( pcomp->src, 1, fsize, f );
		sfs_close( f );
		pcomp->src_ptr = 0;
		pcomp->src_line = 0;
		pcomp->src_size = fsize;
		pcomp->src_name = new_name;
		pcomp->base_path = pix_get_base_path( new_name );
		DPRINT( "New base path: %s\n", pcomp->base_path );
	    }
	    else 
	    {
		YYERROR;
	    }
	    $$.n = node( lnode_empty, 0 );
	}
    ;
exprlist
    : /*empty*/ { $$.n = node( lnode_exprlist, 0 ); }
    | expr { $$.n = node( lnode_exprlist, 1 ); $$.n->n[ 0 ] = $1.n; }
    | exprlist ',' expr
	{
	    //Add new node (expr) to list of parameters (exprlist):
	    resize_node( $1.n, $1.n->nn + 1 );
            $1.n->n[ $1.n->nn - 1 ] = $3.n;
	    $$.n = $1.n;
        }
    ;
basic_expr
    : NUM_I { DPRINT( "NUM_I(%d)\n", (int)$1.i ); $$.n = node( lnode_int, 0 ); $$.n->val.i = $1.i; }
    | NUM_F { DPRINT( "NUM_F(%d)\n", (int)$1.f ); $$.n = node( lnode_float, 0 ); $$.n->val.f = $1.f; }
    | GVAR { DPRINT( "GVAR(%d)\n", (int)$1.i ); $$.n = make_expr_node_from_var( $1.i ); }
    | LVAR { DPRINT( "LVAR(%d)\n", (int)$1.i ); $$.n = make_expr_node_from_local_var( $1.i ); }
    | FNNUM '(' exprlist ')'
        {
	    DPRINT( "FNNUM(%d) ( exprlist )\n", (int)$1.i );
	    $$.n = node( lnode_call_builtin_fn, 1 );
	    $$.n->val.i = $1.i;
	    $$.n->n[ 0 ] = $3.n;
        }
    ;
fn_expr
    : basic_expr '(' exprlist ')' { DPRINT( "basic_expr ( exprlist )\n" ); $$.n = node( lnode_call, 2 ); $$.n->n[ 0 ] = $3.n; $$.n->n[ 1 ] = $1.n; }
    | fn_expr '(' exprlist ')' { DPRINT( "fn_expr ( exprlist )\n" ); $$.n = node( lnode_call, 2 ); $$.n->n[ 0 ] = $3.n; $$.n->n[ 1 ] = $1.n; }
    | mem_expr '(' exprlist ')' { DPRINT( "mem_expr ( exprlist )\n" ); $$.n = node( lnode_call, 2 ); $$.n->n[ 0 ] = $3.n; $$.n->n[ 1 ] = $1.n; }
    | prop_expr '(' exprlist ')' { DPRINT( "prop_expr ( exprlist )\n" ); $$.n = node( lnode_call, 2 ); $$.n->n[ 0 ] = $3.n; $$.n->n[ 1 ] = $1.n; }
    ;
mem_expr
    : basic_expr '[' smem_offset ']' { DPRINT( "basic_expr [ smem_offset ]\n" ); $$.n = node( lnode_load_from_mem, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | fn_expr '[' smem_offset ']' { DPRINT( "fn_expr [ smem_offset ]\n" ); $$.n = node( lnode_load_from_mem, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | mem_expr '[' smem_offset ']' { DPRINT( "mem_expr [ smem_offset ]\n" ); $$.n = node( lnode_load_from_mem, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | prop_expr '[' smem_offset ']' { DPRINT( "prop_expr [ smem_offset ]\n" ); $$.n = node( lnode_load_from_mem, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    ;
prop_expr
    : basic_expr HASH { DPRINT( "basic_expr.%d\n", (int)$2.i ); $$.n = node( lnode_load_from_prop, 1 ); $$.n->n[ 0 ] = $1.n; $$.n->val.i = $2.i; }
    | fn_expr HASH { DPRINT( "fn_expr.%d\n", (int)$2.i ); $$.n = node( lnode_load_from_prop, 1 ); $$.n->n[ 0 ] = $1.n; $$.n->val.i = $2.i; }
    | mem_expr HASH { DPRINT( "mem_expr.%d\n", (int)$2.i ); $$.n = node( lnode_load_from_prop, 1 ); $$.n->n[ 0 ] = $1.n; $$.n->val.i = $2.i; }
    | prop_expr HASH { DPRINT( "prop_expr.%d\n", (int)$2.i ); $$.n = node( lnode_load_from_prop, 1 ); $$.n->n[ 0 ] = $1.n; $$.n->val.i = $2.i; }
    ;
expr
    : basic_expr { DPRINT( "basic expression\n" ); $$.n = $1.n; }
    | fn_expr { DPRINT( "fn_expr\n" ); $$.n = $1.n; }
    | mem_expr { DPRINT( "mem_expr\n" ); $$.n = $1.n; }
    | prop_expr { DPRINT( "prop_expr\n" ); $$.n = $1.n; }
    | '{' 
	{ 
	    DPRINT( "statlist begin (expr)\n" );
	    //Create new empty local symbol table:
	    create_empty_lsym_table( pcomp );
	}
      statlist '}'
        { 
	    DPRINT( "statlist (expr)\n" );
	    //Remove local symbol table:
	    $$.n = remove_lsym_table( pcomp, $3.n );
	    if( $$.n == 0 ) YYERROR;
	    //Add the header:
	    $$.n->flags |= LNODE_FLAG_STATLIST_WITH_JMP_HEADER | LNODE_FLAG_STATLIST_AS_EXPRESSION;
            //Add ret instruction to this statlist, because it is the function now:
            resize_node( $$.n, $$.n->nn + 1 );
	    $$.n->n[ $$.n->nn - 1 ] = node( lnode_ret_int, 0 );
	    $$.n->n[ $$.n->nn - 1 ]->val.i = 0;
        }
    | '(' expr ')' { $$.n = $2.n; }
    | expr '-' expr { DPRINT( "SUB\n" ); $$.n = node( lnode_sub, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '+' expr { DPRINT( "ADD\n" ); $$.n = node( lnode_add, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '*' expr { DPRINT( "MUL\n" ); $$.n = node( lnode_mul, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr IDIV expr { DPRINT( "IDIV\n" ); $$.n = node( lnode_idiv, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '/' expr { DPRINT( "DIV\n" ); $$.n = node( lnode_div, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '%' expr { DPRINT( "MOD\n" ); $$.n = node( lnode_mod, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr AND expr { DPRINT( "AND\n" ); $$.n = node( lnode_and, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr OR expr { DPRINT( "OR\n" ); $$.n = node( lnode_or, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr XOR expr { DPRINT( "XOR\n" ); $$.n = node( lnode_xor, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr ANDAND expr { DPRINT( "ANDAND\n" ); $$.n = node( lnode_andand, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr OROR expr { DPRINT( "OROR\n" ); $$.n = node( lnode_oror, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr EQ expr { DPRINT( "EQ\n" ); $$.n = node( lnode_eq, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr NEQ expr { DPRINT( "NEQ\n" ); $$.n = node( lnode_neq, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '<' expr { DPRINT( "LESS\n" ); $$.n = node( lnode_less, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr LEQ expr { DPRINT( "LEQ\n" ); $$.n = node( lnode_leq, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr '>' expr { DPRINT( "GREATER\n" ); $$.n = node( lnode_greater, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr GEQ expr { DPRINT( "GEQ\n" ); $$.n = node( lnode_geq, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr LSHIFT expr { DPRINT( "LSHIFT\n" ); $$.n = node( lnode_lshift, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | expr RSHIFT expr { DPRINT( "RSHIFT\n" ); $$.n = node( lnode_rshift, 2 ); $$.n->n[ 0 ] = $1.n; $$.n->n[ 1 ] = $3.n; }
    | '-' expr %prec NEG { DPRINT( "NEG\n" ); $$.n = node( lnode_neg, 1 ); $$.n->n[ 0 ] = $2.n; }
    | '!' expr %prec LNOT { DPRINT( "LNOT\n" ); $$.n = node( lnode_lnot, 1 ); $$.n->n[ 0 ] = $2.n; }
    | '~' expr %prec BNOT { DPRINT( "BNOT\n" ); $$.n = node( lnode_bnot, 1 ); $$.n->n[ 0 ] = $2.n; }
    ;
%%

//Compilation error message:
void yyerror( pix_compiler* pcomp, char const* str )
{
    PCOMP_ERROR( "%s", str );
}

//https://en.wikipedia.org/wiki/Escape_sequences_in_C
static size_t handle_control_characters( char* str, size_t size )
{
    size_t r = 0;
    size_t w = 0;
    for( ; r < size; r++, w++ )
    {
	char c = str[ r ];
	if( c == '\\' && r < size - 1 )
	{
	    r++;
	    char next_c = str[ r ];
	    switch( next_c )
	    {
		case 'a': str[ w ] = 0x07; break; //alert
		case 'b': str[ w ] = 0x08; break; //backspace
		case 't': str[ w ] = 0x09; break; //tab
		case 'r': str[ w ] = 0x0D; break; //carriage-return
		case 'n': str[ w ] = 0x0A; break; //newline
		case 'v': str[ w ] = 0x0B; break; //vertical-tab
		case 'f': str[ w ] = 0x0C; break; //form-feed
		case 'e': str[ w ] = 0x1B; break; //escape character
		default:
		{
		    uint8_t val = 0;
		    int i = 0;
		    if( next_c == 'x' )
		    {
		        //hexadecimal number:
		        r++;
		        for( ; i < 2 && r + i < size; i++ )
		        {
		    	    char cc = str[ r + i ];
			    val <<= 4;
			    if( cc <= '9' )
			    {
			        val |= cc - '0';
			    }
			    else
			    {
			        if( cc >= 'a' )
			    	    val |= cc - 'a' + 10;
				else
				    val |= cc - 'A' + 10;
			    }
			}
		    }
		    else
		    {
		        //octal number:
		        for( ; i < 3 && r + i < size; i++ )
		        {
		    	    char cc = str[ r + i ];
			    if( cc < '0' || cc > '7' ) break;
			    val <<= 3;
			    val |= cc - '0';
			}
		    }
		    if( i )
		    {
			//char described by the hex/oct number:
			next_c = val;
			r += i - 1;
		    }
		    str[ w ] = next_c;
		    break;
		}
	    }
	}
	else
	{
	    str[ w ] = str[ r ];
	}
    }
    return w;
}

static void resize_var_flags( pix_compiler* pcomp )
{
    if( pcomp->vm->vars_num > pcomp->var_flags_size )
    {
        size_t new_size = pcomp->vm->vars_num + 64;
	pcomp->var_flags = (char*)smem_resize2( pcomp->var_flags, new_size );
        pcomp->var_flags_size = new_size;
    }
}

static void resize_local_variables( pix_compiler* pcomp )
{
    pix_lsymtab* l = &pcomp->lsym[ pcomp->lsym_num ];
    if( l->lvars_num > l->lvar_flags_size )
    {
        size_t new_size = l->lvars_num + 64;
	l->lvar_flags = (char*)smem_resize2( l->lvar_flags, new_size );
	l->lvar_names = (char**)smem_resize2( l->lvar_names, new_size * sizeof( char* ) );
	l->lvar_flags_size = new_size;
    }
}

//Parser. yylex() get next token from the source:
static int yylex( YYSTYPE* yylval, pix_compiler* pcomp )
{
    char* src = pcomp->src;

    int c = -1;

    int str_symbol = 0;
    int str_start = -1;
    bool str_first_char_is_digit = 0;

    while( pcomp->src_ptr < pcomp->src_size + 2 )
    {
	if( pcomp->src_ptr >= pcomp->src_size )
	{
	    if( pcomp->src_ptr == pcomp->src_size )
	    {
		c = ' '; //Last empty character of the file (we need this to finish some strings)
	    }
	    else
	    {
		c = -1; //EOF
		if( pcomp->inc_num > 0 )
		{
		    DPRINT( "Return from include\n" );
		    //Remove current state:
		    smem_free( pcomp->src );
		    smem_free( pcomp->src_name );
		    smem_free( pcomp->base_path );
		    //Restore previous compiler state:
		    pcomp->inc_num--;
		    pcomp->src = pcomp->inc[ pcomp->inc_num ].src;
		    src = pcomp->src;
		    pcomp->src_ptr = pcomp->inc[ pcomp->inc_num ].src_ptr;
		    pcomp->src_line = pcomp->inc[ pcomp->inc_num ].src_line;
		    pcomp->src_size = pcomp->inc[ pcomp->inc_num ].src_size;
		    pcomp->src_name = pcomp->inc[ pcomp->inc_num ].src_name;
		    pcomp->base_path = pcomp->inc[ pcomp->inc_num ].base_path;
		    //Reset parser state:
		    str_symbol = 0;
		    str_start = -1;
		    str_first_char_is_digit = 0;
		    continue;
		}
	    }
	}
	else
	{
	    c = src[ pcomp->src_ptr ];
	}
	pcomp->src_ptr++;

	//Ignore text strings (quoted):
        if( c == '"' || c == '\'' )
	{
	    if( !str_symbol )
            {
                //String start:
                str_start = pcomp->src_ptr;
                str_symbol = c;
                continue;
            }
            else
            {
                if( str_symbol == c )
                {
                    //End of string:
                    int str_size = pcomp->src_ptr - str_start - 1;
		    if( c == '"' )
                    {
                        //Return number of text container:
			char* data;
			if( str_size > 0 )
			{
			    data = (char*)smem_new( str_size );
			    smem_copy( data, src + str_start, str_size );
			    str_size = handle_control_characters( data, str_size );
			}
			else 
			{
			    data = (char*)smem_new( 1 );
			    data[ 0 ] = 0;
			    str_size = 1;
			}
			yylval->i = pix_vm_new_container( -1, str_size, 1, PIX_CONTAINER_TYPE_INT8, data, pcomp->vm );
			pix_vm_set_container_flags( yylval->i, pix_vm_get_container_flags( yylval->i, pcomp->vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, pcomp->vm );
			DPRINT( "NEW TEXT CONTAINER %d (%d bytes)\n", (int)yylval->i, str_size );
		    }
		    else
		    {
			//Return string converted to integer:
			char ts[ 4 ];
			ts[ 0 ] = src[ str_start ];
			ts[ 1 ] = src[ str_start + 1 ];
			ts[ 2 ] = src[ str_start + 2 ];
			ts[ 3 ] = src[ str_start + 3 ];
			str_size = handle_control_characters( ts, str_size );
			if( str_size == 1 ) 
			    yylval->i = (uint8_t)ts[ 0 ];
                        if( str_size == 2 ) 
			    yylval->i = (uint8_t)ts[ 0 ] 
				+ ( (uint8_t)ts[ 1 ] << 8 );
                        if( str_size == 3 ) 
			    yylval->i = (uint8_t)ts[ 0 ]
				+ ( (uint8_t)ts[ 1 ] << 8 )
				+ ( (uint8_t)ts[ 2 ] << 16 );
                        if( str_size >= 4 ) 
			    yylval->i = (uint8_t)ts[ 0 ] 
				+ ( (uint8_t)ts[ 1 ] << 8 )
				+ ( (uint8_t)ts[ 2 ] << 16 )
				+ ( (uint8_t)ts[ 3 ] << 24 );
		    }
		    c = NUM_I;
		    break;
		}
		else
		{
		    continue;
		}
	    }
	}
	else 
	{
	    if( str_symbol )
	    {
		//String reading.
		switch( c )
		{
		    case '\\':
			//Skip control character:
			pcomp->src_ptr++;
			break;
		    case 0xA:
			//New line:
			pcomp->src_line++;
			break;
		}
		continue;
	    }
	}

	//Not a quoted string:
	bool numeric = NUMERIC( c );
	if( numeric || ABC( c ) || c == '_' || c == '#' || c == '.' || c == '$' )
        {
	    if( str_start == -1 )
	    {
		//String begin:
		if( numeric ) str_first_char_is_digit = 1;
		str_start = pcomp->src_ptr - 1;
	    }
	    else
	    {
		if( c == '.' )
		{
		    if( str_first_char_is_digit == 0 )
		    {
			goto string_end;
		    }
		}
	    }
	    continue;
	}
	else
	{
string_end:
	    if( str_start >= 0 )
	    {
		int str_size = pcomp->src_ptr - str_start - 1;
                if( src[ str_start ] == '#' || 
            	    NUMERIC( src[ str_start ] ) || 
            	    ( str_size > 1 && src[ str_start ] == '.' && NUMERIC( src[ str_start + 1 ] ) ) )
		{
		    //Number:
                    int ni;
                    if( src[ str_start ] == '#' )
                    {
                	if( str_size == 1 && c == '!' )
                	{
                	    //Shebang (UNIX). Treat this as a comment:
            		    for(;;)
            		    {
                		pcomp->src_ptr++;
	                        if( pcomp->src_ptr >= pcomp->src_size ) break;
        		        if( src[ pcomp->src_ptr ] == 0xD || src[ pcomp->src_ptr ] == 0xA ) break;
	                    }
	                    str_symbol = 0;
			    str_start = -1;
			    str_first_char_is_digit = 0;
        		    continue;
        		}
			c = NUM_I;
                        //HEX COLOR:
                        yylval->i = 0;
                        for( ni = str_start + 1; ni < str_start + str_size; ni++ )
                        {
                            yylval->i <<= 4;
                            if( src[ ni ] < 58 ) yylval->i += src[ ni ] - '0';
                            else if( src[ ni ] > 64 && src[ ni ] < 91 ) yylval->i += src[ ni ] - 'A' + 10;
                            else yylval->i += src[ ni ] - 'a' + 10;
                        }
                        yylval->i = get_color( 
			    ( yylval->i >> 16 ) & 255,
                            ( yylval->i >> 8  ) & 255,
                            ( yylval->i       ) & 255 );
                    }
                    else
                    {
			c = NUM_I;
			if( str_size > 2 && src[ str_start + 1 ] == 'x' )
			{
			    //HEX:
			    yylval->i = 0;
			    for( ni = str_start + 2; ni < str_start + str_size; ni++ )
			    {
				yylval->i <<= 4;
				if( src[ ni ] < 58 ) yylval->i += src[ ni ] - '0';
				else if( src[ ni ] > 64 && src[ ni ] < 91 ) yylval->i += src[ ni ] - 'A' + 10;
				else yylval->i += src[ ni ] - 'a' + 10;
			    }
			}
			else if( str_size > 2 && src[ str_start + 1 ] == 'b' )
			{
			    //BIN:
		            yylval->i = 0;
			    for( ni = str_start + 2; ni < str_start + str_size; ni++ )
			    {
				yylval->i <<= 1;
				yylval->i += src[ ni ] - '0';
			    }
			}
			else
			{
			    bool float_num = false;
			    for( ni = str_start; ni < str_start + str_size; ni++ )
				if( src[ ni ] == '.' ) { float_num = true; break; }
			    if( float_num )
			    {
				//FLOATING POINT:
				if( str_size > 256 )
				    str_size = 256;
				smem_copy( pcomp->temp_sym_name, &src[ str_start ], str_size );
				pcomp->temp_sym_name[ str_size ] = 0;
				c = NUM_F;
				yylval->f = atof( pcomp->temp_sym_name );
			    }
			    else 
			    {
				//DEC:
				yylval->i = 0;
				for( ni = str_start; ni < str_start + str_size; ni++ )
				{
				    yylval->i *= 10;
				    yylval->i += src[ ni ] - '0';
				}
			    }
			}
                    }
		}
		else
		{
		    //Name of some symbol:
		    if( str_size == 1 && src[ str_start ] > 0 && src[ str_start ] < 127 )
		    {
			//Standard variable with one char ASCII name:
			c = GVAR;
			yylval->i = src[ str_start ];
			pcomp->var_flags[ yylval->i ] |= VAR_FLAG_USED;
		    }
		    else 
		    {
			//Variable with long name:
			if( str_size > 256 )
			    str_size = 256;
			smem_copy( pcomp->temp_sym_name, &src[ str_start ], str_size );
			pcomp->temp_sym_name[ str_size ] = 0;
			if( src[ str_start ] == '.' )
			{
			    //Container property:
			    c = HASH;
			    //Create global variable with the name and hash of this property:
			    bool created;
                            pix_sym* sym = pix_symtab_lookup( pcomp->temp_sym_name, -1, 1, SYMTYPE_GVAR, 0, 0, &created, &pcomp->sym );
                            if( sym )
                            {
                        	if( created )
                        	{
                            	    //Create new variable:
                            	    sym->val.i = pcomp->vm->vars_num;
                            	    DPRINT( "New global variable: %s (%d)\n", pcomp->temp_sym_name, (int)sym->val.i );
                            	    pcomp->vm->vars_num++;
                            	    pix_vm_resize_variables( pcomp->vm );
                            	    resize_var_flags( pcomp );
                            	    pcomp->var_flags[ sym->val.i ] |= VAR_FLAG_USED | VAR_FLAG_INITIALIZED;
                            	    //Save variable name:
                            	    char* var_name = (char*)smem_new( str_size + 1 );
                            	    if( var_name )
                            	    {
                                	pcomp->vm->var_names[ sym->val.i ] = var_name;
                                	smem_copy( var_name, pcomp->temp_sym_name, str_size + 1 );
                            	    }
                        	}
                        	yylval->i = sym->val.i;
                        	pcomp->vm->vars[ yylval->i ].i = pix_symtab_hash( (const char*)( pcomp->temp_sym_name + 1 ), PIX_CONTAINER_SYMTAB_SIZE );
			    }
			    else
			    {
				PCOMP_ERROR( "can't create a new symbol for property" );
            			return -1;
			    }
			}
			else
			if( src[ str_start ] == '$' )
			{
			    //Local variable:
			    c = LVAR;
			    if( str_size == 1 )
			    {
				yylval->i = 0;
			    }
			    else 
			    {
				if( NUMERIC( src[ str_start + 1 ] ) )
				{
				    yylval->i = string_to_int( pcomp->temp_sym_name + 1 );
				}
				else 
				{
				    //Named local variable:
				    pix_symtab* lsym = pcomp->lsym[ pcomp->lsym_num ].lsym;
				    if( !lsym )
				    {
					lsym = (pix_symtab*)smem_new( sizeof( pix_symtab ) );
					pcomp->lsym[ pcomp->lsym_num ].lsym = lsym;
					pix_symtab_init( PIX_COMPILER_SYMTAB_SIZE, lsym );
				    }
				    bool created;
				    pix_sym* sym = pix_symtab_lookup( pcomp->temp_sym_name + 1, -1, 1, SYMTYPE_LVAR, 0, 0, &created, lsym );
				    if( sym )
				    {
					if( created )
					{
					    if( pcomp->fn_pars_mode )
					    {
						//Fn parameters:
						sym->val.i = 1 + pcomp->lsym[ pcomp->lsym_num ].pars_num;
						pcomp->lsym[ pcomp->lsym_num ].pars_num++;
						DPRINT( "New local variable (fn parameter): %s (%d)\n", pcomp->temp_sym_name, (int)sym->val.i );
					    }
					    else
					    {
						//Variables:
						pix_lsymtab* l = &pcomp->lsym[ pcomp->lsym_num ];
						sym->val.i = -LVAR_OFFSET - l->lvars_num;
						l->lvars_num++;
						resize_local_variables( pcomp );
						int lvar_num = -sym->val.i - LVAR_OFFSET;
						l->lvar_flags[ lvar_num ] |= VAR_FLAG_USED;
						l->lvar_names[ lvar_num ] = (char*)smem_new( str_size + 1 );
						smem_copy( l->lvar_names[ lvar_num ], pcomp->temp_sym_name, str_size + 1 );
						DPRINT( "New local variable: %s (%d)\n", pcomp->temp_sym_name, (int)sym->val.i );
					    }
				        }
				        else
				        {
				    	    //Already created:
				    	    if( pcomp->fn_pars_mode )
				    	    {
				    		PCOMP_ERROR( "parameter %s is already defined", pcomp->temp_sym_name );
				    		return -1;
				    	    }
				        }
					yylval->i = sym->val.i;
				    }
				    else
                        	    {
                            		PCOMP_ERROR( "can't create a new symbol for local variable" );
                            		return -1;
                        	    }
				}
			    }
			}
			else
			{
			    //Global variable
			    bool created;
			    pix_sym* sym = pix_symtab_lookup( pcomp->temp_sym_name, -1, 1, SYMTYPE_GVAR, 0, 0, &created, &pcomp->sym );
			    if( sym == 0 )
                            {
                                PCOMP_ERROR( "can't create a new symbol for global variable" );
                                return -1;
                            }
			    if( created )
			    {
				//Create new variable:
				sym->val.i = pcomp->vm->vars_num;
				yylval->i = sym->val.i;
				c = GVAR;
				DPRINT( "New global variable: %s (%d)\n", pcomp->temp_sym_name, (int)sym->val.i );
				pcomp->vm->vars_num++;
				pix_vm_resize_variables( pcomp->vm );
				resize_var_flags( pcomp );
				pcomp->var_flags[ sym->val.i ] |= VAR_FLAG_USED;
				//Save variable name:
				char* var_name = (char*)smem_new( str_size + 1 );
				if( var_name )
				{
				    pcomp->vm->var_names[ sym->val.i ] = var_name;
                            	    smem_copy( var_name, pcomp->temp_sym_name, str_size + 1 );
                            	}
			    }
			    else
			    {
				switch( sym->type )
				{
				    case SYMTYPE_GVAR:
					yylval->i = sym->val.i;
					c = GVAR;
					break;
				    case SYMTYPE_LVAR:
					yylval->i = sym->val.i;
					c = LVAR;
					break;
				    case SYMTYPE_NUM_I:
					yylval->i = sym->val.i;
					c = NUM_I;
					break;
				    case SYMTYPE_NUM_F:
					yylval->f = sym->val.f;
					c = NUM_F;
					break;
				    case SYMTYPE_WHILE:
					c = WHILE;
					break;
				    case SYMTYPE_FOR:
					c = FOR;
					break;
				    case SYMTYPE_BREAK:
					yylval->i = 1;
					c = BREAK;
					break;
				    case SYMTYPE_BREAK2:
					yylval->i = 2;
					c = BREAK;
					break;
				    case SYMTYPE_BREAK3:
					yylval->i = 3;
					c = BREAK;
					break;
				    case SYMTYPE_BREAK4:
					yylval->i = 4;
					c = BREAK;
					break;
				    case SYMTYPE_BREAKALL:
					yylval->i = -1;
					c = BREAK;
					break;
				    case SYMTYPE_CONTINUE:
					c = CONTINUE;
					break;
				    case SYMTYPE_IF:
					c = IF;
					break;
				    case SYMTYPE_ELSE:
					c = ELSE;
					break;
				    case SYMTYPE_GO:
					c = GO;
					break;
				    case SYMTYPE_RET:
					c = RET;
					break;
				    case SYMTYPE_IDIV:
					c = IDIV;
					break;
				    case SYMTYPE_FNNUM:
					c = FNNUM;
					yylval->i = sym->val.i;
					break;
				    case SYMTYPE_FNDEF:
					c = FNDEF;
					break;
				    case SYMTYPE_INCLUDE:
					c = INCLUDE;
					break;
				    case SYMTYPE_HALT:
					c = HALT;
					break;
				    default:
					break;
				}
			    }
			}
		    }
		}
		pcomp->src_ptr--;
		break;
	    }
        }

	//Parse other symbols:
        if( c == '/' )
        {
            if( pcomp->src_ptr < pcomp->src_size && src[ pcomp->src_ptr ] == '/' )
            { 
		//COMMENTS:
                for(;;)
                {
                    pcomp->src_ptr++;
                    if( pcomp->src_ptr >= pcomp->src_size ) break;
                    if( src[ pcomp->src_ptr ] == 0xD || src[ pcomp->src_ptr ] == 0xA ) break;
                }
                continue;
            }
            else
            if( pcomp->src_ptr < pcomp->src_size && src[ pcomp->src_ptr ] == '*' )
            {
                //COMMENTS 2:
                for(;;)
                {
                    pcomp->src_ptr++;
                    if( pcomp->src_ptr >= pcomp->src_size ) break;
                    if( src[ pcomp->src_ptr ] == 0xA ) pcomp->src_line++;
                    if( pcomp->src_ptr + 1 < pcomp->src_size && 
		        src[ pcomp->src_ptr ] == '*' && 
			src[ pcomp->src_ptr + 1 ] == '/' ) 
		    { 
			pcomp->src_ptr += 2; break; 
		    }
                }
                continue;
            }
        }

	bool need_to_break = 0;
        switch( c )
        {
	    case 0xA:
		//New line:
		pcomp->src_line++;
		break;

            case '-':
            case '+':
            case '*':
            case '/':
            case '%':
            case ':':
            case '(':
            case ')':
            case ',':
            case '[':
            case ']':
            case '{':
            case '}':
	    case '~':
                need_to_break = 1;
                break;

            case ';':
        	if( pcomp->for_pars_mode ) need_to_break = 1; //for ( ; ; )
        	break;

	    case '^':
		c = XOR;
		need_to_break = 1;
		break;
	    case '&':
		if( src[ pcomp->src_ptr ] == '&' )
		{
		    c = ANDAND;
		    pcomp->src_ptr++;
		}
		else
		{
		    c = AND;
		}
		need_to_break = 1;
		break;
	    case '|':
		if( src[ pcomp->src_ptr ] == '|' )
		{
		    c = OROR;
		    pcomp->src_ptr++;
		}
		else
		{
		    c = OR;
		}
		need_to_break = 1;
		break;

	    case '<':
                if( src[ pcomp->src_ptr ] == '=' )
                {
                    c = LEQ;
                    pcomp->src_ptr++;
                }
		else
		if( src[ pcomp->src_ptr ] == '<' )
		{
		    c = LSHIFT;
		    pcomp->src_ptr++;
		}
                need_to_break = 1;
                break;
            case '>':
                if( src[ pcomp->src_ptr ] == '=' )
                {
                    c = GEQ;
		    pcomp->src_ptr++;
                }
		else
		if( src[ pcomp->src_ptr ] == '>' )
		{
		    c = RSHIFT;
		    pcomp->src_ptr++;
		}
                need_to_break = 1;
                break;
            case '!':
                if( src[ pcomp->src_ptr ] == '=' )
                {
                    c = NEQ;
                    pcomp->src_ptr++;
                }
                need_to_break = 1;
                break;
	    case '=':
		if( src[ pcomp->src_ptr ] == '=' )
		{
		    c = EQ;
		    pcomp->src_ptr++;
		}
		need_to_break = 1;
		break;
	}
	if( need_to_break ) break;
    }

    return c;
}

static void push_int( pix_compiler* pcomp, PIX_INT v )
{
    if( v < ( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) &&
	v > -( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) )
    {
	DPRINT( "%d: PUSH_i ( %d << OB )\n", (int)pcomp->vm->code_ptr, (int)v );
	pix_vm_put_opcode( OPCODE_PUSH_i | ( (PIX_OPCODE)v << PIX_OPCODE_BITS ), pcomp->vm );
    }
    else
    {
	DPRINT( "%d: PUSH_I %d\n", (int)pcomp->vm->code_ptr, (int)v );
	pix_vm_put_opcode( OPCODE_PUSH_I, pcomp->vm );
	pix_vm_put_int( v, pcomp->vm );
    }
}

static void push_jmp( pix_compiler* pcomp, size_t dest_ptr, int mode ) //mode: 0 - JMP; 1 - JMP is FALSE
{
    const char* opcode_name = "JMP_i";
    PIX_OPCODE opcode = OPCODE_JMP_i;
    if( mode == 1 )
    {
	opcode_name = "JMP_IF_FALSE_i";
	opcode = OPCODE_JMP_IF_FALSE_i;
    }
    size_t code_ptr = pcomp->vm->code_ptr;
    PIX_INT offset = (PIX_INT)dest_ptr - (PIX_INT)code_ptr;
    if( offset < ( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) &&
        offset > -( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) )
    {
        DPRINT( "%d: %s ( %d << OB )\n", opcode_name, (int)code_ptr, (int)offset );
        pix_vm_put_opcode( opcode | ( (PIX_OPCODE)offset << PIX_OPCODE_BITS ), pcomp->vm );
    }
    else
    {
    	/*DPRINT( "%d: %s %d\n", opcode_name, (int)code_ptr, (int)offset );
	opcode_name = "JMP_I";
	opcode = OPCODE_JMP_I;
        if( mode == 1 )
	{
	    opcode_name = "JMP_IF_FALSE_I";
    	    opcode = OPCODE_JMP_IF_FALSE_I;
        }
    	pix_vm_put_opcode( opcode, pcomp->vm );
    	pix_vm_put_int( offset, pcomp->vm );*/
        slog( "\n!!!!\nJMP is too far! %d\n!!!!\n", (int)offset );
    }
    for( uint i = 0; i < pcomp->statlist_header_size - ( pcomp->vm->code_ptr - code_ptr ); i++ )
    {
        DPRINT( "%d: NOP\n", (int)pcomp->vm->code_ptr, (int)offset );
        pix_vm_put_opcode( OPCODE_NOP, pcomp->vm );
    }
}

//Add lexical node:
static lnode* node( lnode_type type, uint nn )
{
    lnode* n = (lnode*)smem_new( sizeof( lnode ) );
    if( nn > 0 )
    {
        n->n = (lnode**)smem_znew( sizeof( lnode* ) * nn );
    }
    else
    {
	n->n = NULL;
    }
    n->type = type;
    n->nn = nn;
    n->flags = 0;
    n->code_ptr = 0;
    n->code_ptr2 = 0;
    return n;
}

static void resize_node( lnode* n, uint nn )
{
    if( !n ) return;

    if( !n->n )
    {
	n->n = (lnode**)smem_znew( sizeof( lnode* ) * nn );
    }
    else
    {
	if( nn > smem_get_size( n->n ) / sizeof( lnode* ) )
	{
	    n->n = (lnode**)smem_resize2( n->n, sizeof( lnode* ) * ( nn + 8 ) );
	}
    }
    n->nn = nn;
}

static lnode* clone_tree( lnode* n )
{
    if( !n ) return NULL;

    lnode* new_n = node( n->type, n->nn );

    new_n->flags = n->flags;
    new_n->val = n->val;

    for( uint i = 0; i < n->nn; i++ )
    {
	new_n->n[ i ] = clone_tree( n->n[ i ] );
    }

    return new_n;
}

static void remove_tree( lnode* n )
{
    if( !n ) return;

    if( n->n )
    {
	for( uint i = 0; i < n->nn; i++ )
	{
	    remove_tree( n->n[ i ] );
	}
	smem_free( n->n );
    }
    smem_free( n );
}

static void clean_tree( lnode* n ) //replace by lnode_empty
{
    if( !n ) return;

    if( n->n )
    {
	for( uint i = 0; i < n->nn; i++ )
	{
	    remove_tree( n->n[ i ] );
	}
	smem_free( n->n );
    }

    n->type = lnode_empty;
    n->flags = 0;
    n->nn = 0;
}

static void optimize_tree( lnode* n )
{
    if( !n ) return;

    if( n->n )
    {
	for( uint i = 0; i < n->nn; i++ )
	{
	    optimize_tree( n->n[ i ] );
	}
    }

    switch( n->type )
    {
	case lnode_if:
	    {
		int v = 0;
		if( n->n[ 0 ]->type == lnode_int ) 
		{
		    if( n->n[ 0 ]->val.i == 0 )
			v = 1;
		    else
			v = 2;
		}
		else if( n->n[ 0 ]->type == lnode_float )
		{
		    if( n->n[ 0 ]->val.f == 0 )
			v = 1;
		    else
			v = 2;
		}
		if( v )
		{
		    if( v == 1 )
		    {
			//IF 0 { CODE } - IGNORE THIS CODE:
			remove_tree( n->n[ 0 ] );
			remove_tree( n->n[ 1 ] );
			n->nn = 0;
		    }
		    else
		    {
			//IF 1 { CODE } - ALWAYS TRUE:
			remove_tree( n->n[ 0 ] );
			n->n[ 0 ] = 0;
			n->n[ 1 ]->flags = 0; //Just a statlist without any headers
		    }
		}
	    }
	    break;
	case lnode_if_else:
	    {
		int v = 0;
		if( n->n[ 0 ]->type == lnode_int ) 
		{
		    if( n->n[ 0 ]->val.i == 0 )
			v = 1;
		    else
			v = 2;
		}
		else if( n->n[ 0 ]->type == lnode_float )
		{
		    if( n->n[ 0 ]->val.f == 0 )
			v = 1;
		    else
			v = 2;
		}
		if( v )
		{
		    if( v == 1 )
		    {
			//IF 0 { CODE1 } ELSE { CODE2 } - IGNORE CODE1; ALWAYS EXECUTE CODE2:
			remove_tree( n->n[ 0 ] );
			remove_tree( n->n[ 1 ] );
			n->n[ 0 ] = 0;
			n->n[ 1 ] = 0;
			n->n[ 2 ]->flags = 0; //Just a statlist without any headers
		    }
		    else
		    {
			//IF 1 { CODE1 } ELSE { CODE2 } - IGNORE CODE2; ALWAYS EXECUTE CODE1:
			remove_tree( n->n[ 0 ] );
			remove_tree( n->n[ 2 ] );
			n->n[ 0 ] = 0;
			n->n[ 1 ]->flags = 0; //Just a statlist without any headers
			n->n[ 2 ] = 0;
		    }
		}
	    }
	    break;
	case lnode_while:
	    {
		int v = 0;
		if( n->n[ LNODE_WHILE_COND_EXPR ]->type == lnode_int ) 
		{
		    if( n->n[ LNODE_WHILE_COND_EXPR ]->val.i == 0 )
			v = 1;
		    else
			v = 2;
		}
		else if( n->n[ LNODE_WHILE_COND_EXPR ]->type == lnode_float )
		{
		    if( n->n[ LNODE_WHILE_COND_EXPR ]->val.f == 0 )
			v = 1;
		    else
			v = 2;
		}
		if( v )
		{
		    if( v == 1 )
		    {
			//WHILE 0 { CODE1 } - IGNORE THIS CODE:
			for( int i = LNODE_WHILE_INIT_STATLIST + 1; i < LNODE_WHILE_SIZE; i++ )
			{
			    remove_tree( n->n[ i ] );
			    n->n[ i ] = NULL;
			}
			n->nn = 0;
			if( n->n[ LNODE_WHILE_INIT_STATLIST ] )
			    n->nn = 1;
		    }
		    else
		    {
			//WHILE 1 { CODE1 } - INFINITE LOOP:
			clean_tree( n->n[ LNODE_WHILE_COND_EXPR ] );
			clean_tree( n->n[ LNODE_WHILE_JMP_TO_END ] );
		    }
		}
	    }
	    break;

	case lnode_sub:
	case lnode_add:
	case lnode_mul:
	case lnode_idiv:
	case lnode_div:
	case lnode_mod:
	case lnode_and:
	case lnode_or:
	case lnode_xor:
	case lnode_andand:
	case lnode_oror:
	case lnode_eq:
	case lnode_neq:
	case lnode_less:
	case lnode_leq:
	case lnode_greater:
	case lnode_geq:
	case lnode_lshift:
	case lnode_rshift:
	    if( ( n->n[ 0 ]->type == lnode_int || n->n[ 0 ]->type == lnode_float ) &&
		( n->n[ 1 ]->type == lnode_int || n->n[ 1 ]->type == lnode_float ) )
	    {
		bool integer_op = 0;
		if( n->n[ 0 ]->type == lnode_int && n->n[ 1 ]->type == lnode_int ) //Both operands are integer
		    integer_op = 1;
		switch( n->type )
		{
		    case lnode_idiv:
		    case lnode_mod:
		    case lnode_and:
		    case lnode_or:
		    case lnode_xor:
		    case lnode_lshift:
		    case lnode_rshift:
			integer_op = 1; //Allways integer
			break;
		    case lnode_div:
			integer_op = 0; //Allways float
			break;
		    default:
		        break;
		}
		if( integer_op )
		{
		    PIX_INT i1, i2, res = 0;
		    if( n->n[ 0 ]->type == lnode_float )
			i1 = (PIX_INT)n->n[ 0 ]->val.f;
		    else
			i1 = n->n[ 0 ]->val.i;
		    if( n->n[ 1 ]->type == lnode_float )
			i2 = (PIX_INT)n->n[ 1 ]->val.f;
		    else
			i2 = n->n[ 1 ]->val.i;
		    switch( n->type )
		    {
			case lnode_sub: res = i1 - i2; break;
			case lnode_add: res = i1 + i2; break;
			case lnode_mul: res = i1 * i2; break;
			case lnode_idiv: res = i1 / i2; break;
			case lnode_mod: res = i1 % i2; break;
			case lnode_and: res = i1 & i2; break;
			case lnode_or: res = i1 | i2; break;
			case lnode_xor: res = i1 ^ i2; break;
			case lnode_andand: res = ( i1 && i2 ); break;
			case lnode_oror: res = ( i1 || i2 ); break;
			case lnode_eq: res = ( i1 == i2 ); break;
			case lnode_neq: res = !( i1 == i2 ); break;
			case lnode_less: res = ( i1 < i2 ); break;
			case lnode_leq: res = ( i1 <= i2 ); break;
			case lnode_greater: res = ( i1 > i2 ); break;
			case lnode_geq: res = ( i1 >= i2 ); break;
			case lnode_lshift: res = ( i1 << (unsigned)i2 ); break;
			case lnode_rshift: res = ( i1 >> (unsigned)i2 ); break;
			default:
		    	    break;
		    }
		    n->type = lnode_int;
		    n->val.i = res;
		}
		else
		{
		    PIX_FLOAT f1, f2, res = 0;
		    if( n->n[ 0 ]->type == lnode_float )
			f1 = n->n[ 0 ]->val.f;
		    else
			f1 = (PIX_FLOAT)n->n[ 0 ]->val.i;
		    if( n->n[ 1 ]->type == lnode_float )
			f2 = n->n[ 1 ]->val.f;
		    else
			f2 = (PIX_FLOAT)n->n[ 1 ]->val.i;
		    bool int_res = 0;
		    switch( n->type )
		    {
			case lnode_sub: res = f1 - f2; break;
			case lnode_add: res = f1 + f2; break;
			case lnode_mul: res = f1 * f2; break;
			case lnode_div: res = f1 / f2; break;
			case lnode_andand: res = ( f1 && f2 ); int_res = 1; break;
			case lnode_oror: res = ( f1 || f2 ); int_res = 1; break;
			case lnode_eq: res = ( f1 == f2 ); int_res = 1; break;
			case lnode_neq: res = !( f1 == f2 ); int_res = 1; break;
			case lnode_less: res = ( f1 < f2 ); int_res = 1; break;
			case lnode_leq: res = ( f1 <= f2 ); int_res = 1; break;
			case lnode_greater: res = ( f1 > f2 ); int_res = 1; break;
			case lnode_geq: res = ( f1 >= f2 ); int_res = 1; break;
			default:
		    	    break;
		    }
		    if( int_res )
		    {
			n->type = lnode_int;
			n->val.i = (PIX_INT)res;
		    }
		    else
		    {
			n->type = lnode_float;
			n->val.f = res;
		    }
		}
		remove_tree( n->n[ 0 ] );
		remove_tree( n->n[ 1 ] );
		n->nn = 0;
	    }
	    break;

	case lnode_neg:
	    switch( n->n[ 0 ]->type )
	    {
		case lnode_int:
		    n->type = lnode_int;
		    n->val.i = -n->n[ 0 ]->val.i;
		    remove_tree( n->n[ 0 ] );
		    n->nn = 0;
		    break;
		case lnode_float:
		    n->type = lnode_float;
		    n->val.f = -n->n[ 0 ]->val.f;
		    remove_tree( n->n[ 0 ] );
		    n->nn = 0;
		    break;
		default:
		    break;
	    }
	    break;
	case lnode_lnot:
	    switch( n->n[ 0 ]->type )
	    {
		case lnode_int:
		    n->type = lnode_int;
		    n->val.i = !(n->n[ 0 ]->val.i);
		    remove_tree( n->n[ 0 ] );
		    n->nn = 0;
		    break;
		case lnode_float:
		    n->type = lnode_float;
		    n->val.f = !((PIX_INT)n->n[ 0 ]->val.f);
		    remove_tree( n->n[ 0 ] );
		    n->nn = 0;
		    break;
		default:
		    break;
	    }
	    break;
	case lnode_bnot:
	    switch( n->n[ 0 ]->type )
	    {
		case lnode_int:
		    n->type = lnode_int;
		    n->val.i = ~n->n[ 0 ]->val.i;
		    remove_tree( n->n[ 0 ] );
		    n->nn = 0;
		    break;
		case lnode_float:
		    n->type = lnode_float;
		    n->val.f = ~(PIX_INT)n->n[ 0 ]->val.f;
		    remove_tree( n->n[ 0 ] );
		    n->nn = 0;
		    break;
		default:
		    break;
	    }
	    break;

	case lnode_call_builtin_fn:
	case lnode_call_builtin_fn_void:
	case lnode_call:
	case lnode_call_void:
	    {
		lnode* pars = n->n[ 0 ];
	        //Flip parameters:
	        for( uint i = 0; i < pars->nn / 2; i++ )
	        {
		    lnode* temp_node = pars->n[ i ];
		    pars->n[ i ] = pars->n[ pars->nn - 1 - i ];
		    pars->n[ pars->nn - 1 - i ] = temp_node;
	        }
	    }
	    break;

	default:
	    break;
    }
}

static void compile_tree( pix_compiler* pcomp, lnode* n )
{
    if( !n ) return;

    n->code_ptr = pcomp->vm->code_ptr;

    switch( n->type )
    {
	case lnode_statlist:
	    if( ( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_HEADER ) ||
	        ( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER ) )
	    {
		DPRINT( "%d: STATLIST_HEADER (%d NOPs)\n", (int)pcomp->vm->code_ptr, pcomp->statlist_header_size );
		for( int i = 0; i < pcomp->statlist_header_size; i++ )
		    pix_vm_put_opcode( OPCODE_NOP, pcomp->vm );
	    }
	    break;
	default:
	    break;
    }

    if( n->n )
    {
	for( uint i = 0; i < n->nn; i++ )
	{
	    compile_tree( pcomp, n->n[ i ] );
	}
    }

    switch( n->type )
    {
	case lnode_statlist:
	    if( ( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_HEADER ) ||
	        ( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER ) )
	    {
		//Add "skip statlist" code:
		int mode = 0; //0 - JMP; 1 - JMP if FALSE;
		size_t dest_ptr = pcomp->vm->code_ptr;
		if( n->flags & LNODE_FLAG_STATLIST_WITH_JMP_IF_FALSE_HEADER ) mode = 1;
		if( n->flags & LNODE_FLAG_STATLIST_SKIP_NEXT_HEADER ) dest_ptr += pcomp->statlist_header_size;
		size_t prev_code_ptr = pcomp->vm->code_ptr;
		pcomp->vm->code_ptr = n->code_ptr;
		push_jmp( pcomp, dest_ptr, mode );
		pcomp->vm->code_ptr = prev_code_ptr;
		if( n->flags & LNODE_FLAG_STATLIST_AS_EXPRESSION )
		{
		    push_int( pcomp, ( n->code_ptr + pcomp->statlist_header_size ) | PIX_INT_ADDRESS_MARKER );
		}
	    }
	    break;

	case lnode_int:
	    push_int( pcomp, n->val.i );
	    break;
	case lnode_float:
	    DPRINT( "%d: PUSH_F %d\n", (int)pcomp->vm->code_ptr, (int)n->val.f );
	    pix_vm_put_opcode( OPCODE_PUSH_F, pcomp->vm );
	    pix_vm_put_float( n->val.f, pcomp->vm );
	    break;
	case lnode_var:
	    DPRINT( "%d: PUSH_v %d\n", (int)pcomp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_PUSH_v | ( n->val.i << PIX_OPCODE_BITS ), pcomp->vm );
	    break;

	case lnode_halt:
	    DPRINT( "%d: HALT\n", (int)pcomp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_HALT, pcomp->vm );
	    break;

        case lnode_label:
	    DPRINT( "LABEL var:%d offset:%d\n", (int)n->val.i, (int)pcomp->vm->code_ptr );
	    pcomp->vm->vars[ n->val.i ].i = pcomp->vm->code_ptr | PIX_INT_ADDRESS_MARKER;
	    pcomp->vm->vars[ n->val.i ].t = 0;
	    break;
	case lnode_function_label_from_node:
	    DPRINT( "FUNCTION var:%d offset:%d\n", (int)n->val.i, (int)((lnode*)n->n[ 0 ]->val.p)->code_ptr + pcomp->statlist_header_size );
	    pcomp->vm->vars[ n->val.i ].i = ( ((lnode*)n->n[ 0 ]->val.p)->code_ptr + pcomp->statlist_header_size ) | PIX_INT_ADDRESS_MARKER;
	    pcomp->vm->vars[ n->val.i ].t = 0;
	    break;
	    
	case lnode_go:
	    DPRINT( "%d: GO\n", (int)pcomp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_GO, pcomp->vm );
	    break;
	    
	case lnode_jmp_to_node:
	case lnode_jmp_to_end_of_node:
	    {
		size_t ptr;
		if( n->type == lnode_jmp_to_node )
		    ptr = ((lnode*)n->val.p)->code_ptr;
		else
		    ptr = ((lnode*)n->val.p)->code_ptr2;
		int mode = 0;
		if( n->flags & LNODE_FLAG_JMP_IF_FALSE ) mode = 1;
		push_jmp( pcomp, ptr, mode );
		if( ptr == 0 )
		{
		    if( pcomp->fixup_num + 1 >= smem_get_size( pcomp->fixup ) / sizeof( lnode* ) )
			pcomp->fixup = (lnode**)smem_resize2( pcomp->fixup, smem_get_size( pcomp->fixup ) + 8 * sizeof( lnode* ) );
		    pcomp->fixup[ pcomp->fixup_num ] = n;
		    pcomp->fixup_num++;
		}
	    }
	    break;

	case lnode_save_to_var:
	    DPRINT( "%d: SAVE_TO_VAR_v ( %d << OB )\n", (int)pcomp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_SAVE_TO_VAR_v | ( n->val.i << PIX_OPCODE_BITS ), pcomp->vm );
	    break;

	case lnode_save_to_prop:
	    DPRINT( "%d: SAVE_TO_PROP_I %d\n", (int)pcomp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_SAVE_TO_PROP_I, pcomp->vm );
	    pix_vm_put_int( n->val.i, pcomp->vm );
	    break;
	case lnode_load_from_prop:
	    DPRINT( "%d: LOAD_FROM_PROP_I %d\n", (int)pcomp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_LOAD_FROM_PROP_I, pcomp->vm );
	    pix_vm_put_int( n->val.i, pcomp->vm );
	    break;
	    
	case lnode_save_to_mem:
	    DPRINT( "%d: SAVE_TO_MEM\n", (int)pcomp->vm->code_ptr );
	    if( n->n[ 1 ]->type == lnode_empty && n->n[ 1 ]->nn == 2 )
		pix_vm_put_opcode( OPCODE_SAVE_TO_SMEM_2D, pcomp->vm );
	    else
		pix_vm_put_opcode( OPCODE_SAVE_TO_MEM, pcomp->vm );
	    break;
	case lnode_load_from_mem:
	    DPRINT( "%d: LOAD_FROM_MEM\n", (int)pcomp->vm->code_ptr );
	    if( n->n[ 1 ]->type == lnode_empty && n->n[ 1 ]->nn == 2 )
		pix_vm_put_opcode( OPCODE_LOAD_FROM_SMEM_2D, pcomp->vm );
	    else
		pix_vm_put_opcode( OPCODE_LOAD_FROM_MEM, pcomp->vm );
	    break;
	
	case lnode_save_to_stackframe:
	    DPRINT( "%d: SAVE_TO_STACKFRAME_i ( %d << OB )\n", (int)pcomp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_SAVE_TO_STACKFRAME_i | ( (PIX_OPCODE)n->val.i << PIX_OPCODE_BITS ), pcomp->vm );
	    break;
	case lnode_load_from_stackframe:
	    DPRINT( "%d: LOAD_FROM_STACKFRAME_i ( %d << OB )\n", (int)pcomp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_LOAD_FROM_STACKFRAME_i | ( (PIX_OPCODE)n->val.i << PIX_OPCODE_BITS ), pcomp->vm );
	    break;
    
	case lnode_sub:
	case lnode_add:
	case lnode_mul:
	case lnode_idiv:
	case lnode_div:
	case lnode_mod:
	case lnode_and:
	case lnode_or:
	case lnode_xor:
	case lnode_andand:
	case lnode_oror:
	case lnode_eq:
	case lnode_neq:
	case lnode_less:
	case lnode_leq:
	case lnode_greater:
	case lnode_geq:
	case lnode_lshift:
	case lnode_rshift:
	    DPRINT( "%d: MATH OP %d\n", (int)pcomp->vm->code_ptr, n->type - lnode_sub );
	    pix_vm_put_opcode( n->type - lnode_sub + OPCODE_SUB, pcomp->vm );
	    break;
    
	case lnode_neg:
	    DPRINT( "%d: NEG\n", (int)pcomp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_NEG, pcomp->vm );
	    break;
	case lnode_lnot:
	    DPRINT( "%d: LNOT\n", (int)pcomp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_LOGICAL_NOT, pcomp->vm );
	    break;
	case lnode_bnot:
	    DPRINT( "%d: BNOT\n", (int)pcomp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_BITWISE_NOT, pcomp->vm );
	    break;
	    
	case lnode_call_builtin_fn:
	    DPRINT( "%d: CALL_BUILTIN_FN ( %d << OB+FB ) ( %d << OB ) (%s)\n", (int)pcomp->vm->code_ptr, (int)n->n[ 0 ]->nn, (int)n->val.i, g_pix_fn_names[ n->val.i ] );
	    pix_vm_put_opcode( OPCODE_CALL_BUILTIN_FN | ( n->n[ 0 ]->nn << ( PIX_OPCODE_BITS + PIX_FN_BITS ) ) | ( n->val.i << PIX_OPCODE_BITS ), pcomp->vm );
	    break;
	case lnode_call_builtin_fn_void:
	    DPRINT( "%d: CALL_BUILTIN_FN_VOID ( %d << OB+FB ) ( %d << OB ) (%s)\n", (int)pcomp->vm->code_ptr, (int)n->n[ 0 ]->nn, (int)n->val.i, g_pix_fn_names[ n->val.i ] );
	    pix_vm_put_opcode( OPCODE_CALL_BUILTIN_FN_VOID | ( n->n[ 0 ]->nn << ( PIX_OPCODE_BITS + PIX_FN_BITS ) ) | ( n->val.i << PIX_OPCODE_BITS ), pcomp->vm );
	    break;
	case lnode_call:
	case lnode_call_void:
	    DPRINT( "%d: CALL_i ( %d << OB )\n", (int)pcomp->vm->code_ptr, (int)n->n[ 0 ]->nn );
	    pix_vm_put_opcode( OPCODE_CALL_i | ( n->n[ 0 ]->nn << PIX_OPCODE_BITS ), pcomp->vm );
	    if( n->type == lnode_call_void )
	    {
		DPRINT( "%d: INC_SP_i ( %d << OB )\n", (int)pcomp->vm->code_ptr, 1 );
		pix_vm_put_opcode( OPCODE_INC_SP_i | ( 1 << PIX_OPCODE_BITS ), pcomp->vm );
	    }
	    break;
	case lnode_ret_int:
	    if( n->val.i < ( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) &&
		n->val.i > -( 1 << ( ( sizeof( PIX_OPCODE ) * 8 ) - ( PIX_OPCODE_BITS + 1 ) ) ) )
	    {
		DPRINT( "%d: RET_i ( %d << OB )\n", (int)pcomp->vm->code_ptr, (int)n->val.i );
		pix_vm_put_opcode( OPCODE_RET_i | ( (PIX_OPCODE)n->val.i << PIX_OPCODE_BITS ), pcomp->vm );
	    }
	    else
	    {
		DPRINT( "%d: RET_I %d\n", (int)pcomp->vm->code_ptr, (int)n->val.i );
		pix_vm_put_opcode( OPCODE_RET_I, pcomp->vm );
		pix_vm_put_int( n->val.i, pcomp->vm );
	    }
	    break;
	case lnode_ret:
	    DPRINT( "%d: RET\n", (int)pcomp->vm->code_ptr );
	    pix_vm_put_opcode( OPCODE_RET, pcomp->vm );
	    break;
	case lnode_inc_sp:
	    DPRINT( "%d: INC_SP_i ( %d << OB )\n", (int)pcomp->vm->code_ptr, (int)n->val.i );
	    pix_vm_put_opcode( OPCODE_INC_SP_i | ( (PIX_OPCODE)n->val.i << PIX_OPCODE_BITS ), pcomp->vm );
	    break;

	default:
	    break;
    }

    n->code_ptr2 = pcomp->vm->code_ptr;
}

void fix_up( pix_compiler* pcomp )
{
    if( pcomp->fixup && pcomp->fixup_num )
    {
	for( uint i = 0; i < pcomp->fixup_num; i++ )
	{
	    lnode* n = pcomp->fixup[ i ];
    	    int mode = 0;
	    if( n->flags & LNODE_FLAG_JMP_IF_FALSE ) mode = 1;
	    size_t prev_code_ptr = pcomp->vm->code_ptr;
    	    pcomp->vm->code_ptr = n->code_ptr;
	    switch( n->type )
	    {
		case lnode_jmp_to_node:
            	    push_jmp( pcomp, ((lnode*)n->val.p)->code_ptr, mode );
		    break;
		case lnode_jmp_to_end_of_node:
            	    push_jmp( pcomp, ((lnode*)n->val.p)->code_ptr2, mode );
		    break;
		default:
		    break;
	    }
    	    pcomp->vm->code_ptr = prev_code_ptr;
	}
    }
}

#define ADD_SYMBOL( sname, stype, sval ) \
{ \
    if( stype == SYMTYPE_NUM_F ) \
	pix_symtab_lookup( sname, -1, 1, stype, 0, sval, 0, &pcomp->sym ); \
    else \
	pix_symtab_lookup( sname, -1, 1, stype, sval, 0, 0, &pcomp->sym ); \
}

int pix_compile( char* src, int src_size, char* src_name, char* base_path, pix_vm* vm )
{
    int rv = 0;

    DPRINT( "Pixilang compiler started.\n" );

    stime_ms_t start_time = stime_ms();

    DPRINT( "Init...\n" );
    pix_compiler* pcomp = (pix_compiler*)smem_znew( sizeof( pix_compiler ) );
    if( !pcomp ) 
    {
        ERROR( "memory allocation error" );
	rv = 2;
	goto compiler_end;
    }
    pcomp->vm = vm;
    pcomp->src = src;
    pcomp->src_size = src_size;
    pcomp->src_name = src_name;
    if( 0 ) 
    {
	if( sizeof( PIX_INT ) >= sizeof( PIX_OPCODE ) )
    	    pcomp->statlist_header_size = 1 + sizeof( PIX_INT ) / sizeof( PIX_OPCODE );
        else
	    pcomp->statlist_header_size = 1 + 1;
    }
    else
    {
	pcomp->statlist_header_size = 1;
    }
    //Global symbol table:
    if( pix_symtab_init( PIX_COMPILER_SYMTAB_SIZE, &pcomp->sym ) )
    {
	rv = 3;
	goto compiler_end;
    }
    //Local symbol tables:
    pcomp->lsym = (pix_lsymtab*)smem_new( 8 * sizeof( pix_lsymtab ) );
    if( !pcomp->lsym )
    {
        ERROR( "memory allocation error" );
	rv = 4;
	goto compiler_end;
    }
    smem_zero( pcomp->lsym );
    //Set base path:
    pcomp->base_path = base_path;
    DPRINT( "Base path: %s\n", base_path );

    DPRINT( "VM init...\n" );
    vm->vars_num = 128; //standard set of variables with one-char ASCII names
    vm->vars_num += PIX_GVARS - vm->vars_num;
    pix_vm_resize_variables( vm );
    pcomp->var_flags = (char*)smem_new( vm->vars_num );
    pcomp->var_flags_size = vm->vars_num;
    smem_zero( pcomp->var_flags );
    smem_free( vm->base_path );
    vm->base_path = (char*)smem_new( smem_strlen( base_path ) + 1 );
    if( !vm->base_path )
    {
        ERROR( "memory allocation error" );
	rv = 5;
	goto compiler_end;
    }
    vm->base_path[ 0 ] = 0;
    smem_strcat_resize( vm->base_path, base_path );
    //Screen (container):
    vm->screen = pix_vm_new_container( -1, 16, 16, 32, 0, vm );
    smem_zero( pix_vm_get_container_data( vm->screen, vm ) );
    pix_vm_set_container_flags( vm->screen, pix_vm_get_container_flags( vm->screen, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
    pix_vm_gfx_set_screen( vm->screen, vm );
    //Fonts:
    {
	for( int i = 0; i < vm->fonts_num; i++ )
	    vm->fonts[ i ].font = -1;
	COLORPTR font_data = (COLORPTR)smem_new( g_font8x8_xsize * g_font8x8_ysize * COLORLEN );
	if( !font_data )
	{
    	    ERROR( "memory allocation error" );
	    rv = 6;
	    goto compiler_end;
	}
	int src_ptr = 0;
	int dest_ptr = 0;
	for( int y = 0; y < g_font8x8_ysize; y++ )
	{
	    for( int x = 0; x < g_font8x8_xsize; x += 8 )
	    {
		int byte = g_font8x8[ src_ptr ];
		for( int i = 0; i < 8; i++ )
		{
		    if( byte & 128 ) font_data[ dest_ptr ] = get_color( 255, 255, 255 ); else font_data[ dest_ptr ] = get_color( 0, 0, 0 );
		    byte <<= 1;
		    dest_ptr++;
		}
		src_ptr++;
	    }
	}
	PIX_CID fc = pix_vm_new_container( -1, g_font8x8_xsize, g_font8x8_ysize, 32, font_data, vm );
	pix_vm_set_container_flags( fc, pix_vm_get_container_flags( fc, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
	pix_vm_font* font = &vm->fonts[ 0 ];
	font->font = fc;
	font->first = 32;
	font->last = 32 + g_font8x8_xchars * g_font8x8_ychars - 1;
	font->xchars = g_font8x8_xchars;
	font->ychars = g_font8x8_ychars;
	font->char_xsize = g_font8x8_xsize / g_font8x8_xchars;
	font->char_ysize = g_font8x8_ysize / g_font8x8_ychars;
	font->char_xsize2 = font->char_xsize;
	font->char_ysize2 = font->char_ysize;
	font->grid_xoffset = 0;
	font->grid_yoffset = 0;
	font->grid_cell_xsize = font->char_xsize;
	font->grid_cell_ysize = font->char_ysize;
	pix_vm_set_container_flags( fc, pix_vm_get_container_flags( fc, vm ) | PIX_CONTAINER_FLAG_USES_KEY, vm );
	pix_vm_set_container_key_color( fc, font_data[ 0 ], vm );
    }
    //Current event (container):
    vm->event = pix_vm_new_container( -1, 16, 1, PIX_CONTAINER_TYPE_INT32, 0, vm );
    smem_zero( pix_vm_get_container_data( vm->event, vm ) );
    pix_vm_set_container_flags( vm->event, pix_vm_get_container_flags( vm->event, vm ) | PIX_CONTAINER_FLAG_SYSTEM_MANAGED, vm );
    //System info (containers):
    pix_vm_set_systeminfo_containers( vm );
    //Pixilang VM info (features/modes):
    pix_vm_set_pixiinfo( vm );
    //Window parameters:
    vm->vars[ PIX_GVAR_WINDOW_XSIZE ].t = 0;
    vm->vars[ PIX_GVAR_WINDOW_YSIZE ].t = 0;
    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_X ].t = 0;
    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_Y ].t = 0;
    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_W ].t = 0;
    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_H ].t = 0;
    vm->vars[ PIX_GVAR_FPS ].t = 0;
    vm->vars[ PIX_GVAR_PPI ].t = 0;
    vm->vars[ PIX_GVAR_SCALE ].t = 1;
    vm->vars[ PIX_GVAR_FONT_SCALE ].t = 1;
    vm->vars[ PIX_GVAR_WINDOW_XSIZE ].i = 16;
    vm->vars[ PIX_GVAR_WINDOW_YSIZE ].i = 16;
    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_X ].i = 0;
    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_Y ].i = 0;
    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_W ].i = 0;
    vm->vars[ PIX_GVAR_WINDOW_SAFE_AREA_H ].i = 0;
    vm->vars[ PIX_GVAR_FPS ].i = 0;
    vm->vars[ PIX_GVAR_PPI ].i = 0;
    vm->vars[ PIX_GVAR_SCALE ].f = 1;
    vm->vars[ PIX_GVAR_FONT_SCALE ].f = 1;

    DPRINT( "Adding base symbols...\n" );
    ADD_SYMBOL( "while", SYMTYPE_WHILE, 0 );
    ADD_SYMBOL( "for", SYMTYPE_FOR, 0 );
    ADD_SYMBOL( "break", SYMTYPE_BREAK, 0 );
    ADD_SYMBOL( "break2", SYMTYPE_BREAK2, 0 );
    ADD_SYMBOL( "break3", SYMTYPE_BREAK3, 0 );
    ADD_SYMBOL( "break4", SYMTYPE_BREAK4, 0 );
    ADD_SYMBOL( "breakall", SYMTYPE_BREAKALL, 0 );
    ADD_SYMBOL( "continue", SYMTYPE_CONTINUE, 0 );
    ADD_SYMBOL( "if", SYMTYPE_IF, 0 );
    ADD_SYMBOL( "else", SYMTYPE_ELSE, 0 );
    ADD_SYMBOL( "go", SYMTYPE_GO, 0 );
    ADD_SYMBOL( "goto", SYMTYPE_GO, 0 );
    ADD_SYMBOL( "ret", SYMTYPE_RET, 0 );
    ADD_SYMBOL( "div", SYMTYPE_IDIV, 0 );
    ADD_SYMBOL( "halt", SYMTYPE_HALT, 0 );
    ADD_SYMBOL( "fn", SYMTYPE_FNDEF, 0 );
    ADD_SYMBOL( "include", SYMTYPE_INCLUDE, 0 );

    DPRINT( "Adding functions...\n" );
    for( int i = 0; i < FN_NUM; i++ )
    {
	ADD_SYMBOL( g_pix_fn_names[ i ], SYMTYPE_FNNUM, i );
    }
    //Aliases:
    ADD_SYMBOL( "num2str", SYMTYPE_FNNUM, FN_NUM_TO_STRING );
    ADD_SYMBOL( "str2num", SYMTYPE_FNNUM, FN_STRING_TO_NUM );

    DPRINT( "Adding constants...\n" );
    //STDIO constants:
    ADD_SYMBOL( "FOPEN_MAX", SYMTYPE_NUM_I, SFS_FOPEN_MAX ); //Number of streams which the implementation guarantees can be open simultaneously.
    ADD_SYMBOL( "SEEK_CUR", SYMTYPE_NUM_I, SEEK_CUR ); //Seek relative to current position.
    ADD_SYMBOL( "SEEK_END", SYMTYPE_NUM_I, SEEK_END ); //Seek relative to end-of-file.
    ADD_SYMBOL( "SEEK_SET", SYMTYPE_NUM_I, SEEK_SET ); //Seek relative to start-of-file.
    ADD_SYMBOL( "EOF", SYMTYPE_NUM_I, -1 ); //End-of-file return value.
    ADD_SYMBOL( "STDIN", SYMTYPE_NUM_I, SFS_STDIN ); //Standard input stream.
    ADD_SYMBOL( "STDOUT", SYMTYPE_NUM_I, SFS_STDOUT ); //Standard output stream.
    ADD_SYMBOL( "STDERR", SYMTYPE_NUM_I, SFS_STDERR ); //Standard error output stream.
    //ZLib constants:
    ADD_SYMBOL( "Z_NO_COMPRESSION", SYMTYPE_NUM_I, Z_NO_COMPRESSION );
    ADD_SYMBOL( "Z_BEST_SPEED", SYMTYPE_NUM_I, Z_BEST_SPEED );
    ADD_SYMBOL( "Z_BEST_COMPRESSION", SYMTYPE_NUM_I, Z_BEST_COMPRESSION );
    ADD_SYMBOL( "Z_DEFAULT_COMPRESSION", SYMTYPE_NUM_I, Z_DEFAULT_COMPRESSION );
    //Container flags:
    ADD_SYMBOL( "GL_MIN_LINEAR", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_MIN_LINEAR );
    ADD_SYMBOL( "GL_MAG_LINEAR", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_MAG_LINEAR );
    ADD_SYMBOL( "GL_NO_XREPEAT", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_NO_XREPEAT );
    ADD_SYMBOL( "GL_NO_YREPEAT", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_NO_YREPEAT );
    ADD_SYMBOL( "GL_NICEST", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_NICEST );
    ADD_SYMBOL( "GL_NO_ALPHA", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_NO_ALPHA );
    ADD_SYMBOL( "GL_NPOT", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_GL_NPOT );
    ADD_SYMBOL( "CFLAG_INTERP", SYMTYPE_NUM_I, PIX_CONTAINER_FLAG_INTERP );
    //Container copying flags:
    ADD_SYMBOL( "COPY_NO_AUTOROTATE", SYMTYPE_NUM_I, PIX_COPY_NO_AUTOROTATE );
    ADD_SYMBOL( "COPY_CLIPPING", SYMTYPE_NUM_I, PIX_COPY_CLIPPING );
    //Container resizing flags:
    ADD_SYMBOL( "RESIZE_INTERP1", SYMTYPE_NUM_I, PIX_RESIZE_INTERP1 );
    ADD_SYMBOL( "RESIZE_INTERP2", SYMTYPE_NUM_I, PIX_RESIZE_INTERP2 );
    ADD_SYMBOL( "RESIZE_UNSIGNED_INTERP2", SYMTYPE_NUM_I, PIX_RESIZE_INTERP2 | PIX_RESIZE_INTERP_UNSIGNED );
    ADD_SYMBOL( "RESIZE_COLOR_INTERP1", SYMTYPE_NUM_I, PIX_RESIZE_INTERP1 );
    ADD_SYMBOL( "RESIZE_COLOR_INTERP2", SYMTYPE_NUM_I, PIX_RESIZE_INTERP2 | PIX_RESIZE_INTERP_COLOR );
    //Convolution filter flags:
    ADD_SYMBOL( "CONV_FILTER_COLOR", SYMTYPE_NUM_I, PIX_CONV_FILTER_TYPE_COLOR );
    ADD_SYMBOL( "CONV_FILTER_BORDER_EXTEND", SYMTYPE_NUM_I, PIX_CONV_FILTER_BORDER_EXTEND );
    ADD_SYMBOL( "CONV_FILTER_BORDER_SKIP", SYMTYPE_NUM_I, PIX_CONV_FILTER_BORDER_SKIP );
    ADD_SYMBOL( "CONV_FILTER_UNSIGNED", SYMTYPE_NUM_I, PIX_CONV_FILTER_UNSIGNED );
    //Colors:
    ADD_SYMBOL( "ORANGE", SYMTYPE_NUM_I, get_color( 255, 128, 16 ) );
    ADD_SYMBOL( "ORANJ", SYMTYPE_NUM_I, get_color( 255, 128, 16 ) );
    ADD_SYMBOL( "BLACK", SYMTYPE_NUM_I, get_color( 0, 0, 0 ) );
    ADD_SYMBOL( "WHITE", SYMTYPE_NUM_I, get_color( 255, 255, 255 ) );
    ADD_SYMBOL( "SNEG", SYMTYPE_NUM_I, get_color( 255, 255, 255 ) );
    ADD_SYMBOL( "YELLOW", SYMTYPE_NUM_I, get_color( 255, 255, 0 ) );
    ADD_SYMBOL( "SUN", SYMTYPE_NUM_I, get_color( 255, 255, 0 ) );
    ADD_SYMBOL( "RED", SYMTYPE_NUM_I, get_color( 255, 0, 0 ) );
    ADD_SYMBOL( "GREEN", SYMTYPE_NUM_I, get_color( 0, 255, 0 ) );
    ADD_SYMBOL( "ZELEN", SYMTYPE_NUM_I, get_color( 0, 255, 0 ) );
    ADD_SYMBOL( "BLUE", SYMTYPE_NUM_I, get_color( 0, 0, 255 ) );
    //Alignment:
    ADD_SYMBOL( "TOP", SYMTYPE_NUM_I, 1 );
    ADD_SYMBOL( "BOTTOM", SYMTYPE_NUM_I, 2 );
    ADD_SYMBOL( "LEFT", SYMTYPE_NUM_I, 4 );
    ADD_SYMBOL( "RIGHT", SYMTYPE_NUM_I, 8 );
    //Effects:
    ADD_SYMBOL( "EFF_NOISE", SYMTYPE_NUM_I, PIX_EFFECT_NOISE );
    ADD_SYMBOL( "EFF_SPREAD_LEFT", SYMTYPE_NUM_I, PIX_EFFECT_SPREAD_LEFT );
    ADD_SYMBOL( "EFF_SPREAD_RIGHT", SYMTYPE_NUM_I, PIX_EFFECT_SPREAD_RIGHT );
    ADD_SYMBOL( "EFF_SPREAD_UP", SYMTYPE_NUM_I, PIX_EFFECT_SPREAD_UP );
    ADD_SYMBOL( "EFF_SPREAD_DOWN", SYMTYPE_NUM_I, PIX_EFFECT_SPREAD_DOWN );
    ADD_SYMBOL( "EFF_VBLUR", SYMTYPE_NUM_I, PIX_EFFECT_VBLUR );
    ADD_SYMBOL( "EFF_HBLUR", SYMTYPE_NUM_I, PIX_EFFECT_HBLUR );
    ADD_SYMBOL( "EFF_COLOR", SYMTYPE_NUM_I, PIX_EFFECT_COLOR );
    //Video (experimental):
    ADD_SYMBOL( "VIDEO_PROP_FRAME_WIDTH", SYMTYPE_NUM_I, SVIDEO_PROP_FRAME_WIDTH_I );
    ADD_SYMBOL( "VIDEO_PROP_FRAME_HEIGHT", SYMTYPE_NUM_I, SVIDEO_PROP_FRAME_HEIGHT_I );
    ADD_SYMBOL( "VIDEO_PROP_FPS", SYMTYPE_NUM_I, SVIDEO_PROP_FPS_I );
    ADD_SYMBOL( "VIDEO_PROP_FOCUS_MODE", SYMTYPE_NUM_I, SVIDEO_PROP_FOCUS_MODE_I );
    ADD_SYMBOL( "VIDEO_PROP_ORIENTATION", SYMTYPE_NUM_I, SVIDEO_PROP_ORIENTATION_I );
    ADD_SYMBOL( "VIDEO_FOCUS_MODE_AUTO", SYMTYPE_NUM_I, SVIDEO_FOCUS_MODE_AUTO );
    ADD_SYMBOL( "VIDEO_FOCUS_MODE_CONTINUOUS", SYMTYPE_NUM_I, SVIDEO_FOCUS_MODE_CONTINUOUS );
    ADD_SYMBOL( "VIDEO_OPEN_FLAG_READ", SYMTYPE_NUM_I, SVIDEO_OPEN_FLAG_READ );
    ADD_SYMBOL( "VIDEO_OPEN_FLAG_WRITE", SYMTYPE_NUM_I, SVIDEO_OPEN_FLAG_WRITE );
    ADD_SYMBOL( "VIDEO_CAPTURE_FLAG_NO_AUTOROTATE", SYMTYPE_NUM_I, 1 );
    ADD_SYMBOL( "WM_VIDEO_CAPTURE_FLAG_AUDIO_FROM_INPUT", SYMTYPE_NUM_I, VCAP_FLAG_AUDIO_FROM_INPUT );
    //OpenGL:
#ifdef OPENGL
    //gl_draw_arrays() (analog of the glDrawArrays()) modes:
    ADD_SYMBOL( "GL_POINTS", SYMTYPE_NUM_I, GL_POINTS );
    ADD_SYMBOL( "GL_LINE_STRIP", SYMTYPE_NUM_I, GL_LINE_STRIP );
    ADD_SYMBOL( "GL_LINE_LOOP", SYMTYPE_NUM_I, GL_LINE_LOOP );
    ADD_SYMBOL( "GL_LINES", SYMTYPE_NUM_I, GL_LINES );
    ADD_SYMBOL( "GL_TRIANGLE_STRIP", SYMTYPE_NUM_I, GL_TRIANGLE_STRIP );
    ADD_SYMBOL( "GL_TRIANGLE_FAN", SYMTYPE_NUM_I, GL_TRIANGLE_FAN );
    ADD_SYMBOL( "GL_TRIANGLES", SYMTYPE_NUM_I, GL_TRIANGLES );
    //gl_blend_func() (analog of the glBlendFunc()) operations:
    ADD_SYMBOL( "GL_ZERO", SYMTYPE_NUM_I, GL_ZERO );
    ADD_SYMBOL( "GL_ONE", SYMTYPE_NUM_I, GL_ONE );
    ADD_SYMBOL( "GL_SRC_COLOR", SYMTYPE_NUM_I, GL_SRC_COLOR );
    ADD_SYMBOL( "GL_ONE_MINUS_SRC_COLOR", SYMTYPE_NUM_I, GL_ONE_MINUS_SRC_COLOR );
    ADD_SYMBOL( "GL_DST_COLOR", SYMTYPE_NUM_I, GL_DST_COLOR );
    ADD_SYMBOL( "GL_ONE_MINUS_DST_COLOR", SYMTYPE_NUM_I, GL_ONE_MINUS_DST_COLOR );
    ADD_SYMBOL( "GL_SRC_ALPHA", SYMTYPE_NUM_I, GL_SRC_ALPHA );
    ADD_SYMBOL( "GL_ONE_MINUS_SRC_ALPHA", SYMTYPE_NUM_I, GL_ONE_MINUS_SRC_ALPHA );
    ADD_SYMBOL( "GL_DST_ALPHA", SYMTYPE_NUM_I, GL_DST_ALPHA );
    ADD_SYMBOL( "GL_ONE_MINUS_DST_ALPHA", SYMTYPE_NUM_I, GL_ONE_MINUS_DST_ALPHA );
    ADD_SYMBOL( "GL_SRC_ALPHA_SATURATE", SYMTYPE_NUM_I, GL_SRC_ALPHA_SATURATE );
    //gl_bind_framebuffer() flags:
    ADD_SYMBOL( "GL_BFB_IDENTITY_MATRIX", SYMTYPE_NUM_I, GL_BFB_IDENTITY_MATRIX );
    //gl_new_prog() default shader names:
    ADD_SYMBOL( "GL_SHADER_SOLID", SYMTYPE_NUM_I, -1 - GL_SHADER_SOLID );
    ADD_SYMBOL( "GL_SHADER_GRAD", SYMTYPE_NUM_I, -1 - GL_SHADER_GRAD );
    ADD_SYMBOL( "GL_SHADER_TEX_ALPHA_SOLID", SYMTYPE_NUM_I, -1 - GL_SHADER_TEX_ALPHA_SOLID );
    ADD_SYMBOL( "GL_SHADER_TEX_ALPHA_GRAD", SYMTYPE_NUM_I, -1 - GL_SHADER_TEX_ALPHA_GRAD );
	//GL_SHADER_TEX_RGB_* is actually RGBA (with alpha!)
	//(we can't change it to GL_SHADER_TEX_RGBA_* because some pixi apps already use this const)
        //that's why we use PIX_GL_DATA_FLAG_ALPHA_FF in pixilang_vm_opengl.cpp ...
	//we probably need some new const like GL_SHADER_TEX_RGB1_* or GL_SHADER_TEX_RGB_NOALPHA_*
    ADD_SYMBOL( "GL_SHADER_TEX_RGB_SOLID", SYMTYPE_NUM_I, -1 - GL_SHADER_TEX_RGBA_SOLID );
    ADD_SYMBOL( "GL_SHADER_TEX_RGB_GRAD", SYMTYPE_NUM_I, -1 - GL_SHADER_TEX_RGBA_GRAD );
    //Values for gl_get_int() (glGetIntegerv) and gl_get_float() (glGetFloatv):
    ADD_SYMBOL( "GL_MAX_TEXTURE_SIZE", SYMTYPE_NUM_I, GL_MAX_TEXTURE_SIZE );
    ADD_SYMBOL( "GL_MAX_VERTEX_ATTRIBS", SYMTYPE_NUM_I, GL_MAX_VERTEX_ATTRIBS );
    ADD_SYMBOL( "GL_MAX_VERTEX_UNIFORM_VECTORS", SYMTYPE_NUM_I, GL_MAX_VERTEX_UNIFORM_VECTORS );
    ADD_SYMBOL( "GL_MAX_VARYING_VECTORS", SYMTYPE_NUM_I, GL_MAX_VARYING_VECTORS );
    ADD_SYMBOL( "GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS", SYMTYPE_NUM_I, GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS );
    ADD_SYMBOL( "GL_MAX_TEXTURE_IMAGE_UNITS", SYMTYPE_NUM_I, GL_MAX_TEXTURE_IMAGE_UNITS );
    ADD_SYMBOL( "GL_MAX_FRAGMENT_UNIFORM_VECTORS", SYMTYPE_NUM_I, GL_MAX_FRAGMENT_UNIFORM_VECTORS );
#endif
    //File formats:
    ADD_SYMBOL( "FORMAT_RAW", SYMTYPE_NUM_I, SFS_FILE_FMT_UNKNOWN );
    ADD_SYMBOL( "FORMAT_WAVE", SYMTYPE_NUM_I, SFS_FILE_FMT_WAVE );
    ADD_SYMBOL( "FORMAT_AIFF", SYMTYPE_NUM_I, SFS_FILE_FMT_AIFF );
    ADD_SYMBOL( "FORMAT_OGG", SYMTYPE_NUM_I, SFS_FILE_FMT_OGG );
    ADD_SYMBOL( "FORMAT_MP3", SYMTYPE_NUM_I, SFS_FILE_FMT_MP3 );
    ADD_SYMBOL( "FORMAT_FLAC", SYMTYPE_NUM_I, SFS_FILE_FMT_FLAC );
    ADD_SYMBOL( "FORMAT_MIDI", SYMTYPE_NUM_I, SFS_FILE_FMT_MIDI );
    ADD_SYMBOL( "FORMAT_SUNVOX", SYMTYPE_NUM_I, SFS_FILE_FMT_SUNVOX );
    ADD_SYMBOL( "FORMAT_SUNVOXMODULE", SYMTYPE_NUM_I, SFS_FILE_FMT_SUNVOXMODULE );
    ADD_SYMBOL( "FORMAT_XM", SYMTYPE_NUM_I, SFS_FILE_FMT_XM );
    ADD_SYMBOL( "FORMAT_MOD", SYMTYPE_NUM_I, SFS_FILE_FMT_MOD );
    ADD_SYMBOL( "FORMAT_JPEG", SYMTYPE_NUM_I, SFS_FILE_FMT_JPEG );
    ADD_SYMBOL( "FORMAT_PNG", SYMTYPE_NUM_I, SFS_FILE_FMT_PNG );
    ADD_SYMBOL( "FORMAT_GIF", SYMTYPE_NUM_I, SFS_FILE_FMT_GIF );
    ADD_SYMBOL( "FORMAT_AVI", SYMTYPE_NUM_I, SFS_FILE_FMT_AVI );
    ADD_SYMBOL( "FORMAT_MP4", SYMTYPE_NUM_I, SFS_FILE_FMT_MP4 );
    ADD_SYMBOL( "FORMAT_ZIP", SYMTYPE_NUM_I, SFS_FILE_FMT_ZIP );
    ADD_SYMBOL( "FORMAT_PIXICONTAINER", SYMTYPE_NUM_I, SFS_FILE_FMT_PIXICONTAINER );
    //Load/Save options (flags):
    ADD_SYMBOL( "GIF_GRAYSCALE", SYMTYPE_NUM_I, PIX_GIF_GRAYSCALE );
    ADD_SYMBOL( "GIF_DITHER", SYMTYPE_NUM_I, PIX_GIF_DITHER );
    ADD_SYMBOL( "JPEG_H1V1", SYMTYPE_NUM_I, PIX_JPEG_H1V1 );
    ADD_SYMBOL( "JPEG_H2V1", SYMTYPE_NUM_I, PIX_JPEG_H2V1 );
    ADD_SYMBOL( "JPEG_H2V2", SYMTYPE_NUM_I, PIX_JPEG_H2V2 );
    ADD_SYMBOL( "JPEG_TWOPASS", SYMTYPE_NUM_I, PIX_JPEG_TWOPASS );
    ADD_SYMBOL( "LOAD_FIRST_FRAME", SYMTYPE_NUM_I, PIX_LOAD_FIRST_FRAME );
    ADD_SYMBOL( "LOAD_AS_RAW", SYMTYPE_NUM_I, PIX_LOAD_AS_RAW );
    //System clipboard (copy/paste) content types (experimental):
    ADD_SYMBOL( "CLIPBOARD_TYPE_TEXT", SYMTYPE_NUM_I, sclipboard_type_utf8_text );
    ADD_SYMBOL( "CLIPBOARD_TYPE_IMAGE", SYMTYPE_NUM_I, sclipboard_type_image );
    ADD_SYMBOL( "CLIPBOARD_TYPE_AUDIO", SYMTYPE_NUM_I, sclipboard_type_audio );
    ADD_SYMBOL( "CLIPBOARD_TYPE_VIDEO", SYMTYPE_NUM_I, sclipboard_type_video );
    ADD_SYMBOL( "CLIPBOARD_TYPE_MOVIE", SYMTYPE_NUM_I, sclipboard_type_movie );
    ADD_SYMBOL( "CLIPBOARD_TYPE_AV", SYMTYPE_NUM_I, sclipboard_type_av );
    //export_import_file() (experimental):
    ADD_SYMBOL( "EIFILE_MODE_IMPORT", SYMTYPE_NUM_I, EIFILE_MODE_IMPORT );
    ADD_SYMBOL( "EIFILE_MODE_EXPORT", SYMTYPE_NUM_I, EIFILE_MODE_EXPORT );
    ADD_SYMBOL( "EIFILE_MODE_EXPORT2", SYMTYPE_NUM_I, EIFILE_MODE_EXPORT2 );
    ADD_SYMBOL( "EIFILE_FLAG_DELFILE", SYMTYPE_NUM_I, EIFILE_FLAG_DELFILE );
    //file_dialog() options (flags):
    ADD_SYMBOL( "FDIALOG_FLAG_LOAD", SYMTYPE_NUM_I, FDIALOG_FLAG_LOAD );
    //Audio:
    ADD_SYMBOL( "AUDIO_FLAG_INTERP2", SYMTYPE_NUM_I, PIX_AUDIO_FLAG_INTERP2 );
    //MIDI:
    ADD_SYMBOL( "MIDI_PORT_READ", SYMTYPE_NUM_I, MIDI_PORT_READ );
    ADD_SYMBOL( "MIDI_PORT_WRITE", SYMTYPE_NUM_I, MIDI_PORT_WRITE );
    ADD_SYMBOL( "MIDI_NO_DEVICE", SYMTYPE_NUM_I, MIDI_NO_DEVICE );
    //Events:
    ADD_SYMBOL( "EVT", SYMTYPE_NUM_I, vm->event );
    ADD_SYMBOL( "EVT_TYPE", SYMTYPE_NUM_I, 0 );
    ADD_SYMBOL( "EVT_FLAGS", SYMTYPE_NUM_I, 1 );
    ADD_SYMBOL( "EVT_TIME", SYMTYPE_NUM_I, 2 );
    ADD_SYMBOL( "EVT_X", SYMTYPE_NUM_I, 3 );
    ADD_SYMBOL( "EVT_Y", SYMTYPE_NUM_I, 4 );
    ADD_SYMBOL( "EVT_KEY", SYMTYPE_NUM_I, 5 );
    ADD_SYMBOL( "EVT_SCANCODE", SYMTYPE_NUM_I, 6 );
    ADD_SYMBOL( "EVT_PRESSURE", SYMTYPE_NUM_I, 7 );
    ADD_SYMBOL( "EVT_MOUSEBUTTONDOWN", SYMTYPE_NUM_I, PIX_EVT_MOUSEBUTTONDOWN );
    ADD_SYMBOL( "EVT_MOUSEBUTTONUP", SYMTYPE_NUM_I, PIX_EVT_MOUSEBUTTONUP );
    ADD_SYMBOL( "EVT_MOUSEMOVE", SYMTYPE_NUM_I, PIX_EVT_MOUSEMOVE );
    ADD_SYMBOL( "EVT_TOUCHBEGIN", SYMTYPE_NUM_I, PIX_EVT_TOUCHBEGIN );
    ADD_SYMBOL( "EVT_TOUCHEND", SYMTYPE_NUM_I, PIX_EVT_TOUCHEND );
    ADD_SYMBOL( "EVT_TOUCHMOVE", SYMTYPE_NUM_I, PIX_EVT_TOUCHMOVE );
    ADD_SYMBOL( "EVT_BUTTONDOWN", SYMTYPE_NUM_I, PIX_EVT_BUTTONDOWN );
    ADD_SYMBOL( "EVT_BUTTONUP", SYMTYPE_NUM_I, PIX_EVT_BUTTONUP );
    ADD_SYMBOL( "EVT_SCREENRESIZE", SYMTYPE_NUM_I, PIX_EVT_SCREENRESIZE );
    ADD_SYMBOL( "EVT_LOADSTATE", SYMTYPE_NUM_I, PIX_EVT_LOADSTATE );
    ADD_SYMBOL( "EVT_SAVESTATE", SYMTYPE_NUM_I, PIX_EVT_SAVESTATE );
    ADD_SYMBOL( "EVT_QUIT", SYMTYPE_NUM_I, PIX_EVT_QUIT );
    ADD_SYMBOL( "EVT_FLAG_SHIFT", SYMTYPE_NUM_I, EVT_FLAG_SHIFT );
    ADD_SYMBOL( "EVT_FLAG_CTRL", SYMTYPE_NUM_I, EVT_FLAG_CTRL );
    ADD_SYMBOL( "EVT_FLAG_ALT", SYMTYPE_NUM_I, EVT_FLAG_ALT );
    ADD_SYMBOL( "EVT_FLAG_MODE", SYMTYPE_NUM_I, EVT_FLAG_MODE );
    ADD_SYMBOL( "EVT_FLAG_CMD", SYMTYPE_NUM_I, EVT_FLAG_CMD );
    ADD_SYMBOL( "EVT_FLAG_MODS", SYMTYPE_NUM_I, EVT_FLAG_MODS );
    ADD_SYMBOL( "EVT_FLAG_DOUBLECLICK", SYMTYPE_NUM_I, EVT_FLAG_DOUBLECLICK );
    ADD_SYMBOL( "EVT_FLAGS_NUM", SYMTYPE_NUM_I, EVT_FLAGS_NUM );
    ADD_SYMBOL( "KEY_MOUSE_LEFT", SYMTYPE_NUM_I, MOUSE_BUTTON_LEFT );
    ADD_SYMBOL( "KEY_MOUSE_MIDDLE", SYMTYPE_NUM_I, MOUSE_BUTTON_MIDDLE );
    ADD_SYMBOL( "KEY_MOUSE_RIGHT", SYMTYPE_NUM_I, MOUSE_BUTTON_RIGHT );
    ADD_SYMBOL( "KEY_MOUSE_SCROLLUP", SYMTYPE_NUM_I, MOUSE_BUTTON_SCROLLUP );
    ADD_SYMBOL( "KEY_MOUSE_SCROLLDOWN", SYMTYPE_NUM_I, MOUSE_BUTTON_SCROLLDOWN );
    ADD_SYMBOL( "KEY_BACKSPACE", SYMTYPE_NUM_I, KEY_BACKSPACE );
    ADD_SYMBOL( "KEY_TAB", SYMTYPE_NUM_I, KEY_TAB );
    ADD_SYMBOL( "KEY_ENTER", SYMTYPE_NUM_I, KEY_ENTER );
    ADD_SYMBOL( "KEY_ESCAPE", SYMTYPE_NUM_I, KEY_ESCAPE );
    ADD_SYMBOL( "KEY_SPACE", SYMTYPE_NUM_I, KEY_SPACE );
    ADD_SYMBOL( "KEY_F1", SYMTYPE_NUM_I, KEY_F1 );
    ADD_SYMBOL( "KEY_F2", SYMTYPE_NUM_I, KEY_F2 );
    ADD_SYMBOL( "KEY_F3", SYMTYPE_NUM_I, KEY_F3 );
    ADD_SYMBOL( "KEY_F4", SYMTYPE_NUM_I, KEY_F4 );
    ADD_SYMBOL( "KEY_F5", SYMTYPE_NUM_I, KEY_F5 );
    ADD_SYMBOL( "KEY_F6", SYMTYPE_NUM_I, KEY_F6 );
    ADD_SYMBOL( "KEY_F7", SYMTYPE_NUM_I, KEY_F7 );
    ADD_SYMBOL( "KEY_F8", SYMTYPE_NUM_I, KEY_F8 );
    ADD_SYMBOL( "KEY_F9", SYMTYPE_NUM_I, KEY_F9 );
    ADD_SYMBOL( "KEY_F10", SYMTYPE_NUM_I, KEY_F10 );
    ADD_SYMBOL( "KEY_F11", SYMTYPE_NUM_I, KEY_F11 );
    ADD_SYMBOL( "KEY_F12", SYMTYPE_NUM_I, KEY_F12 );
    ADD_SYMBOL( "KEY_UP", SYMTYPE_NUM_I, KEY_UP );
    ADD_SYMBOL( "KEY_DOWN", SYMTYPE_NUM_I, KEY_DOWN );
    ADD_SYMBOL( "KEY_LEFT", SYMTYPE_NUM_I, KEY_LEFT );
    ADD_SYMBOL( "KEY_RIGHT", SYMTYPE_NUM_I, KEY_RIGHT );
    ADD_SYMBOL( "KEY_INSERT", SYMTYPE_NUM_I, KEY_INSERT );
    ADD_SYMBOL( "KEY_DELETE", SYMTYPE_NUM_I, KEY_DELETE );
    ADD_SYMBOL( "KEY_HOME", SYMTYPE_NUM_I, KEY_HOME );
    ADD_SYMBOL( "KEY_END", SYMTYPE_NUM_I, KEY_END );
    ADD_SYMBOL( "KEY_PAGEUP", SYMTYPE_NUM_I, KEY_PAGEUP );
    ADD_SYMBOL( "KEY_PAGEDOWN", SYMTYPE_NUM_I, KEY_PAGEDOWN );
    ADD_SYMBOL( "KEY_CAPS", SYMTYPE_NUM_I, KEY_CAPS );
    ADD_SYMBOL( "KEY_SHIFT", SYMTYPE_NUM_I, KEY_SHIFT );
    ADD_SYMBOL( "KEY_CTRL", SYMTYPE_NUM_I, KEY_CTRL );
    ADD_SYMBOL( "KEY_ALT", SYMTYPE_NUM_I, KEY_ALT );
    ADD_SYMBOL( "KEY_CMD", SYMTYPE_NUM_I, KEY_CMD );
    ADD_SYMBOL( "KEY_MENU", SYMTYPE_NUM_I, KEY_MENU );
    ADD_SYMBOL( "KEY_UNKNOWN", SYMTYPE_NUM_I, KEY_UNKNOWN );
    ADD_SYMBOL( "QA_NONE", SYMTYPE_NUM_I, 0 );
    ADD_SYMBOL( "QA_CLOSE_VM", SYMTYPE_NUM_I, 1 );
    //Threads:
    ADD_SYMBOL( "THREAD_FLAG_AUTO_DESTROY", SYMTYPE_NUM_I, PIX_THREAD_FLAG_AUTO_DESTROY );
    //Mathematical:
    ADD_SYMBOL( "M_E", SYMTYPE_NUM_F, M_E );
    ADD_SYMBOL( "M_LOG2E", SYMTYPE_NUM_F, M_LOG2E );
    ADD_SYMBOL( "M_LOG10E", SYMTYPE_NUM_F, M_LOG10E );
    ADD_SYMBOL( "M_LN2", SYMTYPE_NUM_F, M_LN2 );
    ADD_SYMBOL( "M_LN10", SYMTYPE_NUM_F, M_LN10 );
    ADD_SYMBOL( "M_PI", SYMTYPE_NUM_F, M_PI );
    ADD_SYMBOL( "M_2_SQRTPI", SYMTYPE_NUM_F, M_2_SQRTPI );
    ADD_SYMBOL( "M_SQRT2", SYMTYPE_NUM_F, M_SQRT2 );
    ADD_SYMBOL( "M_SQRT1_2", SYMTYPE_NUM_F, M_SQRT1_2 );
    //Data processing operations (op_cn):
    ADD_SYMBOL( "OP_MIN", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MIN );
    ADD_SYMBOL( "OP_MAX", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MAX );
    ADD_SYMBOL( "OP_MAXMOD", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MAXABS );
    ADD_SYMBOL( "OP_MAXABS", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MAXABS );
    ADD_SYMBOL( "OP_SUM", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SUM );
    ADD_SYMBOL( "OP_LIMIT_TOP", SYMTYPE_NUM_I, PIX_DATA_OPCODE_LIMIT_TOP );
    ADD_SYMBOL( "OP_LIMIT_BOTTOM", SYMTYPE_NUM_I, PIX_DATA_OPCODE_LIMIT_BOTTOM );
    ADD_SYMBOL( "OP_ABS", SYMTYPE_NUM_I, PIX_DATA_OPCODE_ABS );
    ADD_SYMBOL( "OP_SUB2", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SUB2 );
    ADD_SYMBOL( "OP_COLOR_SUB2", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_SUB2 );
    ADD_SYMBOL( "OP_DIV2", SYMTYPE_NUM_I, PIX_DATA_OPCODE_DIV2 );
    ADD_SYMBOL( "OP_H_INTEGRAL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_H_INTEGRAL );
    ADD_SYMBOL( "OP_V_INTEGRAL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_V_INTEGRAL );
    ADD_SYMBOL( "OP_H_DERIVATIVE", SYMTYPE_NUM_I, PIX_DATA_OPCODE_H_DERIVATIVE );
    ADD_SYMBOL( "OP_V_DERIVATIVE", SYMTYPE_NUM_I, PIX_DATA_OPCODE_V_DERIVATIVE );
    ADD_SYMBOL( "OP_H_FLIP", SYMTYPE_NUM_I, PIX_DATA_OPCODE_H_FLIP );
    ADD_SYMBOL( "OP_V_FLIP", SYMTYPE_NUM_I, PIX_DATA_OPCODE_V_FLIP );
    //Data processing operations (op_cn, op_cc):
    ADD_SYMBOL( "OP_ADD", SYMTYPE_NUM_I, PIX_DATA_OPCODE_ADD );
    ADD_SYMBOL( "OP_SADD", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SADD );
    ADD_SYMBOL( "OP_COLOR_ADD", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_ADD );
    ADD_SYMBOL( "OP_SUB", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SUB );
    ADD_SYMBOL( "OP_SSUB", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SSUB );
    ADD_SYMBOL( "OP_COLOR_SUB", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_SUB );
    ADD_SYMBOL( "OP_MUL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MUL );
    ADD_SYMBOL( "OP_SMUL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SMUL );
    ADD_SYMBOL( "OP_MUL_RSHIFT15", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MUL_RSHIFT15 );
    ADD_SYMBOL( "OP_COLOR_MUL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_MUL );
    ADD_SYMBOL( "OP_DIV", SYMTYPE_NUM_I, PIX_DATA_OPCODE_DIV );
    ADD_SYMBOL( "OP_COLOR_DIV", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COLOR_DIV );
    ADD_SYMBOL( "OP_AND", SYMTYPE_NUM_I, PIX_DATA_OPCODE_AND );
    ADD_SYMBOL( "OP_OR", SYMTYPE_NUM_I, PIX_DATA_OPCODE_OR );
    ADD_SYMBOL( "OP_XOR", SYMTYPE_NUM_I, PIX_DATA_OPCODE_XOR );
    ADD_SYMBOL( "OP_LSHIFT", SYMTYPE_NUM_I, PIX_DATA_OPCODE_LSHIFT );
    ADD_SYMBOL( "OP_RSHIFT", SYMTYPE_NUM_I, PIX_DATA_OPCODE_RSHIFT );
    ADD_SYMBOL( "OP_EQUAL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_EQUAL );
    ADD_SYMBOL( "OP_LESS", SYMTYPE_NUM_I, PIX_DATA_OPCODE_LESS );
    ADD_SYMBOL( "OP_GREATER", SYMTYPE_NUM_I, PIX_DATA_OPCODE_GREATER );
    ADD_SYMBOL( "OP_COPY", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COPY );
    ADD_SYMBOL( "OP_COPY_LESS", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COPY_LESS );
    ADD_SYMBOL( "OP_COPY_GREATER", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COPY_GREATER );
    //Data processing operations (op_cc):
    ADD_SYMBOL( "OP_BMUL", SYMTYPE_NUM_I, PIX_DATA_OPCODE_BMUL );
    ADD_SYMBOL( "OP_EXCHANGE", SYMTYPE_NUM_I, PIX_DATA_OPCODE_EXCHANGE );
    ADD_SYMBOL( "OP_COMPARE", SYMTYPE_NUM_I, PIX_DATA_OPCODE_COMPARE )
    //Data processing operations (op_ccn):
    ADD_SYMBOL( "OP_MUL_DIV", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MUL_DIV );
    ADD_SYMBOL( "OP_MUL_RSHIFT", SYMTYPE_NUM_I, PIX_DATA_OPCODE_MUL_RSHIFT );
    //Data processing operations (generator):
    ADD_SYMBOL( "OP_SIN", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SIN );
    ADD_SYMBOL( "OP_SIN8", SYMTYPE_NUM_I, PIX_DATA_OPCODE_SIN8 );
    ADD_SYMBOL( "OP_RAND", SYMTYPE_NUM_I, PIX_DATA_OPCODE_RAND );
    //Sampler:
    ADD_SYMBOL( "SMP_DEST", SYMTYPE_NUM_I, PIX_SAMPLER_DEST );
    ADD_SYMBOL( "SMP_DEST_OFF", SYMTYPE_NUM_I, PIX_SAMPLER_DEST_OFF );
    ADD_SYMBOL( "SMP_DEST_LEN", SYMTYPE_NUM_I, PIX_SAMPLER_DEST_LEN );
    ADD_SYMBOL( "SMP_SRC", SYMTYPE_NUM_I, PIX_SAMPLER_SRC );
    ADD_SYMBOL( "SMP_SRC_OFF_H", SYMTYPE_NUM_I, PIX_SAMPLER_SRC_OFF_H );
    ADD_SYMBOL( "SMP_SRC_OFF_L", SYMTYPE_NUM_I, PIX_SAMPLER_SRC_OFF_L );
    ADD_SYMBOL( "SMP_SRC_SIZE", SYMTYPE_NUM_I, PIX_SAMPLER_SRC_SIZE );
    ADD_SYMBOL( "SMP_LOOP", SYMTYPE_NUM_I, PIX_SAMPLER_LOOP );
    ADD_SYMBOL( "SMP_LOOP_LEN", SYMTYPE_NUM_I, PIX_SAMPLER_LOOP_LEN );
    ADD_SYMBOL( "SMP_VOL1", SYMTYPE_NUM_I, PIX_SAMPLER_VOL1 );
    ADD_SYMBOL( "SMP_VOL2", SYMTYPE_NUM_I, PIX_SAMPLER_VOL2 );
    ADD_SYMBOL( "SMP_DELTA", SYMTYPE_NUM_I, PIX_SAMPLER_DELTA );
    ADD_SYMBOL( "SMP_FLAGS", SYMTYPE_NUM_I, PIX_SAMPLER_FLAGS );
    ADD_SYMBOL( "SMP_INFO_SIZE", SYMTYPE_NUM_I, PIX_SAMPLER_PARAMETERS );
    ADD_SYMBOL( "SMP_FLAG_INTERP2", SYMTYPE_NUM_I, PIX_SAMPLER_FLAG_INTERP2 );
    ADD_SYMBOL( "SMP_FLAG_INTERP4", SYMTYPE_NUM_I, PIX_SAMPLER_FLAG_INTERP4 );
    ADD_SYMBOL( "SMP_FLAG_PINGPONG", SYMTYPE_NUM_I, PIX_SAMPLER_FLAG_PINGPONG );
    ADD_SYMBOL( "SMP_FLAG_REVERSE", SYMTYPE_NUM_I, PIX_SAMPLER_FLAG_REVERSE );
    //Native code:
    ADD_SYMBOL( "CCONV_DEFAULT", SYMTYPE_NUM_I, PIX_CCONV_DEFAULT );
    ADD_SYMBOL( "CCONV_CDECL", SYMTYPE_NUM_I, PIX_CCONV_CDECL );
    ADD_SYMBOL( "CCONV_STDCALL", SYMTYPE_NUM_I, PIX_CCONV_STDCALL );
    ADD_SYMBOL( "CCONV_UNIX_AMD64", SYMTYPE_NUM_I, PIX_CCONV_UNIX_AMD64 );
    ADD_SYMBOL( "CCONV_WIN64", SYMTYPE_NUM_I, PIX_CCONV_WIN64 );
    //Pixilang info flags (for variable PIXILANG_INFO):
    ADD_SYMBOL( "PIXINFO_MODULE", SYMTYPE_NUM_I, PIX_INFO_MODULE );
    ADD_SYMBOL( "PIXINFO_MULTITOUCH", SYMTYPE_NUM_I, PIX_INFO_MULTITOUCH );
    ADD_SYMBOL( "PIXINFO_TOUCHCONTROL", SYMTYPE_NUM_I, PIX_INFO_TOUCHCONTROL );
    ADD_SYMBOL( "PIXINFO_NOWINDOW", SYMTYPE_NUM_I, PIX_INFO_NOWINDOW );
    ADD_SYMBOL( "PIXINFO_MIDIIN", SYMTYPE_NUM_I, PIX_INFO_MIDIIN );
    ADD_SYMBOL( "PIXINFO_MIDIOUT", SYMTYPE_NUM_I, PIX_INFO_MIDIOUT );
    ADD_SYMBOL( "PIXINFO_MIDIOPTIONS", SYMTYPE_NUM_I, PIX_INFO_MIDIOPTIONS );
    ADD_SYMBOL( "PIXINFO_WEBSERVER", SYMTYPE_NUM_I, PIX_INFO_WEBSERVER );
    ADD_SYMBOL( "PIXINFO_CLIPBOARD", SYMTYPE_NUM_I, PIX_INFO_CLIPBOARD );
    ADD_SYMBOL( "PIXINFO_CLIPBOARD_AV", SYMTYPE_NUM_I, PIX_INFO_CLIPBOARD_AV );
    ADD_SYMBOL( "PIXINFO_GALLERY", SYMTYPE_NUM_I, PIX_INFO_GALLERY );
    ADD_SYMBOL( "PIXINFO_EMAIL", SYMTYPE_NUM_I, PIX_INFO_EMAIL );
    ADD_SYMBOL( "PIXINFO_EXPORT", SYMTYPE_NUM_I, PIX_INFO_EXPORT );
    ADD_SYMBOL( "PIXINFO_EXPORT2", SYMTYPE_NUM_I, PIX_INFO_EXPORT2 );
    ADD_SYMBOL( "PIXINFO_IMPORT", SYMTYPE_NUM_I, PIX_INFO_IMPORT );
    ADD_SYMBOL( "PIXINFO_VIDEOCAPTURE", SYMTYPE_NUM_I, PIX_INFO_VIDEOCAPTURE );
    ADD_SYMBOL( "PIXINFO_OPENGL", SYMTYPE_NUM_I, PIX_INFO_OPENGL );
    //Other constants:
    ADD_SYMBOL( "INT_SIZE", SYMTYPE_NUM_I, sizeof( PIX_INT ) );
    ADD_SYMBOL( "INT_MAX", SYMTYPE_NUM_I, PIX_INT_MAX_POSITIVE );
    ADD_SYMBOL( "FLOAT_SIZE", SYMTYPE_NUM_I, sizeof( PIX_FLOAT ) );
    ADD_SYMBOL( "COLORBITS", SYMTYPE_NUM_I, COLORBITS );
    ADD_SYMBOL( "INT8", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT8 );
    ADD_SYMBOL( "INT16", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT16 );
    ADD_SYMBOL( "INT32", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT32 );
    ADD_SYMBOL( "INT64", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT64 );
    ADD_SYMBOL( "FLOAT32", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_FLOAT32 );
    ADD_SYMBOL( "FLOAT64", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_FLOAT64 );
    if( sizeof( COLOR ) == 1 ) ADD_SYMBOL( "PIXEL", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT8 );
    if( sizeof( COLOR ) == 2 ) ADD_SYMBOL( "PIXEL", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT16 );
    if( sizeof( COLOR ) == 4 ) ADD_SYMBOL( "PIXEL", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT32 );
    if( sizeof( COLOR ) == 8 ) ADD_SYMBOL( "PIXEL", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT64 );
    if( sizeof( PIX_INT ) == 4 ) ADD_SYMBOL( "INT", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT32 );
    if( sizeof( PIX_INT ) == 8 ) ADD_SYMBOL( "INT", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_INT64 );
    if( sizeof( PIX_FLOAT ) == 4 ) ADD_SYMBOL( "FLOAT", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_FLOAT32 );
    if( sizeof( PIX_FLOAT ) == 8 ) ADD_SYMBOL( "FLOAT", SYMTYPE_NUM_I, PIX_CONTAINER_TYPE_FLOAT64 );
    ADD_SYMBOL( "PIXILANG_VERSION", SYMTYPE_NUM_I, PIXILANG_VERSION );
    ADD_SYMBOL( "OS_NAME", SYMTYPE_NUM_I, vm->os_name );
    ADD_SYMBOL( "ARCH_NAME", SYMTYPE_NUM_I, vm->arch_name );
    ADD_SYMBOL( "LANG_NAME", SYMTYPE_NUM_I, vm->lang_name );
    ADD_SYMBOL( "CURRENT_PATH", SYMTYPE_NUM_I, vm->current_path );
    ADD_SYMBOL( "USER_PATH", SYMTYPE_NUM_I, vm->user_path );
    ADD_SYMBOL( "TEMP_PATH", SYMTYPE_NUM_I, vm->temp_path );
#ifdef OPENGL
    ADD_SYMBOL( "OPENGL", SYMTYPE_NUM_I, 1 );
#else
    ADD_SYMBOL( "OPENGL", SYMTYPE_NUM_I, 0 );
#endif
    ADD_SYMBOL( "GL_SCREEN", SYMTYPE_NUM_I, PIX_GL_SCREEN );
    ADD_SYMBOL( "GL_ZBUF", SYMTYPE_NUM_I, PIX_GL_ZBUF );
    //SunVox:
#ifndef PIX_NOSUNVOX
    ADD_SYMBOL( "SV_VERSION", SYMTYPE_NUM_I, SUNVOX_ENGINE_VERSION );
    ADD_SYMBOL( "SV_INIT_FLAG_OFFLINE", SYMTYPE_NUM_I, PIX_SV_INIT_FLAG_OFFLINE );
    ADD_SYMBOL( "SV_INIT_FLAG_ONE_THREAD", SYMTYPE_NUM_I, PIX_SV_INIT_FLAG_ONE_THREAD );
    ADD_SYMBOL( "SV_TIME_MAP_SPEED", SYMTYPE_NUM_I, PIX_SV_TIME_MAP_SPEED );
    ADD_SYMBOL( "SV_TIME_MAP_FRAMECNT", SYMTYPE_NUM_I, PIX_SV_TIME_MAP_FRAMECNT );
    ADD_SYMBOL( "SV_MODULE_FLAG_EXISTS", SYMTYPE_NUM_I, PIX_SV_MODULE_FLAG_EXISTS );
    ADD_SYMBOL( "SV_MODULE_FLAG_GENERATOR", SYMTYPE_NUM_I, PIX_SV_MODULE_FLAG_GENERATOR );
    ADD_SYMBOL( "SV_MODULE_FLAG_EFFECT", SYMTYPE_NUM_I, PIX_SV_MODULE_FLAG_EFFECT );
    ADD_SYMBOL( "SV_MODULE_FLAG_MUTE", SYMTYPE_NUM_I, PIX_SV_MODULE_FLAG_MUTE );
    ADD_SYMBOL( "SV_MODULE_FLAG_SOLO", SYMTYPE_NUM_I, PIX_SV_MODULE_FLAG_SOLO );
    ADD_SYMBOL( "SV_MODULE_FLAG_BYPASS", SYMTYPE_NUM_I, PIX_SV_MODULE_FLAG_BYPASS );
    ADD_SYMBOL( "SV_MODULE_INPUTS_OFF", SYMTYPE_NUM_I, PIX_SV_MODULE_INPUTS_OFF );
    ADD_SYMBOL( "SV_MODULE_INPUTS_MASK", SYMTYPE_NUM_I, PIX_SV_MODULE_INPUTS_MASK );
    ADD_SYMBOL( "SV_MODULE_OUTPUTS_OFF", SYMTYPE_NUM_I, PIX_SV_MODULE_OUTPUTS_OFF );
    ADD_SYMBOL( "SV_MODULE_OUTPUTS_MASK", SYMTYPE_NUM_I, PIX_SV_MODULE_OUTPUTS_MASK );
    ADD_SYMBOL( "NOTECMD_NOTE_OFF", SYMTYPE_NUM_I, NOTECMD_NOTE_OFF );
    ADD_SYMBOL( "NOTECMD_ALL_NOTES_OFF", SYMTYPE_NUM_I, NOTECMD_ALL_NOTES_OFF );
    ADD_SYMBOL( "NOTECMD_CLEAN_SYNTHS", SYMTYPE_NUM_I, NOTECMD_CLEAN_MODULES );
    ADD_SYMBOL( "NOTECMD_STOP", SYMTYPE_NUM_I, NOTECMD_STOP );
    ADD_SYMBOL( "NOTECMD_PLAY", SYMTYPE_NUM_I, NOTECMD_PLAY );
    ADD_SYMBOL( "NOTECMD_SET_PITCH", SYMTYPE_NUM_I, NOTECMD_SET_PITCH );
    ADD_SYMBOL( "NOTECMD_CLEAN_MODULE", SYMTYPE_NUM_I, NOTECMD_CLEAN_MODULE );
#endif

    DPRINT( "Adding global variables...\n" );
    ADD_SYMBOL( "WINDOW_XSIZE", SYMTYPE_GVAR, PIX_GVAR_WINDOW_XSIZE );
    ADD_SYMBOL( "WINDOW_YSIZE", SYMTYPE_GVAR, PIX_GVAR_WINDOW_YSIZE );
#ifdef OPENGL
    ADD_SYMBOL( "WINDOW_ZSIZE", SYMTYPE_NUM_I, GL_ORTHO_MAX_DEPTH * 2 );
#else
    ADD_SYMBOL( "WINDOW_ZSIZE", SYMTYPE_NUM_I, 32768 * 2 );
#endif
    ADD_SYMBOL( "WINDOW_SAFE_AREA_X", SYMTYPE_GVAR, PIX_GVAR_WINDOW_SAFE_AREA_X );
    ADD_SYMBOL( "WINDOW_SAFE_AREA_Y", SYMTYPE_GVAR, PIX_GVAR_WINDOW_SAFE_AREA_Y );
    ADD_SYMBOL( "WINDOW_SAFE_AREA_W", SYMTYPE_GVAR, PIX_GVAR_WINDOW_SAFE_AREA_W );
    ADD_SYMBOL( "WINDOW_SAFE_AREA_H", SYMTYPE_GVAR, PIX_GVAR_WINDOW_SAFE_AREA_H );
    ADD_SYMBOL( "FPS", SYMTYPE_GVAR, PIX_GVAR_FPS );
    ADD_SYMBOL( "PPI", SYMTYPE_GVAR, PIX_GVAR_PPI );
    ADD_SYMBOL( "UI_SCALE", SYMTYPE_GVAR, PIX_GVAR_SCALE );
    ADD_SYMBOL( "UI_FONT_SCALE", SYMTYPE_GVAR, PIX_GVAR_FONT_SCALE );
    ADD_SYMBOL( "PIXILANG_INFO", SYMTYPE_GVAR, PIX_GVAR_PIXILANG_INFO );

    DPRINT( "Compilation: lexical tree generation...\n" );
    pcomp->root = node( lnode_statlist, 0 );
    if( !pcomp->root )
    {
	rv = 7;
	goto compiler_end;
    }
    if( yyparse( pcomp ) )
    {
	rv = 8;
	goto compiler_parse_error;
    }
    //Close last local symbol table:
    pcomp->root = remove_lsym_table( pcomp, pcomp->root );
    if( !pcomp->root )
    {
	rv = 9;
	goto compiler_end;
    }
    DPRINT( "Compilation: optimization...\n" );
    optimize_tree( pcomp->root );
    DPRINT( "Compilation: code generation...\n" );
    DPRINT( "0: HALT\n" );
    pix_vm_put_opcode( OPCODE_HALT, vm );
    vm->halt_addr = 0;
    compile_tree( pcomp, pcomp->root );
    fix_up( pcomp );
    DPRINT( "%d: RET_i ( 0 << OB )\n", (int)pcomp->vm->code_ptr );
    pix_vm_put_opcode( OPCODE_RET_i, vm );

#ifdef PIX_CODE_ANALYZER_ENABLED
    pix_vm_code_analyzer( PIX_CODE_ANALYZER_SHOW_STATS, vm );
#endif

    for( size_t i = 0; i < pcomp->vm->vars_num; i++ )
    {
	uint flags = pcomp->var_flags[ i ];
	if( flags & VAR_FLAG_USED )
	{
	    if( ( pcomp->var_flags[ i ] & VAR_FLAG_INITIALIZED ) == 0 )
	    {
	        slog( "Variable %s is not initialized. Default value = 0.\n", pix_vm_get_variable_name( pcomp->vm, i ) );
	    }
	}
    }

compiler_parse_error:

    DPRINT( "Deinit...\n" );
    while( pcomp->lsym_num >= 0 ) remove_lsym_table( pcomp, NULL );
    remove_tree( pcomp->root );
    pix_symtab_deinit( &pcomp->sym );
    smem_free( pcomp->var_flags );
    smem_free( pcomp->lsym );
    smem_free( pcomp->inc );
    smem_free( pcomp->while_stack );
    smem_free( pcomp->fixup );
    smem_free( pcomp );

compiler_end:

    stime_ms_t end_time = stime_ms();

    DPRINT( "Pixilang compiler finished. %d ms.\n", end_time - start_time );

    return rv;
}

#ifdef PIX_ENCODED_SOURCE
extern void pix_decode_source( void* src, size_t size );
#endif

//Load *.pixicode file or compile *.pixi source file
int pix_load( const char* name, pix_vm* vm )
{
    int rv = 0;

    char* src = NULL;
    char* base_path = NULL;

    size_t fsize = sfs_get_file_size( name );
    if( fsize >= 8 )
    {
	sfs_file f = sfs_open( name, "rb" );
	if( f )
	{
	    char sign[ 9 ];
	    sign[ 8 ] = 0;
	    sfs_read( &sign, 1, 8, f );
	    sfs_close( f );
	    if( smem_strcmp( (const char*)sign, "PIXICODE" ) == 0 )
	    {
		//Binary code:
		base_path = pix_get_base_path( name );
		int load_code_err = pix_vm_load_code( name, base_path, vm );
		if( load_code_err )
		{
		    rv = 5 + load_code_err * 100;
		}
		goto pix_compile_end;
	    }
	}
    }
    if( fsize )
    {
	src = (char*)smem_new( fsize );
	if( !src ) 
	{
	    rv = 1;
	    ERROR( "memory allocation error" );
	    goto pix_compile_end;
	}
	sfs_file f = sfs_open( name, "rb" );
	if( f == 0 )
	{
	    rv = 2;
	    ERROR( "can't open file %s", name );
	    goto pix_compile_end;
	}
	int start_offset = 0;
	if( fsize >= 3 )
	{
	    sfs_read( src, 1, 3, f );
	    if( (uint8_t)src[ 0 ] == 0xEF && (uint8_t)src[ 1 ] == 0xBB && (uint8_t)src[ 2 ] == 0xBF )
	    {
		//Byte order mark found. Skip it:
		start_offset += 3;
	    }
	    else
	    {
		sfs_rewind( f );
	    }
	}
	if( start_offset )
	{
	    fsize -= start_offset;
	    sfs_seek( f, start_offset, SFS_SEEK_SET );
	}
	sfs_read( src, 1, fsize, f );
	sfs_close( f );
	base_path = pix_get_base_path( name );
#ifdef PIX_ENCODED_SOURCE
	pix_decode_source( src, fsize );
#endif
	int comp_err = pix_compile( src, fsize, (char*)name, base_path, vm );
	if( comp_err )
	{
	    rv = 3 + comp_err * 100;
	    goto pix_compile_end;
	}
    }
    else
    {
	ERROR( "%s not found (or it's empty)", name );
	rv = 4;
    }

pix_compile_end:

    smem_free( src );
    smem_free( base_path );

    return rv;
}
