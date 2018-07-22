//
//  RSHashBlizzard.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include "RSHashBlizzard.h"

#define	BLIZZARD_HASH_TABLE_PART			10
#define	BLIZZARD_HASH_TABLE_PART_SIZE       0x100
#define BLIZZARD_HASH_TABLE_SIZE			BLIZZARD_HASH_TABLE_PART * BLIZZARD_HASH_TABLE_PART_SIZE

RSReserved RSHashCode	__RSBaseHashTable[BLIZZARD_HASH_TABLE_SIZE] = {0};
RSPrivate void __RSBaseHashBlizzardInitCryptTable()
{
	
	RSHashCode seed = 0x00100001, index1 = 0, index2 = 0, i;
	for( index1 = 0; index1 < BLIZZARD_HASH_TABLE_PART_SIZE; index1++ )
	{
		for( index2 = index1, i = 0; i < BLIZZARD_HASH_TABLE_PART; i++, index2 += BLIZZARD_HASH_TABLE_PART_SIZE )
		{
			RSHashCode temp1, temp2;
			seed = (seed * 125 + 3) % 0x2AAAAB;
			temp1 = (seed & 0xFFFF) << 0x10;
			seed = (seed * 125 + 3) % 0x2AAAAB;
			temp2 = (seed & 0xFFFF);
			__RSBaseHashTable[index2] = ( temp1 | temp2 );
		}
	}
}
RSPrivate void __RSBaseHashBlizzardReleaseCryptTable()
{
    RSZeroMemory(__RSBaseHashTable, sizeof(RSHashCode)*BLIZZARD_HASH_TABLE_SIZE);
}
RSPrivate RSHashCode __RSBaseHashBlizzardHash(RSTypeRef obj, RSIndex size, RSUInteger type)
{
	RSUBuffer key = (RSUBuffer)obj;
	RSHashCode seed1 = 0x7FED7FED, seed2 = 0xEEEEEEEE;
	RSHashCode ch = 0;
    RSHashCode offset = 0;
    RSIndex idx = 0;
	while(*key != 0 && idx < size)
	{
		ch = toupper(*key++);
        offset = (RSHashCode)(type << 8) + (RSHashCode)ch ;
		seed1 = __RSBaseHashTable[offset] ^ (seed1 + seed2);
		seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
        ++idx;
	}
	return seed1;
}
