//
//  RSNoteManager.m
//  RSNoteSR
//
//  Created by closure on 9/30/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#import "RSNoteManager.h"

@interface RSNoteManager () {
    @private
    NSString *_path;
    NSOperationQueue *_operationQueue;
}

@end

@implementation RSNoteManager
+ (instancetype)defaultManager {
    static dispatch_once_t onceToken;
    static RSNoteManager *manager = nil;
    dispatch_once(&onceToken, ^{
        manager = [[RSNoteManager alloc] initWithPath:[@"~/Desktop/" stringByStandardizingPath]];
    });
    return manager;
}

- (instancetype)initWithPath:(NSString *)path {
    if (self = [self init]) {
        _path = path;
        _operationQueue = [[NSOperationQueue alloc] init];
        [_operationQueue setMaxConcurrentOperationCount:10];
        [_operationQueue setSuspended:NO];
    }
    return self;
}

- (BOOL)writeNote:(RSNote *)note {
    NSString *base = [NSString stringWithFormat:@"%@/%@-%@/", _path, [note title], [note auth]];
    NSFileManager *fmg = [NSFileManager defaultManager];
    NSError *error = nil;
    if (![fmg createDirectoryAtPath:base withIntermediateDirectories:YES attributes:nil error:&error]) {
        NSLog(@"%@", error);
        return NO;
    }
    RSNoteChapters *chapters = [note chapters];
    NSMutableArray *baseStack = [[NSMutableArray alloc] init];
    BOOL inAChapter = NO;
    dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_HIGH, 2);
    
    for (RSNoteChapter *chapter in [chapters chapters]) {
        NSString *chapterBase = nil;
    CHATPER_BEGIN:
        chapterBase = [baseStack lastObject] ?: base;
        if (![chapter subURLString] && inAChapter == NO) {
            inAChapter = YES;
            NSString *newBase = [[NSString alloc] initWithFormat:@"%@%@", base, [chapter name]];
            [baseStack addObject:newBase];
            if (![fmg createDirectoryAtPath:newBase withIntermediateDirectories:YES attributes:nil error:&error]) {
                NSLog(@"%@", error);
                return NO;
            }
            continue;
        } else if (![chapter subURLString] && inAChapter == YES) {
            [baseStack removeLastObject];
            inAChapter = NO;
            goto CHATPER_BEGIN;
        } else if ([chapter subURLString] && inAChapter == YES) {
            dispatch_async(queue, ^{
                [NSURLConnection sendAsynchronousRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:[NSString stringWithFormat:@"%@/%@", [note baseURL], [chapter subURLString]]]] queue:_operationQueue completionHandler:^(NSURLResponse *response, NSData *data, NSError *connectionError) {
                    if (connectionError) {
                        NSLog(@"%@", connectionError);
                        dispatch_semaphore_signal(semaphore);
                        return;
                    }
                    NSString *filePath = nil;
                    NSStringEncoding enc = CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingGB_18030_2000);
                    NSString *content = [[NSString alloc] initWithData:data encoding:enc];

                    if ([[chapter name] isEqualToString:@"插图"]) {
                        filePath = [[NSString alloc] initWithFormat:@"%@/%@", chapterBase, [chapter name]];
                        if(![fmg createDirectoryAtPath:filePath withIntermediateDirectories:YES attributes:nil error:&connectionError]) {
                            NSLog(@"%@", connectionError);
                            dispatch_semaphore_signal(semaphore);
                            return;
                        }
                        [chapter parseContent:content handler:^(NSXMLElement *element) {
                            NSArray *imageDivs = [element elementsForName:@"div"];
                            NSUInteger idx = 1;
                            dispatch_group_t group = dispatch_group_create();
                            for (NSXMLElement *imageDiv in imageDivs) {
                                NSString *urlString = [[[[imageDiv elementsForName:@"a"] firstObject] attributeForName:@"href"] objectValue];
                                if (!urlString) continue;
                                NSString *imageBase = [[NSString alloc] initWithFormat:@"%@/%02ld.jpg", filePath, idx++];
                                
                                dispatch_group_async(group, queue, ^{
                                    NSError *error = nil;
                                    NSData *data = [NSURLConnection sendSynchronousRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:urlString]] returningResponse:nil error:&error];
                                    if (data && !error) {
                                        NSLog(@"write chapter %@", imageBase);
                                        [data writeToFile:imageBase atomically:YES];
                                    }
                                });
                            }
                            dispatch_group_wait(group, DISPATCH_TIME_FOREVER);
                        }];
                    } else {
                        filePath = [[NSString alloc] initWithFormat:@"%@/%@.txt", chapterBase, [chapter name]];
                        [chapter setContent:content];
                        NSLog(@"write chapter %@", chapter);
                        BOOL success = [[chapter content] writeToFile:filePath atomically:YES encoding:NSUTF8StringEncoding error:&connectionError];
                        if (!success) {
                            NSLog(@"%@, %@", chapter, connectionError);
                        }
                    }
                    dispatch_semaphore_signal(semaphore);
                    return;
                }];
            });
        }
    }
    
    for (NSUInteger idx = 0; idx < [[chapters chapters] count]; ++idx) {
        dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
    }
    return NO;
}
@end
