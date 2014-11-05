//
//  RSNoteChapter.h
//  RSNoteSR
//
//  Created by closure on 9/30/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#import <Foundation/Foundation.h>

@interface RSNoteChapter : NSObject
@property (nonatomic, strong, readonly) NSString *name;
@property (nonatomic, strong, readonly) NSString *subURLString;
@property (nonatomic, strong) NSString *content;

+ (instancetype)chapterWithName:(NSString *)name subURLString:(NSString *)subURLString;
- (instancetype)initWithName:(NSString *)name subURLString:(NSString *)subURLString;
- (void)parseContent:(NSString *)content handler:(void (^)(NSXMLElement *element))handler;
- (void)setContent:(NSString *)content;
@end
