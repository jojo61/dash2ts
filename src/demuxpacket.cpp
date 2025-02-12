/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "kodi/c-api/addon-instance/inputstream/stream_crypto.h"
#include <string>
#include "demuxpacket.h"

//CryptoSession is usually obtained once per stream, but could change if an key expires

enum CryptoSessionSystem : uint8_t
{
  CRYPTO_SESSION_SYSTEM_NONE,
  CRYPTO_SESSION_SYSTEM_WIDEVINE,
  CRYPTO_SESSION_SYSTEM_PLAYREADY,
  CRYPTO_SESSION_SYSTEM_WISEPLAY,
  CRYPTO_SESSION_SYSTEM_CLEARKEY,
};

struct DemuxCryptoSession
{
  DemuxCryptoSession(const CryptoSessionSystem sys, const char* sData, const uint8_t flags)
    : sessionId(sData), keySystem(sys), flags(flags)
  {
  }

  bool operator == (const DemuxCryptoSession &other) const
  {
    return keySystem == other.keySystem && sessionId == other.sessionId;
  };

  // encryped stream infos
  std::string sessionId;
  CryptoSessionSystem keySystem;

  static const uint8_t FLAG_SECURE_DECODER = 1;
  uint8_t flags;
private:
  DemuxCryptoSession(const DemuxCryptoSession&) = delete;
  DemuxCryptoSession& operator=(const DemuxCryptoSession&) = delete;
};

//CryptoInfo stores the information to decrypt a sample

struct DemuxCryptoInfo : DEMUX_CRYPTO_INFO
{
  explicit DemuxCryptoInfo(const unsigned int numSubs)
  {
    numSubSamples = numSubs;
    flags = 0;
    clearBytes = new uint16_t[numSubs];
    cipherBytes = new uint32_t[numSubs];
  };

  ~DemuxCryptoInfo()
  {
    delete[] clearBytes;
    delete[] cipherBytes;
  }

private:
  DemuxCryptoInfo(const DemuxCryptoInfo&) = delete;
  DemuxCryptoInfo& operator=(const DemuxCryptoInfo&) = delete;
};

void* AlignedMalloc(size_t s, size_t alignTo)
{
  void* p=NULL;
  posix_memalign(&p, alignTo, s);

  return p;
}

void AlignedFree(void* p)
{
  free(p);
}

void FreeDemuxPacket(DemuxPacket* pPacket)
{
  if (pPacket)
  {
    if (pPacket->pData)
      AlignedFree(pPacket->pData);
    if (pPacket->iSideDataElems)
    {
      AVPacket* avPkt = av_packet_alloc();
      if (!avPkt)
      {
        //Log(LOGERROR, "CDVDDemuxUtils::{} - av_packet_alloc failed: {}", __FUNCTION__,
         //         strerror(errno));
      }
      else
      {
        avPkt->side_data = static_cast<AVPacketSideData*>(pPacket->pSideData);
        avPkt->side_data_elems = pPacket->iSideDataElems;

        //! @todo: properly handle avpkt side_data. this works around our improper use of the side_data
        // as we pass pointers to ffmpeg allocated memory for the side_data. we should really be allocating
        // and storing our own AVPacket. This will require some extensive changes.

        // here we make use of ffmpeg to free the side_data, we shouldn't have to allocate an intermediate AVPacket though
        av_packet_free(&avPkt);
      }
    }
    if (pPacket->cryptoInfo)
      delete pPacket->cryptoInfo;
    delete pPacket;
  }
}

DemuxPacket* AllocateDemuxPacket(int iDataSize)
{
  DemuxPacket* pPacket = new DemuxPacket();

  //printf("Alloc Demux Packet Size %d\n",iDataSize);

  if (iDataSize > 0)
  {
    // need to allocate a few bytes more.
    // From avcodec.h (ffmpeg)
    /**
     * Required number of additionally allocated bytes at the end of the input bitstream for decoding.
     * this is mainly needed because some optimized bitstream readers read
     * 32 or 64 bit at once and could read over the end<br>
     * Note, if the first 23 bits of the additional bytes are not 0 then damaged
     * MPEG bitstreams could cause overread and segfault
     */
    pPacket->pData = static_cast<uint8_t*>(AlignedMalloc(iDataSize + AV_INPUT_BUFFER_PADDING_SIZE, 16));
    if (!pPacket->pData)
    {
      FreeDemuxPacket(pPacket);
      return NULL;
    }

    // reset the last 8 bytes to 0;
    memset(pPacket->pData + iDataSize, 0, AV_INPUT_BUFFER_PADDING_SIZE);
  }

  return pPacket;
}

DemuxPacket* AllocateDemuxPacket(unsigned int iDataSize, unsigned int encryptedSubsampleCount)
{
    //printf("Alloc Demux Packet mit encrypt Size %d\n",iDataSize);
  DemuxPacket *ret(AllocateDemuxPacket(iDataSize));
  if (ret && encryptedSubsampleCount > 0)
    ret->cryptoInfo = new DemuxCryptoInfo(encryptedSubsampleCount);
  return ret;
}


void StoreSideData(DemuxPacket *pkt, AVPacket *src)
{
  AVPacket* avPkt = av_packet_alloc();
  if (!avPkt)
  {
    //Log(LOGERROR, "CDVDDemuxUtils::{} - av_packet_alloc failed: {}", __FUNCTION__,
    //          strerror(errno));
    return;
  }

  // here we make allocate an intermediate AVPacket to allow ffmpeg to allocate the side_data
  // via the copy below. we then reference this allocated memory in the DemuxPacket. this behaviour
  // is bad and will require a larger rework.
  av_packet_copy_props(avPkt, src);
  pkt->pSideData = avPkt->side_data;
  pkt->iSideDataElems = avPkt->side_data_elems;

  //! @todo: properly handle avpkt side_data. this works around our improper use of the side_data
  // as we pass pointers to ffmpeg allocated memory for the side_data. we should really be allocating
  // and storing our own AVPacket. This will require some extensive changes.
  av_buffer_unref(&avPkt->buf);
  av_free(avPkt);
}


DEMUX_PACKET* cb_allocate_demux_packet(void* kodiInstance, int data_size)
{
  return AllocateDemuxPacket(data_size);
}

DEMUX_PACKET* cb_allocate_encrypted_demux_packet(
    void* kodiInstance, unsigned int dataSize, unsigned int encryptedSubsampleCount)
{
  return AllocateDemuxPacket(dataSize, encryptedSubsampleCount);
}

void cb_free_demux_packet(void* kodiInstance, DEMUX_PACKET* packet)
{
  FreeDemuxPacket(static_cast<DemuxPacket*>(packet));
}

