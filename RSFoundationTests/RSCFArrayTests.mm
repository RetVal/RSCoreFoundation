//
//  RSFoundationTests.m
//  RSFoundationTests
//
//  Created by closure on 5/26/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>
#include <RSFoundation/RSFoundation.h>

using namespace RSCF;

@interface RSFoundationTests : XCTestCase

@end

@implementation RSFoundationTests

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testArray {
    // This is an example of a functional test case.
    std::vector<String> vec;
    for (int i = 0; i < 10; i++) {
        String s = String::format("%d", i);
        vec.push_back(s);
    }
    Array<String> arr(vec);

    for (int i = 0; i < 10; i++) {
        String v1 = arr[i];
        String v2 = String::format("%d", i);
        XCTAssertTrue(v1 == v2);
    }
    
    
    XCTAssert(YES, @"Pass");
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
