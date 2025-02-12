#pragma once

#include <cstdint>

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
}

typedef struct omx_bitstream_ctx {
    uint8_t  length_size;
    uint8_t  first_idr;
    uint8_t  idr_sps_pps_seen;
    uint8_t *sps_pps_data;
    uint32_t size;
} omx_bitstream_ctx;

extern bool              m_to_annexb;
extern omx_bitstream_ctx m_sps_pps_context;
extern uint8_t          *m_convertBuffer;
extern int               m_convertSize;


bool CBitstreamConverter(uint8_t *pData, int iSize);
bool BitstreamConverterOpen(enum AVCodecID codec, uint8_t *in_extradata, int in_extrasize, bool to_annexb);
void BitstreamConverterInit();
void CBitstreamConverterClose(void);
