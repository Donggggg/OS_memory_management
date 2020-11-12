#include <stdio.h>

typedef struct node 
{ 
	int i; 
	struct node *next; 
}
node; 

#define MAX_NODES   10

node *create_node(int a)
{ 
	static node node_pool[ MAX_NODES ];
	static int  next_node = 0;

	if ( next_node >= MAX_NODES )
		return ( node * )NULL;

	node *n = &(node_pool[ next_node++ ]);

	n->i = a;
	n->next = NULL;

	return n; 
} 

int main( )
{ 
	int i; 
	node *newtemp, *root, *temp; 

	root = create_node( 0 ); 
	temp = root; 

	for (i = 1;(newtemp = create_node(i)) && i<MAX_NODES; ++i)
	{ 
		temp->next = newtemp; 

		if (newtemp)
		{
			printf( "temp->i = %d\n", temp->i );
			printf( "temp->next->i = %d\n", temp->next->i );
			temp = temp->next;
		}
	}


	for ( temp = root; temp != NULL; temp = temp->next )
		printf( " %d ", temp->i );

	return  0;
}
