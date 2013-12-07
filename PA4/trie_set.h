#ifndef TRIE_SET_H_
#define TRIE_SET_H_

/*
 * Author: jrahm
 * created: 2013/12/04
 * trie_set.h: <description>
 */

#include "trie.h"

typedef trie_t trie_set_t ;

int trie_set_contains( const trie_set_t* set, const char* key ) ;

void trie_set_insert( trie_set_t* set, const char* key ) ;

void trie_set_remove( trie_set_t* set, const char* key ) ;

#endif /* TRIE_SET_H_ */
