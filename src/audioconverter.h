

// Sampling Frequency Index table, from ISO 14496-3 Table 1.16
static const uint32_t kSampleRates[] = {96000, 88200, 64000, 48000, 44100,
                                        32000, 24000, 22050, 16000, 12000,
                                        11025, 8000,  7350};


/// Size in bytes of the ADTS header added by ConvertEsdsToADTS().
static const size_t kADTSHeaderSize = 7;

int audio_object_type;
int frequency_index;
int channel_config;

enum {
  CodecProfileMAIN = 27,
  CodecProfileLOW,
  CodecProfileSSR,
  CodecProfileLTP
};

int ConvertAudioCodecProfile(int profile)
{

  printf("Audio Profile %d\n",profile);
  switch (profile)
  {
    case 0:
      return 2;
    case CodecProfileMAIN:
      return 1;
    case CodecProfileLOW:
      return 2;
    case CodecProfileSSR:
      return 3;
    case CodecProfileLTP:
      return 4;

#if 0
    case AACCodecProfileHE:
      return FF_PROFILE;
    case AACCodecProfileHEV2:
      return FF_PROFILE_AAC_HE_V2;
    case AACCodecProfileLD:
      return FF_PROFILE_AAC_LD;
    case AACCodecProfileELD:
      return FF_PROFILE_AAC_ELD;
    case MPEG2AACCodecProfileLOW:
      return FF_PROFILE_MPEG2_AAC_LOW;
    case MPEG2AACCodecProfileHE:
      return FF_PROFILE_MPEG2_AAC_HE;
    case DTSCodecProfile:
      return FF_PROFILE_DTS;
    case DTSCodecProfileES:
      return FF_PROFILE_DTS_ES;
    case DTSCodecProfile9624:
      return FF_PROFILE_DTS_96_24;
    case DTSCodecProfileHDHRA:
      return FF_PROFILE_DTS_HD_HRA;
    case DTSCodecProfileHDMA:
      return FF_PROFILE_DTS_HD_MA;
    case DTSCodecProfileHDExpress:
      return FF_PROFILE_DTS_EXPRESS;
    case DTSCodecProfileHDMAX:
      //! @todo: with ffmpeg >= 6.1 set the appropriate profile
      return FF_PROFILE_UNKNOWN; // FF_PROFILE_DTS_HD_MA_X
    case DTSCodecProfileHDMAIMAX:
      //! @todo: with ffmpeg >= 6.1 set the appropriate profile
      return FF_PROFILE_UNKNOWN; // FF_PROFILE_DTS_HD_MA_X_IMAX
    case DDPlusCodecProfileAtmos:
      //! @todo: with ffmpeg >= 6.1 set the appropriate profile
      return FF_PROFILE_UNKNOWN; // FF_PROFILE_EAC3_DDP_ATMOS
#endif
    default:
      printf("Unknown Audio Profile %d\n",profile);
      return 2;
  }
}

int ConvertRate(uint32_t rate) {
    printf("Audio Rate %d\n",rate);
    
    for (int i=0;i<13;i++) {
        if ( kSampleRates[i] == rate) {
            return i;
        }
    }
    printf("Unknown Audio Rate %d\n",rate);
    return 3;
}

bool AudioConverterOpen(INPUTSTREAM_INFO* stream) {
    audio_object_type = ConvertAudioCodecProfile((int)stream->m_codecProfile);
    frequency_index = ConvertRate((int)stream->m_SampleRate);
    channel_config = stream->m_Channels == 8 ? 7 : stream->m_Channels;
    printf("Audio Channels %d\n",channel_config);
    return true;
}

bool ConvertToADTS(
    const uint8_t* data,
    size_t data_size, unsigned char *audio_frame) {
    
  size_t size = kADTSHeaderSize + data_size;

  // ADTS header uses 13 bits for packet size.
  if (size >= (1 << 13))
    return false;

  audio_frame[0] = 0xff;
  audio_frame[1] = 0xf1;
  audio_frame[2] = ((audio_object_type - 1) << 6) +
                       (frequency_index << 2) + (channel_config >> 2);
  audio_frame[3] =
      ((channel_config & 0x3) << 6) + static_cast<uint8_t>(size >> 11);
  audio_frame[4] = static_cast<uint8_t>((size & 0x7ff) >> 3);
  audio_frame[5] = static_cast<uint8_t>(((size & 7) << 5) + 0x1f);
  audio_frame[6] = 0xfc;

  
  return true;
}