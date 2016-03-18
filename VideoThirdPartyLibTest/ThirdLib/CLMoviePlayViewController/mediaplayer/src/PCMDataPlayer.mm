//
//  PCMDataPlayer.m
//  PCMDataPlayerDemo
//
//  Created by Android88 on 15-2-10.
//  Copyright (c) 2015年 Android88. All rights reserved.
//

#import "PCMDataPlayer.h"
#import <AVFoundation/AVFoundation.h>
static PCMDataPlayer *pcmPlayer = nil;
@implementation PCMDataPlayer

+ (id)sharePCMDataPlayer {
    static dispatch_once_t onceToken;
    dispatch_once(&onceToken, ^{
        pcmPlayer = [[PCMDataPlayer alloc] init];
    });
    return pcmPlayer;
}

- (id)init
{
    self = [super init];
    if (self) {
        AVAudioSession *audioSession = [AVAudioSession sharedInstance];
        [audioSession setCategory:AVAudioSessionCategoryPlayAndRecord error:nil];

    }
    return self;
}

- (void)dealloc
{
    printf("dealloc audioQueue !\n ");
    if (audioQueue != nil)
    {
        AudioQueueStop(audioQueue, true);
    }
    audioQueue = nil;

    sysnLock = nil;
}

static void AudioPlayerAQInputCallback(void* inUserData, AudioQueueRef outQ, AudioQueueBufferRef outQB)
{
    PCMDataPlayer* player = (__bridge PCMDataPlayer*)inUserData;
    [player playerCallback:outQB];
}

- (void)reset:(int)samplerate channel:(int)nchannel
{
   [self stop];
    
    sysnLock = [[NSLock alloc] init];

    ///设置音频参数
    audioDescription.mSampleRate = samplerate; //采样率
    audioDescription.mFormatID = kAudioFormatLinearPCM;
    audioDescription.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked;
    audioDescription.mChannelsPerFrame = nchannel; ///单声道
    audioDescription.mFramesPerPacket = 1; //每一个packet一侦数据
    audioDescription.mBitsPerChannel = 16; //每个采样点16bit量化
    audioDescription.mBytesPerFrame = (audioDescription.mBitsPerChannel / 8) * audioDescription.mChannelsPerFrame;
    audioDescription.mBytesPerPacket = audioDescription.mBytesPerFrame;

    AudioQueueNewOutput(&audioDescription, AudioPlayerAQInputCallback, (__bridge void*)self, nil, nil, 0, &audioQueue); //使用player的内部线程播放
    
    //初始化音频缓冲区
    for (int i = 0; i < QUEUE_BUFFER_SIZE; i++)
    {
        int result = AudioQueueAllocateBuffer(audioQueue, MIN_SIZE_PER_FRAME, &audioQueueBuffers[i]);
        audioQueueUsed[i] = NO;
    }
}

- (void)stop
{
    if (audioQueue != nil)
    {
        AudioQueueStop(audioQueue, true);
        AudioQueueReset(audioQueue);
    }
    audioQueue = nil;
}

- (void)play:(void*)pcmData length:(unsigned int)length samp:(int)samplerate channel:(int)nchannel
{

    ShowLog(@"音频 size %d  samplerate %d  channel %d\n",length,samplerate,nchannel);
   
    if (audioQueue == nil || ![self checkBufferHasUsed])
    {
        [self reset: samplerate  channel: nchannel];
        AudioQueueStart(audioQueue, NULL);
    }
    [sysnLock lock];
    
    AudioQueueBufferRef audioQueueBuffer = NULL;

    while (true)
    {
        audioQueueBuffer = [self getNotUsedBuffer];
        if (audioQueueBuffer != NULL)
        {
            break;
        }
    }

    audioQueueBuffer->mAudioDataByteSize = length;
    Byte* audiodata = (Byte*)audioQueueBuffer->mAudioData;
    /*for (int i = 0; i < length; i++)
    {
        audiodata[i] = ((Byte*)pcmData)[i];
    }*/
    memcpy(audiodata,pcmData,length);

    AudioQueueEnqueueBuffer(audioQueue, audioQueueBuffer, 0, NULL);

    [sysnLock unlock];
}

- (BOOL)checkBufferHasUsed
{
    for (int i = 0; i < QUEUE_BUFFER_SIZE; i++)
    {
        if (YES == audioQueueUsed[i])
        {
            return YES;
        }
    }
    return NO;
}

- (AudioQueueBufferRef)getNotUsedBuffer
{
    for (int i = 0; i < QUEUE_BUFFER_SIZE; i++)
    {
        if (NO == audioQueueUsed[i])
        {
            audioQueueUsed[i] = YES;
            return audioQueueBuffers[i];
        }
    }
    return NULL;
}

- (void)playerCallback:(AudioQueueBufferRef)outQB
{
   // [sysnLock lock];
    
    for (int i = 0; i < QUEUE_BUFFER_SIZE; i++)
    {
        if (outQB == audioQueueBuffers[i])
        {
            audioQueueUsed[i] = NO;
        }
    }
  //  [sysnLock unlock];
}

@end
