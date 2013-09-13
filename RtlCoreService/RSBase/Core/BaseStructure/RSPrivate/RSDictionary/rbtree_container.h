/*
 *
 * Name:	红黑树通用容器（linux内核封装、纯C）
 * Auther:	chishaxie
 * Date:	2012.7.29
 *
 * Description:	log(n)的插入、删除、查询，带迭代器功能
 *
 */
//
//  Used in RSCoreFoundation for dictionary.
//
//  Changed by RetVal on 11/4/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//
#ifndef _RBTREE_CONTAINER_H_
#define _RBTREE_CONTAINER_H_

#include "rbtree.h"
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSRuntime.h>
typedef struct
{
	struct rb_node rb_node; //rbtree内核结构（必须为首个字段）
	//RSIndex dlen; //data的长度
	RSTypeRef key; //指向键
	RSTypeRef value; //指向值
} rbtree_container_node; //节点

/* 比较函数 key1 - key2 为从小到大排序 */

typedef struct
{
	struct rb_root root; //rbtree内核结构
	//size_t klen; //key的长度（定长key）
	RSKeyCompare compare; //key的比较函数
} rbtree_container; //容器

/* 给节点设置key （参数为：节点指针，key的实际类型，值）*/
#define rbtree_container_node_set_key(pnode,type,value) (*((type*)((pnode)->key)) = (value))

/*
 * 基本操作
 */

/* 初始化 */
void rbtree_container_init(rbtree_container *tree, size_t key_length, RSKeyCompare key_compare_function);

/* 查找 （无则返回NULL） */
rbtree_container_node* rbtree_container_search(rbtree_container *tree, RSTypeRef key);

/* 插入 （成功返回0，键重复返回-1） */
int rbtree_container_insert(rbtree_container *tree, rbtree_container_node *cont);

/* 删除 （成功返回删除了的节点，key不存在返回NULL） */
rbtree_container_node *rbtree_container_delete(rbtree_container *tree, RSTypeRef key);

/* 替换 （成功返回被替换掉的节点，cont的key不存在则返回NULL） */
rbtree_container_node *rbtree_container_replace(rbtree_container *tree, rbtree_container_node *cont);

/*
 * 迭代器 （无则返回NULL）
 */

/* 获取迭代的开始节点 */
rbtree_container_node *rbtree_container_first(rbtree_container *tree);

/* 获取迭代的结束节点 */
rbtree_container_node *rbtree_container_last(rbtree_container *tree);

/* 获取下一个节点（向前迭代） */
rbtree_container_node *rbtree_container_next(rbtree_container_node *cont);

/* 获取上一个节点（向后迭代） */
rbtree_container_node *rbtree_container_prev(rbtree_container_node *cont);

/* 直接删除节点（必须保证cont在tree里面，否则产生未知错误） */
void rbtree_container_erase(rbtree_container *tree, rbtree_container_node *cont);

/*
 * 内存分配
 */

 /* 给节点分配内存（高效，节点的dlen已赋值，key、data的位置已经挂载好，直接用即可） */
rbtree_container_node *rbtree_container_node_malloc(rbtree_container *tree, size_t data_length);

/* 给节点数组分配内存，要求数据部分定长（高效，节点的dlen已赋值，key、data的位置已经挂载好，直接用即可） */
rbtree_container_node *rbtree_container_node_array_malloc(rbtree_container *tree,
	size_t data_length, size_t array_length);

/* 释放节点的内存（和 rbtree_container_node_malloc 或 rbtree_container_node_malloc_array 匹配使用） */
void rbtree_container_node_free(rbtree_container_node *tree_node);

#endif
