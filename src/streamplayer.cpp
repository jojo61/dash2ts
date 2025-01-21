
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <filesystem>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <tinyxml2.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>
#include <thread>


#include "mpegts/mpegts_muxer.h"
#include "circular_buffer.hpp"

#include "kodi/versions.h"
#include "kodi/Filesystem.h"
#include "kodi/c-api/gui/general.h"
#include "kodi/c-api/addon_base.h"
#include "kodi/c-api/addon-instance/inputstream.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
}

bool verbose = false;

#include "bitstreamconverter.h"
#include "audioconverter.h"
#include "demuxpacket.h"
#include "curlhandler.h"
#include "xmlhandler.h"
#include "streamplayer.h"
#include "addonhandler.h"


static inline uint64_t GetusTicks(void) {

#ifdef CLOCK_MONOTONIC
    struct timespec tspec;

    clock_gettime(CLOCK_MONOTONIC, &tspec);
    return (uint64_t)(tspec.tv_sec * 1000000) + (tspec.tv_nsec);
#else
    struct timeval tval;

    if (gettimeofday(&tval, NULL) < 0) {
        return 0;
    }
    return (tval.tv_sec * 1000) + (tval.tv_usec / 1000);
#endif
}

//AAC audio
#define TYPE_AUDIO 0x0f
//H.264 video
#define TYPE_VIDEO 0x1b

//Audio PID
#define AUDIO_PID 257
//Video PID
#define VIDEO_PID 256
//PMT PID
#define PMT_PID 100


std::string headers = "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML. like Gecko) Chrome/116.0.0.0&Content-Type=application/octet-stream";

typedef struct {
        uint8_t *data;
        int size;
        int tag;
} cbuf;



// create Ringbuffer for output thread
circular_buffer<cbuf, 2000> rbuf;

//A callback where all the TS-packets are sent from the multiplexer
void muxOutput(mpegts::SimpleBuffer &rTsOutBuffer, uint8_t tag){
    
    if (rTsOutBuffer.size() % 188) {
        printf("packetsize wrong\n");
        return;
    }
        
    cbuf buf;
    buf.data = (uint8_t*)malloc(rTsOutBuffer.size());
    buf.size = rTsOutBuffer.size();
    buf.tag = tag;
    memcpy(buf.data,rTsOutBuffer.data(),rTsOutBuffer.size());
    while (rbuf.full()) {
        //printf("rbuf is full\n");
        usleep(20000);
    }
    rbuf.put(buf);
    usleep(10);

    rTsOutBuffer.clear();
    
}


    StreamPlayer::StreamPlayer(int portnr) {

        // Prepare NALU AUD
        NALUHeader.sync[0] = 0;
        NALUHeader.sync[1] = 0;
        NALUHeader.sync[2] = 0;
        NALUHeader.sync[3] = 1;
        NALUHeader.nalu = 9;
        NALUHeader.length[0] = 0x10;
        
            // Open output Socket
        sockfd = socket(AF_INET, SOCK_DGRAM, 0);
        if (sockfd < 0) {
            printf("ERROR opening socket\n");
            exit(0);
        }

        server = gethostbyname("127.0.0.1");
        if (server == NULL) {
            fprintf(stderr,"ERROR, no such host\n");
            exit(0);
        }
        
        bzero((char *) &serv_addr, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        bcopy((char *)server->h_addr, 
            (char *)&serv_addr.sin_addr.s_addr,
            server->h_length);
        serv_addr.sin_port = htons(portnr);

        int flags = fcntl(sockfd, F_GETFL);
        fcntl(sockfd, F_SETFL, flags&~SOCK_NONBLOCK);

        if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
            printf("ERROR connecting\n");
            exit(0);
        }

        //Create the map defining what datatype to map to what PID
        std::map<uint8_t, int> streamPidMap;
        streamPidMap[TYPE_AUDIO] = AUDIO_PID;
        streamPidMap[TYPE_VIDEO] = VIDEO_PID;

        // Create TS Multiplxer 
        lMuxer = new mpegts::MpegTsMuxer(streamPidMap, PMT_PID, VIDEO_PID,mpegts::MpegTsMuxer::MuxType::segmentType);

        //Provide the callback where TS packets are fed to
        lMuxer->tsOutCallback = std::bind(&muxOutput, std::placeholders::_1,std::placeholders::_2);


    };

    StreamPlayer::~StreamPlayer() {
        if (m_thread.joinable())
            m_thread.join();  
        close(sockfd);
    };

    void StreamPlayer::send_packet(uint8_t *p) {
        int n = 0;
        do {
            ssize_t r = write(sockfd,p,188);
            if (r < 0) {
                usleep(10);
                //printf("failure write %d\n",r);
            }
            else {
                n += r;
            }
            
        } while (n < 188);
    }

    void * StreamPlayer::Send_thread() {
        uint64_t lasttime;
        cbuf buf;
        int first=100; // send first 100 Video PES Packets without delay to stuff buffers

        while (rbuf.size() < 200) // wait for at least 200 PES Packets
            usleep(10);
        
        for (;;) {
            while (rbuf.empty()) {
                usleep(10);  // wait 10 ms
            }
            auto value = rbuf.get();
            buf = value.value();            // Get PES Packet
            int packets = buf.size / 188;   // Calc TS Packets 
            for (int i=0;i<packets;i++) {
                send_packet(buf.data+i*188); // Send all TS Packets 
                usleep(6);
            }
            if (buf.tag) {
                //if (rbuf.size() < 50)
                //   printf("Anz PES Packets %d\n",rbuf.size());
                if (first <= 0) {  // Delay only after first Bufferfilling Packets
                    int sleep = (int)(duration - 1 - ((GetusTicks()-lasttime) / 1000000));
                    //printf("sleep %d\n",sleep);
                    if (sleep > 0 && sleep <= 40)
                        usleep(sleep*1000);
                    else
                        usleep((duration -2)*1000);
                    lasttime = GetusTicks();
                }
                else {
                    first--;
                }
            }
            free(buf.data); 
        }

    }

    void StreamPlayer::StreamPlay(AddonHandler *h) {
        int VideoID=-1,AudioID=-1;
        m_thread = std::thread (&StreamPlayer::Send_thread,this);

        if (verbose) printf("Sucessfull opened Addon\n");
        h->GetCapabilities();
        
        if (h->GetStreamIDs()) {
            BitstreamConverterInit();
            
            for (int i=0;i<IDs.m_streamCount;i++) {
                if (verbose) printf("No. ID%d %d\n",i,IDs.m_streamIds[i]);
                h->GetStream(IDs.m_streamIds[i]);   // Build ID Table:
            }

            h->SelectStreams(&VideoID,&AudioID);
            
            
            h->EnableStream(IDs.m_streamIds[VideoID],true);  // Enable Video Stream
            h->OpenStream(IDs.m_streamIds[VideoID]);

            h->EnableStream(IDs.m_streamIds[AudioID],true); // Enable Audio Stream
            h->OpenStream(IDs.m_streamIds[AudioID]);

            h->GetStream(IDs.m_streamIds[VideoID]);  // Trigger start
            h->GetStream(IDs.m_streamIds[AudioID]);

            do {
                
                // Read Stream Packet
                demux = h->DemuxRead();
                         
                if (demux) {

                    //printf("PTS %f Disptime %d StreamId %p\n",demux->pts,demux->duration,demux->iStreamId);
                    if (demux->iStreamId == DMX_SPECIALID_STREAMCHANGE) {
                        if (verbose) printf("STREAMCHANGE DETECTED Size \n");
                        
                        CBitstreamConverterClose();
                        h->OpenStream(IDs.m_streamIds[ID]);
                        h->GetStream(IDs.m_streamIds[ID]);
                        continue;
                    }

                    if (h->GetType(demux->iStreamId) == INPUTSTREAM_TYPE_VIDEO) {
                        if (!audioseen )
                           continue;
                        
                        mpegts::EsFrame esFrame;

                        CBitstreamConverter(demux->pData,demux->iSize);
                         
                        //Build a frame of data (ES)
                        esFrame.mData = std::make_shared<mpegts::SimpleBuffer>();
                        
                        //Append your ES-Data
                        esFrame.mData->append((const uint8_t *) &NALUHeader, 6);
                        if (firstvideo) {  // resend PPS and SPS
                            esFrame.mData->append((const uint8_t *)m_sps_pps_context.sps_pps_data,m_sps_pps_context.size);
                            firstvideo = false;
                        }
                        esFrame.mData->append((const uint8_t *) m_convertBuffer,m_convertSize);
                        
                        duration = demux->duration / 1000;
                        esFrame.mPts = demux->pts/demux->duration;
                        esFrame.mDts = demux->dts/demux->duration;
                        esFrame.mPts *= 90*(demux->duration/1000);
                        esFrame.mDts *= 90*(demux->duration/1000);

                        //printf("pts %f %f Duration %f\n",demux->pts,demux->dts,demux->duration);
                        esFrame.mPcr = 0;
                        esFrame.mStreamType = TYPE_VIDEO;
                        esFrame.mStreamId = 224;
                        esFrame.mPid = VIDEO_PID;
                        esFrame.mExpectedPesPacketLength = 0;
                        esFrame.mCompleted = true;
#if 0                      
                        time_t now;
	                    now = time(0);
                        if ((now % 10) == 0)
                            makepmt = true;  // send PMT every 10 sek
#endif
                        //Multiplex your data
                        lMuxer->encode(esFrame,1,makepmt);
                        makepmt = false;
                    }

                    if (h->GetType(demux->iStreamId) == INPUTSTREAM_TYPE_AUDIO) {
                        audioseen = true;
                       
                        ConvertToADTS((const uint8_t*) demux->pData, (size_t) demux->iSize, ADTS_Header);
                        
                        //Build a frame of data (ES)
                        mpegts::EsFrame esFrame;
                        esFrame.mData = std::make_shared<mpegts::SimpleBuffer>();
                        //Append your ES-Data
                        
                        esFrame.mData->append((const uint8_t *) ADTS_Header,7);
                        esFrame.mData->append((const uint8_t *) demux->pData, demux->iSize);
                        esFrame.mPts = demux->pts/demux->duration;
                        esFrame.mDts = demux->dts/demux->duration;
                        esFrame.mPts *= 90*(demux->duration/1000);
                        esFrame.mDts *= 90*(demux->duration/1000);
                        //printf("apts %f %f Duration %f\n",demux->pts,demux->dts,demux->duration);
                        esFrame.mPcr = 0;
                        esFrame.mStreamType = TYPE_AUDIO;
                        esFrame.mStreamId = 192;
                        esFrame.mPid = AUDIO_PID;
                        esFrame.mExpectedPesPacketLength = 0;
                        esFrame.mCompleted = true;

                        //Multiplex your data
                        lMuxer->encode(esFrame,0);
                    }

                    usleep(2000);
                }
                
            }
            while (demux );
        }
    }

