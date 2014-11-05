//
//  RSNote.m
//  RSNoteSR
//
//  Created by closure on 9/30/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#import "RSNote.h"

@interface RSNote() {
    @private
    NSXMLDocument *_document;
}

@end

@implementation RSNote
+ (instancetype)note {
    return [[RSNote alloc] init];
}

- (instancetype)init {
    if (self = [super init]) {
        _chapters = [[RSNoteChapters alloc] init];
    }
    return self;
}

+ (instancetype)noteWithContent:(NSString *)fileContent {
    return [[RSNote alloc] initWithContent:fileContent];
}

- (NSArray *)_gen:(NSXMLElement *)tr {
    NSArray *tds = [tr elementsForName:@"td"];
    NSMutableArray *chapters = [[NSMutableArray alloc] init];
    for (NSXMLElement *td in tds) {
        NSXMLNode * xcss = [td attributeForName:@"class"];
        if ([[xcss objectValue] isEqualToString:@"vcss"]) {
            // 卷
            RSNoteChapter *chapter = [[RSNoteChapter alloc] initWithName:[td objectValue] subURLString:nil];
            [chapters addObject:chapter];
        } else if ([[xcss objectValue] isEqualToString:@"ccss"]) {
            // 章
            NSXMLElement *a = [[td elementsForName:@"a"] firstObject];
            if (!a) continue;
            NSXMLNode *href = [a attributeForName:@"href"];
            RSNoteChapter *chapter = [[RSNoteChapter alloc] initWithName:[a objectValue] subURLString:[href objectValue]];
            [chapters addObject:chapter];
        }
    }
    return chapters;
}

- (instancetype)initWithContent:(NSString *)fileContent {
    if (self = [self init]) {
        NSError *error = nil;
        _document = [[NSXMLDocument alloc] initWithXMLString:fileContent options:NSXMLDocumentTidyHTML error:&error];
        if (!_document) {
            NSLog(@"%@", error);
            return nil;
        }
        
        NSXMLElement *root = [_document rootElement];
        NSXMLElement *body = [[root elementsForName:@"body"] firstObject];
        NSArray *tables = [body elementsForName:@"table"];
        NSXMLElement *targetTable = tables[2];
        NSXMLElement *target = [[targetTable elementsForName:@"tbody"] firstObject];
        NSArray *trs = [target elementsForName:@"tr"];
        [self _parseTitle:[trs firstObject]];
        [self _parseAuth:trs[1]];
        [self _parseChapters:trs[3]];
    }
    return self;
}

- (BOOL)isValid {
    return nil != _document;
}

- (void)_parseTitle:(NSXMLElement *)element {
    _title = [element objectValue];
    NSLog(@"%@", _title);
}

- (void)_parseAuth:(NSXMLElement *)element {
    _auth = [element objectValue];
    NSLog(@"%@", _auth);
}

- (void)_parseChapters:(NSXMLElement *)element {
    NSArray *tables = [[[element elementsForName:@"td"] firstObject] elementsForName:@"table"];
    NSXMLElement *tableChapter = tables[1];
    NSXMLElement *tbody = [[tableChapter elementsForName:@"tbody"] firstObject];
    NSArray *trs = [tbody elementsForName:@"tr"];
    [trs enumerateObjectsUsingBlock:^(NSXMLElement *tr, NSUInteger idx, BOOL *stop) {
        NSArray *tds = [tr elementsForName:@"td"];
        [tds enumerateObjectsUsingBlock:^(NSXMLElement *td, NSUInteger idx, BOOL *stop) {
            NSXMLElement *a = [[td elementsForName:@"a"] firstObject];
            NSXMLNode *href = [a attributeForName:@"href"];
            if (href && a) {
                RSNoteChapter *chapter = [[RSNoteChapter alloc] initWithName:[a objectValue] subURLString:[href objectValue]];
                [[_chapters chapters] addObject:chapter];
            }
        }];
    }];
    NSLog(@"%@", _chapters);
}

- (NSString *)description {
    if (![self isValid]) return @"invalid note";
    NSString *desc = [[NSString alloc] initWithFormat:@"title -> %@, auth -> %@\n%@", _title, _auth, _chapters];
    return desc;
}
@end
