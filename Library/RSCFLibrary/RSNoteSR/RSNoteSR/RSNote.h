//
//  RSNote.h
//  RSNoteSR
//
//  Created by closure on 9/30/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#import <Foundation/Foundation.h>

#import "RSNoteChapters.h"
#import "RSNoteChapter.h"

@interface RSNote : NSObject
@property (nonatomic, strong) NSString *title;
@property (nonatomic, strong) NSString *auth;
@property (nonatomic, strong) RSNoteChapters *chapters;
@property (nonatomic, strong) NSString *baseURL;
//@property (nonatomic, strong)
+ (instancetype)note;
+ (instancetype)noteWithContent:(NSString *)fileContent;
- (instancetype)init;
- (instancetype)initWithContent:(NSString *)fileContent;
- (NSString *)description;
@end
