
extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
}



#define BS_WB32(p, d) { \
  ((uint8_t*)(p))[3] = (d); \
  ((uint8_t*)(p))[2] = (d) >> 8; \
  ((uint8_t*)(p))[1] = (d) >> 16; \
  ((uint8_t*)(p))[0] = (d) >> 24; }

#define BS_WL32(p, d) { \
  ((uint8_t*)(p))[0] = (d); \
  ((uint8_t*)(p))[1] = (d) >> 8; \
  ((uint8_t*)(p))[2] = (d) >> 16; \
  ((uint8_t*)(p))[3] = (d) >> 24; }

enum {
  AVC_NAL_SLICE=1,
  AVC_NAL_DPA,
  AVC_NAL_DPB,
  AVC_NAL_DPC,
  AVC_NAL_IDR_SLICE,
  AVC_NAL_SEI,
  AVC_NAL_SPS,
  AVC_NAL_PPS,
  AVC_NAL_AUD,
  AVC_NAL_END_SEQUENCE,
  AVC_NAL_END_STREAM,
  AVC_NAL_FILLER_DATA,
  AVC_NAL_SPS_EXT,
  AVC_NAL_AUXILIARY_SLICE=19
};

enum
{
  HEVC_NAL_TRAIL_N = 0,
  HEVC_NAL_TRAIL_R = 1,
  HEVC_NAL_TSA_N = 2,
  HEVC_NAL_TSA_R = 3,
  HEVC_NAL_STSA_N = 4,
  HEVC_NAL_STSA_R = 5,
  HEVC_NAL_RADL_N = 6,
  HEVC_NAL_RADL_R = 7,
  HEVC_NAL_RASL_N = 8,
  HEVC_NAL_RASL_R = 9,
  HEVC_NAL_BLA_W_LP = 16,
  HEVC_NAL_BLA_W_RADL = 17,
  HEVC_NAL_BLA_N_LP = 18,
  HEVC_NAL_IDR_W_RADL = 19,
  HEVC_NAL_IDR_N_LP = 20,
  HEVC_NAL_CRA_NUT = 21,
  HEVC_NAL_VPS = 32,
  HEVC_NAL_SPS = 33,
  HEVC_NAL_PPS = 34,
  HEVC_NAL_AUD = 35,
  HEVC_NAL_EOS_NUT = 36,
  HEVC_NAL_EOB_NUT = 37,
  HEVC_NAL_FD_NUT = 38,
  HEVC_NAL_SEI_PREFIX = 39,
  HEVC_NAL_SEI_SUFFIX = 40,
  HEVC_NAL_UNSPEC62 = 62, // Dolby Vision RPU
  HEVC_NAL_UNSPEC63 = 63 // Dolby Vision EL
};

enum {
  SEI_BUFFERING_PERIOD = 0,
  SEI_PIC_TIMING,
  SEI_PAN_SCAN_RECT,
  SEI_FILLER_PAYLOAD,
  SEI_USER_DATA_REGISTERED_ITU_T_T35,
  SEI_USER_DATA_UNREGISTERED,
  SEI_RECOVERY_POINT,
  SEI_DEC_REF_PIC_MARKING_REPETITION,
  SEI_SPARE_PIC,
  SEI_SCENE_INFO,
  SEI_SUB_SEQ_INFO,
  SEI_SUB_SEQ_LAYER_CHARACTERISTICS,
  SEI_SUB_SEQ_CHARACTERISTICS,
  SEI_FULL_FRAME_FREEZE,
  SEI_FULL_FRAME_FREEZE_RELEASE,
  SEI_FULL_FRAME_SNAPSHOT,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_START,
  SEI_PROGRESSIVE_REFINEMENT_SEGMENT_END,
  SEI_MOTION_CONSTRAINED_SLICE_GROUP_SET,
  SEI_FILM_GRAIN_CHARACTERISTICS,
  SEI_DEBLOCKING_FILTER_DISPLAY_PREFERENCE,
  SEI_STEREO_VIDEO_INFO,
  SEI_POST_FILTER_HINTS,
  SEI_TONE_MAPPING
};

typedef struct
{
  const uint8_t *data;
  const uint8_t *end;
  int head;
  uint64_t cache;
} nal_bitstream;

typedef struct mpeg2_sequence
{
  uint32_t  width;
  uint32_t  height;
  uint32_t  fps_rate;
  uint32_t  fps_scale;
  float     ratio;
  uint32_t  ratio_info;
} mpeg2_sequence;

typedef struct h264_sequence
{
  uint32_t  width;
  uint32_t  height;
  float     ratio;
  uint32_t  ratio_info;
} h264_sequence;

typedef struct
{
  int profile_idc;
  int level_idc;
  int sps_id;

  int chroma_format_idc;
  int separate_colour_plane_flag;
  int bit_depth_luma_minus8;
  int bit_depth_chroma_minus8;
  int qpprime_y_zero_transform_bypass_flag;
  int seq_scaling_matrix_present_flag;

  int log2_max_frame_num_minus4;
  int pic_order_cnt_type;
  int log2_max_pic_order_cnt_lsb_minus4;

  int max_num_ref_frames;
  int gaps_in_frame_num_value_allowed_flag;
  int pic_width_in_mbs_minus1;
  int pic_height_in_map_units_minus1;

  int frame_mbs_only_flag;
  int mb_adaptive_frame_field_flag;

  int direct_8x8_inference_flag;

  int frame_cropping_flag;
  int frame_crop_left_offset;
  int frame_crop_right_offset;
  int frame_crop_top_offset;
  int frame_crop_bottom_offset;
} sps_info_struct;

enum ELType : int
{
  TYPE_NONE = 0,
  TYPE_FEL,
  TYPE_MEL
};

typedef struct omx_bitstream_ctx {
      uint8_t  length_size;
      uint8_t  first_idr;
      uint8_t  idr_sps_pps_seen;
      uint8_t *sps_pps_data;
      uint32_t size;
  } omx_bitstream_ctx;

  uint8_t          *m_convertBuffer;
  int               m_convertSize;
  uint8_t          *m_inputBuffer;
  int               m_inputSize;

  uint32_t          m_sps_pps_size;
  omx_bitstream_ctx m_sps_pps_context;
  bool              m_convert_bitstream;
  bool              m_to_annexb;
  bool              m_combine;

  uint8_t*          m_extraData;
  bool              m_convert_3byteTo4byteNALSize;
  bool              m_convert_bytestream;
  AVCodecID         m_codec;
  bool              m_start_decode;
  int               m_convert_dovi;
  bool              m_removeDovi;
  bool              m_removeHdr10Plus;
  enum ELType       m_dovi_el_type;

uint8_t * FFmpegExtraData (const uint8_t* data, size_t size) {
    uint8_t *m_data = (uint8_t*)av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE);
    memcpy(m_data, data, size);
    return m_data;
}
/*
 *  GStreamer h264 parser
 *  Copyright (C) 2005 Michal Benes <michal.benes@itonis.tv>
 *            (C) 2008 Wim Taymans <wim.taymans@gmail.com>
 *  gsth264parse.c
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

constexpr uint32_t BS_RB24(const uint8_t* x)
{
  return (x[0] << 16) | (x[1] << 8) | x[2];
}

constexpr uint32_t BS_RB32(const uint8_t* x)
{
  return (x[0] << 24) | (x[1] << 16) | (x[2] << 8) | x[3];
}

static void nal_bs_init(nal_bitstream *bs, const uint8_t *data, size_t size)
{
  bs->data = data;
  bs->end  = data + size;
  bs->head = 0;
  // fill with something other than 0 to detect
  //  emulation prevention bytes
  bs->cache = 0xffffffff;
}

static uint32_t nal_bs_read(nal_bitstream *bs, int n)
{
  uint32_t res = 0;
  int shift;

  if (n == 0)
    return res;

  // fill up the cache if we need to
  while (bs->head < n)
  {
    uint8_t a_byte;
    bool check_three_byte;

    check_three_byte = true;
next_byte:
    if (bs->data >= bs->end)
    {
      // we're at the end, can't produce more than head number of bits
      n = bs->head;
      break;
    }
    // get the byte, this can be an emulation_prevention_three_byte that we need
    // to ignore.
    a_byte = *bs->data++;
    if (check_three_byte && a_byte == 0x03 && ((bs->cache & 0xffff) == 0))
    {
      // next byte goes unconditionally to the cache, even if it's 0x03
      check_three_byte = false;
      goto next_byte;
    }
    // shift bytes in cache, moving the head bits of the cache left
    bs->cache = (bs->cache << 8) | a_byte;
    bs->head += 8;
  }

  // bring the required bits down and truncate
  if ((shift = bs->head - n) > 0)
    res = static_cast<uint32_t>(bs->cache >> shift);
  else
    res = static_cast<uint32_t>(bs->cache);

  // mask out required bits
  if (n < 32)
    res &= (1 << n) - 1;
  bs->head = shift;

  return res;
}

static bool nal_bs_eos(nal_bitstream *bs)
{
  return (bs->data >= bs->end) && (bs->head == 0);
}

// read unsigned Exp-Golomb code
static int nal_bs_read_ue(nal_bitstream *bs)
{
  int i = 0;

  while (nal_bs_read(bs, 1) == 0 && !nal_bs_eos(bs) && i < 31)
    i++;

  return ((1 << i) - 1 + nal_bs_read(bs, i));
}

#if 0
// read signed Exp-Golomb code
static int nal_bs_read_se(nal_bitstream *bs)
{
  int i = 0;

  i = nal_bs_read_ue (bs);
  /* (-1)^(i+1) Ceil (i / 2) */
  i = (i + 1) / 2 * (i & 1 ? 1 : -1);

  return i;
}
#endif

static bool has_sei_recovery_point(const uint8_t *p, const uint8_t *end)
{
  int pt(0), ps(0), offset(1);

  do
  {
    pt = 0;
    do {
      pt += p[offset];
    } while (p[offset++] == 0xFF);

    ps = 0;
    do {
      ps += p[offset];
    } while (p[offset++] == 0xFF);

    if (pt == SEI_RECOVERY_POINT)
    {
      nal_bitstream bs;
      nal_bs_init(&bs, p + offset, ps);
      return nal_bs_read_ue(&bs) >= 0;
    }
    offset += ps;
  } while (p + offset < end && p[offset] != 0x80);

  return false;
}



static const uint8_t* avc_find_startcode_internal(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *a = p + 4 - ((intptr_t)p & 3);

  for (end -= 3; p < a && p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  for (end -= 3; p < end; p += 4)
  {
    uint32_t x = *(const uint32_t*)p;
    if ((x - 0x01010101) & (~x) & 0x80808080) // generic
    {
      if (p[1] == 0)
      {
        if (p[0] == 0 && p[2] == 1)
          return p;
        if (p[2] == 0 && p[3] == 1)
          return p+1;
      }
      if (p[3] == 0)
      {
        if (p[2] == 0 && p[4] == 1)
          return p+2;
        if (p[4] == 0 && p[5] == 1)
          return p+3;
      }
    }
  }

  for (end += 3; p < end; p++)
  {
    if (p[0] == 0 && p[1] == 0 && p[2] == 1)
      return p;
  }

  return end + 3;
}

static const uint8_t* avc_find_startcode(const uint8_t *p, const uint8_t *end)
{
  const uint8_t *out = avc_find_startcode_internal(p, end);
  if (p<out && out<end && !out[-1])
    out--;
  return out;
}

int avc_parse_nal_units(AVIOContext *pb, const uint8_t *buf_in, int size)
{
  const uint8_t *p = buf_in;
  const uint8_t *end = p + size;
  const uint8_t *nal_start, *nal_end;

  size = 0;
  nal_start = avc_find_startcode(p, end);

  for (;;) {
    while (nal_start < end && !*(nal_start++));
    if (nal_start == end)
      break;

    nal_end = avc_find_startcode(nal_start, end);
    avio_wb32(pb, nal_end - nal_start);
    avio_write(pb, nal_start, nal_end - nal_start);
    size += 4 + nal_end - nal_start;
    nal_start = nal_end;
  }
  return size;
}

void BitstreamAllocAndCopy7(uint8_t** poutbuf,
                                                int* poutbuf_size,
                                                const uint8_t* sps_pps,
                                                uint32_t sps_pps_size,
                                                const uint8_t* in,
                                                uint32_t in_size,
                                                uint8_t nal_type)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;
  void *tmp;

  // According to x265, this type is always encoded with four-sized header
  // https://bitbucket.org/multicoreware/x265_git/src/4bf31dc15fb6d1f93d12ecf21fad5e695f0db5c0/source/encoder/nal.cpp#lines-100
  if (nal_type == HEVC_NAL_UNSPEC62)
    nal_header_size = 4;

  *poutbuf_size += sps_pps_size + in_size + nal_header_size;
  tmp = av_realloc(*poutbuf, *poutbuf_size);
  if (!tmp)
    return;
  *poutbuf = (uint8_t*)tmp;
  if (sps_pps)
    memcpy(*poutbuf + offset, sps_pps, sps_pps_size);

  memcpy(*poutbuf + sps_pps_size + nal_header_size + offset, in, in_size);
  if (!offset)
  {
    BS_WB32(*poutbuf + sps_pps_size, 1);
  }
  else if (nal_header_size == 4)
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 0;
    (*poutbuf + offset + sps_pps_size)[3] = 1;
  }
  else
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 1;
  }
}
bool BitstreamConvertInitAVC(void *in_extradata, int in_extrasize)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  m_sps_pps_size = 0;
  m_sps_pps_context.sps_pps_data = NULL;

  // nothing to filter
  if (!in_extradata || in_extrasize < 6)
    return false;

  uint16_t unit_size;
  uint32_t total_size = 0;
  uint8_t *out = NULL, unit_nb, sps_done = 0, sps_seen = 0, pps_seen = 0;
  const uint8_t *extradata = (uint8_t*)in_extradata + 4;
  static const uint8_t nalu_header[4] = {0, 0, 0, 1};

  // retrieve length coded size
  m_sps_pps_context.length_size = (*extradata++ & 0x3) + 1;

  // retrieve sps and pps unit(s)
  unit_nb = *extradata++ & 0x1f;  // number of sps unit(s)
  
  if (!unit_nb)
  {
    goto pps;
  }
  else
  {
    sps_seen = 1;
  }

  while (unit_nb--)
  {
    void *tmp;

    unit_size = extradata[0] << 8 | extradata[1];
    total_size += unit_size + 4;

    if (total_size > INT_MAX - AV_INPUT_BUFFER_PADDING_SIZE ||
      (extradata + 2 + unit_size) > ((uint8_t*)in_extradata + in_extrasize))
    {
      av_free(out);
      return false;
    }
    tmp = av_realloc(out, total_size + AV_INPUT_BUFFER_PADDING_SIZE);
    if (!tmp)
    {
      av_free(out);
      return false;
    }
    out = (uint8_t*)tmp;
    memcpy(out + total_size - unit_size - 4, nalu_header, 4);
    memcpy(out + total_size - unit_size, extradata + 2, unit_size);

#if 0
    printf("-> " );
    for (int i=0;i<unit_size;i++)
      printf("%02x ",extradata[i + 2]);
    printf("\n");
#endif

    extradata += 2 + unit_size;

pps:
    if (!unit_nb && !sps_done++)
    {
      unit_nb = *extradata++;   // number of pps unit(s)
      if (unit_nb)
        pps_seen = 1;
    }
  }

  if (out)
    memset(out + total_size, 0, AV_INPUT_BUFFER_PADDING_SIZE);

  if (!sps_seen)
      printf( "SPS NALU missing or invalid. The resulting stream may not play");
  if (!pps_seen)
      printf( "PPS NALU missing or invalid. The resulting stream may not play");

  m_sps_pps_context.sps_pps_data = out;
  m_sps_pps_context.size = total_size;
  m_sps_pps_context.first_idr = 1;
  m_sps_pps_context.idr_sps_pps_seen = 0;

  return true;
}

bool IsIDR(uint8_t unit_type)
{
  switch (m_codec)
  {
    case AV_CODEC_ID_H264:
      return unit_type == AVC_NAL_IDR_SLICE;
    case AV_CODEC_ID_HEVC:
      return unit_type == HEVC_NAL_IDR_W_RADL ||
             unit_type == HEVC_NAL_IDR_N_LP ||
             unit_type == HEVC_NAL_CRA_NUT;
    default:
      return false;
  }
}

bool IsSlice(uint8_t unit_type)
{
  switch (m_codec)
  {
    case AV_CODEC_ID_H264:
      return unit_type == AVC_NAL_SLICE;
    case AV_CODEC_ID_HEVC:
      return unit_type == HEVC_NAL_TRAIL_R ||
             unit_type == HEVC_NAL_TRAIL_N ||
             unit_type == HEVC_NAL_TSA_N ||
             unit_type == HEVC_NAL_TSA_R ||
             unit_type == HEVC_NAL_STSA_N ||
             unit_type == HEVC_NAL_STSA_R ||
             unit_type == HEVC_NAL_BLA_W_LP ||
             unit_type == HEVC_NAL_BLA_W_RADL ||
             unit_type == HEVC_NAL_BLA_N_LP ||
             unit_type == HEVC_NAL_CRA_NUT ||
             unit_type == HEVC_NAL_RADL_N ||
             unit_type == HEVC_NAL_RADL_R ||
             unit_type == HEVC_NAL_RASL_N ||
             unit_type == HEVC_NAL_RASL_R;
    default:
      return false;
  }
}

void BitstreamAllocAndCopy5(uint8_t** poutbuf,
                                                uint32_t* poutbuf_size,
                                                const uint8_t* in,
                                                uint32_t in_size,
                                                uint8_t nal_type)
{
  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;
  void *tmp;

  if (nal_type == HEVC_NAL_UNSPEC62)
    nal_header_size = 4;
  else if (nal_type == HEVC_NAL_UNSPEC63)
    nal_header_size = 5;

  *poutbuf_size += in_size + nal_header_size;
  tmp = av_realloc(*poutbuf, *poutbuf_size);
  if (!tmp)
    return;
  *poutbuf = (uint8_t*)tmp;

  memcpy(*poutbuf + nal_header_size + offset, in, in_size);

  if (nal_header_size == 5)
  {
    (*poutbuf + offset)[0] = 0;
    (*poutbuf + offset)[1] = 0;
    (*poutbuf + offset)[2] = 1;
    (*poutbuf + offset)[3] = HEVC_NAL_UNSPEC63 << 1;
    (*poutbuf + offset)[4] = 1;
  }
  else if (nal_header_size == 4)
  {
    (*poutbuf + offset)[0] = 0;
    (*poutbuf + offset)[1] = 0;
    (*poutbuf + offset)[2] = 0;
    (*poutbuf + offset)[3] = 1;
  }
  else
  {
    (*poutbuf + offset)[0] = 0;
    (*poutbuf + offset)[1] = 0;
    (*poutbuf + offset)[2] = 1;
  }
}

void BitstreamAllocAndCopy6(uint8_t** poutbuf,
                                                int* poutbuf_size,
                                                const uint8_t* sps_pps,
                                                uint32_t sps_pps_size,
                                                const uint8_t* in,
                                                uint32_t in_size,
                                                uint8_t nal_type)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  uint32_t offset = *poutbuf_size;
  uint8_t nal_header_size = offset ? 3 : 4;
  void *tmp;

  // According to x265, this type is always encoded with four-sized header
  // https://bitbucket.org/multicoreware/x265_git/src/4bf31dc15fb6d1f93d12ecf21fad5e695f0db5c0/source/encoder/nal.cpp#lines-100
  if (nal_type == HEVC_NAL_UNSPEC62)
    nal_header_size = 4;

  *poutbuf_size += sps_pps_size + in_size + nal_header_size;
  tmp = av_realloc(*poutbuf, *poutbuf_size);
  if (!tmp)
    return;
  *poutbuf = (uint8_t*)tmp;
  if (sps_pps)
    memcpy(*poutbuf + offset, sps_pps, sps_pps_size);

  memcpy(*poutbuf + sps_pps_size + nal_header_size + offset, in, in_size);
  if (!offset)
  {
    BS_WB32(*poutbuf + sps_pps_size, 1);
  }
  else if (nal_header_size == 4)
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 0;
    (*poutbuf + offset + sps_pps_size)[3] = 1;
  }
  else
  {
    (*poutbuf + offset + sps_pps_size)[0] = 0;
    (*poutbuf + offset + sps_pps_size)[1] = 0;
    (*poutbuf + offset + sps_pps_size)[2] = 1;
  }
}
bool BitstreamConvert(uint8_t* pData, int iSize, uint8_t **poutbuf, int *poutbuf_size)
{
  // based on h264_mp4toannexb_bsf.c (ffmpeg)
  // which is Copyright (c) 2007 Benoit Fouet <benoit.fouet@free.fr>
  // and Licensed GPL 2.1 or greater

  int i;
  uint8_t *buf = pData;
  uint32_t buf_size = iSize;
  uint8_t  unit_type, nal_sps, nal_pps, nal_sei;
  int32_t  nal_size;
  uint32_t cumul_size = 0;
  const uint8_t *buf_end = buf + buf_size;

#ifdef HAVE_LIBDOVI
  const DoviData* rpu_data = NULL;
#endif

  std::vector<uint8_t> finalPrefixSeiNalu;

  switch (m_codec)
  {
    case AV_CODEC_ID_H264:
      nal_sps = AVC_NAL_SPS;
      nal_pps = AVC_NAL_PPS;
      nal_sei = AVC_NAL_SEI;
      break;
    case AV_CODEC_ID_HEVC:
      nal_sps = HEVC_NAL_SPS;
      nal_pps = HEVC_NAL_PPS;
      nal_sei = HEVC_NAL_SEI_PREFIX;
      break;
    default:
      return false;
  }

  do
  {
    if (buf + m_sps_pps_context.length_size > buf_end)
      goto fail;

    for (nal_size = 0, i = 0; i < m_sps_pps_context.length_size; i++)
      nal_size = (nal_size << 8) | buf[i];

    buf += m_sps_pps_context.length_size;
    if (m_codec == AV_CODEC_ID_H264)
    {
        unit_type = *buf & 0x1f;
    }
    else
    {
        unit_type = (*buf >> 1) & 0x3f;
    }

    if (buf + nal_size > buf_end || nal_size <= 0)
      goto fail;

    // Don't add sps/pps if the unit already contain them
    if (m_sps_pps_context.first_idr && (unit_type == nal_sps || unit_type == nal_pps))
      m_sps_pps_context.idr_sps_pps_seen = 1;

    if (!m_start_decode && (unit_type == nal_sps || IsIDR(unit_type) || (unit_type == nal_sei && has_sei_recovery_point(buf, buf + nal_size))))
      m_start_decode = true;

    // prepend only to the first access unit of an IDR picture, if no sps/pps already present
    if (m_sps_pps_context.first_idr && IsIDR(unit_type) && !m_sps_pps_context.idr_sps_pps_seen)
    {
      BitstreamAllocAndCopy7(poutbuf, poutbuf_size, m_sps_pps_context.sps_pps_data,
                            m_sps_pps_context.size, buf, nal_size, unit_type);
      m_sps_pps_context.first_idr = 0;
    }
    else
    {
      bool write_buf = true;
      const uint8_t* buf_to_write = buf;
      int32_t final_nal_size = nal_size;

      //bool containsHdr10Plus{false};

      if (!m_sps_pps_context.first_idr && IsSlice(unit_type))
      {
          m_sps_pps_context.first_idr = 1;
          m_sps_pps_context.idr_sps_pps_seen = 0;
      }
#if 0
      if (m_removeDovi && (unit_type == HEVC_NAL_UNSPEC62 || unit_type == HEVC_NAL_UNSPEC63))
        write_buf = false;

      // Try removing HDR10+ only if the NAL is big enough, optimization
      if (m_removeHdr10Plus && unit_type == HEVC_NAL_SEI_PREFIX && nal_size >= 7)
      {
        std::tie(containsHdr10Plus, finalPrefixSeiNalu) =
            CHevcSei::RemoveHdr10PlusFromSeiNalu(buf, nal_size);

        if (containsHdr10Plus)
        {
          if (!finalPrefixSeiNalu.empty())
          {
            buf_to_write = finalPrefixSeiNalu.data();
            final_nal_size = finalPrefixSeiNalu.size();
          }
          else
          {
            write_buf = false;
          }
        }
      }

      if (write_buf && (m_convert_dovi || m_dovi_el_type == ELType::TYPE_NONE))
      {
        if (unit_type == HEVC_NAL_UNSPEC62)
        {
#ifdef HAVE_LIBDOVI
          // Convert the RPU itself
          rpu_data = convert_dovi_rpu_nal(buf, nal_size, m_convert_dovi);
          m_dovi_el_type = get_dovi_el_type(buf, nal_size);
          if (rpu_data)
          {
            buf_to_write = rpu_data->data;
            final_nal_size = rpu_data->len;
          }
#endif
        }
        else if (m_convert_dovi && unit_type == HEVC_NAL_UNSPEC63)
        {
          // Ignore the enhancement layer, may or may not help
          write_buf = false;
        }
      }
#endif
      if (write_buf) {
        BitstreamAllocAndCopy7(poutbuf, poutbuf_size, NULL, 0, buf_to_write, final_nal_size,
                              unit_type);
      }

#ifdef HAVE_LIBDOVI
      if (rpu_data)
      {
        dovi_data_free(rpu_data);
        rpu_data = NULL;
      }
#endif
#if 0
      if (containsHdr10Plus && !finalPrefixSeiNalu.empty())
        finalPrefixSeiNalu.clear();
#endif
    }
    buf += nal_size;
    cumul_size += nal_size + m_sps_pps_context.length_size;
  } while (cumul_size < buf_size);

  return true;

fail:
  av_free(*poutbuf), *poutbuf = NULL;
  *poutbuf_size = 0;
  return false;
}

bool CBitstreamConverter(uint8_t *pData, int iSize)
{
  if (m_convertBuffer)
  {
    av_free(m_convertBuffer);
    m_convertBuffer = NULL;
  }
  m_inputSize = 0;
  m_convertSize = 0;
  m_inputBuffer = NULL;

  if (pData)
  {
    if (m_codec == AV_CODEC_ID_H264 ||
        m_codec == AV_CODEC_ID_HEVC)
    {
      if (m_to_annexb)
      {
        int demuxer_bytes = iSize;
        uint8_t *demuxer_content = pData;

        if (m_convert_bitstream)
        {
          // convert demuxer packet from bitstream to bytestream (AnnexB)
          int bytestream_size = 0;
          uint8_t *bytestream_buff = NULL;

          BitstreamConvert(demuxer_content, demuxer_bytes, &bytestream_buff, &bytestream_size);
          if (bytestream_buff && (bytestream_size > 0))
          {
            m_convertSize   = bytestream_size;
            m_convertBuffer = bytestream_buff;
            return true;
          }
          else
          {
            m_convertSize = 0;
            m_convertBuffer = NULL;
            printf( "CBitstreamConverter::Convert: error converting.");
            return false;
          }
        }
        else
        {
          m_inputSize = iSize;
          m_inputBuffer = pData;
          return true;
        }
      }
#if 0
      else
      {
        m_inputSize = iSize;
        m_inputBuffer = pData;

        if (m_convert_bytestream)
        {
          if(m_convertBuffer)
          {
            av_free(m_convertBuffer);
            m_convertBuffer = NULL;
          }
          m_convertSize = 0;

          // convert demuxer packet from bytestream (AnnexB) to bitstream
          AVIOContext *pb;

          if(avio_open_dyn_buf(&pb) < 0)
          {
            return false;
          }
          m_convertSize = avc_parse_nal_units(pb, pData, iSize);
          m_convertSize = avio_close_dyn_buf(pb, &m_convertBuffer);
        }
        else if (m_convert_3byteTo4byteNALSize)
        {
          if(m_convertBuffer)
          {
            av_free(m_convertBuffer);
            m_convertBuffer = NULL;
          }
          m_convertSize = 0;

          // convert demuxer packet from 3 byte NAL sizes to 4 byte
          AVIOContext *pb;
          if (avio_open_dyn_buf(&pb) < 0)
            return false;

          uint32_t nal_size;
          uint8_t *end = pData + iSize;
          uint8_t *nal_start = pData;
          while (nal_start < end)
          {
            nal_size = BS_RB24(nal_start);
            avio_wb32(pb, nal_size);
            nal_start += 3;
            avio_write(pb, nal_start, nal_size);
            nal_start += nal_size;
          }

          m_convertSize = avio_close_dyn_buf(pb, &m_convertBuffer);
        }
        return true;
      }
#endif
    }
  }

  return false;
}

void BitstreamConverterInit() {
    m_convert_bitstream = false;
    m_convertBuffer     = NULL;
    m_convertSize       = 0;
    m_inputBuffer       = NULL;
    m_inputSize         = 0;
    m_to_annexb = false;
    m_convert_3byteTo4byteNALSize = false;
    m_convert_bytestream = false;
    m_sps_pps_context.sps_pps_data = NULL;
    m_start_decode = true;
    m_convert_dovi = 0;
    m_removeDovi = false;
    m_removeHdr10Plus = false;
    m_dovi_el_type = ELType::TYPE_NONE;
    m_combine = false;
}

bool BitstreamConverterOpen(enum AVCodecID codec, uint8_t *in_extradata, int in_extrasize, bool to_annexb)
{

  BitstreamConverterInit();
  m_to_annexb = to_annexb;

  m_codec = codec;
  switch(m_codec)
  {
    case AV_CODEC_ID_H264:
      if (in_extrasize < 7 || in_extradata == NULL)
      {
        //printf( "CBitstreamConverter::Open avcC data too small or missing\n");
        return false;
      }
      // valid avcC data (bitstream) always starts with the value 1 (version)
      if(m_to_annexb)
      {
        if ( in_extradata[0] == 1 )
        {
          m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
          m_convert_bitstream =
              BitstreamConvertInitAVC(m_extraData, in_extrasize);
          return true;
        }
        else
          printf( "CBitstreamConverter::Open Invalid avcC\n");
      }
#if 0
      else
      {
        // valid avcC atom data always starts with the value 1 (version)
        if ( in_extradata[0] != 1 )
        {
          if ( (in_extradata[0] == 0 && in_extradata[1] == 0 && in_extradata[2] == 0 && in_extradata[3] == 1) ||
               (in_extradata[0] == 0 && in_extradata[1] == 0 && in_extradata[2] == 1) )
          {
            printf("CBitstreamConverter::Open annexb to bitstream init");
            // video content is from x264 or from bytestream h264 (AnnexB format)
            // NAL reformatting to bitstream format needed
            AVIOContext *pb;
            if (avio_open_dyn_buf(&pb) < 0)
              return false;
            m_convert_bytestream = true;
            // create a valid avcC atom data from ffmpeg's extradata
            isom_write_avcc(pb, in_extradata, in_extrasize);
            // unhook from ffmpeg's extradata
            in_extradata = NULL;
            // extract the avcC atom data into extradata then write it into avcCData for VDADecoder
            in_extrasize = avio_close_dyn_buf(pb, &in_extradata);
            // make a copy of extradata contents
            m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
            // done with the converted extradata, we MUST free using av_free
            av_free(in_extradata);
            return true;
          }
          else
          {
            printf( "CBitstreamConverter::Open invalid avcC atom data");
            return false;
          }
        }
        else
        {
          if (in_extradata[4] == 0xFE)
          {
            printf( "CBitstreamConverter::Open annexb to bitstream init 3 byte to 4 byte nal");
            // video content is from so silly encoder that think 3 byte NAL sizes
            // are valid, setup to convert 3 byte NAL sizes to 4 byte.
            in_extradata[4] = 0xFF;
            m_convert_3byteTo4byteNALSize = true;

            m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
            return true;
          }
        }
        // valid avcC atom
        m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
        return true;
      }
#endif
      return false;
      break;
#if 0
    case AV_CODEC_ID_HEVC:
      if (in_extrasize < 23 || in_extradata == NULL)
      {
        CLog::Log(LOGERROR, "CBitstreamConverter::Open hvcC data too small or missing");
        return false;
      }
      // valid hvcC data (bitstream) always starts with the value 1 (version)
      if(m_to_annexb)
      {
       /** @todo from Amlogic
        * It seems the extradata is encoded as hvcC format.
        * Temporarily, we support configurationVersion==0 until 14496-15 3rd
        * is finalized. When finalized, configurationVersion will be 1 and we
        * can recognize hvcC by checking if extradata[0]==1 or not.
        */

        if (in_extradata[0] || in_extradata[1] || in_extradata[2] > 1)
        {
          CLog::Log(LOGINFO, "CBitstreamConverter::Open bitstream to annexb init");
          m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
          m_convert_bitstream =
              BitstreamConvertInitHEVC(m_extraData.GetData(), m_extraData.GetSize());
          return true;
        }
        else
          CLog::Log(LOGINFO, "CBitstreamConverter::Open Invalid hvcC");
      }
      else
      {
        // valid hvcC atom data always starts with the value 1 (version)
        if ( in_extradata[0] != 1 )
        {
          if ( (in_extradata[0] == 0 && in_extradata[1] == 0 && in_extradata[2] == 0 && in_extradata[3] == 1) ||
               (in_extradata[0] == 0 && in_extradata[1] == 0 && in_extradata[2] == 1) )
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open annexb to bitstream init");
            //! @todo convert annexb to bitstream format
            return false;
          }
          else
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open invalid hvcC atom data");
            return false;
          }
        }
        else
        {
          if ((in_extradata[4] & 0x3) == 2)
          {
            CLog::Log(LOGINFO, "CBitstreamConverter::Open annexb to bitstream init 3 byte to 4 byte nal");
            // video content is from so silly encoder that think 3 byte NAL sizes
            // are valid, setup to convert 3 byte NAL sizes to 4 byte.
            in_extradata[4] |= 0x03;
            m_convert_3byteTo4byteNALSize = true;
          }
        }
        // valid hvcC atom
        m_extraData = FFmpegExtraData(in_extradata, in_extrasize);
        return true;
      }
      return false;
      break;
#endif
    default:
      return false;
      break;
  }
  return false;
}

void CBitstreamConverterClose(void)
{
  if (m_sps_pps_context.sps_pps_data)
    av_free(m_sps_pps_context.sps_pps_data), m_sps_pps_context.sps_pps_data = NULL;

  if (m_convertBuffer)
    av_free(m_convertBuffer), m_convertBuffer = NULL;
  m_convertSize = 0;

  m_extraData = {};

  m_inputSize = 0;
  m_inputBuffer = NULL;

  m_convert_bitstream = false;
  m_convert_bytestream = false;
  m_convert_3byteTo4byteNALSize = false;
  m_combine = false;
}

