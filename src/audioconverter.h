#pragma once

#include "kodi/c-api/addon-instance/inputstream.h"

enum {
    CodecProfileMAIN = 27,
    CodecProfileLOW,
    CodecProfileSSR,
    CodecProfileLTP
};

int ConvertAudioCodecProfile(int profile);
int ConvertRate(uint32_t rate);
bool AudioConverterOpen(INPUTSTREAM_INFO* stream);
bool ConvertToADTS(const uint8_t* data, size_t data_size, unsigned char *audio_frame);
