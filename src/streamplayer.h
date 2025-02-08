#include <stdlib.h>
#include <string>
#include <cstring>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include "kodi/c-api/addon_base.h"
#include "kodi/c-api/addon-instance/inputstream.h"
#include "mpegts/mpegts_muxer.h"



class AddonHandler
{
    KODI_ADDON_INSTANCE_STRUCT kodi = {};
    AddonGlobalInterface m_interface = {};
    INPUTSTREAM_PROPERTY props = {};
    

private:
    void *handle;
    int (*createfunc)(void *);
    char *(*gettypeversionfunc)(int);
    char *(*gettypeminversionfunc)(int);
    char *error;
    
public:
    
    AddonHandler(std::string);
    ~AddonHandler();

    int LoadAddon();
    bool OpenURL(char *);
    void SetResolution(unsigned int ,unsigned int );
    bool GetStreamIDs();
    void EnableStream(int, bool );
    void OpenStream(int );
    void Close();
    int GetTotalTime(int streamID, bool enable);
    int GetTime();
    int64_t SeekStream(int64_t position, int whence);
    void DemuxFlush();
    int GetCapabilities();
    void* GetStream(int streamId) const;
    struct DEMUX_PACKET* DemuxRead();
    int Read(uint8_t* , int );
    int GetType(int );
    static char * get_user_path(void *);
    bool AddProp(const char *, const char *);
    bool AddSettingString(void *, char *, const char * );
    int LoadXML(std::string );
    void SelectStreams(int *, int*);
    
};

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

