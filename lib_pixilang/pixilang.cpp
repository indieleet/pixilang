/*
    pixilang.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"

int pix_global_init( void )
{
    int rv = 0;
    
    if( NUMBER_OF_OPCODES > ( 1 << PIX_OPCODE_BITS ) )
    {
        slog( "ERROR: NUMBER_OF_OPCODES (%d) > ( 1 << PIX_OPCODE_BITS )\n", NUMBER_OF_OPCODES );
        return -1;
    }
    if( FN_NUM > ( 1 << PIX_FN_BITS ) )
    {
        slog( "ERROR: FN_NUM (%d) > ( 1 << PIX_FN_BITS )\n", FN_NUM );
        return -2;
    }
    if( PIX_OPCODE_BITS + PIX_FN_BITS > sizeof( PIX_OPCODE ) * 8 )
    {
	slog( "ERROR: PIX_OPCODE_BITS (%d) + PIX_FN_BITS (%d)  > PIX_OPCODE SIZE (%d)\n", PIX_OPCODE_BITS, PIX_FN_BITS, sizeof( PIX_OPCODE ) * 8 );
	return -3;
    }
    if( sizeof( PIX_OPCODE ) * 8 - ( PIX_OPCODE_BITS + PIX_FN_BITS ) < 5 )
    {
	slog( "ERROR: Not enough bits (%d) for the number of builtin function parameters\n", sizeof( PIX_OPCODE ) * 8 - ( PIX_OPCODE_BITS + PIX_FN_BITS ) );
	return -4;
    }
    
    return rv;
}

int pix_global_deinit( void )
{
    int rv = 0;

    return rv;
}
