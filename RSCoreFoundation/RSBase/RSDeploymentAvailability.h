//
//  RSDeploymentAvailability.h
//  RSCoreFoundation
//
//  Created by RetVal on 12/27/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#ifndef RSCoreFoundation_RSDeploymentAvailability_h
#define RSCoreFoundation_RSDeploymentAvailability_h

#define DEPLOYMENT_TARGET_MACOSX        1
#define DEPLOYMENT_TARGET_IPHONEOS      0
#define DEPLOYMENT_TARGET_LINUX         0
#define DEPLOYMENT_TARGET_WINDOWS       0
#define DEPLOYMENT_TARGET_EMBEDDED      DEPLOYMENT_TARGET_IPHONEOS
#define DEPLOYMENT_TARGET_EMBEDDED_MINI 0
#define DEPLOYMENT_TARGET_SOLARIS       0
#define DEPLOYMENT_TARGET_FREEBSD       0

#if defined(__GNUC__) && ( defined(__APPLE_CPP__) || defined(__APPLE_CC__) || defined(__MACOS_CLASSIC__) )
#define TARGET_OS_MAC               1
#define TARGET_OS_WIN32             0
#define TARGET_OS_UNIX              0
#define TARGET_OS_OSX               1
#define TARGET_OS_IPHONE            0
#define TARGET_OS_IOS               0
#define TARGET_OS_WATCH             0
#define TARGET_OS_BRIDGE            0
#define TARGET_OS_TV                0
#define TARGET_OS_SIMULATOR         0
#define TARGET_OS_EMBEDDED          0
#define TARGET_IPHONE_SIMULATOR     TARGET_OS_SIMULATOR /* deprecated */
#define TARGET_OS_NANO              TARGET_OS_WATCH /* deprecated */
#if defined(__ppc__)
#define TARGET_CPU_PPC          1
#define TARGET_CPU_PPC64        0
#define TARGET_CPU_68K          0
#define TARGET_CPU_X86          0
#define TARGET_CPU_X86_64       0
#define TARGET_CPU_ARM          0
#define TARGET_CPU_ARM64        0
#define TARGET_CPU_MIPS         0
#define TARGET_CPU_SPARC        0
#define TARGET_CPU_ALPHA        0
#define TARGET_RT_LITTLE_ENDIAN 0
#define TARGET_RT_BIG_ENDIAN    1
#define TARGET_RT_64_BIT        0
#ifdef __MACOS_CLASSIC__
#define TARGET_RT_MAC_CFM    1
#define TARGET_RT_MAC_MACHO  0
#else
#define TARGET_RT_MAC_CFM    0
#define TARGET_RT_MAC_MACHO  1
#endif
#elif defined(__ppc64__)
#define TARGET_CPU_PPC          0
#define TARGET_CPU_PPC64        1
#define TARGET_CPU_68K          0
#define TARGET_CPU_X86          0
#define TARGET_CPU_X86_64       0
#define TARGET_CPU_ARM          0
#define TARGET_CPU_ARM64        0
#define TARGET_CPU_MIPS         0
#define TARGET_CPU_SPARC        0
#define TARGET_CPU_ALPHA        0
#define TARGET_RT_LITTLE_ENDIAN 0
#define TARGET_RT_BIG_ENDIAN    1
#define TARGET_RT_64_BIT        1
#define TARGET_RT_MAC_CFM       0
#define TARGET_RT_MAC_MACHO     1
#elif defined(__i386__)
#define TARGET_CPU_PPC          0
#define TARGET_CPU_PPC64        0
#define TARGET_CPU_68K          0
#define TARGET_CPU_X86          1
#define TARGET_CPU_X86_64       0
#define TARGET_CPU_ARM          0
#define TARGET_CPU_ARM64        0
#define TARGET_CPU_MIPS         0
#define TARGET_CPU_SPARC        0
#define TARGET_CPU_ALPHA        0
#define TARGET_RT_MAC_CFM       0
#define TARGET_RT_MAC_MACHO     1
#define TARGET_RT_LITTLE_ENDIAN 1
#define TARGET_RT_BIG_ENDIAN    0
#define TARGET_RT_64_BIT        0
#elif defined(__x86_64__)
#define TARGET_CPU_PPC          0
#define TARGET_CPU_PPC64        0
#define TARGET_CPU_68K          0
#define TARGET_CPU_X86          0
#define TARGET_CPU_X86_64       1
#define TARGET_CPU_ARM          0
#define TARGET_CPU_ARM64        0
#define TARGET_CPU_MIPS         0
#define TARGET_CPU_SPARC        0
#define TARGET_CPU_ALPHA        0
#define TARGET_RT_MAC_CFM       0
#define TARGET_RT_MAC_MACHO     1
#define TARGET_RT_LITTLE_ENDIAN 1
#define TARGET_RT_BIG_ENDIAN    0
#define TARGET_RT_64_BIT        1
#elif defined(__arm__)
#define TARGET_CPU_PPC          0
#define TARGET_CPU_PPC64        0
#define TARGET_CPU_68K          0
#define TARGET_CPU_X86          0
#define TARGET_CPU_X86_64       0
#define TARGET_CPU_ARM          1
#define TARGET_CPU_ARM64        0
#define TARGET_CPU_MIPS         0
#define TARGET_CPU_SPARC        0
#define TARGET_CPU_ALPHA        0
#define TARGET_RT_MAC_CFM       0
#define TARGET_RT_MAC_MACHO     1
#define TARGET_RT_LITTLE_ENDIAN 1
#define TARGET_RT_BIG_ENDIAN    0
#define TARGET_RT_64_BIT        0
#elif defined(__arm64__)
#define TARGET_CPU_PPC          0
#define TARGET_CPU_PPC64        0
#define TARGET_CPU_68K          0
#define TARGET_CPU_X86          0
#define TARGET_CPU_X86_64       0
#define TARGET_CPU_ARM          0
#define TARGET_CPU_ARM64        1
#define TARGET_CPU_MIPS         0
#define TARGET_CPU_SPARC        0
#define TARGET_CPU_ALPHA        0
#define TARGET_RT_MAC_CFM       0
#define TARGET_RT_MAC_MACHO     1
#define TARGET_RT_LITTLE_ENDIAN 1
#define TARGET_RT_BIG_ENDIAN    0
#define TARGET_RT_64_BIT        1
#else
#error unrecognized GNU C compiler
#endif



/*
 *   CodeWarrior compiler from Metrowerks/Motorola
 */
#elif defined(__MWERKS__)
#define TARGET_OS_MAC               1
#define TARGET_OS_WIN32             0
#define TARGET_OS_UNIX              0
#define TARGET_OS_EMBEDDED          0
#if defined(__POWERPC__)
#define TARGET_CPU_PPC          1
#define TARGET_CPU_PPC64        0
#define TARGET_CPU_68K          0
#define TARGET_CPU_X86          0
#define TARGET_CPU_MIPS         0
#define TARGET_CPU_SPARC        0
#define TARGET_CPU_ALPHA        0
#define TARGET_RT_LITTLE_ENDIAN 0
#define TARGET_RT_BIG_ENDIAN    1
#elif defined(__INTEL__)
#define TARGET_CPU_PPC          0
#define TARGET_CPU_PPC64        0
#define TARGET_CPU_68K          0
#define TARGET_CPU_X86          1
#define TARGET_CPU_MIPS         0
#define TARGET_CPU_SPARC        0
#define TARGET_CPU_ALPHA        0
#define TARGET_RT_LITTLE_ENDIAN 1
#define TARGET_RT_BIG_ENDIAN    0
#else
#error unknown Metrowerks CPU type
#endif
#define TARGET_RT_64_BIT            0
#ifdef __MACH__
#define TARGET_RT_MAC_CFM       0
#define TARGET_RT_MAC_MACHO     1
#else
#define TARGET_RT_MAC_CFM       1
#define TARGET_RT_MAC_MACHO     0
#endif

/*
 *   unknown compiler
 */
#else
#if defined(TARGET_CPU_PPC) && TARGET_CPU_PPC
#define TARGET_CPU_PPC64    0
#define TARGET_CPU_68K      0
#define TARGET_CPU_X86      0
#define TARGET_CPU_X86_64   0
#define TARGET_CPU_ARM      0
#define TARGET_CPU_ARM64    0
#define TARGET_CPU_MIPS     0
#define TARGET_CPU_SPARC    0
#define TARGET_CPU_ALPHA    0
#elif defined(TARGET_CPU_PPC64) && TARGET_CPU_PPC64
#define TARGET_CPU_PPC      0
#define TARGET_CPU_68K      0
#define TARGET_CPU_X86      0
#define TARGET_CPU_X86_64   0
#define TARGET_CPU_ARM      0
#define TARGET_CPU_ARM64    0
#define TARGET_CPU_MIPS     0
#define TARGET_CPU_SPARC    0
#define TARGET_CPU_ALPHA    0
#elif defined(TARGET_CPU_X86) && TARGET_CPU_X86
#define TARGET_CPU_PPC      0
#define TARGET_CPU_PPC64    0
#define TARGET_CPU_X86_64   0
#define TARGET_CPU_68K      0
#define TARGET_CPU_ARM      0
#define TARGET_CPU_ARM64    0
#define TARGET_CPU_MIPS     0
#define TARGET_CPU_SPARC    0
#define TARGET_CPU_ALPHA    0
#elif defined(TARGET_CPU_X86_64) && TARGET_CPU_X86_64
#define TARGET_CPU_PPC      0
#define TARGET_CPU_PPC64    0
#define TARGET_CPU_X86      0
#define TARGET_CPU_68K      0
#define TARGET_CPU_ARM      0
#define TARGET_CPU_ARM64    0
#define TARGET_CPU_MIPS     0
#define TARGET_CPU_SPARC    0
#define TARGET_CPU_ALPHA    0
#elif defined(TARGET_CPU_ARM) && TARGET_CPU_ARM
#define TARGET_CPU_PPC      0
#define TARGET_CPU_PPC64    0
#define TARGET_CPU_X86      0
#define TARGET_CPU_X86_64   0
#define TARGET_CPU_68K      0
#define TARGET_CPU_ARM64    0
#define TARGET_CPU_MIPS     0
#define TARGET_CPU_SPARC    0
#define TARGET_CPU_ALPHA    0
#elif defined(TARGET_CPU_ARM64) && TARGET_CPU_ARM64
#define TARGET_CPU_PPC      0
#define TARGET_CPU_PPC64    0
#define TARGET_CPU_X86      0
#define TARGET_CPU_X86_64   0
#define TARGET_CPU_68K      0
#define TARGET_CPU_ARM      0
#define TARGET_CPU_MIPS     0
#define TARGET_CPU_SPARC    0
#define TARGET_CPU_ALPHA    0
#else
/*
 NOTE:   If your compiler errors out here then support for your compiler
 has not yet been added to TargetConditionals.h.
 
 TargetConditionals.h is designed to be plug-and-play.  It auto detects
 which compiler is being run and configures the TARGET_ conditionals
 appropriately.
 
 The short term work around is to set the TARGET_CPU_ and TARGET_OS_
 on the command line to the compiler (e.g. -DTARGET_CPU_MIPS=1 -DTARGET_OS_UNIX=1)
 
 The long term solution is to add a new case to this file which
 auto detects your compiler and sets up the TARGET_ conditionals.
 Then submit the changes to Apple Computer.
 */
#error TargetConditionals.h: unknown compiler (see comment above)
#define TARGET_CPU_PPC    0
#define TARGET_CPU_68K    0
#define TARGET_CPU_X86    0
#define TARGET_CPU_ARM    0
#define TARGET_CPU_ARM64  0
#define TARGET_CPU_MIPS   0
#define TARGET_CPU_SPARC  0
#define TARGET_CPU_ALPHA  0
#endif
#define TARGET_OS_MAC                1
#define TARGET_OS_WIN32              0
#define TARGET_OS_UNIX               0
#define TARGET_OS_EMBEDDED           0
#if TARGET_CPU_PPC || TARGET_CPU_PPC64
#define TARGET_RT_BIG_ENDIAN     1
#define TARGET_RT_LITTLE_ENDIAN  0
#else
#define TARGET_RT_BIG_ENDIAN     0
#define TARGET_RT_LITTLE_ENDIAN  1
#endif
#if TARGET_CPU_PPC64 || TARGET_CPU_X86_64
#define TARGET_RT_64_BIT         1
#else
#define TARGET_RT_64_BIT         0
#endif
#ifdef __MACH__
#define TARGET_RT_MAC_MACHO      1
#define TARGET_RT_MAC_CFM        0
#else
#define TARGET_RT_MAC_MACHO      0
#define TARGET_RT_MAC_CFM        1
#endif

#endif


#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
    #define RS_DEPRECATED_ATTRIBUTE __attribute__((deprecated))
    #if defined(__clang__) && __has_extension(attribute_deprecated_with_message)
        #define RS_DEPRECATED_ATTRIBUTE_REASON(R) __attribute__((deprecated(R)))
    #else
        #define RS_DEPRECATED_ATTRIBUTE_REASON(R) RS_DEPRECATED_ATTRIBUTE
    #endif
#else
    #define RS_DEPRECATED_ATTRIBUTE
    #define RS_DEPRECATED_ATTRIBUTE_REASON(R) RS_DEPRECATED_ATTRIBUTE
#endif

#if defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
    #define RS_UNAVAILABLE_ATTRIBUTE __attribute__((unavailable))
    #if defined(__clang__) && __has_extension(attribute_unavailable_with_message)
        #define RS_UNAVAILABLE_ATTRIBUTE_REASON(R) __attribute__((unavailable(R)))
    #else
        #define RS_UNAVAILABLE_ATTRIBUTE_REASON(R) RS_UNAVAILABLE_ATTRIBUTE
    #endif
#else
    #define RS_UNAVAILABLE_ATTRIBUTE
    #define RS_UNAVAILABLE_ATTRIBUTE_REASON(R) RS_UNAVAILABLE_ATTRIBUTE
#endif

#define RSCoreFoundationVersionNumber0_0         744
#define RSCoreFoundationVersionNumber0_1         812
#define RSCoreFoundationVersionNumber0_2        1000
#define RSCoreFoundationVersionNumber0_3        1200
#define RSCoreFoundationVersionNumber0_4        1400
#define RSCoreFoundationVersionNumber0_5        2000

#ifndef RSCoreFoundationVersionNumberCurrent
#define RSCoreFoundationVersionNumberCurrent   RSCoreFoundationVersionNumber0_5
#endif

/*
 available - 0.0
 deprecated - 0.0
 available 0.0 but deprecated 0.1
 */

#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_0  RS_DEPRECATED_ATTRIBUTE

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_0
#define AVAILABLE_RSCF_VERSION_0_0_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_0      RS_DEPRECATED_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_0    RS_UNAVAILABLE_ATTRIBUTE
#else
#define AVAILABLE_RSCF_VERSION_0_0_AND_LATER   RS_UNAVAILABLE_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_0      RS_UNAVAILABLE_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_0    
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_1
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_1  RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_1  RS_DEPRECATED_ATTRIBUTE
#else
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_1  
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_1  AVAILABLE_RSCF_VERSION_0_1_AND_LATER
#endif

/*
 available - 0.1
 deprecated - 0.1
 // available 0.1 but deprecated 0.2
 */
#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_1
#define AVAILABLE_RSCF_VERSION_0_1_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_1      RS_DEPRECATED_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_1    RS_UNAVAILABLE_ATTRIBUTE
#else
#define AVAILABLE_RSCF_VERSION_0_1_AND_LATER   RS_UNAVAILABLE_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_1      RS_UNAVAILABLE_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_1
#endif


#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_2
#define AVAILABLE_RSCF_VERSION_0_2_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_2      RS_DEPRECATED_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_2    RS_UNAVAILABLE_ATTRIBUTE
#else
#define AVAILABLE_RSCF_VERSION_0_2_AND_LATER   RS_UNAVAILABLE_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_2      RS_UNAVAILABLE_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_2
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_2
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_2 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_2 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_2 RS_DEPRECATED_ATTRIBUTE
#else
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_2 AVAILABLE_RSCF_VERSION_0_0_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_2 AVAILABLE_RSCF_VERSION_0_1_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_2 AVAILABLE_RSCF_VERSION_0_2_AND_LATER
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_3
#define AVAILABLE_RSCF_VERSION_0_3_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_3      RS_DEPRECATED_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_3    RS_UNAVAILABLE_ATTRIBUTE
#else
#define AVAILABLE_RSCF_VERSION_0_3_AND_LATER   RS_UNAVAILABLE_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_3      RS_UNAVAILABLE_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_3
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_3
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_3 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_3 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_3 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_3_DEP_0_3 RS_DEPRECATED_ATTRIBUTE
#else
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_3 AVAILABLE_RSCF_VERSION_0_0_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_3 AVAILABLE_RSCF_VERSION_0_1_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_3 AVAILABLE_RSCF_VERSION_0_2_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_3_DEP_0_3 AVAILABLE_RSCF_VERSION_0_3_AND_LATER
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_4
#define AVAILABLE_RSCF_VERSION_0_4_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_4      RS_DEPRECATED_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_4    RS_UNAVAILABLE_ATTRIBUTE
#else
#define AVAILABLE_RSCF_VERSION_0_4_AND_LATER   RS_UNAVAILABLE_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_4      RS_UNAVAILABLE_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_4
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_4
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_4 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_4 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_4 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_3_DEP_0_4 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_4_DEP_0_4 RS_DEPRECATED_ATTRIBUTE
#else
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_4 AVAILABLE_RSCF_VERSION_0_0_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_4 AVAILABLE_RSCF_VERSION_0_1_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_4 AVAILABLE_RSCF_VERSION_0_2_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_3_DEP_0_4 AVAILABLE_RSCF_VERSION_0_3_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_4_DEP_0_4 AVAILABLE_RSCF_VERSION_0_4_AND_LATER
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_5
#define AVAILABLE_RSCF_VERSION_0_5_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_5      RS_DEPRECATED_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_5    RS_UNAVAILABLE_ATTRIBUTE
#else
#define AVAILABLE_RSCF_VERSION_0_5_AND_LATER   RS_UNAVAILABLE_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_5      RS_UNAVAILABLE_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_5
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_5
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_5 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_5 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_5 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_3_DEP_0_5 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_4_DEP_0_5 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_5_DEP_0_5 RS_DEPRECATED_ATTRIBUTE
#else
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_5 AVAILABLE_RSCF_VERSION_0_0_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_5 AVAILABLE_RSCF_VERSION_0_1_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_5 AVAILABLE_RSCF_VERSION_0_2_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_3_DEP_0_5 AVAILABLE_RSCF_VERSION_0_3_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_4_DEP_0_5 AVAILABLE_RSCF_VERSION_0_4_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_5_DEP_0_5 AVAILABLE_RSCF_VERSION_0_5_AND_LATER
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_5
#define AVAILABLE_RSCF_VERSION_0_5_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_5      RS_DEPRECATED_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_5    RS_UNAVAILABLE_ATTRIBUTE
#else
#define AVAILABLE_RSCF_VERSION_0_5_AND_LATER   RS_UNAVAILABLE_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_5      RS_UNAVAILABLE_ATTRIBUTE
#define __UNAVAILABILITY_INTERNAL__RSCF_0_5
#endif

#if RSCoreFoundationVersionNumberCurrent >= RSCoreFoundationVersionNumber0_6
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_6 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_6 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_6 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_3_DEP_0_6 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_4_DEP_0_6 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_5_DEP_0_6 RS_DEPRECATED_ATTRIBUTE
#define __AVAILABILITY_INTERNAL__RSCF_0_6_DEP_0_6 RS_DEPRECATED_ATTRIBUTE
#else
#define __AVAILABILITY_INTERNAL__RSCF_0_0_DEP_0_6 AVAILABLE_RSCF_VERSION_0_0_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_1_DEP_0_6 AVAILABLE_RSCF_VERSION_0_1_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_2_DEP_0_6 AVAILABLE_RSCF_VERSION_0_2_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_3_DEP_0_6 AVAILABLE_RSCF_VERSION_0_3_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_4_DEP_0_6 AVAILABLE_RSCF_VERSION_0_4_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_5_DEP_0_6 AVAILABLE_RSCF_VERSION_0_5_AND_LATER
#define __AVAILABILITY_INTERNAL__RSCF_0_6_DEP_0_6 AVAILABLE_RSCF_VERSION_0_6_AND_LATER
#endif



#ifndef __RS_AVAILABLE_STARTING
#define __RS_AVAILABLE_STARTING(_str)    __AVAILABILITY_INTERNAL__RSCF##_str
#define __RS_UNAVAILABLE_STARTING(_str)    __UNAVAILABILITY_INTERNAL__RSCF##_str
#define __RS_AVAILABLE_BUT_DEPRECATED(_rsIntro, _rsDep) __AVAILABILITY_INTERNAL__RSCF_##_rsIntro##_DEP_##_rsDep
#endif

#endif
