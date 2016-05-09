#ifndef __JSON_BARE_H
#define __JSON_BARE_H
#include"parser.h"
#include<stdint.h>
#include"sh_page_manager.h"

char *rootpos;

#define DEBUG
#define newmalloc( type ) ( type * ) malloc( sizeof( type ) )

#define XNODE_HASH 1 // a hash
#define XNODE_ARR  2 // an array of nodes
#define XNODE_TYPED_ARR 3 // an array of things of one type
#define XNODE_STR  4
#define XNODE_STRZ 5
#define XNODE_U8   6
#define XNODE_U16  7
#define XNODE_U32  8

struct jsonnode_s {
  uint8_t type;
  void *ptr;
};
typedef struct jsonnode_s jsonnode;

struct jsonnode_array_node_s {
  jsonnode *node;
  struct jsonnode_array_node_s *next;
};
typedef struct jsonnode_array_node_s jsonnode_array_node;

struct jsonnode_array_s {
  uint8_t type;
  uint16_t count;
  jsonnode_array_node *root;
};
typedef struct jsonnode_array_s jsonnode_array;

#define jb( a, ... ) jsonbare__ ## a( __VA_ARGS__ )
#define jba( a, ... ) jsonbare_array__ ## a( __VA_ARGS__ )
#define jbp( a, ... ) jsonbare_parser__ ## a( __VA_ARGS__ )

sh_page_manager  *jsonbare__new_pageman();
sh_hash          *jsonbare__parse(    sh_page_manager *man, char *jsonsrc );
sh_hash          *jsonbare__get_hash( sh_page_manager *man, sh_hash *hash, char *str, u2 strlen );
jsonnode_array    *jsonbare__get_arr(  sh_page_manager *man, sh_hash *hash, char *str, u2 strlen );
string_with_len  *jsonbare__get_val(  sh_page_manager *man, sh_hash *hash );

jsonnode_array    *jsonbare_array__new();
void              jsonbare_array__push( jsonnode_array *array, jsonnode *item );

struct parserc   *jsonbare_parser__new();
jsonnode          *jsonbare_parser__json2obj(        struct parserc *parser, sh_page_manager *man, struct nodec *curnode );
int               jsonbare_parser__parse_more(     struct parserc *parser, char *text );
int               jsonbare_parser__parse(          struct parserc *parser, char *text );
int               jsonbare_parser__parse_unsafely( struct parserc *parser, char *text );
int               jsonbare_parser__parse_file(     struct parserc *parser, char *filename );
void              jsonbare_parser__del(            struct parserc *parser );
#endif