#include "trie.h"

#include <inttypes.h>
#include <assert.h>
#include <stdlib.h>
#include <memory.h>
#include <stdio.h>

struct value {
    const char* name;
    uint32_t addr;
};

int main( int argc, char ** argv ) {
    trie_t m_trie;
    memset( &m_trie, 0, sizeof( trie_t ) );

    struct value v1 ;
    struct value v2 ;

    v1.name = "f1";
    v2.name = "f2";

    v1.addr = 12;
    v2.addr = 10;

    printf( "-------------- Inserting ----------------\n" );
    trie_insert( &m_trie, v1.name, &v1 );
    printf( "-------------- Inserting ----------------\n" );
    trie_insert( &m_trie, v2.name, &v2 );

    printf( "-------------- Searching ----------------\n" );
    struct value* ret = trie_search( &m_trie, "f1" );
    assert( ret == &v1 );
    printf( "-------------- Searching ----------------\n" );
    ret = trie_search( &m_trie, "f2" );
    assert( ret == &v2 );

    return 0;
}
