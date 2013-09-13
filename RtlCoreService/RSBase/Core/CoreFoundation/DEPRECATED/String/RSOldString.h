//
//  __RSOldString.h
//  RSKit
//
//  Created by RetVal on 10/13/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSKit___RSOldString_h
#define RSKit___RSOldString_h
#include <RSKit/RSBase.h>
typedef RSCode __RSOldStringError;
#define		KX_FILTER_ANTI_ALPHA			0x00000001			//alpha
#define		KX_FILTER_ANTI_UPPER			0x00000002			//set upper
#define		KX_FILTER_ANTI_LOWWER			0x00000004			//set lowwer
#define		KX_FILTER_ANTI_NOCHANGE			0x00000008			//do not change the source
#define		KX_FILTER_ANTI_ENDSEMICOLON		0x00000010			//end with semicolon
#define		KX_FILTER_ANTI_ENDENTER			0x00000020			//end flag is enter key
#define		KX_FILTER_ANTI_ENDEND			0x00000040			//
#define		KX_FILTER_ANTI_ENDFLAGS			0x00000080			//end flag
#define		KX_FILTER_ANTI_SEMICOLON		0x00000100			//start with semicolon
#define		KX_FILTER_ANTI_ENTER			0x00000200			//start with enter
#define		KX_FILTER_ANTI_SPACE			0x00000400			//start with space
#define		KX_FILTER_ANTI_FLAGS			0x00000800			//start with flag
#define		KX_FILTER_ANTI_SYMBOL			0x00001000			//start with symbol
#define		KX_FILTER_ANTI_USER				0x80000000			//user defines

//Resource : the content1
//Reousrce2: the content2
//maxSubString: find the max substring between content1 and content2.
//return code: see RSError.
//version : beta. Bug inside level 9. do not use it public.
RSExport __RSOldStringError __RSOldStringFindMaxSubString(RSBuffer Resource,RSBuffer Resource2,
                                      RSBufferRef maxSubString);

//Resources: the root content.
//WantFind : to find the string in the root content.
//Exchange : use it to replace the WantFind in the root content.
//Output   : should be put outside by caller, the final string will write into this buffer.
//return code: true -> success.
RSExport BOOL 	__RSOldStringExchange(RSBuffer Resources,
                              RSBuffer WantFind,
                              RSBuffer Exchange,
                              RSBuffer Output);

//Resources:the root content.
//WantFind :to find the string in the root content.
//Index    : the offset of the WantFind in the content will store in it.
//return code: true -> success.
RSExport BOOL 	__RSOldStringGetIndex(RSBuffer Resources,
                              RSBuffer WantFind,
                              RSUIntegerRef Index);

//Resources:the root content.
//WantFind :to find the string in the root content.
//Sentinel : only find the content before the sentinel.
//Index    : the offset of the WantFind in the content will store in it.
//return code: true -> success.
RSExport BOOL  __RSOldStringGetPreIndex(RSBuffer	Resources,
                                 RSBuffer WantFd,
                                 RSUInteger Sentinel,
                                 RSUIntegerRef index);

//Resources:the root content.
//WantFind :to find the string in the root content.
//Sentinel : only find the content after the sentinel.
//Index    : the offset of the WantFind in the content will store in it.
//return code: true -> success.
RSExport BOOL    __RSOldStringGetNxtIndex(RSBuffer	Resources,
                                   RSBuffer WantFd,
                                   RSUInteger Sentinel,
                                   RSUIntegerRef index);

//Resources:the root content.
//WantF1,WantF2 :to find the string block between fd1 and fd2 in the root content.
//autoOffset : use for start part of the root content.
//nSwitch  : if fd1 and fd2 are not found, set 0. else 1 or 2.
//Index    : the offset of the string block in the content will store in it.
//return code: true -> success.
//version : beta, do not use it public. level 4.
RSExport BOOL	__RSOldStringGetNextBlock(RSBuffer Resources,
                                  RSBuffer WantFd1,RSBuffer WantFd2,
                                  RSUIntegerRef  autoOffset,
                                  int * nSwitch,
                                  RSUIntegerRef index);

//Resources:the root content.
//WantFind :to find the last string in the root content.
//Index    : the offset of the last WantFind in the content will store in it.
//return code: true -> success.
RSExport BOOL 	__RSOldStringGetLast(RSBuffer Resources,
                             RSBuffer WantFind,
                             RSUIntegerRef Index);

//same as strcmp
//uLength : the compare length
//bIsCase : is case sensitive
RSExport int		__RSOldStringCmp(RSBuffer Dec,
                             RSBuffer Src,
                             RSUInteger uLength,
                             BOOL bIsCase);

//upper the characters in the Des, length is equal to uLength.
RSExport void 	__RSOldStringToUpper(RSBuffer Dec,
                             RSUInteger uLength);

//lowwer the characters in the Des, length is equal to uLength.
RSExport void 	__RSOldStringToLowwer(RSBuffer Dec,
                              RSUInteger uLength);

//pszFilterBuffer : need filter content
//uFilterCode : KX_FILTER_ANTI_ALPHA & KX_FILTER_ANTI_SYMBOL
//puBufferLength : the return buffer length
//pOutBuffer : the return buffer, should be freed by MSFree after use or memeory leaks.
//return code : see RSError.
RSExport __RSOldStringError   __RSOldStringFilterCall(RSBuffer pszFilterBuffer,
                                  RSUInteger uFilterCode,
                                  RSUIntegerRef puBufferLength,
                                  RSBuffer pOutBuffer);

//pszAntiBuffer : need to anti-copy
//pszRetBuffer  : output buffer address
//uRetBufferLength : return size
//return code : see RSError.
//for instance : pszAntiBuffer = "Hello world." the pszRetBuffer = ".dlrow olleH".
RSExport __RSOldStringError   __RSOldStringAntiCopyCall(RSBuffer pszAntiBuffer,
                                    RSBuffer pszRetBuffer,
                                    RSUIntegerRef uRetBufferLength);

//pszCopyBuffer : content need to copy
//ppszOutBuffer : the new buffer which is copy the pszCopyBuffer
//uStartOffset  : the start address that need to copy
//uEndOffset    : the end address. size = uEndOffset - uStartOffset
//return code   : see RSError.
//ppszOutBuffer should be freed by MSFree after use or memory leaks.
RSExport __RSOldStringError   __RSOldStringCopyByOffset(RSBuffer pszCopyBuffer,
                                    RSBufferRef ppszOutBuffer,
                                    RSUInteger uStartOffset,
                                    RSUInteger uEndOffset);

//pszCopyBuffer : content need to copy
//ppszOutBuffer : the new buffer which is copy the pszCopyBuffer
//uCopyLength   : the length need to copy.
//return code   : see RSError.
//ppszOutBuffer should be freed by MSFree after use or memory leaks.
RSExport __RSOldStringError   __RSOldStringCopyByLength(RSBuffer pszCopyBuffer,
                                    RSBufferRef ppszOutBuffer,
                                    RSUInteger uCopyLength);

//pFilterBuffer : content need to filter
//pReserved     : the start string
//pReservedE    : the end string, pReserved and pReservedE must use pair and the Filter code will be ignored.
//puFilterCode  : if want to use the Filter code, the pReserved and pReservedE must be nil. define :
/*
 #define		KX_FILTER_ANTI_ALPHA			0x00000001			//alpha
 #define		KX_FILTER_ANTI_UPPER			0x00000002			//set upper
 #define		KX_FILTER_ANTI_LOWWER			0x00000004			//set lowwer
 #define		KX_FILTER_ANTI_NOCHANGE			0x00000008			//do not change the source
 #define		KX_FILTER_ANTI_ENDSEMICOLON		0x00000010			//end with semicolon
 #define		KX_FILTER_ANTI_ENDENTER			0x00000020			//end flag is enter key
 #define		KX_FILTER_ANTI_ENDEND			0x00000040			//
 #define		KX_FILTER_ANTI_ENDFLAGS			0x00000080			//end flag
 #define		KX_FILTER_ANTI_SEMICOLON		0x00000100			//start with semicolon
 #define		KX_FILTER_ANTI_ENTER			0x00000200			//start with enter
 #define		KX_FILTER_ANTI_SPACE			0x00000400			//start with space
 #define		KX_FILTER_ANTI_FLAGS			0x00000800			//start with flag
 #define		KX_FILTER_ANTI_SYMBOL			0x00001000			//start with symbol
 #define		KX_FILTER_ANTI_USER				0x80000000			//user defines
 */
//ppOutBuffer   : the new buffer which is filter over the content
//puStartOffset : the last time that the start part(flag)'s offset appear in the content.
//puEndOffset   : the last time that the end part(flag)'s offset appear in the content.
//return code   : see RSError.
//ppOutBuffer should be freed by MSFree after use or memory leaks.
//Recursive call inside itself. So do not depend on the puStartOffset or puEndOffset anyway.
RSExport __RSOldStringError   __RSOldStringFilterCallEx(RSBuffer pFilterBuffer,
                                    RSBuffer pReserved ,
                                    RSBuffer pReservedE ,
                                    RSUIntegerRef puFilterCode,
                                    RSBufferRef ppOutBuffer,
                                    RSUIntegerRef puStartOffset,
                                    RSUIntegerRef puEndOffset);

//Resources : the root content.
//WantFind  : the string want to find in the root content.
//Counter   : the WantFind appear numbers in the root content.
//return code: true -> appear.
RSExport BOOL 	__RSOldStringGetCounter(RSBuffer Resources,
                                RSBuffer WantFind,
                                RSUIntegerRef Counter);

//Resources : the root content.
//WantFind  : the string want to find in the root content.
//sentinelS : the start offset of the root content.
//sentinelE : the end offset of the root content. search size is equal to sentinelE - sentinelS.
//Counter   : the WantFind appear numbers in the root content.
//return code: true -> appear.
RSExport BOOL	__RSOldStringGetCounterWithSentinel(RSBuffer Resources,
                                            RSBuffer WantFd,
                                            RSUInteger sentinelS,
                                            RSUInteger sentinelE,
                                            RSUIntegerRef counter);
//filterBuf  : the content need to filter.
//target     : filter the target in the content.
//ppBuf      : the result buffer.
//return code: see RSError
//ppBuffer should be freed by MSFree after use or memory leaks.
RSExport __RSOldStringError __RSOldStringFilterTargetInBuffer(	RSBuffer filterBuf,
                                          RSBuffer target,
                                          RSBufferRef ppBuf);

#endif
