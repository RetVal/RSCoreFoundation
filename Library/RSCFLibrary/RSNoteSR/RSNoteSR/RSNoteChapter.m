//
//  RSNoteChapter.m
//  RSNoteSR
//
//  Created by closure on 9/30/14.
//  Copyright (c) 2014 closure. All rights reserved.
//

#import "RSNoteChapter.h"

@interface RSNoteChapter () {
    @private
    NSString *_content;
}

@end

@implementation RSNoteChapter
+ (instancetype)chapterWithName:(NSString *)name subURLString:(NSString *)subURLString {
    return [[RSNoteChapter alloc] initWithName:name subURLString:subURLString];
}

- (instancetype)initWithName:(NSString *)name subURLString:(NSString *)subURLString {
    if (self = [self init]) {
        _name = name;
        _subURLString = subURLString;
    }
    return self;
}

- (NSString *)description {
    if (_subURLString == nil) return [NSString stringWithFormat:@"%@", _name];
    return [NSString stringWithFormat:@"%@, %@", _name, _subURLString];
}

- (void)parseContent:(NSString *)content handler:(void (^)(NSXMLElement *element))handler {
    NSError *error = nil;
    NSXMLDocument *document = [[NSXMLDocument alloc] initWithXMLString:content options:NSXMLDocumentTidyHTML error:&error];
    if (!document) {
        _content = content;
        return;
    }
    
    NSXMLElement *root = [document rootElement];
    NSArray *divs = [[[root elementsForName:@"body"] firstObject] elementsForName:@"div"];
    for (NSXMLElement *rootDiv in divs) {
        NSXMLNode *IDAttribute = [rootDiv attributeForName:@"id"];
        if (!IDAttribute || (![[IDAttribute objectValue] isEqualToString:@"contentmain"])) {
            continue;
        }
        NSArray *divs = [rootDiv elementsForName:@"div"];
        if (!divs) continue;
        for (NSXMLElement *div in divs) {
            NSXMLNode *IDAttirbute = [div attributeForName:@"id"];
            if (!IDAttirbute || (![[IDAttirbute objectValue] isEqualToString:@"content"])) {
                continue;
            }
            if (handler) {
                handler(div);
            }
        }
    }
}

- (void)setContent:(NSString *)content {
    [self parseContent:content handler:^(NSXMLElement *element) {
        _content = [[element objectValue] stringByReplacingOccurrencesOfString:@"    " withString:@""];
    }];
}
@end
