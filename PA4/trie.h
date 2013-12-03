#ifndef TRIE_H_
#define TRIE_H_

/*
 * Author: jrahm
 * created: 2013/12/02
 * trie.h: <description>
 */

#define ITERATE_FUNCTION( func ) ((void(*)(void*,void*))func)

struct trie ;

struct trie_lev2 {
    struct trie* _m_children[16];
};

struct trie {
    struct trie_lev2* _m_children[16];
    void*        _m_values[16];
};

typedef struct trie_lev2 trie_t;

void* trie_get( trie_t* ths, const char* key ) ;

void  trie_put( trie_t* ths, const char* key, void* val );

void  trie_iterate( trie_t* ths, void(*func)(void*arg,void*val), void* arg );

#endif /* TRIE_H_ */
