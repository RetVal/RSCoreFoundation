//
//  RSConnection.c
//  RSCoreFoundation
//
//  Created by RetVal on 6/26/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSConnection.h"
#include <RSCoreFoundation/RSSocket.h>
#include <RSCoreFoundation/RSData.h>
#include "RSDelegate.h"

#include <dispatch/dispatch.h>
#import <arpa/inet.h>
#import <fcntl.h>
#import <ifaddrs.h>
#import <netdb.h>
#import <netinet/in.h>
#import <net/if.h>
#import <sys/socket.h>
#import <sys/types.h>
#import <sys/ioctl.h>
#import <sys/poll.h>
#import <sys/uio.h>
#import <unistd.h>

#include <RSCoreFoundation/RSRuntime.h>

#pragma mark -
#pragma mark __RSConnectionPreBuffer
typedef struct ____RSConnectionPreBuffer *__RSConnectionPreBufferRef;

struct ____RSConnectionPreBuffer
{
    RSRuntimeBase _base;
    uint8_t *preBuffer;
	size_t preBufferSize;
	
	uint8_t *readPointer;
	uint8_t *writePointer;
};

static void ____RSConnectionPreBufferClassDeallocate(RSTypeRef rs)
{
    __RSConnectionPreBufferRef instance = (__RSConnectionPreBufferRef)rs;
    if (instance->preBuffer) RSAllocatorDeallocate(RSAllocatorSystemDefault, instance->preBuffer);
}

static RSStringRef ____RSConnectionPreBufferClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("__RSConnectionPreBuffer %p"), rs);
    return description;
}

static RSRuntimeClass ____RSConnectionPreBufferClass =
{
    _RSRuntimeScannedObject,
    "__RSConnectionPreBuffer",
    nil,
    nil,
    ____RSConnectionPreBufferClassDeallocate,
    nil,
    nil,
    ____RSConnectionPreBufferClassDescription,
    nil,
    nil
};

static RSTypeID _k__RSConnectionPreBufferTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID __RSConnectionPreBufferGetTypeID()
{
    return _k__RSConnectionPreBufferTypeID;
}

RSPrivate void ____RSConnectionPreBufferInitialize()
{
    _k__RSConnectionPreBufferTypeID = __RSRuntimeRegisterClass(&____RSConnectionPreBufferClass);
    __RSRuntimeSetClassTypeID(&____RSConnectionPreBufferClass, _k__RSConnectionPreBufferTypeID);
}


static __RSConnectionPreBufferRef ____RSConnectionPreBufferCreateInstance(RSAllocatorRef allocator, size_t numBytes)
{
    __RSConnectionPreBufferRef instance = (__RSConnectionPreBufferRef)__RSRuntimeCreateInstance(allocator, _k__RSConnectionPreBufferTypeID, sizeof(struct ____RSConnectionPreBuffer) - sizeof(RSRuntimeBase));
    
    instance->preBufferSize = numBytes;
    instance->preBuffer = RSAllocatorAllocate(RSAllocatorSystemDefault, instance->preBufferSize);
    instance->readPointer = instance->preBuffer;
    instance->writePointer = instance->preBuffer;
    
    return instance;
}

static void __RSConnectionPreBufferEnsureCapacityForWrite(__RSConnectionPreBufferRef instance, size_t numBytes)
{
    size_t availableSpace = instance->preBufferSize - (instance->writePointer - instance->readPointer);
    if (numBytes - availableSpace)
    {
        size_t additionalBytes = numBytes - availableSpace;
		
		size_t newPreBufferSize = instance->preBufferSize + additionalBytes;
		uint8_t *newPreBuffer = realloc(instance->preBuffer, newPreBufferSize);
		
		size_t readPointerOffset = instance->readPointer - instance->preBuffer;
		size_t writePointerOffset = instance->writePointer - instance->preBuffer;
		
		instance->preBuffer = newPreBuffer;
		instance->preBufferSize = newPreBufferSize;
		
		instance->readPointer = instance->preBuffer + readPointerOffset;
		instance->writePointer = instance->preBuffer + writePointerOffset;
    }
}

static size_t __RSConnectionPreBufferAvaiableBytes(__RSConnectionPreBufferRef instance)
{
    return instance->writePointer - instance->readPointer;
}

static RSBitU8 *__RSConnectionPreBufferReadBuffer(__RSConnectionPreBufferRef instance)
{
    return instance->readPointer;
}

static void __RSConnectionPreBufferGetReadBuffer(__RSConnectionPreBufferRef instance, RSBitU8 **bufferPtr, size_t *availableBytes)
{
    if (bufferPtr) *bufferPtr = instance->readPointer;
    if (availableBytes) *availableBytes = instance->writePointer - instance->readPointer;
}

static void __RSConnectionPreBufferDidRead(__RSConnectionPreBufferRef instance, size_t bytesRead)
{
    instance->readPointer += bytesRead;
    if (instance->readPointer == instance->writePointer)
    {
        instance->readPointer = instance->preBuffer;
        instance->writePointer= instance->preBuffer;
    }
}

static size_t __RSConnectionPreBufferAvailableSpace(__RSConnectionPreBufferRef instance)
{
    return instance->preBufferSize - (instance->writePointer - instance->readPointer);
}

static RSBitU8 *__RSConnectionPreBufferWriteBuffer(__RSConnectionPreBufferRef instance)
{
    return instance->writePointer;
}

static void __RSConnectionPreBufferGetWriteBuffer(__RSConnectionPreBufferRef instance, RSBitU8 **bufferPtr, size_t *availableSpace)
{
    if (bufferPtr) *bufferPtr = instance->writePointer;
    if (availableSpace) *availableSpace = instance->preBufferSize - (instance->writePointer - instance->readPointer);
}

static void __RSConnectionPreBufferDidWrite(__RSConnectionPreBufferRef instance, size_t bytesWritten)
{
    instance->writePointer += bytesWritten;
}

static void __RSConnectionPreBufferReset(__RSConnectionPreBufferRef instance)
{
    instance->readPointer = instance->preBuffer;
    instance->writePointer = instance->preBuffer;
}

#pragma mark -
#pragma mark __RSConnectionReadPacket
typedef struct ____RSConnectionReadPacket *__RSConnectionReadPacketRef;

struct ____RSConnectionReadPacket
{
    RSRuntimeBase _base;
    RSMutableDataRef buffer;
    RSUInteger startOffset;
    RSUInteger bytesDone;
    RSUInteger maxLength;
    RSTimeInterval timeout;
    RSUInteger readLength;
    RSDataRef term;
    BOOL bufferOwner;
    RSUInteger originalBufferLength;
    long tag;
};

static RSTypeRef ____RSConnectionReadPacketClassCopy(RSAllocatorRef allocator, RSTypeRef rs, BOOL mutableCopy)
{
    return RSRetain(rs);
}

static void ____RSConnectionReadPacketClassDeallocate(RSTypeRef rs)
{
    __RSConnectionReadPacketRef packet = (__RSConnectionReadPacketRef)rs;
    if (packet->buffer) RSRelease(packet->buffer);
    if (packet->term) RSRelease(packet->term);
}

static BOOL ____RSConnectionReadPacketClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    __RSConnectionReadPacketRef __RSConnectionReadPacket1 = (__RSConnectionReadPacketRef)rs1;
    __RSConnectionReadPacketRef __RSConnectionReadPacket2 = (__RSConnectionReadPacketRef)rs2;
    BOOL result = NO;
    
    if (__RSConnectionReadPacket1->startOffset != __RSConnectionReadPacket2->startOffset) return result;
    if (__RSConnectionReadPacket1->maxLength != __RSConnectionReadPacket2->maxLength) return result;
    if (__RSConnectionReadPacket1->timeout != __RSConnectionReadPacket2->timeout) return result;
    if (__RSConnectionReadPacket1->readLength != __RSConnectionReadPacket2->readLength) return result;
    if (__RSConnectionReadPacket1->bufferOwner != __RSConnectionReadPacket2->bufferOwner) return result;
    if (__RSConnectionReadPacket1->originalBufferLength != __RSConnectionReadPacket2->originalBufferLength) return result;
    if (NO == RSEqual(__RSConnectionReadPacket1->buffer, __RSConnectionReadPacket2->buffer)) return result;
    if (NO == RSEqual(__RSConnectionReadPacket1->term, __RSConnectionReadPacket2->term)) return result;
    result = YES;
    return result;
}

static RSHashCode ____RSConnectionReadPacketClassHash(RSTypeRef rs)
{
    return RSHash(((__RSConnectionReadPacketRef)rs)->buffer);
}

static RSStringRef ____RSConnectionReadPacketClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("____RSConnectionReadPacket = %p"), rs);
    return description;
}

static RSRuntimeClass ____RSConnectionReadPacketClass =
{
    _RSRuntimeScannedObject,
    "__RSConnectionReadPacket",
    nil,
    ____RSConnectionReadPacketClassCopy,
    ____RSConnectionReadPacketClassDeallocate,
    ____RSConnectionReadPacketClassEqual,
    ____RSConnectionReadPacketClassHash,
    ____RSConnectionReadPacketClassDescription,
    nil,
    nil
};

static RSTypeID _k__RSConnectionReadPacketTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID __RSConnectionReadPacketGetTypeID()
{
    return _k__RSConnectionReadPacketTypeID;
}

RSPrivate void ____RSConnectionReadPacketInitialize()
{
    _k__RSConnectionReadPacketTypeID = __RSRuntimeRegisterClass(&____RSConnectionReadPacketClass);
    __RSRuntimeSetClassTypeID(&____RSConnectionReadPacketClass, _k__RSConnectionReadPacketTypeID);
}

static __RSConnectionReadPacketRef ____RSConnectionReadPacketCreateInstance(RSAllocatorRef allocator, RSMutableDataRef data, RSUInteger startOffset, RSUInteger maxLength, RSTimeInterval timeout, RSUInteger readLength, RSDataRef term, long tag)
{
    __RSConnectionReadPacketRef instance = (__RSConnectionReadPacketRef)__RSRuntimeCreateInstance(allocator, _k__RSConnectionReadPacketTypeID, sizeof(struct ____RSConnectionReadPacket) - sizeof(RSRuntimeBase));
    
    instance->bytesDone = 0;
    instance->maxLength = maxLength;
    instance->timeout = timeout;
    instance->readLength = readLength;
    instance->term = RSCopy(RSAllocatorSystemDefault, term);
    instance->tag = tag;
    
    if (data)
    {
        instance->buffer = (RSMutableDataRef)RSRetain(data);
        instance->startOffset = startOffset;
        instance->bufferOwner = NO;
        instance->originalBufferLength = RSDataGetLength(data);
    }
    else
    {
        if (instance->readLength)
            instance->buffer = RSDataCreateMutable(allocator, readLength);
        else
            instance->buffer = RSDataCreateMutable(allocator, 0);
        instance->startOffset = 0;
        instance->bufferOwner = YES;
        instance->originalBufferLength = 0;
    }
    
    return instance;
}

static void __RSConnectionReadPacketEnsureCapacityForAdditionalDataOfLength(__RSConnectionReadPacketRef rp, RSUInteger bytesToRead)
{
    RSUInteger buffSize = RSDataGetLength(rp->buffer);
    RSUInteger buffUsed = rp->startOffset + rp->bytesDone;
    RSUInteger buffSpace = buffSize - buffUsed;
    
    if (bytesToRead > buffSize)
    {
        RSUInteger buffInc = bytesToRead - buffSpace;
        RSDataIncreaseLength(rp->buffer, buffInc);
    }
}

static RSUInteger __RSConnectionReadPacketOptimalReadLength(__RSConnectionReadPacketRef instance, RSUInteger defaultValue, BOOL *shouldPreBufferPtr)
{
    RSUInteger result;
	
	if (instance->readLength > 0)
	{
		// Read a specific length of data
		
		result = MIN(defaultValue, (instance->readLength - instance->bytesDone));
		
		// There is no need to prebuffer since we know exactly how much data we need to read.
		// Even if the buffer isn't currently big enough to fit this amount of data,
		// it would have to be resized eventually anyway.
		
		if (shouldPreBufferPtr)
			*shouldPreBufferPtr = NO;
	}
	else
	{
		// Either reading until we find a specified terminator,
		// or we're simply reading all available data.
		//
		// In other words, one of:
		//
		// - readDataToData packet
		// - readDataWithTimeout packet
		
		if (instance->maxLength > 0)
			result =  MIN(defaultValue, (instance->maxLength - instance->bytesDone));
		else
			result = defaultValue;
		
		// Since we don't know the size of the read in advance,
		// the shouldPreBuffer decision is based upon whether the returned value would fit
		// in the current buffer without requiring a resize of the buffer.
		//
		// This is because, in all likelyhood, the amount read from the socket will be less than the default value.
		// Thus we should avoid over-allocating the read buffer when we can simply use the pre-buffer instead.
		
		if (shouldPreBufferPtr)
		{
			RSUInteger buffSize = RSDataGetLength(instance->buffer);
			RSUInteger buffUsed = instance->startOffset + instance->bytesDone;
			
			RSUInteger buffSpace = buffSize - buffUsed;
			
			if (buffSpace >= result)
				*shouldPreBufferPtr = NO;
			else
				*shouldPreBufferPtr = YES;
		}
	}
	
	return result;
}

static RSUInteger __RSConnectionReadPacketReadLengthForNonTerm(__RSConnectionReadPacketRef instance, RSUInteger bytesAvailable)
{
    assert(instance->term == nil);
    assert(bytesAvailable > 0);
    if (instance->readLength)
        return MIN(bytesAvailable, (instance->readLength - instance->bytesDone));
    else
    {
        RSUInteger result = bytesAvailable;
        if (instance->maxLength > 0){
            result = MIN(result, (instance->maxLength - instance->bytesDone));
        }
        return result;
    }
}

static RSUInteger __RSConnectionReadPacketReadLengthForTerm(__RSConnectionReadPacketRef instance, RSUInteger bytesAvailable, BOOL *shouldPreBufferPtr)
{
    assert(instance->term != nil);
	assert(bytesAvailable > 0);
    
    RSUInteger result = bytesAvailable;
    
    if (instance->maxLength > 0)
        result = MIN(result, (instance->maxLength - instance->bytesDone));
    
    if (shouldPreBufferPtr)
    {
        RSUInteger buffSize = RSDataGetLength(instance->buffer);
        RSUInteger buffUsed = instance->startOffset + instance->bytesDone;
        if ((buffSize - buffUsed) >= result)
            *shouldPreBufferPtr = NO;
        else *shouldPreBufferPtr = YES;
    }
    return result;
}

static RSUInteger __RSConnectionReadPacketReadForTerm(__RSConnectionReadPacketRef instance, __RSConnectionPreBufferRef preBuffer, BOOL *foundPtr)
{
    assert(instance->term != nil);
    assert(__RSConnectionPreBufferAvaiableBytes(preBuffer) > 0);
    
    BOOL found = NO;
    
    RSUInteger termLength = RSDataGetLength(instance->term);
    RSUInteger preBufferLength = __RSConnectionPreBufferAvaiableBytes(preBuffer);
    
    if ((instance->bytesDone + preBufferLength) < termLength) return preBufferLength;
    
    RSUInteger maxPreBufferLength = 0;
    if (instance->maxLength > 0) maxPreBufferLength = MIN(preBufferLength, (instance->maxLength - instance->bytesDone));
    else maxPreBufferLength = preBufferLength;
    RSBitU8 seq[termLength];
    
    const void *termBuf = RSDataGetBytesPtr(instance->term);
    
    RSUInteger bufLen = MIN(instance->bytesDone, (termLength - 1));
    RSBitU8 *buf = (RSBitU8 *)RSDataMutableBytes(instance->buffer) + instance->startOffset + instance->bytesDone - bufLen;
    
    RSUInteger preLen = termLength - bufLen;
    const RSBitU8 *pre = __RSConnectionPreBufferReadBuffer(preBuffer);
    
    RSUInteger loopCount = bufLen + maxPreBufferLength - termLength + 1;
    
    RSUInteger result = maxPreBufferLength;
    
    RSUInteger i;
    
    for (i = 0; i < loopCount; i++)
    {
        if (bufLen > 0)
        {
            memcpy(seq, buf, bufLen);
            memcpy(seq + bufLen, pre, preLen);
            
            if (memcmp(seq, termBuf, termLength) == 0)
            {
                result = preLen;
                found = YES;
                break;
            }
            
            ++buf;
            --bufLen;
            ++preLen;
        }
        else
        {
            if (memcmp(pre, termBuf, termLength) == 0)
            {
                RSUInteger preOffset = pre - __RSConnectionPreBufferReadBuffer(preBuffer);
                result = preOffset + termLength;
                found = YES;
                break;
            }
            
            ++pre;
        }
    }
    
    if (foundPtr) *foundPtr = found;
    return result;
}

static RSUInteger __RSConnectionReadPacketSearchForTermAfterPreBuffering(__RSConnectionReadPacketRef instance, ssize_t numBytes)
{
    assert(instance->term != nil);
    
    RSBitU8 *buff = RSDataMutableBytes(instance->buffer);
    RSUInteger buffLength = instance->bytesDone + numBytes;
    const void *termBuff = RSDataGetBytesPtr(instance->term);
    RSUInteger termLength = RSDataGetLength(instance->term);
    
    RSUInteger i = ((buffLength - numBytes) >= termLength) ? (buffLength - numBytes - termLength + 1) : 0;
    
    while (i + termLength <= buffLength) {
        RSBitU8 *subBuffer = buff + instance->startOffset + i;
        if (memcmp(subBuffer, termBuff, termLength) == 0)
        {
            return buffLength - (i + termLength);
        }
        ++i;
    }
    return -1;
}

#pragma mark -
#pragma mark __RSConnectionWritePacket
typedef struct ____RSConnectionWritePacket *__RSConnectionWritePacketRef;
struct ____RSConnectionWritePacket
{
    RSRuntimeBase _base;
    RSDataRef buffer;
	RSUInteger bytesDone;
	long tag;
	RSTimeInterval timeout;;
};

static void ____RSConnectionWritePacketClassDeallocate(RSTypeRef rs)
{
    __RSConnectionWritePacketRef instance = (__RSConnectionWritePacketRef)rs;
    if (instance->buffer) RSRelease(instance->buffer);
    instance->buffer = nil;
}

static RSStringRef ____RSConnectionWritePacketClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("__RSConnectionWritePacket %p"), rs);
    return description;
}

static RSRuntimeClass ____RSConnectionWritePacketClass =
{
    _RSRuntimeScannedObject,
    "__RSConnectionWritePacket",
    nil,
    nil,
    ____RSConnectionWritePacketClassDeallocate,
    nil,
    nil,
    ____RSConnectionWritePacketClassDescription,
    nil,
    nil
};

static RSTypeID _k__RSConnectionWritePacketTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID __RSConnectionWritePacketGetTypeID()
{
    return _k__RSConnectionWritePacketTypeID;
}

RSPrivate void ____RSConnectionWritePacketInitialize()
{
    _k__RSConnectionWritePacketTypeID = __RSRuntimeRegisterClass(&____RSConnectionWritePacketClass);
    __RSRuntimeSetClassTypeID(&____RSConnectionWritePacketClass, _k__RSConnectionWritePacketTypeID);
}

static __RSConnectionWritePacketRef ____RSConnectionWritePacketCreateInstance(RSAllocatorRef allocator, RSDataRef data, RSTimeInterval timeout, long tag)
{
    __RSConnectionWritePacketRef instance = (__RSConnectionWritePacketRef)__RSRuntimeCreateInstance(allocator, _k__RSConnectionWritePacketTypeID, sizeof(struct ____RSConnectionWritePacket) - sizeof(RSRuntimeBase));
    
    if (data) instance->buffer = RSRetain(data);
    instance->bytesDone = 0;
    instance->timeout = timeout;
    instance->tag = tag;
    
    return instance;
}


#pragma mark -
#pragma mark __RSConnectionSpecialPacket

typedef struct ____RSConnectionSpecialPacket *__RSConnectionSpecialPacketRef;

struct ____RSConnectionSpecialPacket
{
    RSRuntimeBase _base;
    RSDictionaryRef tlsSettings;
};

static void ____RSConnectionSpecialPacketClassDeallocate(RSTypeRef rs)
{
    __RSConnectionSpecialPacketRef instance = (__RSConnectionSpecialPacketRef)rs;
    if (instance->tlsSettings) RSRelease(instance->tlsSettings);
    instance->tlsSettings = nil;
}

static RSStringRef ____RSConnectionSpecialPacketClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("__RSConnectionSpecialPacket %p"), rs);
    return description;
}

static RSRuntimeClass ____RSConnectionSpecialPacketClass =
{
    _RSRuntimeScannedObject,
    "__RSConnectionSpecialPacket",
    nil,
    nil,
    ____RSConnectionSpecialPacketClassDeallocate,
    nil,
    nil,
    ____RSConnectionSpecialPacketClassDescription,
    nil,
    nil
};

static RSTypeID _k__RSConnectionSpecialPacketTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID __RSConnectionSpecialPacketGetTypeID()
{
    return _k__RSConnectionSpecialPacketTypeID;
}

RSPrivate void ____RSConnectionSpecialPacketInitialize()
{
    _k__RSConnectionSpecialPacketTypeID = __RSRuntimeRegisterClass(&____RSConnectionSpecialPacketClass);
    __RSRuntimeSetClassTypeID(&____RSConnectionSpecialPacketClass, _k__RSConnectionSpecialPacketTypeID);
}

static __RSConnectionSpecialPacketRef ____RSConnectionSpecialPacketCreateInstance(RSAllocatorRef allocator, RSDictionaryRef setting)
{
    __RSConnectionSpecialPacketRef instance = (__RSConnectionSpecialPacketRef)__RSRuntimeCreateInstance(allocator, _k__RSConnectionSpecialPacketTypeID, sizeof(struct ____RSConnectionSpecialPacket) - sizeof(RSRuntimeBase));
    
    if (setting) instance->tlsSettings = RSRetain(setting); // copy ?
    
    return instance;
}

#pragma mark -
#pragma mark __RSConnection

#define SOCKET_NULL -1

enum RSConnectionFlags
{
	kSocketStarted                 = 1 <<  0,  // If set, socket has been started (accepting/connecting)
	kConnected                     = 1 <<  1,  // If set, the socket is connected
	kForbidReadsWrites             = 1 <<  2,  // If set, no new reads or writes are allowed
	kReadsPaused                   = 1 <<  3,  // If set, reads are paused due to possible timeout
	kWritesPaused                  = 1 <<  4,  // If set, writes are paused due to possible timeout
	kDisconnectAfterReads          = 1 <<  5,  // If set, disconnect after no more reads are queued
	kDisconnectAfterWrites         = 1 <<  6,  // If set, disconnect after no more writes are queued
	kSocketCanAcceptBytes          = 1 <<  7,  // If set, we know socket can accept bytes. If unset, it's unknown.
	kReadSourceSuspended           = 1 <<  8,  // If set, the read source is suspended
	kWriteSourceSuspended          = 1 <<  9,  // If set, the write source is suspended
	kQueuedTLS                     = 1 << 10,  // If set, we've queued an upgrade to TLS
	kStartingReadTLS               = 1 << 11,  // If set, we're waiting for TLS negotiation to complete
	kStartingWriteTLS              = 1 << 12,  // If set, we're waiting for TLS negotiation to complete
	kSocketSecure                  = 1 << 13,  // If set, socket is using secure communication via SSL/TLS
	kSocketHasReadEOF              = 1 << 14,  // If set, we have read EOF from socket
	kReadStreamClosed              = 1 << 15,  // If set, we've read EOF plus prebuffer has been drained
#if TARGET_OS_IPHONE
	kAddedStreamsToRunLoop         = 1 << 16,  // If set, CFStreams have been added to listener thread
	kUsingCFStreamForTLS           = 1 << 17,  // If set, we're forced to use CFStream instead of SecureTransport
	kSecureSocketHasBytesAvailable = 1 << 18,  // If set, CFReadStream has notified us of bytes available
#endif
};


enum RSConnectionConfig
{
	kIPv4Disabled              = 1 << 0,  // If set, IPv4 is disabled
	kIPv6Disabled              = 1 << 1,  // If set, IPv6 is disabled
	kPreferIPv6                = 1 << 2,  // If set, IPv6 is preferred over IPv4
	kAllowHalfDuplexConnection = 1 << 3,  // If set, the socket will stay open even if the read stream closes
};


enum RSConnectionError
{
	RSConnectionNoError = 10070,           // Never used
	RSConnectionBadConfigError,        // Invalid configuration
	RSConnectionBadParamError,         // Invalid parameter was passed
	RSConnectionConnectTimeoutError,   // A connect operation timed out
	RSConnectionReadTimeoutError,      // A read operation timed out
	RSConnectionWriteTimeoutError,     // A write operation timed out
	RSConnectionReadMaxedOutError,     // Reached set maxLength without completing
	RSConnectionClosedError,           // The remote peer closed the connection
	RSConnectionOtherError,            // Description provided in userInfo
};
typedef enum RSConnectionError RSConnectionError;

RS_PUBLIC_CONST_STRING_DECL(RSConnectionException, "RSConnectionException")
RS_PUBLIC_CONST_STRING_DECL(RSConnectionErrorDomain, "RSConnectionErrorDomain")
RS_PUBLIC_CONST_STRING_DECL(RSConnectionQueueName, "RSConnectionQueueName")
RS_PUBLIC_CONST_STRING_DECL(RSConnectionThreadName, "RSConnectionThreadName")


struct __RSConnection
{
    RSRuntimeBase _base;
    RSBitU32 flags;
	RSBitU16 config;
	
    ISA delegate;  // deleagate
    
	dispatch_queue_t delegateQueue;
	
	RSSocketHandle socket4FD;
	RSSocketHandle socket6FD;
	int connectIndex;
	RSDataRef connectInterface4;
	RSDataRef connectInterface6;
	
	dispatch_queue_t socketQueue;
	
	dispatch_source_t accept4Source;
	dispatch_source_t accept6Source;
	dispatch_source_t connectTimer;
	dispatch_source_t readSource;
	dispatch_source_t writeSource;
	dispatch_source_t readTimer;
	dispatch_source_t writeTimer;
	
	RSMutableArrayRef readQueue;
	RSMutableArrayRef writeQueue;
	
    __RSConnectionReadPacketRef currentRead;
    __RSConnectionWritePacketRef currentWrite;
	
	unsigned long socketFDBytesAvailable;
	
    __RSConnectionPreBufferRef preBuffer;
    
#if TARGET_OS_IPHONE
	CFStreamClientContext streamContext;
	CFReadStreamRef readStream;
	CFWriteStreamRef writeStream;
#endif
#if SECURE_TRANSPORT_MAYBE_AVAILABLE
	SSLContextRef sslContext;
	GCDAsyncSocketPreBuffer *sslPreBuffer;
	size_t sslWriteCachedLength;
	OSStatus sslErrCode;
#endif
	
	void *IsOnSocketQueueOrTargetQueueKey;
	
	RSTypeRef userData;
};

static void __RSConnectionClassInit(RSTypeRef rs)
{
    
}

static void __RSConnectionClassDeallocate(RSTypeRef rs)
{
    RSConnectionRef instance = (RSConnectionRef)rs;
    if (dispatch_get_specific(instance->IsOnSocketQueueOrTargetQueueKey))
    {
        // close with error nil
    }
    else
    {
        dispatch_sync(instance->socketQueue, ^{
            // close with error nil
        });
    }
    
    instance->delegate = nil;
    
#if !OS_OBJECT_USE_OBJC
	if (instance->delegateQueue) dispatch_release(instance->delegateQueue);
#endif
    instance->delegateQueue = nil;
    
#if !OS_OBJECT_USE_OBJC
	if (instance->socketQueue) dispatch_release(instance->socketQueue);
#endif
	instance->socketQueue = NULL;
    
    if (instance->connectInterface4) RSRelease(instance->connectInterface4);
    if (instance->connectInterface6) RSRelease(instance->connectInterface6);
    if (instance->readQueue) RSRelease(instance->readQueue);
    if (instance->writeQueue) RSRelease(instance->writeQueue);
    if (instance->currentRead) RSRelease(instance->currentRead);
    if (instance->currentWrite) RSRelease(instance->currentWrite);
    if (instance->preBuffer) RSRelease(instance->preBuffer);
    if (instance->userData) RSRelease(instance->userData);
    
}

static BOOL __RSConnectionClassEqual(RSTypeRef rs1, RSTypeRef rs2)
{
    RSConnectionRef RSConnection1 __unused = (RSConnectionRef)rs1;
    RSConnectionRef RSConnection2 __unused = (RSConnectionRef)rs2;
    BOOL result = NO;
    
    
    
    return result;
}

static RSStringRef __RSConnectionClassDescription(RSTypeRef rs)
{
    RSStringRef description = RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("RSConnection <%p>"), rs);
    return description;
}

static RSRuntimeClass __RSConnectionClass =
{
    _RSRuntimeScannedObject,
    "RSConnection",
    __RSConnectionClassInit,
    nil,
    __RSConnectionClassDeallocate,
    __RSConnectionClassEqual,
    nil,
    __RSConnectionClassDescription,
    nil,
    nil
};

static RSTypeID _RSConnectionTypeID = _RSRuntimeNotATypeID;

RSExport RSTypeID RSConnectionGetTypeID()
{
    return _RSConnectionTypeID;
}

RSPrivate void __RSConnectionInitialize()
{
    _RSConnectionTypeID = __RSRuntimeRegisterClass(&__RSConnectionClass);
    __RSRuntimeSetClassTypeID(&__RSConnectionClass, _RSConnectionTypeID);
}

#pragma mark -
#pragma mark RSConnection Class API


RSExport RSStringRef RSConnectionGetHostFromSockaddr4(const struct sockaddr_in *pSockaddr4)
{
    char addrBuf[INET_ADDRSTRLEN];
	
	if (inet_ntop(AF_INET, &pSockaddr4->sin_addr, addrBuf, (socklen_t)sizeof(addrBuf)) == NULL)
	{
		addrBuf[0] = '\0';
	}
	
	return RSStringWithCString(addrBuf, RSStringEncodingASCII);
}

RSExport RSStringRef RSConnectionGetHostFromSockaddr6(const struct sockaddr_in6 *pSockaddr6)
{
    char addrBuf[INET6_ADDRSTRLEN];
	
	if (inet_ntop(AF_INET6, &pSockaddr6->sin6_addr, addrBuf, (socklen_t)sizeof(addrBuf)) == NULL)
	{
		addrBuf[0] = '\0';
	}
	
	return RSStringWithCString(addrBuf, RSStringEncodingASCII);
}

RSExport RSBitU16 RSConnectionGetPortFromSockaddr4(const struct sockaddr_in *pSockAddr4)
{
    return ntohs(pSockAddr4->sin_port);
}

RSExport RSBitU16 RSConnectionGetPortFromSockaddr6(const struct sockaddr_in6 *pSockAddr6)
{
    return ntohs(pSockAddr6->sin6_port);
}

RSExport BOOL RSConnectionGetHostInformation(RSStringRef *hostPtr, RSBitU16 *portPtr, RSDataRef fromAddress)
{
    if (!fromAddress) return NO;
    if (RSDataGetLength(fromAddress) >= sizeof(struct sockaddr))
    {
        const struct sockaddr *sockaddrX = RSDataGetBytesPtr(fromAddress);
        if (sockaddrX->sa_family == AF_INET)
        {
            if (RSDataGetLength(fromAddress) >= sizeof(struct sockaddr_in))
            {
                struct sockaddr_in sockaddr4;
                memcpy(&sockaddr4, sockaddrX, sizeof(sockaddr4));
                if (hostPtr) *hostPtr = RSConnectionGetHostFromSockaddr4(&sockaddr4);
                if (portPtr) *portPtr = RSConnectionGetPortFromSockaddr4(&sockaddr4);
                return YES;
            }
        }
        else if (sockaddrX->sa_family == AF_INET6)
        {
            if (RSDataGetLength(fromAddress) >= sizeof(struct sockaddr_in6))
            {
                struct sockaddr_in6 sockaddr6;
                memcpy(&sockaddr6, sockaddrX, sizeof(sockaddr6));
                if (hostPtr) *hostPtr = RSConnectionGetHostFromSockaddr6(&sockaddr6);
                if (portPtr) *portPtr = RSConnectionGetPortFromSockaddr6(&sockaddr6);
                return YES;
            }
        }
    }
    return NO;
}

RSExport RSStringRef RSConnectionGetHostFromAddress(RSDataRef address)
{
    RSStringRef host = nil;
    if (RSConnectionGetHostInformation(&host, nil, address))
        return host;
    return nil;
}

RSExport RSBitU16 RSConnectionGetPortFromAddress(RSDataRef address)
{
    RSBitU16 port;
    if (RSConnectionGetHostInformation(nil, &port, address))
        return port;
    return 0;
}

static RSConnectionRef __RSConnectionCreateInstance(RSAllocatorRef allocator, void *deleagate, dispatch_queue_t dq, dispatch_queue_t sq)
{
    RSConnectionRef instance = (RSConnectionRef)__RSRuntimeCreateInstance(allocator, _RSConnectionTypeID, sizeof(struct __RSConnection) - sizeof(RSRuntimeBase));
    
    instance->delegate = deleagate;
    instance->delegateQueue = dq;
    
#if !OS_OBJECT_USE_OBJC
    if (dq) dispatch_retain(dq);
#endif
    
    instance->socket4FD = SOCKET_NULL;
    instance->socket6FD = SOCKET_NULL;
    instance->connectIndex = 0;
    
    if (sq)
    {
        assert(sq != dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_LOW, 0));
        assert(sq != dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 0));
        assert(sq != dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0));
        instance->socketQueue = sq;
#if !OS_OBJECT_USE_OBJC
        dispatch_retain(sq);
#endif
    }
    else
    {
        instance->socketQueue = dispatch_queue_create(RSStringGetCStringPtr(RSConnectionQueueName, RSStringEncodingASCII), 0);
    }
    
    instance->IsOnSocketQueueOrTargetQueueKey = &instance->IsOnSocketQueueOrTargetQueueKey;
    
    void *nonNullUnusedPointer = (void *)instance;
    dispatch_queue_set_specific(instance->socketQueue, instance->IsOnSocketQueueOrTargetQueueKey, nonNullUnusedPointer, NULL);
    
    instance->readQueue = RSArrayCreateMutable(RSAllocatorSystemDefault, 5);
    instance->currentRead = nil;
    
    instance->writeQueue = RSArrayCreateMutable(RSAllocatorSystemDefault, 5);
    instance->currentWrite = nil;
    
    instance->preBuffer = ____RSConnectionPreBufferCreateInstance(RSAllocatorSystemDefault, 1024 * 4);
    
    return instance;
}

static void __RSConnectionDoBlock(RSConnectionRef connection, dispatch_block_t block)
{
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
		block();
	else
		dispatch_sync(connection->socketQueue, block);
}

static void __RSConnectionDoBlockAsync(RSConnectionRef connection, dispatch_block_t block)
{
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
		block();
	else
		dispatch_async(connection->socketQueue, block);
}

RSExport ISA RSConnectionGetDelegate(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
    {
        return connection->delegate;
    }
    else
    {
        __block ISA result = nil;
        dispatch_sync(connection->socketQueue, ^{
            result = connection->delegate;
        });
        return result;
    }
}

RSExport void RSConnectionSetDelegate(RSConnectionRef connection, ISA newDelegate, BOOL synchronously)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    dispatch_block_t block = ^{
		connection->delegate = newDelegate;
	};
    
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
    {
        block();
    }
    else {
        if (synchronously)
            dispatch_sync(connection->socketQueue, block);
        else dispatch_async(connection->socketQueue, block);
    }
}

RSExport void RSConnectionSetDelegateSyn(RSConnectionRef connection, ISA newDelegate)
{
    return RSConnectionSetDelegate(connection, newDelegate, YES);
}

RSExport void RSConnectionSetDelegateAsyn(RSConnectionRef connection, ISA newDelegate)
{
    return RSConnectionSetDelegate(connection, newDelegate, NO);
}

RSExport dispatch_queue_t RSConnectionDelegateQueue(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
    {
        return connection->delegateQueue;
    }
    else
    {
        __block dispatch_queue_t result;
        dispatch_sync(connection->socketQueue, ^{
            result = connection->delegateQueue;
        });
        return result;
    }
}

RSExport void RSConnectionSetDelegateQueueWithMode(RSConnectionRef connection, dispatch_queue_t newDelegateQueue, BOOL synchronously)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    dispatch_block_t block = ^{
#if !OS_OBJECT_USE_OBJC
		if (connection->delegateQueue) dispatch_release(connection->delegateQueue);
		if (newDelegateQueue) dispatch_retain(newDelegateQueue);
#endif
    };
    
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey)) {
		block();
	}
	else {
		if (synchronously)
			dispatch_sync(connection->socketQueue, block);
		else
			dispatch_async(connection->socketQueue, block);
	}
}

RSExport void RSConnectionSetDelegateQueueSync(RSConnectionRef connection, dispatch_queue_t newDelegateQueue)
{
    return RSConnectionSetDelegateQueueWithMode(connection, newDelegateQueue, YES);
}

RSExport void RSConnectionSetDelegateQueue(RSConnectionRef connection, dispatch_queue_t newDelegateQueue)
{
    return RSConnectionSetDelegateQueueWithMode(connection, newDelegateQueue, NO);
}

RSExport void RSConnectionGetDelegateAndQueue(RSConnectionRef connection, ISA *delegatePtr, dispatch_queue_t* delegateQueuePtr)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
	{
		if (delegatePtr) *delegatePtr = connection->delegate;
		if (delegateQueuePtr) *delegateQueuePtr = connection->delegateQueue;
	}
	else
	{
		__block ISA dPtr = NULL;
		__block dispatch_queue_t dqPtr = NULL;
		
		dispatch_sync(connection->socketQueue, ^{
			dPtr = connection->delegate;
			dqPtr = connection->delegateQueue;
		});
		
		if (delegatePtr) *delegatePtr = dPtr;
		if (delegateQueuePtr) *delegateQueuePtr = dqPtr;
	}
}

RSExport void RSConnectionSetDelegateAndQueueWithMode(RSConnectionRef connection, ISA newDelegate, dispatch_queue_t newDelegateQueue, BOOL synchronously)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    dispatch_block_t block = ^{
		
		connection->delegate = newDelegate;
		
#if !OS_OBJECT_USE_OBJC
		if (connection->delegateQueue) dispatch_release(connection->delegateQueue);
		if (newDelegateQueue) dispatch_retain(newDelegateQueue);
#endif
		
		connection->delegateQueue = newDelegateQueue;
	};
	
	if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey)) {
		block();
	}
	else {
		if (synchronously)
			dispatch_sync(connection->socketQueue, block);
		else
			dispatch_async(connection->socketQueue, block);
	}
}

RSExport void RSConnectionSetDelegateAndQueueSync(RSConnectionRef connection, ISA newDelegate, dispatch_queue_t newDelegateQueue)
{
    return RSConnectionSetDelegateAndQueueWithMode(connection, newDelegate, newDelegateQueue, YES);
}

RSExport void RSConnectionSetDelegateAndQueue(RSConnectionRef connection, ISA newDelegate, dispatch_queue_t newDelegateQueue)
{
    return RSConnectionSetDelegateAndQueueWithMode(connection, newDelegate, newDelegateQueue, NO);
}

RSExport BOOL RSConnectionIsIPv4Enabled(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
	{
		return ((connection->config & kIPv4Disabled) == 0);
	}
	else
	{
		__block BOOL result;
		dispatch_sync(connection->socketQueue, ^{
			result = ((connection->config & kIPv4Disabled) == 0);
		});
		return result;
	}
}

RSExport void RSConnectionSetIPv4Enabled(RSConnectionRef connection, BOOL enabled)
{
    // Note: YES means kIPv4Disabled is OFF
	__RSGenericValidInstance(connection, _RSConnectionTypeID);
	__RSConnectionDoBlockAsync(connection, ^{
		if (enabled)
			connection->config &= ~kIPv4Disabled;
		else
			connection->config |= kIPv4Disabled;
	});
}

RSExport BOOL RSConnectionIsIPv6Enabled(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
	{
		return ((connection->config & kIPv6Disabled) == 0);
	}
	else
	{
		__block BOOL result;
		dispatch_sync(connection->socketQueue, ^{
			result = ((connection->config & kIPv6Disabled) == 0);
		});
		return result;
	}
}

RSExport void RSConnectionSetIPv6Enabled(RSConnectionRef connection, BOOL enabled)
{
    // Note: YES means kIPv4Disabled is OFF
	__RSGenericValidInstance(connection, _RSConnectionTypeID);
	__RSConnectionDoBlockAsync(connection, ^{
		if (enabled)
			connection->config &= ~kIPv6Disabled;
		else
			connection->config |= kIPv6Disabled;
	});
}

RSExport BOOL RSConnectionIsIPv4PreferredOverIPv6(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
		return ((connection->config & kPreferIPv6) == 0);
	else
	{
		__block BOOL result;
		dispatch_sync(connection->socketQueue, ^{
			result = ((connection->config & kPreferIPv6) == 0);
		});
		return result;
	}
}

RSExport void RSConnectionSetIPv4PreferredOverIPv6(RSConnectionRef connection, BOOL flag)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
	__RSConnectionDoBlockAsync(connection, ^{
        if (flag)
			connection->config &= ~kPreferIPv6;
		else
			connection->config |= kPreferIPv6;
    });
}

RSExport RSTypeRef RSConnectionGetUserData(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    __block RSTypeRef result = nil;
    __RSConnectionDoBlock(connection, ^{
        result = connection->userData;
    });
    return result;
}

RSExport void RSConnectionSetUserData(RSConnectionRef connection, RSTypeRef userData)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    __RSConnectionDoBlockAsync(connection, ^{
        if (connection->userData != userData)
		{
			connection->userData = RSRetain(userData);
		}
    });
}

static RSErrorRef __RSConnectionBadConfigError(RSStringRef msg)
{
    return RSAutorelease(RSErrorCreate(RSAllocatorSystemDefault, RSConnectionErrorDomain, RSConnectionBadConfigError, RSDictionaryWithObjectForKey(msg, RSErrorLocalizedDescriptionKey)));
}

static RSErrorRef __RSConnectionBadParamError(RSStringRef msg)
{
    return RSAutorelease(RSErrorCreate(RSAllocatorSystemDefault, RSConnectionErrorDomain, RSConnectionBadParamError, RSDictionaryWithObjectForKey(msg, RSErrorLocalizedDescriptionKey)));
}

static RSErrorRef __RSConnectionGaiError(RSErrorCode gaiCode)
{
    return RSErrorWithDomainCodeAndUserInfo(RSSTR("kCFStreamErrorDomainNetDB"), gaiCode, RSDictionaryWithObjectForKey(RSStringWithCString(gai_strerror(gaiCode), RSStringEncodingUTF8), RSErrorLocalizedDescriptionKey));
}

static RSErrorRef __RSConnectionErrnoError()
{
    return RSErrorWithDomainCodeAndUserInfo(RSErrorDomainPOSIX, errno, RSDictionaryWithObjectForKey(RSStringWithUTF8String((UTF8Char *)strerror(errno)), RSErrorLocalizedDescriptionKey));
}

RSExport BOOL RSConnectionIsDisconnected(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    __block BOOL result = NO;
    __RSConnectionDoBlock(connection, ^{
        result = (connection->flags & kSocketStarted) ? NO : YES;
    });
    return result;
}

RSExport BOOL RSConnectionIsConnected(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    __block BOOL result = NO;
    __RSConnectionDoBlock(connection, ^{
        result = (connection->flags & kConnected) ? YES : NO;
    });
    return result;
}

RSExport RSStringRef RSConnectionGetConnectedHostFromSocket4(RSSocketHandle sfd)
{
    struct sockaddr_in sockaddr4;
	socklen_t sockaddr4len = sizeof(sockaddr4);
	
	if (getpeername(sfd, (struct sockaddr *)&sockaddr4, &sockaddr4len) < 0)
	{
		return nil;
	}
    return RSConnectionGetHostFromSockaddr4(&sockaddr4);
}

RSExport RSStringRef RSConnectionGetConnectedHostFromSocket6(RSSocketHandle sfd)
{
    struct sockaddr_in6 sockaddr6;
	socklen_t sockaddr6len = sizeof(sockaddr6);
	
	if (getpeername(sfd, (struct sockaddr *)&sockaddr6, &sockaddr6len) < 0)
	{
		return nil;
	}
    return RSConnectionGetHostFromSockaddr6(&sockaddr6);
}

RSExport RSStringRef RSConnectionConnectedHost(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
	{
		if (connection->socket4FD != SOCKET_NULL)
			return RSConnectionGetConnectedHostFromSocket4(connection->socket4FD);
		if (connection->socket6FD != SOCKET_NULL)
			return RSConnectionGetConnectedHostFromSocket6(connection->socket6FD);
		
		return nil;
	}
	else
	{
		__block RSStringRef result = nil;
		
		dispatch_sync(connection->socketQueue, ^{
            RSAutoreleaseBlock(^{
                if (connection->socket4FD != SOCKET_NULL)
                    result = RSRetain(RSConnectionGetConnectedHostFromSocket4(connection->socket4FD));
                if (connection->socket6FD != SOCKET_NULL)
                    result = RSRetain(RSConnectionGetConnectedHostFromSocket6(connection->socket6FD));
            });
		});
		return RSAutorelease(result);
	}
}

RSExport RSBitU16 RSConnectionConnectedPortFromSocket4(RSSocketHandle sfd)
{
    struct sockaddr_in sockaddr4;
    socklen_t sockaddr4len = sizeof(sockaddr4);
    if (getpeername(sfd, (struct sockaddr *)&sockaddr4, &sockaddr4len) < 0)
	{
		return 0;
	}
    return RSConnectionGetPortFromSockaddr4(&sockaddr4);
}

RSExport RSBitU16 RSConnectionConnectedPortFromSocket6(RSSocketHandle sfd)
{
    struct sockaddr_in6 sockaddr6;
	socklen_t sockaddr6len = sizeof(sockaddr6);
	if (getpeername(sfd, (struct sockaddr *)&sockaddr6, &sockaddr6len) < 0)
	{
		return 0;
	}
    return RSConnectionGetPortFromSockaddr6(&sockaddr6);
}

RSExport RSBitU16 RSConnectionConnectedPort(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
	{
		if (connection->socket4FD != SOCKET_NULL)
			return RSConnectionConnectedPortFromSocket4(connection->socket4FD);
		if (connection->socket6FD != SOCKET_NULL)
			return RSConnectionConnectedPortFromSocket6(connection->socket6FD);
		
		return 0;
	}
	else
	{
		__block RSBitU16 result = 0;
		dispatch_sync(connection->socketQueue, ^{
			// No need for autorelease pool
			if (connection->socket4FD != SOCKET_NULL)
                result = RSConnectionConnectedPortFromSocket4(connection->socket4FD);
            if (connection->socket6FD != SOCKET_NULL)
                result = RSConnectionConnectedPortFromSocket6(connection->socket6FD);
		});
		return result;
	}
}

RSExport RSStringRef RSConnectionGetLocalHostFromSocket4(RSSocketHandle sfd)
{
    struct sockaddr_in sockaddr4;
	socklen_t sockaddr4len = sizeof(sockaddr4);
	if (getsockname(sfd, (struct sockaddr *)&sockaddr4, &sockaddr4len) < 0)
	{
		return nil;
	}
    return RSConnectionGetHostFromSockaddr4(&sockaddr4);
}

RSExport RSStringRef RSConnectionGetLocalHostFromSocket6(RSSocketHandle sfd)
{
    struct sockaddr_in6 sockaddr6;
	socklen_t sockaddr6len = sizeof(sockaddr6);
	
	if (getsockname(sfd, (struct sockaddr *)&sockaddr6, &sockaddr6len) < 0)
	{
		return nil;
	}
    return RSConnectionGetHostFromSockaddr6(&sockaddr6);
}

RSExport RSStringRef RSConnectionGetLocalHost(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
	{
		if (connection->socket4FD != SOCKET_NULL)
			return RSConnectionGetLocalHostFromSocket4(connection->socket4FD);
		if (connection->socket6FD != SOCKET_NULL)
			return RSConnectionGetLocalHostFromSocket6(connection->socket6FD);
		
		return nil;
	}
	else
	{
		__block RSStringRef result = nil;
		
		dispatch_sync(connection->socketQueue, ^{
            RSAutoreleaseBlock(^{
                if (connection->socket4FD != SOCKET_NULL)
                    result = RSRetain(RSConnectionGetLocalHostFromSocket4(connection->socket4FD));
                else if (connection->socket6FD != SOCKET_NULL)
                    result = RSRetain(RSConnectionGetLocalHostFromSocket6(connection->socket6FD));
            });
        });
		
		return RSAutorelease(result);
	}
}

RSExport RSBitU16 RSConnectionGetLocalPortFromSocket4(RSSocketHandle sfd)
{
    struct sockaddr_in sockaddr4;
	socklen_t sockaddr4len = sizeof(sockaddr4);
	
	if (getsockname(sfd, (struct sockaddr *)&sockaddr4, &sockaddr4len) < 0)
	{
		return 0;
	}
    return RSConnectionGetPortFromSockaddr4(&sockaddr4);
}

RSExport RSBitU16 RSConnectionGetLocalPortFromSocket6(RSSocketHandle sfd)
{
    struct sockaddr_in6 sockaddr6;
	socklen_t sockaddr6len = sizeof(sockaddr6);
	
	if (getsockname(sfd, (struct sockaddr *)&sockaddr6, &sockaddr6len) < 0)
	{
		return 0;
	}
    return RSConnectionGetPortFromSockaddr6(&sockaddr6);
}

RSExport RSBitU16 RSConnectionGetLocalPort(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (dispatch_get_specific(connection->IsOnSocketQueueOrTargetQueueKey))
	{
		if (connection->socket4FD != SOCKET_NULL)
			return RSConnectionGetLocalPortFromSocket4(connection->socket4FD);
		if (connection->socket6FD != SOCKET_NULL)
			return RSConnectionGetLocalPortFromSocket6(connection->socket6FD);
		
		return 0;
	}
	else
	{
		__block RSBitU16 result = 0;
		
		dispatch_sync(connection->socketQueue, ^{
			if (connection->socket4FD != SOCKET_NULL)
				result = RSConnectionGetLocalPortFromSocket4(connection->socket4FD);
			else if (connection->socket6FD != SOCKET_NULL)
				result = RSConnectionGetLocalPortFromSocket6(connection->socket6FD);
		});
		return result;
	}
}

RSExport RSStringRef RSConnectionGetConnectedHost4(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (connection->socket4FD != SOCKET_NULL) return RSConnectionGetConnectedHostFromSocket4(connection->socket4FD);
    return nil;
}

RSExport RSStringRef RSConnectionGetConnectedHost6(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (connection->socket6FD != SOCKET_NULL) return RSConnectionGetConnectedHostFromSocket6(connection->socket6FD);
    return nil;
}

RSExport RSBitU16 RSConnectionGetConnectedPort4(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (connection->socket4FD != SOCKET_NULL) return RSConnectionGetConnectedHostFromSocket4(connection->socket4FD);
    return 0;
}

RSExport RSBitU16 RSConnectionGetConnectedPort6(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (connection->socket6FD != SOCKET_NULL) return RSConnectionGetConnectedHostFromSocket6(connection->socket6FD);
    return 0;
}

RSExport RSStringRef RSConnectionGetLocalHost4(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (connection->socket4FD != SOCKET_NULL) return RSConnectionGetLocalHostFromSocket4(connection->socket4FD);
    return nil;
}

RSExport RSStringRef RSConnectionGetLocalHost6(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (connection->socket6FD != SOCKET_NULL) return RSConnectionGetLocalHostFromSocket6(connection->socket6FD);
    return nil;
}

RSExport RSBitU16 RSConnectionGetLocalPort4(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (connection->socket4FD != SOCKET_NULL) return RSConnectionGetLocalPortFromSocket4(connection->socket4FD);
    return 0;
}

RSExport RSBitU16 RSConnectionGetLocalPort6(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    if (connection->socket6FD != SOCKET_NULL) return RSConnectionGetLocalPortFromSocket6(connection->socket6FD);
    return 0;
}

RSExport RSDataRef RSConnectionGetConnectedAddress(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    __block RSDataRef result = nil;
    __RSConnectionDoBlock(connection, ^{
        if (connection->socket4FD != SOCKET_NULL)
        {
            struct sockaddr_in sockaddr4;
			socklen_t sockaddr4len = sizeof(sockaddr4);
			
			if (getpeername(connection->socket4FD, (struct sockaddr *)&sockaddr4, &sockaddr4len) == 0)
			{
                result = RSDataWithBytes(&sockaddr4, sockaddr4len);
            }
        }
        
        if (connection->socket6FD != SOCKET_NULL)
        {
            struct sockaddr_in6 sockaddr6;
			socklen_t sockaddr6len = sizeof(sockaddr6);
			
			if (getpeername(connection->socket6FD, (struct sockaddr *)&sockaddr6, &sockaddr6len) == 0)
			{
				result = RSDataWithBytes(&sockaddr6, sockaddr6len);
			}
        }
    });
    return result;
}

RSExport RSDataRef RSConnectionGetLocalAddress(RSConnectionRef connection)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    __block RSDataRef result = nil;
    __RSConnectionDoBlock(connection, ^{
        if (connection->socket4FD != SOCKET_NULL)
        {
            struct sockaddr_in sockaddr4;
			socklen_t sockaddr4len = sizeof(sockaddr4);
			
			if (getsockname(connection->socket4FD, (struct sockaddr *)&sockaddr4, &sockaddr4len) == 0)
			{
				result = RSDataWithBytes(&sockaddr4, sockaddr4len);
			}
        }
        if (connection->socket6FD != SOCKET_NULL)
		{
			struct sockaddr_in6 sockaddr6;
			socklen_t sockaddr6len = sizeof(sockaddr6);
			
			if (getsockname(connection->socket6FD, (struct sockaddr *)&sockaddr6, &sockaddr6len) == 0)
			{
				result = RSDataWithBytes(&sockaddr6, sockaddr6len);
			}
		}
    });
    return result;
}

RSExport BOOL RSConnectionIsIPv4(RSConnectionRef connection)
{
    __block BOOL result = NO;
    __RSConnectionDoBlock(connection, ^{
        result = (connection->socket4FD != SOCKET_NULL);
    });
    return result;
}

RSExport BOOL RSConnectionIsIPv6(RSConnectionRef connection)
{
    __block BOOL result = NO;
    __RSConnectionDoBlock(connection, ^{
        result = (connection->socket6FD != SOCKET_NULL);
    });
    return result;
}

RSExport BOOL RSConnectionIsSecure(RSConnectionRef connection)
{
    __block BOOL result = NO;
    __RSConnectionDoBlock(connection, ^{
        result = (connection->flags & kSocketSecure) ? YES : NO;
    });
    return result;
}

RSExport void RSConnectionGetInterfaceInformation(RSMutableDataRef *interfaceAddress4Ptr, RSMutableDataRef *interfaceAddress6Ptr, RSStringRef interfaceDescription, RSBitU16 port)
{
    RSMutableDataRef addr4 = nil, addr6 = nil;
    RSStringRef interface = nil;
    RSArrayRef components = RSStringCreateArrayBySeparatingStrings(RSAllocatorSystemDefault, interfaceDescription, RSSTR(":"));
    
    if (RSArrayGetCount(components) > 0)
    {
        RSStringRef temp = RSArrayObjectAtIndex(components, 0);
        if (RSStringGetLength(temp) > 0)
        {
//            interface = RSRetain(temp);
//            RSRelease(temp);
            interface = temp;
        }
    }
    
    if (RSArrayGetCount(components) > 1 && port == 0)
    {
        long portL = RSStringLongValue(RSArrayObjectAtIndex(components, 1));
        if (portL > 0 && portL < UINT16_MAX)
        {
            port = (RSBitU16)portL;
        }
    }
    
    if (interface == nil)
    {
        struct sockaddr_in sockaddr4;
        memset(&sockaddr4, 0, sizeof(struct sockaddr_in));
        
        sockaddr4.sin_len         = sizeof(sockaddr4);
		sockaddr4.sin_family      = AF_INET;
		sockaddr4.sin_port        = htons(port);
		sockaddr4.sin_addr.s_addr = htonl(INADDR_ANY);
		
		struct sockaddr_in6 sockaddr6;
		memset(&sockaddr6, 0, sizeof(sockaddr6));
		
		sockaddr6.sin6_len       = sizeof(sockaddr6);
		sockaddr6.sin6_family    = AF_INET6;
		sockaddr6.sin6_port      = htons(port);
		sockaddr6.sin6_addr      = in6addr_any;
        
        addr4 = RSMutableDataWithBytes(&sockaddr4, sizeof(sockaddr4));
        addr6 = RSMutableDataWithBytes(&sockaddr6, sizeof(sockaddr6));
    }
    
    else if (RSEqual(interface, RSSTR("localhost")) ||
             RSEqual(interface, RSSTR("loopback")))
    {
        // LOOPBACK address
        struct sockaddr_in sockaddr4;
		memset(&sockaddr4, 0, sizeof(sockaddr4));
		
		sockaddr4.sin_len         = sizeof(sockaddr4);
		sockaddr4.sin_family      = AF_INET;
		sockaddr4.sin_port        = htons(port);
		sockaddr4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		
		struct sockaddr_in6 sockaddr6;
		memset(&sockaddr6, 0, sizeof(sockaddr6));
		
		sockaddr6.sin6_len       = sizeof(sockaddr6);
		sockaddr6.sin6_family    = AF_INET6;
		sockaddr6.sin6_port      = htons(port);
		sockaddr6.sin6_addr      = in6addr_loopback;
        
        addr4 = RSMutableDataWithBytes(&sockaddr4, sizeof(sockaddr4));
        addr6 = RSMutableDataWithBytes(&sockaddr6, sizeof(sockaddr6));
    }
    else
    {
        const char *iface = RSStringGetCStringPtr(interface, RSStringEncodingASCII);
        struct ifaddrs *addrs;
        const struct ifaddrs *cursor = nil;
        
        if (getifaddrs(&addrs))
        {
            cursor = addrs;
			while (cursor != NULL)
			{
				if ((addr4 == nil) && (cursor->ifa_addr->sa_family == AF_INET))
				{
					// IPv4
					
					struct sockaddr_in nativeAddr4;
					memcpy(&nativeAddr4, cursor->ifa_addr, sizeof(nativeAddr4));
					
					if (strcmp(cursor->ifa_name, iface) == 0)
					{
						// Name match
						
						nativeAddr4.sin_port = htons(port);
                        addr4 = RSMutableDataWithBytes(&nativeAddr4, sizeof(nativeAddr4));
                    }
                    else
                    {
                        char ip[INET_ADDRSTRLEN];
						
						const char *conversion = inet_ntop(AF_INET, &nativeAddr4.sin_addr, ip, sizeof(ip));
						
						if ((conversion != NULL) && (strcmp(ip, iface) == 0))
						{
							// IP match
							
							nativeAddr4.sin_port = htons(port);
							
							addr4 = RSMutableDataWithBytes(&nativeAddr4, sizeof(nativeAddr4));
						}
                    }
                }
                else if ((addr6 == nil) && (cursor->ifa_addr->sa_family == AF_INET6))
                {
                    struct sockaddr_in6 nativeAddr6;
					memcpy(&nativeAddr6, cursor->ifa_addr, sizeof(nativeAddr6));
					
					if (strcmp(cursor->ifa_name, iface) == 0)
					{
						// Name match
						
						nativeAddr6.sin6_port = htons(port);
						
						addr6 = RSMutableDataWithBytes(&nativeAddr6, sizeof(nativeAddr6));
					}
					else
					{
						char ip[INET6_ADDRSTRLEN];
						
						const char *conversion = inet_ntop(AF_INET6, &nativeAddr6.sin6_addr, ip, sizeof(ip));
						
						if ((conversion != NULL) && (strcmp(ip, iface) == 0))
						{
							// IP match
							
							nativeAddr6.sin6_port = htons(port);
							
							addr6 = RSMutableDataWithBytes(&nativeAddr6, sizeof(nativeAddr6));
						}
					}
                }
                
                cursor = cursor->ifa_next;
            }
            freeifaddrs(addrs);
        }
    }
    
    if (interfaceAddress4Ptr) *interfaceAddress4Ptr = addr4;
    if (interfaceAddress6Ptr) *interfaceAddress6Ptr = addr6;
    
    if (interface) RSRelease(interface);
    
    if (components) RSRelease(components);
}

RSExport BOOL RSConnectionDoAccept(RSConnectionRef connection, RSSocketHandle parentSfd)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    BOOL isIPv4 = NO;
    RSSocketHandle childSfd = SOCKET_NULL;
    RSDataRef childSocketAddress = nil;
    
    if (parentSfd == connection->socket4FD)
    {
        isIPv4 = YES;
		
		struct sockaddr_in addr;
		socklen_t addrLen = sizeof(addr);
		
		childSfd = accept(parentSfd, (struct sockaddr *)&addr, &addrLen);
		
		if (childSfd == SOCKET_NULL)
		{
			__RSLog(RSLogLevelNotice, RSSTR("Accept failed with error: %r"), __RSConnectionErrnoError());
			return NO;
		}
        
        childSocketAddress = RSDataWithBytes(&addr, addrLen);
    }
    else
    {
        isIPv4 = NO;
        struct sockaddr_in6 addr;
		socklen_t addrLen = sizeof(addr);
        
		childSfd = accept(parentSfd, (struct sockaddr *)&addr, &addrLen);
        
		if (childSfd == SOCKET_NULL)
		{
			__RSLog(RSLogLevelNotice, RSSTR("Accept failed with error: %r"), __RSConnectionErrnoError());
			return NO;
		}
        
        childSocketAddress = RSDataWithBytes(&addr, addrLen);
    }
    
    RSErrorCode result = fcntl(childSfd, F_SETFL, O_NONBLOCK);
    if (result == -1)
    {
        __RSCLog(RSLogLevelNotice, "Error enabling non-blocking IO on accepted socket (fcntl)");
        close(childSfd);
        return NO;
    }
    
    int nosigpipe = 1;
    setsockopt(childSfd, SOL_SOCKET, SO_NOSIGPIPE, &nosigpipe, sizeof(nosigpipe));
    
    if (connection->delegate)
    {
        // create a new RSConnection
    }
    return YES;
}

RSExport BOOL RSConnectionAcceptOnInterface(RSConnectionRef connection, RSStringRef interface, RSBitU16 port, RSErrorRef *error)
{
    __RSGenericValidInstance(connection, _RSConnectionTypeID);
    RSStringRef _interface = RSCopy(RSAllocatorSystemDefault, interface);
    __block BOOL result = NO;
	__block RSErrorRef err = nil;
    
    RSSocketHandle (^createSocket)(RSBit32, RSDataRef) = ^RSSocketHandle (RSBit32 domain, RSDataRef interfaceAddress) {
        RSSocketHandle sfd = socket(domain, SOCK_STREAM, 0);
        if (sfd == SOCKET_NULL)
        {
            err = RSErrorWithReason(RSSTR("Error in socket() function"));
            return SOCKET_NULL;
        }
        
        RSErrorCode status = kSuccess;
        status = fcntl(sfd, F_SETFL, O_NONBLOCK);
        
        if (status == -1) {
            err = RSErrorWithReason(RSSTR("Error enabling non-blocking IO on socket (fcntl)"));
            close(sfd);
            return SOCKET_NULL;
        }
        
        int reuseOn = YES;
        status = setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuseOn, sizeof(reuseOn));
        if (status == -1) {
            err = RSErrorWithReason(RSSTR("Error enabling address reuse (setsockopt)"));
            close(sfd);
            return SOCKET_NULL;
        }
        
        status = bind(sfd, (const struct sockaddr *)RSDataGetBytesPtr(interfaceAddress), (socklen_t)RSDataGetLength(interfaceAddress));
        if (status == -1)
        {
            err = RSErrorWithReason(RSSTR("Error in bind() function"));
            close(sfd);
            return SOCKET_NULL;
        }
        
        status = listen(sfd, 1024);
        if (status == -1)
        {
            err = RSErrorWithReason(RSSTR("Error in listen() function"));
            close(sfd);
            return SOCKET_NULL;
        }
        
        return sfd;
    };
    
    dispatch_block_t block = ^{
        RSAutoreleaseBlock(^{
            if (connection->delegate == nil)
            {
                err = (RSErrorRef)RSRetain(__RSConnectionBadConfigError(RSSTR("Attempting to accept without a delegate. Set a delegate first.")));
                return;
            }
            
            if (connection->delegateQueue == nil)
            {
                err = (RSErrorRef)RSRetain(__RSConnectionBadConfigError(RSSTR("Attempting to accept without a delegate queue. Set a delegate queue first.")));
                return;
            }
            
            BOOL isIPv4Disabled = (connection->config & kIPv4Disabled) ? YES : NO;
            BOOL isIPv6Disabled = (connection->config & kIPv6Disabled) ? YES : NO;
            
            if (isIPv4Disabled && isIPv6Disabled)
            {
                err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("Both IPv4 and IPv6 have been disabled. Must enable at least one protocol first.")));
                return;
            }
            
            if (!RSConnectionIsDisconnected(connection))
            {
                err = (RSErrorRef)RSRetain(__RSConnectionBadConfigError(RSSTR("Attempting to accept while connected or accepting connections. Disconnect first.")));
                return;
            }
            
            RSArrayRemoveAllObjects(connection->readQueue);
            RSArrayRemoveAllObjects(connection->writeQueue);
            
            RSMutableDataRef interface4 = nil;
            RSMutableDataRef interface6 = nil;
            
            RSConnectionGetInterfaceInformation(&interface4, &interface6, interface, port);
            
            if ((interface4 == nil) && (interface6 == nil))
            {
                err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("Unknown interface. Specify valid interface by name (e.g. \"en1\") or IP address.")));
                return;
            }
            
            if (isIPv4Disabled && (interface6 == nil))
            {
                err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("IPv4 has been disabled and specified interface doesn't support IPv6.")));
                return;
            }
            
            if (isIPv6Disabled && (interface4 == nil))
            {
                err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("IPv6 has been disabled and specified interface doesn't support IPv4.")));
                return;
            }
            
            BOOL enableIPv4 = !isIPv4Disabled && (interface4 != nil);
            BOOL enableIPv6 = !isIPv6Disabled && (interface6 != nil);
            
            // Create sockets, configure, bind, and listen
            
            if (enableIPv4)
            {
                
                connection->socket4FD = createSocket(AF_INET, interface4);
                
                if (connection->socket4FD == SOCKET_NULL)
                {
                    return;
                }
            }
            
            if (enableIPv6)
            {
                if (enableIPv4 && (port == 0))
                {
                    // No specific port was specified, so we allowed the OS to pick an available port for us.
                    // Now we need to make sure the IPv6 socket listens on the same port as the IPv4 socket.
                    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)RSDataCopyBytes(interface6);
                
                    addr6->sin6_port = htons(RSConnectionGetLocalPort4(connection));
                }
                
                connection->socket6FD = createSocket(AF_INET6, interface6);
                
                if (connection->socket6FD == SOCKET_NULL)
                {
                    if (connection->socket4FD != SOCKET_NULL)
                    {
                        close(connection->socket4FD);
                    }
                    return;
                }
            }
            
            if (enableIPv4)
            {
                connection->accept4Source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, connection->socket4FD, 0, connection->socketQueue);
                RSSocketHandle sfd = connection->socket4FD;
                dispatch_source_t acceptSource = connection->accept4Source;
                dispatch_source_set_event_handler(acceptSource, ^{
                    RSAutoreleaseBlock(^{
                        unsigned long i = 0;
                        unsigned long numPendingConnections = dispatch_source_get_data(acceptSource);
                        //[self doAccept:socketFD]
                        __RSCLog(RSLogLevelNotice, "numPendingConnections: %lu", numPendingConnections);
                        while (1 && (++i < numPendingConnections));
                    });
                });
                
                dispatch_source_set_cancel_handler(connection->accept4Source, ^{
#if !OS_OBJECT_USE_OBJC
                    dispatch_release(acceptSource);
#endif
                    __RSCLog(RSLogLevelNotice, "close(socket4FD)");
                    close(sfd);
                });
                
                __RSCLog(RSLogLevelNotice, "resume accept4Source");
                dispatch_resume(connection->accept4Source);
            }
            
            if (enableIPv6)
            {
                connection->accept6Source = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, connection->socket6FD, 0, connection->socketQueue);
                RSSocketHandle sfd = connection->socket6FD;
                dispatch_source_t acceptSource = connection->accept6Source;
                
                dispatch_source_set_event_handler(connection->accept6Source, ^{
                    RSAutoreleaseBlock(^{
                        unsigned long i = 0;
                        unsigned long numPendingConnections = dispatch_source_get_data(acceptSource);
                        
                        __RSCLog(RSLogLevelNotice, "numPendingConnections: %lu", numPendingConnections);
                        //[self doAccept:socketFD]
                        while (1 && (++i < numPendingConnections));
                    });
                });
                
                dispatch_source_set_cancel_handler(connection->accept6Source, ^{
#if !OS_OBJECT_USE_OBJC
                    __RSCLog(RSLogLevelNotice, "dispatch_release(accept6Source)");
                    dispatch_release(acceptSource);
#endif
                    
                    __RSCLog(RSLogLevelNotice, "close(socket6FD)");
                    close(sfd);
                });
                
                __RSCLog(RSLogLevelNotice, "dispatch_resume(accept6Source)");
                dispatch_resume(connection->accept6Source);
            }
            
            connection->flags |= kSocketStarted;
            
            result = YES;
        });
        if (err) RSAutorelease(err);
    };
    
    __RSConnectionDoBlock(connection, block);
    if (result == NO) {
        RSLog(RSSTR("%r"), err);
        if (error) *error = err;
    }
    RSRelease(_interface);
    return result;
}

RSExport void RSConnectionEndConnectTimeout(RSConnectionRef connection)
{
	if (connection->connectTimer)
	{
		dispatch_source_cancel(connection->connectTimer);
		connection->connectTimer = NULL;
	}
	
	// Increment connectIndex.
	// This will prevent us from processing results from any related background asynchronous operations.
	//
	// Note: This should be called from close method even if connectTimer is NULL.
	// This is because one might disconnect a socket prior to a successful connection which had no timeout.
	
	connection->connectIndex++;
	
	if (connection->connectInterface4)
	{
		RSRelease(connection->connectInterface4);
		connection->connectInterface4 = nil;
	}
	if (connection->connectInterface6)
	{
		RSRelease(connection->connectInterface6);
		connection->connectInterface6 = nil;
	}
}

RSExport void RSConnectionEndCurrentRead(RSConnectionRef connection)
{
	if (connection->readTimer)
	{
		dispatch_source_cancel(connection->readTimer);
		connection->readTimer = NULL;
	}
	
    RSRelease(connection->currentRead);
	connection->currentRead = nil;
}

RSExport void RSConnectionEndCurrentWrite(RSConnectionRef connection)
{
	if (connection->writeTimer)
	{
		dispatch_source_cancel(connection->writeTimer);
		connection->writeTimer = NULL;
	}
	
    RSRelease(connection->currentWrite);
	connection->currentWrite = nil;
}

RSExport void RSConnectionSuspendReadSource(RSConnectionRef connection)
{
	if (!(connection->flags & kReadSourceSuspended))
	{
		dispatch_suspend(connection->readSource);
		connection->flags |= kReadSourceSuspended;
	}
}

RSExport void RSConnectionResumeReadSource(RSConnectionRef connection)
{
	if (connection->flags & kReadSourceSuspended)
	{
		dispatch_resume(connection->readSource);
		connection->flags &= ~kReadSourceSuspended;
	}
}

RSExport void RSConnectionSuspendWriteSource(RSConnectionRef connection)
{
	if (!(connection->flags & kWriteSourceSuspended))
	{
		dispatch_suspend(connection->writeSource);
		connection->flags |= kWriteSourceSuspended;
	}
}

RSExport void RSConnectionResumeWriteSource(RSConnectionRef connection)
{
	if (connection->flags & kWriteSourceSuspended)
	{
		dispatch_resume(connection->writeSource);
		connection->flags &= ~kWriteSourceSuspended;
	}
}


RSExport void RSConnectionCloseWithError(RSConnectionRef connection, RSErrorRef error)
{
	if (dispatch_get_current_queue() == connection->socketQueue)
    {
        RSConnectionEndConnectTimeout(connection);
        if (connection->currentRead != nil)
            RSConnectionEndCurrentRead(connection);
        if (connection->currentWrite != nil)
            RSConnectionEndCurrentWrite(connection);
        RSArrayRemoveAllObjects(connection->readQueue);
        RSArrayRemoveAllObjects(connection->writeQueue);
        __RSConnectionPreBufferReset(connection->preBuffer);
        // For some crazy reason (in my opinion), cancelling a dispatch source doesn't
        // invoke the cancel handler if the dispatch source is paused.
        // So we have to unpause the source if needed.
        // This allows the cancel handler to be run, which in turn releases the source and closes the socket.
        
        if (!connection->accept4Source && !connection->accept6Source && !connection->readSource && !connection->writeSource)
        {
            if (connection->socket4FD != SOCKET_NULL)
            {
                close(connection->socket4FD);
                connection->socket4FD = SOCKET_NULL;
            }
            
            if (connection->socket6FD != SOCKET_NULL)
            {
                close(connection->socket6FD);
                connection->socket6FD = SOCKET_NULL;
            }
        }
        else
        {
            if (connection->accept4Source)
            {
                dispatch_source_cancel(connection->accept4Source);
                // We never suspend accept4Source
                connection->accept4Source = NULL;
            }
            
            if (connection->accept6Source)
            {
                dispatch_source_cancel(connection->accept6Source);
                // We never suspend accept6Source
                connection->accept6Source = NULL;
            }
            
            if (connection->readSource)
            {
                dispatch_source_cancel(connection->readSource);
                RSConnectionResumeReadSource(connection);
                connection->readSource = NULL;
            }
            
            if (connection->writeSource)
            {
                dispatch_source_cancel(connection->writeSource);
                RSConnectionResumeWriteSource(connection);
                connection->writeSource = NULL;
            }
            
            // The sockets will be closed by the cancel handlers of the corresponding source
            connection->socket4FD = SOCKET_NULL;
            connection->socket6FD = SOCKET_NULL;
        }
        // If the client has passed the connect/accept method, then the connection has at least begun.
        // Notify delegate that it is now ending.
        BOOL shouldCallDelegate = (connection->flags & kSocketStarted);
        
        // Clear stored socket info and all flags (config remains as is)
        connection->socketFDBytesAvailable = 0;
        connection->flags = 0;
        
        if (shouldCallDelegate)
        {
            
//            if (connection->delegateQueue && [delegate respondsToSelector: @selector(socketDidDisconnect:withError:)])
//            {
//                __strong id theDelegate = delegate;
//                
//                dispatch_async(delegateQueue, ^{ @autoreleasepool {
//                    
//                    [theDelegate socketDidDisconnect:self withError:error];
//                }});
//            }	
        }
    }
}
/**
 * This method is called if the DNS lookup fails.
 * This method is executed on the socketQueue.
 *
 * Since the DNS lookup executed synchronously on a global concurrent queue,
 * the original connection request may have already been cancelled or timed-out by the time this method is invoked.
 * The lookupIndex tells us whether the lookup is still valid or not.
 **/


RSExport void RSConnectionLookupDidFail(RSConnectionRef connection, int connectIndex, RSErrorRef error)
{
    if (dispatch_get_current_queue() == connection->socketQueue)
    {
        
    }
	
	
	if (connectIndex != connection->connectIndex)
	{
		
		// The connect operation has been cancelled.
		// That is, socket was disconnected, or connection has already timed out.
		return;
	}
	
    RSConnectionEndConnectTimeout(connection);
    RSConnectionCloseWithError(connection, error);
}

RSExport void RSConnectionStartConnectTimeout(RSConnectionRef connection, RSTimeInterval timeout)
{
	if (timeout >= 0.0)
	{
		connection->connectTimer = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, connection->socketQueue);
		
		dispatch_source_set_event_handler(connection->connectTimer, ^{
            RSAutoreleaseBlock(^{
//			[self doConnectTimeout];                
            });

		});
		
#if !OS_OBJECT_USE_OBJC
		dispatch_source_t theConnectTimer = connection->connectTimer;
		dispatch_source_set_cancel_handler(connection->connectTimer, ^{
			dispatch_release(theConnectTimer);
		});
#endif
		
		dispatch_time_t tt = dispatch_time(DISPATCH_TIME_NOW, (timeout * NSEC_PER_SEC));
		dispatch_source_set_timer(connection->connectTimer, tt, DISPATCH_TIME_FOREVER, 0);
		dispatch_resume(connection->connectTimer);
	}
}

RSExport void RSConnectionLookupHost(RSConnectionRef connection, int connectIndex, RSStringRef host, RSBitU16 port)
{
    // This method is executed on a global concurrent queue.
	// It posts the results back to the socket queue.
	// The lookupIndex is used to ignore the results if the connect operation was cancelled or timed out.
	
	RSErrorRef error = nil;
	
	RSDataRef address4 = nil;
	RSDataRef address6 = nil;
	
    if (RSEqual(host, RSSTR("localhost")) || RSEqual(host, RSSTR("loopback")))
    {
        // Use LOOPBACK address
		struct sockaddr_in nativeAddr;
		nativeAddr.sin_len         = sizeof(struct sockaddr_in);
		nativeAddr.sin_family      = AF_INET;
		nativeAddr.sin_port        = htons(port);
		nativeAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		memset(&(nativeAddr.sin_zero), 0, sizeof(nativeAddr.sin_zero));
		
		struct sockaddr_in6 nativeAddr6;
		nativeAddr6.sin6_len       = sizeof(struct sockaddr_in6);
		nativeAddr6.sin6_family    = AF_INET6;
		nativeAddr6.sin6_port      = htons(port);
		nativeAddr6.sin6_flowinfo  = 0;
		nativeAddr6.sin6_addr      = in6addr_loopback;
		nativeAddr6.sin6_scope_id  = 0;
        
        address4 = RSDataWithBytes(&nativeAddr, sizeof(nativeAddr));
		address6 = RSDataWithBytes(&nativeAddr6, sizeof(nativeAddr6));
    }
	else
	{
		RSStringRef portStr = RSStringWithFormat(RSSTR("%hu"), port);
		
		struct addrinfo hints, *res, *res0;
		
		memset(&hints, 0, sizeof(hints));
		hints.ai_family   = PF_UNSPEC;
		hints.ai_socktype = SOCK_STREAM;
		hints.ai_protocol = IPPROTO_TCP;
		
		int gai_error = getaddrinfo(RSStringGetCStringPtr(host, RSStringEncodingUTF8), RSStringGetCStringPtr(portStr, RSStringEncodingUTF8), &hints, &res0);
		
		if (gai_error)
		{
			error = __RSConnectionGaiError(gai_error);
		}
		else
		{
			for(res = res0; res; res = res->ai_next)
			{
				if ((address4 == nil) && (res->ai_family == AF_INET))
				{
					// Found IPv4 address
					// Wrap the native address structure
                    address4 = RSDataWithBytes(res->ai_addr, res->ai_addrlen);
				}
				else if ((address6 == nil) && (res->ai_family == AF_INET6))
				{
					// Found IPv6 address
					// Wrap the native address structure
                    address6 = RSDataWithBytes(res->ai_addr, res->ai_addrlen);
				}
			}
			freeaddrinfo(res0);
			
			if ((address4 == nil) && (address6 == nil))
			{
				error = __RSConnectionGaiError(EAI_FAIL);
			}
		}
	}
	
	if (error)
	{
		dispatch_async(connection->socketQueue, ^{
            RSAutoreleaseBlock(^{
                RSConnectionLookupDidFail(connection, connectIndex, error);
            });
		});
	}
	else
	{
		dispatch_async(connection->socketQueue, ^{
            RSAutoreleaseBlock(^{
//               [self lookup:aConnectIndex didSucceedWithAddress4:address4 address6:address6];
            });
		});
	}
}

RSExport BOOL RSConnectionConnectTo(RSConnectionRef connection, RSStringRef host, RSBitU16 port, RSStringRef interface, RSTimeInterval timeout, __autorelease RSErrorRef *error)
{
    __block BOOL result = YES;
	__block RSErrorRef err = nil;
	
	dispatch_block_t block = ^{
		RSAutoreleaseBlock(^{
            if (connection->delegate == nil)
            {
                result = NO;
                err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("Attempting to connect without a delegate. Set a delegate first.")));
                return;
            }
            
            if (!connection->delegateQueue)
            {
                result = NO;
                err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("Attempting to connect without a delegate queue. Set a delegate queue first.")));
                return;
            }
            
            BOOL isIPv4Disabled = (connection->config & kIPv4Disabled) ? YES : NO;
            BOOL isIPv6Disabled = (connection->config & kIPv6Disabled) ? YES : NO;
            
            if (isIPv4Disabled && isIPv6Disabled)
            {
                result = NO;
                err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("Both IPv4 and IPv6 have been disabled. Must enable at least one protocol first.")));
                return;
            }
            
            if (!RSConnectionIsConnected(connection))
            {
                result = NO;
                err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("Attempting to connect while connected or accepting connections. Disconnect first.")));
                return;
            }
            
            RSArrayRemoveAllObjects(connection->readQueue);
            RSArrayRemoveAllObjects(connection->writeQueue);
            if (interface)
            {
                RSMutableDataRef interface4 = nil, interface6 = nil;
                RSConnectionGetInterfaceInformation(&interface4, &interface6, interface, 0);
                if ((interface4 == nil) && (interface6 == nil))
                {
                    result = NO;
                    err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("Unknown interface. Specify valid interface by name (e.g. \"en1\") or IP address.")));
                    return;
                }
                
                if (isIPv4Disabled && (interface6 == nil))
                {
                    result = NO;
                    err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("IPv4 has been disabled and specified interface doesn't support IPv6.")));
                    return;
                }
                
                if (isIPv6Disabled && (interface4 == nil))
                {
                    result = NO;
                    err = (RSErrorRef)RSRetain(__RSConnectionBadParamError(RSSTR("IPv6 has been disabled and specified interface doesn't support IPv4.")));
                    return;
                }
                
                connection->connectInterface4 = RSRetain(interface4);
                connection->connectInterface6 = RSRetain(interface6);
            }
            
            connection->flags |= kSocketStarted;
            
            __RSCLog(RSLogLevelNotice, "Dispatching DNS lookup...");
            
            int aConnectIndex = connection->connectIndex;
            RSStringRef hostCpy = RSAutorelease(RSCopy(RSAllocatorSystemDefault, host));
            
            dispatch_queue_t globalConcurrentQueue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
            dispatch_block_t lookupBlock = ^{
                RSAutoreleaseBlock(^{
                    RSConnectionLookupHost(connection, aConnectIndex, hostCpy, port);
                });
            };
            dispatch_async(globalConcurrentQueue, lookupBlock);
            RSConnectionStartConnectTimeout(connection, timeout);
        });
	};
	
	if (dispatch_get_current_queue() == connection->socketQueue)
		block();
	else
		dispatch_sync(connection->socketQueue, block);
	
	if (result == NO)
	{
		if (error)
			*error = RSAutorelease(err);
		else
			RSRelease(err);
	}
	return result;
}