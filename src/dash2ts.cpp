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
bool videoconvert = true;
bool verbose = false;
std::string path_to_kodi;


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

std::string headers = "User-Agent: Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML. like Gecko) Chrome/116.0.0.0&Content-Type=application/octet-stream";

// create Ringbuffer for output thread
circular_buffer<cbuf, 2000> rbuf;
int duration;  // Duration of one Frame in ms

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
        usleep(20000);
    }
    rbuf.put(buf);
    usleep(10);
  
    rTsOutBuffer.clear();
    //myfile.write((const char *)rTsOutBuffer.data(),rTsOutBuffer.size());
    
}

void send_packet(uint8_t *p) {
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


void * Send_thread(void* dummy) {
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
            //printf("Anz PES Packets %d\n",rbuf.size());
            if (first <= 0) {  // Delay only after first Bufferfilling Packets
                int sleep = (int)(duration - 1 - ((GetusTicks()-lasttime) / 1000000));
                //printf("sleep %d\n",sleep);
                if (sleep > 0 && sleep < 40)
                    usleep(sleep*1000);
                else
                    usleep(1000);
                lasttime = GetusTicks();
            }
            else {
                first--;
            }
        }
        free(buf.data); 
    }

}


struct {
    unsigned char  sync[4];
    unsigned char  nalu;
    unsigned char  length[1];
} NALUHeader;

void usage() {
    printf("Usage: dash2ts -f url_to_manifest.mpd\n");
    printf("               -p portnr\n");
    printf("              [-k path_to_kodi]\n");
    printf("              [-h http_headers]\n");
    printf("              [-d drm_token] or [-w widevine_url]\n");
    printf("              [-v] \n");
    printf("       path_to_kodi default is /storage/.kodi\n");
    printf("       http_headers default is %s\n",headers.c_str());
    printf("       only -d or -w is allowed. Not both\n");
    exit(0);
}

int
main(int argc, char *argv[])
{

    AddonHandler h;
    std::string r;
    
    bool audioseen = false;
    bool firstvideo = true;
    
#if 0
    char *test1 = "https://livesim.dashif.org/livesim/chunkdur_1/ato_7/testpic4_8s/Manifest.mpd";
    char *test2 = "https://apasfiis.sf.apa.at/dash/cms-worldwide/2024-12-08_1734_tl_02_Advent-in-Vorar_____14254423__o__1040094888__s15775910_0__ORF2BHD_17335613P_18215516P_QXB.mp4/manifest.mpd";
    char *test3 = "http://localhost/video/manifest.mpd";
    char *test4 = "https://orf3-247.mdn.ors.at/orf/orf3/qxa-247/manifest.mpd";
    char *test5 = "https://orf2-247.mdn.ors.at/orf/orf2/qxa-247/manifest.mpd";
    char *test6 = "https://media.axprod.net/TestVectors/v6.1-MultiDRM-MultiKey/Manifest_1080p.mpd";
#endif
    int ID = 0;
    struct DEMUX_PACKET* demux;
    unsigned char ADTS_Header[7];
    bool makepmt = true;
    char *url = NULL;
    int api_version = 0;
    
    std::string xmlsettings = "/addons/inputstream.adaptive/resources/settings.xml";
    
    std::string orf_widevine_url ="https://drm.ors.at/acquire-license/widevine?BrandGuid=13f2e056-53fe-4469-ba6d-999970dbe549&userToken=";
    std::string cenc = "com.widevine.alpha";
    std::string cdm = "/cdm";
    std::string drm_token;
    std::string widevine_url;
    
    INPUTSTREAM_INFO* stream;
   

    // Prepare NALU AUD
    NALUHeader.sync[0] = 0;
    NALUHeader.sync[1] = 0;
    NALUHeader.sync[2] = 0;
    NALUHeader.sync[3] = 1;
    NALUHeader.nalu = 9;
    NALUHeader.length[0] = 0x10;

    printf("-------Start---------\n");
    int c;
    while ((c = getopt (argc, argv, "u:p:k:d:w:h:v")) != -1) {
        switch (c) {
            case 'u': // URL to Manifest
                url = optarg;
                continue;
            case 'p': // Portnr
                portno = atoi(optarg);
                continue;
            case 'k': // Path to Kodi
                path_to_kodi.append(optarg);
                continue;
            case 'd': // drm token
                if (strcmp(optarg,"null")) {
                    drm_token.append(optarg);
                    Trim(drm_token,"\"");
                }
                continue;
            case 'w': // Widevine URL
                widevine_url.append(optarg);
                Trim(widevine_url,"\"");
                continue;
            case 'h': // Set new Headers
                headers.clear();
                headers.append(optarg);
                continue;
            case 'v': // Verbose
                verbose = true;
                continue;             
            default:
                usage();
                exit(0);
        }
        break;
    }

    if (!url || (drm_token.size() && widevine_url.size()) || !headers.size()) {
        usage();
    }

    if (!path_to_kodi.size())  // no path set
        path_to_kodi = "/storage/.kodi";  // Use default

    if (verbose) {
        printf("Path %s\n",path_to_kodi.c_str());
        printf("drm_token: %s\n",drm_token.c_str());
        printf("Server Port %d \n",portno);
    }

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
    std::string xml = path_to_kodi+xmlsettings;
    ReadXML(xml.c_str());
    
    //h.AddProp("inputstream.adaptive.drm_legacy","com.widevine.alpha|https://licensing.bitmovin.com/licensing|User-Agent=Mozilla%2F5.0+%28Windows+NT+10.0%3B+Win64%3B+x64%29+AppleWebKit%2F537.36+%28KHTML%2C+like+Gecko%29+Chrome%2F106.0.0.0+Safari%2F537.36");
    //h.AddProp("inputstream.adaptive.drm_legacy","com.widevine.alpha|https://drm.ors.at/acquire-license/widevine?BrandGuid=13f2e056-53fe-4469-ba6d-999970dbe549&userToken=B{SSM}|User-Agent=Mozilla%2F5.0+%28Windows+NT+10.0%3B+Win64%3B+x64%29+AppleWebKit%2F537.36+%28KHTML%2C+like+Gecko%29+Chrome%2F106.0.0.0+Safari%2F537.36&Content-Type=application/octet-stream");
    
    // serach for inputstream.adaptive lib
    std::string path = path_to_kodi + "/addons/inputstream.adaptive";
    std::string addon;

    for (const auto & entry : std::filesystem::directory_iterator(path)) {
        std::string tmp = entry.path();
        if ( tmp.find("inputstream.adaptive.so.") != std::string::npos && tmp.size() > addon.size())
            addon = entry.path();
    }

    if (!addon.size()) {
        printf(" Did not find any inputstream.adaptive lib\n");
        exit(0);
    }
    if (addon.find(".so.21.4") != std::string::npos) {
        api_version = 0;
    }
    if (addon.find(".so.21.5") != std::string::npos) {
        api_version = 1;
    }
    if (addon.find(".so.22.") != std::string::npos) {
        api_version = 2;
    }

    if (verbose) printf("Use lib: %s \nwith API Version %d\n",addon.c_str(),api_version);

    // Make Properties for inputsream-adaptive
    if (drm_token.size()) {
        std::string prop = cenc + "|" + orf_widevine_url + drm_token + "|" + headers;
        if (api_version >= 1) {
            h.AddProp("inputstream.adaptive.drm_legacy",prop.c_str());
        } else {
            prop = orf_widevine_url + drm_token + "|" + headers + "|R{SSM}|R";
            h.AddProp("inputstream.adaptive.license_key",prop.c_str());
            h.AddProp("inputstream.adaptive.license_type",cenc.c_str());
        }
    } else if (widevine_url.size()) {
        std::string prop = cenc + "|" + widevine_url + "|" + headers;
        if (api_version >= 1) {
            h.AddProp("inputstream.adaptive.drm_legacy",prop.c_str());
        } else {
            prop = widevine_url + "|" + headers + "|R{SSM}|R";
            h.AddProp("inputstream.adaptive.license_key",prop.c_str());
            h.AddProp("inputstream.adaptive.license_type",cenc.c_str());
        }
    }
    h.AddProp("inputstream.adaptive.stream_selection_type","adaptive");

    // Make Settings
    std::string decrypt = path_to_kodi+cdm;
    AddSettingString(NULL,"DECRYPTERPATH",decrypt.c_str());
    AddSettingString(NULL,"debug.save.license","false");   
    AddSettingString(NULL,"debug.save.manifest","false");  

    
    

    
        
    // Finally Load the Addon
    h.LoadAddon(addon);
    
    h.SetResolution(1920,1080);
    
    bool ret = h.OpenAddon(url);

    if (ret) {
        if (verbose) printf("Sucessfull opened Addon\n");
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
                        if (verbose) printf("STREAMCHANGE DETECTED Size \n");
                        
                        CBitstreamConverterClose();
                        h.OpenStream(IDs.m_streamIds[ID]);
                        h.GetStream(IDs.m_streamIds[ID]);
                        continue;
                    }

                    if (h.GetType(demux->iStreamId) == INPUTSTREAM_TYPE_VIDEO) {
                        if (!audioseen )
                           continue;
                        
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
