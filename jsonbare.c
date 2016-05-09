// JEdit mode Line -> :folding=indent:mode=c++:indentSize=2:noTabs=true:tabSize=2:
#include"parser.h"
#include"jsonbare.h"
#include<stdlib.h>
#include<stdio.h>
#include<string.h>
#include"shared_hashing.h"

typedef sh_page_manager pageman;
typedef sh_bighash bighash;
typedef sh_hash hash;
#define pageman( a, ... ) sh_page_manager__ ## a( __VA_ARGS__ )

//#define DEBUG

sh_page_manager  *jsonbare__new_pageman() {
  return sh_page_manager__new();
}

sh_hash *jsonbare__parse( sh_page_manager *man, char *jsonsrc ) {
  struct parserc *parser = jbp( new );
  jbp( parse, parser, jsonsrc );
  jsonnode *base = jbp( json2obj, parser, man, parser->rootnode );
  sh_hash *basehash = ( sh_hash * ) base->ptr;
  return basehash;
}

sh_hash *jsonbare__get_hash( sh_page_manager *man, sh_hash *hash, char *str, u2 strlen ) {
   jsonnode **nodeptr = (jsonnode **) pageman( fetch, man, hash, str, strlen );
   if( !nodeptr ) return NULL;
   jsonnode *node = *nodeptr;
   if( node->type != XNODE_HASH ) return NULL; 
   return ( sh_hash * ) node->ptr;
}

jsonnode_array *jsonbare__get_arr( sh_page_manager *man, sh_hash *hash, char *str, u2 strlen ) {
   jsonnode **nodeptr = (jsonnode **) pageman( fetch, man, hash, str, strlen );
   if( !nodeptr ) return NULL;
   jsonnode *node = *nodeptr;
   if( node->type != XNODE_ARR ) return NULL; 
   return ( jsonnode_array * ) node->ptr;
}

string_with_len * jsonbare__get_val( sh_page_manager *man, sh_hash *hash ) {
    string_with_len **nodeptr = (string_with_len **) pageman( fetch, man, hash, "_value", 6 );
    if( !nodeptr ) return NULL;
    string_with_len *node = *nodeptr;
    return node;
}

jsonnode_array *jsonbare_array__new() {
  jsonnode_array *array = newmalloc( jsonnode_array );
  array->type = XNODE_ARR;
  array->count = 0;
  array->root = NULL;
  
  #ifdef DEBUG
  printf("Created array %p\n", array );
  #endif
  
  return array;
}

void jsonbare_array__push( jsonnode_array *array, jsonnode *item ) {
  jsonnode_array_node *node = newmalloc( jsonnode_array_node );
  node->node = item;
  if( !array->count ) {
    node->next = NULL;
  }
  else {
    node->next = array->root;
  }
  array->root = node;// array root is always an array_node
  array->count++;
  #ifdef DEBUG
  printf("Created array node %p pointing to %p\n", node, item );
  #endif
}

jsonnode *jsonbare_parser__json2obj( struct parserc *parser, sh_page_manager *man, struct nodec *curnode ) {
  hash *output = pageman( new_hash, man ); // the root
  int i; // loop index; defined at the top because this is C
  struct attc *curatt; // current attribute being worked with
  int numatts = curnode->numatt; // total number of attributes on the current node
  int cur_type;
  int length = curnode->numchildren;
  
  pageman( store_u2, man, output, "_pos", 4, curnode->pos );
  pageman( store_u2, man, output, "_i"  , 2, curnode->name - rootpos );
  pageman( store_u2, man, output, "_z"  , 2, curnode->z );
  
  #ifdef DEBUG
  printf("Node: %.*s\n", curnode->namelen, curnode->name );
  #endif
  
  // node without children
  if( !length ) {
    if( curnode->vallen ) {
      pageman( store_str, man, output, "_value", 6, curnode->value, curnode->vallen );
      if( curnode->type ) pageman( store_u1, man, output, "_cdata", 6, 1 );
    }
    if( curnode->comlen ) pageman( store_str, man, output, "_comment", 8, curnode->comment, curnode->comlen );
  }
  
  // node with children
  else {
    if( curnode->vallen ) {
      pageman( store_str, man, output, "_value", 6, curnode->value, curnode->vallen );
      if( curnode->type ) pageman( store_u1, man, output, "_cdata", 6, 1 );
    }
    if( curnode->comlen ) pageman( store_str, man, output, "_comment", 8, curnode->comment, curnode->comlen );
    
    // loop through child nodes
    curnode = curnode->firstchild;
    for( i = 0; i < length; i++ ) {
      jsonnode **cur = (jsonnode **) pageman( fetch, man, output, curnode->name, curnode->namelen );
      
      // check for multi_[name] nodes
      if( curnode->namelen > 6 ) {
        if( !strncmp( curnode->name, "multi_", 6 ) ) {
          char *subname = &curnode->name[6];
          int subnamelen = curnode->namelen-6;
          jsonnode **old = (jsonnode **) pageman( fetch, man, output, subname, subnamelen );
          jsonnode_array *newarray = jba( new );
          if( !old ) pageman( store, man, output, subname, subnamelen, (void *) newarray );
          else if( (*old)->type == XNODE_HASH ) { // check for hash ref
            //pageman( del, man, output, subname, subnamelen );
            pageman( store, man, output, subname, subnamelen, (void *) newarray );
            jba( push, newarray, *old );
          }
        }
      }
      
      if( !cur ) {
        jsonnode *ob = jbp( json2obj, parser, man, curnode );
        #ifdef DEBUG
        printf("Creating a node named %.*s under hash %p, storing %p\n", curnode->namelen, curnode->name, output, ob );
        #endif
        pageman( store, man, output, curnode->name, curnode->namelen, ob );
      }
      else { // there is already a node stored with this name
        cur_type = (*cur)->type;
        if( cur_type == XNODE_HASH ) { // sub value is a hash; must be anode
          jsonnode_array *newarray = jba( new );// this is a raw array
          #ifdef DEBUG
          printf("**** Created new array %p\n", newarray );
          #endif
          
          jsonnode *arr_container = (jsonnode *) malloc( sizeof( jsonnode ) );
          arr_container->type = XNODE_ARR;
          arr_container->ptr = newarray;
          jsonnode *nodeptr = *cur;
          
          #ifdef DEBUG
          printf("**** C-Pushing %p into array, contents=%p\n", nodeptr, nodeptr->ptr );
          #endif
          pageman( del, man, output, curnode->name, curnode->namelen ); // we need to do the delete otherwise we will get a conflict
          pageman( store, man, output, curnode->name, curnode->namelen, (void *) arr_container );
          jba( push, newarray, nodeptr );
          #ifdef DEBUG
          // This cur is a jsonnode of type XNODE_HASH, and ->ptr on it points to an output hash of the first item
          printf("**** A-Pushing %p into array, contents=%p\n", nodeptr, nodeptr->ptr );
          #endif
          jsonnode *ob = jbp( json2obj, parser, man, curnode );
          jba( push, newarray, ob );
          #ifdef DEBUG
          printf("**** B-Pushing %p into array, contents=%p\n", ob, ob->ptr );
          #endif
        }
        else if( cur_type == XNODE_ARR ) {
          jsonnode *ob = jbp( json2obj, parser, man, curnode );
          jba( push, ( jsonnode_array *) cur, ob );
        }
        else {
          // something else; probably an existing value node; just wipe it out
          jsonnode *ob = jbp( json2obj, parser, man, curnode );
          pageman( store, man, output, curnode->name, curnode->namelen, ob );
        }
      }
      if( i != ( length - 1 ) ) curnode = curnode->next;
    }
    
    curnode = curnode->parent;
  }
  
  if( numatts ) {
    curatt = curnode->firstatt;
    for( i = 0; i < numatts; i++ ) {
      hash *atth = pageman( new_hash, man );
      pageman( store, man, output, curatt->name, curatt->namelen, atth );
      
      char *attval;
      uint16_t attlen;
      if( curatt->value == -1 ) { attval = "1";           attlen = 1;              }
      else                      { attval = curatt->value; attlen = curatt->vallen; }
      pageman( store_str, man, atth, "_value", 6, attval, attlen );
      pageman( store_u1, man, atth, "_att", 4, 1 );
      if( i != ( numatts - 1 ) ) curatt = curatt->next;
    }
  }
  
  jsonnode *ret = (jsonnode *) malloc( sizeof( jsonnode ) );
  ret->type = XNODE_HASH;
  ret->ptr = output;
  return ret;
}

//jsonnode *json2obj( struct parserc *parser ) {
//  if( parser->err ) return 0;//parser->err;
//  else return cjson2obj( parser, parser->rootnode );
//}
    
int jsonbare_parser__parse_more( struct parserc *parser, char *text ) {
  int err = parserc_parse( parser, text );
  return err;
}

struct parserc *jsonbare_parser__new() {
  struct parserc *parser = (struct parserc *) malloc( sizeof( struct parserc ) );
  return parser;
}

int jsonbare_parser__parse( struct parserc *parser, char *text ) {
  parser->last_state = 0;
  int err = parserc_parse( parser, text );
  return err;
}

int jsonbare_parser__parse_unsafely( struct parserc *parser, char *text ) {
  parser->last_state = 0;
  int err = parserc_parse_unsafely( parser, text );
  return err;
}

int jsonbare_parser__parse_file( struct parserc *parser, char *filename ) {
  char *data;
  unsigned long len;
  FILE *handle;
  
  handle = fopen(filename,"r");
  
  fseek( handle, 0, SEEK_END );
  
  len = ftell( handle );
  
  fseek( handle, 0, SEEK_SET );
  data = (char *) malloc( len );
  rootpos = data;
  fread( data, 1, len, handle );
  fclose( handle );
  parser->last_state = 0;
  int err = parserc_parse( parser, data );
  return err;
}

void jsonbare_parser__del( struct parserc *parser ) {
  struct nodec *rootnode = parser->rootnode;
  del_nodec( rootnode ); // note this frees the pointer as well
  free( parser );
}