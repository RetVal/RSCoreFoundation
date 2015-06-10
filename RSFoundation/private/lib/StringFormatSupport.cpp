//
//  StringFormatSupport.cpp
//  RSCoreFoundation
//
//  Created by closure on 6/10/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#include "StringFormatSupport.hpp"

// format supporting
namespace RSFoundation {
    namespace Collection {
        void StringFormat::ParseFormatSpec(const UniChar *uformat, const uint8_t *cformat, Index *fmtIdx, Index fmtLen, RSFoundation::Collection::StringFormat::FormatSpec *spec, RSFoundation::Collection::String **configKeyPointer) {
            BOOL seenDot = NO;
            BOOL seenSharp = NO;
            Index keyIndex = NotFound;
            
            for (;;) {
                UniChar ch;
                if (fmtLen <= *fmtIdx) return;	/* no type */
                if (cformat) ch = (UniChar)cformat[(*fmtIdx)++]; else ch = uformat[(*fmtIdx)++];
                
                if (keyIndex >= 0) {
                    if (ch == 'R' || ch == 'r') {
                        // found the key
                        Index length = (*fmtIdx) - 1 - keyIndex;
                        
                        spec->flags |= FormatExternalSpecFlag;
                        spec->type = FormatType::RS;
                        spec->size = FormatSize::SizePointer;  // 4 or 8 depending on LP64
                        
                        if ((nil != configKeyPointer) && (length > 0)) {
                            if (cformat) {
//                                *configKeyPointer = RSStringCreateWithBytes(nil, cformat + keyIndex, length, __RSStringGetEightBitStringEncoding(), FALSE);
                            } else {
//                                *configKeyPointer = RSStringCreateWithCharactersNoCopy(nil, uformat + keyIndex, length, nullptr);
                            }
                        }
                        return;
                    }
                    if ((ch < '0') || ((ch > '9') && (ch < 'A')) || ((ch > 'Z') && (ch < 'a') && (ch != '_')) || (ch > 'z')) {
                        //                if (ch == 'R' || ch == 'r') // ch == '@'
                        //                {
                        //                    // found the key
                        //                    Index length = (*fmtIdx) - 1 - keyIndex;
                        //
                        //                    spec->flags |= RSStringFormatExternalSpecFlag;
                        //                    spec->type = RSFormatRSType;
                        //                    spec->size = RSFormatSizePointer;  // 4 or 8 depending on LP64
                        //
                        //                    if ((nil != configKeyPointer) && (length > 0))
                        //                    {
                        //                        if (cformat)
                        //                        {
                        //                            *configKeyPointer = RSStringCreateWithBytes(nil, cformat + keyIndex, length, __RSStringGetEightBitStringEncoding(), FALSE);
                        //                        }
                        //                        else
                        //                        {
                        //                            *configKeyPointer = RSStringCreateWithCharactersNoCopy(nil, uformat + keyIndex, length, RSAllocatorNull);
                        //                        }
                        //                    }
                        //                    return;
                        //                }
                        keyIndex = NotFound;
                    }
                    continue;
                }
                
            reswtch:
                switch (ch)
                {
                    case '#':	// ignored for now
                        seenSharp = YES;
                        break;
                    case 0x20:
                        if (!(spec->flags & FormatPlusFlag)) spec->flags |= FormatSpaceFlag;
                        break;
                    case '-':
                        spec->flags |= FormatMinusFlag;
                        spec->flags &= ~FormatZeroFlag;	// remove zero flag
                        break;
                    case '+':
                        spec->flags |= FormatPlusFlag;
                        spec->flags &= ~FormatSpaceFlag;	// remove space flag
                        break;
                    case '0':
                        if (seenDot) {    // after we see '.' and then we see '0', it is 0 precision. We should not see '.' after '0' if '0' is the zero padding flag
                            spec->precArg = 0;
                            break;
                        }
                        if (!(spec->flags & FormatMinusFlag)) spec->flags |= FormatZeroFlag;
                        break;
                    case 'h':
                        if (*fmtIdx < fmtLen) {
                            // fetch next character, don't increment fmtIdx
                            if (cformat) ch = (UniChar)cformat[(*fmtIdx)]; else ch = uformat[(*fmtIdx)];
                            if ('h' == ch) {	// 'hh' for char, like 'c'
                                (*fmtIdx)++;
                                spec->size = FormatSize::Size1;
                                break;
                            }
                        }
                        spec->size = FormatSize::Size2;
                        break;
                    case 'l':
                        if (*fmtIdx < fmtLen) {
                            // fetch next character, don't increment fmtIdx
                            if (cformat) ch = (UniChar)cformat[(*fmtIdx)]; else ch = uformat[(*fmtIdx)];
                            if ('l' == ch) {	// 'll' for long long, like 'q'
                                (*fmtIdx)++;
                                spec->size = FormatSize::Size8;
                                break;
                            }
                        }
                        spec->size = FormatSize::SizeLong;  // 4 or 8 depending on LP64
                        break;
#if LONG_DOUBLE_SUPPORT
                    case 'L':
                        spec->size = FormatSize::Size16;
                        break;
#endif
                    case 'q':
                        spec->size = FormatSize::Size8;
                        break;
                    case 't': case 'z':
                        spec->size = FormatSize::SizeLong;  // 4 or 8 depending on LP64
                        break;
                    case 'j':
                        spec->size = FormatSize::Size8;
                        break;
                    case 'c':
                        spec->type = FormatType::Long;
                        spec->size = FormatSize::Size1;
                        return;
                    case 'D': case 'd': case 'i': case 'U': case 'u':
                        // we can localize all but octal or hex
                        //                if (_RSExecutableLinkedOnOrAfter(RSSystemVersionMountainLion))
                        spec->flags |= FormatLocalizable;
                        spec->numericFormatStyle = (UInt16)NumericFormatStyle::Decimal;
                        if (ch == 'u' || ch == 'U') spec->numericFormatStyle = (UInt16)NumericFormatStyle::Unsigned;
                        // fall thru
                    case 'O': case 'o': case 'x': case 'X':
                        spec->type = FormatType::Long;
                        // Seems like if spec->size == 0, we should spec->size = RSFormatSize4. However, 0 is handled correctly.
                        return;
                    case 'f': case 'F': case 'g': case 'G': case 'e': case 'E': {
                        // we can localize all but hex float output
                        //                if (_RSExecutableLinkedOnOrAfter(RSSystemVersionMountainLion))
                        spec->flags |= FormatLocalizable;
                        char lch = (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
                        spec->numericFormatStyle = ((lch == 'e' || lch == 'g') ? (UInt16)NumericFormatStyle::Scientific : 0) | ((lch == 'f' || lch == 'g') ? (UInt16)NumericFormatStyle::Decimal : 0);
                        if (seenDot && spec->precArg == -1 && spec->precArgNum == -1) { // for the cases that we have '.' but no precision followed, not even '*'
                            spec->precArg = 0;
                        }
                    }
                        // fall thru
                    case 'a': case 'A':
                        spec->type = FormatType::Double;
                        if (spec->size != FormatSize::Size16) spec->size = FormatSize::Size8;
                        return;
                    case 'n':		/* %n is not handled correctly; for Leopard or newer apps, we disable it further */
                        spec->type = /* DISABLES CODE */ (1) ? FormatType::DummyPointer : FormatType::Pointer;
                        spec->size = FormatSize::SizePointer;  // 4 or 8 depending on LP64
                        return;
                    case 'p':
                        spec->type = FormatType::Pointer;
                        spec->size = FormatSize::SizePointer;  // 4 or 8 depending on LP64
                        return;
                    case 's':
                        spec->type = FormatType::Chars;
                        spec->size = FormatSize::SizePointer;  // 4 or 8 depending on LP64
                        return;
                    case 'S':
                        spec->type = FormatType::Unichars;
                        spec->size = FormatSize::SizePointer;  // 4 or 8 depending on LP64
                        return;
                    case 'C':
                        spec->type = FormatType::SingleUnichar;
                        spec->size = FormatSize::Size2;
                        return;
                    case 'P':
                        spec->type = FormatType::PascalChars;
                        spec->size = FormatSize::SizePointer;  // 4 or 8 depending on LP64
                        return;
                        //            case '@':
                    case 'R':
                    case 'r':
                        if (seenSharp) {
                            seenSharp = NO;
                            keyIndex = *fmtIdx;
                            break;
                        } else {
                            spec->type = FormatType::RS;
                            spec->size = FormatSize::SizePointer;  // 4 or 8 depending on LP64
                            return;
                        }
                    case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9': {
                        int64_t number = 0;
                        do {
                            number = 10 * number + (ch - '0');
                            if (cformat) ch = (UniChar)cformat[(*fmtIdx)++]; else ch = uformat[(*fmtIdx)++];
                        } while ((UInt32)(ch - '0') <= 9);
                        if ('$' == ch) {
                            if (-2 == spec->precArgNum) {
                                spec->precArgNum = (int8_t)number - 1;	// Arg numbers start from 1
                            } else if (-2 == spec->widthArgNum) {
                                spec->widthArgNum = (int8_t)number - 1;	// Arg numbers start from 1
                            } else {
                                spec->mainArgNum = (int8_t)number - 1;	// Arg numbers start from 1
                            }
                            break;
                        } else if (seenDot) {	/* else it's either precision or width */
                            spec->precArg = (SInt32)number;
                        } else {
                            spec->widthArg = (SInt32)number;
                        }
                        goto reswtch;
                    }
                    case '*':
                        spec->widthArgNum = -2;
                        break;
                    case '.':
                        seenDot = YES;
                        if (cformat) ch = (UniChar)cformat[(*fmtIdx)++]; else ch = uformat[(*fmtIdx)++];
                        if ('*' == ch) {
                            spec->precArgNum = -2;
                            break;
                        }
                        goto reswtch;
                    default:
                        spec->type = FormatType::Literal;
                        return;
                }
            }
        }
    }
}