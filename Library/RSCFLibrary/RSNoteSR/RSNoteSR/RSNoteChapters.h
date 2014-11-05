//
//  RSNoteChapters.h
//  RSNoteSR
//
//  Created by closure on 9/30/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RSNoteChapters : NSObject
@property (nonatomic, strong, readonly) NSMutableArray *chapters;
+ (instancetype)chapters;
- (instancetype)init;
- (NSString *)description;
@end
