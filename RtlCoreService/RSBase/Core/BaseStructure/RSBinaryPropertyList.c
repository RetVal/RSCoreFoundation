//
//  RSBinaryPropertyList.c
//  RSCoreFoundation
//
//  Created by RetVal on 2/21/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include "RSBinaryPropertyList.h"
#include <RSCoreFoundation/RSBaseHash.h>
#include <stdio.h>
static RSTypeID stringtype = _RSRuntimeNotATypeID, datatype = _RSRuntimeNotATypeID, numbertype = _RSRuntimeNotATypeID, datetype = _RSRuntimeNotATypeID;
static RSTypeID booltype __unused = _RSRuntimeNotATypeID, nulltype = _RSRuntimeNotATypeID, dicttype = _RSRuntimeNotATypeID, arraytype = _RSRuntimeNotATypeID;
static RSTypeID uuidtype = _RSRuntimeNotATypeID, urltype __unused = _RSRuntimeNotATypeID, osettype = _RSRuntimeNotATypeID, settype = _RSRuntimeNotATypeID;

RSExport void __RSBinaryPropertyListInitStatics()
{
    if (_RSRuntimeNotATypeID == stringtype) stringtype = RSStringGetTypeID();
    if (_RSRuntimeNotATypeID == datatype)   datatype = RSDataGetTypeID();
    if (_RSRuntimeNotATypeID == numbertype) numbertype = RSNumberGetTypeID();
//    if (_RSRuntimeNotATypeID == booltype) booltype = RSBooleanGetTypeID();
    if (_RSRuntimeNotATypeID == datetype)   datetype = RSDateGetTypeID();
    if (_RSRuntimeNotATypeID == dicttype)   dicttype = RSDictionaryGetTypeID();
    if (_RSRuntimeNotATypeID == arraytype)  arraytype = RSArrayGetTypeID();
    if (_RSRuntimeNotATypeID == settype)    settype = RSSetGetTypeID();
    if (_RSRuntimeNotATypeID == nulltype)   nulltype = RSNilGetTypeID();
    if (_RSRuntimeNotATypeID == uuidtype)   uuidtype = RSUUIDGetTypeID();
//    if (_RSRuntimeNotATypeID == urltype)  urltype = RSURLGetTypeID();
    if (_RSRuntimeNotATypeID == osettype)   osettype = _RSRuntimeNotATypeID;
}
/*
 HEADER
 magic number ("bplist")
 file format version
 byte length of plist incl. header, an encoded int number object (as below) [v.2+ only]
 32-bit CRC (ISO/IEC 8802-3:1989) of plist bytes w/o CRC, encoded always as
 "0x12 0x__ 0x__ 0x__ 0x__", big-endian, may be 0 to indicate no CRC [v.2+ only]
 
 OBJECT TABLE
 variable-sized objects
 
 Object Formats (marker byte followed by additional info in some cases)
 null	0000 0000			// null object [v1+ only]
 bool	0000 1000			// NO
 bool	0000 1001			// YES
 url	0000 1100	string		// URL with no base URL, recursive encoding of URL string [v1+ only]
 url	0000 1101	base string	// URL with base URL, recursive encoding of base URL, then recursive encoding of URL string [v1+ only]
 uuid	0000 1110			// 16-byte UUID [v1+ only]
 fill	0000 1111			// fill byte
 int	0001 0nnn	...		// # of bytes is 2^nnn, big-endian bytes
 real	0010 0nnn	...		// # of bytes is 2^nnn, big-endian bytes
 date	0011 0011	...		// 8 byte float follows, big-endian bytes
 data	0100 nnnn	[int]	...	// nnnn is number of bytes unless 1111 then int count follows, followed by bytes
 string	0101 nnnn	[int]	...	// ASCII string, nnnn is # of chars, else 1111 then int count, then bytes
 string	0110 nnnn	[int]	...	// Unicode string, nnnn is # of chars, else 1111 then int count, then big-endian 2-byte uint16_t
 0111 xxxx			// unused
 uid	1000 nnnn	...		// nnnn+1 is # of bytes
 1001 xxxx			// unused
 array	1010 nnnn	[int]	objref*	// nnnn is count, unless '1111', then int count follows
 ordset	1011 nnnn	[int]	objref* // nnnn is count, unless '1111', then int count follows [v1+ only]
 set	1100 nnnn	[int]	objref* // nnnn is count, unless '1111', then int count follows [v1+ only]
 dict	1101 nnnn	[int]	keyref* objref*	// nnnn is count, unless '1111', then int count follows
 1110 xxxx			// unused
 1111 xxxx			// unused
 
 OFFSET TABLE
 list of ints, byte size of which is given in trailer
 -- these are the byte offsets into the file
 -- number of these is in the trailer
 
 TRAILER
 byte size of offset ints in offset table
 byte size of object refs in arrays and dicts
 number of offsets in offset table (also is number of objects)
 element # in offset table which is top level object
 offset table offset
 
 Version 1.5 binary plists do not use object references (uid),
 but instead inline the object serialization itself at that point.
 It also doesn't use an offset table or a trailer.  It does have
 an extended header, and the top-level object follows the header.
 
 */

enum ___RSBPLFileSystemTypeIDLow
{
    ___LRSBPLNil = 0b0000,
    ___LRSBPLBooleanFalse = 0b1000,
    ___LRSBPLBooleanTrue = 0b1001,
    ___LRSBPLURLString   = 0b1100,
    ___LRSBPLURLBaseString = 0b1101,
    ___LRSBPLUUID = 0b1110,
    ___LRSBPLFill = 0b1111,
    ___LRSBPLInteger = 0b0111,
    ___LRSBPLReal = 0b0111,
    ___LRSBPLDate = 0b0011,
    ___LRSBPLData = 0b1111,
    ___LRSBPLStringASCII = 0b1111,
    ___LRSBPLStringUnicode = 0b1111,
    ___LRSBPLReserved0 = 0b0000,
    ___LRSBPLUID = 0b1111,
    ___LRSBPLReserved1 = 0b0000,
    ___LRSBPLArray = 0b1111,
    ___LRSBPLOrderSet = 0b1111,
    ___LRSBPLSet = 0b1111,
    ___LRSBPLDictionary = 0b1111,
    ___LRSBPLReserved2 = 0b0000,
    ___LRSBPLReserved3 = 0b0000,
};
typedef enum ___RSBPLFileSystemTypeIDLow ___RSBPLFileSystemTypeIDLow;

enum ___RSBPLFileSystemTypeIDHigh
{
    ___RSBPLNil = 0b0000,
    ___RSBPLBoolean = 0b0000,
    ___RSBPLURLString   = 0b0000,
    ___RSBPLURLBaseString = 0b0000,
    ___RSBPLUUID = 0b0000,
    ___RSBPLFill = 0b0000,
    ___RSBPLInteger = 0b0001,
    ___RSBPLReal = 0b0010,
    ___RSBPLDate = 0b0011,
    ___RSBPLData = 0b0100,
    ___RSBPLStringASCII = 0b0101,
    ___RSBPLStringUnicode = 0b0110,
    ___RSBPLReserved0 = 0b0111,
    ___RSBPLUID = 0b1000,
    ___RSBPLReserved1 = 0b1001,
    ___RSBPLArray = 0b1010,
    ___RSBPLOrderSet = 0b1011,
    ___RSBPLSet = 0b1100,
    ___RSBPLDictionary = 0b1101,
    ___RSBPLReserved2 = 0b1110,
    ___RSBPLReserved3 = 0b1111,
};
typedef enum ___RSBPLFileSystemTypeIDHigh ___RSBPLFileSystemTypeIDHigh;

enum __RSBinaryPropertyListVersion
{
    _RSBPListVersion0 = 0,         /*
                                     Version header
                                     Content
                                     */
    
    _RSBPListVersion1 = 1,         /*
                                     Version header
                                     Content with encode with 'R'
                                     */
    
    _RSBPListVersion2 = 2,         /*
                                     Version header
                                     Content with encode with 'R'
                                     Tail with sha hash code
                                     */
    _RSBPListVersionMax
};
typedef enum __RSBinaryPropertyListVersion __RSBinaryPropertyListVersion;

typedef struct __RSBinaryPropertyListHeader
{
    RSBitU8     _magic[6];
    RSBitU8     _version[2];
}__RSBinaryPropertyListHeader;

typedef struct __RSBinaryPropertyListTailerV1
{
    RSUBlock    _sha386[64];
}__RSBinaryPropertyListTailer, __RSBinaryPropertyListTailerV1;

typedef struct __RSBinaryPropertyListObjectHeaderV0
{
    RSBitU8     _type : 4;
    RSBitU8     _stct : 4;   //sub type or count of objects or mask 1111
}__RSBinaryPropertyListObjectHeader, __RSBinaryPropertyListObjectHeaderV0;

typedef struct __RSBinaryPropertyListObjectHeaderV1
{
    RSBitU8     _type : 4;
    RSBitU8     _mask : 4;   //sub type or count of objects or mask 1111
    RSUInteger   _count;
}__RSBinaryPropertyListObjectHeaderV1;

typedef struct __RSBinaryPropertyListObject
{
    struct __RSBinaryPropertyListObjectHeaderV0 _v0;
    RSUInteger _count;
    RSTypeRef _memory;
}__RSBinaryPropertyListObject;

#define __RSBinaryPropertyListWrittenBufferCacheSize (8192 - sizeof(RSTypeRef)*2 - sizeof(RSBitU64))
#undef  __RSBinaryPropertyListWrittenBufferCacheSize
typedef struct __RSBinaryPropertyListWrittenBuffer
{
    RSMutableDataRef _stream;
    RSErrorRef _error;
    RSBitU64 _written;
    __RSBinaryPropertyListVersion _version;
    //RSBitU8 _buffer[__RSBinaryPropertyListWrittenBufferCacheSize];
}__RSBinaryPropertyListWrittenBuffer;

static int ___RSBPLObjectHeaderCalculatePowBaseTwoI(unsigned long long number, __RSBinaryPropertyListObjectHeader* type)
{
    if (number < 0xff) {type->_stct |= 1; return 1;}
    if (number < 0xffff) {type->_stct |= 2; return 2;}
    if (number < 0xffffffff) {type->_stct |= 3; return 4;}
    else {type->_stct |= 4; return 8;}
    return 0;
}

static int ___RSBPLObjectHeaderCalculatePowBaseTwoF(double number, __RSBinaryPropertyListObjectHeader* type)
{
    if (number < 0xff) {type->_stct |= 1; return 1;}
    if (number < 0xffff) {type->_stct |= 2; return 2;}
    if (number < 0xffffffff) {type->_stct |= 3; return 4;}
    else {type->_stct |= 4; return 8;}
    return 0;
}

// WARNNING: __RSBPLObjectHeaderV0InitWithObject must put structure __RSBinaryPropertyListObjectHeaderV1 to headerV0.
static BOOL __RSBPLObjectHeaderV0InitWithObject(RSTypeRef obj, __RSBinaryPropertyListObjectHeader* headerV0)
{
    if (headerV0 == nil) return NO;
    RSTypeID objID = RSGetTypeID(obj);
    RSIndex cnt = 0;
    if (obj == nil || obj == RSNil || RSEqual(obj, RSNil))
    {
        headerV0->_type = 0b0000;
        headerV0->_stct = 0b0000;
        return YES;
    }
    else if (objID == numbertype)
    {
        if (RSNumberIsBooleanType(obj))
        {
            BOOL booleanValue = NO;
            RSNumberGetValue(obj, &booleanValue);
            headerV0->_type = ___RSBPLBoolean;
            if (booleanValue)
                headerV0->_stct = ___LRSBPLBooleanTrue;
            else
                headerV0->_stct = ___LRSBPLBooleanFalse;
            return YES;
        }
        if (RSNumberIsFloatType(obj))
        {
            double floatValue = .0f;
            RSNumberGetValue(obj, &floatValue);
            headerV0->_type = ___RSBPLReal;
            headerV0->_stct = ___RSBPLObjectHeaderCalculatePowBaseTwoF(floatValue, headerV0);
            return YES;
        }
        unsigned long long intValue = 0;
        RSNumberGetValue(obj, &intValue);
        headerV0->_type = ___RSBPLInteger;
        headerV0->_stct = ___RSBPLObjectHeaderCalculatePowBaseTwoI(intValue, headerV0);
        return YES;
    }
    else if (objID == datetype)
    {
        headerV0->_type = ___RSBPLDate;
        headerV0->_stct = ___LRSBPLDate;
        return YES;
    }
    else if (objID == datatype)
    {
        headerV0->_type = ___RSBPLData;
        headerV0->_stct = ((cnt = RSDataGetLength(obj)) < ___LRSBPLData) ? cnt : (((__RSBinaryPropertyListObjectHeaderV1*)headerV0)->_count = cnt, ___LRSBPLData);
        return YES;
    }
    else if (objID == stringtype)
    {
        headerV0->_type = ___RSBPLStringASCII;
        headerV0->_stct = ((cnt = RSStringGetLength(obj)) < ___LRSBPLStringASCII) ? cnt : (((__RSBinaryPropertyListObjectHeaderV1*)headerV0)->_count = cnt, ___LRSBPLStringASCII);
        return YES;
    }
    else if (objID == arraytype)
    {
        headerV0->_type = ___RSBPLArray;
        headerV0->_stct = ((cnt = RSArrayGetCount(obj)) < ___LRSBPLArray) ? cnt : (((__RSBinaryPropertyListObjectHeaderV1*)headerV0)->_count = cnt, ___LRSBPLArray);
        return YES;
    }
    else if (objID == settype)
    {
        headerV0->_type = ___RSBPLSet;
        headerV0->_stct = ((cnt = RSSetGetCount(obj)) < ___LRSBPLSet) ? cnt : (((__RSBinaryPropertyListObjectHeaderV1*)headerV0)->_count = cnt, ___LRSBPLSet);
        return YES;
    }
    else if (objID == dicttype)
    {
        headerV0->_type = ___RSBPLDictionary;
        headerV0->_stct = ((cnt = RSDictionaryGetCount(obj)) < ___LRSBPLDictionary) ? cnt : (((__RSBinaryPropertyListObjectHeaderV1*)headerV0)->_count = cnt, ___LRSBPLDictionary);
        return YES;
    }
    else if (objID == uuidtype)
    {
        headerV0->_type = ___RSBPLUUID;
        headerV0->_stct = ___LRSBPLUUID;
        return YES;
    }
    headerV0->_type = 0;
    headerV0->_stct = 0;
    return NO;
}

static void __RSBPLWriteenBufferFlush(__RSBinaryPropertyListWrittenBuffer* buffer)
{
    //___writeBytes(buffer->_stream, buffer->_buffer, buffer->_written);
    //memset(buffer->_buffer, 0, buffer->_written);
    //buffer->_written = 0;
}

RSInline uint8_t _byteCount(uint64_t count)
{
    uint64_t mask = ~(uint64_t)0;
    uint8_t size = 0;
    
    // Find something big enough to hold 'count'
    while (count & mask) {
        size++;
        mask = mask << 8;
    }
    
    // Ensure that 'count' is a power of 2
    // For sizes bigger than 8, just use the required count
    while ((size != 1 && size != 2 && size != 4 && size != 8) && size <= 8) {
        size++;
    }
    
    return size;
}

static BOOL __RSBPLWriteBuffer(__RSBinaryPropertyListWrittenBuffer* buffer, const RSBitU8* data, RSIndex length, BOOL needEncode)
{
    if (buffer == nil || data == nil || length == 0) return NO;
    //static RSBitU64 ___kRSBPLWBCacheSize = __RSBinaryPropertyListWrittenBufferCacheSize;
//    if (___kRSBPLWBCacheSize <= buffer->_written + length)
//    {
//        __RSBPLWriteenBufferFlush(buffer);
//    }
//    __builtin_memcpy(buffer->_buffer, data, length);
    if (needEncode == YES)
    {
        RSBitU8 _translateCache[256] = {0};
        RSBitU8* _ptr = length <= 256 ? _translateCache : RSAllocatorAllocate(RSAllocatorSystemDefault, length);
        __builtin_memcpy(_ptr, data, length);
        for (RSIndex idx = 0; idx < length; idx++)
            _ptr[idx] ^= 'R';
        RSDataAppendBytes(buffer->_stream, _ptr, length);
        if (_ptr != _translateCache) RSAllocatorDeallocate(RSAllocatorSystemDefault, _ptr);
    }
    else
        RSDataAppendBytes(buffer->_stream, data, length);
    return YES;
}

static BOOL __RSBPLReadBuffer(__RSBinaryPropertyListWrittenBuffer* buffer, RSBitU8* addressToWrite, RSIndex length, BOOL needDecode)
{
    if (buffer == nil || addressToWrite == nil || length == 0) return NO;
    if (needDecode == YES)
    {
        RSBitU8 _translateCache[256] = {0};
        RSBitU8* _ptr = length <= 256 ? _translateCache : RSAllocatorAllocate(RSAllocatorSystemDefault, length);
        __builtin_memcpy(_ptr, RSDataGetBytesPtr(buffer->_stream) + buffer->_written, length);
        for (RSIndex idx = 0; idx < length; idx++)
            _ptr[idx] ^= 'R';
        __builtin_memcpy(addressToWrite, _ptr, length);
        if (_ptr != _translateCache) RSAllocatorDeallocate(RSAllocatorSystemDefault, _ptr);
    }
    else
    {
        __builtin_memcpy(addressToWrite,  RSDataGetBytesPtr(buffer->_stream) + buffer->_written, length);
    }
    buffer->_written += length;
    return YES;
}

static BOOL __RSBPLBufferInitWithFileHandle(__RSBinaryPropertyListWrittenBuffer* buffer, RSFileHandleRef handle)
{
    if (buffer == nil || handle == nil) return NO;
    RSDataRef data = nil;
    buffer->_stream = RSMutableCopy(RSAllocatorSystemDefault, data = RSFileHandleReadDataToEndOfFile(handle));
    RSRelease(data);
    if (buffer->_stream) return YES;
    return NO;
}

static void __RSBPLBufferInit(__RSBinaryPropertyListWrittenBuffer* buffer)
{
    if (buffer == nil) return;
    if (buffer->_stream == nil)
        buffer->_stream = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
    buffer->_version = _RSBPListVersionMax - 1; // alway write the lastest version.
    buffer->_written = 0;
}

static BOOL __RSBPLBufferInitWithData(__RSBinaryPropertyListWrittenBuffer* buffer, RSDataRef data)
{
    if (buffer == nil || data == nil) return NO;
    buffer->_version = _RSBPListVersion0;
    buffer->_written = 0;
    buffer->_error = nil;
    buffer->_stream = RSMutableCopy(RSAllocatorSystemDefault, data);
    if (buffer->_stream) return YES;
    return NO;
}

static void __RSBPLBufferGetTailer(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListTailer* tailer)
{
    if (buffer->_version > _RSBPListVersion1 && buffer->_stream)
    {
        // have the tailer. should cut it
        RSIndex sizeOfTail = 0;
        switch (buffer->_version)
        {
            case _RSBPListVersion2:
                sizeOfTail = sizeof(__RSBinaryPropertyListTailerV1);
                break;
            default:
                break;
        }
//        RSBitU8 bytes[256] = {0}, *ptr = nil;
//        ptr = sizeOfTail <= 256 ? bytes : RSAllocatorAllocate(RSAllocatorSystemDefault, sizeOfTail);
//        __builtin_memcpy(ptr, RSDataGetBytesPtr(buffer->_stream) + RSDataGetLength(buffer->_stream) - sizeOfTail, sizeOfTail);
        __builtin_memcpy(tailer, RSDataGetBytesPtr(buffer->_stream) + RSDataGetLength(buffer->_stream) - sizeOfTail, sizeOfTail);
        RSDataDelete(buffer->_stream, RSMakeRange(RSDataGetLength(buffer->_stream) - sizeOfTail, sizeOfTail));
//        if (ptr != bytes) RSAllocatorDeallocate(RSAllocatorSystemDefault, ptr);
    }
}

static void __RSBPLBufferSetVersionOfBPL(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListVersion version)
{
    if (buffer == nil) return;
    buffer->_version = version;
}

static void __RSBPLBufferFinalize(__RSBinaryPropertyListWrittenBuffer* buffer)
{
    if (buffer == nil) return;
    if (buffer->_error) RSRelease(buffer->_error);
    buffer->_error = nil;
    if (buffer->_stream) RSRelease(buffer->_stream);
    buffer->_stream = nil;
    if (buffer->_written) buffer->_written = 0;
    return;
}

static void ___appendString(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListObject* v1)
{
    char BUF[512] = {0};
    char *buf = (char *)RSStringGetCStringPtr(v1->_memory, RSStringEncodingUTF8);
    RSIndex length = RSStringGetMaximumSizeForEncoding(RSStringGetLength(v1->_memory), RSStringEncodingUTF8);
    BOOL shouldReleaseBuf = NO;
    if (!buf)
    {
        buf = length > 512 ? (shouldReleaseBuf = YES, RSAllocatorAllocate(RSAllocatorSystemDefault, length)) : BUF;
        length = RSStringGetCString(v1->_memory, buf, length, RSStringEncodingUTF8);
    }
    
    if (length > ___LRSBPLStringASCII)
    {
        v1->_v0._stct = 0b1111;
        v1->_count = length;
    }
    else
    {
        v1->_v0._stct = length;
    }
    
    __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
    if (v1->_v0._stct == 0b1111)
    {
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_count, sizeof(RSUInteger), NO);
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)buf, v1->_count, (buffer->_version > _RSBPListVersion0));
    }
    else
    {
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)buf, v1->_v0._stct, (buffer->_version > _RSBPListVersion0));
    }
    if (buf != BUF && shouldReleaseBuf) RSAllocatorDeallocate(RSAllocatorSystemDefault, buf);
}

static void ___appendData(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListObject* v1)
{
    __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
    if (v1->_v0._stct == 0b1111)
    {
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_count, sizeof(RSUInteger), NO);
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)(RSDataGetBytesPtr(v1->_memory)), v1->_count, (buffer->_version > _RSBPListVersion0));
    }
    else
    {
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)(RSDataGetBytesPtr(v1->_memory)), v1->_v0._stct, (buffer->_version > _RSBPListVersion0));
    }
}

static void ___appendNumber(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListObject* v1)
{
    __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
    if (v1->_v0._type == 0b0001)
    {
        // integer
        long long __int64Value = 0;
        RSNumberGetValue(v1->_memory, &__int64Value);
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)&__int64Value, sizeof(long long), NO);
    }
    else if (v1->_v0._type == 0b0010)
    {
        double __doubleValue = 0.0f;
        RSNumberGetValue(v1->_memory, &__doubleValue);
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)&__doubleValue, sizeof(double), NO);
    }
}

static void ___appendDate(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListObject* v1)
{
    __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
    RSAbsoluteTime time = RSDateGetAbsoluteTime(v1->_memory);
    __RSBPLWriteBuffer(buffer, (const RSBitU8*)&time, sizeof(RSAbsoluteTime), NO);
}

static void ___appendUUID(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListObject* v1)
{
    __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
    RSUUIDBytes bytes = RSUUIDGetBytes(v1->_memory);
    __RSBPLWriteBuffer(buffer, (const RSBitU8*)&bytes, sizeof(RSUUIDBytes), (buffer->_version > _RSBPListVersion0));
}

static void __RSBPLAppendObject(__RSBinaryPropertyListWrittenBuffer* buffer, RSTypeRef obj)
{
    if (buffer->_version >= _RSBPListVersionMax) return;
    RSTypeID objectID = RSGetTypeID(obj);
    __RSBinaryPropertyListObject _v1 = {0}, *v1 = &_v1;
    v1->_count = 0;
    __RSBPLObjectHeaderV0InitWithObject(obj, (__RSBinaryPropertyListObjectHeader*)v1);
    v1->_memory = obj;
    if (objectID == stringtype)
    {
        ___appendString(buffer, v1);
    }
    else if (objectID == numbertype)
    {
        ___appendNumber(buffer, v1);
    }
    else if (objectID == datatype)
    {
        ___appendData(buffer, v1);
    }
    else if (objectID == datetype)
    {
        ___appendDate(buffer, v1);
    }
    else if (objectID == uuidtype)
    {
        ___appendUUID(buffer, v1);
    }
    else if (objectID == dicttype)
    {
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
        RSUInteger cnt = v1->_v0._stct;
        if (v1->_v0._stct == 0xf)
        {
            cnt = v1->_count;
            __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_count, sizeof(RSUInteger), NO);
        }
        
        RSTypeRef _contain[256*2] = {0};
        RSTypeRef* keyValuePairs = (((v1->_v0._stct == 0xf) ? (v1->_count > 256 ? RSAllocatorAllocate(RSAllocatorSystemDefault, v1->_count*2) : &_contain[0]) : (&_contain[0])));
        RSDictionaryGetKeysAndValues(v1->_memory, &keyValuePairs[0], &keyValuePairs[cnt]);
        
        for (RSUInteger idx = 0; idx < cnt; idx++)
        {
            __RSBPLAppendObject(buffer, keyValuePairs[idx]);
            __RSBPLAppendObject(buffer, keyValuePairs[cnt + idx]);
        }
        if (keyValuePairs != &_contain[0]) RSAllocatorDeallocate(RSAllocatorSystemDefault, keyValuePairs);
    }
    else if (objectID == arraytype)
    {
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
        RSUInteger cnt = v1->_v0._stct;
        if (v1->_v0._stct == 0xf)
        {
            cnt = v1->_count;
            __RSBPLWriteBuffer(buffer, (const RSBitU8*)&cnt, sizeof(RSUInteger), NO);
        }
        
        RSTypeRef _contain[256] = {nil};
        RSTypeRef* objects = (((v1->_v0._stct == 0xf) ? (v1->_count > 256 ? RSAllocatorAllocate(RSAllocatorSystemDefault, v1->_count) : &_contain[0]) : (&_contain[0])));
        RSArrayGetObjects(v1->_memory, RSMakeRange(0, cnt), objects);
        
        for (RSUInteger idx = 0; idx < cnt; idx++)
        {
            __RSBPLAppendObject(buffer, objects[idx]);
        }
        if (objects != &_contain[0]) RSAllocatorDeallocate(RSAllocatorSystemDefault, objects);
        objects = nil;
    }
}

static RSTypeRef __parserNil(__RSBinaryPropertyListObject* object)
{
    return RSNil;
}

static RSTypeRef __parserBoolean(__RSBinaryPropertyListObject* object)
{
    switch (object->_v0._stct)
    {
        case ___LRSBPLBooleanFalse:
            return RSBooleanFalse;
        case ___LRSBPLBooleanTrue:
            return RSBooleanTrue;
        default:
            HALTWithError(RSInvalidArgumentException, "the bplistRx is be damaged");
            break;
    }
    return nil;
}

static RSUUIDRef __parserUUID(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListObject* object)
{
    RSUUIDBytes bytes = {0};
    __RSBPLReadBuffer(buffer, (RSBitU8*)&bytes, sizeof(RSUUIDBytes), YES);
    return RSUUIDCreateWithUUIDBytes(RSAllocatorSystemDefault, bytes);
}

static void __RSBPLParserObject(__RSBinaryPropertyListWrittenBuffer* buffer, RSTypeRef* object)
{
    if (buffer->_version >= _RSBPListVersionMax) return;
    __RSBinaryPropertyListObject _bpl_object = {0};
    __RSBPLReadBuffer(buffer, (RSBitU8*)&_bpl_object, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
    long long int64Value = 0;
    double doubleValue = .0f;
    RSAbsoluteTime dateValue = .0f;
    RSBitU8* dataPtr = nil;
    RSBitU8 staticDataCache[256] = {0};
    RSUInteger length = 0;
    
    RSArrayRef array = nil;
    RSDictionaryRef dictionary = nil;
    RSSetRef set = nil;
    
    RSTypeRef* objectPtr = nil;
    RSTypeRef objectDataCahce[256] = {0};
    
    BOOL _v1_coder = (buffer->_version > _RSBPListVersion0);
    
    switch (_bpl_object._v0._type)
    {
        case 0:
            switch (_bpl_object._v0._stct)
            {
                case ___LRSBPLNil:
                    *object = RSNil;
                    break;
                case ___LRSBPLBooleanFalse:
                    *object = RSBooleanFalse;
                    break;
                case ___LRSBPLBooleanTrue:
                    *object = RSBooleanTrue;
                    break;
                case ___LRSBPLUUID:
                    *object = __parserUUID(buffer, &_bpl_object);
                    break;
                case ___LRSBPLFill:
                default:
                    break;
            }
            break;
        case ___RSBPLInteger:
            __RSBPLReadBuffer(buffer, (RSBitU8*)&int64Value, sizeof(long long), NO);
            *object = RSNumberCreateLonglong(RSAllocatorSystemDefault, int64Value);
            break;
        case ___RSBPLReal:
            __RSBPLReadBuffer(buffer, (RSBitU8*)&doubleValue, sizeof(double), NO);
            *object = RSNumberCreateDouble(RSAllocatorSystemDefault, doubleValue);
            break;
        case ___RSBPLDate:
            __RSBPLReadBuffer(buffer, (RSBitU8*)&dateValue, sizeof(RSAbsoluteTime), NO);
            *object = RSDateCreate(RSAllocatorSystemDefault, dateValue);
            break;
        case ___RSBPLData:
            length = _bpl_object._v0._stct;
            if (_bpl_object._v0._stct == 0xf)
            {
                __RSBPLReadBuffer(buffer, (RSBitU8*)&_bpl_object._count, sizeof(RSUInteger), NO);
                length = _bpl_object._count;
            }
            dataPtr = (length <= 256) ? staticDataCache : RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(RSBitU8)*length);
            __RSBPLReadBuffer(buffer, dataPtr, length, _v1_coder);
            *object = (length <= 256) ? RSDataCreate(RSAllocatorSystemDefault, dataPtr, length) : RSDataCreateWithNoCopy(RSAllocatorSystemDefault, dataPtr, length, YES, RSAllocatorSystemDefault);
            //if (dataPtr != staticDataCache) RSAllocatorDeallocate(RSAllocatorSystemDefault, dataPtr);
            break;
        case ___RSBPLStringASCII:
            length = _bpl_object._v0._stct;
            if (_bpl_object._v0._stct == 0xf)
            {
                __RSBPLReadBuffer(buffer, (RSBitU8*)&_bpl_object._count, sizeof(RSUInteger), NO);
                length = _bpl_object._count;
            }
            dataPtr = (length <= 256) ? staticDataCache : RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(RSBitU8)*length);
            __RSBPLReadBuffer(buffer, dataPtr, length, _v1_coder);
            if (length)
                *object = (dataPtr == staticDataCache) ? RSStringCreateWithBytes(RSAllocatorSystemDefault, dataPtr, length, RSStringEncodingASCII, NO) : RSStringCreateWithBytesNoCopy(RSAllocatorSystemDefault, dataPtr, length, RSStringEncodingASCII, NO, RSAllocatorSystemDefault);
            else *object = RSStringGetEmptyString();
            break;
        case ___RSBPLStringUnicode:
            length = _bpl_object._v0._stct;
            if (_bpl_object._v0._stct == 0xf)
            {
                __RSBPLReadBuffer(buffer, (RSBitU8*)&_bpl_object._count, sizeof(RSUInteger), NO);
                length = _bpl_object._count;
            }
            dataPtr = (length <= 256) ? staticDataCache : RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(RSBitU8)*length);
            __RSBPLReadBuffer(buffer, dataPtr, length, _v1_coder);
            if (length)
                *object = (dataPtr == staticDataCache) ? RSStringCreateWithBytes(RSAllocatorSystemDefault, dataPtr, length, RSStringEncodingUnicode, YES) : RSStringCreateWithBytesNoCopy(RSAllocatorSystemDefault, dataPtr, length, RSStringEncodingUnicode, YES, RSAllocatorSystemDefault);
            else *object = RSStringGetEmptyString();
            break;
        case ___RSBPLArray:
            length = _bpl_object._v0._stct;
            if (_bpl_object._v0._stct == 0xf)
            {
                __RSBPLReadBuffer(buffer, (RSBitU8*)&_bpl_object._count, sizeof(RSUInteger), NO);
                length = _bpl_object._count;
            }
            
            objectPtr = (length <= 256) ? objectDataCahce : RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(RSTypeRef)*length);
            for (RSUInteger idx = 0; idx < length; idx++)
            {
                __RSBPLParserObject(buffer, &objectPtr[idx]);
            }
            if (length)
                array = (objectPtr == objectDataCahce) ? RSArrayCreateWithObjects(RSAllocatorSystemDefault, objectPtr, length) : RSArrayCreateWithObjectsNoCopy(RSAllocatorSystemDefault, objectPtr, length, YES);
            else array = RSArrayCreateWithObject(RSAllocatorSystemDefault, nil);
            if (objectPtr == objectDataCahce)
            {
                for (RSUInteger idx = 0; idx < length; idx++)
                {
                    RSRelease(objectPtr[idx]);
                }
            }
            *object = array;
            break;
        case ___RSBPLDictionary:
            length = _bpl_object._v0._stct;
            if (_bpl_object._v0._stct == 0xf)
            {
                __RSBPLReadBuffer(buffer, (RSBitU8*)&_bpl_object._count, sizeof(RSUInteger), NO);
                length = _bpl_object._count;
            }
            objectPtr = (length <= 128) ? objectDataCahce : RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(RSTypeRef)*length*2);
            for (RSUInteger idx = 0; idx < length; idx++)
            {
                __RSBPLParserObject(buffer, &objectPtr[idx]);           // keyRef
                __RSBPLParserObject(buffer, &objectPtr[length + idx]);  // valueRef
            }
            dictionary = RSDictionaryCreate(RSAllocatorSystemDefault, &objectPtr[0], &objectPtr[length], RSDictionaryRSTypeContext, length);
            for (RSUInteger idx = 0; idx < length * 2; idx++)
            {
                RSRelease(objectPtr[idx]);
            }
            *object = dictionary;
            if (objectPtr != objectDataCahce) RSAllocatorDeallocate(RSAllocatorSystemDefault, objectPtr);
            break;
        case ___RSBPLOrderSet:
        case ___RSBPLSet:
            length = _bpl_object._v0._stct;
            if (_bpl_object._v0._stct == 0xf)
            {
                __RSBPLReadBuffer(buffer, (RSBitU8*)&_bpl_object._count, sizeof(RSUInteger), NO);
                length = _bpl_object._count;
            }
            
            objectPtr = (length <= 256) ? objectDataCahce : RSAllocatorAllocate(RSAllocatorSystemDefault, sizeof(RSTypeRef)*length);
            for (RSUInteger idx = 0; idx < length; idx++)
            {
                __RSBPLParserObject(buffer, &objectPtr[idx]);
            }
            set = RSSetCreate(RSAllocatorSystemDefault, &objectPtr[0], length, &RSTypeSetCallBacks);
            *object = set;
            for (RSUInteger idx = 0; idx < length; idx++)
            {
                RSRelease(objectPtr[idx]);
            }
            if (objectPtr != objectDataCahce) RSAllocatorDeallocate(RSAllocatorSystemDefault, objectPtr);
            break;
        default:
            break;
    }
}

static BOOL __RSBPLParserHeader(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListHeader* header)
{
    if (buffer == nil || header == nil) return NO;
    __RSBPLReadBuffer(buffer, (RSBitU8*)header, sizeof(__RSBinaryPropertyListHeader), NO);
    return YES;
}

static BOOL __RSBPLBuildTailer(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListTailer* tailer)
{
    if (buffer->_version > _RSBPListVersion1)
    {
        BOOL hashResult = RSBaseHash(RSSHA2Hash, RSDataGetBytesPtr(buffer->_stream), RSDataGetLength(buffer->_stream), (RSHashCode*)&tailer->_sha386[0], sizeof(tailer->_sha386));
        return hashResult;
    }
    return YES;
}

static void __RSBPLAppendTailer(__RSBinaryPropertyListWrittenBuffer* buffer)
{
    __RSBinaryPropertyListTailer tailer = {0};
    if (__RSBPLBuildTailer(buffer, &tailer))
    {
        __RSBPLWriteBuffer(buffer, (const RSBitU8*)&tailer, sizeof(__RSBinaryPropertyListTailer), NO);
    }
}

static void __RSBPLAppendCore(__RSBinaryPropertyListWrittenBuffer* buffer, RSTypeRef object, __RSBinaryPropertyListVersion version)
{
    if (version >= _RSBPListVersionMax) return;
    __RSBinaryPropertyListHeader header = {0};
    __builtin_memcpy(&header._magic, "bplist", 6);
    header._version[0] = 'R';
    header._version[1] = version + '0';
    __RSBPLWriteBuffer(buffer, (const RSBitU8*)&header, sizeof(__RSBinaryPropertyListHeader), NO);
    __RSBPLAppendObject(buffer, object);
    __RSBPLAppendTailer(buffer);
}

RSPrivate BOOL __RSBPLCheckHeader(RSDataRef data)
{
    if (RSDataGetLength(data) > 8)
    {
        if (__builtin_memcmp(RSDataGetBytesPtr(data), "bplist", 6))
            return NO;
    }
    return YES;
}

static BOOL __RSBPLCheckTailer(__RSBinaryPropertyListWrittenBuffer* buffer, __RSBinaryPropertyListTailer* tailer)
{
    if (tailer == nil) return NO;
    __RSBinaryPropertyListTailer __now = {0};
    switch (buffer->_version)
    {
        case _RSBPListVersion0:
        case _RSBPListVersion1:
            return YES;
        case _RSBPListVersion2:
            if (YES == __RSBPLBuildTailer(buffer, &__now))
            {
                if (0 == __builtin_memcmp(&__now, tailer, sizeof(__RSBinaryPropertyListTailer)))
                {
                    return YES;
                }
                return NO;
            }
            break;
        default:
            break;
    }
    return NO;
}

static RSTypeRef __RSBPLParserCore(__RSBinaryPropertyListWrittenBuffer* buffer)
{
    RSTypeRef bplist = nil;
    BOOL result = NO;
    __RSBinaryPropertyListHeader header = {0};
    result = __RSBPLParserHeader(buffer, &header);
    if (result == NO) return nil;
    if (__builtin_memcmp(&header._magic, "bplist", 6) || header._version[0] != 'R') return nil;
    __RSBinaryPropertyListTailer tailer = {0};
    // get the tailer if version over _RSBPListVersion2
    __RSBPLBufferSetVersionOfBPL(buffer, header._version[1] - '0');
    __RSBPLBufferGetTailer(buffer, &tailer);
    if (__RSBPLCheckTailer(buffer, &tailer))
        __RSBPLParserObject(buffer, &bplist);
    return bplist;
}

RSExport BOOL RSBinaryPropertyListWriteToFile(RSPropertyListRef propertyList, RSStringRef filePath, __autorelease RSErrorRef* error)
{
    if (propertyList == nil || filePath == nil) return NO;
    RSTypeRef content = RSPropertyListGetContent(propertyList);
    if (content)
    {
        RSFileHandleRef handle = RSFileHandleCreateForWritingAtPath(filePath);
        if (handle)
        {
            __RSBinaryPropertyListWrittenBuffer buffer = {0};
            __RSBPLBufferInit(&buffer);
            __RSBPLAppendCore(&buffer, content, _RSBPListVersion2);
            RSFileHandleWriteData(handle, buffer._stream);
            __RSBPLBufferFinalize(&buffer);
            RSRelease(handle);
            return YES;
        }
    }
    return NO;
}

/*
 only called by RSPrivate RSTypeRef __RSPropertyListParser(RSDataRef plistData, RSIndex* offset).
 so the data in here may be include the tailerV1. should check it first.
 */
RSPrivate RSTypeRef __RSBPLCreateWithData(RSAllocatorRef allocator, RSDataRef data)
{
    if (data == nil) return nil;
    RSTypeRef bplist = nil;
    __RSBinaryPropertyListWrittenBuffer buffer;
    if (NO == __RSBPLBufferInitWithData(&buffer, data)) return nil;
    bplist = __RSBPLParserCore(&buffer);
    __RSBPLBufferFinalize(&buffer);
    return bplist;
}

RSExport RSDataRef RSBinaryPropertyListGetData(RSPropertyListRef plist, __autorelease RSErrorRef* error)
{
    if (plist == nil) return nil;
    RSTypeRef content = RSPropertyListGetContent(plist);
    if (content)
    {
        __RSBinaryPropertyListWrittenBuffer buffer = {0};
        __RSBPLBufferInit(&buffer);
        __RSBPLAppendCore(&buffer, content, _RSBPListVersion2);
        RSDataRef data = RSRetain(buffer._stream);
        __RSBPLBufferFinalize(&buffer);
        return data;
    }
    return nil;
}

RSExport RSDataRef RSBinaryPropertyListCreateXMLData(RSAllocatorRef allocator, RSDictionaryRef requestDictionary)
{
    if (requestDictionary)
    {
        __RSBinaryPropertyListWrittenBuffer buffer = {0};
        __RSBPLBufferInit(&buffer);
        __RSBPLAppendCore(&buffer, requestDictionary, _RSBPListVersion2);
        RSDataRef data = RSRetain(buffer._stream);
        __RSBPLBufferFinalize(&buffer);
        return data;
    }
    return nil;
}

#include <RSCoreFoundation/RSArchiver.h>
RSPrivate RSTypeRef __RSBPLCreateObjectWithData(RSArchiverRef archiver, RSDataRef data)
{
    RSTypeRef obj = nil;
    __RSBinaryPropertyListWrittenBuffer buffer = {0};
    buffer._error = nil;
    buffer._version = _RSBPListVersion2;
    buffer._stream = (RSMutableDataRef)RSRetain(data);
    __RSBPLParserObject(&buffer, &obj);
    __RSBPLBufferFinalize(&buffer);
    return obj;
}

static BOOL __RSDataAppendWithEncode(RSMutableDataRef buffer, const RSBitU8* data, RSIndex length, BOOL needEncode)
{
    if (buffer == nil || data == nil || length == 0) return NO;
    //static RSBitU64 ___kRSBPLWBCacheSize = __RSBinaryPropertyListWrittenBufferCacheSize;
    //    if (___kRSBPLWBCacheSize <= buffer->_written + length)
    //    {
    //        __RSBPLWriteenBufferFlush(buffer);
    //    }
    //    __builtin_memcpy(buffer->_buffer, data, length);
    if (needEncode == YES)
    {
        RSBitU8 _translateCache[256] = {0};
        RSBitU8* _ptr = length <= 256 ? _translateCache : RSAllocatorAllocate(RSAllocatorSystemDefault, length);
        __builtin_memcpy(_ptr, data, length);
        for (RSIndex idx = 0; idx < length; idx++)
            _ptr[idx] ^= 'R';
        RSDataAppendBytes(buffer, _ptr, length);
        if (_ptr != _translateCache) RSAllocatorDeallocate(RSAllocatorSystemDefault, _ptr);
    }
    else
        RSDataAppendBytes(buffer, data, length);
    return YES;
}

RSPrivate RSDataRef __RSBPLCreateDataForObject(RSArchiverRef archiver, RSTypeRef obj)
{
    RSMutableDataRef data = nil;
    RSTypeID objectID = RSGetTypeID(obj);
    __RSBinaryPropertyListObject _v1 = {0}, *v1 = &_v1;
    v1->_count = 0;
    __RSBPLObjectHeaderV0InitWithObject(obj, (__RSBinaryPropertyListObjectHeader*)v1);
    v1->_memory = obj;
    if (objectID == stringtype)
    {
        data = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
        char BUF[512] = {0};
        char *buf = (char *)RSStringGetCStringPtr(v1->_memory, RSStringEncodingUTF8);
        RSIndex length = RSStringGetMaximumSizeForEncoding(RSStringGetLength(v1->_memory), RSStringEncodingUTF8);
        BOOL shouldReleaseBuf = NO;
        if (!buf)
        {
            buf = length > 512 ? (shouldReleaseBuf = YES, RSAllocatorAllocate(RSAllocatorSystemDefault, length)) : BUF;
            length = RSStringGetCString(v1->_memory, buf, length, RSStringEncodingUTF8);
        }
        
        if (length > ___LRSBPLStringASCII)
        {
            v1->_v0._stct = 0b1111;
            v1->_count = length;
        }
        else
        {
            v1->_v0._stct = length;
        }
        
        __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
        if (v1->_v0._stct == 0b1111)
        {
            __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_count, sizeof(RSUInteger), NO);
            __RSDataAppendWithEncode(data, (const RSBitU8*)buf, v1->_count, YES);
        }
        else
        {
            __RSDataAppendWithEncode(data, (const RSBitU8*)buf, v1->_v0._stct, YES);
        }
        if (buf != BUF && shouldReleaseBuf) RSAllocatorDeallocate(RSAllocatorSystemDefault, buf);
    }
    else if (objectID == numbertype)
    {
        data = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
        __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
        if (v1->_v0._type == 0b0001)
        {
            // integer
            long long __int64Value = 0;
            RSNumberGetValue(v1->_memory, &__int64Value);
            __RSDataAppendWithEncode(data, (const RSBitU8*)&__int64Value, sizeof(long long), NO);
        }
        else if (v1->_v0._type == 0b0010)
        {
            double __doubleValue = 0.0f;
            RSNumberGetValue(v1->_memory, &__doubleValue);
            __RSDataAppendWithEncode(data, (const RSBitU8*)&__doubleValue, sizeof(double), NO);
        }
    }
    else if (objectID == datatype)
    {
        data = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
        __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
        if (v1->_v0._stct == 0b1111)
        {
            __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_count, sizeof(RSUInteger), NO);
            __RSDataAppendWithEncode(data, (const RSBitU8*)(RSDataGetBytesPtr(v1->_memory)), v1->_count, YES);
        }
        else
        {
            __RSDataAppendWithEncode(data, (const RSBitU8*)(RSDataGetBytesPtr(v1->_memory)), v1->_v0._stct, YES);
        }
    }
    else if (objectID == datetype)
    {
        data = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
        __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
        RSAbsoluteTime time = RSDateGetAbsoluteTime(v1->_memory);
        __RSDataAppendWithEncode(data, (const RSBitU8*)&time, sizeof(RSAbsoluteTime), NO);
    }
    else if (objectID == uuidtype)
    {
        data = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
        __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
        RSUUIDBytes bytes = RSUUIDGetBytes(v1->_memory);
        __RSDataAppendWithEncode(data, (const RSBitU8*)&bytes, sizeof(RSUUIDBytes), YES);
    }
//    else if (objectID == dicttype)
//    {
//        data = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
//        __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
//        RSUInteger cnt = v1->_v0._stct;
//        if (v1->_v0._stct == 0xf)
//        {
//            cnt = v1->_count;
//            __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_count, sizeof(RSUInteger), NO);
//        }
//        
//        RSTypeRef _contain[256*2] = {0};
//        RSTypeRef* keyValuePairs = (((v1->_v0._stct == 0xf) ? (v1->_count > 256 ? RSAllocatorAllocate(RSAllocatorSystemDefault, v1->_count*2) : &_contain[0]) : (&_contain[0])));
//        RSDictionaryGetKeysAndValues(v1->_memory, &keyValuePairs[0], &keyValuePairs[cnt]);
//        
//        for (RSUInteger idx = 0; idx < cnt; idx++)
//        {
//            RSDataRef key = __RSBPLCreateDataForObject(archiver, keyValuePairs[idx]);
//            RSDataAppend(data, key);
//            RSRelease(key);
//            RSDataRef value = __RSBPLCreateDataForObject(archiver, keyValuePairs[cnt + idx]);
//            RSDataAppend(data, value);
//            RSRelease(value);
//        }
//        if (keyValuePairs != &_contain[0]) RSAllocatorDeallocate(RSAllocatorSystemDefault, keyValuePairs);
//    }
//    else if (objectID == arraytype)
//    {
//        data = RSDataCreateMutable(RSAllocatorSystemDefault, 0);
//        __RSDataAppendWithEncode(data, (const RSBitU8*)&v1->_v0, sizeof(__RSBinaryPropertyListObjectHeaderV0), NO);
//        RSUInteger cnt = v1->_v0._stct;
//        if (v1->_v0._stct == 0xf)
//        {
//            cnt = v1->_count;
//            __RSDataAppendWithEncode(data, (const RSBitU8*)&cnt, sizeof(RSUInteger), NO);
//        }
//        
//        RSTypeRef _contain[256] = {nil};
//        RSTypeRef* objects = (((v1->_v0._stct == 0xf) ? (v1->_count > 256 ? RSAllocatorAllocate(RSAllocatorSystemDefault, v1->_count) : &_contain[0]) : (&_contain[0])));
//        RSArrayGetObjects(v1->_memory, RSMakeRange(0, cnt), objects);
//        
//        for (RSUInteger idx = 0; idx < cnt; idx++)
//        {
//            RSDataRef data1 = __RSBPLCreateDataForObject(archiver, objects[idx]);
//            RSDataAppend(data, data1);
//            RSRelease(data1);
//        }
//        if (objects != &_contain[0]) RSAllocatorDeallocate(RSAllocatorSystemDefault, objects);
//        objects = nil;
//    }
    return data;
}
