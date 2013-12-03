#include "trie.h"

#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>
#include <time.h>

struct value {
    char* name;
    uint32_t addr;
} ;

struct value_list {
    int i;
    struct value* vals[128];
} ;

void func( struct value_list* arg, struct value* val ) {
    printf( "name: %s, addr: %d\n", val->name, val->addr ) ;
    arg->vals[ arg->i ++ ] = val;
}

void rand_key( char * key, size_t len ) {
    int i;
    for( i = 0 ; i < len - 1 ; ++ i ) {
        key[i] = (char) (rand() % ( 'z' - 'A' ) + 'A');
    }
    key[i] = 0;
}

void fill_trie( trie_t* ths ) {
    int i;
    char *key;
    struct value* val;
    for ( i = 0 ; i < 30 ; ++ i ) {
        key = malloc( 10 ) ;
        val = calloc( sizeof( struct value ), 1 );
        rand_key( key, 10 );

        val->name = key ;
        val->addr = rand();
        trie_put( ths, key, val );
    }
}

int main( int argc, char ** argv ) {
    int i;
    trie_t m_trie;
    memset( &m_trie, 0, sizeof( trie_t ) );
    srand( time(NULL) );

    struct value v1 ;
    struct value v2 ;

    v1.name = "f1";
    v2.name = "f2";

    v1.addr = 12;
    v2.addr = 10;

    printf( "-------------- Inserting ----------------\n" );
    trie_put( &m_trie, v1.name, &v1 );
    printf( "-------------- Inserting ----------------\n" );
    trie_put( &m_trie, v2.name, &v2 );

    printf( "-------------- Searching ----------------\n" );
    struct value* ret = trie_get( &m_trie, "f1" );
    assert( ret == &v1 );
    printf( "-------------- Searching ----------------\n" );
    ret = trie_get( &m_trie, "f2" );
    assert( ret == &v2 );

    struct value_list list;
    memset( &list, 0, sizeof( list ) );

    fill_trie( &m_trie ) ;
    trie_iterate( &m_trie, ITERATE_FUNCTION(func), &list ) ;

    for( i = 0 ; i < list.i ; ++ i ) {
        ret = trie_get( &m_trie, list.vals[i]->name ) ;
        assert( list.vals[i] == ret );
    }

    trie_iterate( &m_trie, ITERATE_FUNCTION(func), &list ) ;
    return 0;
}
