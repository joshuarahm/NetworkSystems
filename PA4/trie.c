#include "trie.h"

#include "trie.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

void* trie_loose_search( struct trie* tree, const char* key ) {
    if ( tree == NULL ) {
        return NULL;
    }

    char ch = * key ;
    int index = ch & 0x0F ;
    printf( "Index: %d\n", index );

    if ( * ( key + 1 ) == 0 ) {
        return tree->_m_values[ index ] ;
    } else {
        struct trie_lev2* next = tree->_m_children[ index ] ;

        if ( next == NULL ) {
            return NULL ;
        }
        
        ch = * ( key + 1 ) ;
        index = ch >> 4 ;
        printf( "Index: %d\n", index );

        return trie_loose_search( next->_m_children[index], key + 1 ) ;
    }
}

void* trie_search( trie_t* ths, const char* key ) {
    char ch = * key ;
    int index = (ch & 0xF0) >> 4 ;
    struct trie* next = ths->_m_children[ index ];

    return trie_loose_search( next, key );
}

void trie_loose_insert( struct trie* tree, const char* key, void* val ) {
    assert( tree );

    char ch = * key ;
    int index = ch & 0x0F ;
    printf( "Index: %d\n", index );

    if ( * ( key + 1 ) == 0 ) {
        tree->_m_values[ index ] = val ; 
    } else {
        struct trie_lev2** next = &tree->_m_children[ index ] ;

        if( * next == NULL ) {
            * next = calloc( sizeof( struct trie_lev2 ), 1 ) ;
        }

        ch = * ( key + 1 ) ;
        index = ch >> 4 ;
        printf( "Index: %d\n", index );

        struct trie** nextnext = &(*next)->_m_children[ index ] ;
        if( *nextnext == NULL ) {
            *nextnext = calloc( sizeof( struct trie), 1 ) ;
        }

        trie_loose_insert( *nextnext, key + 1, val ) ;
    }
}

void trie_insert( trie_t* ths, const char* key, void* val ) {
    char ch = * key;
    int index = (ch & 0xF0) >> 4 ;
    struct trie** next = &ths->_m_children[ index ] ;   

    if ( ! *next ) {
        *next = calloc( sizeof( struct trie ), 1 ) ;
    }

    trie_loose_insert( * next, key, val ) ;
}
