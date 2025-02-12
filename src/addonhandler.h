#pragma once

#include <string>
#include "kodi/c-api/addon_base.h"
#include "kodi/c-api/addon-instance/inputstream.h"
#include "kodi/c-api/gui/general.h"
#include "bitstreamconverter.h"
#include "audioconverter.h"

extern "C" {
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavcodec/avcodec.h>
}

#define DMX_SPECIALID_STREAMINFO -10
#define DMX_SPECIALID_STREAMCHANGE -11

extern bool verbose;
extern struct INPUTSTREAM_IDS IDs;

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
    int GetTotalTime(int streamID, bool enable);
    int GetTime();
    int64_t SeekStream(int64_t position, int whence);

};
