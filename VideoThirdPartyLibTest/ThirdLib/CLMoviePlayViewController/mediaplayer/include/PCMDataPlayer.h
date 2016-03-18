//
//  PCMDataPlayer.h
//  PCMDataPlayerDemo
//
//  Created by Android88 on 15-2-10.
//  Copyright (c) 2015年 Android88. All rights reserved.

#import <Foundation/Foundation.h>
#import <AudioToolbox/AudioToolbox.h>


#define QUEUE_BUFFER_SIZE 20     //队列缓冲个数
#define MIN_SIZE_PER_FRAME 5120 * 4 //每帧最小数据长度

@interface PCMDataPlayer : NSObject
{
    AudioStreamBasicDescription audioDescription; ///音频参数
    AudioQueueRef audioQueue; //音频播放队列
    AudioQueueBufferRef audioQueueBuffers[QUEUE_BUFFER_SIZE]; //音频缓存
    BOOL audioQueueUsed[QUEUE_BUFFER_SIZE];

    NSLock* sysnLock;
}

+ (id)sharePCMDataPlayer;
- (void)reset:(int)samplerate channel:(int)nchannel;
- (void)stop;
- (void)play:(void*)pcmData length:(unsigned int)length samp:(int)samplerate channel:(int)nchannel;
@end
