#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct tnode
{
  int samples ;
  bool volume ;
  struct tnode *next ;
} node ;

typedef struct audio_queue // bit of a waste
{
    node *start ;  // start node

} audio_queue ;


void insert ( int samples , bool volume , audio_queue *a  ) ;
void delete ( audio_queue *a ) ;
node * peek ( audio_queue *a  ) ;
void queue_print ( audio_queue *a  ) ;


void insert ( int samples , bool volume , audio_queue *a  )
{
  node *new = ( node * ) malloc ( sizeof(node) );
  if ( !a -> start )
    a -> start = new ;
  else
  {
    node *n ;
    for ( n = a -> start ; n -> next ; n = n -> next )
      ;
    n -> next = new ;
  }
  new -> samples = samples ;
  new -> volume = volume ;
  new -> next = NULL ;
}

void delete ( audio_queue *a )
{
  int value ;
  if ( ! a -> start )
    return ;
  a -> start = a -> start -> next ;
}
node *  peek ( audio_queue *a  )
{
  if ( ! a -> start )
    return NULL ;
  return a -> start ;
}

void queue_print ( audio_queue *a  )
{
  putchar('\n') ;
  for ( node *new = a -> start ; new ; new = new -> next)
    printf("%d %s  ", new -> samples , new -> volume ? "on" : "off"   );

  putchar('\n') ;
}

bool is_empty ( audio_queue *a )
{
    return a -> start == NULL ? true : false ;
}
