#include <stdlib.h>
#include <string>
#include <cstring>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include "kodi/c-api/addon_base.h"
#include "kodi/c-api/addon-instance/inputstream.h"
#include "mpegts/mpegts_muxer.h"
#include "addonhandler.h"
#include "bitstreamconverter.h"

class StreamPlayer
{
 
private:
    static pthread_t SendThread; 
    
    struct sockaddr_in serv_addr;
    struct hostent *server;
    bool audioseen = false;
    bool videoseen = false;
    bool firstvideo = true;
    int ID = 0;
    struct DEMUX_PACKET* demux;
    unsigned char ADTS_Header[7];
    bool makepmt = true;
    mpegts::MpegTsMuxer *lMuxer;

    
    int duration;  // Duration of one Frame in ms

    struct {
        unsigned char  sync[4];
        unsigned char  nalu;
        unsigned char  length[1];
    } NALUHeader;
    std::thread m_thread;

public:

    StreamPlayer(int );
    ~StreamPlayer();
    
    void send_packet(uint8_t *,int);
    void *Send_thread();
    void StreamPlay(AddonHandler *);
};

