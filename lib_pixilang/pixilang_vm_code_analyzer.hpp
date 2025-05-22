/*
    pixilang_vm_code_analyzer.hpp
    This file is part of the Pixilang.
    Copyright (C) 2014 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#define LOAD_INT \
{ \
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_INT ) ) \
    { \
	pc++; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 2 ) \
    { \
	pc += 2; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 4 ) \
    { \
	pc += 4; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 8 ) \
    { \
	pc += 8; \
    } \
}
#define LOAD_FLOAT \
{ \
    if( sizeof( PIX_OPCODE ) >= sizeof( PIX_INT ) ) \
    { \
	pc++; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 2 ) \
    { \
	pc += 2; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 4 ) \
    { \
	pc += 4; \
    } \
    if( sizeof( PIX_INT ) / sizeof( PIX_OPCODE ) == 8 ) \
    { \
	pc += 8; \
    } \
}

#define PIX_STAT_BLOCK_SIZE 8

int pix_vm_code_analyzer( uint flags, pix_vm* vm )
{
    int rv = 0;
    
    PIX_OPCODE* code = vm->code;
    
    int stat_block_ptr = 0;
    char* stat_block[ PIX_STAT_BLOCK_SIZE ];
    char* stat_item = 0;
    pix_symtab stat;
    if( flags & PIX_CODE_ANALYZER_SHOW_STATS )
    {
	for( int i = 0; i < PIX_STAT_BLOCK_SIZE; i++ )
	{
	    stat_block[ i ] = (char*)smem_new( 64 );
	    smem_zero( stat_block[ i ] );
        }
	stat_item = (char*)smem_new( PIX_STAT_BLOCK_SIZE * 64 );
	smem_zero( stat_item );
	pix_symtab_init( 13, &stat );
    }
    
    for( size_t pc = 0; pc < vm->code_size; )
    {
	size_t start_pc = pc;
	PIX_OPCODE c = code[ pc ];
	pc++;
	const char* opcode_name = 0;	
	switch( (pix_vm_opcode)( c & PIX_OPCODE_MASK ) )
	{
	    case OPCODE_NOP:
		opcode_name = "NOP";
	        break;
		
    	    case OPCODE_HALT:
	        opcode_name = "HALT";
	        break;
		
	    case OPCODE_PUSH_I:
	        opcode_name = "PUSH_I";
		LOAD_INT;
		break;
	    case OPCODE_PUSH_i:
	        opcode_name = "PUSH_i";
		break;
    	    case OPCODE_PUSH_F:
	        opcode_name = "PUSH_F";
    		LOAD_FLOAT;
		break;
	    case OPCODE_PUSH_v:
	        opcode_name = "PUSH_v";
		break;
		    
	    case OPCODE_GO:
	        opcode_name = "GO";
		break;
	    case OPCODE_JMP_i:
	        opcode_name = "JMP_i";
		break;
	    case OPCODE_JMP_IF_FALSE_i:
	        opcode_name = "JMP_IF_FALSE_i";
		break;
		    
	    case OPCODE_SAVE_TO_VAR_v:
	        opcode_name = "SAVE_TO_VAR_v";
		break;

	    case OPCODE_SAVE_TO_PROP_I:
	        opcode_name = "SAVE_TO_PROP_I";
		LOAD_INT;
		break;
	    case OPCODE_LOAD_FROM_PROP_I:
	        opcode_name = "LOAD_FROM_PROP_I";
		LOAD_INT;
		break;
		    
	    case OPCODE_SAVE_TO_MEM:
	        opcode_name = "SAVE_TO_MEM";
		break;
	    case OPCODE_SAVE_TO_SMEM_2D:
	        opcode_name = "SAVE_TO_SMEM_2D";
		break;
	    case OPCODE_LOAD_FROM_MEM:
	        opcode_name = "LOAD_FROM_MEM";
		break;
	    case OPCODE_LOAD_FROM_SMEM_2D:
	        opcode_name = "LOAD_FROM_SMEM_2D";
		break;
		
	    case OPCODE_SAVE_TO_STACKFRAME_i:
	        opcode_name = "SAVE_TO_STACKFRAME_i";
		break;
    	    case OPCODE_LOAD_FROM_STACKFRAME_i:
	        opcode_name = "LOAD_FROM_STACKFRAME_i";
		break;
		
	    case OPCODE_SUB:
		opcode_name = "SUB";
		break;
	    case OPCODE_ADD:
		opcode_name = "ADD";
		break;
	    case OPCODE_MUL:
		opcode_name = "MUL";
		break;
    	    case OPCODE_IDIV:
		opcode_name = "IDIV";
		break;
	    case OPCODE_DIV:
		opcode_name = "DIV";
		break;
	    case OPCODE_MOD:
		opcode_name = "MOD";
		break;
	    case OPCODE_AND:
		opcode_name = "AND";
		break;
	    case OPCODE_OR:
		opcode_name = "OR";
		break;
	    case OPCODE_XOR:
		opcode_name = "XOR";
		break;
	    case OPCODE_ANDAND:
		opcode_name = "ANDAND";
		break;
	    case OPCODE_OROR:
		opcode_name = "OROR";
		break;
	    case OPCODE_EQ:
		opcode_name = "EQ";
		break;
	    case OPCODE_NEQ:
		opcode_name = "NEQ";
		break;
	    case OPCODE_LESS:
		opcode_name = "LESS";
		break;
	    case OPCODE_LEQ:
		opcode_name = "LEQ";
		break;
	    case OPCODE_GREATER:
		opcode_name = "GREATER";
		break;
	    case OPCODE_GEQ:
		opcode_name = "GEQ";
		break;
    	    case OPCODE_LSHIFT:
		opcode_name = "LSHIFT";
		break;
    	    case OPCODE_RSHIFT:
		opcode_name = "RSHIFT";
		break;

	    case OPCODE_NEG:
		opcode_name = "NEG";
		break;
	    case OPCODE_LOGICAL_NOT:
		opcode_name = "LOGICAL_NOT";
		break;
	    case OPCODE_BITWISE_NOT:
		opcode_name = "BITWISE_NOT";
		break;

	    case OPCODE_CALL_BUILTIN_FN:
		opcode_name = "CALL_BUILTIN_FN";
		break;
    	    case OPCODE_CALL_BUILTIN_FN_VOID:
		opcode_name = "CALL_BUILTIN_FN_VOID";
		break;
	    case OPCODE_CALL_i:
		opcode_name = "CALL_i";
		break;
	    case OPCODE_INC_SP_i:
		opcode_name = "INC_SP_i";
		break;
	    case OPCODE_RET_i:
		opcode_name = "RET_i";
		break;
    	    case OPCODE_RET_I:
		opcode_name = "RET_I";
		LOAD_INT;
		break;
	    case OPCODE_RET:
		opcode_name = "RET";
		break;
	}	
	bool print = 0;
	if( flags & PIX_CODE_ANALYZER_SHOW_ADDRESS )
	{
	    slog( "%d: ", (int)start_pc );
	    print = 1;
	}
	if( flags & PIX_CODE_ANALYZER_SHOW_OPCODES )
	{
	    slog( "%s ", opcode_name );
	    print = 1;
	}
	if( print )
	{
	    slog( "\n" );
	}
	if( flags & PIX_CODE_ANALYZER_SHOW_STATS )
	{
	    char* str = stat_block[ stat_block_ptr & ( PIX_STAT_BLOCK_SIZE - 1 ) ];
	    stat_block_ptr++;
	    sprintf( str, "%s", opcode_name );
	    stat_item[ 0 ] = 0;
	    for( int i = 0; i < PIX_STAT_BLOCK_SIZE; i++ )
	    {
		str = stat_block[ ( stat_block_ptr + i ) & ( PIX_STAT_BLOCK_SIZE - 1 ) ];
		if( str )
		{
		    smem_strcat_resize( stat_item, str );
		    smem_strcat_resize( stat_item, "  " );
		}
		if( stat_item[ 0 ] != 0 )
		{
		    bool created = 0;
		    pix_sym* stat_sym = pix_symtab_lookup( stat_item, -1, 1, SYMTYPE_NUM_I, 1, 0, &created, &stat );
		    if( stat_sym && !created )
		    {
			stat_sym->val.i++;
		    }
		}
	    }
	}
    }

    if( flags & PIX_CODE_ANALYZER_SHOW_STATS )
    {
	slog( "Pixilang code statistics:\n" );
	pix_sym* stat_items = pix_symtab_get_list( &stat );
	if( stat_items )
	{
	    size_t items_num = smem_get_size( stat_items ) / sizeof( pix_sym );
	    slog( "Total items: %d\n", (int)items_num );
	    bool sort_again = 1;
	    while( sort_again )
	    {
		sort_again = 0;
		for( size_t i = 0; i < items_num - 1; i++ )
		{
		    if( stat_items[ i ].val.i < stat_items[ i + 1 ].val.i )
		    {
			pix_sym temp = stat_items[ i ];
			stat_items[ i ] = stat_items[ i + 1 ];
			stat_items[ i + 1 ] = temp;
			sort_again = 1;
		    }
		}
	    }
	    for( size_t i = 0; i < items_num; i++ )
	    {
		slog( "%d: %s\n", stat_items[ i ].val.i, stat_items[ i ].name );
		if( stat_items[ i ].val.i < 100 ) break;
	    }
	    smem_free( stat_items );
	}
	for( int i = 0; i < PIX_STAT_BLOCK_SIZE; i++ )
	{
	    smem_free( stat_block[ i ] );
	}
	smem_free( stat_item );
	pix_symtab_deinit( &stat );
    }
    
    return rv;
}
