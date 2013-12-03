#ifndef TRIE_H_
#define TRIE_H_

/*
 * Author: jrahm
 * created: 2013/12/02
 * trie.h: <description>
 */

struct trie ;

struct trie_lev2 {
    struct trie* _m_children[16];
};

struct trie {
    struct trie_lev2* _m_children[16];
    void*        _m_values[16];
};

struct trie_head {
    struct trie* _m_children[16];
};

typedef struct trie_head trie_t;

void* trie_search( trie_t* ths, const char* key ) ;

void  trie_insert( trie_t* ths, const char* key, void* val );

#endif /* TRIE_H_ */
