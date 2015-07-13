//
//  Protocol.cpp
//  RSCoreFoundation
//
//  Created by closure on 5/27/15.
//  Copyright (c) 2015 RetVal. All rights reserved.
//

#include <RSFoundation/Protocol.hpp>

namespace RSFoundation {
    namespace Basic {
//        NotCopyable::NotCopyable() {
//        }
//        
//        NotCopyable::NotCopyable(const NotCopyable &) {
//        }
//        
//        NotCopyable& NotCopyable::operator=(const NotCopyable &) {
//            return *this;
//        }
//        
//        NotCopyable::~NotCopyable() {
//            
//        }
    }
}

/*
while (true) {
    Call kCFRunLoopBeforeTimers observer callbacks;
    Call kCFRunLoopBeforeSources observer callbacks;
    Perform blocks queued by CFRunLoopPerformBlock;
    Call the callback of each version 0 CFRunLoopSource that has been signalled;
    if (any version 0 source callbacks were called) {
        Perform blocks newly queued by CFRunLoopPerformBlock;
    }
    if (I didn't drain the main queue on the last iteration
        AND the main queue has any blocks waiting)
    {
        while (main queue has blocks) {
            perform the next block on the main queue
        }
    } else {
        Call kCFRunLoopBeforeWaiting observer callbacks;
        Wait for a CFRunLoopSource to be signalled
            OR for a timer to fire
                OR for a block to be added to the main queue;
        Call kCFRunLoopAfterWaiting observer callbacks;
        if (the event was a timer) {
            call CFRunLoopTimer callbacks for timers that should have fired by now
        } else if (event was a block arriving on the main queue) {
            while (main queue has blocks) {
                perform the next block on the main queue
            }
        } else {
            look up the version 1 CFRunLoopSource for the event
            if (I found a version 1 source) {
                call the source's callback
            }
        }
    }
    Perform blocks queued by CFRunLoopPerformBlock;
}
 */