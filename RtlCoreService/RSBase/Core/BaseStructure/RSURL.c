//
//  RSURL.c
//  RSCoreFoundation
//
//  Created by RetVal on 12/16/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSString+extension.h>
#include <RSCoreFoundation/RSURL.h>
//#include <RSKit/RSPriv.h>
//#include <RSKit/RSCharacterSetPriv.h>
#include <RSCoreFoundation/RSNumber.h>
#include <RSCoreFoundation/RSInternal.h>
#include "RSPrivate/CString/RSStringInlineBuffer.h"
#include "RSPrivate/RSPrivateOperatingSystem/RSPrivateFileSystem.h"
//#include <RSKit/RSStringEncodingConverter.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
#if DEPLOYMENT_TARGET_MACOSX
//#include <RSKit/RSNumberFormatter.h>
#endif
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <MacTypes.h>
//#include <RSKit/RSURLPriv.h>
#endif


#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
static RSArrayRef HFSPathToURLComponents(RSStringRef path, RSAllocatorRef alloc, BOOL isDir);
static RSStringRef HFSPathToURLPath(RSStringRef path, RSAllocatorRef alloc, BOOL isDir);
#endif
static RSArrayRef WindowsPathToURLComponents(RSStringRef path, RSAllocatorRef alloc, BOOL isDir);
static RSStringRef WindowsPathToURLPath(RSStringRef path, RSAllocatorRef alloc, BOOL isDir);
static RSStringRef POSIXPathToURLPath(RSStringRef path, RSAllocatorRef alloc, BOOL isDirectory);
RSStringRef RSURLCreateStringWithFileSystemPath(RSAllocatorRef allocator, RSURLRef anURL, RSURLPathStyle fsType, BOOL resolveAgainstBase);
RS_EXPORT RSURLRef _RSURLCreateCurrentDirectoryURL(RSAllocatorRef allocator);
#if DEPLOYMENT_TARGET_MACOSX
static BOOL _RSURLHasFileURLScheme(RSURLRef url, BOOL *hasScheme);
BOOL _RSURLIsFileReferenceURL(RSURLRef url);
#endif

// END PUBLIC

// INTERNAL
typedef enum RSURLComponentDecomposition {
	RSURLComponentDecompositionNonHierarchical,
	RSURLComponentDecompositionRFC1808, /* use this for RFC 1738 decompositions as well */
	RSURLComponentDecompositionRFC2396
}RSURLComponentDecomposition;

typedef struct {
	RSStringRef scheme;
	RSStringRef schemeSpecific;
} RSURLComponentsNonHierarchical;

typedef struct {
	RSStringRef scheme;
	RSStringRef user;
	RSStringRef password;
	RSStringRef host;
	RSIndex port; /* RSNotFound means ignore/omit */
	RSArrayRef pathComponents;
	RSStringRef parameterString;
	RSStringRef query;
	RSStringRef fragment;
	RSURLRef baseURL;
} RSURLComponentsRFC1808;

typedef struct {
	RSStringRef scheme;
    
	/* if the registered name form of the net location is used, userinfo is nil, port is RSNotFound, and host is the entire registered name. */
	RSStringRef userinfo;
	RSStringRef host;
	RSIndex port;
    
	RSArrayRef pathComponents;
	RSStringRef query;
	RSStringRef fragment;
	RSURLRef baseURL;
} RSURLComponentsRFC2396;

enum {
    // Resource I/O related errors, with RSErrorURLKey containing URL
    RSURLNoSuchResourceError = 4,			   // Attempt to do a file system operation on a non-existent file
    RSURLResourceLockingError = 255,			   // Couldn't get a lock on file
    RSURLReadUnknownError = 256,                          // Read error (reason unknown)
    RSURLReadNoPermissionError = 257,                     // Read error (permission problem)
    RSURLReadInvalidResourceNameError = 258,              // Read error (invalid file name)
    RSURLReadCorruptResourceError = 259,                  // Read error (file corrupt, bad format, etc)
    RSURLReadNoSuchResourceError = 260,                   // Read error (no such file)
    RSURLReadInapplicableStringEncodingError = 261,       // Read error (string encoding not applicable) also RSStringEncodingErrorKey
    RSURLReadUnsupportedSchemeError = 262,		   // Read error (unsupported URL scheme)
    RSURLReadTooLargeError = 263,			   // Read error (file too large)
    RSURLReadUnknownStringEncodingError = 264,		   // Read error (string encoding of file contents could not be determined)
    RSURLWriteUnknownError = 512,			   // Write error (reason unknown)
    RSURLWriteNoPermissionError = 513,                    // Write error (permission problem)
    RSURLWriteInvalidResourceNameError = 514,             // Write error (invalid file name)
    RSURLWriteInapplicableStringEncodingError = 517,      // Write error (string encoding not applicable) also RSStringEncodingErrorKey
    RSURLWriteUnsupportedSchemeError = 518,		   // Write error (unsupported URL scheme)
    RSURLWriteOutOfSpaceError = 640,                      // Write error (out of storage space)
    RSURLWriteVolumeReadOnlyError = 642,		   // Write error (readonly volume)
};
// END INTERNAL


#ifndef DEBUG_URL_MEMORY_USAGE
#define DEBUG_URL_MEMORY_USAGE 0
#endif

#if DEBUG_URL_MEMORY_USAGE
static RSBitU32 numURLs = 0;
static RSBitU32 numDealloced = 0;
static RSBitU32 numFileURLsCreated = 0;
static RSBitU32 numExtraDataAllocated = 0;
static RSBitU32 numURLsWithBaseURL = 0;
static RSBitU32 numNonUTF8EncodedURLs = 0;
#endif

/* The bit flags in myURL->_flags */
// component bits
#define HAS_SCHEME                      (0x00000001)
#define HAS_USER                        (0x00000002)
#define HAS_PASSWORD                    (0x00000004)
#define HAS_HOST                        (0x00000008)
#define HAS_PORT                        (0x00000010)
#define HAS_PATH                        (0x00000020)
#define HAS_PARAMETERS                  (0x00000040)
#define HAS_QUERY                       (0x00000080)
#define HAS_FRAGMENT                    (0x00000100)
// various boolean flags
#define IS_IPV6_ENCODED                 (0x00000400)
#define IS_DIRECTORY                    (0x00000800)
#define IS_CANONICAL_FILE_URL           (0x00001000)
#define PATH_HAS_FILE_ID                (0x00002000)
#define IS_ABSOLUTE                     (0x00004000)
#define IS_DECOMPOSABLE                 (0x00008000)
#define POSIX_AND_URL_PATHS_MATCH       (0x00010000) // POSIX_AND_URL_PATHS_MATCH will only be YES if the URL path and the POSIX path are identical, character for character, except for the presence/absence of a trailing slash on directories
#define ORIGINAL_AND_URL_STRINGS_MATCH  (0x00020000)
#define USES_EIGHTBITSTRINGENCODING     (0x00040000)
// scheme bits and amount to shift it to translate to the kXXXXScheme enums
#define SCHEME_TYPE_MASK                (0xE0000000)
#define SCHEME_SHIFT                    29
enum {
    kHasUncommonScheme  = 0,
    kHasHttpScheme      = 1,
    kHasHttpsScheme     = 2,
    kHasFileScheme      = 3,
    kHasDataScheme      = 4,
    kHasFtpScheme       = 5,
    kMaxScheme
};
// accessors for the scheme bits in _flags
RSInline RSBitU32 _getSchemeTypeFromFlags(RSBitU32 flags);
RSInline void _setSchemeTypeInFlags(RSBitU32 *flags, RSBitU32 schemeType);

// Other useful defines
#define NET_LOCATION_MASK (HAS_HOST | HAS_USER | HAS_PASSWORD | HAS_PORT)
#define RESOURCE_SPECIFIER_MASK  (HAS_PARAMETERS | HAS_QUERY | HAS_FRAGMENT)
// These flags can be compared for equality since these are all set once when the RSURL is created.
// IS_CANONICAL_FILE_URL cannot be compared since we don't create the URL string.
// POSIX_AND_URL_PATHS_MATCH cannot be compared because it may not be set
// ORIGINAL_AND_URL_STRINGS_MATCH cannot be compared because it gets set on demand later.
#define EQUAL_FLAGS_MASK (HAS_SCHEME | HAS_USER | HAS_PASSWORD | HAS_HOST | HAS_PORT | HAS_PATH | HAS_PARAMETERS | HAS_QUERY | HAS_FRAGMENT | IS_IPV6_ENCODED | IS_DIRECTORY | PATH_HAS_FILE_ID | IS_ABSOLUTE | IS_DECOMPOSABLE | SCHEME_TYPE_MASK )

// The value of FULL_URL_REPRESENTATION must not be in the RSURLPathStyle enums. Also, its value is exposed via _RSURLCopyPropertyListRepresentation to the Finder so don't change it.
#define FULL_URL_REPRESENTATION (0xF)

/* The bit flags in _RSURLAdditionalData->_additionalDataFlags */
/* If ORIGINAL_AND_URL_STRINGS_MATCH in myURL->_flags is NO, these bits determine where they differ. XXXX_DIFFERS must match the HAS_XXXX */
#define SCHEME_DIFFERS                  HAS_SCHEME      // Scheme can actually never differ because if there were escaped characters prior to the colon, we'd interpret the string as a relative path
#define USER_DIFFERS                    HAS_USER
#define PASSWORD_DIFFERS                HAS_PASSWORD
#define HOST_DIFFERS                    HAS_HOST
#define PORT_DIFFERS                    HAS_PORT        // Port can actually never differ because if there were a non-digit following a colon in the net location, we'd interpret the whole net location as the host
#define PATH_DIFFERS                    HAS_PATH        // unused
#define PARAMETERS_DIFFER               HAS_PARAMETERS  // unused
#define QUERY_DIFFER                    HAS_QUERY       // unused
#define FRAGMENT_DIFFER                 HAS_FRAGMENT    // unused

#define FILE_ID_PREFIX ".file"
#define FILE_ID_KEY "id"
#define FILE_ID_PREAMBLE "/.file/id="
#define FILE_ID_PREAMBLE_LENGTH 10

#define FILE_PREFIX "file://"
#define FILE_PREFIX_WITH_AUTHORITY "file://localhost"
static const RSBitU8 fileURLPrefixWithAuthority[] = FILE_PREFIX_WITH_AUTHORITY;

//	In order to reduce the sizeof ( __RSURL ), move these items into a seperate structure which is
//	only allocated when necessary.  In my tests, it's almost never needed -- very rarely does a RSURL have
//	either a sanitized string or a reserved pointer for URLHandle.
struct _RSURLAdditionalData {
    void *_reserved; // Reserved for URLHandle's use.
    RSStringRef _sanitizedString; // The fully compliant RFC string.  This is only non-nil if ORIGINAL_AND_URL_STRINGS_MATCH is NO.
    RSBitU32 _additionalDataFlags; // these flags only apply to things we need to keep state for in _RSURLAdditionalData (like the XXXX_DIFFERS flags)
};

struct __RSURL {
    RSRuntimeBase _rsBase;
    RSBitU32 _flags;
    RSStringEncoding _encoding; // The encoding to use when asked to remove percent escapes
    RSStringRef _string; // Never nil
    RSURLRef _base;
    RSRange *_ranges;
    struct _RSURLAdditionalData* _extra;
    void *_resourceInfo;    // For use by CarbonCore to cache property values. Retained and released by RSURL.
};


RSInline void* _getReserved ( const struct __RSURL* url )
{
    if ( url && url->_extra ) {
        return ( url->_extra->_reserved );
    }
    else {
        return ( nil );
    }
}

RSInline RSStringRef _getSanitizedString(const struct __RSURL* url)
{
    if ( url && url->_extra ) {
        return ( url->_extra->_sanitizedString );
    }
    else {
        return ( nil );
    }
}

RSInline RSBitU32 _getAdditionalDataFlags(const struct __RSURL* url)
{
    if ( url && url->_extra ) {
        return ( url->_extra->_additionalDataFlags );
    }
    else {
        return ( 0 );
    }
}

RSInline void* _getResourceInfo ( const struct __RSURL* url )
{
    if ( url ) {
        return url->_resourceInfo;
    }
    else {
        return nil;
    }
}

static void _RSURLAllocateExtraDataspace( struct __RSURL* url )
{
    if ( url && ! url->_extra )
    {
        struct _RSURLAdditionalData* extra = nil;//(struct _RSURLAdditionalData*) RSAllocatorAllocate( RSGetAllocator( url), sizeof( struct _RSURLAdditionalData ));
        
        extra = RSAllocatorAllocate(RSGetAllocator( url), sizeof(struct _RSURLAdditionalData));
        extra->_reserved = _getReserved( url );
        extra->_additionalDataFlags = _getAdditionalDataFlags(url);
        extra->_sanitizedString = _getSanitizedString(url);
        
        url->_extra = extra;
        
#if DEBUG_URL_MEMORY_USAGE
        numExtraDataAllocated ++;
#endif
    }
}

RSInline void _setReserved ( struct __RSURL* url, void* reserved )
{
    if ( url )
    {
        // Don't allocate extra space if we're just going to be storing nil
        if ( !url->_extra && reserved )
            _RSURLAllocateExtraDataspace( url );
        
        if ( url->_extra )
            __RSAssignWithWriteBarrier((void **)&url->_extra->_reserved, reserved);
    }
}

RSInline void _setSanitizedString( struct __RSURL* url, RSMutableStringRef sanitizedString )
{
    if ( url )
    {
        // Don't allocate extra space if we're just going to be storing nil
        if ( !url->_extra && sanitizedString ) {
            _RSURLAllocateExtraDataspace( url );
        }
        
        if ( url->_extra ) {
            if ( url->_extra->_sanitizedString ) {
                RSRelease(url->_extra->_sanitizedString);
            }
            url->_extra->_sanitizedString = RSStringCreateCopy(RSGetAllocator(url), sanitizedString);
            
        }
    }
}

RSInline void _setAdditionalDataFlags(struct __RSURL* url, RSBitU32 additionalDataFlags)
{
    if ( url )
    {
        // Don't allocate extra space if we're just going to be storing 0
        if ( !url->_extra && (additionalDataFlags != 0) ) {
            _RSURLAllocateExtraDataspace( url );
        }
        
        if ( url->_extra ) {
            url->_extra->_additionalDataFlags = additionalDataFlags;
        }
    }
}

RSInline void _setResourceInfo ( struct __RSURL* url, void* resourceInfo )
{
    // Must be atomic
    // Never a GC object
    if ( url && OSAtomicCompareAndSwapPtrBarrier( nil, resourceInfo, &url->_resourceInfo )) {
        RSRetain( resourceInfo );
    }
}

RSInline RSBitU32 _getSchemeTypeFromFlags(RSBitU32 flags)
{
    return ( (flags & SCHEME_TYPE_MASK) >> SCHEME_SHIFT );
}

RSInline void _setSchemeTypeInFlags(RSBitU32 *flags, RSBitU32 schemeType)
{
    RSAssert2((schemeType >= kHasUncommonScheme) &&  (schemeType < kMaxScheme), __RSLogAssertion, "%s(): Received bad schemeType %d", __PRETTY_FUNCTION__, schemeType);
    *flags = (*flags & ~SCHEME_TYPE_MASK) + (schemeType << SCHEME_SHIFT);
}

/* Returns whether the provided bytes can be stored in ASCII
 */
static BOOL __RSBytesInASCII(const uint8_t *bytes, RSIndex len) {
#if __LP64__
    /* A bit of unrolling; go by 32s, 16s, and 8s first */
    while (len >= 32) {
        uint64_t val = *(const uint64_t *)bytes;
        uint64_t hiBits = (val & 0x8080808080808080ULL);    // More efficient to collect this rather than do a conditional at every step
        bytes += 8;
        val = *(const uint64_t *)bytes;
        hiBits |= (val & 0x8080808080808080ULL);
        bytes += 8;
        val = *(const uint64_t *)bytes;
        hiBits |= (val & 0x8080808080808080ULL);
        bytes += 8;
        val = *(const uint64_t *)bytes;
        if (hiBits | (val & 0x8080808080808080ULL)) return NO;
        bytes += 8;
        len -= 32;
    }
    
    while (len >= 16) {
        uint64_t val = *(const uint64_t *)bytes;
        uint64_t hiBits = (val & 0x8080808080808080ULL);
        bytes += 8;
        val = *(const uint64_t *)bytes;
        if (hiBits | (val & 0x8080808080808080ULL)) return NO;
        bytes += 8;
        len -= 16;
    }
    
    while (len >= 8) {
        uint64_t val = *(const uint64_t *)bytes;
        if (val & 0x8080808080808080ULL) return NO;
        bytes += 8;
        len -= 8;
    }
#endif
    /* Go by 4s */
    while (len >= 4) {
        uint32_t val = *(const uint32_t *)bytes;
        if (val & 0x80808080U) return NO;
        bytes += 4;
        len -= 4;
    }
    /* Handle the rest one byte at a time */
    while (len--) {
        if (*bytes++ & 0x80) return NO;
    }
    
    return YES;
}

static BOOL _pathHasFileIDPrefix(RSStringRef path);
static void _convertToURLRepresentation(struct __RSURL *url, RSBitU32 fsType);
static RSStringRef _resolveFileSystemPaths(RSStringRef relativePath, RSStringRef basePath, BOOL baseIsDir, RSURLPathStyle fsType, RSAllocatorRef alloc);
static void _parseComponents(RSAllocatorRef alloc, RSStringRef string, RSURLRef base, RSBitU32 *flags, RSRange **range);
static RSRange _rangeForComponent(RSBitU32 flags, RSRange *ranges, RSBitU32 compFlag);
static RSRange _netLocationRange(RSBitU32 flags, RSRange *ranges);
static RSBitU32 _firstResourceSpecifierFlag(RSBitU32 flags);
static void computeSanitizedString(RSURLRef url);
static RSStringRef correctedComponent(RSStringRef component, RSBitU32 compFlag, RSStringEncoding enc);
static RSMutableStringRef resolveAbsoluteURLString(RSAllocatorRef alloc, RSStringRef relString, RSBitU32 relFlags, RSRange *relRanges, RSStringRef baseString, RSBitU32 baseFlags, RSRange *baseRanges);
static RSStringRef _resolvedPath(UniChar *pathStr, UniChar *end, UniChar pathDelimiter, BOOL stripLeadingDotDots, BOOL stripTrailingDelimiter, RSAllocatorRef alloc);


RSInline void _parseComponentsOfURL(RSURLRef url) {
    _parseComponents(RSGetAllocator(url), url->_string, url->_base, &(((struct __RSURL *)url)->_flags), &(((struct __RSURL *)url)->_ranges));
}

enum {
	VALID = 1,
	UNRESERVED = 2,
	PATHVALID = 4,
	SCHEME = 8,
	HEXDIGIT = 16
};

static const unsigned char sURLValidCharacters[128] = {
    /* nul   0 */   0,
    /* soh   1 */   0,
    /* stx   2 */   0,
    /* etx   3 */   0,
    /* eot   4 */   0,
    /* enq   5 */   0,
    /* ack   6 */   0,
    /* bel   7 */   0,
    /* bs    8 */   0,
    /* ht    9 */   0,
    /* nl   10 */   0,
    /* vt   11 */   0,
    /* np   12 */   0,
    /* cr   13 */   0,
    /* so   14 */   0,
    /* si   15 */   0,
    /* dle  16 */   0,
    /* dc1  17 */   0,
    /* dc2  18 */   0,
    /* dc3  19 */   0,
    /* dc4  20 */   0,
    /* nak  21 */   0,
    /* syn  22 */   0,
    /* etb  23 */   0,
    /* can  24 */   0,
    /* em   25 */   0,
    /* sub  26 */   0,
    /* esc  27 */   0,
    /* fs   28 */   0,
    /* gs   29 */   0,
    /* rs   30 */   0,
    /* us   31 */   0,
    /* sp   32 */   0,
    /* '!'  33 */   VALID | UNRESERVED | PATHVALID ,
    /* '"'  34 */   0,
    /* '#'  35 */   0,
    /* '$'  36 */   VALID | PATHVALID ,
    /* '%'  37 */   0,
    /* '&'  38 */   VALID | PATHVALID ,
    /* '''  39 */   VALID | UNRESERVED | PATHVALID ,
    /* '('  40 */   VALID | UNRESERVED | PATHVALID ,
    /* ')'  41 */   VALID | UNRESERVED | PATHVALID ,
    /* '*'  42 */   VALID | UNRESERVED | PATHVALID ,
    /* '+'  43 */   VALID | SCHEME | PATHVALID ,
    /* ','  44 */   VALID | PATHVALID ,
    /* '-'  45 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* '.'  46 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* '/'  47 */   VALID | PATHVALID ,
    /* '0'  48 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* '1'  49 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* '2'  50 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* '3'  51 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* '4'  52 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* '5'  53 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* '6'  54 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* '7'  55 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* '8'  56 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* '9'  57 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* ':'  58 */   VALID ,
    /* ';'  59 */   VALID ,
    /* '<'  60 */   0,
    /* '='  61 */   VALID | PATHVALID ,
    /* '>'  62 */   0,
    /* '?'  63 */   VALID ,
    /* '@'  64 */   VALID ,
    /* 'A'  65 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'B'  66 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'C'  67 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'D'  68 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'E'  69 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'F'  70 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'G'  71 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'H'  72 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'I'  73 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'J'  74 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'K'  75 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'L'  76 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'M'  77 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'N'  78 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'O'  79 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'P'  80 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'Q'  81 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'R'  82 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'S'  83 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'T'  84 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'U'  85 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'V'  86 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'W'  87 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'X'  88 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'Y'  89 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'Z'  90 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* '['  91 */   0,
    /* '\'  92 */   0,
    /* ']'  93 */   0,
    /* '^'  94 */   0,
    /* '_'  95 */   VALID | UNRESERVED | PATHVALID ,
    /* '`'  96 */   0,
    /* 'a'  97 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'b'  98 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'c'  99 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'd' 100 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'e' 101 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'f' 102 */   VALID | UNRESERVED | SCHEME | PATHVALID | HEXDIGIT ,
    /* 'g' 103 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'h' 104 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'i' 105 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'j' 106 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'k' 107 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'l' 108 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'm' 109 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'n' 110 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'o' 111 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'p' 112 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'q' 113 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'r' 114 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 's' 115 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 't' 116 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'u' 117 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'v' 118 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'w' 119 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'x' 120 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'y' 121 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* 'z' 122 */   VALID | UNRESERVED | SCHEME | PATHVALID ,
    /* '{' 123 */   0,
    /* '|' 124 */   0,
    /* '}' 125 */   0,
    /* '~' 126 */   VALID | UNRESERVED | PATHVALID ,
    /* del 127 */   0,
};

RSInline BOOL isURLLegalCharacter(UniChar ch) {
    return (ch <= 127) ? (sURLValidCharacters[ch] & VALID) : NO;
}

RSInline BOOL scheme_valid(UniChar ch) {
    return (ch <= 127) ? (sURLValidCharacters[ch] & SCHEME) : NO;
}

// "Unreserved" as defined by RFC 2396
RSInline BOOL isUnreservedCharacter(UniChar ch) {
    return (ch <= 127) ? (sURLValidCharacters[ch] & UNRESERVED) : NO;
}

RSInline BOOL isPathLegalCharacter(UniChar ch) {
    return (ch <= 127) ? (sURLValidCharacters[ch] & PATHVALID) : NO;
}

RSInline BOOL isHexDigit(UniChar ch) {
    return (ch <= 127) ? (sURLValidCharacters[ch] & HEXDIGIT) : NO;
}

// Returns NO if ch1 or ch2 isn't properly formatted
RSInline BOOL _translateBytes(UniChar ch1, UniChar ch2, uint8_t *result) {
    *result = 0;
    if (ch1 >= '0' && ch1 <= '9') *result += (ch1 - '0');
    else if (ch1 >= 'a' && ch1 <= 'f') *result += 10 + ch1 - 'a';
    else if (ch1 >= 'A' && ch1 <= 'F') *result += 10 + ch1 - 'A';
    else return NO;
    
    *result  = (*result) << 4;
    if (ch2 >= '0' && ch2 <= '9') *result += (ch2 - '0');
    else if (ch2 >= 'a' && ch2 <= 'f') *result += 10 + ch2 - 'a';
    else if (ch2 >= 'A' && ch2 <= 'F') *result += 10 + ch2 - 'A';
    else return NO;
    
    return YES;
}

RSInline BOOL _haveTestedOriginalString(RSURLRef url) {
    return ((url->_flags & ORIGINAL_AND_URL_STRINGS_MATCH) != 0) || (_getSanitizedString(url) != nil);
}
#include "RSPrivate/CString/RSString/RSFoundationEncoding.h"
/*
 CreateStringFromFileSystemRepresentationByAddingPercentEscapes creates a RSString
 for the path-absolute form of a URI path component from the native file system representation.
 
 The rules for path-absolute from rfc3986 are:
 path-absolute = "/" [ segment-nz *( "/" segment ) ]
 segment       = *pchar
 segment-nz    = 1*pchar
 pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
 pct-encoded   = "%" HEXDIG HEXDIG
 unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
 sub-delims    = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
 */
static RSStringRef CreateStringFromFileSystemRepresentationByAddingPercentEscapes(RSAllocatorRef alloc, const RSBitU8 *bytes, RSIndex numBytes, BOOL windowsPath)
{
    static const RSBitU8 hexchars[] = "0123456789ABCDEF";
    STACK_BUFFER_DECL(RSBitU8, stackBuf, PATH_MAX * 3);   // worst case is every byte needs to be percent-escaped
    RSBitU8 *bufStartPtr;
    RSBitU8 *bufBytePtr;
    const RSBitU8 *bytePtr = bytes;
    RSIndex idx;
    RSStringRef result;
    
    // choose a buffer to percent-escape into.
    if ( numBytes <= PATH_MAX ) {
        bufStartPtr = &stackBuf[0];
    }
    else {
        // worst case is every byte needs to be percent-escaped (numBytes * 3)
        bufStartPtr = (RSBitU8 *)malloc(numBytes * 3);
    }
    
    if ( bufStartPtr != nil ) {
        bufBytePtr = bufStartPtr;
        for ( idx = 0; (idx < numBytes) && (*bytePtr != 0); ++idx ) {
            switch ( *bytePtr ) {
                    // these are the visible 7-bit ascii characters that are not legal pchar octets
                case '"':
                case '#':
                case '%':
                case ';':	// we need to percent-escape ';' in file system paths so it won't be mistaken for the start of the obsolete param rule (rfc2396) that RSURL still supports
                case '<':
                case '>':
                case '?':	// we need to percent-escape '?' in file system paths so it won't be mistaken for the start of a query
                case '[':
                case '\\':
                case ']':
                case '^':
                case '`':
                case '{':
                case '|':
                case '}':
                    // percent-escape non-pchar octets spread throughout the visible 7-bit ascii range
                    *bufBytePtr++ = '%';
                    *bufBytePtr++ = hexchars[*bytePtr >> 4];
                    *bufBytePtr++ = hexchars[*bytePtr & 0x0f];
                    break;
                default:
                    if ( (*bytePtr <= ' ') ||	// percent-escape non-pchar octets that are space or less (control characters)
                        (*bytePtr >= 0x7f) ||	// percent-escape non-pchar octets that del and 8-bit ascii with the high bit set
                        (windowsPath && (*bytePtr == '/')) ) {	// percent-escape the forward slash if this is a windowsPath
                        *bufBytePtr++ = '%';
                        *bufBytePtr++ = hexchars[*bytePtr >> 4];
                        *bufBytePtr++ = hexchars[*bytePtr & 0x0f];
                    }
                    else {
                        // copy everything else
                        *bufBytePtr++ = *bytePtr;
                    }
                    break;
            }
            ++bytePtr;
        }
        
        // did we convert numBytes?
        if ( idx == numBytes )
        {
            // create the result
            result = RSStringCreateWithBytes(alloc, bufStartPtr, (RSIndex)(bufBytePtr-bufStartPtr), RSStringEncodingUTF8, NO);
        }
        else
        {
            // no, but it's OK if the remaining bytes are all nul (embedded nul bytes are not allowed)
            for ( /* start where we left off */; (idx < numBytes) && (bufStartPtr[idx] == 0); ++idx ) {
                // do nothing
            }
            if ( idx == numBytes ) {
                // create the result
                result = RSStringCreateWithBytes(alloc, bufStartPtr, (RSIndex)(bufBytePtr-bufStartPtr), RSStringEncodingUTF8, NO);
            }
            else
            {
                // the remaining bytes were not all nul
                result = nil;
            }
        }
        
        // free the buffer if we malloc'd it
        if ( bufStartPtr != &stackBuf[0] ) {
            free(bufStartPtr);
        }
    }
    else {
        result = nil;
    }
    return ( result );
}

// Returns nil if str cannot be converted for whatever reason, str if str contains no characters in need of escaping, or a newly-created string with the appropriate % escape codes in place.  Caller must always release the returned string.
RSInline RSStringRef _replacePathIllegalCharacters(RSStringRef str, RSAllocatorRef alloc, BOOL preserveSlashes) {
    RSStringRef result = NULL;
    STACK_BUFFER_DECL(char, buffer, PATH_MAX);
    if ( RSStringGetCString(str, buffer, PATH_MAX, RSStringEncodingUTF8) )
    {
        result = CreateStringFromFileSystemRepresentationByAddingPercentEscapes(RSAllocatorSystemDefault, (const RSBitU8 *)buffer, strlen(buffer), !preserveSlashes);
    }
    return result;
}
    
#include "RSPrivate/CString/RSString/RSStringEncodingConverter.h"

// We have 2 UTF8Chars of a surrogate; we must convert to the correct percent-encoded UTF8 string and append to str.  Added so that file system URLs can always be converted from POSIX to full URL representation.  -- REW, 8/20/2001
static BOOL _hackToConvertSurrogates(UniChar highChar, UniChar lowChar, RSMutableStringRef str)
{
    UniChar surrogate[2];
    uint8_t bytes[6]; // Aki sez it should never take more than 6 bytes
    RSIndex len;
    uint8_t *currByte;
    surrogate[0] = highChar;
    surrogate[1] = lowChar;
    if (RSStringEncodingUnicodeToBytes(RSStringEncodingUTF8, 0, surrogate, 2, nil, bytes, 6, &len) != RSStringEncodingConversionSuccess) {
        return NO;
    }
    for (currByte = bytes; currByte < bytes + len; currByte ++) {
        UniChar escapeSequence[3] = {'%', '\0', '\0'};
        unsigned char high, low;
        high = ((*currByte) & 0xf0) >> 4;
        low = (*currByte) & 0x0f;
        escapeSequence[1] = (high < 10) ? '0' + high : 'A' + high - 10;
        escapeSequence[2] = (low < 10) ? '0' + low : 'A' + low - 10;
        RSStringAppendCharacters(str, escapeSequence, 3);
    }
    return YES;
}

static BOOL _appendPercentEscapesForCharacter(UniChar ch, RSStringEncoding encoding, RSMutableStringRef str) {
    uint8_t bytes[6]; // 6 bytes is the maximum a single character could require in UTF8 (most common case); other encodings could require more
    uint8_t *bytePtr = bytes, *currByte;
    RSIndex byteLength;
    RSAllocatorRef alloc = nil;
    if (RSStringEncodingUnicodeToBytes(encoding, 0, &ch, 1, nil, bytePtr, 6, &byteLength) != RSStringEncodingConversionSuccess) {
        byteLength = RSStringEncodingByteLengthForCharacters(encoding, 0, &ch, 1);
        if (byteLength <= 6) {
            // The encoding cannot accomodate the character
            return NO;
        }
        alloc = RSGetAllocator(str);
        bytePtr = (uint8_t *)RSAllocatorAllocate(alloc, byteLength);
        if (!bytePtr || RSStringEncodingUnicodeToBytes(encoding, 0, &ch, 1, nil, bytePtr, byteLength, &byteLength) != RSStringEncodingConversionSuccess) {
            if (bytePtr) RSAllocatorDeallocate(alloc, bytePtr);
            return NO;
        }
    }
    for (currByte = bytePtr; currByte < bytePtr + byteLength; currByte ++) {
        UniChar escapeSequence[3] = {'%', '\0', '\0'};
        unsigned char high, low;
        high = ((*currByte) & 0xf0) >> 4;
        low = (*currByte) & 0x0f;
        escapeSequence[1] = (high < 10) ? '0' + high : 'A' + high - 10;
        escapeSequence[2] = (low < 10) ? '0' + low : 'A' + low - 10;
        RSStringAppendCharacters(str, escapeSequence, 3);
    }
    if (bytePtr != bytes) {
        RSAllocatorDeallocate(alloc, bytePtr);
    }
    return YES;
}

// Uses UTF-8 to translate all percent escape sequences; returns nil if it encounters a format failure.  May return the original string.
RSExport RSStringRef  RSURLCreateStringByReplacingPercentEscapes(RSAllocatorRef alloc, RSStringRef  originalString, RSStringRef  charactersToLeaveEscaped) {
    RSMutableStringRef newStr = nil;
    RSIndex length;
    RSIndex mark = 0;
    RSRange percentRange, searchRange;
    RSStringRef escapedStr = nil;
    RSMutableStringRef strForEscapedChar = nil;
    UniChar escapedChar;
    BOOL escapeAll = (charactersToLeaveEscaped && RSStringGetLength(charactersToLeaveEscaped) == 0);
    BOOL failed = NO;
    
    if (!originalString) return nil;
    
    if (charactersToLeaveEscaped == nil) {
        return (RSStringRef)RSStringCreateCopy(alloc, originalString);
    }
    
    length = RSStringGetLength(originalString);
    searchRange = RSMakeRange(0, length);
    
    while (!failed && RSStringFindWithOptions(originalString, RSSTR("%"), searchRange, 0, &percentRange)) {
        uint8_t bytes[4]; // Single UTF-8 character could require up to 4 bytes.
        uint8_t numBytesExpected;
        UniChar ch1, ch2;
        
        escapedStr = nil;
        // Make sure we have at least 2 more characters
        if (length - percentRange.location < 3) { failed = YES; break; }
        
        // if we don't have at least 2 more characters, we can't interpret the percent escape code,
        // so we assume the percent character is legit, and let it pass into the string
        ch1 = RSStringGetCharacterAtIndex(originalString, percentRange.location+1);
        ch2 = RSStringGetCharacterAtIndex(originalString, percentRange.location+2);
        if (!_translateBytes(ch1, ch2, bytes)) { failed = YES;  break; }
        if (!(bytes[0] & 0x80)) {
            numBytesExpected = 1;
        } else if (!(bytes[0] & 0x20)) {
            numBytesExpected = 2;
        } else if (!(bytes[0] & 0x10)) {
            numBytesExpected = 3;
        } else {
            numBytesExpected = 4;
        }
        if (numBytesExpected == 1) {
            // one byte sequence (most common case); handle this specially
            escapedChar = bytes[0];
            if (!strForEscapedChar) {
                strForEscapedChar = RSStringCreateMutableWithExternalCharactersNoCopy(alloc, &escapedChar, 1, 1, RSAllocatorDefault);
            }
            escapedStr = (RSStringRef)RSRetain(strForEscapedChar);
        } else {
            RSIndex j;
            // Make sure up front that we have enough characters
            if (length < percentRange.location + numBytesExpected * 3) { failed = YES; break; }
            for (j = 1; j < numBytesExpected; j ++) {
                if (RSStringGetCharacterAtIndex(originalString, percentRange.location + 3*j) != '%') { failed = YES; break; }
                ch1 = RSStringGetCharacterAtIndex(originalString, percentRange.location + 3*j + 1);
                ch2 = RSStringGetCharacterAtIndex(originalString, percentRange.location + 3*j + 2);
                if (!_translateBytes(ch1, ch2, bytes+j)) { failed = YES; break; }
            }
            
            // !!! We should do the low-level bit-twiddling ourselves; this is expensive!  REW, 6/10/99
            escapedStr = RSStringCreateWithBytes(alloc, bytes, numBytesExpected, RSStringEncodingUTF8, NO);
            if (!escapedStr) {
                failed = YES;
            } else if (RSStringGetLength(escapedStr) == 0 && numBytesExpected == 3 && bytes[0] == 0xef && bytes[1] == 0xbb && bytes[2] == 0xbf) {
                // Somehow, the UCS-2 BOM got translated in to a UTF8 string
                escapedChar = 0xfeff;
                if (!strForEscapedChar) {
                    strForEscapedChar = RSStringCreateMutableWithExternalCharactersNoCopy(alloc, &escapedChar, 1, 1, RSAllocatorDefault);
                }
                RSRelease(escapedStr);
                escapedStr = (RSStringRef)RSRetain(strForEscapedChar);
            }
            if (failed) break;
        }
        
        // The new character is in escapedChar; the number of percent escapes it took is in numBytesExpected.
        searchRange.location = percentRange.location + 3 * numBytesExpected;
        searchRange.length = length - searchRange.location;
        
        if (!escapeAll)
        {
            RSRange result = {RSNotFound};
            if (RSStringFind(charactersToLeaveEscaped, escapedStr, RSStringGetRange(charactersToLeaveEscaped), &result) && result.location != RSNotFound) {
                if (escapedStr) {
                    RSRelease(escapedStr);
                    escapedStr = nil;
                }
                continue;
            }
        }
        
        if (!newStr) {
            newStr = RSStringCreateMutable(alloc, length);
        }
        if (percentRange.location - mark > 0) {
            // The creation of this temporary string is unfortunate.
            RSStringRef substring = nil;//RSStringCreateWithSubstring(alloc, originalString, RSMakeRange(mark, percentRange.location - mark));
            
            RSStringAppendString(newStr, substring);
            RSRelease(substring);
        }
        RSStringAppendString(newStr, escapedStr);
        if (escapedStr) {
            RSRelease(escapedStr);
            escapedStr = nil;
        }
        mark = searchRange.location;// We need mark to be the index of the first character beyond the escape sequence
    }
    
    if (escapedStr) RSRelease(escapedStr);
    if (strForEscapedChar) RSRelease(strForEscapedChar);
    if (failed) {
        if (newStr) RSRelease(newStr);
        return nil;
    } else if (newStr) {
        if (mark < length) {
            // Need to cat on the remainder of the string
            RSStringRef substring = RSStringCreateSubStringWithRange(alloc, originalString, RSMakeRange(mark, length - mark));
            RSStringAppendString(newStr, substring);
            RSRelease(substring);
        }
        return newStr;
    } else {
        return (RSStringRef)RSStringCreateCopy(alloc, originalString);
    }
}


RSExport RSStringRef RSURLCreateStringByReplacingPercentEscapesUsingEncoding(RSAllocatorRef alloc, RSStringRef  originalString, RSStringRef  charactersToLeaveEscaped, RSStringEncoding enc) {
    if (enc == RSStringEncodingUTF8) {
        return RSURLCreateStringByReplacingPercentEscapes(alloc, originalString, charactersToLeaveEscaped);
    } else {
        RSMutableStringRef newStr = nil;
        RSMutableStringRef escapedStr = nil;
        RSIndex length;
        RSIndex mark = 0;
        RSRange percentRange, searchRange;
        BOOL escapeAll = (charactersToLeaveEscaped && RSStringGetLength(charactersToLeaveEscaped) == 0);
        BOOL failed = NO;
        uint8_t byteBuffer[8];
        uint8_t *bytes = byteBuffer;
        int capacityOfBytes = 8;
        
        if (!originalString) return nil;
        
        if (charactersToLeaveEscaped == nil) {
            return (RSStringRef)RSStringCreateCopy(alloc, originalString);
        }
        
        length = RSStringGetLength(originalString);
        searchRange = RSMakeRange(0, length);
        
        while (!failed && RSStringFindWithOptions(originalString, RSSTR("%"), searchRange, 0, &percentRange)) {
            UniChar ch1, ch2;
            RSIndex percentLoc = percentRange.location;
            RSStringRef convertedString;
            int numBytesUsed = 0;
            do {
                // Make sure we have at least 2 more characters
                if (length - percentLoc < 3) { failed = YES; break; }
                
                if (numBytesUsed == capacityOfBytes) {
                    if (bytes == byteBuffer) {
                        bytes = (uint8_t *)RSAllocatorAllocate(alloc, 16 * sizeof(uint8_t));
                        memmove(bytes, byteBuffer, capacityOfBytes);
                        capacityOfBytes = 16;
                    } else {
                        void *oldbytes = bytes;
                        int oldcap = capacityOfBytes;
                        capacityOfBytes = 2*capacityOfBytes;
                        bytes = (uint8_t *)RSAllocatorAllocate(alloc, capacityOfBytes * sizeof(uint8_t));
                        memmove(bytes, oldbytes, oldcap);
                        RSAllocatorDeallocate(alloc, oldbytes);
                    }
                }
                percentLoc ++;
                ch1 = RSStringGetCharacterAtIndex(originalString, percentLoc);
                percentLoc ++;
                ch2 = RSStringGetCharacterAtIndex(originalString, percentLoc);
                percentLoc ++;
                if (!_translateBytes(ch1, ch2, bytes + numBytesUsed)) { failed = YES;  break; }
                numBytesUsed ++;
            } while (RSStringGetCharacterAtIndex(originalString, percentLoc) == '%');
            searchRange.location = percentLoc;
            searchRange.length = length - searchRange.location;
            
            if (failed) break;
            convertedString = RSStringCreateWithBytes(alloc, bytes, numBytesUsed, enc, NO);
            if (!convertedString) {
                failed = YES;
                break;
            }
            
            if (!newStr) {
                newStr = RSStringCreateMutable(alloc, length);
            }
            if (percentRange.location - mark > 0) {
                // The creation of this temporary string is unfortunate.
                RSStringRef substring = RSStringCreateSubStringWithRange(alloc, originalString, RSMakeRange(mark, percentRange.location - mark));
                RSStringAppendString(newStr, substring);
                RSRelease(substring);
            }
            
            if (escapeAll) {
                RSStringAppendString(newStr, convertedString);
            } else {
                RSIndex i, c = RSStringGetLength(convertedString);
                if (!escapedStr) {
                    escapedStr = RSStringCreateMutableWithExternalCharactersNoCopy(alloc, &ch1, 1, 1, RSAllocatorDefault);
                }
                for (i = 0; i < c; i ++) {
                    ch1 = RSStringGetCharacterAtIndex(convertedString, i);
                    RSRange result = {RSNotFound};
                    if (RSStringFind(charactersToLeaveEscaped, escapedStr, RSStringGetRange(charactersToLeaveEscaped), &result) && result.location != RSNotFound) {
                        RSStringAppendCharacters(newStr, &ch1, 1);
                    } else {
                        // Must regenerate the escape sequence for this character; because we started with percent escapes, we know this call cannot fail
                        _appendPercentEscapesForCharacter(ch1, enc, newStr);
                    }
                }
            }
            RSRelease(convertedString);
            mark = searchRange.location;// We need mark to be the index of the first character beyond the escape sequence
        }
        
        if (escapedStr) RSRelease(escapedStr);
        if (bytes != byteBuffer) RSAllocatorDeallocate(alloc, bytes);
        if (failed) {
            if (newStr) RSRelease(newStr);
            return nil;
        } else if (newStr) {
            if (mark < length) {
                // Need to cat on the remainder of the string
                RSStringRef substring = RSStringCreateSubStringWithRange(alloc, originalString, RSMakeRange(mark, length - mark));
                RSStringAppendString(newStr, substring);
                RSRelease(substring);
            }
            return newStr;
        } else {
            return (RSStringRef)RSStringCreateCopy(alloc, originalString);
        }
    }
}
RSInline BOOL RSCharacterSetIsSurrogateHighCharacter(UniChar character) {
    return ((character >= 0xD800UL) && (character <= 0xDBFFUL) ? true : false);
}

RSInline BOOL RSCharacterSetIsSurrogateLowCharacter(UniChar character) {
    return ((character >= 0xDC00UL) && (character <= 0xDFFFUL) ? true : false);
}

static RSStringRef _addPercentEscapesToString(RSAllocatorRef allocator, RSStringRef originalString, BOOL (*shouldReplaceChar)(UniChar, void*), RSIndex (*handlePercentChar)(RSIndex, RSStringRef, RSStringRef *, void *), RSStringEncoding encoding, void *context) {
    RSMutableStringRef newString = nil;
    RSIndex idx, length;
    RSStringInlineBuffer buf;
    
    if (!originalString) return nil;
    length = RSStringGetLength(originalString);
    if (length == 0) return (RSStringRef)RSStringCreateCopy(allocator, originalString);
    RSStringInitInlineBuffer(originalString, &buf, RSMakeRange(0, length));
    
    for (idx = 0; idx < length; idx ++) {
        UniChar ch = RSStringGetCharacterFromInlineBuffer(&buf, idx);
        BOOL shouldReplace = shouldReplaceChar(ch, context);
        if (shouldReplace) {
            // Perform the replacement
            if (!newString) {
                newString = RSStringCreateMutableCopy(RSGetAllocator(originalString), 0, originalString);
                RSStringDelete(newString, RSMakeRange(idx, length-idx));
            }
            if (!_appendPercentEscapesForCharacter(ch, encoding, newString)) {
                //#warning FIXME - once RSString supports finding glyph boundaries walk by glyph boundaries instead of by UTF8Chars
                if (encoding == RSStringEncodingUTF8 && RSCharacterSetIsSurrogateHighCharacter(ch) && idx + 1 < length && RSCharacterSetIsSurrogateLowCharacter(RSStringGetCharacterFromInlineBuffer(&buf, idx+1))) {
                    // Hack to guarantee we always safely convert file URLs between POSIX & full URL representation
                    if (_hackToConvertSurrogates(ch, RSStringGetCharacterFromInlineBuffer(&buf, idx+1), newString)) {
                        idx ++; // We consumed 2 characters, not 1
                    } else {
                        break;
                    }
                } else {
                    break;
                }
            }
        } else if (ch == '%' && handlePercentChar) {
            RSStringRef replacementString = nil;
            RSIndex newIndex = handlePercentChar(idx, originalString, &replacementString, context);
            if (newIndex < 0) {
                break;
            } else if (replacementString) {
                if (!newString) {
                    newString = RSStringCreateMutableCopy(RSGetAllocator(originalString), 0, originalString);
                    RSStringDelete(newString, RSMakeRange(idx, length-idx));
                }
                RSStringAppendString(newString, replacementString);
                RSRelease(replacementString);
            }
            if (newIndex == idx) {
                if (newString) {
                    RSStringAppendCharacters(newString, &ch, 1);
                }
            } else {
                if (!replacementString && newString) {
                    RSIndex tmpIndex;
                    for (tmpIndex = idx; tmpIndex < newIndex; tmpIndex ++) {
                        ch = RSStringGetCharacterAtIndex(originalString, idx);
                        RSStringAppendCharacters(newString, &ch, 1);
                    }
                }
                idx = newIndex - 1;
            }
        } else if (newString) {
            RSStringAppendCharacters(newString, &ch, 1);
        }
    }
    if (idx < length) {
        // Ran in to an encoding failure
        if (newString) RSRelease(newString);
        return nil;
    } else if (newString) {
        return newString;
    } else {
        return (RSStringRef)RSStringCreateCopy(RSGetAllocator(originalString), originalString);
    }
}


static BOOL _stringContainsCharacter(RSStringRef string, UniChar ch) {
    RSIndex i, c = RSStringGetLength(string);
    RSStringInlineBuffer buf;
    RSStringInitInlineBuffer(string, &buf, RSMakeRange(0, c));
    for (i = 0; i < c; i ++)
        if (buf.directBuffer[i] == ch) return YES;
    //if (__RSStringGetCharacterFromInlineBufferQuick(&buf, i) == ch) return YES;
    return NO;
}

static BOOL _shouldPercentReplaceChar(UniChar ch, void *context) {
    RSStringRef unescape = ((RSStringRef *)context)[0];
    RSStringRef escape = ((RSStringRef *)context)[1];
    BOOL shouldReplace = (isURLLegalCharacter(ch) == NO);
    if (shouldReplace) {
        if (unescape && _stringContainsCharacter(unescape, ch)) {
            shouldReplace = NO;
        }
    } else if (escape && _stringContainsCharacter(escape, ch)) {
        shouldReplace = YES;
    }
    return shouldReplace;
}

RSExport RSStringRef RSURLCreateStringByAddingPercentEscapes(RSAllocatorRef allocator, RSStringRef originalString, RSStringRef charactersToLeaveUnescaped, RSStringRef legalURLCharactersToBeEscaped, RSStringEncoding encoding) {
    RSStringRef strings[2];
    strings[0] = charactersToLeaveUnescaped;
    strings[1] = legalURLCharactersToBeEscaped;
    return _addPercentEscapesToString(allocator, originalString, _shouldPercentReplaceChar, nil, encoding, strings);
}

static BOOL __RSURLEqual(RSTypeRef  rs1, RSTypeRef  rs2) {
    BOOL result;
    RSURLRef  url1 = (RSURLRef)rs1;
    RSURLRef  url2 = (RSURLRef)rs2;
    
    __RSGenericValidInstance(rs1, RSURLGetTypeID());
    __RSGenericValidInstance(rs2, RSURLGetTypeID());
    
    if ( url1 == url2 ) {
        result = YES;
    }
    else {
        if ( (url1->_flags & EQUAL_FLAGS_MASK) != (url2->_flags & EQUAL_FLAGS_MASK) ) {
            result = NO;
        }
        else {
            if ( (url1->_base && !url2->_base) ||
                (!url1->_base && url2->_base) ||
                (url1->_base && url2->_base && !RSEqual(url1->_base, url2->_base)) ) {
                result = NO;
            }
            else {
                // no base urls, so compare the URL strings
                // Do not compare the original strings; compare the sanatized strings.
                result = RSEqual(RSURLGetString(url1), RSURLGetString(url2));
            }
        }
    }
    return ( result ) ;
}

static RSHashCode __RSURLHash(RSTypeRef rs)
{
    RSHashCode result;
    
    if ( rs ) {
        // use the RSHashCode of the URL
        result = RSHash(RSURLGetString((RSURLRef)rs));
    }
    else {
        // no object, no hashcode
        result = 0;
    }
    
    return ( result );
}

static RSStringRef  __RSURLCopyFormattingDescription(RSTypeRef  rs, RSDictionaryRef formatOptions) {
    RSURLRef  url = (RSURLRef)rs;
    __RSGenericValidInstance(rs, RSURLGetTypeID());
    if (! url->_base) {
        RSRetain(url->_string);
        return url->_string;
    } else {
        // Do not dereference url->_base; it may be an ObjC object
        return RSStringCreateWithFormat(RSGetAllocator(url), nil, RSSTR("%R -- %R"), url->_string, url->_base);
    }
}


static RSStringRef __RSURLCopyDescription(RSTypeRef rs) {
    RSURLRef url = (RSURLRef)rs;
    RSStringRef result;
    RSAllocatorRef alloc = RSGetAllocator(url);
    if ( url->_base) {
        RSStringRef baseString = RSDescription(url->_base);
        result = RSStringCreateWithFormat(alloc, RSSTR("<RSURL %p [%p]>{string = %R, encoding = %d\n\tbase = %R}"), rs, alloc, url->_string, url->_encoding, baseString);
        RSRelease(baseString);
    } else {
        result = RSStringCreateWithFormat(alloc, RSSTR("<RSURL %p [%p]>{string = %R, encoding = %d, base = (null)}"), rs, alloc, url->_string, url->_encoding);
    }
    return result;
}

#if DEBUG_URL_MEMORY_USAGE

extern __attribute((used)) void __RSURLDumpMemRecord(void) {
    RSStringRef str = RSStringCreateWithFormat(RSAllocatorSystemDefault, nil, RSSTR("%d URLs created; %d destroyed\n%d file URLs created; %d urls had 'extra' data allocated, %d had base urls, %d were not UTF8 encoded\n"), numURLs, numDealloced, numFileURLsCreated, numExtraDataAllocated, numURLsWithBaseURL, numNonUTF8EncodedURLs );
    RSShow(str);
    RSRelease(str);
}
#endif

static void __RSURLDeallocate(RSTypeRef  rs) {
    RSURLRef  url = (RSURLRef)rs;
    RSAllocatorRef alloc;
    __RSGenericValidInstance(rs, RSURLGetTypeID());
    alloc = RSGetAllocator(url);
#if DEBUG_URL_MEMORY_USAGE
    numDealloced ++;
#endif
    if (url->_string) RSRelease(url->_string); // GC: 3879914
    if (url->_base) RSRelease(url->_base);
    if (url->_ranges) RSAllocatorDeallocate(alloc, url->_ranges);
    RSStringRef sanitizedString = _getSanitizedString(url);
    if (sanitizedString) RSRelease(sanitizedString);
    if ( url->_extra != nil ) RSAllocatorDeallocate( alloc, url->_extra );
    if (_getResourceInfo(url)) RSRelease(_getResourceInfo(url));
}

static RSTypeID __RSURLTypeID = _RSRuntimeNotATypeID;

static const RSRuntimeClass __RSURLClass = {
    _RSRuntimeScannedObject,                                  // version
    "RSURL",                            // className
    nil,                               // init
    nil,                               // copy
    __RSURLDeallocate,                  // finalize
    __RSURLEqual,                       // equal
    __RSURLHash,                        // hash
    __RSURLCopyDescription,             // copyDebugDesc
    nil,                               // reclaim
    nil,                               // refcount
};

// When __CONSTANT_RSSTRINGS__ is not defined, we have separate macros for static and exported constant strings, but
// when it is defined, we must prefix with static to prevent the string from being exported
#ifdef __CONSTANT_RSSTRINGS__
RS_CONST_STRING_DECL(RSURLHTTPScheme, "http")
RS_CONST_STRING_DECL(RSURLHTTPSScheme, "https")
RS_CONST_STRING_DECL(RSURLFileScheme, "file")
RS_CONST_STRING_DECL(RSURLDataScheme, "data")
RS_CONST_STRING_DECL(RSURLFTPScheme, "ftp")
RS_CONST_STRING_DECL(RSURLLocalhost, "localhost")
#else
RS_CONST_STRING_DECL(RSURLHTTPScheme, "http")
RS_CONST_STRING_DECL(RSURLHTTPSScheme, "https")
RS_CONST_STRING_DECL(RSURLFileScheme, "file")
RS_CONST_STRING_DECL(RSURLDataScheme, "data")
RS_CONST_STRING_DECL(RSURLFTPScheme, "ftp")
RS_CONST_STRING_DECL(RSURLLocalhost, "localhost")
#endif
RSPrivate void __RSURLInitialize(void) {
    __RSURLTypeID = __RSRuntimeRegisterClass(&__RSURLClass);
    __RSRuntimeSetClassTypeID(&__RSURLClass, __RSURLTypeID);
}

/* Toll-free bridging support; get the YES RSURL from an NSURL */
RSInline RSURLRef _RSURLFromNSURL(RSURLRef url) {
    RS_OBJC_FUNCDISPATCHV(__RSURLTypeID, RSURLRef, (NSURL *)url, _rsurl);
    return url;
}

RSExport RSTypeID RSURLGetTypeID() {
    return __RSURLTypeID;
}

RSPrivate void RSShowURL(RSURLRef url) {
    if (!url) {
        fprintf(stdout, "(null)\n");
        return;
    }
    fprintf(stdout, "<RSURL %p>{", (const void*)url);
    if (RS_IS_OBJC(__RSURLTypeID, url)) {
        fprintf(stdout, "ObjC bridged object}\n");
        return;
    }
    fprintf(stdout, "\n\tRelative string: ");
    RSShow(url->_string);
    fprintf(stdout, "\tBase URL: ");
    if (url->_base) {
        fprintf(stdout, "<%p> ", (const void*)url->_base);
        RSShow(url->_base);
    } else {
        fprintf(stdout, "(null)\n");
    }
    fprintf(stdout, "\tFlags: 0x%x\n}\n", (unsigned int)url->_flags);
}


/***************************************************/
/* URL creation and String/Data creation from URLS */
/***************************************************/
static void constructBuffers(RSAllocatorRef alloc, RSStringRef string, BOOL useEightBitStringEncoding, RSBitU8 *inBuffer, RSIndex inBufferSize, const char **cstring, const UniChar **ustring, BOOL *useCString, BOOL *freeCharacters) {
    RSIndex neededLength;
    RSIndex length;
    RSRange rg;
    
    *cstring = RSStringGetCStringPtr(string, (useEightBitStringEncoding ? __RSStringGetEightBitStringEncoding() : RSStringEncodingISOLatin1));
    if (*cstring) {
        *ustring = NULL;
        *useCString = true;
        *freeCharacters = false;
        return;
    }
    
    *ustring = RSStringGetCharactersPtr(string);
    if (*ustring) {
        *useCString = false;
        *freeCharacters = false;
        return;
    }
    
    length = RSStringGetLength(string);
    rg = RSMakeRange(0, length);
    RSStringGetBytes(string, rg, RSStringEncodingISOLatin1, 0, false, NULL, INT_MAX, &neededLength);
    if (neededLength == length) {
        char *buf;
        if ( (inBuffer != NULL) && (length <= inBufferSize) ) {
            buf = (char *)inBuffer;
            *freeCharacters = false;
        }
        else {
            buf = (char *)RSAllocatorAllocate(alloc, length);
            *freeCharacters = true;
        }
        RSStringGetBytes(string, rg, RSStringEncodingISOLatin1, 0, false, (uint8_t *)buf, length, NULL);
        *cstring = buf;
        *useCString = true;
    } else {
        UniChar *buf;
        if ( (inBuffer != NULL) && ((length * sizeof(UniChar)) <= inBufferSize) ) {
            buf = (UniChar *)inBuffer;
            *freeCharacters = false;
        }
        else {
            buf = (UniChar *)RSAllocatorAllocate(alloc, length * sizeof(UniChar));
            *freeCharacters = true;
        }
        RSStringGetCharacters(string, rg, buf);
        *ustring = buf;
        *useCString = false;
    }
//    RSIndex length;
//    RSRange rg;
//    
//    *cstring = RSStringGetCStringPtr(string, (useEightBitStringEncoding ? __RSStringGetEightBitStringEncoding() : RSStringEncodingISOLatin1));
//    if (*cstring) {
//        *ustring = nil;
//        *useCString = YES;
//        *freeCharacters = NO;
//        return;
//    }
//    *ustring = (UniChar *)RSStringGetCharactersPtr(string);
//    if (*ustring) {
//        *useCString = NO;
//        *freeCharacters = NO;
//        return;
//    }
//    
//    length = RSStringGetLength(string);
//    rg = RSMakeRange(0, length);
//    RSStringRef convertedString = RSStringCreateConvert(string, RSStringEncodingISOLatin1);
//    if (convertedString)
//    {
//        UniChar *buf = (UniChar *)RSAllocatorAllocate(alloc, length * sizeof(UniChar));
//        *freeCharacters = YES;
//        RSStringGetCharacters(convertedString, rg, buf);
//        RSRelease(convertedString);
//        *useCString = NO;
//    }
//    else
//    {
//        char *buf = RSAllocatorAllocate(alloc, length);
//        *freeCharacters = YES;
//        RSStringGetCharacters(string, rg, (UniChar *)buf);
//        *cstring = buf;
//        *useCString = YES;
//    }
}

#define STRING_CHAR(x) (useCString ? cstring[(x)] : ustring[(x)])
static void _parseComponents(RSAllocatorRef alloc, RSStringRef string, RSURLRef baseURL, RSBitU32 *theFlags, RSRange **range) {
    RSRange ranges[9];
    /* index gives the URL part involved; to calculate the correct range index, use the number of the bit of the equivalent flag (i.e. the host flag is HAS_HOST, which is 0x8.  so the range index for the host is 3.)  Note that this is YES in this function ONLY, since the ranges stored in (*range) are actually packed, skipping those URL components that don't exist.  This is why the indices are hard-coded in this function. */
    
    RSIndex idx, base_idx = 0;
    RSIndex string_length;
    RSBitU32 flags = *theFlags;
    BOOL useEightBitStringEncoding = (flags & USES_EIGHTBITSTRINGENCODING) != 0;
    BOOL useCString, freeCharacters, isCompliant;
    uint8_t numRanges = 0;
    const char *cstring = nil;
    const UniChar *ustring = nil;
    RSIndex stackBufferSize = 4096;
    STACK_BUFFER_DECL(RSBitU8, stackBuffer, stackBufferSize);
    
    string_length = RSStringGetLength(string);
    constructBuffers(alloc, string, useEightBitStringEncoding, stackBuffer, stackBufferSize, &cstring, &ustring, &useCString, &freeCharacters);
    
    // Algorithm is as described in RFC 1808
    // 1: parse the fragment; remainder after left-most "#" is fragment
    for (idx = base_idx; idx < string_length; idx++) {
        if ('#' == STRING_CHAR(idx)) {
            flags |= HAS_FRAGMENT;
            ranges[8].location = idx + 1;
            ranges[8].length = string_length - (idx + 1);
            numRanges ++;
            string_length = idx;	// remove fragment from parse string
            break;
        }
    }
    // 2: parse the scheme
    for (idx = base_idx; idx < string_length; idx++) {
        UniChar ch = STRING_CHAR(idx);
        if (':' == ch) {
            flags |= HAS_SCHEME;
            flags |= IS_ABSOLUTE;
            ranges[0].location = base_idx;
            ranges[0].length = idx;
            numRanges ++;
            base_idx = idx + 1;
            // optimization for ftp urls
            if (idx == 3 && STRING_CHAR(0) == 'f' && STRING_CHAR(1) == 't' && STRING_CHAR(2) == 'p') {
                _setSchemeTypeInFlags(&flags, kHasFtpScheme);
            }
            else if (idx == 4) {
                // optimization for http urls
                if (STRING_CHAR(0) == 'h' && STRING_CHAR(1) == 't' && STRING_CHAR(2) == 't' && STRING_CHAR(3) == 'p') {
                    _setSchemeTypeInFlags(&flags, kHasHttpScheme);
                }
                // optimization for file urls
                if (STRING_CHAR(0) == 'f' && STRING_CHAR(1) == 'i' && STRING_CHAR(2) == 'l' && STRING_CHAR(3) == 'e') {
                    _setSchemeTypeInFlags(&flags, kHasFileScheme);
                }
                // optimization for data urls
                if (STRING_CHAR(0) == 'd' && STRING_CHAR(1) == 'a' && STRING_CHAR(2) == 't' && STRING_CHAR(3) == 'a') {
                    _setSchemeTypeInFlags(&flags, kHasDataScheme);
                }
            }
            // optimization for https urls
            else if (idx == 5 && STRING_CHAR(0) == 'h' && STRING_CHAR(1) == 't' && STRING_CHAR(2) == 't' && STRING_CHAR(3) == 'p' && STRING_CHAR(3) == 's') {
                _setSchemeTypeInFlags(&flags, kHasHttpsScheme);
            }
            break;
        } else if (!scheme_valid(ch)) {
            break;	// invalid scheme character -- no scheme
        }
    }
    
    // Make sure we have an RFC-1808 compliant URL - that's either something without a scheme, or scheme:/(stuff) or scheme://(stuff)
    // Strictly speaking, RFC 1808 & 2396 bar "scheme:" (with nothing following the colon); however, common usage
    // expects this to be treated identically to "scheme://" - REW, 12/08/03
    if (!(flags & HAS_SCHEME)) {
        isCompliant = YES;
    } else if (base_idx == string_length) {
        isCompliant = NO;
    } else if (STRING_CHAR(base_idx) != '/') {
        isCompliant = NO;
    } else {
        isCompliant = YES;
    }
    
    if (!isCompliant) {
        // Clear the fragment flag if it's been set
        if (flags & HAS_FRAGMENT) {
            flags &= (~HAS_FRAGMENT);
            string_length = RSStringGetLength(string);
        }
        (*theFlags) = flags;
        (*range) = (RSRange *)RSAllocatorAllocate(alloc, sizeof(RSRange));
        (*range)->location = ranges[0].location;
        (*range)->length = ranges[0].length;
        
        if (freeCharacters) {
            RSAllocatorDeallocate(alloc, useCString ? (void *)cstring : (void *)ustring);
        }
        return;
    }
    // URL is 1808-compliant
    flags |= IS_DECOMPOSABLE;
    
    // 3: parse the network location and login
    if (2 <= (string_length - base_idx) && '/' == STRING_CHAR(base_idx) && '/' == STRING_CHAR(base_idx+1)) {
        RSIndex base = 2 + base_idx, extent;
        for (idx = base; idx < string_length; idx++) {
            if ('/' == STRING_CHAR(idx) || '?' == STRING_CHAR(idx)) break;
        }
        extent = idx;
        
        // net_loc parts extend from base to extent (but not including), which might be to end of string
        // net location is "<user>:<password>@<host>:<port>"
        if (extent != base) {
            for (idx = base; idx < extent; idx++) {
                if ('@' == STRING_CHAR(idx)) {   // there is a user
                    RSIndex idx2;
                    flags |= HAS_USER;
                    numRanges ++;
                    ranges[1].location = base;  // base of the user
                    for (idx2 = base; idx2 < idx; idx2++) {
                        if (':' == STRING_CHAR(idx2)) {	// found a password separator
                            flags |= HAS_PASSWORD;
                            numRanges ++;
                            ranges[2].location = idx2+1; // base of the password
                            ranges[2].length = idx-(idx2+1);  // password extent
                            ranges[1].length = idx2 - base; // user extent
                            break;
                        }
                    }
                    if (!(flags & HAS_PASSWORD)) {
                        // user extends to the '@'
                        ranges[1].length = idx - base; // user extent
                    }
                    base = idx + 1;
                    break;
                }
            }
            flags |= HAS_HOST;
            numRanges ++;
            ranges[3].location = base; // base of host
            
            // base has been advanced past the user and password if they existed
            for (idx = base; idx < extent; idx++) {
                // IPV6 support (RFC 2732) DCJ June/10/2002
                if ('[' == STRING_CHAR(idx)) {	// starting IPV6 explicit address
					//	Find the ']' terminator of the IPv6 address, leave idx pointing to ']' or end
					for ( ; idx < extent; ++ idx ) {
						if ( ']' == STRING_CHAR(idx)) {
							flags |= IS_IPV6_ENCODED;
							break;
						}
					}
				}
                // there is a port if we see a colon.  Only the last one is the port, though.
                else if ( ':' == STRING_CHAR(idx)) {
                    flags |= HAS_PORT;
                    numRanges ++;
                    ranges[4].location = idx+1; // base of port
                    ranges[4].length = extent - (idx+1); // port extent
                    ranges[3].length = idx - base; // host extent
                    break;
                }
            }
            if (!(flags & HAS_PORT)) {
                ranges[3].length = extent - base;  // host extent
            }
        }
        base_idx = extent;
    }
    
    // 4: parse the query; remainder after left-most "?" is query
    for (idx = base_idx; idx < string_length; idx++) {
        if ('?' == STRING_CHAR(idx)) {
            flags |= HAS_QUERY;
            numRanges ++;
            ranges[7].location = idx + 1;
            ranges[7].length = string_length - (idx+1);
            string_length = idx;	// remove query from parse string
            break;
        }
    }
    
    // 5: parse the parameters; remainder after left-most ";" is parameters
    for (idx = base_idx; idx < string_length; idx++) {
        if (';' == STRING_CHAR(idx)) {
            flags |= HAS_PARAMETERS;
            numRanges ++;
            ranges[6].location = idx + 1;
            ranges[6].length = string_length - (idx+1);
            string_length = idx;	// remove parameters from parse string
            break;
        }
    }
    
    // 6: parse the path; it's whatever's left between string_length & base_idx
    if (string_length - base_idx != 0 || (flags & NET_LOCATION_MASK))
    {
        // If we have a net location, we are 1808-compliant, and an empty path substring implies a path of "/"
        UniChar ch;
        BOOL isDir;
        RSRange pathRg;
        flags |= HAS_PATH;
        numRanges ++;
        pathRg.location = base_idx;
        pathRg.length = string_length - base_idx;
        ranges[5] = pathRg;
        
        if (pathRg.length > 0) {
            BOOL sawPercent = NO;
            for (idx = pathRg.location; idx < string_length; idx++) {
                if ('%' == STRING_CHAR(idx)) {
                    sawPercent = YES;
                    break;
                }
            }
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI
            if (pathRg.length > 6 && STRING_CHAR(pathRg.location) == '/' && STRING_CHAR(pathRg.location + 1) == '.' && STRING_CHAR(pathRg.location + 2) == 'f' && STRING_CHAR(pathRg.location + 3) == 'i' && STRING_CHAR(pathRg.location + 4) == 'l' && STRING_CHAR(pathRg.location + 5) == 'e' && STRING_CHAR(pathRg.location + 6) == '/') {
                flags |= PATH_HAS_FILE_ID;
            } else if (!sawPercent) {
                flags |= POSIX_AND_URL_PATHS_MATCH;
            }
#elif DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_WINDOWS
            if (!sawPercent) {
                flags |= POSIX_AND_URL_PATHS_MATCH;
            }
#endif
            
            ch = STRING_CHAR(pathRg.location + pathRg.length - 1);
            if (ch == '/') {
                isDir = YES;
            } else if (ch == '.') {
                if (pathRg.length == 1) {
                    isDir = YES;
                } else {
                    ch = STRING_CHAR(pathRg.location + pathRg.length - 2);
                    if (ch == '/') {
                        isDir = YES;
                    } else if (ch != '.') {
                        isDir = NO;
                    } else if (pathRg.length == 2) {
                        isDir = YES;
                    } else {
                        isDir = (STRING_CHAR(pathRg.location + pathRg.length - 3) == '/');
                    }
                }
            } else {
                isDir = NO;
            }
        } else {
            isDir = (baseURL != nil) ? RSURLHasDirectoryPath(baseURL) : NO;
        }
        if (isDir) {
            flags |= IS_DIRECTORY;
        }
    }
    
    if (freeCharacters) {
        RSAllocatorDeallocate(alloc, useCString ? (void *)cstring : (void *)ustring);
    }
    (*theFlags) = flags;
    (*range) = (RSRange *)RSAllocatorAllocate(alloc, sizeof(RSRange)*numRanges);
    numRanges = 0;
    for (idx = 0, flags = 1; flags != (1<<9); flags = (flags<<1), idx ++) {
        if ((*theFlags) & flags) {
            (*range)[numRanges] = ranges[idx];
            numRanges ++;
        }
    }
}

static BOOL scanCharacters(RSAllocatorRef alloc, RSMutableStringRef *escapedString, RSBitU32 *flags, const char *cstring, const UniChar *ustring, BOOL useCString, RSIndex base, RSIndex end, RSIndex *mark, RSBitU32 componentFlag, RSStringEncoding encoding) {
    RSIndex idx;
    BOOL sawIllegalChar = NO;
    for (idx = base; idx < end; idx ++) {
        BOOL shouldEscape;
        UniChar ch = STRING_CHAR(idx);
        if (isURLLegalCharacter(ch)) {
            if ((componentFlag == HAS_USER || componentFlag == HAS_PASSWORD) && (ch == '/' || ch == '?' || ch == '@')) {
                shouldEscape = YES;
            } else {
                shouldEscape = NO;
            }
        } else if (ch == '%' && idx + 2 < end && isHexDigit(STRING_CHAR(idx + 1)) && isHexDigit(STRING_CHAR(idx+2))) {
            shouldEscape = NO;
        } else if (componentFlag == HAS_HOST && ((idx == base && ch == '[') || (idx == end-1 && ch == ']'))) {
            shouldEscape = NO;
        } else {
            shouldEscape = YES;
        }
        if (shouldEscape) {
            sawIllegalChar = YES;
            if (componentFlag && flags) {
                *flags |= componentFlag;
            }
            if (!*escapedString) {
                *escapedString = RSStringCreateMutable(alloc, 0);
            }
            if (useCString) {
                RSStringRef tempString = RSStringCreateWithBytes(alloc, (uint8_t *)&(cstring[*mark]), idx - *mark, RSStringEncodingISOLatin1, NO);
                RSStringAppendString(*escapedString, tempString);
                RSRelease(tempString);
            } else {
                RSStringAppendCharacters(*escapedString, &(ustring[*mark]), idx - *mark);
            }
            *mark = idx + 1;
            _appendPercentEscapesForCharacter(ch, encoding, *escapedString); // This can never fail because anURL->_string was constructed from the encoding passed in
        }
    }
    return sawIllegalChar;
}

static void computeSanitizedString(RSURLRef url) {
    RSAllocatorRef alloc = RSGetAllocator(url);
    RSIndex string_length = RSStringGetLength(url->_string);
    BOOL useCString, freeCharacters;
    const char *cstring = nil;
    const UniChar *ustring = nil;
    RSIndex base; // where to scan from
    RSIndex mark; // first character not-yet copied to sanitized string
    RSIndex stackBufferSize = 4096;
    STACK_BUFFER_DECL(RSBitU8, stackBuffer, stackBufferSize);
    RSMutableStringRef sanitizedString = nil;
    RSBitU32 additionalDataFlags = 0;
    BOOL useEightBitStringEncoding = (url->_flags & USES_EIGHTBITSTRINGENCODING) != 0;
    
    constructBuffers(alloc, url->_string, useEightBitStringEncoding, stackBuffer, stackBufferSize, &cstring, &ustring, &useCString, &freeCharacters);
    if (!(url->_flags & IS_DECOMPOSABLE)) {
        // Impossible to have a problem character in the scheme
        base = _rangeForComponent(url->_flags, url->_ranges, HAS_SCHEME).length + 1;
        mark = 0;
        if (!scanCharacters(alloc, &sanitizedString, &additionalDataFlags, cstring, ustring, useCString, base, string_length, &mark, 0, url->_encoding)) {
            ((struct __RSURL *)url)->_flags |= ORIGINAL_AND_URL_STRINGS_MATCH;
        }
        if ( sanitizedString ) {
            _setAdditionalDataFlags((struct __RSURL*)url, additionalDataFlags);
        }
    } else {
        // Go component by component
        RSIndex currentComponent = HAS_USER;
        mark = 0;
        while (currentComponent < (HAS_FRAGMENT << 1)) {
            RSRange componentRange = _rangeForComponent(url->_flags, url->_ranges, (RSBitU32)currentComponent);
            if (componentRange.location != RSNotFound) {
                scanCharacters(alloc, & sanitizedString, &additionalDataFlags, cstring, ustring, useCString, componentRange.location, componentRange.location + componentRange.length, &mark, (RSBitU32)currentComponent, url->_encoding);
            }
            currentComponent = currentComponent << 1;
        }
        if (sanitizedString) {
            _setAdditionalDataFlags((struct __RSURL*)url, additionalDataFlags);
        } else {
            ((struct __RSURL *)url)->_flags |= ORIGINAL_AND_URL_STRINGS_MATCH;
        }
    }
    if (sanitizedString && mark != string_length) {
        if (useCString) {
            RSStringRef tempString = RSStringCreateWithBytes(alloc, (uint8_t *)&(cstring[mark]), string_length - mark, RSStringEncodingISOLatin1, NO);
            RSStringAppendString(sanitizedString, tempString);
            RSRelease(tempString);
        } else {
            RSStringAppendCharacters(sanitizedString, &(ustring[mark]), string_length - mark);
        }
    }
    if ( sanitizedString ) {
        _setSanitizedString((struct __RSURL*) url, sanitizedString);
        RSRelease(sanitizedString);
    }
    if (freeCharacters) {
        RSAllocatorDeallocate(alloc, useCString ? (void *)cstring : (void *)ustring);
    }
}


static RSStringRef correctedComponent(RSStringRef comp, RSBitU32 compFlag, RSStringEncoding enc) {
    RSAllocatorRef alloc = RSGetAllocator(comp);
    RSIndex string_length = RSStringGetLength(comp);
    BOOL useCString, freeCharacters;
    const char *cstring = nil;
    const UniChar *ustring = nil;
    RSIndex mark = 0; // first character not-yet copied to sanitized string
    RSMutableStringRef result = nil;
    RSIndex stackBufferSize = 1024;
    STACK_BUFFER_DECL(RSBitU8, stackBuffer, stackBufferSize);
    
    constructBuffers(alloc, comp, NO, stackBuffer, stackBufferSize, &cstring, &ustring, &useCString, &freeCharacters);
    scanCharacters(alloc, &result, nil, cstring, ustring, useCString, 0, string_length, &mark, compFlag, enc);
    if (result) {
        if (mark < string_length) {
            if (useCString) {
                RSStringRef tempString = RSStringCreateWithBytes(alloc, (uint8_t *)&(cstring[mark]), string_length - mark, RSStringEncodingISOLatin1, NO);
                RSStringAppendString(result, tempString);
                RSRelease(tempString);
            } else {
                RSStringAppendCharacters(result, &(ustring[mark]), string_length - mark);
            }
        }
    } else {
        // This should nevr happen
        RSRetain(comp);
        result = (RSMutableStringRef)comp;
    }
    if (freeCharacters) {
        RSAllocatorDeallocate(alloc, useCString ? (void *)cstring : (void *)ustring);
    }
    return result;
}

#undef STRING_CHAR
RSExport RSURLRef _RSURLAlloc(RSAllocatorRef allocator) {
    struct __RSURL *url;
#if DEBUG_URL_MEMORY_USAGE
    numURLs ++;
#endif
    url = (struct __RSURL *)__RSRuntimeCreateInstance(allocator, __RSURLTypeID, sizeof(struct __RSURL) - sizeof(struct __RSRuntimeBase));
    if (url) {
        url->_flags = 0;
        url->_encoding = RSStringEncodingUTF8;
        url->_string = nil;
        url->_base = nil;
        url->_ranges = nil;
        url->_extra = nil;
        url->_resourceInfo = nil;
    }
    return url;
}

// It is the caller's responsibility to guarantee that if URLString is absolute, base is nil.  This is necessary to avoid duplicate processing for file system URLs, which had to decide whether to compute the cwd for the base; we don't want to duplicate that work.  This ALSO means it's the caller's responsibility to set the IS_ABSOLUTE bit, since we may have a degenerate URL whose string is relative, but lacks a base.
static void _RSURLInit(struct __RSURL *url, RSStringRef URLString, RSURLPathStyle fsType, RSURLRef base) {
    RSAssert2((fsType == FULL_URL_REPRESENTATION) || (fsType == RSURLPOSIXPathStyle) || (fsType == RSURLWindowsPathStyle) || (fsType == RSURLHFSPathStyle), __RSLogAssertion, "%s(): Received bad fsType %d", __PRETTY_FUNCTION__, fsType);
    
    // Coming in, the url has its allocator flag properly set, and its base initialized, and nothing else.
    url->_string = RSCopy(RSGetAllocator(url), URLString);
    url->_base = base ? RSURLCopyAbsoluteURL(base) : nil;
    
#if DEBUG_URL_MEMORY_USAGE
    if ( (fsType == RSURLPOSIXPathStyle) || (fsType == RSURLHFSPathStyle) || (fsType == RSURLWindowsPathStyle) ) {
        numFileURLsCreated ++;
    }
    if ( url->_base ) {
        numURLsWithBaseURL ++;
    }
#endif
    if (fsType != FULL_URL_REPRESENTATION) {
        // _convertToURLRepresentation parses the URL
        _convertToURLRepresentation((struct __RSURL *)url, (RSBitU32)fsType);
    }
    else {
        _parseComponentsOfURL(url);
    }
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
RS_EXPORT void _RSURLInitFSPath(RSURLRef url, RSStringRef path) {
    RSIndex len = RSStringGetLength(path);
    if (len && RSStringGetCharacterAtIndex(path, 0) == '/') {
        _RSURLInit((struct __RSURL *)url, path, RSURLPOSIXPathStyle, nil);
        ((struct __RSURL *)url)->_flags |= IS_ABSOLUTE;
    } else {
        RSURLRef cwdURL = _RSURLCreateCurrentDirectoryURL(RSGetAllocator(url));
        _RSURLInit((struct __RSURL *)url, path, RSURLPOSIXPathStyle, cwdURL);
        if ( cwdURL )
            RSRelease(cwdURL);
    }
    if (!len || '/' == RSStringGetCharacterAtIndex(path, len - 1))
        ((struct __RSURL *)url)->_flags |= IS_DIRECTORY;
}
#elif DEPLOYMENT_TARGET_WINDOWS
RS_EXPORT void _RSURLInitFSPath(RSURLRef url, RSStringRef path) {
    RSIndex len = RSStringGetByteLength(path);
	// be sure to use the windows path separator when checking the path to see if it's a directory here
    if (!len || '\\' == RSStringGetCharacterAtIndex(path, len - 1))
        ((struct __RSURL *)url)->_flags |= IS_DIRECTORY;
    UTF8Char firstChar = 0 < len ? RSStringGetCharacterAtIndex(path, 0) : 0;
    UTF8Char secondChar = 1 < len ? RSStringGetCharacterAtIndex(path, 1) : 0;
    BOOL isDrive = ('A' <= firstChar && firstChar <= 'Z') || ('a' <= firstChar && firstChar <= 'z');
    if (!len || '/' == RSStringGetCharacterAtIndex(path, len - 1))
        ((struct __RSURL *)url)->_flags |= IS_DIRECTORY;
    isDrive = isDrive && (secondChar == ':' || secondChar == '|');
    if (isDrive || (firstChar == '\\' && secondChar == '\\')) {
        _RSURLInit((struct __RSURL *)url, path, RSURLWindowsPathStyle, nil);
        ((struct __RSURL *)url)->_flags |= IS_ABSOLUTE;
    } else if (firstChar == '/') {
        if (!len || '/' == RSStringGetCharacterAtIndex(path, len - 1))
            ((struct __RSURL *)url)->_flags |= IS_DIRECTORY;
        _RSURLInit((struct __RSURL *)url, path, RSURLPOSIXPathStyle, nil);
        ((struct __RSURL *)url)->_flags |= IS_ABSOLUTE;
    } else {
        RSURLRef cwdURL = _RSURLCreateCurrentDirectoryURL(RSGetAllocator(url));
        _RSURLInit((struct __RSURL *)url, path, RSURLPOSIXPathStyle, cwdURL);
        if ( cwdURL )
            RSRelease(cwdURL);
    }
}
#endif

// Exported for Foundation's use
RSExport BOOL _RSStringIsLegalURLString(RSStringRef string) {
    // Check each character to make sure it is a legal URL char.  The valid characters are 'A'-'Z', 'a' - 'z', '0' - '9', plus the characters in "-_.!~*'()", and the set of reserved characters (these characters have special meanings in the URL syntax), which are ";/?:@&=+$,".  In addition, percent escape sequences '%' hex-digit hex-digit are permitted.
    // Plus the hash character '#' which denotes the beginning of a fragment, and can appear exactly once in the entire URL string. -- REW, 12/13/2000
    RSStringInlineBuffer stringBuffer;
    RSIndex idx = 0, length;
    BOOL sawHash = NO;
    if (!string) {
        RSAssert(NO, __RSLogAssertion, "Cannot create an RSURL from a nil string");
        return NO;
    }
    length = RSStringGetLength(string);
    RSStringInitInlineBuffer(string, &stringBuffer, RSMakeRange(0, length));
    while (idx < length) {
        UniChar ch = RSStringGetCharacterFromInlineBuffer(&stringBuffer, idx);
        idx ++;
		
		//	Make sure that two valid hex digits follow a '%' character
		if ( ch == '%' ) {
			if ( idx + 2 > length )
			{
                RSAssert1(NO, __RSLogAssertion, "Detected illegal percent escape sequence at character %d when trying to create a RSURL", idx-1);
				idx = -1;  // To guarantee index < length, and our failure case is triggered
				break;
			}
			
			ch = RSStringGetCharacterFromInlineBuffer(&stringBuffer, idx);
			idx ++;
			if (! isHexDigit(ch) ) {
                RSAssert1(NO, __RSLogAssertion, "Detected illegal percent escape sequence at character %d when trying to create a RSURL", idx-2);
				idx = -1;
				break;
			}
			ch = RSStringGetCharacterFromInlineBuffer(&stringBuffer, idx);
			idx ++;
			if (! isHexDigit(ch) ) {
                RSAssert1(NO, __RSLogAssertion, "Detected illegal percent escape sequence at character %d when trying to create a RSURL", idx-3);
				idx = -1;
				break;
			}
            
			continue;
        }
		if (ch == '[' || ch == ']') continue; // IPV6 support (RFC 2732) DCJ June/10/2002
        if (ch == '#') {
            if (sawHash) break;
            sawHash = YES;
            continue;
        }
#if DEPLOYMENT_TARGET_WINDOWS
        // <rdar://problem/7134119> RS on Windows: RSURLCreateWithString should work with | in path on Windows
        if (isURLLegalCharacter(ch) || ch == '|')
#else
            if ( isURLLegalCharacter( ch ) )
#endif
                continue;
		break;
    }
    if (idx < length) {
        return NO;
    }
    return YES;
}

RSExport void _RSURLInitWithString(RSURLRef myURL, RSStringRef string, RSURLRef baseURL) {
    struct __RSURL *url = (struct __RSURL *)myURL; // Supress annoying compile warnings
    BOOL isAbsolute = NO;
    RSRange colon = {RSNotFound};
    if (RSStringFind(string, RSSTR(":"), RSStringGetRange(string), &colon) && colon.location != RSNotFound) {
        isAbsolute = YES;
        RSIndex i;
        for (i = 0; i < colon.location; i++) {
            char ch = (char)RSStringGetCharacterAtIndex(string, i);
            if (!scheme_valid(ch)) {
                isAbsolute = NO;
                break;
            }
        }
    }
    _RSURLInit(url, string, FULL_URL_REPRESENTATION, isAbsolute ? nil : baseURL);
    if (isAbsolute) {
        url->_flags |= IS_ABSOLUTE;
    }
}

struct __RSURLEncodingTranslationParameters {
    RSStringEncoding fromEnc;
    RSStringEncoding toEnc;
    const UniChar *addlChars;
    int count;
    BOOL escapeHighBit;
    BOOL escapePercents;
    BOOL agreesOverASCII;
    BOOL encodingsMatch;
} ;

// encoding will be used both to interpret the bytes of URLBytes, and to interpret any percent-escapes within the bytes.
RSExport RSURLRef RSURLCreateWithBytes(RSAllocatorRef allocator, const uint8_t *URLBytes, RSIndex length, RSStringEncoding encoding, RSURLRef baseURL) {
    RSStringRef  urlString;
    BOOL useEightBitStringEncoding = ( __RSStringEncodingIsSupersetOfASCII(encoding) && __RSBytesInASCII(URLBytes, length) );
    urlString = RSStringCreateWithBytes(allocator, URLBytes, length, useEightBitStringEncoding ? RSStringEncodingUTF8 : encoding, NO);
    RSURLRef  result;
    if (!urlString || RSStringGetLength(urlString) == 0) {
        if (urlString) RSRelease(urlString);
        return nil;
    }
    result = _RSURLAlloc(allocator);
    if (result) {
        if ( useEightBitStringEncoding ) {
            ((struct __RSURL *)result)->_flags |= USES_EIGHTBITSTRINGENCODING;
        }
        _RSURLInitWithString(result, urlString, baseURL);
        if (encoding != RSStringEncodingUTF8) {
            ((struct __RSURL *)result)->_encoding = encoding;
#if DEBUG_URL_MEMORY_USAGE
            numNonUTF8EncodedURLs++;
#endif
        }
    }
    RSRelease(urlString); // it's retained by result, now.
    return result;
}
#include <RSCoreFoundation/RSData.h>
RSExport RSDataRef RSURLCreateData(RSAllocatorRef allocator, RSURLRef  url, RSStringEncoding encoding, BOOL escapeWhitespace) {
    RSDataRef result = nil;
    if ( url ) {
        RSStringRef myStr = RSURLGetString(url);
        if ( myStr ) {
            result = RSStringCreateExternalRepresentation(allocator, myStr, encoding, 0);
        }
    }
    return result;
}

// Any escape sequences in URLString will be interpreted via UTF-8.
RSExport RSURLRef RSURLCreateWithString(RSAllocatorRef allocator, RSStringRef  URLString, RSURLRef  baseURL) {
    RSURLRef url;
    if (!URLString || RSStringGetLength(URLString) == 0) return nil;
    if (!_RSStringIsLegalURLString(URLString)) return nil;
    url = _RSURLAlloc(allocator);
    if (url) {
        _RSURLInitWithString(url, URLString, baseURL);
    }
    return url;
}

static RSURLRef _RSURLCreateWithArbitraryString(RSAllocatorRef allocator, RSStringRef URLString, RSURLRef baseURL) {
    RSURLRef url;
    if (!URLString || RSStringGetLength(URLString) == 0) return nil;
    url = _RSURLAlloc(allocator);
    if (url) {
        _RSURLInitWithString(url, URLString, baseURL);
    }
    return url;
}

RSExport RSURLRef RSURLCreateAbsoluteURLWithBytes(RSAllocatorRef alloc, const RSBitU8 *relativeURLBytes, RSIndex length, RSStringEncoding encoding, RSURLRef baseURL, BOOL useCompatibilityMode) {
    RSURLRef result = nil;
    
    // if not useCompatibilityMode, use RSURLCreateWithBytes and then RSURLCopyAbsoluteURL if there's a baseURL
    if ( !useCompatibilityMode ) {
        RSURLRef url = RSURLCreateWithBytes(alloc, relativeURLBytes, length, encoding, baseURL);
        if ( url != nil ) {
            if ( baseURL != nil ) {
                result = RSURLCopyAbsoluteURL(url);
                RSRelease(url);
            } else {
                result = url;
            }
        }
    } else {
        RSBitU32 absFlags = 0;
        RSRange *absRanges;
        RSStringRef absString = nil;
        BOOL absStringIsMutable = NO;
        RSURLRef absURL;
        RSStringRef relativeString;
        BOOL useEightBitStringEncoding;
        
        useEightBitStringEncoding = ( __RSStringEncodingIsSupersetOfASCII(encoding) && __RSBytesInASCII(relativeURLBytes, length) );
        relativeString = RSStringCreateWithBytes(alloc, relativeURLBytes, length, useEightBitStringEncoding ? RSStringEncodingUTF8 : encoding, NO);
        if ( relativeString != nil ) {
            if (!baseURL) {
                if ( useEightBitStringEncoding ) {
                    absFlags |= USES_EIGHTBITSTRINGENCODING;
                }
                absString = relativeString;
            } else {
                UniChar ch = RSStringGetCharacterAtIndex(relativeString, 0);
                if (ch == '?' || ch == ';' || ch == '#') {
                    // Nothing but parameter + query + fragment; append to the baseURL string
                    RSStringRef baseString;
                    if (RS_IS_OBJC(__RSURLTypeID, baseURL)) {
                        baseString = RSURLGetString(baseURL);
                    } else {
                        baseString = baseURL->_string;
                    }
                    absString = RSStringCreateMutable(alloc, RSStringGetLength(baseString) + RSStringGetLength(relativeString));
                    RSStringAppendString((RSMutableStringRef)absString, baseString);
                    RSStringAppendString((RSMutableStringRef)absString, relativeString);
                    absStringIsMutable = YES;
                } else {
                    RSBitU32 relFlags = 0;
                    RSRange *relRanges;
                    RSStringRef relString = nil;
                    _parseComponents(alloc, relativeString, baseURL, &relFlags, &relRanges);
                    if (relFlags & HAS_SCHEME) {
                        RSStringRef baseScheme = RSURLCopyScheme(baseURL);
                        RSRange relSchemeRange = _rangeForComponent(relFlags, relRanges, HAS_SCHEME);
                        if (baseScheme && RSStringGetLength(baseScheme) == relSchemeRange.length && RSStringHasPrefix(relativeString, baseScheme)) {
                            relString = RSStringCreateWithSubstring(alloc, relativeString, RSMakeRange(relSchemeRange.length+1, RSStringGetLength(relativeString) - relSchemeRange.length - 1));
                            RSAllocatorDeallocate(alloc, relRanges);
                            relFlags = 0;
                            _parseComponents(alloc, relString, baseURL, &relFlags, &relRanges);
                        } else {
                            // Discard the base string; the relative string is absolute and we're not in the funky edge case where the schemes match
                            if ( useEightBitStringEncoding ) {
                                absFlags |= USES_EIGHTBITSTRINGENCODING;
                            }
                            RSRetain(relativeString);
                            absString = relativeString;
                        }
                        if (baseScheme) RSRelease(baseScheme);
                    } else {
                        RSRetain(relativeString);
                        relString = relativeString;
                    }
                    if (!absString) {
                        if (!RS_IS_OBJC(__RSURLTypeID, baseURL)) {
                            absString = resolveAbsoluteURLString(alloc, relString, relFlags, relRanges, baseURL->_string, baseURL->_flags, baseURL->_ranges);
                        } else {
                            RSStringRef baseString;
                            RSBitU32 baseFlags = 0;
                            RSRange *baseRanges;
                            if (RS_IS_OBJC(__RSURLTypeID, baseURL)) {
                                baseString = RSURLGetString(baseURL);
                            } else {
                                baseString = baseURL->_string;
                            }
                            _parseComponents(alloc, baseString, nil, &baseFlags, &baseRanges);
                            absString = resolveAbsoluteURLString(alloc, relString, relFlags, relRanges, baseString, baseFlags, baseRanges);
                            RSAllocatorDeallocate(alloc, baseRanges);
                        }
                        absStringIsMutable = YES;
                    }
                    if (relString) RSRelease(relString);
                    RSAllocatorDeallocate(alloc, relRanges);
                }
                RSRelease(relativeString);
            }
        }
        if ( absString ) {
            _parseComponents(alloc, absString, nil, &absFlags, &absRanges);
            if (absFlags & HAS_PATH) {
                RSRange pathRg = _rangeForComponent(absFlags, absRanges, HAS_PATH);
                // This is expensive, but it allows us to reuse _resolvedPath.  It should be cleaned up to get this allocation removed at some point. - REW
                UniChar *buf = (UniChar *)RSAllocatorAllocate(alloc, sizeof(UniChar) * (pathRg.length + 1));
                RSStringRef newPath;
                RSStringGetCharacters(absString, pathRg, buf);
                buf[pathRg.length] = '\0';
                newPath = _resolvedPath(buf, buf + pathRg.length, '/', YES, NO, alloc);
                if (RSStringGetLength(newPath) != pathRg.length) {
                    if (!absStringIsMutable) {
                        RSStringRef tmp = RSStringCreateMutableCopy(alloc, RSStringGetLength(absString), absString);
                        RSRelease(absString);
                        absString = tmp;
                    }
                    RSStringReplace((RSMutableStringRef)absString, pathRg, newPath);
                }
                RSRelease(newPath);
                // Do not deallocate buf; newPath took ownership of it.
            }
            RSAllocatorDeallocate(alloc, absRanges);
            absURL = _RSURLCreateWithArbitraryString(alloc, absString, nil);
            RSRelease(absString);
            if (absURL) {
                ((struct __RSURL *)absURL)->_encoding = encoding;
#if DEBUG_URL_MEMORY_USAGE
                if ( encoding != RSStringEncodingUTF8 ) {
                    numNonUTF8EncodedURLs++;
                }
#endif
            }
            result = absURL;
        }
    }
    
    return ( result );
}

/* This function is this way because I pulled it out of _resolvedURLPath (so that _resolvedFileSystemPath could use it), and I didn't want to spend a bunch of energy reworking the code.  So instead of being a bit more intelligent about inputs, it just demands a slightly perverse set of parameters, to match the old _resolvedURLPath code.  -- REW, 6/14/99 */
static RSStringRef _resolvedPath(UniChar *pathStr, UniChar *end, UniChar pathDelimiter, BOOL stripLeadingDotDots, BOOL stripTrailingDelimiter, RSAllocatorRef alloc) {
    UniChar *idx = pathStr;
    while (idx < end) {
        if (*idx == '.') {
            if (idx+1 == end) {
                if (idx != pathStr) {
                    *idx = '\0';
                    end = idx;
                }
                break;
            } else if (*(idx+1) == pathDelimiter) {
                if (idx + 2 != end || idx != pathStr) {
                    memmove(idx, idx+2, (end-(idx+2)+1) * sizeof(UniChar));
                    end -= 2;
                    continue;
                } else {
                    // Do not delete the sole path component
                    break;
                }
            } else if (( end-idx >= 2 ) &&  *(idx+1) == '.' && (idx+2 == end || (( end-idx > 2 ) && *(idx+2) == pathDelimiter))) {
                if (idx - pathStr >= 2) {
                    // Need at least 2 characters between index and pathStr, because we know if index != newPath, then *(index-1) == pathDelimiter, and we need something before that to compact out.
                    UniChar *lastDelim = idx-2;
                    while (lastDelim >= pathStr && *lastDelim != pathDelimiter) lastDelim --;
                    lastDelim ++;
                    if (lastDelim != idx && (idx-lastDelim != 3 || *lastDelim != '.' || *(lastDelim +1) != '.')) {
                        // We have a genuine component to compact out
                        if (idx+2 != end) {
                            unsigned numCharsToMove = (unsigned)(end - (idx+3) + 1); // +1 to move the '\0' as well
                            memmove(lastDelim, idx+3, numCharsToMove * sizeof(UniChar));
                            end -= (idx + 3 - lastDelim);
                            idx = lastDelim;
                            continue;
                        } else if (lastDelim != pathStr) {
                            *lastDelim = '\0';
                            end = lastDelim;
                            break;
                        } else {
                            // Don't allow the path string to devolve to the empty string.  Fall back to "." instead. - REW
                            pathStr[0] = '.';
                            pathStr[1] = '/';
                            pathStr[2] = '\0';
							end = & pathStr[3];
                            break;
                        }
                    }
                } else if (stripLeadingDotDots) {
                    if (idx + 3 != end) {
                        unsigned numCharsToMove = (unsigned)(end - (idx + 3) + 1);
                        memmove(idx, idx+3, numCharsToMove * sizeof(UniChar));
                        end -= 3;
                        continue;
                    } else {
                        // Do not devolve the last path component
                        break;
                    }
                }
            }
        }
		while (idx < end && *idx != pathDelimiter) idx ++;
        idx ++;
    }
    if (stripTrailingDelimiter && end > pathStr && end-1 != pathStr && *(end-1) == pathDelimiter) {
        end --;
    }
    return RSStringCreateWithCharactersNoCopy(alloc, pathStr, end - pathStr, alloc);
}

static RSMutableStringRef resolveAbsoluteURLString(RSAllocatorRef alloc, RSStringRef relString, RSBitU32 relFlags, RSRange *relRanges, RSStringRef baseString, RSBitU32 baseFlags, RSRange *baseRanges) {
    RSMutableStringRef newString = RSStringCreateMutable(alloc, 0);
    RSIndex bufLen = RSStringGetLength(baseString) + RSStringGetLength(relString); // Overkill, but guarantees we never allocate again
    UniChar *buf = (UniChar *)RSAllocatorAllocate(alloc, bufLen * sizeof(UniChar));
    RSRange rg;
    
    rg = _rangeForComponent(baseFlags, baseRanges, HAS_SCHEME);
    if (rg.location != RSNotFound) {
        RSStringGetCharacters(baseString, rg, buf);
        RSStringAppendCharacters(newString, buf, rg.length);
        RSStringAppendCString(newString, ":", RSStringEncodingASCII);
    }
    
    if (relFlags & NET_LOCATION_MASK) {
        RSStringAppendString(newString, relString);
    } else {
        RSStringAppendCString(newString, "//", RSStringEncodingASCII);
        rg = _netLocationRange(baseFlags, baseRanges);
        if (rg.location != RSNotFound) {
            RSStringGetCharacters(baseString, rg, buf);
            RSStringAppendCharacters(newString, buf, rg.length);
        }
        
        if (relFlags & HAS_PATH) {
            RSRange relPathRg = _rangeForComponent(relFlags, relRanges, HAS_PATH);
            RSRange basePathRg = _rangeForComponent(baseFlags, baseRanges, HAS_PATH);
            RSStringRef newPath;
            BOOL useRelPath = NO;
            BOOL useBasePath = NO;
            if (basePathRg.location == RSNotFound) {
                useRelPath = YES;
            } else if (relPathRg.length == 0) {
                useBasePath = YES;
            } else if (RSStringGetCharacterAtIndex(relString, relPathRg.location) == '/') {
                useRelPath = YES;
            } else if (basePathRg.location == RSNotFound || basePathRg.length == 0) {
                useRelPath = YES;
            }
            if (useRelPath) {
                
                newPath = RSStringCreateWithSubstring(alloc, relString, relPathRg);
            } else if (useBasePath) {
                newPath = RSStringCreateWithSubstring(alloc, baseString, basePathRg);
            } else {
                // #warning FIXME - Get rid of this allocation
                UniChar *newPathBuf = (UniChar *)RSAllocatorAllocate(alloc, sizeof(UniChar) * (relPathRg.length + basePathRg.length + 1));
                UniChar *idx, *end;
                RSStringGetCharacters(baseString, basePathRg, newPathBuf);
                idx = newPathBuf + basePathRg.length - 1;
                while (idx != newPathBuf && *idx != '/') idx --;
                if (*idx == '/') idx ++;
                RSStringGetCharacters(relString, relPathRg, idx);
                end = idx + relPathRg.length;
                *end = 0;
                newPath = _resolvedPath(newPathBuf, end, '/', NO, NO, alloc);
            }
            /* Under Win32 absolute path can begin with letter
             * so we have to add one '/' to the newString
             * (Sergey Zubarev)
             */
            // No - the input strings here are URL path strings, not Win32 paths.
            // Absolute paths should have had a '/' prepended before this point.
            // I have removed Sergey Zubarev's change and left his comment (and
            // this one) as a record. - REW, 1/5/2004
            
            // if the relative URL does not begin with a slash and
            // the base does not end with a slash, add a slash
            if ((basePathRg.location == RSNotFound || basePathRg.length == 0) && RSStringGetCharacterAtIndex(newPath, 0) != '/') {
                RSStringAppendCString(newString, "/", RSStringEncodingASCII);
            }
            
            RSStringAppendString(newString, newPath);
            RSRelease(newPath);
            rg.location = relPathRg.location + relPathRg.length;
            rg.length = RSStringGetLength(relString);
            if (rg.length > rg.location) {
                rg.length -= rg.location;
                RSStringGetCharacters(relString, rg, buf);
                RSStringAppendCharacters(newString, buf, rg.length);
            }
        } else {
            rg = _rangeForComponent(baseFlags, baseRanges, HAS_PATH);
            if (rg.location != RSNotFound) {
                RSStringGetCharacters(baseString, rg, buf);
                RSStringAppendCharacters(newString, buf, rg.length);
            }
            
            if (!(relFlags & RESOURCE_SPECIFIER_MASK)) {
                // ???  Can this ever happen?
                RSBitU32 rsrRSlag = _firstResourceSpecifierFlag(baseFlags);
                if (rsrRSlag) {
                    rg.location = _rangeForComponent(baseFlags, baseRanges, rsrRSlag).location;
                    rg.length = RSStringGetLength(baseString) - rg.location;
                    rg.location --; // To pick up the separator
                    rg.length ++;
                    RSStringGetCharacters(baseString, rg, buf);
                    RSStringAppendCharacters(newString, buf, rg.length);
                }
            } else if (relFlags & HAS_PARAMETERS) {
                rg = _rangeForComponent(relFlags, relRanges, HAS_PARAMETERS);
                rg.location --; // To get the semicolon that starts the parameters
                rg.length = RSStringGetLength(relString) - rg.location;
                RSStringGetCharacters(relString, rg, buf);
                RSStringAppendCharacters(newString, buf, rg.length);
            } else {
                // Sigh; we have to resolve these against one another
                rg = _rangeForComponent(baseFlags, baseRanges, HAS_PARAMETERS);
                if (rg.location != RSNotFound) {
                    RSStringAppendCString(newString, ";", RSStringEncodingASCII);
                    RSStringGetCharacters(baseString, rg, buf);
                    RSStringAppendCharacters(newString, buf, rg.length);
                }
                rg = _rangeForComponent(relFlags, relRanges, HAS_QUERY);
                if (rg.location != RSNotFound) {
                    RSStringAppendCString(newString, "?", RSStringEncodingASCII);
                    RSStringGetCharacters(relString, rg, buf);
                    RSStringAppendCharacters(newString, buf, rg.length);
                } else {
                    rg = _rangeForComponent(baseFlags, baseRanges, HAS_QUERY);
                    if (rg.location != RSNotFound) {
                        RSStringAppendCString(newString, "?", RSStringEncodingASCII);
                        RSStringGetCharacters(baseString, rg, buf);
                        RSStringAppendCharacters(newString, buf, rg.length);
                    }
                }
                // Only the relative portion of the URL can supply the fragment; otherwise, what would be in the relativeURL?
                rg = _rangeForComponent(relFlags, relRanges, HAS_FRAGMENT);
                if (rg.location != RSNotFound) {
                    RSStringAppendCString(newString, "#", RSStringEncodingASCII);
                    RSStringGetCharacters(relString, rg, buf);
                    RSStringAppendCharacters(newString, buf, rg.length);
                }
            }
        }
    }
    RSAllocatorDeallocate(alloc, buf);
    return newString;
}

RSExport RSURLRef RSURLCopyAbsoluteURL(RSURLRef  relativeURL) {
    RSURLRef  anURL, base;
    RSAllocatorRef alloc = RSGetAllocator(relativeURL);
    RSStringRef baseString, newString;
    RSBitU32 baseFlags;
    RSRange *baseRanges;
    BOOL baseIsObjC;
    
    RSAssert1(relativeURL != nil, __RSLogAssertion, "%s(): Cannot create an absolute URL from a nil relative URL", __PRETTY_FUNCTION__);
    //    if (RS_IS_OBJC(__RSURLTypeID, relativeURL)) {
    //        anURL = (RSURLRef) RS_OBJC_CALLV((NSURL *)relativeURL, absoluteURL);
    //        if (anURL) RSRetain(anURL);
    //        return anURL;
    //    }
    
    __RSGenericValidInstance(relativeURL, __RSURLTypeID);
    
    base = relativeURL->_base;
    if (!base) {
        return (RSURLRef)RSRetain(relativeURL);
    }
    baseIsObjC = RS_IS_OBJC(__RSURLTypeID, base);
    
    if (!baseIsObjC) {
        baseString = base->_string;
        baseFlags = base->_flags;
        baseRanges = base->_ranges;
    } else {
        baseString = RSURLGetString(base);
        baseFlags = 0;
        baseRanges = nil;
        _parseComponents(alloc, baseString, nil, &baseFlags, &baseRanges);
    }
    
    newString = resolveAbsoluteURLString(alloc, relativeURL->_string, relativeURL->_flags, relativeURL->_ranges, baseString, baseFlags, baseRanges);
    if (baseIsObjC) {
        RSAllocatorDeallocate(alloc, baseRanges);
    }
    anURL = _RSURLCreateWithArbitraryString(alloc, newString, nil);
    RSRelease(newString);
    ((struct __RSURL *)anURL)->_encoding = relativeURL->_encoding;
#if DEBUG_URL_MEMORY_USAGE
    if ( relativeURL->_encoding != RSStringEncodingUTF8 ) {
        numNonUTF8EncodedURLs++;
    }
#endif
    return anURL;
}


/*******************/
/* Basic accessors */
/*******************/
RSStringEncoding _RSURLGetEncoding(RSURLRef url) {
    return url->_encoding;
}

RSExport BOOL RSURLCanBeDecomposed(RSURLRef  anURL) {
    anURL = _RSURLFromNSURL(anURL);
    return ((anURL->_flags & IS_DECOMPOSABLE) != 0);
}

RSExport RSStringRef  RSURLGetString(RSURLRef  url) {
    if (!url) return nil;
    RS_OBJC_FUNCDISPATCHV(__RSURLTypeID, RSStringRef, (NSURL *)url, relativeString);
    if (!_haveTestedOriginalString(url)) {
        computeSanitizedString(url);
    }
    if (url->_flags & ORIGINAL_AND_URL_STRINGS_MATCH) {
        return url->_string;
    } else {
        return _getSanitizedString( url );
    }
}

RSExport RSIndex RSURLGetBytes(RSURLRef url, RSBitU8 *buffer, RSIndex bufferLength) {
    RSIndex length, charsConverted, usedLength;
    RSStringRef string;
    RSStringEncoding enc;
    if (RS_IS_OBJC(__RSURLTypeID, url)) {
        string = RSURLGetString(url);
        enc = RSStringEncodingUTF8;
    } else {
        string = url->_string;
        enc = url->_encoding;
    }
    length = RSStringGetLength(string);
    charsConverted = RSStringGetBytes(string, RSMakeRange(0, length), enc, 0, NO, buffer, bufferLength, &usedLength);
    if (charsConverted != length) {
        return -1;
    } else {
        return usedLength;
    }
}

RSExport RSURLRef  RSURLGetBaseURL(RSURLRef  anURL) {
    RS_OBJC_FUNCDISPATCHV(__RSURLTypeID, RSURLRef, (NSURL *)anURL, baseURL);
    return anURL->_base;
}

// Assumes the URL is already parsed
static RSRange _rangeForComponent(RSBitU32 flags, RSRange *ranges, RSBitU32 compFlag) {
    RSBitU32 idx = 0;
    if (!(flags & compFlag)) return RSMakeRange(RSNotFound, 0);
    while (!(compFlag & 1)) {
        compFlag = compFlag >> 1;
        if (flags & 1) {
            idx ++;
        }
        flags = flags >> 1;
    }
    return ranges[idx];
}

static RSStringRef _retainedComponentString(RSURLRef url, RSBitU32 compFlag, BOOL fromOriginalString, BOOL removePercentEscapes) {
    RSRange rg;
    RSStringRef comp;
    RSAllocatorRef alloc = RSGetAllocator(url);
    if (removePercentEscapes) {
        fromOriginalString = YES;
    }
    rg = _rangeForComponent(url->_flags, url->_ranges, compFlag);
    if (rg.location == RSNotFound) {
        comp = nil;
    }
    else {
        if ( compFlag & HAS_SCHEME ) {
            switch ( _getSchemeTypeFromFlags(url->_flags) ) {
                case kHasHttpScheme:
                    comp = (RSStringRef)RSRetain(RSURLHTTPScheme);
                    break;
                    
                case kHasHttpsScheme:
                    comp = (RSStringRef)RSRetain(RSURLHTTPSScheme);
                    break;
                    
                case kHasFileScheme:
                    comp = (RSStringRef)RSRetain(RSURLFileScheme);
                    break;
                    
                case kHasDataScheme:
                    comp = (RSStringRef)RSRetain(RSURLDataScheme);
                    break;
                    
                case kHasFtpScheme:
                    comp = (RSStringRef)RSRetain(RSURLFTPScheme);
                    break;
                    
                default:
                    comp = RSStringCreateWithSubstring(alloc, url->_string, rg);
                    break;
            }
        }
        else {
            comp = RSStringCreateWithSubstring(alloc, url->_string, rg);
        }
        
        if (!fromOriginalString) {
            if (!_haveTestedOriginalString(url)) {
                computeSanitizedString(url);
            }
            if (!(url->_flags & ORIGINAL_AND_URL_STRINGS_MATCH) && (_getAdditionalDataFlags(url) & compFlag)) {
                RSStringRef newComp = correctedComponent(comp, compFlag, url->_encoding);
                RSRelease(comp);
                comp = newComp;
            }
        }
        if (removePercentEscapes) {
            RSStringRef tmp;
            if (url->_encoding == RSStringEncodingUTF8) {
                tmp = RSURLCreateStringByReplacingPercentEscapes(alloc, comp, RSSTR(""));
            } else {
                tmp = RSURLCreateStringByReplacingPercentEscapesUsingEncoding(alloc, comp, RSSTR(""), url->_encoding);
            }
            RSRelease(comp);
            comp = tmp;
        }
        
    }
    return comp;
}

RSExport RSStringRef  RSURLCopyScheme(RSURLRef  anURL) {
    RSStringRef scheme;
    if (RS_IS_OBJC(__RSURLTypeID, anURL)) {
        //        scheme = (RSStringRef) RS_OBJC_CALLV((NSURL *)anURL, scheme);
        //        if ( scheme ) {
        //            RSRetain(scheme);
        //        }
        return nil;
    }
    else {
        switch ( _getSchemeTypeFromFlags(anURL->_flags) ) {
            case kHasHttpScheme:
                scheme = (RSStringRef)RSRetain(RSURLHTTPScheme);
                break;
                
            case kHasHttpsScheme:
                scheme = (RSStringRef)RSRetain(RSURLHTTPSScheme);
                break;
                
            case kHasFileScheme:
                scheme = (RSStringRef)RSRetain(RSURLFileScheme);
                break;
                
            case kHasDataScheme:
                scheme = (RSStringRef)RSRetain(RSURLDataScheme);
                break;
                
            case kHasFtpScheme:
                scheme = (RSStringRef)RSRetain(RSURLFTPScheme);
                break;
                
            default:
                scheme = _retainedComponentString(anURL, HAS_SCHEME, YES, NO);
                if ( !scheme ) {
                    if (anURL->_base) {
                        scheme = RSURLCopyScheme(anURL->_base);
                    } else {
                        scheme = nil;
                    }
                }
                break;
        }
    }
    return ( scheme );
}

static RSRange _netLocationRange(RSBitU32 flags, RSRange *ranges) {
    RSRange netRgs[4];
    RSRange netRg = {RSNotFound, 0};
    RSIndex i, c = 4;
    
    if ((flags & NET_LOCATION_MASK) == 0) return RSMakeRange(RSNotFound, 0);
    
    netRgs[0] = _rangeForComponent(flags, ranges, HAS_USER);
    netRgs[1] = _rangeForComponent(flags, ranges, HAS_PASSWORD);
    netRgs[2] = _rangeForComponent(flags, ranges, HAS_HOST);
    netRgs[3] = _rangeForComponent(flags, ranges, HAS_PORT);
    for (i = 0; i < c; i ++) {
        if (netRgs[i].location == RSNotFound) continue;
        if (netRg.location == RSNotFound) {
            netRg = netRgs[i];
        } else {
            netRg.length = netRgs[i].location + netRgs[i].length - netRg.location;
        }
    }
    return netRg;
}

RSExport RSStringRef RSURLCopyNetLocation(RSURLRef  anURL) {
    anURL = _RSURLFromNSURL(anURL);
    if (anURL->_flags & NET_LOCATION_MASK) {
        // We provide the net location
        RSRange netRg = _netLocationRange(anURL->_flags, anURL->_ranges);
        RSStringRef netLoc;
        if (!_haveTestedOriginalString(anURL)) {
            computeSanitizedString(anURL);
        }
        if (!(anURL->_flags & ORIGINAL_AND_URL_STRINGS_MATCH) && (_getAdditionalDataFlags(anURL) & (USER_DIFFERS | PASSWORD_DIFFERS | HOST_DIFFERS | PORT_DIFFERS))) {
            // Only thing that can come before the net location is the scheme.  It's impossible for the scheme to contain percent escapes.  Therefore, we can use the location of netRg in _sanatizedString, just not the length.
            RSRange netLocEnd;
            RSStringRef sanitizedString = _getSanitizedString(anURL);
            netRg.length = RSStringGetLength(sanitizedString) - netRg.location;
            if (RSStringFindWithOptions(sanitizedString, RSSTR("/"), netRg, 0, &netLocEnd)) {
                netRg.length = netLocEnd.location - netRg.location;
            }
            netLoc = RSStringCreateWithSubstring(RSGetAllocator(anURL), sanitizedString, netRg);
        } else {
            netLoc = RSStringCreateWithSubstring(RSGetAllocator(anURL), anURL->_string, netRg);
        }
        return netLoc;
    } else if (anURL->_base) {
        return RSURLCopyNetLocation(anURL->_base);
    } else {
        return nil;
    }
}

// NOTE - if you want an absolute path, you must first get the absolute URL.  If you want a file system path, use the file system methods above.
RSExport RSStringRef  RSURLCopyPath(RSURLRef  anURL) {
    anURL = _RSURLFromNSURL(anURL);
    return _retainedComponentString(anURL, HAS_PATH, NO, NO);
}

/* nil if RSURLCanBeDecomposed(anURL) is NO; also does not resolve the URL against its base.  See also RSCreateAbsoluteURL().  Note that, strictly speaking, any leading '/' is not considered part of the URL's path, although its presence or absence determines whether the path is absolute.  RSURLCopyPath()'s return value includes any leading slash (giving the path the normal POSIX appearance); RSURLCopyStrictPath()'s return value omits any leading slash, and uses isAbsolute to report whether the URL's path is absolute.
 
 RSURLCopyFileSystemPath() returns the URL's path as a file system path for the given path style.  All percent escape sequences are replaced.  The URL is not resolved against its base before computing the path.
 */
RSExport RSStringRef RSURLCopyStrictPath(RSURLRef anURL, BOOL *isAbsolute) {
    RSStringRef path = RSURLCopyPath(anURL);
    if (!path || RSStringGetLength(path) == 0) {
        if (path) RSRelease(path);
        if (isAbsolute) *isAbsolute = NO;
        return nil;
    }
    if (RSStringGetCharacterAtIndex(path, 0) == '/') {
        RSStringRef tmp;
        if (isAbsolute) *isAbsolute = YES;
        tmp = RSStringCreateWithSubstring(RSGetAllocator(path), path, RSMakeRange(1, RSStringGetLength(path)-1));
        RSRelease(path);
        path = tmp;
    } else {
        if (isAbsolute) *isAbsolute = NO;
    }
    return path;
}

RSExport BOOL RSURLHasDirectoryPath(RSURLRef  anURL) {
    __RSGenericValidInstance(anURL, __RSURLTypeID);
    if (!anURL->_base || (anURL->_flags & (HAS_PATH | NET_LOCATION_MASK))) {
        return ((anURL->_flags & IS_DIRECTORY) != 0);
    }
    else {
        return RSURLHasDirectoryPath(anURL->_base);
    }
}

static RSBitU32 _firstResourceSpecifierFlag(RSBitU32 flags) {
    RSBitU32 firstRsrcSpeRSlag = 0;
    RSBitU32 flag = HAS_FRAGMENT;
    while (flag != HAS_PATH) {
        if (flags & flag) {
            firstRsrcSpeRSlag = flag;
        }
        flag = flag >> 1;
    }
    return firstRsrcSpeRSlag;
}

RSExport RSStringRef  RSURLCopyResourceSpecifier(RSURLRef  anURL) {
    anURL = _RSURLFromNSURL(anURL);
    __RSGenericValidInstance(anURL, __RSURLTypeID);
    if (!(anURL->_flags & IS_DECOMPOSABLE)) {
        RSRange schemeRg = _rangeForComponent(anURL->_flags, anURL->_ranges, HAS_SCHEME);
        RSIndex base = schemeRg.location + schemeRg.length + 1;
        if (!_haveTestedOriginalString(anURL)) {
            computeSanitizedString(anURL);
        }
        
        RSStringRef sanitizedString = _getSanitizedString(anURL);
        
        if (sanitizedString) {
            // It is impossible to have a percent escape in the scheme (if there were one, we would have considered the URL a relativeURL with a  colon in the path instead), so this range computation is always safe.
            return RSStringCreateWithSubstring(RSGetAllocator(anURL), sanitizedString, RSMakeRange(base, RSStringGetLength(sanitizedString)-base));
        } else {
            return RSStringCreateWithSubstring(RSGetAllocator(anURL), anURL->_string, RSMakeRange(base, RSStringGetLength(anURL->_string)-base));
        }
    } else {
        RSBitU32 firstRsrcSpeRSlag = _firstResourceSpecifierFlag(anURL->_flags);
        RSBitU32 flag;
        if (firstRsrcSpeRSlag) {
            BOOL canUseOriginalString = YES;
            BOOL canUseSanitizedString = YES;
            RSAllocatorRef alloc = RSGetAllocator(anURL);
            if (!_haveTestedOriginalString(anURL)) {
                computeSanitizedString(anURL);
            }
            
            RSBitU32 additionalDataFlags = _getAdditionalDataFlags(anURL);
            RSStringRef sanitizedString = _getSanitizedString(anURL);
            
            if (!(anURL->_flags & ORIGINAL_AND_URL_STRINGS_MATCH)) {
                // See if any pieces in the resource specifier differ between sanitized string and original string
                for (flag = firstRsrcSpeRSlag; flag != (HAS_FRAGMENT << 1); flag = flag << 1) {
                    if (additionalDataFlags & flag) {
                        canUseOriginalString = NO;
                        break;
                    }
                }
            }
            if (!canUseOriginalString) {
                // If none of the pieces prior to the first resource specifier flag differ, then we can use the offset from the original string as the offset in the sanitized string.
                for (flag = firstRsrcSpeRSlag >> 1; flag != 0; flag = flag >> 1) {
                    if (additionalDataFlags & flag) {
                        canUseSanitizedString = NO;
                        break;
                    }
                }
            }
            if (canUseOriginalString) {
                RSRange rg = _rangeForComponent(anURL->_flags, anURL->_ranges, firstRsrcSpeRSlag);
                rg.location --; // Include the character that demarcates the component
                rg.length = RSStringGetLength(anURL->_string) - rg.location;
                return RSStringCreateWithSubstring(alloc, anURL->_string, rg);
            } else if (canUseSanitizedString) {
                RSRange rg = _rangeForComponent(anURL->_flags, anURL->_ranges, firstRsrcSpeRSlag);
                rg.location --; // Include the character that demarcates the component
                rg.length = RSStringGetLength(sanitizedString) - rg.location;
                return RSStringCreateWithSubstring(alloc, sanitizedString, rg);
            } else {
                // Must compute the correct string to return; just reparse....
                RSBitU32 sanFlags = 0;
                RSRange *sanRanges = nil;
                RSRange rg;
                _parseComponents(alloc, sanitizedString, anURL->_base, &sanFlags, &sanRanges);
                rg = _rangeForComponent(sanFlags, sanRanges, firstRsrcSpeRSlag);
                RSAllocatorDeallocate(alloc, sanRanges);
                rg.location --; // Include the character that demarcates the component
                rg.length = RSStringGetLength(sanitizedString) - rg.location;
                return RSStringCreateWithSubstring(RSGetAllocator(anURL), sanitizedString, rg);
            }
        } else {
            // The resource specifier cannot possibly come from the base.
            return nil;
        }
    }
}

/*************************************/
/* Accessors that create new objects */
/*************************************/

// For the next four methods, it is important to realize that, if a URL supplies any part of the net location (host, user, port, or password), it must supply all of the net location (i.e. none of it comes from its base URL).  Also, it is impossible for a URL to be relative, supply none of the net location, and still have its (empty) net location take precedence over its base URL (because there's nothing that precedes the net location except the scheme, and if the URL supplied the scheme, it would be absolute, and there would be no base).
RSExport RSStringRef  RSURLCopyHostName(RSURLRef  anURL) {
    RSStringRef tmp;
    if (RS_IS_OBJC(__RSURLTypeID, anURL)) {
        //        tmp = (RSStringRef) RS_OBJC_CALLV((NSURL *)anURL, host);
        //        if (tmp) RSRetain(tmp);
        //        return tmp;
    }
    __RSGenericValidInstance(anURL, __RSURLTypeID);
    tmp = _retainedComponentString(anURL, HAS_HOST, YES, YES);
    if (tmp) {
        if (anURL->_flags & IS_IPV6_ENCODED) {
            // Have to strip off the brackets to get the YES hostname.
            // Assume that to be legal the first and last characters are brackets!
            RSStringRef	strippedHost = RSStringCreateWithSubstring(RSGetAllocator(anURL), tmp, RSMakeRange(1, RSStringGetLength(tmp) - 2));
            RSRelease(tmp);
            tmp = strippedHost;
        }
        return tmp;
    } else if (anURL->_base && !(anURL->_flags & NET_LOCATION_MASK) && !(anURL->_flags & HAS_SCHEME)) {
        return RSURLCopyHostName(anURL->_base);
    } else {
        return nil;
    }
}

extern BOOL __RSStringScanInteger(RSStringInlineBuffer *buf, RSTypeRef locale, SInt32 *indexPtr, BOOL doLonglong, void *result);
// Return -1 to indicate no port is specified
SInt32 RSURLGetPortNumber(RSURLRef  anURL) {
    RSStringRef port;
    if (RS_IS_OBJC(__RSURLTypeID, anURL)) {
        //        RSNumberRef rsPort = (RSNumberRef) RS_OBJC_CALLV((NSURL *)anURL, port);
        //        SInt32 num;
        //        if (rsPort && RSNumberGetValue(rsPort, RSNumberUnsignedInteger, &num)) return num;
        //        return -1;
    }
    __RSGenericValidInstance(anURL, __RSURLTypeID);
    port = _retainedComponentString(anURL, HAS_PORT, YES, NO);
    if (port) {
        RSBitU32 portNum, idx, length = (RSBitU32)RSStringGetLength(port);
        RSStringInlineBuffer buf;
        RSStringInitInlineBuffer(port, &buf, RSMakeRange(0, length));
        idx = 0;
        if (!__RSStringScanInteger(&buf, nil, (SInt32)&idx, NO, &portNum) || (idx != length)) {
            portNum = -1;
        }
        RSRelease(port);
        return portNum;
    } else if (anURL->_base && !(anURL->_flags & NET_LOCATION_MASK) && !(anURL->_flags & HAS_SCHEME)) {
        return RSURLGetPortNumber(anURL->_base);
    } else {
        return -1;
    }
}

RSExport RSStringRef  RSURLCopyUserName(RSURLRef  anURL) {
    RSStringRef user;
    if (RS_IS_OBJC(__RSURLTypeID, anURL)) {
        //        user = (RSStringRef) RS_OBJC_CALLV((NSURL *)anURL, user);
        //        if (user) RSRetain(user);
        //        return user;
    }
    __RSGenericValidInstance(anURL, __RSURLTypeID);
    user = _retainedComponentString(anURL, HAS_USER, YES, YES);
    if (user) {
        return user;
    } else if (anURL->_base && !(anURL->_flags & NET_LOCATION_MASK) && !(anURL->_flags & HAS_SCHEME)) {
        return RSURLCopyUserName(anURL->_base);
    } else {
        return nil;
    }
}

RSExport RSStringRef  RSURLCopyPassword(RSURLRef  anURL) {
    RSStringRef passwd;
    if (RS_IS_OBJC(__RSURLTypeID, anURL)) {
        //        passwd = (RSStringRef) RS_OBJC_CALLV((NSURL *)anURL, password);
        //        if (passwd) RSRetain(passwd);
        //        return passwd;
    }
    __RSGenericValidInstance(anURL, __RSURLTypeID);
    passwd = _retainedComponentString(anURL, HAS_PASSWORD, YES, YES);
    if (passwd) {
        return passwd;
    } else if (anURL->_base && !(anURL->_flags & NET_LOCATION_MASK) && !(anURL->_flags & HAS_SCHEME)) {
        return RSURLCopyPassword(anURL->_base);
    } else {
        return nil;
    }
}

// The NSURL methods do not deal with escaping escape characters at all; therefore, in order to properly bridge NSURL methods, and still provide the escaping behavior that we want, we need to create functions that match the ObjC behavior exactly, and have the public RSURL... functions call these. -- REW, 10/29/98

static RSStringRef  _unescapedParameterString(RSURLRef  anURL) {
    RSStringRef str;
    if (RS_IS_OBJC(__RSURLTypeID, anURL)) {
        //        str = (RSStringRef) RS_OBJC_CALLV((NSURL *)anURL, parameterString);
        //        if (str) RSRetain(str);
        //        return str;
    }
    __RSGenericValidInstance(anURL, __RSURLTypeID);
    str = _retainedComponentString(anURL, HAS_PARAMETERS, NO, NO);
    if (str) return str;
    if (!(anURL->_flags & IS_DECOMPOSABLE)) return nil;
    if (!anURL->_base || (anURL->_flags & (NET_LOCATION_MASK | HAS_PATH | HAS_SCHEME))) {
        return nil;
        // Parameter string definitely coming from the relative portion of the URL
    }
    return _unescapedParameterString( anURL->_base);
}

RSExport RSStringRef  RSURLCopyParameterString(RSURLRef  anURL, RSStringRef charactersToLeaveEscaped) {
    RSStringRef  param = _unescapedParameterString(anURL);
    if (param) {
        RSStringRef result;
        if (anURL->_encoding == RSStringEncodingUTF8) {
            result = RSURLCreateStringByReplacingPercentEscapes(RSGetAllocator(anURL), param, charactersToLeaveEscaped);
        } else {
            result = RSURLCreateStringByReplacingPercentEscapesUsingEncoding(RSGetAllocator(anURL), param, charactersToLeaveEscaped, anURL->_encoding);
        }
        RSRelease(param);
        return result;
    }
    return nil;
}

static RSStringRef  _unescapedQueryString(RSURLRef  anURL) {
    RSStringRef str;
    if (RS_IS_OBJC(__RSURLTypeID, anURL)) {
        //        str = (RSStringRef) RS_OBJC_CALLV((NSURL *)anURL, query);
        //        if (str) RSRetain(str);
        //        return str;
    }
    __RSGenericValidInstance(anURL, __RSURLTypeID);
    str = _retainedComponentString(anURL, HAS_QUERY, NO, NO);
    if (str) return str;
    if (!(anURL->_flags & IS_DECOMPOSABLE)) return nil;
    if (!anURL->_base || (anURL->_flags & (HAS_SCHEME | NET_LOCATION_MASK | HAS_PATH | HAS_PARAMETERS))) {
        return nil;
    }
    return _unescapedQueryString(anURL->_base);
}

RSExport RSStringRef  RSURLCopyQueryString(RSURLRef  anURL, RSStringRef  charactersToLeaveEscaped) {
    RSStringRef  query = _unescapedQueryString(anURL);
    if (query) {
        RSStringRef tmp;
        if (anURL->_encoding == RSStringEncodingUTF8) {
            tmp = RSURLCreateStringByReplacingPercentEscapes(RSGetAllocator(anURL), query, charactersToLeaveEscaped);
        } else {
            tmp = RSURLCreateStringByReplacingPercentEscapesUsingEncoding(RSGetAllocator(anURL), query, charactersToLeaveEscaped, anURL->_encoding);
        }
        RSRelease(query);
        return tmp;
    }
    return nil;
}

// Fragments are NEVER taken from a base URL
static RSStringRef  _unescapedFragment(RSURLRef  anURL) {
    RSStringRef str;
    if (RS_IS_OBJC(__RSURLTypeID, anURL)) {
        //        str = (RSStringRef) RS_OBJC_CALLV((NSURL *)anURL, fragment);
        //        if (str) RSRetain(str);
        //        return str;
    }
    __RSGenericValidInstance(anURL, __RSURLTypeID);
    str = _retainedComponentString(anURL, HAS_FRAGMENT, NO, NO);
    return str;
}

RSExport RSStringRef  RSURLCopyFragment(RSURLRef  anURL, RSStringRef  charactersToLeaveEscaped) {
    RSStringRef  fragment = _unescapedFragment(anURL);
    if (fragment) {
        RSStringRef tmp;
        if (anURL->_encoding == RSStringEncodingUTF8) {
            tmp = RSURLCreateStringByReplacingPercentEscapes(RSGetAllocator(anURL), fragment, charactersToLeaveEscaped);
        } else {
            tmp = RSURLCreateStringByReplacingPercentEscapesUsingEncoding(RSGetAllocator(anURL), fragment, charactersToLeaveEscaped, anURL->_encoding);
        }
        RSRelease(fragment);
        return tmp;
    }
    return nil;
}

static RSIndex insertionLocationForMask(RSURLRef url, RSOptionFlags mask) {
    RSIndex firstMaskFlag = 1;
    RSIndex lastComponentBeforeMask = 0;
    while (firstMaskFlag <= HAS_FRAGMENT) {
        if (firstMaskFlag & mask) break;
        if (url->_flags & firstMaskFlag) lastComponentBeforeMask = firstMaskFlag;
        firstMaskFlag = firstMaskFlag << 1;
    }
    if (lastComponentBeforeMask == 0) {
        // mask includes HAS_SCHEME
        return 0;
    } else if (lastComponentBeforeMask == HAS_SCHEME) {
        // Do not have to worry about the non-decomposable case here.  However, we must be prepared for the degenerate
        // case file:/path/immediately/without/host
        RSRange schemeRg = _rangeForComponent(url->_flags, url->_ranges, HAS_SCHEME);
        RSRange pathRg = _rangeForComponent(url->_flags, url->_ranges, HAS_PATH);
        if (schemeRg.length + 1 == pathRg.location) {
            return schemeRg.length + 1;
        } else {
            return schemeRg.length + 3;
        }
    } else {
        // For all other components, the separator precedes the component, so there's no need
        // to add extra chars to get to the next insertion point
        RSRange rg = _rangeForComponent(url->_flags, url->_ranges, (RSBitU32)lastComponentBeforeMask);
        return rg.location + rg.length;
    }
}

static RSRange _RSURLGetCharRangeForMask(RSURLRef url, RSOptionFlags mask, RSRange *charRangeWithSeparators) {
    RSOptionFlags currentOption;
    RSOptionFlags firstMaskFlag = HAS_SCHEME;
    BOOL haveReachedMask = NO;
    RSIndex beforeMask = 0;
    RSIndex afterMask = RSNotFound;
    RSRange *currRange = url->_ranges;
    RSRange maskRange = {RSNotFound, 0};
    for (currentOption = 1; currentOption <= HAS_FRAGMENT; currentOption = currentOption << 1) {
        if (!haveReachedMask && (currentOption & mask) != 0) {
            firstMaskFlag = currentOption;
            haveReachedMask = YES;
        }
        if (!(url->_flags & currentOption)) continue;
        if (!haveReachedMask) {
            beforeMask = currRange->location + currRange->length;
        } else if (currentOption <= mask) {
            if (maskRange.location == RSNotFound) {
                maskRange = *currRange;
            } else {
                maskRange.length = currRange->location + currRange->length - maskRange.location;
            }
        } else {
            afterMask = currRange->location;
            break;
        }
        currRange ++;
    }
    if (afterMask == RSNotFound) {
        afterMask = maskRange.location + maskRange.length;
    }
    charRangeWithSeparators->location = beforeMask;
    charRangeWithSeparators->length = afterMask - beforeMask;
    return maskRange;
}

static RSRange _getCharRangeInDecomposableURL(RSURLRef url, RSURLComponentType component, RSRange *rangeIncludingSeparators) {
    RSOptionFlags mask;
    switch (component) {
        case RSURLComponentScheme:
            mask = HAS_SCHEME;
            break;
        case RSURLComponentNetLocation:
            mask = NET_LOCATION_MASK;
            break;
        case RSURLComponentPath:
            mask = HAS_PATH;
            break;
        case RSURLComponentResourceSpecifier:
            mask = RESOURCE_SPECIFIER_MASK;
            break;
        case RSURLComponentUser:
            mask = HAS_USER;
            break;
        case RSURLComponentPassword:
            mask = HAS_PASSWORD;
            break;
        case RSURLComponentUserInfo:
            mask = HAS_USER | HAS_PASSWORD;
            break;
        case RSURLComponentHost:
            mask = HAS_HOST;
            break;
        case RSURLComponentPort:
            mask = HAS_PORT;
            break;
        case RSURLComponentParameterString:
            mask = HAS_PARAMETERS;
            break;
        case RSURLComponentQuery:
            mask = HAS_QUERY;
            break;
        case RSURLComponentFragment:
            mask = HAS_FRAGMENT;
            break;
        default:
            rangeIncludingSeparators->location = RSNotFound;
            rangeIncludingSeparators->length = 0;
            return RSMakeRange(RSNotFound, 0);
    }
    
    if ((url->_flags & mask) == 0) {
        rangeIncludingSeparators->location = insertionLocationForMask(url, mask);
        rangeIncludingSeparators->length = 0;
        return RSMakeRange(RSNotFound, 0);
    } else {
        return _RSURLGetCharRangeForMask(url, mask, rangeIncludingSeparators);
    }
}

static RSRange _getCharRangeInNonDecomposableURL(RSURLRef url, RSURLComponentType component, RSRange *rangeIncludingSeparators) {
    if (component == RSURLComponentScheme) {
        RSRange schemeRg = _rangeForComponent(url->_flags, url->_ranges, HAS_SCHEME);
        rangeIncludingSeparators->location = 0;
        rangeIncludingSeparators->length = schemeRg.length + 1;
        return schemeRg;
    } else if (component == RSURLComponentResourceSpecifier) {
        RSRange schemeRg = _rangeForComponent(url->_flags, url->_ranges, HAS_SCHEME);
        RSIndex stringLength = RSStringGetLength(url->_string);
        if (schemeRg.length + 1 == stringLength) {
            rangeIncludingSeparators->location = schemeRg.length + 1;
            rangeIncludingSeparators->length = 0;
            return RSMakeRange(RSNotFound, 0);
        } else {
            rangeIncludingSeparators->location = schemeRg.length;
            rangeIncludingSeparators->length = stringLength - schemeRg.length;
            return RSMakeRange(schemeRg.length + 1, rangeIncludingSeparators->length - 1);
        }
    } else {
        rangeIncludingSeparators->location = RSNotFound;
        rangeIncludingSeparators->length = 0;
        return RSMakeRange(RSNotFound, 0);
    }
    
}

RSExport RSRange RSURLGetByteRangeForComponent(RSURLRef url, RSURLComponentType component, RSRange *rangeIncludingSeparators) {
    RSRange charRange, charRangeWithSeparators;
    RSRange byteRange;
    RSAssert2(component > 0 && component < 13, __RSLogAssertion, "%s(): passed invalid component %d", __PRETTY_FUNCTION__, component);
    url = _RSURLFromNSURL(url);
    
    if (!(url->_flags & IS_DECOMPOSABLE)) {
        // Special-case this because non-decomposable URLs have a slightly strange flags setup
        charRange = _getCharRangeInNonDecomposableURL(url, component, &charRangeWithSeparators);
    } else {
        charRange = _getCharRangeInDecomposableURL(url, component, &charRangeWithSeparators);
    }
    
    if (charRangeWithSeparators.location == RSNotFound) {
        if (rangeIncludingSeparators) {
            rangeIncludingSeparators->location = RSNotFound;
            rangeIncludingSeparators->length = 0;
        }
        return RSMakeRange(RSNotFound, 0);
    } else if (rangeIncludingSeparators) {
        RSStringGetBytes(url->_string, RSMakeRange(0, charRangeWithSeparators.location), url->_encoding, 0, NO, nil, 0, &(rangeIncludingSeparators->location));
        
        if (charRange.location == RSNotFound) {
            byteRange = charRange;
            RSStringGetBytes(url->_string, charRangeWithSeparators, url->_encoding, 0, NO, nil, 0, &(rangeIncludingSeparators->length));
        } else {
            RSIndex maxCharRange = charRange.location + charRange.length;
            RSIndex maxCharRangeWithSeparators = charRangeWithSeparators.location + charRangeWithSeparators.length;
            
            if (charRangeWithSeparators.location == charRange.location) {
                byteRange.location = rangeIncludingSeparators->location;
            } else {
                RSIndex numBytes;
                RSStringGetBytes(url->_string, RSMakeRange(charRangeWithSeparators.location, charRange.location - charRangeWithSeparators.location), url->_encoding, 0, NO, nil, 0, &numBytes);
                byteRange.location = charRangeWithSeparators.location + numBytes;
            }
            RSStringGetBytes(url->_string, charRange, url->_encoding, 0, NO, nil, 0, &(byteRange.length));
            if (maxCharRangeWithSeparators == maxCharRange) {
                rangeIncludingSeparators->length = byteRange.location + byteRange.length - rangeIncludingSeparators->location;
            } else {
                RSIndex numBytes;
                RSRange rg;
                rg.location = maxCharRange;
                rg.length = maxCharRangeWithSeparators - rg.location;
                RSStringGetBytes(url->_string, rg, url->_encoding, 0, NO, nil, 0, &numBytes);
                rangeIncludingSeparators->length = byteRange.location + byteRange.length + numBytes - rangeIncludingSeparators->location;
            }
        }
    } else if (charRange.location == RSNotFound) {
        byteRange = charRange;
    } else {
        RSStringGetBytes(url->_string, RSMakeRange(0, charRange.location), url->_encoding, 0, NO, nil, 0, &(byteRange.location));
        RSStringGetBytes(url->_string, charRange, url->_encoding, 0, NO, nil, 0, &(byteRange.length));
    }
    return byteRange;
}

/* Component support */

static BOOL decomposeToNonHierarchical(RSURLRef url, RSURLComponentsNonHierarchical *components) {
    if ( RSURLGetBaseURL(url) != nil)  {
        components->scheme = nil;
    } else {
        components->scheme = RSURLCopyScheme(url);
    }
    components->schemeSpecific = RSURLCopyResourceSpecifier(url);
    return YES;
}

static RSURLRef composeFromNonHierarchical(RSAllocatorRef alloc, const RSURLComponentsNonHierarchical *components) {
    RSStringRef str;
    if (components->scheme) {
        UniChar ch = ':';
        str = RSStringCreateMutableCopy(alloc, RSStringGetLength(components->scheme) + 1 + (components->schemeSpecific ? RSStringGetLength(components->schemeSpecific): 0), components->scheme);
        RSStringAppendCharacters((RSMutableStringRef)str, &ch, 1);
        if (components->schemeSpecific) RSStringAppendString((RSMutableStringRef)str, components->schemeSpecific);
    } else if (components->schemeSpecific) {
        str = components->schemeSpecific;
        RSRetain(str);
    } else {
        str = nil;
    }
    if (str) {
        RSURLRef url = RSURLCreateWithString(alloc, str, nil);
        RSRelease(str);
        return url;
    } else {
        return nil;
    }
}

static BOOL decomposeToRFC1808(RSURLRef url, RSURLComponentsRFC1808 *components) {
    RSAllocatorRef alloc = RSGetAllocator(url);
    static RSStringRef emptyStr = nil;
    if (!emptyStr) {
        emptyStr = RSSTR("");
    }
    
    if (!RSURLCanBeDecomposed(url)) {
        return NO;
    }
    
    RSStringRef path = RSURLCopyPath(url);
    if (path) {
        components->pathComponents = RSStringCreateArrayBySeparatingStrings(alloc, path, RSSTR("/"));
        RSRelease(path);
    } else {
        components->pathComponents = nil;
    }
    components->baseURL = RSURLGetBaseURL(url);
    if (components->baseURL)  {
        RSRetain(components->baseURL);
        components->scheme = nil;
    } else {
        components->scheme = _retainedComponentString(url, HAS_SCHEME, YES, NO);
    }
    components->user = _retainedComponentString(url, HAS_USER, NO, NO);
    components->password = _retainedComponentString(url, HAS_PASSWORD, NO, NO);
    components->host = _retainedComponentString(url, HAS_HOST, NO, NO);
    if (url->_flags & HAS_PORT) {
        components->port = RSURLGetPortNumber(url);
    } else {
        components->port = RSNotFound;
    }
    components->parameterString = _retainedComponentString(url, HAS_PARAMETERS, NO, NO);
    components->query = _retainedComponentString(url, HAS_QUERY, NO, NO);
    components->fragment = _retainedComponentString(url, HAS_FRAGMENT, NO, NO);
    return YES;
}

static RSURLRef composeFromRFC1808(RSAllocatorRef alloc, const RSURLComponentsRFC1808 *comp) {
    RSMutableStringRef urlString = RSStringCreateMutable(alloc, 0);
    RSURLRef base = comp->baseURL;
    RSURLRef url;
    BOOL hadPrePathComponent = NO;
    if (comp->scheme) {
        base = nil;
        RSStringAppendString(urlString, comp->scheme);
        RSStringAppendString(urlString, RSSTR("://"));
        hadPrePathComponent = YES;
    }
    if (comp->user || comp->password) {
        if (comp->user) {
            RSStringAppendString(urlString, comp->user);
        }
        if (comp->password) {
            RSStringAppendString(urlString, RSSTR(":"));
            RSStringAppendString(urlString, comp->password);
        }
        RSStringAppendString(urlString, RSSTR("@"));
        hadPrePathComponent = YES;
    }
    if (comp->host) {
        RSStringAppendString(urlString, comp->host);
        hadPrePathComponent = YES;
    }
    if (comp->port != RSNotFound) {
        RSStringAppendStringWithFormat(urlString, RSSTR(":%d"), comp->port);
        hadPrePathComponent = YES;
    }
    
    if (hadPrePathComponent && (comp->pathComponents == nil || RSArrayGetCount( comp->pathComponents ) == 0 || RSStringGetLength((RSStringRef)RSArrayObjectAtIndex(comp->pathComponents, 0)) != 0)) {
        RSStringAppendString(urlString, RSSTR("/"));
    }
    if (comp->pathComponents) {
        RSStringRef pathStr = RSStringCreateByCombiningStrings(alloc, comp->pathComponents, RSSTR("/"));
        RSStringAppendString(urlString, pathStr);
        RSRelease(pathStr);
    }
    if (comp->parameterString) {
        RSStringAppendString(urlString, RSSTR(";"));
        RSStringAppendString(urlString, comp->parameterString);
    }
    if (comp->query) {
        RSStringAppendString(urlString, RSSTR("?"));
        RSStringAppendString(urlString, comp->query);
    }
    if (comp->fragment) {
        RSStringAppendString(urlString, RSSTR("#"));
        RSStringAppendString(urlString, comp->fragment);
    }
    url = RSURLCreateWithString(alloc, urlString, base);
    RSRelease(urlString);
    return url;
}

static BOOL decomposeToRFC2396(RSURLRef url, RSURLComponentsRFC2396 *comp) {
    RSAllocatorRef alloc = RSGetAllocator(url);
    RSURLComponentsRFC1808 oldComp;
    RSStringRef tmpStr;
    if (!decomposeToRFC1808(url, &oldComp)) {
        return NO;
    }
    comp->scheme = oldComp.scheme;
    if (oldComp.user) {
        if (oldComp.password) {
            comp->userinfo = RSStringCreateWithFormat(alloc, RSSTR("%R:%R"), oldComp.user, oldComp.password);
            RSRelease(oldComp.password);
            RSRelease(oldComp.user);
        } else {
            comp->userinfo = oldComp.user;
        }
    } else {
        comp->userinfo = nil;
    }
    comp->host = oldComp.host;
    comp->port = oldComp.port;
    if (!oldComp.parameterString) {
        comp->pathComponents = oldComp.pathComponents;
    } else {
        int length = (int)RSArrayGetCount(oldComp.pathComponents);
        comp->pathComponents = RSMutableCopy(alloc, oldComp.pathComponents);
        tmpStr = RSStringCreateWithFormat(alloc, RSSTR("%R;%R"), RSArrayObjectAtIndex(comp->pathComponents, length - 1), oldComp.parameterString);
        RSArraySetObjectAtIndex((RSMutableArrayRef)comp->pathComponents, length - 1, tmpStr);
        RSRelease(tmpStr);
        RSRelease(oldComp.pathComponents);
        RSRelease(oldComp.parameterString);
    }
    comp->query = oldComp.query;
    comp->fragment = oldComp.fragment;
    comp->baseURL = oldComp.baseURL;
    return YES;
}

static RSURLRef composeFromRFC2396(RSAllocatorRef alloc, const RSURLComponentsRFC2396 *comp) {
    RSMutableStringRef urlString = RSStringCreateMutable(alloc, 0);
    RSURLRef base = comp->baseURL;
    RSURLRef url;
    BOOL hadPrePathComponent = NO;
    if (comp->scheme) {
        base = nil;
        RSStringAppendString(urlString, comp->scheme);
        RSStringAppendString(urlString, RSSTR("://"));
        hadPrePathComponent = YES;
    }
    if (comp->userinfo) {
        RSStringAppendString(urlString, comp->userinfo);
        RSStringAppendString(urlString, RSSTR("@"));
        hadPrePathComponent = YES;
    }
    if (comp->host) {
        RSStringAppendString(urlString, comp->host);
        if (comp->port != RSNotFound) {
            RSStringAppendStringWithFormat(urlString, RSSTR(":%d"), comp->port);
        }
        hadPrePathComponent = YES;
    }
    if (hadPrePathComponent && (comp->pathComponents == nil || RSStringGetLength((RSStringRef)RSArrayObjectAtIndex(comp->pathComponents, 0)) != 0)) {
        RSStringAppendString(urlString, RSSTR("/"));
    }
    if (comp->pathComponents) {
        RSStringRef pathStr = RSStringCreateByCombiningStrings(alloc, comp->pathComponents, RSSTR("/"));
        RSStringAppendString(urlString, pathStr);
        RSRelease(pathStr);
    }
    if (comp->query) {
        RSStringAppendString(urlString, RSSTR("?"));
        RSStringAppendString(urlString, comp->query);
    }
    if (comp->fragment) {
        RSStringAppendString(urlString, RSSTR("#"));
        RSStringAppendString(urlString, comp->fragment);
    }
    url = RSURLCreateWithString(alloc, urlString, base);
    RSRelease(urlString);
    return url;
}

#undef RSURLCopyComponents
#undef RSURLCreateFromComponents

RSExport BOOL _RSURLCopyComponents(RSURLRef url, RSURLComponentDecomposition decompositionType, void *components) {
    url = _RSURLFromNSURL(url);
    switch (decompositionType) {
        case RSURLComponentDecompositionNonHierarchical:
            return decomposeToNonHierarchical(url, (RSURLComponentsNonHierarchical *)components);
        case RSURLComponentDecompositionRFC1808:
            return decomposeToRFC1808(url, (RSURLComponentsRFC1808 *)components);
        case RSURLComponentDecompositionRFC2396:
            return decomposeToRFC2396(url, (RSURLComponentsRFC2396 *)components);
        default:
            return NO;
    }
}

RSExport RSURLRef _RSURLCreateFromComponents(RSAllocatorRef alloc, RSURLComponentDecomposition decompositionType, const void *components) {
    switch (decompositionType) {
        case RSURLComponentDecompositionNonHierarchical:
            return composeFromNonHierarchical(alloc, (const RSURLComponentsNonHierarchical *)components);
        case RSURLComponentDecompositionRFC1808:
            return composeFromRFC1808(alloc, (const RSURLComponentsRFC1808 *)components);
        case RSURLComponentDecompositionRFC2396:
            return composeFromRFC2396(alloc, (const RSURLComponentsRFC2396 *)components);
        default:
            return nil;
    }
}

RSExport void *__RSURLReservedPtr(RSURLRef  url) {
    return _getReserved(url);
}

RSExport void __RSURLSetReservedPtr(RSURLRef  url, void *ptr) {
    _setReserved ( (struct __RSURL*) url, ptr );
}

RSExport void *__RSURLResourceInfoPtr(RSURLRef url) {
    return _getResourceInfo(url);
}

RSExport void __RSURLSetResourceInfoPtr(RSURLRef url, void *ptr) {
    _setResourceInfo ( (struct __RSURL*) url, ptr );
}

/* File system stuff */

/* HFSPath<->URLPath functions at the bottom of the file */
static RSArrayRef WindowsPathToURLComponents(RSStringRef path, RSAllocatorRef alloc, BOOL isDir) {
    RSArrayRef tmp;
    RSMutableArrayRef urlComponents = nil;
    RSIndex i=0;
    
    tmp = RSStringCreateArrayBySeparatingStrings(alloc, path, RSSTR("\\"));
    urlComponents = RSMutableCopy(alloc, tmp);
    RSRelease(tmp);
    
    RSStringRef str = (RSStringRef)RSArrayObjectAtIndex(urlComponents, 0);
    if (RSStringGetLength(str) == 2 && RSStringGetCharacterAtIndex(str, 1) == ':') {
        RSArrayInsertObjectAtIndex(urlComponents, 0, RSSTR("")); // So we get a leading '/' below
        i = 2; // Skip over the drive letter and the empty string we just inserted
    }
    RSIndex c;
    for (c = RSArrayGetCount(urlComponents); i < c; i ++) {
        RSStringRef fileComp = (RSStringRef)RSArrayObjectAtIndex(urlComponents,i);
        RSStringRef urlComp = _replacePathIllegalCharacters(fileComp, alloc, NO);
        if (!urlComp) {
            // Couldn't decode fileComp
            RSRelease(urlComponents);
            return nil;
        }
        if (urlComp != fileComp) {
            RSArraySetObjectAtIndex(urlComponents, i, urlComp);
        }
        RSRelease(urlComp);
    }
    
    if (isDir) {
        if (RSStringGetLength((RSStringRef)RSArrayObjectAtIndex(urlComponents, RSArrayGetCount(urlComponents) - 1)) != 0)
            RSArrayAddObject(urlComponents, RSSTR(""));
    }
    return urlComponents;
}

static RSStringRef WindowsPathToURLPath(RSStringRef path, RSAllocatorRef alloc, BOOL isDir) {
    RSArrayRef urlComponents;
    RSStringRef str;
    
    if (RSStringGetLength(path) == 0) return RSStringCreateWithCString(alloc, "", RSStringEncodingASCII);
    urlComponents = WindowsPathToURLComponents(path, alloc, isDir);
    if (!urlComponents) return RSStringCreateWithCString(alloc, "", RSStringEncodingASCII);
    
    // WindowsPathToURLComponents already added percent escapes for us; no need to add them again here.
    str = RSStringCreateByCombiningStrings(alloc, urlComponents, RSSTR("/"));
    RSRelease(urlComponents);
    return str;
}

static RSStringRef POSIXPathToURLPath(RSStringRef path, RSAllocatorRef alloc, BOOL isDirectory) {
    RSStringRef pathString = _replacePathIllegalCharacters(path, alloc, YES);
    if (isDirectory && RSStringGetCharacterAtIndex(path, RSStringGetLength(path)-1) != '/') {
        RSStringRef tmp = RSStringCreateWithFormat(alloc, RSSTR("%R/"), pathString);
        RSRelease(pathString);
        pathString = tmp;
    }
    return pathString;
}

static RSStringRef URLPathToPOSIXPath(RSStringRef path, RSAllocatorRef allocator, RSStringEncoding encoding) {
    // This is the easiest case; just remove the percent escape codes and we're done
    RSStringRef result = RSURLCreateStringByReplacingPercentEscapesUsingEncoding(allocator, path, RSSTR(""), encoding);
    if (result) {
        RSIndex length = RSStringGetLength(result);
        if (length > 1 && RSStringGetCharacterAtIndex(result, length-1) == '/') {
            RSStringRef tmp = RSStringCreateWithSubstring(allocator, result, RSMakeRange(0, length-1));
            RSRelease(result);
            result = tmp;
        }
    }
    return result;
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_LINUX
static BOOL CanonicalFileURLStringToFileSystemRepresentation(RSStringRef str, RSAllocatorRef alloc, RSBitU8 *inBuffer, RSIndex inBufferLen)
{
    BOOL result;
    if ( inBuffer && inBufferLen ) {
        STACK_BUFFER_DECL(RSBitU8, stackEscapedBuf, PATH_MAX * 3);    // worst case size is every unicode code point could be a 3-byte UTF8 sequence
        RSBitU8 *escapedBuf;
        RSIndex strLength = RSStringGetLength(str) - (sizeof(fileURLPrefixWithAuthority) - 1);
        if ( strLength != 0 ) {
            RSIndex maxBufLength = strLength * 3;
            RSIndex usedBufLen;
            RSIndex charsConverted;
            if ( strLength <= PATH_MAX ) {
                escapedBuf = &stackEscapedBuf[0];
            }
            else {
                // worst case size is every unicode code point could be a 3-byte UTF8 sequence
                escapedBuf = (RSBitU8 *)malloc(maxBufLength);
            }
            if ( escapedBuf != nil ) {
                charsConverted = RSStringGetBytes(str, RSMakeRange(sizeof(fileURLPrefixWithAuthority) - 1, strLength), RSStringEncodingUTF8, 0, NO, escapedBuf, maxBufLength, &usedBufLen);
                if ( charsConverted ) {
                    static const RSBitU8 hexvalues[] = {
                        /* 00 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 08 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 10 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 18 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 20 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 28 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 30 */  0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7,
                        /* 38 */  0x8, 0x9, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 40 */  0x0, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0,
                        /* 48 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 50 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 58 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 60 */  0x0, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0x0,
                        /* 68 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 70 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 78 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        
                        /* 80 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 88 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 90 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* 98 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* A0 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* A8 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* B0 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* B8 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* C0 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* C8 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* D0 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* D8 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* E0 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* E8 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* F0 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                        /* F8 */  0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
                    };
                    RSBitU8 *bufStartPtr;
                    RSBitU8 *bufEndPtr;
                    RSBitU8 *bufPtr;
                    const RSBitU8 *bytePtr = escapedBuf;
                    RSIndex idx;
                    BOOL trailingSlash = NO;
                    
                    bufPtr = bufStartPtr = inBuffer;
                    bufEndPtr = inBuffer + inBufferLen;
                    result = YES;
                    
                    for ( idx = 0; (idx < usedBufLen) && result; ++idx ) {
                        if ( bufPtr == bufEndPtr ) {
                            // ooops, ran out of inBuffer
                            *bufStartPtr = '\0';
                            result = NO;
                        }
                        else {
                            switch ( *bytePtr ) {
                                case '%':
                                    idx += 2;
                                    if ( idx < usedBufLen ) {
                                        // skip over %
                                        bytePtr++;
                                        // convert hex digits
                                        *bufPtr = hexvalues[*bytePtr++] << 4;
                                        *bufPtr += hexvalues[*bytePtr++];
                                        trailingSlash = (*bufPtr == '/');
                                    }
                                    break;
                                default:
                                    // copy everything else
                                    *bufPtr = *bytePtr++;
                                    trailingSlash = (*bufPtr == '/');
                                    break;
                            }
                            ++bufPtr;
                        }
                    }
                    if ( result ) {
                        // remove trailing slash (if any)
                        if ( (bufPtr > (bufStartPtr + 1)) && trailingSlash ) {
                            --bufPtr;
                        }
                        if ( bufPtr < bufEndPtr ) {
                            *bufPtr = '\0';
                        }
                    }
                }
                else {
                    // RSStringGetBytes failed
                    result = NO;
                }
                
                // free the buffer if we malloc'd it
                if ( escapedBuf != &stackEscapedBuf[0] ) {
                    free(escapedBuf);
                }
            }
            else {
                // could not allocate escapedBuf
                result = NO;
            }
        }
        else {
            // str was zero characters
            *inBuffer = '\0';
            result = YES;
        }
    }
    else {
        // no inBuffer or inBufferLen is zero
        result = NO;
    }
    
    return ( result );
}
#endif

#if DEPLOYMENT_TARGET_WINDOWS
// From RSPlatform.c
extern RSStringRef RSCreateWindowsDrivePathFromVolumeName(RSStringRef volNameStr);
#endif

static RSStringRef URLPathToWindowsPath(RSStringRef path, RSAllocatorRef allocator, RSStringEncoding encoding) {
    // Check for a drive letter, then flip all the slashes
    RSStringRef result;
    RSArrayRef tmp = RSStringCreateArrayBySeparatingStrings(allocator, path, RSSTR("/"));
    SInt32 count = (SInt32)RSArrayGetCount(tmp);
    RSMutableArrayRef components = RSMutableCopy(allocator, tmp);
    RSStringRef newPath;
    
    
    
    RSRelease(tmp);
    if (RSStringGetLength((RSStringRef)RSArrayObjectAtIndex(components,count-1)) == 0) {
        RSArrayRemoveObjectAtIndex(components, count-1);
        count --;
    }
    
    if (count > 1 && RSStringGetLength((RSStringRef)RSArrayObjectAtIndex(components, 0)) == 0) {
        // Absolute path; we need to check for a drive letter in the second component, and if so, remove the first component
        RSStringRef firstComponent = RSURLCreateStringByReplacingPercentEscapesUsingEncoding(allocator, (RSStringRef)RSArrayObjectAtIndex(components, 1), RSSTR(""), encoding);
        UniChar ch;
        
        {
            if (firstComponent) {
                if (RSStringGetLength(firstComponent) == 2 && ((ch = RSStringGetCharacterAtIndex(firstComponent, 1)) == '|' || ch == ':')) {
                    // Drive letter
                    RSArrayRemoveObjectAtIndex(components, 0);
                    if (ch == '|') {
                        RSStringRef driveStr = RSStringCreateWithFormat(allocator, nil, RSSTR("%c:"), RSStringGetCharacterAtIndex(firstComponent, 0));
                        RSArraySetObjectAtIndex(components, 0, driveStr);
                        RSRelease(driveStr);
                    }
                }
#if DEPLOYMENT_TARGET_WINDOWS
                else {
                    // From <rdar://problem/5623405> [DEFECT] RSURL returns a Windows path that contains volume name instead of a drive letter
                    // we need to replace the volume name (it is not valid on Windows) with the drive mounting point path
                    // remove the first component and set the component with the drive letter to be the first component
                    RSStringRef driveRootPath = RSCreateWindowsDrivePathFromVolumeName(firstComponent);
                    
                    if (driveRootPath) {
                        // remove trailing slash
                        if (RSStringHasSuffix(driveRootPath, RSSTR("\\"))) {
                            RSStringRef newDriveRootPath = RSStringCreateWithSubstring(RSAllocatorSystemDefault, driveRootPath, RSMakeRange(0, RSStringGetByteLength(driveRootPath) - 1));
                            RSRelease(driveRootPath);
                            driveRootPath = newDriveRootPath;
                        }
                        
                        // replace the first component of the path with the drive path
                        RSArrayRemoveValueAtIndex(components, 0);
                        RSArraySetValueAtIndex(components, 0, driveRootPath);
                        
                        RSRelease(driveRootPath);
                    }
                }
#endif
            }
        }
        if ( firstComponent ) {
            RSRelease(firstComponent);
        }
    }
    newPath = RSStringCreateByCombiningStrings(allocator, components, RSSTR("\\"));
    RSRelease(components);
    result = RSURLCreateStringByReplacingPercentEscapesUsingEncoding(allocator, newPath, RSSTR(""), encoding);
    RSRelease(newPath);
    return result;
}



// converts url from a file system path representation to a standard representation
static void _convertToURLRepresentation(struct __RSURL *url, RSBitU32 fsType) {
    RSStringRef path = nil;
    BOOL isDir = ((url->_flags & IS_DIRECTORY) != 0);
    BOOL isFileReferencePath = NO;
    RSAllocatorRef alloc = RSGetAllocator(url);
    
    switch (fsType) {
        case RSURLPOSIXPathStyle:
            isFileReferencePath = _pathHasFileIDPrefix(url->_string);
            if (url->_flags & POSIX_AND_URL_PATHS_MATCH) {
                path = (RSStringRef)RSRetain(url->_string);
            } else {
                path = POSIXPathToURLPath(url->_string, alloc, isDir);
            }
            break;
        case RSURLWindowsPathStyle:
            path = WindowsPathToURLPath(url->_string, alloc, isDir);
            break;
    }
    RSAssert2(path != nil, __RSLogAssertion, "%s(): Encountered malformed file system URL %R", __PRETTY_FUNCTION__, url);
    if ( path )
    {
        if (!url->_base) {
            RSMutableStringRef str = RSStringCreateMutable(alloc, 0);
            RSStringAppendString(str, isFileReferencePath ? RSSTR(FILE_PREFIX) : RSSTR(FILE_PREFIX_WITH_AUTHORITY));
            RSStringAppendString(str, path);
            url->_flags = (url->_flags & (IS_DIRECTORY)) | IS_DECOMPOSABLE | IS_ABSOLUTE | HAS_SCHEME | HAS_PATH | ORIGINAL_AND_URL_STRINGS_MATCH | ( isFileReferencePath ? PATH_HAS_FILE_ID : HAS_HOST );
            _setSchemeTypeInFlags(&url->_flags, kHasFileScheme);
            RSRelease(url->_string);
            url->_string = RSCopy(alloc, str);
            RSRelease(str);
            if (isFileReferencePath) {
                url->_ranges = (RSRange *)RSAllocatorAllocate(alloc, sizeof(RSRange) * 2);
                url->_ranges[0] = RSMakeRange(0, 4); // scheme "file"
                url->_ranges[1] = RSMakeRange(7, RSStringGetLength(path)); // path
            }
            else {
                url->_ranges = (RSRange *)RSAllocatorAllocate(alloc, sizeof(RSRange) * 3);
                url->_ranges[0] = RSMakeRange(0, 4); // scheme "file"
                url->_ranges[1] = RSMakeRange(7, 9); // host "localhost"
                url->_ranges[2] = RSMakeRange(16, RSStringGetLength(path)); // path
            }
            RSRelease(path);
        } else {
            RSRelease(url->_string);
            url->_flags = (url->_flags & (IS_DIRECTORY)) | IS_DECOMPOSABLE | HAS_PATH | ORIGINAL_AND_URL_STRINGS_MATCH;
            url->_string = RSCopy(alloc, path);
            RSRelease(path);
            url->_ranges = (RSRange *)RSAllocatorAllocate(alloc, sizeof(RSRange));
            *(url->_ranges) = RSMakeRange(0, RSStringGetLength(url->_string));
        }
    }
}

// Caller must release the returned string
static RSStringRef _resolveFileSystemPaths(RSStringRef relativePath, RSStringRef basePath, BOOL baseIsDir, RSURLPathStyle fsType, RSAllocatorRef alloc) {
    RSIndex baseLen = RSStringGetLength(basePath);
    RSIndex relLen = RSStringGetLength(relativePath);
    UniChar pathDelimiter = '/';
    UniChar *buf = (UniChar *)RSAllocatorAllocate(alloc, sizeof(UniChar)*(relLen + baseLen + 2));
    RSStringGetCharacters(basePath, RSMakeRange(0, baseLen), buf);
    if (baseIsDir) {
        if (buf[baseLen-1] != pathDelimiter) {
            buf[baseLen] = pathDelimiter;
            baseLen ++;
        }
    } else {
        UniChar *ptr = buf + baseLen - 1;
        while (ptr > buf && *ptr != pathDelimiter) {
            ptr --;
        }
        baseLen = ptr - buf + 1;
    }
    if (fsType == RSURLHFSPathStyle) {
        // HFS relative paths will begin with a colon, so we must remove the trailing colon from the base path first.
        baseLen --;
    }
    RSStringGetCharacters(relativePath, RSMakeRange(0, relLen), buf + baseLen);
    *(buf + baseLen + relLen) = '\0';
    return _resolvedPath(buf, buf + baseLen + relLen, pathDelimiter, NO, YES, alloc);
}

RSExport RSURLRef _RSURLCreateCurrentDirectoryURL(RSAllocatorRef allocator) {
    RSURLRef url = nil;
    uint8_t buf[RSMaxPathSize + 1];
    if (_RSGetCurrentDirectory((char *)buf, RSMaxPathLength)) {
        url = RSURLCreateFromFileSystemRepresentation(allocator, buf, strlen((char *)buf), YES);
    }
    return url;
}

RSExport RSURLRef RSURLCreateWithFileSystemPath(RSAllocatorRef allocator, RSStringRef filePath, RSURLPathStyle fsType, BOOL isDirectory) {
    BOOL isAbsolute = YES;
    RSIndex len;
    RSURLRef baseURL, result;
    
    RSAssert2(fsType == RSURLPOSIXPathStyle || fsType == RSURLHFSPathStyle || fsType == RSURLWindowsPathStyle, __RSLogAssertion, "%s(): encountered unknown path style %d", __PRETTY_FUNCTION__, fsType);
    
    RSAssert1(filePath != nil, __RSLogAssertion, "%s(): nil filePath argument not permitted", __PRETTY_FUNCTION__);
    
	len = RSStringGetLength(filePath);
	
    switch(fsType) {
        case RSURLPOSIXPathStyle:
            isAbsolute = (len > 0 && RSStringGetCharacterAtIndex(filePath, 0) == '/');
            break;
        case RSURLWindowsPathStyle:
            isAbsolute = (len >= 3 && RSStringGetCharacterAtIndex(filePath, 1) == ':' && RSStringGetCharacterAtIndex(filePath, 2) == '\\');
            /* Absolute path under Win32 can begin with "\\"
             * (Sergey Zubarev)
             */
            if (!isAbsolute) isAbsolute = (len > 2 && RSStringGetCharacterAtIndex(filePath, 0) == '\\' && RSStringGetCharacterAtIndex(filePath, 1) == '\\');
            break;
        case RSURLHFSPathStyle:
            isAbsolute = (len > 0 && RSStringGetCharacterAtIndex(filePath, 0) != ':');
            break;
    }
    if (isAbsolute) {
        baseURL = nil;
    } else {
        baseURL = _RSURLCreateCurrentDirectoryURL(allocator);
    }
    result = RSURLCreateWithFileSystemPathRelativeToBase(allocator, filePath, fsType, isDirectory, baseURL);
    if (baseURL) RSRelease(baseURL);
    return result;
}

RSExport RSURLRef RSURLCreateWithFileSystemPathRelativeToBase(RSAllocatorRef allocator, RSStringRef filePath, RSURLPathStyle fsType, BOOL isDirectory, RSURLRef baseURL) {
    RSURLRef url;
    BOOL isAbsolute = YES, releaseFilePath = NO, releaseBaseURL = NO;
    UniChar pathDelim = '\0';
    RSIndex len;
    
    RSAssert1(filePath != nil, __RSLogAssertion, "%s(): nil path string not permitted", __PRETTY_FUNCTION__);
    RSAssert2(fsType == RSURLPOSIXPathStyle || fsType == RSURLHFSPathStyle || fsType == RSURLWindowsPathStyle, __RSLogAssertion, "%s(): encountered unknown path style %d", __PRETTY_FUNCTION__, fsType);
    
	len = RSStringGetLength(filePath);
    
    switch(fsType) {
        case RSURLPOSIXPathStyle:
            isAbsolute = (len > 0 && RSStringGetCharacterAtIndex(filePath, 0) == '/');
			
            pathDelim = '/';
            break;
        case RSURLWindowsPathStyle:
            isAbsolute = (len >= 3 && RSStringGetCharacterAtIndex(filePath, 1) == ':' && RSStringGetCharacterAtIndex(filePath, 2) == '\\');
            /* Absolute path under Win32 can begin with "\\"
             * (Sergey Zubarev)
             */
            if (!isAbsolute)
                isAbsolute = (len > 2 && RSStringGetCharacterAtIndex(filePath, 0) == '\\' && RSStringGetCharacterAtIndex(filePath, 1) == '\\');
            pathDelim = '\\';
            break;
        case RSURLHFSPathStyle:
		{	RSRange	fullStrRange = RSMakeRange( 0, RSStringGetLength( filePath ) );
            
            isAbsolute = (len > 0 && RSStringGetCharacterAtIndex(filePath, 0) != ':');
            pathDelim = ':';
			
			if ( filePath && RSStringFindWithOptions( filePath, RSSTR("::"), fullStrRange, 0, nil ) ) {
				UniChar *	chars = (UniChar *) malloc( fullStrRange.length * sizeof( UniChar ) );
				RSIndex index, writeIndex, firstColonOffset = -1;
                
				RSStringGetCharacters( filePath, fullStrRange, chars );
                
				for ( index = 0, writeIndex = 0 ; index < fullStrRange.length; index ++ ) {
					if ( chars[ index ] == ':' ) {
						if ( index + 1 < fullStrRange.length && chars[ index + 1 ] == ':' ) {
                            
							// Don't let :: go off the 'top' of the path -- which means that there always has to be at
							//	least one ':' to the left of the current write position to go back to.
							if ( writeIndex > 0 && firstColonOffset >= 0 )
							{
								writeIndex --;
								while ( writeIndex > 0 && writeIndex >= firstColonOffset && chars[ writeIndex ] != ':' )
									writeIndex --;
							}
							index ++;	// skip over the first ':', so we replace the ':' which is there with a new one
						}
						
						if ( firstColonOffset == -1 )
							firstColonOffset = writeIndex;
					}
					
					chars[ writeIndex ++ ] = chars[ index ];
				}
                
				if ( releaseFilePath && filePath )
					RSRelease( filePath );
                filePath = RSStringCreateWithCharacters(allocator, chars, writeIndex);
				// reset len because a canonical HFS path can be a different length than the original RSString
				len = RSStringGetLength(filePath);
				releaseFilePath = YES;
				
				free( chars );
			}
			
            break;
		}
    }
    if (isAbsolute) {
        baseURL = nil;
    }
    else if ( baseURL == nil ) {
        baseURL = _RSURLCreateCurrentDirectoryURL(allocator);
        releaseBaseURL = YES;
    }
    
	
    if (isDirectory && len > 0 && RSStringGetCharacterAtIndex(filePath, len-1) != pathDelim) {
        RSMutableStringRef tempRef = RSStringCreateMutable(allocator, 0);
        RSStringAppendString(tempRef, filePath);
        RSStringAppendCharacters(tempRef, &pathDelim, 1);
    	if ( releaseFilePath && filePath ) RSRelease( filePath );
    	filePath = tempRef;
        releaseFilePath = YES;
    } else if (!isDirectory && len > 0 && RSStringGetCharacterAtIndex(filePath, len-1) == pathDelim) {
        if (len == 1 || RSStringGetCharacterAtIndex(filePath, len-2) == pathDelim) {
            // Override isDirectory
            isDirectory = YES;
        } else {
            RSStringRef tempRef = RSStringCreateWithSubstring(allocator, filePath, RSMakeRange(0, len-1));
			if ( releaseFilePath && filePath )
				RSRelease( filePath );
			filePath = tempRef;
            releaseFilePath = YES;
        }
    }
    if (!filePath || RSStringGetLength(filePath) == 0) {
        if (releaseFilePath && filePath) RSRelease(filePath);
        if (releaseBaseURL && baseURL) RSRelease(baseURL);
        return nil;
    }
    url = _RSURLAlloc(allocator);
    _RSURLInit((struct __RSURL *)url, filePath, fsType, baseURL);
    if (releaseFilePath) RSRelease(filePath);
    if (releaseBaseURL && baseURL) RSRelease(baseURL);
    if (isDirectory) ((struct __RSURL *)url)->_flags |= IS_DIRECTORY;
    if (fsType == RSURLPOSIXPathStyle) {
        // Check if relative path is equivalent to URL representation; this will be YES if url->_string contains only characters from the unreserved character set, plus '/' to delimit the path, plus ';', '@', '&', '=', '+', '$', ',' (according to RFC 2396) -- REW, 12/1/2000
        // Per Section 5 of RFC 2396, there's a special problem if a colon apears in the first path segment - in this position, it can be mistaken for the scheme name.  Otherwise, it's o.k., and can be safely identified as part of the path.  In this one case, we need to prepend "./" to make it clear what's going on.... -- REW, 8/24/2001
        RSStringInlineBuffer buf;
        BOOL sawSlash = NO;
        BOOL mustPrependDotSlash = NO;
        RSIndex idx, length = RSStringGetLength(url->_string);
        RSStringInitInlineBuffer(url->_string, &buf, RSMakeRange(0, length));
        for (idx = 0; idx < length; idx ++) {
            UniChar ch = RSStringGetCharacterFromInlineBuffer(&buf, idx);
            if (!isPathLegalCharacter(ch)) break;
            if (!sawSlash) {
                if (ch == '/') {
                    sawSlash = YES;
                } else if (ch == ':') {
                    mustPrependDotSlash = YES;
                }
            }
        }
        if (idx == length) {
            ((struct __RSURL *)url)->_flags |= POSIX_AND_URL_PATHS_MATCH;
        }
        if (mustPrependDotSlash) {
            RSMutableStringRef newString = RSStringCreateMutable(allocator, 0);
            RSStringAppendString(newString, RSSTR("./"));
            RSStringAppendString(newString, url->_string);
            RSRelease(url->_string);
            ((struct __RSURL *)url)->_string = RSStringCreateCopy(allocator, newString);
            RSRelease(newString);
        }
    }
    return url;
}

static BOOL _pathHasFileIDPrefix( RSStringRef path )
{
    // path is not nil, path has prefix "/.file/" and has at least one character following the prefix.
#ifdef __CONSTANT_STRINGS__
    static const
#endif
    RSStringRef fileIDPrefix = RSSTR( "/" FILE_ID_PREFIX "/" );
    return path && RSStringHasPrefix( path, fileIDPrefix ) && RSStringGetLength( path ) > RSStringGetLength( fileIDPrefix );
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
static BOOL _pathHasFileIDOnly( RSStringRef path )
{
    // Is file ID rooted and contains no additonal path segments
    RSRange slashRange;
    return _pathHasFileIDPrefix( path ) && ( !RSStringFindWithOptions( path, RSSTR("/"), RSMakeRange( sizeof(FILE_ID_PREFIX) + 1, RSStringGetLength( path ) - sizeof(FILE_ID_PREFIX) - 1), 0, &slashRange ) || slashRange.location == RSStringGetLength( path ) - 1 );
}
#endif

RSExport RSStringRef RSURLCopyFileSystemPath(RSURLRef anURL, RSURLPathStyle pathStyle) {
    RSAssert2(pathStyle == RSURLPOSIXPathStyle || pathStyle == RSURLHFSPathStyle || pathStyle == RSURLWindowsPathStyle, __RSLogAssertion, "%s(): Encountered unknown path style %d", __PRETTY_FUNCTION__, pathStyle);
    
    RSStringRef result;
    
    result = RSURLCreateStringWithFileSystemPath(RSGetAllocator(anURL), anURL, pathStyle, NO);
    return ( result );
}


// There is no matching ObjC method for this functionality; because this function sits on top of the RSURL primitives, it's o.k. not to check for the need to dispatch an ObjC method instead, but this means care must be taken that this function never call anything that will result in dereferencing anURL without first checking for an ObjC dispatch.  -- REW, 10/29/98
RSExport RSStringRef RSURLCreateStringWithFileSystemPath(RSAllocatorRef allocator, RSURLRef anURL, RSURLPathStyle fsType, BOOL resolveAgainstBase) {
    RSURLRef base = resolveAgainstBase ? RSURLGetBaseURL(anURL) : nil;
    RSStringRef basePath = base ? RSURLCreateStringWithFileSystemPath(allocator, base, fsType, NO) : nil;
    RSStringRef relPath = nil;
    
    if (!RS_IS_OBJC(__RSURLTypeID, anURL)) {
        // We can grope the ivars
        if (fsType == RSURLPOSIXPathStyle) {
            if (anURL->_flags & POSIX_AND_URL_PATHS_MATCH) {
                relPath = _retainedComponentString(anURL, HAS_PATH, YES, YES);
            }
        }
    }
    
    if (relPath == nil) {
        RSStringRef urlPath = RSURLCopyPath(anURL);
        RSStringEncoding enc = anURL->_encoding;
        if (urlPath) {
            switch (fsType) {
                case RSURLPOSIXPathStyle:
                    relPath = URLPathToPOSIXPath(urlPath, allocator, enc);
                    break;
                case RSURLHFSPathStyle:
                    relPath = nil;
                    break;
                case RSURLWindowsPathStyle:
                    relPath = URLPathToWindowsPath(urlPath, allocator, enc);
                    break;
                default:
                    break;
                RSAssert2(YES, __RSLogAssertion, "%s(): Received unknown path type %d", __PRETTY_FUNCTION__, fsType);
            }
            RSRelease(urlPath);
        }
    }
	
    //	For Tiger, leave this behavior in for all path types.  For Leopard, it would be nice to remove this entirely
    //	and do a linked-on-or-later check so we don't break third parties.
    //	See <rdar://problem/4003028> Converting volume name from POSIX to HFS form fails and
    //	<rdar://problem/4018895> RS needs to back out 4003028 for icky details.
    if ( relPath && RSURLHasDirectoryPath(anURL) && RSStringGetLength(relPath) > 1 && RSStringGetCharacterAtIndex(relPath, RSStringGetLength(relPath)-1) == '/') {
        RSStringRef tmp = RSStringCreateWithSubstring(allocator, relPath, RSMakeRange(0, RSStringGetLength(relPath)-1));
        RSRelease(relPath);
        relPath = tmp;
    }
    
    if ( relPath ) {
        if ( basePath ) {
            // we have both basePath and relPath -- resolve them
            RSStringRef result = _resolveFileSystemPaths(relPath, basePath, RSURLHasDirectoryPath(base), fsType, allocator);
            RSRelease(basePath);
            RSRelease(relPath);
            return result;
        }
        else {
            // we only have the relPath -- return it
            return relPath;
        }
    }
    else if ( basePath ) {
        // we only have the basePath --- return it
        return basePath;
    }
    else {
        // we have nothing to return
        return nil;
    }
}

RSExport BOOL RSURLGetFileSystemRepresentation(RSURLRef url, BOOL resolveAgainstBase, uint8_t *buffer, RSIndex bufLen) {
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_LINUX || DEPLOYMENT_TARGET_WINDOWS
    RSAllocatorRef alloc = RSGetAllocator(url);
    RSStringRef path;
    
    if (!url) return NO;
#endif
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_LINUX
    if ( !resolveAgainstBase || (RSURLGetBaseURL(url) == nil) ) {
        if (!RS_IS_OBJC(__RSURLTypeID, url)) {
            // We can grope the ivars
            if ( url->_flags & IS_CANONICAL_FILE_URL ) {
                return CanonicalFileURLStringToFileSystemRepresentation(url->_string, alloc, buffer, bufLen);
            }
        }
    }
    // else fall back to slower way.
    path = RSURLCreateStringWithFileSystemPath(alloc, url, RSURLPOSIXPathStyle, resolveAgainstBase);
    if (path) {
        BOOL convResult = _RSStringGetFileSystemRepresentation(path, buffer, bufLen);
        RSRelease(path);
        return convResult;
    }
#elif DEPLOYMENT_TARGET_WINDOWS
    path = RSURLCreateStringWithFileSystemPath(alloc, url, RSURLWindowsPathStyle, resolveAgainstBase);
    if (path) {
        RSIndex usedLen;
        RSIndex pathLen = RSStringGetByteLength(path);
        RSIndex numConverted = RSStringGetBytes(path, RSMakeRange(0, pathLen), RSStringFileSystemEncoding(), 0, YES, buffer, bufLen-1, &usedLen); // -1 because we need one byte to zero-terminate.
        RSRelease(path);
        if (numConverted == pathLen) {
            buffer[usedLen] = '\0';
            return YES;
        }
    }
#endif
    return NO;
}

#if DEPLOYMENT_TARGET_WINDOWS
RS_EXPORT BOOL _RSURLGetWideFileSystemRepresentation(RSURLRef url, BOOL resolveAgainstBase, wchar_t *buffer, RSIndex bufferLength) {
	RSStringRef path = RSURLCreateStringWithFileSystemPath(RSGetAllocator(url), url, RSURLWindowsPathStyle, resolveAgainstBase);
	RSIndex pathLength, charsConverted, usedBufLen;
	if (!path) return NO;
	pathLength = RSStringGetByteLength(path);
	if (pathLength+1 > bufferLength) {
		RSRelease(path);
		return NO;
	}
	charsConverted = RSStringGetBytes(path, RSMakeRange(0, pathLength), RSStringEncodingUTF16, 0, NO, (RSBitU8 *)buffer, bufferLength*sizeof(wchar_t), &usedBufLen);
    //	RSStringGetCharacters(path, RSMakeRange(0, pathLength), (UTF8Char *)buffer);
	RSRelease(path);
	if (charsConverted != pathLength || usedBufLen%sizeof(wchar_t) != 0) {
		return NO;
	} else {
		buffer[usedBufLen/sizeof(wchar_t)] = 0;
        //		buffer[pathLength] = 0;
		return YES;
	}
}
#endif

static RSStringRef _createPathFromFileSystemRepresentation(RSAllocatorRef allocator, const uint8_t *buffer, RSIndex bufLen, BOOL isDirectory) {
    char pathDelim = PATH_SEP;
    RSStringRef path;
    if ( isDirectory ) {
        // it is a directory: if it doesn't end with pathDelim, append a pathDelim. Limit stack buffer to PATH_MAX+1 in case a large bogus value is passed.
        if ( (bufLen > 0) && (bufLen <= PATH_MAX) && (buffer[bufLen-1] != pathDelim) ) {
            STACK_BUFFER_DECL(uint8_t, tempBuf, bufLen + 1);
            memcpy(tempBuf, buffer, bufLen);
            tempBuf[bufLen] = pathDelim;
            path = RSStringCreateWithBytes(allocator, tempBuf, bufLen + 1, RSStringFileSystemEncoding(), NO);
        }
        else {
            // already had pathDelim at end of buffer or bufLen is really large
            path = RSStringCreateWithBytes(allocator, buffer, bufLen, RSStringFileSystemEncoding(), NO);
        }
    }
    else {
        // it is not a directory: remove any pathDelim characters at end (leaving at least one character)
        while ( (bufLen > 1) && (buffer[bufLen-1] == pathDelim) ) {
            --bufLen;
        }
        path = RSStringCreateWithBytes(allocator, buffer, bufLen, RSStringFileSystemEncoding(), NO);
    }
    return path;
}

/*
 CreatePOSIXFileURLStringFromPOSIXAbsolutePath creates file URL string from the native file system representation.
 
 The rules for path-absolute from rfc3986 are:
 path-absolute = "/" [ segment-nz *( "/" segment ) ]
 segment       = *pchar
 segment-nz    = 1*pchar
 pchar         = unreserved / pct-encoded / sub-delims / ":" / "@"
 pct-encoded   = "%" HEXDIG HEXDIG
 unreserved    = ALPHA / DIGIT / "-" / "." / "_" / "~"
 sub-delims    = "!" / "$" / "&" / "'" / "(" / ")" / "*" / "+" / "," / ";" / "="
 */
static RSStringRef CreatePOSIXFileURLStringFromPOSIXAbsolutePath(RSAllocatorRef alloc, const RSBitU8 *bytes, RSIndex numBytes, BOOL isDirectory, BOOL *addedPercentEncoding)
{
    static const RSBitU8 hexchars[] = "0123456789ABCDEF";
    STACK_BUFFER_DECL(RSBitU8, stackBuf, (PATH_MAX * 3) + sizeof(fileURLPrefixWithAuthority));    // worst case is every byte needs to be percent-escaped
    RSBitU8 *bufStartPtr;
    RSBitU8 *bufBytePtr;
    const RSBitU8 *bytePtr = bytes;
    RSIndex idx;
    RSStringRef result;
    BOOL addedPercent = NO;
    
    // choose a buffer to percent-escape into.
    if ( numBytes <= PATH_MAX ) {
        bufStartPtr = &stackBuf[0];
    }
    else {
        // worst case is every byte needs to be percent-escaped (numBytes * 3)
        bufStartPtr = (RSBitU8 *)malloc((numBytes * 3) + sizeof(fileURLPrefixWithAuthority));
    }
    
    if ( bufStartPtr != nil ) {
        // start with the fileURLPrefixWithAuthority
        strcpy((char *)bufStartPtr, (char *)fileURLPrefixWithAuthority);
        bufBytePtr = bufStartPtr + sizeof(fileURLPrefixWithAuthority) - 1;
        // append the percent-escaped path
        for ( idx = 0; (idx < numBytes) && (*bytePtr != 0); ++idx ) {
            switch ( *bytePtr ) {
                    // these are the visible 7-bit ascii characters that are not legal pchar octets
                case '"':
                case '#':
                case '%':
                case ';':	// we need to percent-escape ';' in file system paths so it won't be mistaken for the start of the obsolete param rule (rfc2396) that RSURL still supports
                case '<':
                case '>':
                case '?':	// we need to percent-escape '?' in file system paths so it won't be mistaken for the start of a query
                case '[':
                case '\\':
                case ']':
                case '^':
                case '`':
                case '{':
                case '|':
                case '}':
                    // percent-escape non-pchar octets spread throughout the visible 7-bit ascii range
                    *bufBytePtr++ = '%';
                    *bufBytePtr++ = hexchars[*bytePtr >> 4];
                    *bufBytePtr++ = hexchars[*bytePtr & 0x0f];
                    addedPercent = YES;
                    break;
                default:
                    if ( (*bytePtr <= ' ') ||	// percent-escape non-pchar octets that are space or less (control characters)
                        (*bytePtr >= 0x7f) ) {	// percent-escape non-pchar octets that del and 8-bit ascii with the high bit set
                        *bufBytePtr++ = '%';
                        *bufBytePtr++ = hexchars[*bytePtr >> 4];
                        *bufBytePtr++ = hexchars[*bytePtr & 0x0f];
                        addedPercent = YES;
                    }
                    else {
                        // copy everything else
                        *bufBytePtr++ = *bytePtr;
                    }
                    break;
            }
            ++bytePtr;
        }
        
        // did we convert numBytes?
        if ( idx == numBytes ) {
            // if it is a directory and it doesn't end with PATH_SEP, append a PATH_SEP.
            if ( isDirectory && bytes[numBytes-1] != PATH_SEP) {
                *bufBytePtr++ = PATH_SEP;
            }
            
            // create the result
            result = RSStringCreateWithBytes(alloc, bufStartPtr, (RSIndex)(bufBytePtr-bufStartPtr), RSStringEncodingUTF8, NO);
        }
        else {
            // no, but it's OK if the remaining bytes are all nul (embedded nul bytes are not allowed)
            const RSBitU8 *nullBytePtr = bytePtr;
            for ( /* start where we left off */; (idx < numBytes) && (*nullBytePtr == '\0'); ++idx, ++nullBytePtr ) {
                // do nothing
            }
            if ( idx == numBytes ) {
                // create the result
                result = RSStringCreateWithBytes(alloc, bufStartPtr, (RSIndex)(bufBytePtr-bufStartPtr), RSStringEncodingUTF8, NO);
            }
            else {
                // the remaining bytes were not all nul
                result = nil;
            }
        }
        // free the buffer if we malloc'd it
        if ( bufStartPtr != &stackBuf[0] ) {
            free(bufStartPtr);
        }
    }
    else {
        result = nil;
    }
    
    if ( addedPercentEncoding ) {
        *addedPercentEncoding = addedPercent;
    }
    
    return ( result );
}

RSExport RSURLRef RSURLCreateFromFileSystemRepresentation(RSAllocatorRef allocator, const uint8_t *buffer, RSIndex bufLen, BOOL isDirectory) {
    RSURLRef newURL = nil;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_EMBEDDED_MINI || DEPLOYMENT_TARGET_LINUX
    if ( bufLen && (*buffer == '/') ) {
        BOOL addedPercentEncoding;
        
        RSStringRef urlStr = CreatePOSIXFileURLStringFromPOSIXAbsolutePath(allocator, buffer, bufLen, isDirectory, &addedPercentEncoding);
        if ( urlStr ) {
            newURL = _RSURLAlloc(allocator);
            if ( newURL ) {
                ((struct __RSURL *)newURL)->_flags |= USES_EIGHTBITSTRINGENCODING;
                // we know urlStr has a scheme and is absolute
                _RSURLInit((struct __RSURL *)newURL, urlStr, FULL_URL_REPRESENTATION, nil);
                ((struct __RSURL *)newURL)->_flags |= (IS_ABSOLUTE | IS_CANONICAL_FILE_URL);
                if ( !addedPercentEncoding ) {
                    ((struct __RSURL *)newURL)->_flags |= POSIX_AND_URL_PATHS_MATCH;
                }
            }
            RSRelease(urlStr);
        }
        else {
            newURL = nil;
        }
    }
    else {
        RSStringRef path = _createPathFromFileSystemRepresentation(allocator, buffer, bufLen, isDirectory);
        if ( path ) {
            newURL = RSURLCreateWithFileSystemPath(allocator, path, RSURLPOSIXPathStyle, isDirectory);
            RSRelease(path);
        }
        else {
            newURL = nil;
        }
    }
#elif DEPLOYMENT_TARGET_WINDOWS
    RSStringRef path = _createPathFromFileSystemRepresentation(allocator, buffer, bufLen, isDirectory);
    if ( path ) {
        newURL = RSURLCreateWithFileSystemPath(allocator, path, RSURLWindowsPathStyle, isDirectory);
        RSRelease(path);
    }
    else {
        newURL = nil;
    }
#endif
    return newURL;
}

RSExport RSURLRef RSURLCreateFromFileSystemRepresentationRelativeToBase(RSAllocatorRef allocator, const uint8_t *buffer, RSIndex bufLen, BOOL isDirectory, RSURLRef baseURL) {
    RSStringRef path = _createPathFromFileSystemRepresentation(allocator, buffer, bufLen, isDirectory);
    RSURLRef newURL = nil;
    if (!path) return nil;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED || DEPLOYMENT_TARGET_LINUX
    newURL = RSURLCreateWithFileSystemPathRelativeToBase(allocator, path, RSURLPOSIXPathStyle, isDirectory, baseURL);
#elif DEPLOYMENT_TARGET_WINDOWS
    newURL = RSURLCreateWithFileSystemPathRelativeToBase(allocator, path, RSURLWindowsPathStyle, isDirectory, baseURL);
#endif
    RSRelease(path);
    return newURL;
}


/******************************/
/* Support for path utilities */
/******************************/

// Assumes url is a RSURL (not an Obj-C NSURL)
static RSRange _rangeOfLastPathComponent(RSURLRef url) {
    RSRange pathRg, componentRg;
    
    pathRg = _rangeForComponent(url->_flags, url->_ranges, HAS_PATH);
    
    if (pathRg.location == RSNotFound || pathRg.length == 0) {
        // No path
        return pathRg;
    }
    if (RSStringGetCharacterAtIndex(url->_string, pathRg.location + pathRg.length - 1) == '/') {
        pathRg.length --;
        if (pathRg.length == 0) {
            pathRg.length ++;
            return pathRg;
        }
    }
    if (RSStringFindWithOptions(url->_string, RSSTR("/"), pathRg, RSCompareBackwards, &componentRg)) {
        componentRg.location ++;
        componentRg.length = pathRg.location + pathRg.length - componentRg.location;
    } else {
        componentRg = pathRg;
    }
    return componentRg;
}

RSExport RSStringRef RSURLCopyLastPathComponent(RSURLRef url) {
    RSStringRef result;
    
    if (RS_IS_OBJC(__RSURLTypeID, url)) {
        RSStringRef path = RSURLCreateStringWithFileSystemPath(RSGetAllocator(url), url, RSURLPOSIXPathStyle, NO);
        RSIndex length;
        RSRange rg, compRg;
        if (!path) return nil;
        rg = RSMakeRange(0, RSStringGetLength(path));
        if ( rg.length == 0 ) return path;
        length = rg.length; // Remember this for comparison later
        if (RSStringGetCharacterAtIndex(path, rg.length - 1) == '/' ) {
            rg.length --;
        }
        if ( rg.length == 0 )
        {
            //	If we have reduced the string to empty, then it's "/", and that's what we return as
            //	the last path component.
            return path;
        }
        else if (RSStringFindWithOptions(path, RSSTR("/"), rg, RSCompareBackwards, &compRg)) {
            rg.length = rg.location + rg.length - (compRg.location+1);
            rg.location = compRg.location + 1;
        }
        if (rg.location == 0 && rg.length == length) {
            result = path;
        } else {
            result = RSStringCreateWithSubstring(RSGetAllocator(url), path, rg);
            RSRelease(path);
        }
    } else {
        BOOL filePathURLCreated = NO;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
        if ( _RSURLIsFileReferenceURL(url) ) {
            // use a file path URL or fail
            RSURLRef filePathURL = RSURLCreateFilePathURL(RSGetAllocator(url), url, nil);
            if ( filePathURL ) {
                filePathURLCreated = YES;
                url = filePathURL;
            }
            else {
                return nil;
            }
        }
#endif
        
        RSRange rg = _rangeOfLastPathComponent(url);
        if (rg.location == RSNotFound || rg.length == 0) {
            // No path
            if ( filePathURLCreated ) {
                RSRelease(url);
            }
            return (RSStringRef)RSRetain(RSSTR(""));
        }
        if (rg.length == 1 && RSStringGetCharacterAtIndex(url->_string, rg.location) == '/') {
            if ( filePathURLCreated ) {
                RSRelease(url);
            }
            return (RSStringRef)RSRetain(RSSTR("/"));
        }
        result = RSStringCreateWithSubstring(RSGetAllocator(url), url->_string, rg);
        if (!(url->_flags & POSIX_AND_URL_PATHS_MATCH)) {
            RSStringRef tmp;
            if (url->_encoding == RSStringEncodingUTF8) {
                tmp = RSURLCreateStringByReplacingPercentEscapes(RSGetAllocator(url), result, RSSTR(""));
            } else {
                tmp = RSURLCreateStringByReplacingPercentEscapesUsingEncoding(RSGetAllocator(url), result, RSSTR(""), url->_encoding);
            }
            RSRelease(result);
            result = tmp;
        }
        if ( filePathURLCreated ) {
            RSRelease(url);
        }
    }
    return result;
}

RSExport RSStringRef RSURLCopyPathExtension(RSURLRef url) {
    RSStringRef lastPathComp = RSURLCopyLastPathComponent(url);
    RSStringRef ext = nil;
    
    if (lastPathComp)
    {
        RSRange rg = {RSNotFound};
        BOOL result = RSStringFindWithOptions(lastPathComp, RSSTR("."), RSStringGetRange(lastPathComp), RSCompareBackwards, &rg);
        if (result && rg.location != RSNotFound) {
            rg.location ++;
            rg.length = RSStringGetLength(lastPathComp) - rg.location;
            if (rg.length > 0) {
                ext = RSStringCreateWithSubstring(RSGetAllocator(url), lastPathComp, rg);
            } else {
                ext = (RSStringRef)RSRetain(RSSTR(""));
            }
        }
        RSRelease(lastPathComp);
    }
    return ext;
}

RSExport RSURLRef RSURLCreateCopyAppendingPathComponent(RSAllocatorRef allocator, RSURLRef url, RSStringRef pathComponent, BOOL isDirectory) {
    RSURLRef result;
    url = _RSURLFromNSURL(url);
    __RSGenericValidInstance(url, __RSURLTypeID);
    RSAssert1(pathComponent != nil, __RSLogAssertion, "%s(): Cannot be called with a nil component to append", __PRETTY_FUNCTION__);
    
    BOOL filePathURLCreated = NO;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    if ( _RSURLIsFileReferenceURL(url) ) {
        // use a file path URL if possible (only because this is appending a path component)
        RSURLRef filePathURL = RSURLCreateFilePathURL(allocator, url, nil);
        if ( filePathURL ) {
            filePathURLCreated = YES;
            url = filePathURL;
        }
    }
#endif
    
    RSMutableStringRef newString;
    RSStringRef newComp;
    RSRange pathRg;
    if (!(url->_flags & HAS_PATH)) {
        result = nil;
    }
    else {
        newString = RSStringCreateMutableCopy(allocator, 0, url->_string);
        newComp = RSURLCreateStringByAddingPercentEscapes(allocator, pathComponent, nil, RSSTR(";?"), url->_encoding);
        pathRg = _rangeForComponent(url->_flags, url->_ranges, HAS_PATH);
        if ( (!pathRg.length || RSStringGetCharacterAtIndex(url->_string, pathRg.location + pathRg.length - 1) != '/') && (RSStringGetCharacterAtIndex(newComp, 0) != '/') ) {
            RSStringInsert(newString, pathRg.location + pathRg.length, RSSTR("/"));
            pathRg.length ++;
        }
        RSStringInsert(newString, pathRg.location + pathRg.length, newComp);
        if (isDirectory) {
            RSStringInsert(newString, pathRg.location + pathRg.length + RSStringGetLength(newComp), RSSTR("/"));
        }
        RSRelease(newComp);
        result = _RSURLCreateWithArbitraryString(allocator, newString, url->_base);
        RSRelease(newString);
    }
    if ( filePathURLCreated ) {
        RSRelease(url);
    }
    return result;
}

RSExport RSURLRef RSURLCreateCopyDeletingLastPathComponent(RSAllocatorRef allocator, RSURLRef url) {
    RSURLRef result;
    RSMutableStringRef newString;
    RSRange lastCompRg, pathRg;
    BOOL appendDotDot = NO;
    
    url = _RSURLFromNSURL(url);
    RSAssert1(url != nil, __RSLogAssertion, "%s(): nil argument not allowed", __PRETTY_FUNCTION__);
    __RSGenericValidInstance(url, __RSURLTypeID);
    
    BOOL filePathURLCreated = NO;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    if ( _RSURLIsFileReferenceURL(url) ) {
        // use a file path URL or fail
        RSURLRef filePathURL = RSURLCreateFilePathURL(allocator, url, nil);
        if ( filePathURL ) {
            filePathURLCreated = YES;
            url = filePathURL;
        }
        else {
            return nil;
        }
    }
#endif
    
    if (!(url->_flags & HAS_PATH)) {
        if ( filePathURLCreated ) {
            RSRelease(url);
        }
        return nil;
    }
    pathRg = _rangeForComponent(url->_flags, url->_ranges, HAS_PATH);
    lastCompRg = _rangeOfLastPathComponent(url);
    if (lastCompRg.length == 0) {
        appendDotDot = YES;
    } else if (lastCompRg.length == 1) {
        UniChar ch = RSStringGetCharacterAtIndex(url->_string, lastCompRg.location);
        if (ch == '.' || ch == '/') {
            appendDotDot = YES;
        }
    } else if (lastCompRg.length == 2 && RSStringGetCharacterAtIndex(url->_string, lastCompRg.location) == '.' && RSStringGetCharacterAtIndex(url->_string, lastCompRg.location+1) == '.') {
        appendDotDot = YES;
    }
    
    newString = RSStringCreateMutableCopy(allocator, 0, url->_string);
    if (appendDotDot) {
        RSIndex delta = 0;
        if (pathRg.length > 0 && RSStringGetCharacterAtIndex(url->_string, pathRg.location + pathRg.length - 1) != '/') {
            RSStringInsert(newString, pathRg.location + pathRg.length, RSSTR("/"));
            delta ++;
        }
        RSStringInsert(newString, pathRg.location + pathRg.length + delta, RSSTR(".."));
        delta += 2;
        RSStringInsert(newString, pathRg.location + pathRg.length + delta, RSSTR("/"));
        delta ++;
        // We know we have "/../" at the end of the path; we wish to know if that's immediately preceded by "/." (but that "/." doesn't start the string), in which case we want to delete the "/.".
        if (pathRg.length + delta > 4 && RSStringGetCharacterAtIndex(newString, pathRg.location + pathRg.length + delta - 5) == '.') {
            if (pathRg.length+delta > 7 && RSStringGetCharacterAtIndex(newString, pathRg.location + pathRg.length + delta - 6) == '/') {
                RSStringDelete(newString, RSMakeRange(pathRg.location + pathRg.length + delta - 6, 2));
            } else if (pathRg.length+delta == 5) {
                RSStringDelete(newString, RSMakeRange(pathRg.location + pathRg.length + delta - 5, 2));
            }
        }
    } else if (lastCompRg.location == pathRg.location) {
        RSStringReplace(newString, pathRg, RSSTR("."));
        RSStringInsert(newString, 1, RSSTR("/"));
    } else {
        RSStringDelete(newString, RSMakeRange(lastCompRg.location, pathRg.location + pathRg.length - lastCompRg.location));
    }
    result = _RSURLCreateWithArbitraryString(allocator, newString, url->_base);
    RSRelease(newString);
    if ( filePathURLCreated ) {
        RSRelease(url);
    }
    return result;
}

RSExport RSURLRef RSURLCreateCopyAppendingPathExtension(RSAllocatorRef allocator, RSURLRef url, RSStringRef extension) {
    RSMutableStringRef newString;
    RSURLRef result;
    RSRange rg;
    
    RSAssert1(url != nil && extension != nil, __RSLogAssertion, "%s(): nil argument not allowed", __PRETTY_FUNCTION__);
    url = _RSURLFromNSURL(url);
    __RSGenericValidInstance(url, __RSURLTypeID);
    __RSGenericValidInstance(extension, RSStringGetTypeID());
    
    BOOL filePathURLCreated = NO;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    if ( _RSURLIsFileReferenceURL(url) ) {
        // use a file path URL or fail
        RSURLRef filePathURL = RSURLCreateFilePathURL(allocator, url, nil);
        if ( filePathURL ) {
            filePathURLCreated = YES;
            url = filePathURL;
        }
        else {
            return nil;
        }
    }
#endif
    
    rg = _rangeOfLastPathComponent(url);
    if (rg.location < 0) {
        if ( filePathURLCreated ) {
            RSRelease(url);
        }
        return nil; // No path
    }
    
    newString = RSStringCreateMutableCopy(allocator, 0, url->_string);
    RSStringInsert(newString, rg.location + rg.length, RSSTR("."));
    RSStringRef newExt = RSURLCreateStringByAddingPercentEscapes(allocator, extension, nil, RSSTR(";?/"), url->_encoding);
    RSStringInsert(newString, rg.location + rg.length + 1, newExt);
    RSRelease(newExt);
    result =  _RSURLCreateWithArbitraryString(allocator, newString, url->_base);
    RSRelease(newString);
    if ( filePathURLCreated ) {
        RSRelease(url);
    }
    return result;
}

RSExport RSURLRef RSURLCreateCopyDeletingPathExtension(RSAllocatorRef allocator, RSURLRef url) {
    RSRange rg, dotRg;
    RSURLRef result;
    
    RSAssert1(url != nil, __RSLogAssertion, "%s(): nil argument not allowed", __PRETTY_FUNCTION__);
    url = _RSURLFromNSURL(url);
    __RSGenericValidInstance(url, __RSURLTypeID);
    
    BOOL filePathURLCreated = NO;
#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
    if ( _RSURLIsFileReferenceURL(url) ) {
        // use a file path URL or fail
        RSURLRef filePathURL = RSURLCreateFilePathURL(allocator, url, nil);
        if ( filePathURL ) {
            filePathURLCreated = YES;
            url = filePathURL;
        }
        else {
            return nil;
        }
    }
#endif
    
    rg = _rangeOfLastPathComponent(url);
    if (rg.location < 0) {
        result = nil;
    } else if (rg.length && RSStringFindWithOptions(url->_string, RSSTR("."), rg, RSCompareBackwards, &dotRg)) {
        RSMutableStringRef newString = RSStringCreateMutableCopy(allocator, 0, url->_string);
        dotRg.length = rg.location + rg.length - dotRg.location;
        RSStringDelete(newString, dotRg);
        result = _RSURLCreateWithArbitraryString(allocator, newString, url->_base);
        RSRelease(newString);
    } else {
        result = (RSURLRef)RSRetain(url);
    }
    if ( filePathURLCreated ) {
        RSRelease(url);
    }
    return result;
}


#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
static RSArrayRef HFSPathToURLComponents(RSStringRef path, RSAllocatorRef alloc, BOOL isDir) {
    RSArrayRef components = RSStringCreateArrayBySeparatingStrings(alloc, path, RSSTR(":"));
    RSMutableArrayRef newComponents = RSMutableCopy(alloc, components);
    BOOL doSpecialLeadingColon = NO;
    UniChar firstChar = RSStringGetCharacterAtIndex(path, 0);
    RSBitU32 i, cnt;
    RSRelease(components);
    
    
    if (!doSpecialLeadingColon && firstChar != ':') {
        RSArrayInsertObjectAtIndex(newComponents, 0, RSSTR(""));
    } else if (firstChar != ':') {
        // see what we need to add at the beginning. Under MacOS, if the
        // first character isn't a ':', then the first component is the
        // volume name, and we need to find the mount point.  Bleah. If we
        // don't find a mount point, we're going to have to lie, and make something up.
        RSStringRef firstComp = (RSStringRef)RSArrayObjectAtIndex(newComponents, 0);
        if (RSStringGetLength(firstComp) == 1 && RSStringGetCharacterAtIndex(firstComp, 0) == '/') {
            // "/" is the "magic" path for a UFS root directory
            RSArrayRemoveObjectAtIndex(newComponents, 0);
            RSArrayInsertObjectAtIndex(newComponents, 0, RSSTR(""));
        } else {
            // See if we can get a mount point.
            BOOL foundMountPoint = NO;
            if (!foundMountPoint) {
                // Fall back to treating the volume name as the top level directory
                RSArrayInsertObjectAtIndex(newComponents, 0, RSSTR(""));
            }
        }
    } else {
        RSArrayRemoveObjectAtIndex(newComponents, 0);
    }
    
    cnt = (RSBitU32)RSArrayGetCount(newComponents);
    for (i = 0; i < cnt; i ++) {
        RSStringRef comp = (RSStringRef)RSArrayObjectAtIndex(newComponents, i);
        RSStringRef newComp = nil;
        RSRange searchRg, slashRg;
        searchRg.location = 0;
        searchRg.length = RSStringGetLength(comp);
        while (RSStringFindWithOptions(comp, RSSTR("/"), searchRg, 0, &slashRg)) {
            if (!newComp) {
                newComp = RSStringCreateMutableCopy(alloc, searchRg.location + searchRg.length, comp);
            }
            RSStringReplace((RSMutableStringRef)newComp, slashRg, RSSTR(":"));
            searchRg.length = searchRg.location + searchRg.length - slashRg.location - 1;
            searchRg.location = slashRg.location + 1;
        }
        if (newComp) {
            RSArraySetObjectAtIndex(newComponents, i, newComp);
            RSRelease(newComp);
        }
    }
    if (isDir && RSStringGetLength((RSStringRef)RSArrayObjectAtIndex(newComponents, cnt-1)) != 0) {
        RSArrayAddObject(newComponents, RSSTR(""));
    }
    return newComponents;
}

typedef RSStringRef (*StringTransformation)(RSAllocatorRef, RSStringRef, RSIndex);
static RSArrayRef copyStringArrayWithTransformation(RSArrayRef array, StringTransformation transformation) {
    RSAllocatorRef alloc = RSGetAllocator(array);
    RSMutableArrayRef mArray = nil;
    RSIndex i, c = RSArrayGetCount(array);
    for (i = 0; i < c; i ++) {
        RSStringRef origComp = (RSStringRef)RSArrayObjectAtIndex(array, i);
        RSStringRef unescapedComp = transformation(alloc, origComp, i);
        if (!unescapedComp) {
            break;
        }
        if (unescapedComp != origComp) {
            if (!mArray) {
                mArray = RSMutableCopy(alloc, array);
            }
            RSArraySetObjectAtIndex(mArray, i, unescapedComp);
        }
        RSRelease(unescapedComp);
    }
    if (i != c) {
        if (mArray) RSRelease(mArray);
        return nil;
    } else if (mArray) {
        return mArray;
    } else {
        RSRetain(array);
        return array;
    }
}

static RSStringRef escapePathComponent(RSAllocatorRef alloc, RSStringRef origComponent, RSIndex componentIndex) {
    return RSURLCreateStringByAddingPercentEscapes(alloc, origComponent, nil, RSSTR(";?/"), RSStringEncodingUTF8);
}

static RSStringRef HFSPathToURLPath(RSStringRef path, RSAllocatorRef alloc, BOOL isDir) {
    RSArrayRef components = HFSPathToURLComponents(path, alloc, isDir);
    RSArrayRef newComponents = components ? copyStringArrayWithTransformation(components, escapePathComponent) : nil;
    RSIndex cnt;
    RSStringRef result;
    if (components) RSRelease(components);
    if (!newComponents) return nil;
    
    cnt = RSArrayGetCount(newComponents);
    if (cnt == 1 && RSStringGetLength((RSStringRef)RSArrayObjectAtIndex(newComponents, 0)) == 0) {
        result = (RSStringRef)RSRetain(RSSTR("/"));
    } else {
        result = RSStringCreateByCombiningStrings(alloc, newComponents, RSSTR("/"));
    }
    RSRelease(newComponents);
    return result;
}
#endif



// keys and vals must have space for at least 4 key/value pairs.  No argument can be nil.
// Caller must release values, but not keys
static void __RSURLCopyPropertyListKeysAndValues(RSURLRef url, RSTypeRef *keys, RSTypeRef *vals, RSIndex *count) {
    RSAllocatorRef alloc = RSGetAllocator(url);
    RSURLRef base = RSURLGetBaseURL(url);
    keys[0] = RSSTR("_RSURLStringType");
    keys[1] = RSSTR("_RSURLString");
    keys[2] = RSSTR("_RSURLBaseStringType");
    keys[3] = RSSTR("_RSURLBaseURLString");
    if (RS_IS_OBJC(__RSURLTypeID, url)) {
        SInt32 urlType = FULL_URL_REPRESENTATION;
        vals[0] = RSNumberCreate(alloc, RSNumberUnsignedInteger, &urlType);
        vals[1] = RSURLGetString(url);
    } else {
        SInt32 urlType = FULL_URL_REPRESENTATION;
        vals[0] = RSNumberCreate(alloc, RSNumberUnsignedInteger, &urlType);
        if (url->_flags & IS_DIRECTORY) {
            if (RSStringGetCharacterAtIndex(url->_string, RSStringGetLength(url->_string) - 1) == '/') {
                vals[1] = RSRetain(url->_string);
            } else {
                vals[1] = RSStringCreateWithFormat(alloc, RSSTR("%R%c"), url->_string, '/');
            }
        } else {
            if (RSStringGetCharacterAtIndex(url->_string, RSStringGetLength(url->_string) - 1) != '/') {
                vals[1] = RSRetain(url->_string);
            } else {
                vals[1] = RSStringCreateWithSubstring(alloc, url->_string, RSMakeRange(0, RSStringGetLength(url->_string) - 1));
            }
        }
    }
    if (base != nil) {
        if (RS_IS_OBJC(__RSURLTypeID, base)) {
            SInt32 urlType = FULL_URL_REPRESENTATION;
            vals[2] = RSNumberCreate(alloc, RSNumberUnsignedInteger, &urlType);
            vals[3] = RSURLGetString(base);
        } else {
            SInt32 urlType = FULL_URL_REPRESENTATION;
            vals[2] = RSNumberCreate(alloc, RSNumberUnsignedInteger, &urlType);
            if (base->_flags & IS_DIRECTORY) {
                if (RSStringGetCharacterAtIndex(base->_string, RSStringGetLength(base->_string) - 1) == '/') {
                    vals[3] = RSRetain(base->_string);
                } else {
                    vals[3] = RSStringCreateWithFormat(alloc, RSSTR("%R%c"), base->_string, '/');
                }
            } else {
                if (RSStringGetCharacterAtIndex(base->_string, RSStringGetLength(base->_string) - 1) != '/') {
                    vals[3] = RSRetain(base->_string);
                } else {
                    vals[3] = RSStringCreateWithSubstring(alloc, base->_string, RSMakeRange(0, RSStringGetLength(base->_string) - 1));
                }
            }
        }
        *count = 4;
    } else {
        *count = 2;
    }
}
#include <RSCoreFoundation/RSPropertyList.h>
// Private API for Finder to use
RSDictionaryRef _RSURLCopyPropertyListRepresentation(RSURLRef url) {
    RSTypeRef keys[4], vals[4];
    RSDictionaryRef dict;
    RSIndex count, idx;
    __RSURLCopyPropertyListKeysAndValues(url, keys, vals, &count);
    dict = RSDictionaryCreate(RSGetAllocator(url), keys, vals, count, RSDictionaryRSTypeContext);
    for (idx = 0; idx < count; idx ++) {
        RSRelease(vals[idx]);
    }
    return dict;
}

RSURLRef _RSURLCreateFromPropertyListRepresentation(RSAllocatorRef alloc, RSPropertyListRef pListRepresentation) {
    RSStringRef baseString, string;
    RSNumberRef baseTypeNum, urlTypeNum;
    SInt32 baseType, urlType;
    RSURLRef baseURL = nil, url;
    RSDictionaryRef dict = (RSDictionaryRef)pListRepresentation;
    
    // Start by getting all the pieces and verifying they're of the correct type.
    if (RSGetTypeID(pListRepresentation) != RSDictionaryGetTypeID()) {
        return nil;
    }
    string = (RSStringRef)RSDictionaryGetValue(dict, RSSTR("_RSURLString"));
    if (!string || RSGetTypeID(string) != RSStringGetTypeID()) {
        return nil;
    }
    urlTypeNum = (RSNumberRef)RSDictionaryGetValue(dict, RSSTR("_RSURLStringType"));
    if (!urlTypeNum || RSGetTypeID(urlTypeNum) != RSNumberGetTypeID() || !RSNumberGetValue(urlTypeNum, &urlType) || (urlType != FULL_URL_REPRESENTATION && urlType != RSURLPOSIXPathStyle && urlType != RSURLHFSPathStyle && urlType != RSURLWindowsPathStyle)) {
        return nil;
    }
    baseString = (RSStringRef)RSDictionaryGetValue(dict, RSSTR("_RSURLBaseURLString"));
    if (baseString) {
        if (RSGetTypeID(baseString) != RSStringGetTypeID()) {
            return nil;
        }
        baseTypeNum = (RSNumberRef)RSDictionaryGetValue(dict, RSSTR("_RSURLBaseStringType"));
        if (!baseTypeNum || RSGetTypeID(baseTypeNum) != RSNumberGetTypeID() || !RSNumberGetValue(baseTypeNum, &baseType) ||
            (baseType != FULL_URL_REPRESENTATION && baseType != RSURLPOSIXPathStyle && baseType != RSURLHFSPathStyle && baseType != RSURLWindowsPathStyle)) {
            return nil;
        }
        if (baseType == FULL_URL_REPRESENTATION) {
            baseURL = _RSURLCreateWithArbitraryString(alloc, baseString, nil);
        } else {
            baseURL = RSURLCreateWithFileSystemPathRelativeToBase(alloc, baseString, (RSURLPathStyle)baseType, RSStringGetCharacterAtIndex(baseString, RSStringGetLength(baseString)-1) == '/', nil);
        }
    }
    if (urlType == FULL_URL_REPRESENTATION) {
        url = _RSURLCreateWithArbitraryString(alloc, string, baseURL);
    } else {
        url = RSURLCreateWithFileSystemPathRelativeToBase(alloc, string, (RSURLPathStyle)urlType, RSStringGetCharacterAtIndex(string, RSStringGetLength(string)-1) == '/', baseURL);
    }
    if (baseURL) RSRelease(baseURL);
    return url;
}

#if DEPLOYMENT_TARGET_MACOSX || DEPLOYMENT_TARGET_EMBEDDED
BOOL _RSURLIsFileReferenceURL(RSURLRef url)
{
    // returns YES if url is is a file URL whose path starts with a file ID reference
    BOOL result = NO;
    RSURLRef baseURL = RSURLGetBaseURL(url);
    if ( baseURL ) {
        result = _RSURLIsFileReferenceURL(baseURL);
    }
    else {
        if ( RS_IS_OBJC(__RSURLTypeID, url) ) {
            //            result = (BOOL) RS_OBJC_CALLV((NSURL *)url, isFileReferenceURL);
        }
        else {
            result = ((_getSchemeTypeFromFlags(url->_flags) == kHasFileScheme) && (url->_flags & PATH_HAS_FILE_ID));
        }
    }
    return ( result );
}

static BOOL _RSURLHasFileURLScheme(RSURLRef url, BOOL *hasScheme)
{
    BOOL result;
    RSURLRef baseURL = RSURLGetBaseURL(url);
    
    if ( baseURL ) {
        result = _RSURLHasFileURLScheme(baseURL, hasScheme);
    }
    else {
        if ( RS_IS_OBJC(__RSURLTypeID, url) ) {
            RSStringRef scheme = RSURLCopyScheme(url);
            if ( scheme ) {
                if ( scheme == RSURLFileScheme ) {
                    result = YES;
                }
                else {
                    result = RSStringCompareCaseInsensitive(scheme, RSURLFileScheme) == RSCompareEqualTo;
                }
                if ( hasScheme ) {
                    *hasScheme = YES;
                }
                RSRelease(scheme);
            }
            else {
                if ( hasScheme ) {
                    *hasScheme = NO;
                }
                result = NO;
            }
        }
        else {
            if ( hasScheme ) {
                *hasScheme = (url->_flags & HAS_SCHEME) != 0;
            }
            result = (_getSchemeTypeFromFlags(url->_flags) == kHasFileScheme);
        }
    }
    return ( result );
}

BOOL _RSURLIsFileURL(RSURLRef url)
{
    BOOL result = _RSURLHasFileURLScheme(url, nil);
    return ( result );
}

RSExport RSURLRef RSURLCreateFilePathURL(RSAllocatorRef alloc, RSURLRef url, RSErrorRef *error)
{
    RSURLRef result = nil;
    BOOL hasScheme;
    if (!_RSURLHasFileURLScheme(url, &hasScheme)) {
        if ( !hasScheme ) {
            RSLog(RSSTR("RSURLCreateFilePathURL failed because it was passed this URL which has no scheme: %R"), url);
        }
        if ( error ) {
            *error = RSErrorCreate( RSAllocatorDefault, RSErrorDomainRSCoreFoundation, RSURLReadUnsupportedSchemeError, nil );
        }
        result = nil;
    } else {
        // File URL. Form of the path is unknown. Make a new URL.
        RSStringRef newURLString;
        RSStringRef netLoc;
        RSStringRef fsPath;
        RSStringRef rSpec;
        
        if ( RSURLGetBaseURL( url )) {
            RSURLRef absURL = RSURLCopyAbsoluteURL( url );
            fsPath = RSURLCreateStringWithFileSystemPath(RSGetAllocator(absURL), absURL, RSURLPOSIXPathStyle, NO);
            netLoc = RSURLCopyNetLocation( absURL );
            rSpec = RSURLCopyResourceSpecifier( absURL );
            RSRelease( absURL );
        } else {
            fsPath = RSURLCreateStringWithFileSystemPath(RSGetAllocator(url), url, RSURLPOSIXPathStyle, NO);
            netLoc = RSURLCopyNetLocation( url );
            rSpec = RSURLCopyResourceSpecifier( url );
        }
        if ( fsPath ) {
            RSStringRef urlPath = _replacePathIllegalCharacters( fsPath, alloc, YES );
            newURLString = RSStringCreateWithFormat( alloc, RSSTR("file://%R%R%R%R"), (netLoc ? netLoc : RSSTR("")), urlPath, ((RSStringCompare(urlPath, RSSTR("/"), nil) != RSCompareEqualTo) ? (RSURLHasDirectoryPath( url ) ? RSSTR("/") : RSSTR("")) : RSSTR("")), (rSpec ? rSpec : RSSTR("")));
            result = RSURLCreateWithString( alloc, newURLString, nil );
            RSRelease( newURLString );
            RSRelease( urlPath );
            RSRelease( fsPath );
        } else {
            if ( error ) {
                // Would be better here to get an underlying error back from RSURLCreateStringWithFileSystemPath
                *error = RSErrorCreate( RSAllocatorDefault, RSErrorDomainRSCoreFoundation, RSURLNoSuchResourceError, nil );
            }
            result = nil;
        }
        if ( netLoc ) {
            RSRelease( netLoc );
        }
        if ( rSpec ) {
            RSRelease( rSpec );
        }
    }
    return result;
}

#endif


RSURLRef RSURLCreateFileReferenceURL(RSAllocatorRef alloc, RSURLRef url, RSErrorRef *error) { return nil; }

