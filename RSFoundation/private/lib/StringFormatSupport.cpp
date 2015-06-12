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

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
#define SNPRINTF(TYPE, WHAT) {				\
TYPE value = (TYPE) WHAT;				\
if (-1 != specs[curSpec].widthArgNum) {		\
if (-1 != specs[curSpec].precArgNum) {		\
snprintf_l(buffer, BUFFER_LEN-1, nil, formatBuffer, width, precision, value); \
} else {					\
snprintf_l(buffer, BUFFER_LEN-1, nil, formatBuffer, width, value); \
}						\
} else {						\
if (-1 != specs[curSpec].precArgNum) {		\
snprintf_l(buffer, BUFFER_LEN-1, nil, formatBuffer, precision, value); \
} else {					\
snprintf_l(buffer, BUFFER_LEN-1, nil, formatBuffer, value);	\
}						\
}}
#else
#define SNPRINTF(TYPE, WHAT) {				\
TYPE value = (TYPE) WHAT;				\
if (-1 != specs[curSpec].widthArgNum) {		\
if (-1 != specs[curSpec].precArgNum) {		\
sprintf(buffer, formatBuffer, width, precision, value); \
} else {					\
sprintf(buffer, formatBuffer, width, value); \
}						\
} else {						\
if (-1 != specs[curSpec].precArgNum) {		\
sprintf(buffer, formatBuffer, precision, value); \
} else {					\
sprintf(buffer, formatBuffer, value);	\
}						\
}}
#endif
        
        void StringFormat::AppendFormatCore(RSFoundation::Collection::String *outputString, void *options, RSFoundation::Collection::String *formatString, Index initialArgPosition, const void *origValues, Index orignalValuesSize, va_list args) {
            Index numSpecs, sizeSpecs, sizeArgNum, formatIdx, curSpec, argNum;
            Index formatLen;
#define FORMAT_BUFFER_LEN 400
            const UInt8 *cformat = nil;
            const UniChar *uformat = nil;
            UniChar *formatChars = nil;
            UniChar localFormatBuffer[FORMAT_BUFFER_LEN];
            
#define VPRINTF_BUFFER_LEN 61
            FormatSpec localSpecsBuffer[VPRINTF_BUFFER_LEN];
            FormatSpec *specs;
            PrintValue localValuesBuffer[VPRINTF_BUFFER_LEN];
            PrintValue *values;
            const PrintValue *orignalValues = (const PrintValue *)origValues;
//            RSDictionaryRef localConfigs[VPRINTF_BUFFER_LEN];
//            RSDictionaryRef *configs;
            Index numConfigs;
//            Allocator<<#typename T#>>
//            RSAllocatorRef tmpAlloc = nil;
            intmax_t dummyLocation;	    // A place for %n to do its thing in; should be the widest possible int value
            
            numSpecs = 0;
            sizeSpecs = 0;
            sizeArgNum = 0;
            numConfigs = 0;
            specs = nil;
            values = nil;
//            configs = nil;
            
            formatLen = formatString->GetLength();
            if (!dynamic_cast<String*>(formatString)) {
                if (!formatString->_IsUnicode()) {
                    cformat = (const UInt8 *)formatString->_Contents();
                    if (cformat) {
                        cformat += formatString->_SkipAnyLengthByte();
                    }
                } else {
                    uformat = (const UniChar *)formatString->_Contents();
                }
            }
            
            if (!cformat && !uformat) {
                formatChars = (formatLen > FORMAT_BUFFER_LEN) ? (UniChar *)Allocator<UniChar>::SystemDefault.Allocate<UniChar>(formatLen) : localFormatBuffer;
                // get characters
                uformat = formatChars;
            }

            /* Compute an upper bound for the number of format specifications */
            if (cformat) {
                for (formatIdx = 0; formatIdx < formatLen; formatIdx++) if ('%' == cformat[formatIdx]) sizeSpecs++;
            } else {
                for (formatIdx = 0; formatIdx < formatLen; formatIdx++) if ('%' == uformat[formatIdx]) sizeSpecs++;
            }
            
            specs = ((2 * sizeSpecs + 1) > VPRINTF_BUFFER_LEN) ? Allocator<FormatSpec>::SystemDefault.Allocate<FormatSpec>(2 * sizeSpecs + 1) : localSpecsBuffer;
            //    if (specs != localSpecsBuffer && __RSOASafe) __RSSetLastAllocationEventName(specs, "RSString (temp)");
            
//            configs = ((sizeSpecs < VPRINTF_BUFFER_LEN) ? localConfigs : (RSDictionaryRef *)RSAllocatorAllocate(tmpAlloc, sizeof(RSStringRef) * sizeSpecs));
            
            /* Collect format specification information from the format string */
            for (curSpec = 0, formatIdx = 0; formatIdx < formatLen; curSpec++) {
                Index newFmtIdx;
                specs[curSpec].loc = formatIdx;
                specs[curSpec].len = 0;
                specs[curSpec].size = (FormatSize)0;
                specs[curSpec].type = (FormatType)0;
                specs[curSpec].flags = 0;
                specs[curSpec].widthArg = -1;
                specs[curSpec].precArg = -1;
                specs[curSpec].mainArgNum = -1;
                specs[curSpec].precArgNum = -1;
                specs[curSpec].widthArgNum = -1;
                specs[curSpec].configDictIndex = -1;
                if (cformat) {
                    for (newFmtIdx = formatIdx; newFmtIdx < formatLen && '%' != cformat[newFmtIdx]; newFmtIdx++);
                } else {
                    for (newFmtIdx = formatIdx; newFmtIdx < formatLen && '%' != uformat[newFmtIdx]; newFmtIdx++);
                }
                if (newFmtIdx != formatIdx) {	/* Literal chunk */
                    specs[curSpec].type = FormatType::Literal;
                    specs[curSpec].len = newFmtIdx - formatIdx;
                } else {
                    String* configKey = nullptr;
                    newFmtIdx++;	/* Skip % */
                    ParseFormatSpec(uformat, cformat, &newFmtIdx, (UInt32)formatLen, &(specs[curSpec]), &configKey);
                    if (FormatType::Literal == specs[curSpec].type) {
                        specs[curSpec].loc = formatIdx + 1;
                        specs[curSpec].len = 1;
                    } else {
                        specs[curSpec].len = newFmtIdx - formatIdx;
                    }
                }
                formatIdx = newFmtIdx;
                
                // fprintf(stderr, "specs[%d] = {\n  size = %d,\n  type = %d,\n  loc = %d,\n  len = %d,\n  mainArgNum = %d,\n  precArgNum = %d,\n  widthArgNum = %d\n}\n", curSpec, specs[curSpec].size, specs[curSpec].type, specs[curSpec].loc, specs[curSpec].len, specs[curSpec].mainArgNum, specs[curSpec].precArgNum, specs[curSpec].widthArgNum);
                
            }
            numSpecs = curSpec;
            
            // Max of three args per spec, reasoning thus: 1 width, 1 prec, 1 value
            sizeArgNum = ((nullptr == orignalValues) ? (3 * sizeSpecs + 1) : orignalValuesSize);
            
            values = (sizeArgNum > VPRINTF_BUFFER_LEN) ? Allocator<PrintValue>::SystemDefault.Allocate<PrintValue>(sizeArgNum) : localValuesBuffer;
            //    if (values != localValuesBuffer && __RSOASafe) __RSSetLastAllocationEventName(values, "RSString (temp)");
            memset(values, 0, sizeArgNum * sizeof(PrintValue));
            
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD
            // va_copy is a C99 extension. No support on Windows
            va_list copiedArgs;
            if (numConfigs > 0) va_copy(copiedArgs, args); // we need to preserve the original state for passing down
#endif
            
            /* Compute values array */
            argNum = initialArgPosition;
            for (curSpec = 0; curSpec < numSpecs; curSpec++) {
                Index newMaxArgNum;
                if ((FormatType)0 == specs[curSpec].type) continue;
                if (FormatType::Literal == specs[curSpec].type) continue;
                newMaxArgNum = sizeArgNum;
                if (newMaxArgNum < specs[curSpec].mainArgNum) {
                    newMaxArgNum = specs[curSpec].mainArgNum;
                }
                if (newMaxArgNum < specs[curSpec].precArgNum) {
                    newMaxArgNum = specs[curSpec].precArgNum;
                }
                if (newMaxArgNum < specs[curSpec].widthArgNum) {
                    newMaxArgNum = specs[curSpec].widthArgNum;
                }
                if (sizeArgNum < newMaxArgNum) {
                    if (specs != localSpecsBuffer) Allocator<FormatSpec*>::SystemDefault.Deallocate(specs);
                    if (values != localValuesBuffer) Allocator<PrintValue*>::SystemDefault.Deallocate(values);
                    if (formatChars && (formatChars != localFormatBuffer)) Allocator<decltype(formatChars)>::SystemDefault.Deallocate(formatChars);
                    return;  // more args than we expected!
                }
                /* It is actually incorrect to reorder some specs and not all; we just do some random garbage here */
                if (-2 == specs[curSpec].widthArgNum) {
                    specs[curSpec].widthArgNum = argNum++;
                }
                if (-2 == specs[curSpec].precArgNum) {
                    specs[curSpec].precArgNum = argNum++;
                }
                if (-1 == specs[curSpec].mainArgNum) {
                    specs[curSpec].mainArgNum = argNum++;
                }
                
                values[specs[curSpec].mainArgNum].size = specs[curSpec].size;
                values[specs[curSpec].mainArgNum].type = specs[curSpec].type;
                
                
                if (-1 != specs[curSpec].widthArgNum) {
                    values[specs[curSpec].widthArgNum].size = (FormatSize)0;
                    values[specs[curSpec].widthArgNum].type = FormatType::Long;
                }
                if (-1 != specs[curSpec].precArgNum) {
                    values[specs[curSpec].precArgNum].size = (FormatSize)0;
                    values[specs[curSpec].precArgNum].type = FormatType::Long;
                }
            }
            
            /* Collect the arguments in correct type from vararg list */
            for (argNum = 0; argNum < sizeArgNum; argNum++) {
                if ((nullptr != orignalValues) && ((FormatType)0 == values[argNum].type)) values[argNum] = orignalValues[argNum];
                switch (values[argNum].type) {
                    case (FormatType)0:
                    case FormatType::Literal:
                        break;
                    case FormatType::Long:
                    case FormatType::SingleUnichar:
                        if (FormatSize::Size1 == values[argNum].size) {
                            values[argNum].value.int64Value = (int64_t)(int8_t)va_arg(args, int);
                        } else if (FormatSize::Size2 == values[argNum].size) {
                            values[argNum].value.int64Value = (int64_t)(int16_t)va_arg(args, int);
                        } else if (FormatSize::Size4 == values[argNum].size) {
                            values[argNum].value.int64Value = (int64_t)va_arg(args, int32_t);
                        } else if (FormatSize::Size8 == values[argNum].size) {
                            values[argNum].value.int64Value = (int64_t)va_arg(args, int64_t);
                        } else {
                            values[argNum].value.int64Value = (int64_t)va_arg(args, int);
                        }
                        break;
                    case FormatType::Double:
#if LONG_DOUBLE_SUPPORT
                        if (FormatSize::Size16 == values[argNum].size) {
                            values[argNum].value.longDoubleValue = va_arg(args, long double);
                        } else
#endif
                        {
                            values[argNum].value.doubleValue = va_arg(args, double);
                        }
                        break;
                    case FormatType::Pointer:
                    case FormatType::Object:
                    case FormatType::RS:
                    case FormatType::Unichars:
                    case FormatType::Chars:
                    case FormatType::PascalChars:
                        values[argNum].value.pointerValue = va_arg(args, void *);
                        break;
                    case FormatType::DummyPointer:
                        (void)va_arg(args, void *);	    // Skip the provided argument
                        values[argNum].value.pointerValue = &dummyLocation;
                        break;
                }
            }
            va_end(args);
            
            /* Format the pieces together */
            
            if (nil == orignalValues) {
                orignalValues = values;
                orignalValuesSize = sizeArgNum;
            }
            
            for (curSpec = 0; curSpec < numSpecs; curSpec++) {
                Index width = 0, precision = 0;
                UniChar *up, ch;
                BOOL hasWidth = NO, hasPrecision = NO;
                
                // widthArgNum and widthArg are never set at the same time; same for precArg*
                if (-1 != specs[curSpec].widthArgNum) {
                    width = (SInt32)values[specs[curSpec].widthArgNum].value.int64Value;
                    hasWidth = YES;
                }
                if (-1 != specs[curSpec].precArgNum) {
                    precision = (SInt32)values[specs[curSpec].precArgNum].value.int64Value;
                    hasPrecision = YES;
                }
                if (-1 != specs[curSpec].widthArg) {
                    width = specs[curSpec].widthArg;
                    hasWidth = YES;
                }
                if (-1 != specs[curSpec].precArg) {
                    precision = specs[curSpec].precArg;
                    hasPrecision = YES;
                }
                
                switch (specs[curSpec].type) {
                    case FormatType::Long:
                    case FormatType::Double:
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_WINDOWS
                        //                if (formatOptions && (specs[curSpec].flags & RSStringFormatLocalizable) && (RSGetTypeID(formatOptions) == RSLocaleGetTypeID())) {    // We have a locale, so we do localized formatting
                        //                    if (__RSStringFormatLocalizedNumber(outputString, (RSLocaleRef)formatOptions, values, &specs[curSpec], width, precision, hasPrecision)) break;
                        //                }
                        /* Otherwise fall-thru to the next case! */
#endif
                    case FormatType::Pointer: {
                        char formatBuffer[128];
#if defined(__GNUC__)
                        char buffer[BUFFER_LEN + width + precision];
#else
                        char stackBuffer[BUFFER_LEN];
                        char *dynamicBuffer = nil;
                        char *buffer = stackBuffer;
                        if (256+width+precision > BUFFER_LEN) {
                            dynamicBuffer = (char *)RSAllocatorAllocate(RSAllocatorSystemDefault, 256+width+precision, 0);
                            buffer = dynamicBuffer;
                        }
#endif
                        Index cidx, idx, loc;
                        BOOL appended = NO;
                        loc = specs[curSpec].loc;
                        // In preparation to call snprintf(), copy the format string out
                        if (cformat) {
                            for (idx = 0, cidx = 0; cidx < specs[curSpec].len; idx++, cidx++) {
                                if ('$' == cformat[loc + cidx]) {
                                    for (idx--; '0' <= formatBuffer[idx] && formatBuffer[idx] <= '9'; idx--);
                                } else {
                                    formatBuffer[idx] = cformat[loc + cidx];
                                }
                            }
                        } else {
                            for (idx = 0, cidx = 0; cidx < specs[curSpec].len; idx++, cidx++) {
                                if ('$' == uformat[loc + cidx]) {
                                    for (idx--; '0' <= formatBuffer[idx] && formatBuffer[idx] <= '9'; idx--);
                                } else {
                                    formatBuffer[idx] = (int8_t)uformat[loc + cidx];
                                }
                            }
                        }
                        formatBuffer[idx] = '\0';
                        // Should modify format buffer here if necessary; for example, to translate %qd to
                        // the equivalent, on architectures which do not have %q.
                        buffer[sizeof(buffer) - 1] = '\0';
                        switch (specs[curSpec].type) {
                            case FormatType::Long:
                                if (FormatSize::Size8 == specs[curSpec].size) {
                                    SNPRINTF(int64_t, values[specs[curSpec].mainArgNum].value.int64Value)
                                } else {
                                    SNPRINTF(SInt32, values[specs[curSpec].mainArgNum].value.int64Value)
                                }
                                break;
                            case FormatType::Pointer:
                            case FormatType::DummyPointer:
                                SNPRINTF(void *, values[specs[curSpec].mainArgNum].value.pointerValue)
                                break;
                                
                            case FormatType::Double:
#if LONG_DOUBLE_SUPPORT
                                if (FormatSize::Size16 == specs[curSpec].size) {
                                    SNPRINTF(long double, values[specs[curSpec].mainArgNum].value.longDoubleValue)
                                } else
#endif
                                {
                                    SNPRINTF(double, values[specs[curSpec].mainArgNum].value.doubleValue)
                                }
                                // See if we need to localize the decimal point
//                                if (formatOptions) {	// We have localization info
//#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_WINDOWS || DEPLOYMENT_TARGET_LINUX
//                                    //                            RSStringRef decimalSeparator = (RSGetTypeID(formatOptions) == RSLocaleGetTypeID()) ? (RSStringRef)RSLocaleGetValue((RSLocaleRef)formatOptions, RSLocaleDecimalSeparatorKey) : (RSStringRef)RSDictionaryGetValue(formatOptions, RSSTR("NSDecimalSeparator"));
//#else
//                                    RSStringRef decimalSeparator = RSSTR(".");
//#endif
//                                    RSStringRef decimalSeparator = RSSTR(".");
//                                    if (decimalSeparator != nil)
//                                    {
//                                        // We have a decimal separator in there
//                                        Index decimalPointLoc = 0;
//                                        while (buffer[decimalPointLoc] != 0 && buffer[decimalPointLoc] != '.') decimalPointLoc++;
//                                        if (buffer[decimalPointLoc] == '.') {	// And we have a decimal point in the formatted string
//                                            buffer[decimalPointLoc] = 0;
//                                            RSStringAppendCString(outputString, (const char *)buffer, __RSStringGetEightBitStringEncoding());
//                                            RSStringAppendString(outputString, decimalSeparator);
//                                            RSStringAppendCString(outputString, (const char *)(buffer + decimalPointLoc + 1), __RSStringGetEightBitStringEncoding());
//                                            appended = YES;
//                                        }
//                                    }
//                                }
                                break;
                        }
//                        if (!appended) RSStringAppendCString(outputString, (const char *)buffer, __RSStringGetEightBitStringEncoding());
#if !defined(__GNUC__)
                        if (dynamicBuffer) {
                            RSAllocatorDeallocate(RSAllocatorSystemDefault, dynamicBuffer);
                        }
#endif
                    }
                        break;
                    case FormatType::Literal:
                        if (cformat) {
//                            __RSStringAppendBytes(outputString, (const char *)(cformat+specs[curSpec].loc), specs[curSpec].len, __RSStringGetEightBitStringEncoding());
                        } else {
//                            RSStringAppendCharacters(outputString, uformat+specs[curSpec].loc, specs[curSpec].len);
                        }
                        break;
                    case FormatType::PascalChars:
                    case FormatType::Chars:
                        if (values[specs[curSpec].mainArgNum].value.pointerValue == nil) {
//                            RSStringAppendCString(outputString, "(null)", RSStringEncodingASCII);
                        } else {
                            UInt32 len = 0;
                            const char *str = (const char *)values[specs[curSpec].mainArgNum].value.pointerValue;
                            if (specs[curSpec].type == FormatType::PascalChars)
                            {
                                // Pascal string case
                                len = ((unsigned char *)str)[0];
                                str++;
                                if (hasPrecision && precision < len) len = precision;
                            }
                            else
                            {
                                // C-string case
                                if (!hasPrecision)
                                {
                                    // No precision, so rely on the terminating null character
                                    len = strlen(str);
                                }
                                else
                                {
                                    // Don't blindly call strlen() if there is a precision; the string might not have a terminating null (3131988)
                                    const char *terminatingNull = (const char *)memchr(str, 0, precision);	// Basically strlen() on only the first precision characters of str
                                    if (terminatingNull) {	// There was a null in the first precision characters
                                        len = terminatingNull - str;
                                    } else {
                                        len = precision;
                                    }
                                }
                            }
                            // Since the spec says the behavior of the ' ', '0', '#', and '+' flags is undefined for
                            // '%s', and since we have ignored them in the past, the behavior is hereby cast in stone
                            // to ignore those flags (and, say, never pad with '0' instead of space).
                            if (specs[curSpec].flags & FormatMinusFlag) {
//                                __RSStringAppendBytes(outputString, str, len, String::GetSystemEncoding());
                                if (hasWidth && width > len) {
                                    Index w = width - len;	// We need this many spaces; do it ten at a time
//                                    do {__RSStringAppendBytes(outputString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                }
                            } else {
                                if (hasWidth && width > len) {
                                    Index w = width - len;	// We need this many spaces; do it ten at a time
//                                    do {__RSStringAppendBytes(outputString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                }
//                                __RSStringAppendBytes(outputString, str, len, RSStringGetSystemEncoding());
                            }
                        }
                        break;
                    case FormatType::SingleUnichar:
                        ch = (UniChar)values[specs[curSpec].mainArgNum].value.int64Value;
//                        RSStringAppendCharacters(outputString, &ch, 1);
                        break;
                    case FormatType::Unichars:
                        //??? need to handle width, precision, and padding arguments
                        up = (UniChar *)values[specs[curSpec].mainArgNum].value.pointerValue;
                        if (nil == up) {
//                            RSStringAppendCString(outputString, "(null)", RSStringEncodingASCII);
                        } else {
                            UInt32 len;
                            for (len = 0; 0 != up[len]; len++);
                            // Since the spec says the behavior of the ' ', '0', '#', and '+' flags is undefined for
                            // '%s', and since we have ignored them in the past, the behavior is hereby cast in stone
                            // to ignore those flags (and, say, never pad with '0' instead of space).
                            if (hasPrecision && precision < len) len = precision;
                            if (specs[curSpec].flags & FormatMinusFlag) {
//                                RSStringAppendCharacters(outputString, up, len);
                                if (hasWidth && width > len) {
                                    UInt32 w = width - len;	// We need this many spaces; do it ten at a time
//                                    do {__RSStringAppendBytes(outputString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                }
                            } else {
                                if (hasWidth && width > len) {
                                    UInt32 w = width - len;	// We need this many spaces; do it ten at a time
//                                    do {__RSStringAppendBytes(outputString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                }
//                                RSStringAppendCharacters(outputString, up, len);
                            }
                        }
                        break;
                    case FormatType::RS:
                    case FormatType::Object:
                        if (specs[curSpec].configDictIndex != -1) { // config dict
                            Object *object = nullptr;
                            //                    RSStringRef innerFormat = nil;
                            
                            switch (values[specs[curSpec].mainArgNum].type) {
                                case FormatType::Long:
//                                    object = RSNumberCreate(tmpAlloc, RSNumberLong, &(values[specs[curSpec].mainArgNum].value.int64Value));
                                    break;
                                    
                                case FormatType::Double:
#if LONG_DOUBLE_SUPPORT
                                    if (FormatSize::Size16 == values[specs[curSpec].mainArgNum].size) {
                                        double aValue = values[specs[curSpec].mainArgNum].value.longDoubleValue; // losing precision
//                                        object = RSNumberCreate(tmpAlloc, RSNumberDouble, &aValue);
                                    }
                                    else
#endif
                                    {
//                                        object = RSNumberCreate(tmpAlloc, RSNumberDouble, &(values[specs[curSpec].mainArgNum].value.doubleValue));
                                    }
                                    break;
                                    
                                case FormatType::Pointer:
//                                    object = RSNumberCreate(tmpAlloc, RSNumberLonglong, &(values[specs[curSpec].mainArgNum].value.pointerValue));
                                    break;
                                    
                                case FormatType::PascalChars:
                                case FormatType::Chars:
                                    if (nil != values[specs[curSpec].mainArgNum].value.pointerValue) {
//                                        RSMutableStringRef aString = RSStringCreateMutable(tmpAlloc, 0);
                                        UInt32 len;
                                        const char *str = (const char *)values[specs[curSpec].mainArgNum].value.pointerValue;
                                        if (specs[curSpec].type == FormatType::PascalChars) {
                                            // Pascal string case
                                            len = ((unsigned char *)str)[0];
                                            str++;
                                            if (hasPrecision && precision < len) len = precision;
                                        } else {
                                            // C-string case
                                            if (!hasPrecision) {
                                                // No precision, so rely on the terminating null character
                                                len = strlen(str);
                                            } else {
                                                // Don't blindly call strlen() if there is a precision; the string might not have a terminating null (3131988)
                                                const char *terminatingNull = (const char *)memchr(str, 0, precision);	// Basically strlen() on only the first precision characters of str
                                                if (terminatingNull) {
                                                    // There was a null in the first precision characters
                                                    len = terminatingNull - str;
                                                } else {
                                                    len = precision;
                                                }
                                            }
                                        }
                                        // Since the spec says the behavior of the ' ', '0', '#', and '+' flags is undefined for
                                        // '%s', and since we have ignored them in the past, the behavior is hereby cast in stone
                                        // to ignore those flags (and, say, never pad with '0' instead of space).
                                        if (specs[curSpec].flags & FormatMinusFlag) {
//                                            __RSStringAppendBytes(aString, str, len, RSStringGetSystemEncoding());
                                            if (hasWidth && width > len) {
                                                UInt32 w = width - len;	// We need this many spaces; do it ten at a time
//                                                do {__RSStringAppendBytes(aString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                            }
                                        } else {
                                            if (hasWidth && width > len) {
                                                UInt32 w = width - len;	// We need this many spaces; do it ten at a time
//                                                do {__RSStringAppendBytes(aString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                            }
//                                            __RSStringAppendBytes(aString, str, len, RSStringGetSystemEncoding());
                                        }
                                        
//                                        object = aString;
                                    }
                                    break;
                                    
                                case FormatType::SingleUnichar:
                                    ch = (UniChar)values[specs[curSpec].mainArgNum].value.int64Value;
//                                    object = RSStringCreateWithCharactersNoCopy(tmpAlloc, &ch, 1, RSAllocatorNull);
                                    break;
                                    
                                case FormatType::Unichars:
                                    //??? need to handle width, precision, and padding arguments
                                    up = (UniChar *)values[specs[curSpec].mainArgNum].value.pointerValue;
                                    if (nil != up) {
//                                        RSMutableStringRef aString = RSStringCreateMutable(tmpAlloc, 0);
                                        UInt32 len;
                                        for (len = 0; 0 != up[len]; len++);
                                        // Since the spec says the behavior of the ' ', '0', '#', and '+' flags is undefined for
                                        // '%s', and since we have ignored them in the past, the behavior is hereby cast in stone
                                        // to ignore those flags (and, say, never pad with '0' instead of space).
                                        if (hasPrecision && precision < len) len = precision;
                                        if (specs[curSpec].flags & FormatMinusFlag) {
//                                            RSStringAppendCharacters(aString, up, len);
                                            if (hasWidth && width > len) {
                                                UInt32 w = width - len;	// We need this many spaces; do it ten at a time
//                                                do {__RSStringAppendBytes(aString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                            }
                                        } else {
                                            if (hasWidth && width > len) {
                                                UInt32 w = width - len;	// We need this many spaces; do it ten at a time
//                                                do {__RSStringAppendBytes(aString, "          ", (w > 10 ? 10 : w), RSStringEncodingASCII);} while ((w -= 10) > 0);
                                            }
//                                            RSStringAppendCharacters(aString, up, len);
                                        }
//                                        object = aString;
                                    }
                                    break;
                                    
                                case FormatType::RS:
                                case FormatType::Object:
//                                    if (nil != values[specs[curSpec].mainArgNum].value.pointerValue) object = RSRetain(values[specs[curSpec].mainArgNum].value.pointerValue);
                                    break;
                            }
                            
//                            if (nullptr != object) RSRelease(object);
                            
                        } else if (nil != values[specs[curSpec].mainArgNum].value.pointerValue) {
//                            RSStringRef str = nil;
//                            if (copyDescfunc) {
//                                str = copyDescfunc(values[specs[curSpec].mainArgNum].value.pointerValue, formatOptions);
//                            } else {
                                //                        str = __RSCopyFormattingDescription(values[specs[curSpec].mainArgNum].value.pointerValue, formatOptions);
                                //                        if (nil == str)
                                //                        {
                                //                            str = (RSStringRef)RSDescription(values[specs[curSpec].mainArgNum].value.pointerValue);
                                //                        }
//                                if (RSGetTypeID(values[specs[curSpec].mainArgNum].value.pointerValue) == 7)
//                                    str = RSRetain(values[specs[curSpec].mainArgNum].value.pointerValue);
//                                else
//                                    str = (RSStringRef)RSDescription(values[specs[curSpec].mainArgNum].value.pointerValue);
//                            }
//                            if (str) {
//                                RSStringAppendString(outputString, str);                        
//                                RSRelease(str);
//                            }
//                            else {
//                                RSStringAppendCString(outputString, "(null description)", RSStringEncodingASCII);
//                            }
                        } else {
//                            RSStringAppendCString(outputString, "(null)", RSStringEncodingASCII);
                        }
                        break;
                }
            }
            
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_FREEBSD
            // va_copy is a C99 extension. No support on Windows
            if (numConfigs > 0) va_end(copiedArgs);
#endif
//            if (specs != localSpecsBuffer) RSAllocatorDeallocate(tmpAlloc, specs);
//            if (values != localValuesBuffer) RSAllocatorDeallocate(tmpAlloc, values);
//            if (formatChars && (formatChars != localFormatBuffer)) RSAllocatorDeallocate(tmpAlloc, formatChars);
//            if (configs != localConfigs) RSAllocatorDeallocate(tmpAlloc, configs);
        }
//        void StringFormat::AppendFormatCore(String *outputString, void* options, String *formatString, Index initialArgPosition, const void*origValues, Index orginalValuesSize, va_list args) {
//            
//        }
    }
}