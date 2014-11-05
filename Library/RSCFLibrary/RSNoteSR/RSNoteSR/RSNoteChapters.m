//
//  RSNoteChapters.m
//  RSNoteSR
//
//  Created by closure on 9/30/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#import "RSNoteChapters.h"

@implementation RSNoteChapters
+ (instancetype)chapters {
    return [[RSNoteChapters alloc] init];
}

- (instancetype)init {
    if (self = [super init]) {
        _chapters = [[NSMutableArray alloc] init];
    }
    return self;
}

- (NSString *)description {
    return [_chapters description];
}
@end
