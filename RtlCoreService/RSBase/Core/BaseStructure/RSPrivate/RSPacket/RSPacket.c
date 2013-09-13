//
//  RSPacket.c
//  RSCoreFoundation
//
//  Created by RetVal on 11/20/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSPacket.h>
#include <RSCoreFoundation/RSBase.h>
#ifndef __RSPacketHeaderMagic
#define __RSPacketHeaderMagic 0xcafeface
#endif
struct __RSPacketHeader
{
    unsigned long magic;
    struct
    {
        unsigned long encrypt:1;        // is encrypted.
        unsigned long encryptType:4;    // 16
        
        unsigned long cmd:1;            // is command packet.
        unsigned long cmdType:10;       // 1024
        unsigned long cmdArgs:3;        // if is the command packet, and if the command arguments is not 0,
        // should recv the cmdArgs sub packet for command arguments.
        unsigned long needFeedback:1;   // is need feed back packet
        
        unsigned long reserved : sizeof(unsigned long)*8 - 20;
    }flag;
    
    unsigned long dataLength;
    unsigned long payloadLength;   //RSPacketPayloadLength constant
    
    unsigned long crc32code;
    char secretKey[32];
#if defined(DEBUG)
    char name[16];                  // the name of the packet, just for debug.
#endif
};

struct __RSPacketPayload
{
    unsigned char data[RSPacketPayloadLength];
};

struct __RSPacket
{
    struct __RSPacketHeader header;
    struct __RSPacketPayload payload;
};

RSInline BOOL isValidPacket(__RSPacket* packet)
{
    return (packet->header.magic == __RSPacketHeaderMagic) ? (YES) : (NO);
}

RSInline void markPacketValid(__RSPacket* packet)
{
    packet->header.magic = __RSPacketHeaderMagic;
}

RSInline void unMarkPacketValid(__RSPacket* packet)
{
    packet->header.magic = 0;
}

RSInline unsigned long getDataLength(__RSPacket* packet)
{
    return packet->header.dataLength;
}

RSInline void setDataLength(__RSPacket* packet, unsigned long dataLength)
{
    packet->header.dataLength = dataLength;
}

RSInline unsigned long getPayloadSize(__RSPacket* packet)
{
    return packet->header.payloadLength;
}

RSInline void setPayloadSize(__RSPacket* packet, unsigned long payloadSize)
{
    packet->header.payloadLength = payloadSize;
}

RSInline unsigned long getPayloadCheckSum(__RSPacket* packet)
{
    return packet->header.crc32code;
}

RSInline void setPayloadCheckSum(__RSPacket* packet, unsigned long crc32)
{
    packet->header.crc32code = crc32;
}

RSInline void getPacketKey(__RSPacket* packet, char key[32])
{
    memcpy(key, packet->header.secretKey, sizeof(char)*32);
}

RSInline void setPacketKey(__RSPacket* packet, const char key[32])
{
    memcpy(packet->header.secretKey, key, sizeof(char)*32);
}

RSInline void resetPacketKey(__RSPacket* packet)
{
    memset(packet->header.secretKey, 0, sizeof(char)*32);
}

RSInline const char* getPacketKeyPtr(__RSPacket* packet)
{
    return packet->header.secretKey;
}
#if defined(DEBUG)
RSInline void getPacketName(__RSPacket* packet, char name[16])
{
    memcpy(name, packet->header.name, sizeof(char)*16);
}

RSInline void setPacketName(__RSPacket* packet, const char name[16])
{
    memcpy(packet->header.name, name, sizeof(char)*16);
}

RSInline void cleanPacketName(__RSPacket* packet)
{
    memset(packet->header.name, 0, sizeof(char)*16);
}

RSInline const char* getPacketNamePtr(__RSPacket* packet)
{
    return packet->header.name;
}
#else 
RSInline void getPacketName(__RSPacket* packet, char name[16])
{
    
}

RSInline void setPacketName(__RSPacket* packet, const char name[16])
{
    
}

RSInline void cleanPacketName(__RSPacket* packet)
{
    
}

RSInline const char* getPacketNamePtr(__RSPacket* packet)
{
    return "";
}
#endif
RSInline BOOL isNeedFeedback(__RSPacket *p)
{
	return (p->header.flag).needFeedback;
}

RSInline void markNeedFeedback(__RSPacket *p)
{
	(p->header.flag).needFeedback = YES;
}
RSInline void unMarkNeedFeedback(__RSPacket *p)
{
    (p->header.flag).needFeedback = NO;
}

RSInline BOOL isEncrypted(__RSPacket* packet)
{
    return packet->header.flag.encrypt;
}

RSInline void markEncrypt(__RSPacket *p)
{
	(p->header.flag).encrypt = YES;
}

RSInline void unMarkEncrypt(__RSPacket *p)
{
	(p->header.flag).encrypt = NO;
}

RSInline enum RSPacketCommand getEncryptType(__RSPacket *p)
{
	return (p->header.flag).encryptType;
}

RSInline void setEncryptType(__RSPacket *p, unsigned long encryptType)
{
	(p->header.flag).encryptType = encryptType;
}



RSInline BOOL isDataPacket(__RSPacket *p)
{
	return ((p->header.flag).cmd == NO);
}

RSInline BOOL isCmdPacket(__RSPacket *p)
{
	return ((p->header.flag).cmd == YES);
}

RSInline void markDataPacket(__RSPacket* packet)
{
    packet->header.flag.cmd = NO;
}

RSInline void markCmdPacket(__RSPacket* packet)
{
    packet->header.flag.cmd = YES;
}


RSInline unsigned int getCmdType(__RSPacket *p)
{
	return (p->header.flag).cmdType;
}

RSInline unsigned int getCmdArgsNumber(__RSPacket *p)
{
	return (p->header.flag).cmdArgs;
}


RSInline void setCmdType(__RSPacket *p, enum RSPacketCommand cmdType)
{
	(p->header.flag).cmdType = cmdType;
}

RSInline void setCmdArgsNumber(__RSPacket *p, unsigned int cmdArgs)
{
	(p->header.flag).cmdArgs = cmdArgs;
}


__RSPacket* __RSPacketInit(__RSPacket* packet)
{
    if (packet == nil) return nil;
    markPacketValid(packet);
    markDataPacket(packet);
    unMarkEncrypt(packet);
    unMarkNeedFeedback(packet);

    return packet;
}


__RSPacket* __RSPacketBuildData(__RSPacket* packet, const unsigned char* data, unsigned long length)
{
    if (packet == nil || data == nil || length == 0) return packet;
    __RSPacketInit(packet);
    if (length > getPayloadSize(packet))
    {
        length = getPayloadSize(packet);
    }
    memcpy(packet->payload.data, data, length);
    setDataLength(packet, length);
    return packet;
}

__RSPacket* __RSPacketDataEncrypt(__RSPacket* packet, RSPacketCoding coding, RSPacketCodingType type, const char key[32], void* context)
{
    if (packet == nil || coding == nil) return packet;
    if (isCmdPacket(packet)) return packet;
    BOOL result = coding(packet->payload.data, key, packet->header.dataLength, context);
    if (result == YES)
    {
        setEncryptType(packet, type);
        setPacketKey(packet, key);
        markEncrypt(packet);
    }
    return packet;
}

__RSPacket* __RSPacketDataDecrypt(__RSPacket* packet, RSPacketCoding coding, RSPacketCodingType type, void* context)
{
    if (packet == nil || coding == nil) return packet;
    if (isCmdPacket(packet)) return packet;
    if (getEncryptType(packet) != type) return nil;
    BOOL result = coding(packet->payload.data, getPacketKeyPtr(packet), packet->header.dataLength, context);
    if (result == YES)
    {
        unMarkEncrypt(packet);
    }
    return packet;
}

__RSPacket* __RSPacketCheckSum(__RSPacket* packet, RSPacketCheckSum cs, void* context)
{
    if (packet == nil || cs == nil) return packet;
    packet->header.crc32code = 0x0;
    if (isCmdPacket(packet))
    {
        packet->header.crc32code = cs(&packet->header, sizeof(struct __RSPacketHeader), context);
    }
    else
    {
        packet->header.crc32code = cs(packet, sizeof(struct __RSPacket), context);
    }
    return packet;
}

BOOL __RSPacketIsNeedFeedBack(__RSPacket* packet)
{
    if (packet)
        return isNeedFeedback(packet);
    return NO;
}

BOOL __RSPacketIsEncrypted(__RSPacket* packet)
{
    if (packet)
        return isEncrypted(packet);
    return NO;
}

BOOL __RSPacketIsCommand(__RSPacket* packet)
{
    if (packet)
        return isCmdPacket(packet);
    return NO;
}
BOOL __RSPacketIsValid(__RSPacket* packet)
{
    if (packet)
        return isValidPacket(packet);
    return NO;
}

BOOL __RSPacketIsEmpty(__RSPacket* packet)
{
    if (packet)
        return (getDataLength(packet) == 0);
    return YES;
}

unsigned long __RSPacketGetData(__RSPacket* packet, unsigned char* databuf, unsigned long bufferSize)
{
    if (packet && isValidPacket(packet) && databuf && bufferSize)
    {
        databuf[bufferSize - 1] = 0; // test the data buffer size is ture or NO.
        bufferSize = min(bufferSize, getDataLength(packet));
        if (bufferSize)
        {
            memcpy(databuf, packet->payload.data, bufferSize);
        }
        return bufferSize;
    }
    return 0;
}
unsigned long __RSPacketGetDataLength(__RSPacket* packet)
{
    if (packet && isValidPacket(packet))
    {
        return getDataLength(packet);
    }
    return 0;
}

unsigned long __RSPacketGetPacketSize(__RSPacket* packet)
{
    if (packet && isValidPacket(packet))
    {
        return getPayloadSize(packet);
    }
    return 0;
}