//
//  RSAllocator.c
//  RSCoreFoundation
//
//  Created by RetVal on 10/23/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSBase.h>
#include <RSCoreFoundation/RSAllocator.h>
#include <RSCoreFoundation/RSRuntime.h>
#include <sys/mman.h>
#ifndef _WIN32
#ifndef __THROW
#define __THROW
#endif


#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONEOS
#include <malloc/malloc.h>
#endif

#define RSAllocatorUseMallocZone    1
#define RSAllocatorUseTCMalloc      2
#define RSAllocatorUseJEMalloc      3
#define RSAllocatorUseFastMalloc    4

#define RSAllocatorMallocSelector   RSAllocatorUseMallocZone

#if RSAllocatorMallocSelector == RSAllocatorUseTCMalloc
# define ATTRIBUTE_SECTION(name) __attribute__ ((section ("__TEXT, " #name)))

RS_EXTERN_C_BEGIN
    void* tc_malloc(size_t size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    void tc_free(void* ptr) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    void* tc_realloc(void* ptr, size_t size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    void* tc_calloc(size_t nmemb, size_t size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    void tc_cfree(void* ptr) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    
    void* tc_memalign(size_t __alignment, size_t __size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    int tc_posix_memalign(void** ptr, size_t align, size_t size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    void* tc_valloc(size_t __size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    void* tc_pvalloc(size_t __size) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    
    void tc_malloc_stats(void) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    int tc_mallopt(int cmd, int value) __THROW
    ATTRIBUTE_SECTION(google_malloc);
#ifdef HAVE_STRUCT_MALLINFO
    struct mallinfo tc_mallinfo(void) __THROW
    ATTRIBUTE_SECTION(google_malloc);
#endif
    
    void* tc_new(size_t size)
    ATTRIBUTE_SECTION(google_malloc);
    void tc_delete(void* p) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    void* tc_newarray(size_t size)
    ATTRIBUTE_SECTION(google_malloc);
    void tc_deletearray(void* p) __THROW
    ATTRIBUTE_SECTION(google_malloc);
    // Some non-standard extensions that we support.
    
    // This is equivalent to
    //    OS X: malloc_size()
    //    glibc: malloc_usable_size()
    //    Windows: _msize()
    size_t tc_malloc_size(void* p) __THROW
    ATTRIBUTE_SECTION(google_malloc);
RS_EXTERN_C_END // extern "C"
#endif  // #ifndef _WIN32
#endif

static RSSpinLock ___RSAllocatorAllocateUnitNumberLock = RSSpinLockInit;
static RSBitU64 ___RSAllocatorAllocateUnitNumber = 0;

static RSBitU64 __RSAllocatorUpdateUnit(RSBit8 operand)
{
    RSBitU64 result = 0;
    RSSpinLockLock(&___RSAllocatorAllocateUnitNumberLock);
    switch (operand)
    {
        case 0:
            result = ___RSAllocatorAllocateUnitNumber;
            break;
        case 1:
            ___RSAllocatorAllocateUnitNumber++;
#if __RSRuntimeDebugPreference
            if (___RSDebugLogPreference._RSRuntimeInstanceAllocFreeWatcher) {
                __RSCLog(RSLogLevelDebug, "runtime unit - %llu\n", ___RSAllocatorAllocateUnitNumber);
            }
            
#endif
            result = YES;
            break;
        case -1:
            ___RSAllocatorAllocateUnitNumber--;
#if __RSRuntimeDebugPreference
            if (___RSDebugLogPreference._RSRuntimeInstanceAllocFreeWatcher) {
                __RSCLog(RSLogLevelDebug, "runtime unit - %llu\n", ___RSAllocatorAllocateUnitNumber);
            }
#endif
            result = YES;
        default:
            break;
    }
    if (___RSAllocatorAllocateUnitNumber == ~0)
    {
        HALTWithError(RSGenericException, "What the hell was that?! You malloc to much memory blocks!");
    }
    RSSpinLockUnlock(&___RSAllocatorAllocateUnitNumberLock);
    return result;
}

#include <RSCoreFoundation/RSNumber.h>
RSExport void RSAllocatorLogUnitCount()
{
    //RSLog(RSSTR("runtime unit - %R"), RSAutorelease(RSNumberCreateUnsignedLonglong(RSAllocatorDefault, __RSAllocatorUpdateUnit(0))));
    RSBitU64 unit = __RSAllocatorUpdateUnit(0);
    if (unit) __RSCLog(RSLogLevelDebug, "runtime unit - %lld\n", unit);
}

static kern_return_t __RSAllocatorZoneIntrospectNoOp(void) {
    return 0;
}

static BOOL __RSAllocatorZoneIntrospectTrue(void) {
    return 1;
}

static size_t __RSAllocatorCustomGoodSize(malloc_zone_t *zone, size_t size) {
    RSAllocatorRef allocator __unused = (RSAllocatorRef)zone;
    return malloc_good_size(size);//RSAllocatorGetPreferredSizeForSize(allocator, size, 0);
}

static struct malloc_introspection_t __RSAllocatorZoneIntrospect = {
    (void *)__RSAllocatorZoneIntrospectNoOp,
    (void *)__RSAllocatorCustomGoodSize,
    (void *)__RSAllocatorZoneIntrospectTrue,
    (void *)__RSAllocatorZoneIntrospectNoOp,
    (void *)__RSAllocatorZoneIntrospectNoOp,
    (void *)__RSAllocatorZoneIntrospectNoOp,
    (void *)__RSAllocatorZoneIntrospectNoOp,
    (void *)__RSAllocatorZoneIntrospectNoOp
};

#if !(DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_IPHONE)
typedef struct _rs_malloc_zone_t rs_malloc_zone_t;
typedef	int		kern_return_t;

#if defined(__x86_64__) && !defined(KERNEL)
    typedef unsigned int	boolean_t;
#else
    typedef int		boolean_t;
#endif

#ifndef _UINTPTR_T
#define _UINTPTR_T
typedef unsigned long		uintptr_t;
#endif /* _UINTPTR_T */

#ifdef __LP64__
typedef uintptr_t		vm_offset_t;
#else	/* __LP64__ */
typedef	unsigned int    vm_offset_t;
#endif	/* __LP64__ */

#ifdef __LP64__
    typedef uintptr_t		vm_size_t;
#else	/* __LP64__ */
    typedef	unsigned int		vm_size_t;
#endif	/* __LP64__ */

typedef struct rs_malloc_statistics_t {
    unsigned	blocks_in_use;
    size_t	size_in_use;
    size_t	max_size_in_use;	/* high water mark of touched memory */
    size_t	size_allocated;		/* reserved in memory */
} rs_malloc_statistics_t;

typedef vm_offset_t     	vm_address_t;
typedef kern_return_t rs_memory_reader_t(task_t remote_task, vm_address_t remote_address, vm_size_t size, void **local_memory);

typedef struct rs_malloc_introspection_t {
    kern_return_t (*enumerator)(task_t task, void *, unsigned type_mask, vm_address_t zone_address, rs_memory_reader_t reader, vm_range_recorder_t recorder); /* enumerates all the malloc pointers in use */
    size_t	(*good_size)(rs_malloc_zone_t *zone, size_t size);
    boolean_t 	(*check)(rs_malloc_zone_t *zone); /* Consistency checker */
    void 	(*print)(rs_malloc_zone_t *zone, boolean_t verbose); /* Prints zone  */
    void	(*log)(rs_malloc_zone_t *zone, void *address); /* Enables logging of activity */
    void	(*force_lock)(rs_malloc_zone_t *zone); /* Forces locking zone */
    void	(*force_unlock)(rs_malloc_zone_t *zone); /* Forces unlocking zone */
    void	(*statistics)(rs_malloc_zone_t *zone, rs_malloc_statistics_t *stats); /* Fills statistics */
    boolean_t   (*zone_locked)(rs_malloc_zone_t *zone); /* Are any zone locks held */
    
    /* Discharge checking. Present in version >= 7. */
    boolean_t	(*enable_discharge_checking)(rs_malloc_zone_t *zone);
    void	(*disable_discharge_checking)(rs_malloc_zone_t *zone);
    void	(*discharge)(rs_malloc_zone_t *zone, void *memory);
#ifdef RS_BLOCKS_AVAILABLE
    void        (*enumerate_discharged_pointers)(rs_malloc_zone_t *zone, void (^report_discharged)(void *memory, void *info));
#else
    void	*enumerate_unavailable_without_blocks;
#endif /* __BLOCKS__ */
} rs_malloc_introspection_t;

struct _rs_malloc_zone_t {
    /* Only zone implementors should depend on the layout of this structure;
     Regular callers should use the access functions below */
    void	*reserved1;	/* RESERVED FOR CFAllocator DO NOT USE */
    void	*reserved2;	/* RESERVED FOR CFAllocator DO NOT USE */
    size_t 	(*size)(struct _rs_malloc_zone_t *zone, const void *ptr); /* returns the size of a block or 0 if not in this zone; must be fast, especially for negative answers */
    void 	*(*malloc)(struct _rs_malloc_zone_t *zone, size_t size);
    void 	*(*calloc)(struct _rs_malloc_zone_t *zone, size_t num_items, size_t size); /* same as malloc, but block returned is set to zero */
    void 	*(*valloc)(struct _rs_malloc_zone_t *zone, size_t size); /* same as malloc, but block returned is set to zero and is guaranteed to be page aligned */
    void 	(*free)(struct _rs_malloc_zone_t *zone, void *ptr);
    void 	*(*realloc)(struct _rs_malloc_zone_t *zone, void *ptr, size_t size);
    void 	(*destroy)(struct _rs_malloc_zone_t *zone); /* zone is destroyed and all memory reclaimed */
    const char	*zone_name;
    
    /* Optional batch callbacks; these may be nil */
    unsigned	(*batch_malloc)(struct _rs_malloc_zone_t *zone, size_t size, void **results, unsigned num_requested); /* given a size, returns pointers capable of holding that size; returns the number of pointers allocated (maybe 0 or less than num_requested) */
    void	(*batch_free)(struct _rs_malloc_zone_t *zone, void **to_be_freed, unsigned num_to_be_freed); /* frees all the pointers in to_be_freed; note that to_be_freed may be overwritten during the process */
    
    struct rs_malloc_introspection_t	*introspect;
    unsigned	version;
    
    /* aligned memory allocation. The callback may be nil. Present in version >= 5. */
    void *(*memalign)(struct _rs_malloc_zone_t *zone, size_t alignment, size_t size);
    
    /* free a pointer known to be in zone and known to have the given size. The callback may be nil. Present in version >= 6.*/
    void (*free_definite_size)(struct _rs_malloc_zone_t *zone, void *ptr, size_t size);
    
    /* Empty out caches in the face of memory pressure. The callback may be nil. Present in version >= 8. */
    size_t 	(*pressure_relief)(struct _rs_malloc_zone_t *zone, size_t goal);
};

static void *rs_malloc_zone_malloc(rs_malloc_zone_t *zone, size_t size)
{
    if (zone == nil || size == 0) return nil;
    return zone->malloc(zone, size);
}

static void *rs_malloc_zone_calloc(rs_malloc_zone_t *zone, size_t num_items, size_t size)
{
    if (zone == nil || size == 0 || num_items == 0) return nil;
    return zone->calloc(zone, num_items, size);
}

static void *rs_malloc_zone_valloc(rs_malloc_zone_t *zone, size_t size)
{
    if (zone == nil || size == 0) return nil;
    return zone->valloc(zone, size);
}

static void rs_malloc_zone_free(rs_malloc_zone_t *zone, void *ptr)
{
    if (zone == nil || ptr == nil) return;
    return zone->free(zone, ptr);
}
/* Frees pointer in zone; zone must be non-nil */

static void *rs_malloc_zone_realloc(rs_malloc_zone_t *zone, void *ptr, size_t size)
{
    if (zone == nil || ptr == nil || size == 0) return nil;
    return zone->realloc(zone, ptr, size);
}

static void rs_malloc_destroy_zone(rs_malloc_zone_t *zone)
{
    if (zone == nil) return;
    zone->destroy(zone);
}

static const char *rs_malloc_zone_name(rs_malloc_zone_t *zone)
{
    if (!zone) return nil;
    return zone->zone_name;
}
/* Enlarges block if necessary; zone must be non-nil */
/* Returns the zone for a pointer, or nil if not in any zone.
 The ptr must have been returned from a malloc or realloc call. */

static size_t rs_malloc_good_size(size_t size)
{
    return 0;
}

static size_t msize_for_zone(struct _rs_malloc_zone_t *zone, const void *ptr)
{
#if DEPLOYMENT_TARGET_LINUX
    
#elif DEPLOYMENT_TARGET_WINDOWS
    
#endif
    return 0;
}

static void *malloc_for_zone(struct _rs_malloc_zone_t *zone, size_t size)
{
    return malloc(size);
}

static void *calloc_for_zone(struct _rs_malloc_zone_t *zone, size_t num_items, size_t size)
{
    return calloc(num_items, size);
}

static void *valloc_for_zone(struct _rs_malloc_zone_t *zone, size_t size)
{
    return valloc(size);
}

static void free_for_zone(struct _rs_malloc_zone_t *zone, void *ptr)
{
    free(ptr);
}

static void*realloc_for_zone(struct _rs_malloc_zone_t *zone, void *ptr, size_t size)
{
    return realloc(ptr, size);
}

static void destory_for_zone(rs_malloc_zone_t *zone)
{
    return;
}


static rs_malloc_zone_t *rs_malloc_default_zone(void)
{
    static rs_malloc_zone_t _rs_malloc_default_zone = {
        .reserved1 = nil,
        .reserved2 = nil,
        .size   = msize_for_zone,
        .malloc = malloc_for_zone,
        .calloc = calloc_for_zone,
        .valloc = valloc_for_zone,
        .free   = free_for_zone,
        .realloc= realloc_for_zone,
        .destroy= destory_for_zone,
        "RSMallocDefaultZone",
        .batch_malloc = nil,
        .batch_free   = nil,
        .introspect  = (rs_malloc_introspection_t*)&__RSAllocatorZoneIntrospect,
        .version = 6,
        .memalign = nil,
        .free_definite_size = nil
    };
    return &_rs_malloc_default_zone;
}

static size_t rs_malloc_size(const void *ptr)
{
    rs_malloc_zone_t *zone = rs_malloc_default_zone();
    return zone->size(zone, ptr);
}

#endif

typedef struct __RSAllocator
{
    RSRuntimeBase _base;
    size_t 	(*size)(struct _malloc_zone_t *zone, const void *ptr); /* returns the size of a block or 0 if not in this zone; must be fast, especially for negative answers */
    void 	*(*malloc)(struct _malloc_zone_t *zone, size_t size);
    void 	*(*calloc)(struct _malloc_zone_t *zone, size_t num_items, size_t size); /* same as malloc, but block returned is set to zero */
    void 	*(*valloc)(struct _malloc_zone_t *zone, size_t size); /* same as malloc, but block returned is set to zero and is guaranteed to be page aligned */
    void 	(*free)(struct _malloc_zone_t *zone, void *ptr);
    void 	*(*realloc)(struct _malloc_zone_t *zone, void *ptr, size_t size);
    void 	(*destroy)(struct _malloc_zone_t *zone); /* zone is destroyed and all memory reclaimed */
    const char	*zone_name;
    
    /* Optional batch callbacks; these may be nil */
    unsigned	(*batch_malloc)(struct _malloc_zone_t *zone, size_t size, void **results, unsigned num_requested); /* given a size, returns pointers capable of holding that size; returns the number of pointers allocated (maybe 0 or less than num_requested) */
    void	(*batch_free)(struct _malloc_zone_t *zone, void **to_be_freed, unsigned num_to_be_freed); /* frees all the pointers in to_be_freed; note that to_be_freed may be overwritten during the process */
    
    struct malloc_introspection_t	*introspect;
    unsigned	version;
    
    /* aligned memory allocation. The callback may be nil. Present in version >= 5. */
    void *(*memalign)(struct _malloc_zone_t *zone, size_t alignment, size_t size);
    
    /* free a pointer known to be in zone and known to have the given size. The callback may be nil. Present in version >= 6.*/
    void (*free_definite_size)(struct _malloc_zone_t *zone, void *ptr, size_t size);
    
    /* Empty out caches in the face of memory pressure. The callback may be nil. Present in version >= 8. */
    size_t 	(*pressure_relief)(struct _malloc_zone_t *zone, size_t goal);
    
    void * info;
}RSAllocator;

static size_t __RSAllocatorCustomSize(malloc_zone_t *zone, const void *ptr) {
    return malloc_size(ptr);
    
    // The only way to implement this with a version 0 allocator would be
    // for RSAllocator to keep track of all blocks allocated itself, which
    // could be done, but would be bad for performance, so we don't do it.
    //    size_t (*size)(struct _malloc_zone_t *zone, const void *ptr);
    /* returns the size of a block or 0 if not in this zone;
     * must be fast, especially for negative answers */
}

static void *__RSAllocatorCustomMalloc(malloc_zone_t *zone, size_t size) {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
    RSAllocatorRef allocator = (RSAllocatorRef)zone;
#pragma clang diagnostic pop
#if RSAllocatorMallocSelector == RSAllocatorUseMallocZone
    return RSAllocatorAllocate(allocator, size);
#elif RSAllocatorMallocSelector == RSAllocatorUseTCMalloc
    return tc_malloc(size);
#else
    void *ptr = malloc(size);
    __builtin_memset(ptr, 0, size);
    return ptr;
#endif
}

static void *__RSAllocatorCustomCalloc(malloc_zone_t *zone, size_t num_items, size_t size) {
#if RSAllocatorMallocSelector == RSAllocatorUseMallocZone
    RSAllocatorRef allocator = (RSAllocatorRef)zone;
    void* ptr = malloc_zone_calloc((malloc_zone_t*)allocator->info, num_items, size);
#elif RSAllocatorMallocSelector == RSAllocatorUseTCMalloc
    void* ptr = tc_calloc(num_items, size);
#else
    void* ptr = calloc(num_items, size);
#endif
    if (ptr) __RSAllocatorUpdateUnit(1);
    return ptr;
}

extern	vm_size_t	vm_page_size;
#define trunc_page(x)	((x) & (~(vm_page_size - 1)))
#define round_page(x)	trunc_page((x) + (vm_page_size - 1))

static void *__RSAllocatorCustomValloc(malloc_zone_t *zone, size_t size) {
#if RSAllocatorMallocSelector == RSAllocatorUseMallocZone
    RSAllocatorRef allocator = (RSAllocatorRef)zone;
    void* ptr = malloc_zone_valloc((malloc_zone_t*)allocator->info, size);
#elif RSAllocatorMallocSelector == RSAllocatorUseTCMalloc
    void* ptr = tc_valloc(size);
#else
    void* ptr = valloc(size);
#endif
    __builtin_memset(ptr, 0, size);
    if (ptr) __RSAllocatorUpdateUnit(1);
    return ptr;
}

static void __RSAllocatorCustomFree(malloc_zone_t *zone, void *ptr) {
//    RSIndex size = RSAllocatorInstanceSize((RSAllocatorRef)zone, ptr);
//    mprotect(ptr, size, PROT_NONE);
//    mprotect(ptr, size, PROT_READ | PROT_WRITE);
//    madvise(ptr, size, MADV_FREE);
#if RSAllocatorMallocSelector == RSAllocatorUseMallocZone
    RSAllocatorRef allocator = (RSAllocatorRef)zone;
    malloc_zone_free((malloc_zone_t*)allocator->info, (void*)ptr);
#elif RSAllocatorMallocSelector == RSAllocatorUseTCMalloc
    tc_free(ptr);
#else
    free(ptr);
#endif
    __RSAllocatorUpdateUnit(-1);
}
static void __RSAllocatorClassDeallocate(RSTypeRef obj);
static void *__RSAllocatorCustomRealloc(malloc_zone_t *zone, void *ptr, size_t size) {
    RSAllocatorRef allocator = (RSAllocatorRef)zone;
    RSIndex orgSize = RSAllocatorInstanceSize(allocator, ptr);
#if RSAllocatorMallocSelector == RSAllocatorUseMallocZone
    void* newPtr = malloc_zone_realloc((malloc_zone_t*)allocator->info, ptr, size);
#elif RSAllocatorMallocSelector == RSAllocatorUseTCMalloc
    void* newPtr = tc_realloc(ptr, size);
#else
    void *newPtr = realloc(ptr, size);
#endif
    if (size >= orgSize) __builtin_memset(newPtr + orgSize, 0, size - orgSize);
    return newPtr;
}

static void __RSAllocatorCustomDestroy(malloc_zone_t *zone) {
    RSAllocatorRef allocator = (RSAllocatorRef)zone;
    // !!! we do it, and caller of malloc_destroy_zone() assumes
    // COMPLETE responsibility for the result; NO Apple library
    // code should be modified as a result of discovering that
    // some activity results in inconveniences to developers
    // trying to use malloc_destroy_zone() with a RSAllocatorRef;
    // that's just too bad for them.
    __RSAllocatorClassDeallocate(allocator);
}
static void __RSAllocatorNullDestroy(malloc_zone_t *zone) {
}

static const struct __RSRuntimeBase __RSRuntimeClassBase = (const struct __RSRuntimeBase){
    ._rsisa = 0,
    ._rc = 1,
    ._rsinfo._reserved = 0,
    ._rsinfo._special = 1,
    ._rsinfo._objId = 0,
    ._rsinfo._rsinfo1 = 0,
    ._rsinfo._rsinfo = (1 << 1) | (1 << 3)
};

static struct __RSAllocator __RSAllocatorDefault = (const struct __RSAllocator){
    {0},
    __RSAllocatorCustomSize,
    __RSAllocatorCustomMalloc,
    __RSAllocatorCustomCalloc,
    __RSAllocatorCustomValloc,
    __RSAllocatorCustomFree,
    __RSAllocatorCustomRealloc,
    __RSAllocatorNullDestroy,
    "RSAllocatorDefault",
    nil,
    nil,
    &__RSAllocatorZoneIntrospect,
    6,
    nil,
    nil,
};

static struct __RSAllocator __RSAllocatorSystemDefault = {
    {0},
    __RSAllocatorCustomSize,
    __RSAllocatorCustomMalloc,
    __RSAllocatorCustomCalloc,
    __RSAllocatorCustomValloc,
    __RSAllocatorCustomFree,
    __RSAllocatorCustomRealloc,
    __RSAllocatorNullDestroy,
    "RSAllocatorSystemDefault",
    nil,
    nil,
    &__RSAllocatorZoneIntrospect,
    6,
    nil,
    nil,
};

RSAllocatorRef RSAllocatorDefault = &__RSAllocatorDefault;
RSAllocatorRef RSAllocatorSystemDefault = &__RSAllocatorSystemDefault;    // used for buffer allocate(not for instance).


RSAllocatorRef RSAllocatorGetDefault()
{
    return RSAllocatorDefault;
}
void RSAllocatorSetDefault(RSAllocatorRef allocator)
{
    return;
}

void *RSAllocatorAllocate(RSAllocatorRef allocator, RSIndex size)
{
    if (size <= 0) return nil;
    if (allocator == nil) allocator = RSAllocatorDefault;
#if RSAllocatorMallocSelector == RSAllocatorUseFastMalloc
    void *ptr = malloc(size);
    __builtin_memset(ptr, 0, size);
    return ptr;
#else
    return RSAllocatorCallocate(allocator, 1, size);
#endif
}

void *RSAllocatorCallocate(RSAllocatorRef allocator, RSIndex part, RSIndex size)
{
    if (size <= 0 || part <= 0) return nil;
    if (allocator == nil) allocator = RSAllocatorDefault;
#if RSAllocatorMallocSelector == RSAllocatorUseFastMalloc
    void *ptr = calloc(part, size);
#else
    void *ptr = malloc_zone_calloc((malloc_zone_t *)allocator, part, size);
#endif
    return ptr;
}

void  RSAllocatorDeallocate(RSAllocatorRef allocator, const void *ptr)
{
    if (ptr == nil) return ;
    if (allocator == nil) allocator = RSAllocatorDefault;
#if RSAllocatorMallocSelector == RSAllocatorUseFastMalloc
    free((void*)ptr);
#else
    malloc_make_purgeable((void *)ptr);
    malloc_zone_free((malloc_zone_t *)allocator, (void *)ptr);
#endif
}

void *RSAllocatorCopy(RSAllocatorRef allocator, RSIndex newsize, void *ptr, RSIndex needCopy)
{
    if (ptr == nil) return nil;
    if (allocator == nil) allocator = RSAllocatorDefault;
    void* copy = nil;
    size_t size = max(newsize, RSAllocatorInstanceSize(allocator, ptr));
    copy = RSAllocatorAllocate(allocator, size);
    if (copy) memcpy(copy, ptr, min(size, needCopy));
    return copy;
}

void *RSAllocatorReallocate(RSAllocatorRef allocator, void *ptr, RSIndex newsize)
{
    if (ptr == nil) return nil;
    if (allocator == nil) allocator = RSAllocatorDefault;
#if RSAllocatorMallocSelector == RSAllocatorUseFastMalloc
    return realloc(ptr, newsize);
#else
    return malloc_zone_realloc((malloc_zone_t *)allocator, ptr, newsize);
#endif
}

void *RSAllocatorVallocate(RSAllocatorRef allocator, RSIndex size)
{
    if (size <= 0) return nil;
    if (allocator == nil) allocator = RSAllocatorDefault;
#if RSAllocatorMallocSelector == RSAllocatorUseFastMalloc
    return valloc(size);
#else
    return malloc_zone_valloc((malloc_zone_t *)allocator, size);
#endif
}

RSIndex RSAllocatorSize(RSAllocatorRef allocator, RSIndex size)
{
    return malloc_good_size(size);
}

RSIndex RSAllocatorInstanceSize(RSAllocatorRef allocator, RSTypeRef rs)
{
    if (rs == nil) return 0;
    return allocator->size((malloc_zone_t *)allocator, rs);
}

static void __RSAllocatorClassDeallocate(RSTypeRef obj) {
    RSAllocatorRef allocator = (RSAllocatorRef)obj;
    if (allocator == RSAllocatorDefault) HALTWithError(RSInvalidArgumentException, "RSAllocatorDefault can not be deallocated.");
    if (allocator == RSAllocatorSystemDefault) HALTWithError(RSInvalidArgumentException, "RSAllocatorSystemDefault can not be deallocated.");
}

static RSStringRef __RSAllocatorClassDescription(RSTypeRef rs)
{
    return RSStringCreateWithFormat(RSAllocatorDefault, RSSTR("%s - %s <%p>"), __RSRuntimeGetClassNameWithInstance(rs), ((RSAllocatorRef)rs)->zone_name, rs);
}

static RSRuntimeClass __RSAllocatorClass =
{
    _RSRuntimeScannedObject,
    0,
    "RSAllocator",
    nil,
    nil,
    __RSAllocatorClassDeallocate,
    nil,
    nil,
    __RSAllocatorClassDescription,
};

static RSTypeID __RSAllocatorTypeID = _RSRuntimeNotATypeID;

RSPrivate void __RSAllocatorInitialize()
{
    __RSAllocatorTypeID = __RSRuntimeRegisterClass(&__RSAllocatorClass);
    
    __RSAllocatorDefault.info = malloc_default_purgeable_zone();
    __RSAllocatorSystemDefault.info = malloc_default_purgeable_zone();
    
    __RSRuntimeSetClassTypeID(&__RSAllocatorClass, __RSAllocatorTypeID);
    
    __RSRuntimeSetInstanceSpecial(RSAllocatorDefault, YES);
    __RSSetValid(RSAllocatorDefault);
    __RSRuntimeSetInstanceTypeID(RSAllocatorDefault, __RSAllocatorTypeID);
    
    __RSRuntimeSetInstanceSpecial(RSAllocatorSystemDefault, YES);
    __RSSetValid(RSAllocatorSystemDefault);
    __RSRuntimeSetInstanceTypeID(RSAllocatorSystemDefault, __RSAllocatorTypeID);
}

RSTypeID RSAllocatorGetTypeID()
{
    return __RSAllocatorTypeID;
}

RSExport RSAllocatorRef RSGetAllocator(RSTypeRef obj)
{
    __RSRuntimeCheckInstanceAvailable(obj);
    return RSAllocatorDefault;
}

RSExport RSIndex RSGetInstanceSize(RSTypeRef obj)
{
    __RSRuntimeCheckInstanceAvailable(obj);
    return RSAllocatorInstanceSize(RSAllocatorSystemDefault, obj);
}
