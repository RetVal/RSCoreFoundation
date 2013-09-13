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

#endif
