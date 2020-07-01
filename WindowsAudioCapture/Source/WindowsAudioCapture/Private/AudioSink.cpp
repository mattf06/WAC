//Windows Audio Capture (WAC) by KwstasG (Kostas Giannakakis)
#include "AudioSink.h"
#include "WindowsAudioCapture.h"

AudioSink::AudioSink()
{
}

AudioSink::~AudioSink()
{
    AudioChunk tmp;
    EmptyQueue(tmp);
}

bool AudioSink::Dequeue(AudioChunk& Chunk)
{
    FScopeLock lock(&m_mutex);

    if (m_queue.IsEmpty())
        return false;

    m_queue.Dequeue(Chunk);
    return true;
}

bool AudioSink::EmptyQueue(AudioChunk& Chunk)
{
    //std::lock_guard<std::mutex> lock(m_mutex);

    if (Chunk.chunk != nullptr) {
        delete[] Chunk.chunk;
        Chunk.chunk = nullptr;
    }

    while (!m_queue.IsEmpty()) {
        m_queue.Dequeue(Chunk);
        if (Chunk.chunk != nullptr) {
            delete[] Chunk.chunk;
            Chunk.chunk = nullptr;
        }
    }

    return true;
}

int AudioSink::CopyData(const BYTE* Data, const int NumFramesAvailable)
{
    FScopeLock lock(&m_mutex);

    AudioChunk chunk;
    if (Data == NULL) {
        chunk.size = 0;
        m_queue.Enqueue(chunk);
        return 0;
    }
    chunk.size = NumFramesAvailable * 2;
    chunk.chunk = new int16[chunk.size];
    const int multiplier = sizeof(int16) / sizeof(unsigned char);
    //std::memcpy(chunk.chunk, Data, chunk.size * multiplier);
    FMemory::Memcpy(chunk.chunk, Data, chunk.size * multiplier);
    bool nonZero = false;

    for (int i = 0; i < chunk.size; i++) {
        if (chunk.chunk[i] == -1 || chunk.chunk[i] == 1) {
            chunk.chunk[i] = 0;
        }
        if (chunk.chunk[i] != 0) {
            nonZero = true;
            //UE_LOG(LogTemp, Log, TEXT("NumFramesAvailable: %d, Sample number %d, Sample value %hi"), NumFramesAvailable, i, chunk.chunk[i]);
        }
    }

    if (nonZero) {
        m_queue.Enqueue(chunk);
    }

    return 0;
}
