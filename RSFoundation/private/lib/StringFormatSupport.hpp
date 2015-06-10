//
//  StringFormatSupport.hpp
//  RSCoreFoundation
//
//  Created by closure on 6/10/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef StringFormatSupport_cpp
#define StringFormatSupport_cpp

#include <RSFoundation/String.hpp>

namespace RSFoundation {
    namespace Collection {
        class StringFormat : public Object, public NotCopyable {
        public:
        private:
            enum class FormatType : Int16 {
                Literal = 6,
                UnsignedLong = 7,
                Long = 8,
                Double = 9,
                Pointer = 10,
                Object = 11,
                RS = 12,
                Unichars = 13,
                Char = 14,
                Chars = 15,
                PascalChars = 16,
                SingleUnichar = 17,
                DummyPointer = 18,
            };
            
            enum class NumericFormatStyle : Int8 {
                Decimal = (1 << 0),
                Scientific = (1 << 1),
                DecimalOrScientific = Decimal|Scientific,
                Unsigned = (1 << 2)
            };
            
            enum class FormatSize : Int8 {
                DefaultSize = 0,
                Size1 = 1,
                Size2 = 2,
                Size4 = 3,
                Size8 = 4,
                Size16 = 5,
#if __LP64__
                SizeLong = Size8,
                SizePointer = Size8
#else
                SizeLong = Size4,
                SizePointer = Size4
#endif
            };
            
            struct RSPrintValue {
                Int16 type;
                Int16 size;
                union {
                    Int64 int64Value;
                    double doubleValue;
#if LONG_DOUBLE_SUPPORT
                    long double longDoubleValue;
#endif
                    void *pointerValue;
                } value;
            };
            
            enum {
                FormatZeroFlag = (1 << 0),         // if not, padding is space char
                FormatMinusFlag = (1 << 1),        // if not, no flag implied
                FormatPlusFlag = (1 << 2),         // if not, no flag implied, overrides space
                FormatSpaceFlag = (1 << 3),        // if not, no flag implied
                FormatExternalSpecFlag = (1 << 4), // using config dict
                FormatLocalizable = (1 << 5)       // explicitly mark the specs we can localize
            };
            
            struct FormatSpec {
                FormatSize size;
                FormatType type;
                Index loc;
                Index len;
                Index widthArg;
                Index precArg;
                UInt32 flags;
                Int8 mainArgNum;
                Int8 precArgNum;
                Int8 widthArgNum;
                Int8 configDictIndex;
                UInt16 numericFormatStyle;        // Only set for localizable numeric quantities
            };
            
            const static Index BUFFER_LEN = 512;
            
            void ParseFormatSpec(const UniChar *uformat, const uint8_t *cformat, Index *fmtIdx, Index fmtLen, FormatSpec *spec, String **configKeyPointer);
        };
    }
}
#endif /* StringFormatSupport_cpp */
