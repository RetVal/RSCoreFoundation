//
//  RSNoteManager.h
//  RSNoteSR
//
//  Created by closure on 9/30/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "RSNote.h"

@interface RSNoteManager : NSObject
+ (instancetype)defaultManager;
- (instancetype)initWithPath:(NSString *)path;
- (BOOL)writeNote:(RSNote *)note;
@end
