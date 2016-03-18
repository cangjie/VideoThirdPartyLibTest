//
//  Sc_Timer.m
//  player
//
//  Created by liwu on 15/5/28.
//  Copyright (c) 2015年 liwu. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef void(*pread_pcm_data)();

@interface selfOBTimer : NSObject
{
    pread_pcm_data  pcallback;
    NSTimer*  readTimer;
}

- (void)readNextPCMData:(NSTimer*)timer;
- (void) timeStart:(pread_pcm_data) pcall span:(float) nSpanTime;
- (void) timeEnd;

@end