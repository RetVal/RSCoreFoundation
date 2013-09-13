//
//  RSBaseIODefine
//
//  Created by RetVal on 9/1/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include "fcntl.h"

#include "MSKit.h"
#include "RtlBufferKit.h"
#include "RtlServices.h"

#ifndef RtlCustomerType_IOERR
#define RtlCustomerType_IOERR
typedef RtlError IOERR;
#endif

#define IO_CREATE_DEFAULT		0		//IO_CREATE_DEFAULT IS IO_CREATE_ALWAYS
#define IO_CREATE_ALWAYS		1		//
#define IO_CREATE_OPEN_EXISTING	2		//if is not existing, error
#define IO_CREATE_CREATE_NEW	3		//if is exiting, error
#define IO_CREATE_LIMIT			4       //the count, reserved.


#define IO_OPEN_RB				1       //read
#define	IO_OPEN_WB				2       //write
#define IO_OPEN_RWB				3       //read & write
#define IO_OPEN_LIMIT			4       //the count, reserved.

