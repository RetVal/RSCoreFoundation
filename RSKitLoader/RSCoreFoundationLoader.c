//
//  RSCoreFoundationLoader.c
//  RSCoreFoundation
//
//  Created by Closure on 10/31/13.
//  Copyright (c) 2013 RetVal. All rights reserved.
//

#include <stdio.h>
#include <RSCoreFoundation/RSCoreFoundation.h>
#include <RSCoreFoundation/RSDelegate.h>
#include <RSCoreFoundation/RSProtocol.h>

void contextAddLog(RSTypeRef context, ...)
{
    va_list va;
    va_start(va, context);
    RSTypeRef key = context;
    RSMutableStringRef log = RSStringCreateMutable(RSAllocatorDefault, 0);
    RSStringAppendString(log, RSAutorelease(RSUUIDCreateString(RSAllocatorDefault, context)));
    RSStringRef splitString = RSSTR(" | ");
    for (;key;)
    {
        key = va_arg(va, RSTypeRef);
        // check type
        if (!key) break;
        RSStringAppendStringWithFormat(log, RSSTR("%r%r"), splitString, key);
    }
    va_end(va);
    RSShow(log);
    RSRelease(log);
}

void func1(RSTypeRef context, RSStringRef uid)
{
    contextAddLog(context, __RSStringMakeConstantString(__FUNCTION__), uid, RSNumberWithInt(5), nil);
}

void func2(RSTypeRef context, RSStringRef uid)
{
    func1(context, uid);
    contextAddLog(context, __RSStringMakeConstantString(__FUNCTION__), uid, RSNumberWithInt(7), nil);
}

void publicApi(RSStringRef uid)
{
    RSTypeRef context = RSUUIDCreate(RSAllocatorDefault);
    func2(context, uid);
    RSRelease(context);
}

RSProtocolItem *__RSProtocolItemSetup(RSProtocolItem *item, PCE name, ISA imp)
{
    item->sel = name;
    item->imp = imp;
    return item;
}

RSProtocolContext* __RSProtocolContextSetup(RSProtocolContext *context, const char *protocolName, const unsigned int sizeOfMethods, RSProtocolItem items[])
{
    if (!context) context = RSAllocatorAllocate(RSAllocatorDefault, sizeof(context) + (sizeOfMethods - 1) * sizeof(RSProtocolItem));
    for (unsigned int idx = 0; idx < sizeOfMethods; idx++) {
        context->items[idx] = items[idx];
    }
    context->delegateName = protocolName;
    context->countOfItems = sizeOfMethods;
    return context;
}

void caller()
{
    
//    RSProtocolItem items[2] = {{"RSURLConnectionDidReceiveData", publicApi}, {"RSURLConnectionDidReceiveData2", func1}};
//    RSProtocolContext *protocolContext = __RSProtocolContextSetup(nil, "RSURLConnectionDataDelegate", sizeof(items) / sizeof(RSProtocolItem), items);
//    RSProtocolRef RSURLConnectionDataDelegate = RSProtocolCreateWithContext(RSAllocatorDefault, protocolContext, nil);
//    RSDelegateRef delegate = RSDelegateCreateWithProtocols(RSAllocatorDefault, RSSTR("RSURLConnectionDataDelegate"), RSArrayWithObject(RSURLConnectionDataDelegate));
    
    
    
//    RSAllocatorDeallocate(RSAllocatorDefault, protocolContext);
//    RSRelease(RSURLConnectionDataDelegate);
    
}