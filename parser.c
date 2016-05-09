#include "parser.h"
#include<stdio.h>
#ifdef DARWIN
  #include "stdlib.h"
#endif
#ifdef NOSTRING
  void memset(char *s, int c, int n) {
    char *se = s + n;
    while(s < se)	*s++ = c;
	}
#else
  #include <string.h>
#endif

#define DEBUG

int dh_memcmp(char *a,char *b,int n) {
  int c = 0;
  while( c < n ) {
    if( *a != *b ) return c+1;
    a++; b++; c++;
  }
  return 0;
}

struct nodec *new_nodecp( struct nodec *newparent ) {
  static int pos = 0;
  int size = sizeof( struct nodec );
  struct nodec *self = (struct nodec *) malloc( size );
  memset( (char *) self, 0, size );
  self->parent      = newparent;
  self->pos = ++pos;
  return self;
}

struct nodec *new_nodec() {
  int size = sizeof( struct nodec );
  struct nodec *self = (struct nodec *) malloc( size );
  memset( (char *) self, 0, size );
  return self;
}

void del_nodec( struct nodec *node ) {
  struct nodec *curnode;
  struct attc *curatt;
  struct nodec *next;
  struct attc *nexta;
  curnode = node->firstchild;
  while( curnode ) {
    next = curnode->next;
    del_nodec( curnode );
    if( !next ) break;
    curnode = next;
  }
  curatt = node->firstatt;
  while( curatt ) {
    nexta = curatt->next;
    free( curatt );
    curatt = nexta;
  }
  free( node );
}

struct attc* new_attc( struct nodec *newparent ) {
  int size = sizeof( struct attc );
  struct attc *self = (struct attc *) malloc( size );
  memset( (char *) self, 0, size );
  self->parent  = newparent;
  return self;
}

//#define DEBUG

#define ST_val_1 54
#define ST_val_x 55

#define ST_outside 99
#define ST_value 1
#define ST_hash 2
#define ST_array 4
#define ST_string 5
#define ST_number_1 6
#define ST_before_dot 7
#define ST_after_day 8
#define ST_e 9
#define ST_e_number 10
#define ST_true 11
#define ST_false 12
#define ST_null 13
#define ST_error 100

int parserc_parse( struct parserc *self, char *xmlin ) {
    // Variables that represent current 'state'
    struct nodec *root    = NULL;
    char  *tagname        = NULL; int    tagname_len    = 0;
    char  *attname        = NULL; int    attname_len    = 0;
    char  *attval         = NULL; int    attval_len     = 0;
    int    att_has_val    = 0;
    struct nodec *curnode = NULL;
    struct attc  *curatt  = NULL;
    int    last_state     = 0;
    self->rootpos = xmlin;
    
    // Variables used temporarily during processing
    struct nodec *temp;
    char   *cpos          = &xmlin[0];
    int    res            = 0;
    int    dent;
    register int let;
    int context = 0; // 0 = hash, 1 = array
    
    if( self->last_state ) {
      #ifdef DEBUG
      printf( "Resuming parse in state %i\n", self->last_state );
      #endif
      self->err = 0;
      root = self->rootnode;
      curnode = self->curnode;
      curatt = self->curatt;
      tagname = self->tagname; tagname_len = self->tagname_len;
      attname = self->attname; attname_len = self->attname_len;
      attval = self->attval; attval_len = self->attval_len;
      att_has_val = self->att_has_val;
      switch( self->last_state ) {
        case ST_outside: goto outside;
        case ST_value: goto value;
      }
    }
    else {
      self->err = 0;
      curnode = root = self->rootnode = new_nodec();
      curnode->context = 0; // hash
    }
    
    #ifdef DEBUG
    printf("Entry to C Parser\n");
    #endif
    
    outside:
      #ifdef DEBUG
      printf( "outside: %c\n", *cpos );
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_outside; goto done;
        case '{': cpos++; goto hash;
      }
      cpos++; // ignore anything else; lolz
      goto outside;
    hash:
      #ifdef DEBUG
      printf( "hash_1: %c\n", *cpos );
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case '"': cpos++; goto key_1;
        case '}':
          cpos++;
          curnode = curnode->parent;
          if( !curnode ) goto done;
          if( curnode->context == 0 ) goto hash;
          if( curnode->context == 1 ) goto array;
      }
      cpos++;
      goto hash;
    array:
      #ifdef DEBUG
      printf( "array: %c\n", *cpos );
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case '"':
          curnode = nodec_addchildr( curnode, "item", 4 );
          cpos++;
          goto string_1;
        case '{':
          // add a hash
          curnode = nodec_addchildr( curnode, "item", 4 );
          curnode->context = 0;
          goto hash;
        case '[':
          // add an array
          curnode = nodec_addchildr( curnode, "item", 4 );
          curnode->context = 1;
          goto array;
        case ']':
          cpos++; 
          curnode = curnode->parent;
          if( !curnode ) goto done;
          if( curnode->context == 0 ) goto hash;
          if( curnode->context == 1 ) goto array;
      }
      if( ( let >= '0' && let <= '9' ) || let == '-' ) { 
        curnode = nodec_addchildr( curnode, "item", 4 );
        cpos++;
        goto number_1;
      }
      cpos++;
      goto array;
    key_1:
      #ifdef DEBUG
      printf("key_1: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case '"':
          cpos++;
          curnode = nodec_addchildr( curnode, tagname, tagname_len );
          goto colon_wait;
      }
      tagname = cpos;
      tagname_len = 1;
      cpos++;
    key_x:
      #ifdef DEBUG
      printf("key_x: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case '"':
          cpos++;
          curnode = nodec_addchildr( curnode, tagname, tagname_len );
          //tagname = "val";
          //tagname_len = 3;
          goto colon_wait;
      }
      tagname_len++;
      cpos++;
      goto key_x;
    colon_wait:
      #ifdef DEBUG
      printf("colon_wait: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case ':': cpos++; goto value;
      }
      cpos++;
      goto colon_wait;
    value:
      #ifdef DEBUG
      printf("value: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case '{':
          cpos++;
          curnode->context = 0;
          goto hash;
        case '[':
          cpos++;
          curnode->context = 1;
          goto array;
        case '"': 
          cpos++;
          goto string_1;
      }
      if( ( let >= '0' && let <= '9' ) || let == '-' ) goto number_1;
      cpos++;
      goto value;
    number_1:
      #ifdef DEBUG
      printf("number: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case ',':
          curnode = curnode->parent;
          if( curnode->context == 0 ) goto hash_comma_wait;
          if( curnode->context == 1 ) goto array_comma_wait;
      }
      curnode->value = cpos;
      curnode->vallen = 1;
      cpos++;
    number_x:
      #ifdef DEBUG
      printf("number: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case ',':
          curnode = curnode->parent;
          if( curnode->context == 0 ) goto hash_comma_wait;
          if( curnode->context == 1 ) goto array_comma_wait;
      }
      curnode->vallen++;
      cpos++;
      goto number_x;
    string_1:
      #ifdef DEBUG
      printf("string_1: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case '"':
          curnode = curnode->parent;
          if( curnode->context == 0 ) goto hash_comma_wait;
          if( curnode->context == 1 ) goto array_comma_wait;
      }
      curnode->value = cpos;
      curnode->vallen = 1;
      cpos++;
    string_x:
      #ifdef DEBUG
      printf("string_x: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case '"':
          curnode = curnode->parent;
          if( curnode->context == 0 ) goto hash_comma_wait;
          if( curnode->context == 1 ) goto array_comma_wait;
      }
      curnode->vallen++;
      cpos++;
      goto string_x;
    hash_comma_wait:
      #ifdef DEBUG
      printf("hash_comma_wait: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case ',': cpos++; goto hash;
      }
      goto hash_comma_wait;
    array_comma_wait:
      #ifdef DEBUG
      printf("array_comma_wait: %c\n", *cpos);
      #endif
      let = *cpos;
      switch( let ) {
        case 0: last_state = ST_val_1; goto done;
        case ',': cpos++; goto array;
      }
      goto array_comma_wait;
    error:
      self->err = - ( int ) ( cpos - &xmlin[0] );
      return self->err;
    done:
      #ifdef DEBUG
      printf("done\n", *cpos);
      #endif
      
      // store the current state of the parser
      self->last_state = last_state;
      self->curnode = curnode;
      self->curatt = curatt;
      self->tagname = tagname; self->tagname_len = tagname_len;
      self->attname = attname; self->attname_len = attname_len;
      self->attval  = attval;  self->attval_len  = attval_len;
      self->att_has_val = att_has_val;
      
      #ifdef DEBUG
      printf("returning\n", *cpos);
      #endif
      return 0;//no error
}

struct utfchar {
  char high;
  char low;
};

struct nodec *nodec_addchildr(  struct nodec *self, char *newname, int newnamelen ) {
  struct nodec *newnode = new_nodecp( self );
  newnode->name    = newname;
  newnode->namelen = newnamelen;
  if( self->numchildren == 0 ) {
    self->firstchild = newnode;
    self->lastchild  = newnode;
    self->numchildren++;
    return newnode;
  }
  else {
    self->lastchild->next = newnode;
    self->lastchild = newnode;
    self->numchildren++;
    return newnode;
  }
}

struct attc *nodec_addattr( struct nodec *self, char *newname, int newnamelen ) {
  struct attc *newatt = new_attc( self );
  newatt->name    = newname;
  newatt->namelen = newnamelen;
  
  if( !self->numatt ) {
    self->firstatt = newatt;
    self->lastatt  = newatt;
    self->numatt++;
    return newatt;
  }
  else {
    self->lastatt->next = newatt;
    self->lastatt = newatt;
    self->numatt++;
    return newatt;
  }
}
