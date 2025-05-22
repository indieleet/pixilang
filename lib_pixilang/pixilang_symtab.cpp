/*
    pixilang_symtab.cpp
    This file is part of the Pixilang.
    Copyright (C) 2006 - 2024 Alexander Zolotov <nightradio@gmail.com>
    WarmPlace.ru
*/

#include "sundog.h"
#include "pixilang.h"

int pix_symtab_init( int size_level, pix_symtab* st )
{
    int rv = 0;

    if( !st ) return -1;

    if( (unsigned)size_level >= (unsigned)SSYMTAB_TABSIZE_NUM )
	st->size = size_level;
    else
	st->size = g_ssymtab_tabsize[ size_level ];
    st->symtab = (pix_sym**)smem_new( st->size * sizeof( pix_sym* ) );
    smem_zero( st->symtab );

    return rv;
}

int pix_symtab_hash( const char* name, int size ) //32bit version!
{
    uint32_t h = 0;
    uint8_t* p = (uint8_t*)name;

    for( ; *p != 0; p++ )
	h = 31 * h + *p;

    return h % (unsigned)size;
}

pix_sym* pix_symtab_lookup( const char* name, int hash, bool create, pix_sym_type type, PIX_INT ival, PIX_FLOAT fval, bool* created, pix_symtab* st )
{
    pix_sym* s;

    if( !st ) return NULL;
    if( !st->symtab ) return NULL;

    if( created ) *created = 0;

    if( hash < 0 ) hash = pix_symtab_hash( name, st->size );
    for( s = st->symtab[ hash ]; s != NULL; s = s->next )
	if( smem_strcmp( name, s->name ) == 0 )
	    return s;

    if( create )
    {
	//Create new symbol:
	s = (pix_sym*)smem_new( sizeof( pix_sym ) );
	s->name = smem_strdup( name );
	s->type = type;
	if( type == SYMTYPE_NUM_F )
	    s->val.f = fval;
	else
	    s->val.i = ival;
	s->next = st->symtab[ hash ];
	st->symtab[ hash ] = s;
	if( created ) *created = 1;
    }

    return s;
}

pix_sym* pix_sym_clone( pix_sym* s )
{
    if( !s ) return NULL;
    pix_sym* s2 = (pix_sym*)smem_new( sizeof( pix_sym ) );
    smem_copy( s2, s, sizeof( pix_sym ) );
    s2->name = smem_strdup( s->name );
    if( s->next )
    {
	s2->next = pix_sym_clone( s->next );
    }
    return s2;
}

int pix_symtab_clone( pix_symtab* dest_st, pix_symtab* src_st )
{
    if( !dest_st ) return -1;
    if( !src_st ) return -1;

    if( pix_symtab_init( src_st->size, dest_st ) ) return -1;

    for( int i = 0; i < src_st->size; i++ )
    {
	pix_sym* s = src_st->symtab[ i ];
	if( s )
	{
	    dest_st->symtab[ i ] = pix_sym_clone( s );
	}
    }

    return 0;
}

pix_sym* pix_symtab_get_list( pix_symtab* st )
{
    pix_sym* rv = NULL;
    size_t size = 0;
    if( !st ) return NULL;
    if( !st->symtab ) return NULL;

    for( int i = 0; i < st->size; i++ )
    {
	pix_sym* s = st->symtab[ i ];
	while( s )
	{
	    if( s->name && s->type != SYMTYPE_DELETED )
	    {
		if( size == 0 )
		    rv = (pix_sym*)smem_new( sizeof( pix_sym ) * 8 );
		else
		{
		    if( size >= smem_get_size( rv ) / sizeof( pix_sym ) )
			rv = (pix_sym*)smem_resize( rv, ( size + 8 ) * sizeof( pix_sym ) );
		}
		rv[ size ].name = s->name;
		rv[ size ].type = s->type;
		rv[ size ].val = s->val;
		size++;
	    }
	    s = s->next;
	}
    }

    if( size > 0 )
    {
	rv = (pix_sym*)smem_resize( rv, size * sizeof( pix_sym ) );
    }

    return rv;
}

int pix_symtab_deinit( pix_symtab* st )
{
    int rv = 0;

    if( !st ) return -1;
    if( !st->symtab ) return -1;

    for( int i = 0; i < st->size; i++ )
    {
	pix_sym* s = st->symtab[ i ];
	while( s )
	{
	    pix_sym* next = s->next;
	    smem_free( s->name );
	    smem_free( s );
	    s = next;
	}
    }
    smem_free( st->symtab );
    st->symtab = NULL;

    return rv;
}
