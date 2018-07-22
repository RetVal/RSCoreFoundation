//
//  RSErrorCode.h
//  RSCoreFoundation
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSErrorCode_h
#define RSCoreFoundation_RSErrorCode_h

enum	_RSError
{
	kSuccess = 0,
	kNetSuccess = kSuccess,
    
	kOperatorFail = 10000,
	///////////////////////////////////////////////////////////////////////////
	kErrOpen,
	kErrRead,
	kErrWrite,
	kErrExecute,
	kErrDelete,
    kErrExisting,
	//kErrResource,
	kErrPrivilege,
	kErrUnknown,
	kErrFileEnd,
	kErrMiss,           // something not found
	kErrCreateDirectory,
	///////////////////////////////////////////////////////////////////////////
	kErrMmRes,
	kErrMmNode,
	kErrMmSmall,
	kErrMmBig,
	kErrMmPrivilege,
	kErrNil,			//the pointer is point to nil
	kErrNNil,			//the pointer is not point to nil
	kErrMmUnknown,
	///////////////////////////////////////////////////////////////////////////
	kErrStkFull,				//the stack is full
	///////////////////////////////////////////////////////////////////////////
	kErrUnAvailable,				//something is unavailable
	kErrFormat,
	kErrVerify,					//not pass the verify
	///////////////////////////////////////////////////////////////////////////
	kErrInit,
	kErrTimeExpire,
	kErrCount,
    kErrReservedLine  =  200
};

#endif
