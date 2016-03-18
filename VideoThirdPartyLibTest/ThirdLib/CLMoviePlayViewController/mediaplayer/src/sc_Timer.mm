//
//  Sc_Timer.m
//  player
//
//  Created by liwu on 15/5/28.
//  Copyright (c) 2015年 liwu. All rights reserved.
//
#import "sc_Timer.h"

@implementation selfOBTimer

- (id)init
{
    self = [super init];
    return self;
}

- (void) timeStart:(pread_pcm_data) pcall span:(float) nSpanTime
{
    if (readTimer)
    {
        [readTimer invalidate];
        readTimer = nil;
    }
    
    pcallback = pcall;
    
    readTimer = [NSTimer scheduledTimerWithTimeInterval:0.01 target:self selector:@selector(readNextPCMData:) userInfo:nil repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:readTimer forMode:NSRunLoopCommonModes];
    
}

- (void) timeEnd
{
    if (readTimer)
    {
        [readTimer invalidate];
        readTimer = nil;
    }
}

- (void)readNextPCMData:(NSTimer*)timer
{
    if(pcallback)
    {
        pcallback();
    }
}
@end
