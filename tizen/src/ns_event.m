#import <Cocoa/Cocoa.h>                                         
#import "ns_event.h"

// ns event loop for receive the events from cocoa framework
void ns_event_loop(int* keepRunning)
{
    NSDate* distantFuture;    

    NSRunLoop* theRunLoop = [NSRunLoop currentRunLoop];
    do {
         distantFuture = [NSDate dateWithTimeIntervalSinceNow:0.5];
    }
    while (*keepRunning && [theRunLoop runMode:NSDefaultRunLoopMode beforeDate:distantFuture]);    
    // return [[NSRunLoop currentRunLoop] runUntilDate: [NSDate dateWithTimeIntervalSinceNow: 1]];
}
