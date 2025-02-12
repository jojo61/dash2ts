/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "kodi/c-api/addon-instance/inputstream.h"
#include "kodi/c-api/addon-instance/inputstream/stream_crypto.h"
#include <string>

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
}

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

struct DemuxPacket : DEMUX_PACKET
{
    DemuxPacket()
    {
        pData = nullptr;
        iSize = 0;
        iStreamId = -1;
        isDualStream = false;
        isELPackage = false;
        demuxerId = -1;
        iGroupId = -1;

        pSideData = nullptr;
        iSideDataElems = 0;

        pts = AV_NOPTS_VALUE;
        dts = AV_NOPTS_VALUE;
        duration = 0;
        dispTime = 0;
        recoveryPoint = false;

        cryptoInfo = nullptr;
    }

    //! @brief PTS offset correction applied to the PTS and DTS.
    double m_ptsOffsetCorrection{0};
    //! @brief Indicate package is from a Dolby Vision dual stream source.
    bool isDualStream;
    //! @brief Indicate package is from a Dolby Vision enhancement layer.
    bool isELPackage;
};

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

void cb_free_demux_packet(void* kodiInstance, DEMUX_PACKET* packet);
DEMUX_PACKET* cb_allocate_demux_packet(void* kodiInstance, int data_size);
DEMUX_PACKET* cb_allocate_encrypted_demux_packet(void* kodiInstance, unsigned int dataSize, unsigned int encryptedSubsampleCount);