//
//  StringImpl.hpp
//  RSCoreFoundation
//
//  Created by closure on 7/1/15.
//  Copyright © 2015 RetVal. All rights reserved.
//

#ifndef StringImpl_hpp
#define StringImpl_hpp

#include "Object.hpp"
#include "String.hpp"
#include "Allocator.hpp"

namespace RSFoundation {
    namespace Collection {
        class StringImpl : public Object, public virtual Copying, public virtual Hashable {
        public:
            static StringImpl Empty;
            
            static const StringImpl *Create();
            static const StringImpl *Create(const char*cStr, String::Encoding encoding);
            
            StringImpl() {
            };
            
            StringImpl(const char*);
            ~StringImpl();
            
            friend class StringFormat;
            
            friend StringImpl *_CreateInstanceImmutable(Allocator<StringImpl> *allocator,
                                                        const void *bytes,
                                                        Index numBytes,
                                                        String::Encoding encoding,
                                                        bool possiblyExternalFormat,
                                                        bool tryToReduceUnicode,
                                                        bool hasLengthByte,
                                                        bool hasNullByte,
                                                        bool noCopy,
                                                        Allocator<StringImpl> *contentsDeallocator,
                                                        UInt32 reservedFlags);
            
//            friend StringImpl *Allocator<StringImpl>::_AllocateImpl<StringImpl, false>::_Allocate(size_t size);
//            friend class Allocator<StringImpl>;
//            friend class Allocator<StringImpl>::_AllocateImpl<StringImpl, false>;
            
            void *operator new(size_t size) noexcept {
                return nullptr;
            }
            
            void *operator new(size_t size, void *memory) noexcept {
                return memory;
            }
            
            HashCode Hash() const;
            Index GetLength() const;
            const StringImpl *Copy() const;
            
        private:
            enum {
                FreeContentsWhenDoneMask = 0x020,
                FreeContentsWhenDone = 0x020,
                ContentsMask = 0x060,
                HasInlineContents = 0x000,
                NotInlineContentsNoFree = 0x040,		// Don't free
                NotInlineContentsDefaultFree = 0x020,	// Use allocator's free function
                NotInlineContentsCustomFree = 0x060,		// Use a specially provided free function
                HasContentsAllocatorMask = 0x060,
                HasContentsAllocator = 0x060,		// (For mutable strings) use a specially provided allocator
                HasContentsDeallocatorMask = 0x060,
                HasContentsDeallocator = 0x060,
                IsMutableMask = 0x01,
                IsMutable = 0x01,
                IsUnicodeMask = 0x10,
                IsUnicode = 0x10,
                HasNullByteMask = 0x08,
                HasNullByte = 0x08,
                HasLengthByteMask = 0x04,
                HasLengthByte = 0x04,
            };
            
            inline void _SetInfo(UInt32 info) {
                _info1 = info;
            }
            
            inline bool _IsConstant() const {
                return false;
            }
            
            inline bool _IsMutable() const {
                return (_info1 & IsMutableMask) == IsMutable;
            }
            
            inline bool _IsInline() const {
                return (_info1 & ContentsMask) == HasInlineContents;
            }
            
            inline bool _IsFreeContentsWhenDone() const {
                return (_info1 & FreeContentsWhenDoneMask) == FreeContentsWhenDone;
            }
            
            inline bool _HasContentsDeallocator() const {
                return (_info1 & HasContentsDeallocatorMask) == HasContentsDeallocator;
            }
            
            inline bool _IsUnicode() const {
                return (_info1 & IsUnicodeMask) == IsUnicode;
            }
            
            inline bool _IsEightBit() const {
                return (_info1 & IsUnicodeMask) != IsUnicode;
            }
            
            inline bool _HasNullByte() const {
                return (_info1 & HasNullByteMask) == HasNullByte;
            }
            
            inline bool _HasLengthByte() const {
                return (_info1 & HasLengthByteMask) == HasLengthByteMask;
            }
            
            inline bool _HasExplicitLength() const {
                return (_info1 & (IsMutableMask | HasLengthByteMask)) != HasLengthByte;
            }
            
            inline UInt32 _SkipAnyLengthByte() const {
                return _HasLengthByte() ? 1 : 0;
            }
            
            inline const void *_Contents() const {
                if (_IsInline()) {
                    uintptr_t vbase = (uintptr_t)&(this->_variants);
                    uintptr_t offset = (_HasExplicitLength() ? sizeof(Index) : 0);
                    uintptr_t rst = vbase + offset;
                    return (const void *)rst;
                }
                return this->_variants._notInlineImmutable1._buffer;
            }
            
            inline Allocator<StringImpl>** _ContentsDeallocatorPtr() const {
                return _HasExplicitLength() ? (Allocator<StringImpl> **)(&(this->_variants._notInlineImmutable1._contentsDeallocator)) : (Allocator<StringImpl> **)(&this->_variants._notInlineImmutable2._contentsDeallocator);
            }
            
            inline Allocator<StringImpl> *_ContentsDeallocator() const {
                return *_ContentsDeallocatorPtr();
            }
            
            void _SetContentsDeallocator(Allocator<StringImpl> *allocator) {
                *_ContentsDeallocatorPtr() = allocator;
            }
            
            inline Allocator<StringImpl> **_ContentsAllocatorPtr() const {
                return nullptr;
//                return (Allocator<StringImpl> **)&this->_variants._notInlineMutable1._contentsAllocator;
            }
            
            inline Allocator<StringImpl> *_ContentsAllocator() const {
                return *_ContentsAllocatorPtr();
            }
            
            inline void _SetContentsAllocator(Allocator<StringImpl> *allocator) {
                *_ContentsAllocatorPtr() = allocator;
            }
            
            inline Index _Length() const {
                if (_HasExplicitLength()) {
                    if (_IsInline()) {
                        return this->_variants._inline1._length;
                    } else {
                        return this->_variants._notInlineImmutable1._length;
                    }
                } else {
                    return (Index)(*((UInt8 *)_Contents()));
                }
            }
            
            inline Index _Length2(const void *buffer) const {
                if (_HasExplicitLength()) {
                    if (_IsInline()) {
                        return this->_variants._inline1._length;
                    } else {
                        return this->_variants._notInlineImmutable1._length;
                    }
                } else {
                    return (Index)(*((UInt8 *)buffer));
                }
            }
            
            inline void _SetContentPtr(const void *buffer) {
                this->_variants._notInlineImmutable1._buffer = (void *)buffer;
            }
            
            inline void _SetExplicitLength(Index length) {
                if (_IsInline()) {
                    this->_variants._inline1._length = length;
                } else {
                    this->_variants._notInlineImmutable1._length = length;
                }
            }
            
            inline void _SetUnicode() {
                _info1 |= IsUnicode;
            }
            
            inline void _ClearUnicode() {
                _info1 &= ~IsUnicode;
            }
            
            inline void _SetHasLengthAndNullBytes() {
                _info1 |= (HasLengthByte | HasNullByte);
            }
            
            inline void _ClearHasLengthAndNullBytes() {
                _info1 &= ~(HasLengthByte | HasNullByte);
            }
            
            inline bool _IsFixed() const {
                return this->_variants._notInlineMutable1._isFixedCapacity;
            }
            
            inline bool _IsExternalMutable() const {
                return this->_variants._notInlineMutable1._isExternalMutable;
            }
            
            inline bool _HasContentsAllocator() {
                return (_info1 & HasContentsAllocatorMask) == HasContentsAllocator;
            }
            
            inline void _SetIsFixed(bool v = true) {
                this->_variants._notInlineMutable1._isFixedCapacity = v;
            }
            
            inline void _SetIsExternalMutable(bool v = true) {
                this->_variants._notInlineMutable1._isExternalMutable = v;
            }
            
            inline void _SetHasGap(bool v = true) {
                this->_variants._notInlineMutable1._hasGap = v;
            }
            
            inline bool _CapacityProvidedExternally() const {
                return this->_variants._notInlineMutable1._capacityProvidedExternally;
            }
            
            inline void _SetCapacityProvidedExternally(bool v = true) {
                this->_variants._notInlineMutable1._capacityProvidedExternally = v;
            }
            
            inline void _ClearCapacityProvidedExternally() {
                return _SetCapacityProvidedExternally(false);
            }
            
            inline Index _Capacity() const {
                return this->_variants._notInlineMutable1._capacity;
            }
            
            inline void _SetCapacity(Index capacity) {
                this->_variants._notInlineMutable1._capacity = capacity;
            }
            
            inline Index _DesiredCapacity() const {
                return this->_variants._notInlineMutable1._desiredCapacity;
            }
            
            inline void _SetDesiredCapacity(Index desiredCapacity) {
                this->_variants._notInlineMutable1._desiredCapacity = desiredCapacity;
            }
            
            static void *_AllocateMutableContents(const StringImpl &str, size_t size) {
                return new char(size);
            }
            
            /* Basic algorithm is to shrink memory when capacity is SHRINKFACTOR times the required capacity or to allocate memory when the capacity is less than GROWFACTOR times the required capacity.  This function will return -1 if the new capacity is just too big (> LONG_MAX).
             Additional complications are applied in the following order:
             - desiredCapacity, which is the minimum (except initially things can be at zero)
             - rounding up to factor of 8
             - compressing (to fit the number if 16 bits), which effectively rounds up to factor of 256
             - we need to make sure GROWFACTOR computation doesn't suffer from overflow issues on 32-bit, hence the casting to unsigned. Normally for required capacity of C bytes, the allocated space is (3C+1)/2. If C > ULONG_MAX/3, we instead simply return LONG_MAX
             */
#define SHRINKFACTOR(c) (c / 2)
            
#if __LP64__
#define GROWFACTOR(c) ((c * 3 + 1) / 2)
#else
#define GROWFACTOR(c) (((c) >= (ULONG_MAX / 3UL)) ? __Max(LONG_MAX - 4095, (c)) : (((unsigned long)c * 3 + 1) / 2))
#endif
            
            inline Index _NewCapacity(unsigned long reqCapacity, Index capacity, BOOL leaveExtraRoom, Index charSize)
            {
                if (capacity != 0 || reqCapacity != 0)
                {
                    /* If initially zero, and space not needed, leave it at that... */
                    if ((capacity < reqCapacity) ||		/* We definitely need the room... */
                        (!_CapacityProvidedExternally() && 	/* Assuming we control the capacity... */
                         ((reqCapacity < SHRINKFACTOR(capacity)) ||		/* ...we have too much room! */
                          (!leaveExtraRoom && (reqCapacity < capacity)))))
                    {
                        /* ...we need to eliminate the extra space... */
                        if (reqCapacity > LONG_MAX) return -1;  /* Too big any way you cut it */
                        unsigned long newCapacity = leaveExtraRoom ? GROWFACTOR(reqCapacity) : reqCapacity;	/* Grow by 3/2 if extra room is desired */
                        Index desiredCapacity = _DesiredCapacity() * charSize;
                        if (newCapacity < desiredCapacity) {
                            /* If less than desired, bump up to desired */
                            newCapacity = desiredCapacity;
                        } else if (_IsFixed()) {
                            /* Otherwise, if fixed, no need to go above the desired (fixed) capacity */
                            newCapacity = max((unsigned long)desiredCapacity, reqCapacity);	/* !!! So, fixed is not really fixed, but "tight" */
                        }
                        if (_HasContentsAllocator()) {
                            /* Also apply any preferred size from the allocator  */
                            newCapacity = newCapacity;
                            //                            newCapacity = AllocatorSize(__StrContentsAllocator(str), newCapacity);
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
                        } else {
                            newCapacity = malloc_good_size(newCapacity);
#endif
                        }
                        return (newCapacity > LONG_MAX) ? -1 : (Index)newCapacity; // If packing: __StrUnpackNumber(__StrPackNumber(newCapacity));
                    }
                }
                return capacity;
            }
            
            void _DeallocateMutableContents(void *buffer);
            
        private:
            UInt32 _info1;
#ifdef __LP64__
            UInt32 _reserved;
#endif
            struct __notInlineMutable {
                void *_buffer;
                Index _length;
                Index _capacity;                           // Capacity in bytes
                unsigned long _hasGap:1;                      // Currently unused
                unsigned long _isFixedCapacity:1;
                unsigned long _isExternalMutable:1;
                unsigned long _capacityProvidedExternally:1;
#if __LP64__
                unsigned long _desiredCapacity:60;
#else
                unsigned long _desiredCapacity:28;
#endif
//                Allocator<void>* _contentsAllocator;           // Optional
            };
            
            union __variants {
                struct __inline1 {
                    Index _length;
                } _inline1;
                
                struct __inline2 {
                    Index _length;
                    void *_buffer;
                } _inline2;
                
                struct __notInlineImmutable1 {
                    void *_buffer;
                    Index _length;
                    Allocator<StringImpl> *_contentsDeallocator;
                } _notInlineImmutable1;
                
                struct __notInlineImmutable2 {
                    void *_buffer;
                    Allocator<StringImpl> *_contentsDeallocator;
                } _notInlineImmutable2;
                struct __notInlineMutable _notInlineMutable1;
                
                __variants() {
                    
                }
                
                ~__variants() {
                    
                }
            } _variants;
        };
    }
}

#endif /* StringImpl_hpp */
