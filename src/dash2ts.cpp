#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <unistd.h>
#include <tinyxml2.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <fcntl.h>

#include "mpegts/mpegts_muxer.h"
#include "circular_buffer.hpp"

#include "addons/kodi-dev-kit/include/kodi/versions.h"
#include "addons/kodi-dev-kit/include/kodi/Filesystem.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/gui/general.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon_base.h"
#include "addons/kodi-dev-kit/include/kodi/c-api/addon-instance/inputstream.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
}
std::fstream myfile;
#include "bitstreamconverter.h"
#include "audioconverter.h"
#include "demuxpacket.h"
#include "curlhandler.h"
#include "xmlhandler.h"
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


static pthread_t SendThread; 
int sockfd, portno, n;
struct sockaddr_in serv_addr;
struct hostent *server;

typedef struct {
    uint8_t *data;
    int size;
    int tag;
} cbuf;

// create Ringbuffer for output thread
circular_buffer<cbuf, 10000> rbuf;

//A callback where all the TS-packets are sent from the multiplexer
void muxOutput(mpegts::SimpleBuffer &rTsOutBuffer, uint8_t tag){
   //Double to fail at non integer data
   int n;
    int packets = rTsOutBuffer.size() / 188;
    if (rTsOutBuffer.size() % 188) {
        printf("packetsize wrong\n");
        return;
    }
        
#if 0
    for (int i=0;i<(int)packets;i++) {
        n=0;
        do {
            ssize_t r = write(sockfd,rTsOutBuffer.data()+i*188,188);
            if (r < 0) {
                //printf("failure write %d\n",r);
                return;
            }
            else {
                n += r;
            }
            
        } while (n < 188);
        usleep(3);
    }
#else
    cbuf buf;
    buf.data = (uint8_t*)malloc(rTsOutBuffer.size());
    buf.size = rTsOutBuffer.size();
    buf.tag = tag;
    memcpy(buf.data,rTsOutBuffer.data(),rTsOutBuffer.size());
    while (rbuf.full()) {
        usleep(20000);
    }
    rbuf.put(buf);
    usleep(10);
#endif    
    rTsOutBuffer.clear();
    //myfile.write((const char *)rTsOutBuffer.data(),rTsOutBuffer.size());
    
}


void * Send_thread(void* dummy) {
    uint64_t lasttime;
    cbuf buf;
    while (rbuf.size() < 75)
        usleep(10);
    for (;;) {
        while (rbuf.empty()) {
            usleep(10);  // wait 10 ms
        }
        auto value = rbuf.get();
        buf = value.value();
        int packets = buf.size / 188;
        for (int i=0;i<packets;i++) {
            int n=0;
            do {
                ssize_t r = write(sockfd,buf.data+i*188,188);
                if (r < 0) {
                    usleep(10);
                    printf("failure write %d\n",r);
                }
                else {
                    n += r;
                }
                
            } while (n < 188);
            usleep(6);
        }
        if (buf.tag) {
            int sleep = (int)(40 - 2 - ((GetusTicks()-lasttime) / 1000000));
            //printf("sleep %d\n",sleep);
            if (sleep > 0 && sleep < 40)
                usleep(sleep*1000);
            else
                usleep(1000);
            lasttime = GetusTicks();
        }
        free(buf.data); 
    }

}


struct {
    unsigned char  sync[4];
    unsigned char  nalu;
    unsigned char  length[1];
} NALUHeader;



int
main(int argc, char *argv[])
{

    AddonHandler h;
    std::string r;
    
    char drm_string[500];
    bool audioseen = false;
    bool videoseen = false;
    int stuffed = 0;
    
    bool firstvideo = true;
    
    
    char *test1 = "https://livesim.dashif.org/livesim/chunkdur_1/ato_7/testpic4_8s/Manifest.mpd";
    char *test2 = "https://apasfiis.sf.apa.at/dash/cms-worldwide/2024-12-08_1734_tl_02_Advent-in-Vorar_____14254423__o__1040094888__s15775910_0__ORF2BHD_17335613P_18215516P_QXB.mp4/manifest.mpd";
    char *test3 = "http://localhost/video/manifest.mpd";
    char *test4 = "https://orf3-247.mdn.ors.at/orf/orf3/qxa-247/manifest.mpd";
    char *test5 = "https://orf2-247.mdn.ors.at/orf/orf2/qxa-247/manifest.mpd";
    char *test6 = "https://media.axprod.net/TestVectors/v6.1-MultiDRM-MultiKey/Manifest_1080p.mpd";
    int ID = 0;
    struct DEMUX_PACKET* demux;
    unsigned char ADTS_Header[7];
    bool makepmt = true;
    char *url;
    char *drm_token;
    INPUTSTREAM_INFO* stream;

   

    // Prepare NALU AUD
    NALUHeader.sync[0] = 0;
    NALUHeader.sync[1] = 0;
    NALUHeader.sync[2] = 0;
    NALUHeader.sync[3] = 1;
    NALUHeader.nalu = 9;
    NALUHeader.length[0] = 0x10;

    if (argc < 3) {
        printf("Usage: dash2ts <url_to_manifest.mpd> <portnr> [drm_token]\n");
        exit(0);
    }

    printf("-------Start---------\n");

    url=strdup(argv[1]);
    if (url == NULL || strlen(url) < 5) {
        printf("Using Test URL \n");
        url = test1;
    }

    if (argc < 4) {
        printf("No drm Token found \n");
        drm_string[0] = 0;
    }
    else {
        drm_token=strdup(argv[3]);
        drm_token[strlen(drm_token)-1] = 0;
        sprintf(drm_string,"com.widevine.alpha|https://drm.ors.at/acquire-license/widevine?BrandGuid=13f2e056-53fe-4469-ba6d-999970dbe549&userToken=%s|User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML. like Gecko) Chrome/116.0.0.0&Content-Type=application/octet-stream",drm_token+1);
    }
    printf("%s\n",drm_string);

    // Open output Socket
    portno = atoi(argv[2]);
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
    //printf("Server Port %d \n",portno);
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
         (char *)&serv_addr.sin_addr.s_addr,
         server->h_length);
    serv_addr.sin_port = htons(portno);

    int flags = fcntl(sockfd, F_GETFL);
    fcntl(sockfd, F_SETFL, flags&~SOCK_NONBLOCK);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        printf("ERROR connecting\n");
        exit(0);
    }

    pthread_create(&SendThread, NULL, Send_thread, NULL);

    //Create the map defining what datatype to map to what PID
    std::map<uint8_t, int> streamPidMap;
    streamPidMap[TYPE_AUDIO] = AUDIO_PID;
    streamPidMap[TYPE_VIDEO] = VIDEO_PID;

    //Create the multiplexer
    //param1 = PID map
    //param2 = PMT PID 
    //param3 = PCR PID
    mpegts::MpegTsMuxer lMuxer(streamPidMap, PMT_PID, VIDEO_PID,mpegts::MpegTsMuxer::MuxType::segmentType);

    //Provide the callback where TS packets are fed to
    lMuxer.tsOutCallback = std::bind(&muxOutput, std::placeholders::_1,std::placeholders::_2);
    ReadXML("/home/jojo/xbmc/xbmc/addons/inputstream.adaptive/resources/settings.xml");
    
    //h.AddProp("inputstream.adaptive.drm_legacy","com.widevine.alpha|https://licensing.bitmovin.com/licensing|User-Agent=Mozilla%2F5.0+%28Windows+NT+10.0%3B+Win64%3B+x64%29+AppleWebKit%2F537.36+%28KHTML%2C+like+Gecko%29+Chrome%2F106.0.0.0+Safari%2F537.36");
    //h.AddProp("inputstream.adaptive.drm_legacy","com.widevine.alpha|https://drm.ors.at/acquire-license/widevine?BrandGuid=13f2e056-53fe-4469-ba6d-999970dbe549&userToken=B{SSM}|User-Agent=Mozilla%2F5.0+%28Windows+NT+10.0%3B+Win64%3B+x64%29+AppleWebKit%2F537.36+%28KHTML%2C+like+Gecko%29+Chrome%2F106.0.0.0+Safari%2F537.36&Content-Type=application/octet-stream");
    if (drm_string[0])
        h.AddProp("inputstream.adaptive.drm_legacy",drm_string);
    AddSettingString(NULL,"DECRYPTERPATH","/home/jojo/.kodi/cdm");
    //AddSettingString(NULL,"debug.save.license","true");
    //AddSettingString(NULL,"debug.save.manifest","true");
    h.LoadAddon("/home/jojo/xbmc/xbmc/addons/inputstream.adaptive/inputstream.adaptive.so.21.5.7");
    
    h.SetResolution(1920,1080);
    
    bool ret = h.OpenAddon(url);

    if (ret) {
        printf("Sucessfull opened Addon\n");
        h.GetCapabilities();
        
        if (h.GetStreamIDs()) {
            BitstreamConverterInit();
            printf("No. Streams %d ID0 %d ID1 %d\n",IDs.m_streamCount,IDs.m_streamIds[0],IDs.m_streamIds[1]);
            h.EnableStream(IDs.m_streamIds[ID],true);
            h.OpenStream(IDs.m_streamIds[ID]);
            h.EnableStream(IDs.m_streamIds[ID+1],true);
            h.OpenStream(IDs.m_streamIds[ID+1]);
            stream = h.GetStream(IDs.m_streamIds[ID]);
                 
            //myfile.open ("stream.mp4", std::ios::out  | std::ios::binary);
            
            h.GetStream(IDs.m_streamIds[ID+1]);

            do {
                
                demux = h.DemuxRead();
                         
                if (demux) {

                    //printf("PTS %f Disptime %d StreamId %p\n",demux->pts,demux->duration,demux->iStreamId);
                    if (demux->iStreamId == DMX_SPECIALID_STREAMCHANGE) {
                        printf("STREAMCHANGE DETECTED Size \n");
                        
                        CBitstreamConverterClose();
                        h.OpenStream(IDs.m_streamIds[ID]);
                        h.GetStream(IDs.m_streamIds[ID]);
                        continue;
                    }

                    if (h.GetType(demux->iStreamId) == INPUTSTREAM_TYPE_VIDEO) {
                        if (!audioseen )
                           continue;
                        videoseen = true;
                        mpegts::EsFrame esFrame;

                        //myfile.write((const char *) demux->pData,demux->iSize);

                        CBitstreamConverter(demux->pData,demux->iSize);
                        
                        //myfile.write((const char *) m_convertBuffer,m_convertSize);
                        
                        //Build a frame of data (ES)
                        esFrame.mData = std::make_shared<mpegts::SimpleBuffer>();
                        
                        //Append your ES-Data
                        esFrame.mData->append((const uint8_t *) &NALUHeader, 6);
                        if (firstvideo) {  // resend PPS and SPS
                            esFrame.mData->append((const uint8_t *)m_sps_pps_context.sps_pps_data,m_sps_pps_context.size);
                            firstvideo = false;
                        }
                        esFrame.mData->append((const uint8_t *) m_convertBuffer,m_convertSize);
                        
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
                        
                        time_t now;
	                    now = time(0);
                        if ((now % 10) == 0)
                            makepmt = true;  // send PMT every 10 sek

                        //Multiplex your data
                        lMuxer.encode(esFrame,1,makepmt);
                        makepmt = false;
                        //myfile.write((const char *)m_convertBuffer,m_convertSize);
                        //printf("got %d Video\n",demux->iSize);
                    }

                    if (h.GetType(demux->iStreamId) == INPUTSTREAM_TYPE_AUDIO) {
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
                        lMuxer.encode(esFrame,0);
                        //printf("Write %d Audio Bytes ID: %d  \n ",demux->iSize,demux->iStreamId);
                    }

                    usleep(2000);
                }
                
            }
            while (demux );
            close(sockfd);
            //myfile.close();

        }
    }
    else {
        printf("Can't Open URL\n");
    }

    exit(EXIT_SUCCESS);
}
