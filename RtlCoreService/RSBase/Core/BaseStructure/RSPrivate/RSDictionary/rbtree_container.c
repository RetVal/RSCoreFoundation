//
//  Used in RSCoreFoundation for dictionary.
//
//  Changed by RetVal on 11/4/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//
#include <stdlib.h>

#include "rbtree_container.h"
#include <RSCoreFoundation/RSAllocator.h>
void rbtree_container_init(rbtree_container *tree, size_t key_length, RSKeyCompare key_compare_function)
{
#ifdef _C99
	tree->root = RB_ROOT;
#else
	tree->root.rb_node = nil;
#endif
	tree->compare = key_compare_function;
	//tree->klen = key_length;
}

rbtree_container_node *rbtree_container_search(rbtree_container *tree, RSTypeRef key)
{
	struct rb_node *node = tree->root.rb_node;
	
	while (node) 
	{
		rbtree_container_node *cur = rb_entry(node, rbtree_container_node, rb_node);
		
		RSComparisonResult result = tree->compare(key, cur->key);
		
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return cur;
	}
	return nil;
}

int rbtree_container_insert(rbtree_container *tree, rbtree_container_node *cont)
{
	struct rb_node **new = &(tree->root.rb_node); 
	struct rb_node  *parent = nil;
	
	while (*new) 
	{
		rbtree_container_node *cur = rb_entry(*new, rbtree_container_node, rb_node);
		
		RSComparisonResult result = tree->compare(cont->key, cur->key);
		
		parent = *new;
		
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
		else
			return -1;
	}

	rb_link_node(&(cont->rb_node), parent, new);
	rb_insert_color(&(cont->rb_node), &(tree->root));

	return 0;
}

rbtree_container_node *rbtree_container_delete(rbtree_container *tree, RSTypeRef key)
{
	rbtree_container_node *find = rbtree_container_search(tree, key);
	if (!find)
		return nil;
	rb_erase(&find->rb_node, &(tree->root));
	return find;
}

rbtree_container_node *rbtree_container_replace(rbtree_container *tree, rbtree_container_node *cont)
{
	rbtree_container_node *find = rbtree_container_search(tree, cont->key);
	if (!find)
		return nil;
	rb_replace_node(&(find->rb_node), &(cont->rb_node), &(tree->root));
	return find;
}

rbtree_container_node *rbtree_container_first(rbtree_container *tree)
{
	struct rb_node *node = rb_first(&(tree->root));
	if (!node)
		return nil;
	return rb_entry(node, rbtree_container_node, rb_node);
}

rbtree_container_node *rbtree_container_last(rbtree_container *tree)
{
	struct rb_node *node = rb_last(&(tree->root));
	if (!node)
		return nil;
	return rb_entry(node, rbtree_container_node, rb_node);
}

rbtree_container_node *rbtree_container_next(rbtree_container_node *cont)
{
	struct rb_node *node = rb_next(&(cont->rb_node));
	if (!node)
		return nil;
	return rb_entry(node, rbtree_container_node, rb_node);
}

rbtree_container_node *rbtree_container_prev(rbtree_container_node *cont)
{
	struct rb_node *node = rb_prev(&(cont->rb_node));
	if (!node)
		return nil;
	return rb_entry(node, rbtree_container_node, rb_node);
}

void rbtree_container_erase(rbtree_container *tree, rbtree_container_node *cont)
{
	rb_erase(&(cont->rb_node), &(tree->root));
}

rbtree_container_node *rbtree_container_node_malloc(rbtree_container *tree, size_t data_length)
{
	rbtree_container_node *p = nil;
    if ((p = RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(rbtree_container_node))))
    {
        //p->dlen = data_length;
        p->key = nil;
        p->value = nil;
    }
	return p;
}

rbtree_container_node *rbtree_container_node_array_malloc(rbtree_container *tree,
	size_t data_length, size_t array_length)
{
	rbtree_container_node *p = (rbtree_container_node *)malloc((sizeof(rbtree_container_node)) * array_length);
	size_t i;
	for(i=0; i<array_length; i++)
	{
		//p[i].dlen = data_length;
		p[i].key = nil;//(void*)(((char*)p) + sizeof(rbtree_container_node) * array_length + tree->klen * i);
		p[i].value = nil;//(unsigned char*)(((char*)p) + (sizeof(rbtree_container_node) + tree->klen) * array_length + data_length * i);
	}
	return p;
}

void rbtree_container_node_free(rbtree_container_node *tree_node)
{
	RSAllocatorDeallocate(RSAllocatorSystemDefault, (tree_node));
}
