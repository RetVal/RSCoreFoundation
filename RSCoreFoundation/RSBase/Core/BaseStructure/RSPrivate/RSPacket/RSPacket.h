//
//  RSPacket.h
//  RSCoreFoundation
//
//  Created by RetVal on 11/20/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSPacket_h
#define RSCoreFoundation_RSPacket_h

#define RSPacketPayloadLength   512

typedef struct __RSPacketHeader __RSPacketHeader;
typedef struct __RSPacketPayload __RSPacketPayload;
typedef struct __RSPacket __RSPacket;

enum RSPacketCommand {
    __RSPacketMail = 10,
};

typedef BOOL (*RSPacketCoding)(unsigned char* data, const char key[32], unsigned long length, void* context);

typedef unsigned long (*RSPacketCheckSum)(void* check, unsigned long size, void* context);
typedef int RSPacketCodingType;

__RSPacket* __RSPacketBuildData(__RSPacket* packet, const unsigned char* data, unsigned long length);

__RSPacket* __RSPacketDataEncrypt(__RSPacket* packet, RSPacketCoding coding, RSPacketCodingType type, const char key[32], void* context);
__RSPacket* __RSPacketDataDecrypt(__RSPacket* packet, RSPacketCoding coding, RSPacketCodingType type, void* context);

__RSPacket* __RSPacketCheckSum(__RSPacket* packet, RSPacketCheckSum cs, void* context);

BOOL __RSPacketIsNeedFeedBack(__RSPacket* packet);
BOOL __RSPacketIsEncrypted(__RSPacket* packet);
BOOL __RSPacketIsCommand(__RSPacket* packet);
BOOL __RSPacketIsValid(__RSPacket* packet);
BOOL __RSPacketIsEmpty(__RSPacket* packet);

// if the buffer is too small to hold all data in the packet, the return is the need size, or return the write size.
unsigned long __RSPacketGetData(__RSPacket* packet, unsigned char* databuf, unsigned long bufferSize);
unsigned long __RSPacketGetDataLength(__RSPacket* packet);
unsigned long __RSPacketGetPacketSize(__RSPacket* packet);

#endif
