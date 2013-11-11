//
//  RSPrivateTask.h
//  RSCoreFoundation
//
//  Created by RetVal on 5/15/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPrivateTask_h
#define RSCoreFoundation_RSPrivateTask_h

#ifndef __RS_TID
typedef pid_t tid_t;
#define __RS_TID
#endif

pid_t __RSGetPid();
tid_t __RSGetTid();
tid_t __RSGetTidWithThread(pthread_t thread);

typedef void* pthread_workqueue_t;
typedef void  pthread_workitem_handle_t;

#ifdef __APPLE__
typedef struct {
    uint32_t sig;
    int queueprio;
    int overcommit;
    unsigned int resv2[13];
} pthread_workqueue_attr_t;

#define WORKQ_HIGH_PRIOQUEUE	0	/* high priority queue */
#define WORKQ_DEFAULT_PRIOQUEUE	1	/* default priority queue */
#define WORKQ_LOW_PRIOQUEUE	2	/* low priority queue */
#define WORKQ_BG_PRIOQUEUE	3	/* background priority queue */

#define WORKQ_NUM_PRIOQUEUE	4

extern __int32_t workq_targetconc[WORKQ_NUM_PRIOQUEUE];

int pthread_workqueue_init_np(void);
int pthread_workqueue_attr_init_np(pthread_workqueue_attr_t * attr);
int pthread_workqueue_attr_destroy_np(pthread_workqueue_attr_t * attr);
int pthread_workqueue_attr_getqueuepriority_np(const pthread_workqueue_attr_t * attr, int * qprio);
/* WORKQ_HIGH/DEFAULT/LOW_PRIOQUEUE are the only valid values */
int pthread_workqueue_attr_setqueuepriority_np(pthread_workqueue_attr_t * attr, int qprio);
int pthread_workqueue_attr_getovercommit_np(const pthread_workqueue_attr_t * attr, int * ocommp);
int pthread_workqueue_attr_setovercommit_np(pthread_workqueue_attr_t * attr, int ocomm);

int pthread_workqueue_create_np(pthread_workqueue_t * workqp, const pthread_workqueue_attr_t * attr);
int pthread_workqueue_additem_np(pthread_workqueue_t workq, void ( *workitem_func)(void *), void * workitem_arg, pthread_workitem_handle_t * itemhandlep, unsigned int *gencountp);
/* If the queue value is WORKQ_NUM_PRIOQUEUE, the request for concurrency is for all queues */
int pthread_workqueue_requestconcurrency_np(int queue, int concurrency);
int pthread_workqueue_getovercommit_np(pthread_workqueue_t workq,  unsigned int *ocommp);

typedef void (*pthread_workqueue_function_t)(int queue_priority, int options, void *ctxt);
int pthread_workqueue_setdispatch_np(pthread_workqueue_function_t worker_func);

#define WORKQ_ADDTHREADS_OPTION_OVERCOMMIT 0x00000001
int pthread_workqueue_addthreads_np(int queue_priority, int options, int numthreads);
#endif

int __pthread_workqueue_setkill(int);
#endif
