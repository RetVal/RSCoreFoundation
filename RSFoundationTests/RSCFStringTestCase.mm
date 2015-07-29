//
//  RSCFStringTestCase.m
//  
//
//  Created by closure on 7/19/15.
//
//

#import <Cocoa/Cocoa.h>
#import <XCTest/XCTest.h>

#include <RSFoundation/RSFoundation.h>

#include <typeinfo>

using namespace RSCF;

@interface RSCFStringTestCase : XCTestCase

@end

@implementation RSCFStringTestCase

- (void)setUp {
    [super setUp];
    // Put setup code here. This method is called before the invocation of each test method in the class.
}

- (void)tearDown {
    // Put teardown code here. This method is called after the invocation of each test method in the class.
    [super tearDown];
}

- (void)testString {
    String s1;
    String s2("");
    String s3("s3");
    String s4 = "s4";
    String s5(s1);
    String s6 = s4;
    String s7 = String("s7");
    String s8 = s7.subString(RSMakeRange(0, 1));
    
    XCTAssertTrue(s2.getLength() == 0);
    XCTAssertTrue(s3.getLength() == 2);
    XCTAssertTrue(s3 == "s3");
    XCTAssertTrue(s4 == "s4");
    XCTAssertTrue(s5 == s1);
    XCTAssertTrue(s6);
    
    RSRange rangeResult;
    s7.find("s7", s7.getRange(), rangeResult);
    
    XCTAssertTrue(rangeResult.location == s7.getRange().location);
    XCTAssertTrue(rangeResult.length == s7.getRange().length);
    
    const String cs1;
    const String cs2("");
    const String cs3("cs3");
    const String cs4 = "cs4";
    const String cs5(cs1);
    const String cs6 = cs4;
    const String cs7 = String("cs7");
    const String cs8 = cs7.subString(RSMakeRange(0, 1));
    
    XCTAssertTrue(cs2.getLength() == 0);
    XCTAssertTrue(cs3.getLength() == 3);
    XCTAssertTrue(cs3 == "cs3");
    XCTAssertTrue(cs4 == "cs4");
    XCTAssertTrue(cs5 == s1);
    XCTAssertTrue(cs6);

    cs7.find("cs7", cs7.getRange(), rangeResult);
    
    XCTAssertTrue(rangeResult.location == cs7.getRange().location);
    XCTAssertTrue(rangeResult.length == cs7.getRange().length);
    
    StringI i2;
    if (1) {
        StringI i1 = "123";
        i2 = i1;
    }
    
    XCTAssert(YES, @"Pass");
}

- (void)testStringM {
    String s1 = "s1";
    const String cs1 = "cs1";
    
    StringM sm1;
    StringM sm2(cs1);
    StringM sm3(s1);
    
    StringM sm4 = cs1;
    StringM sm5 = s1;
    StringM sm6(String("sm6"));
    StringM sm7("sm7", RSMakeRange(0, 3));
    
}

- (void)testPerformanceExample {
    // This is an example of a performance test case.
    [self measureBlock:^{
        // Put the code you want to measure the time of here.
    }];
}

@end
