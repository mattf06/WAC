//Windows Audio Capture (WAC) by KwstasG (Kostas Giannakakis)
#pragma once

#ifndef NTDDI_THRESHOLD
#define NTDDI_THRESHOLD NTDDI_VERSION
#endif

typedef unsigned char BYTE;
class IAudioSink {
public:
	virtual ~IAudioSink() {}
	virtual int CopyData(const BYTE* Data, const int NumFramesAvailable) = 0;
};
