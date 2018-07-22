//
//  RSProcessInfoDrawin.c
//  RSCoreFoundation
//
//  Created by RetVal on  8/20/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <RSCoreFoundation/RSCoreFoundation.h>
#include "../../../RSBase/Core/RSInternal.h"

#include <mach/mach.h>

#if DEPLOYMENT_TARGET_MACOSX

#include <IOKit/IOKitLib.h> /*get serinal number*/

#elif DEPLOYMENT_TARGET_IPHONEOS

#endif

static RSStringRef ___sn()
{
    RSStringRef result = nil;
	mach_port_t         masterPort;
	kern_return_t       kr = noErr;
	io_registry_entry_t  entry = MACH_PORT_NULL;
	CFDataRef         propData;
	CFTypeRef         prop;
	CFTypeID         propID = 0;
	UInt8           *data;
	unsigned int        i, bufSize;
	char            *s, *t;
	char            firstPart[64], secondPart[64];
	
	kr = IOMasterPort(MACH_PORT_NULL, &masterPort);
	if (kr == noErr)
    {
		entry = IORegistryGetRootEntry(masterPort);
		if (entry != MACH_PORT_NULL)
        {
			prop = IORegistryEntrySearchCFProperty( entry,
                                                   kIODeviceTreePlane,
                                                   CFSTR("serial-number"),
                                                   nil, kIORegistryIterateRecursively);
			if (prop == nil)
            {
				result = RSSTR("null");
			}
            else
            {
				propID = CFGetTypeID(prop);
                
                if (propID == CFDataGetTypeID())
                {
                    propData = (CFDataRef)prop;
                    bufSize = (unsigned int)CFDataGetLength(propData);
                    if (bufSize > 0)
                    {
                        data = (UInt8*)CFDataGetBytePtr(propData);
                        if (data)
                        {
                            i = 0;
                            s = (char*)data;
                            t = firstPart;
                            while (i < bufSize)
                            {
                                i++;
                                if (*s != '\0')
                                {
                                    *t++ = *s++;
                                } else
                                {
                                    break;
                                }
                            }
                            *t = '\0';
                            
                            while ((i < bufSize) && (*s == '\0'))
                            {
                                i++;
                                s++;
                            }
                            
                            t = secondPart;
                            while (i < bufSize)
                            {
                                i++;
                                if (*s != '\0')
                                {
                                    *t++ = *s++;
                                } else
                                {
                                    break;
                                }
                            }
                            *t = '\0';
                            result =  RSStringCreateWithFormat(RSAllocatorSystemDefault, RSSTR("%s%s"),secondPart,firstPart);
                        }
                    }
                    CFRelease((CFTypeRef)prop);
                }
                else if (NULL != prop)
                    CFRelease((CFTypeRef)prop);
            }
            IOObjectRelease(entry);
		}
		mach_port_deallocate(mach_task_self(), masterPort);
	}
	return(result);
}

RSExport RSStringRef __RSProcessInfoGetSerinalNumber()
{
    return ___sn();
}
