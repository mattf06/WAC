//Windows Audio Capture (WAC) by KwstasG (Kostas Giannakakis)
#pragma once

#include "IAudioSink.h"
#include <mutex>
#include <queue>

struct AudioChunk {
    int16* chunk = nullptr;
    int size = 0;
};

class AudioSink : public IAudioSink {
public:
    bool Dequeue(AudioChunk& Chunk);
    bool EmptyQueue(AudioChunk& Chunk);
    int CopyData(const BYTE* Data, const int NumFramesAvailable) override;
    AudioSink();
    ~AudioSink();

private:
    TQueue<AudioChunk> m_queue;
    int m_bitDepth;
    int m_maxSampleVal;
    int m_nChannels;
    int m_chunkSize;
    FCriticalSection m_mutex;
};
