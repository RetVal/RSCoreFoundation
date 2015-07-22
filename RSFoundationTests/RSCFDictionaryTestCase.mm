//
//  RSCFDictionaryTestCase.m
//  RSCoreFoundation
//
//  Created by closure on 7/22/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>
#include <RSFoundation/RSFoundation.h>

using namespace RSCF;

@interface RSCFDictionaryTestCase : XCTestCase

@end

@implementation RSCFDictionaryTestCase

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testDictionary {
    Dictionary<String, String> dict;
    dict["321"] = "123";
    
    String &val = dict["321"];
    XCTAssertTrue(val == "123");
    XCTAssertFalse(val == "122");
    
    auto copy = dict;
    XCTAssertTrue(copy["321"] == val);
    
    // This is an example of a functional test case.
    XCTAssert(YES, @"Pass");
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
