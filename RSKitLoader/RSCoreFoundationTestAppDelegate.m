//
//  RSCoreFoundationTestAppDelegate.m
//  RSCoreFoundationLoader
//
//  Created by RetVal on 10/14/12.
//  Copyright (c) 2012 RetVal. All rights reserved.
//

#import "RSCoreFoundationTestAppDelegate.h"
#import <RSCoreFoundation/RSCoreFoundation.h>
#include <pthread.h>
#include <asl.h>
@interface RSCoreFoundationTestAppDelegate()

@end

@implementation RSCoreFoundationTestAppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification
{
    RSNumberRef speicific = RSNumberCreateBoolean(RSAllocatorDefault, YES);
    RSShow(speicific);
//    RSDictionarySetValue((RSMutableDictionaryRef)plistAboutInfo, _RSBundleHasSpeicificInfoKey, speicific);
    RSRelease(speicific);
    
    RSBundleRef mainBundle = RSBundleGetMainBundle();
    RSArchiverRef archiver = RSArchiverCreate(RSAllocatorDefault);
    RSDictionaryRef dict = RSBundleGetInfoDict(mainBundle);
    RSArchiverEncodeObjectForKey(archiver, RSSTR("bundle"), dict);
    RSDataWriteToFile(RSAutorelease(RSArchiverCopyData(archiver)), RSFileManagerStandardizingPath(RSSTR("~/Desktop/RSBundle.archiver.plist")), RSWriteFileAutomatically);
    RSRelease(archiver);
    return;
    RSShow(RSSTR("START"));
    for (RSUInteger idx = 0; idx < 1000000; idx++) {
        RSNumberCreate(RSAllocatorDefault, RSNumberInteger, &idx);
    }
    RSShow(RSSTR("END"));
    return ;
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
    return YES;
}
@end
