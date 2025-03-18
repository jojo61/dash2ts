#include <dlfcn.h>
#include <filesystem>
#include "kodi/versions.h"
#include "addonhandler.h"
#include "StringUtils.h"
#include "curlhandler.h"
#include "demuxpacket.h"
#include "xmlhandler.h"

extern "C" {
#include "kodi/c-api/filesystem.h"
}

void addon_log_msg(const KODI_ADDON_BACKEND_HDL hdl,
                    const int level,
                    const char *msg)
{
    if (verbose) printf("Addon LOG: %s\n", msg);
}

void free_string(const KODI_ADDON_BACKEND_HDL hdl, char* str)
{
    if (str)
        free(str);
    }

void free_string_array(const KODI_ADDON_BACKEND_HDL hdl,
                                    char** arr,
                                    int numElements)
{
    if (arr)
    {
        for (int i = 0; i < numElements; ++i)
        {
        free(arr[i]);
        }
        free(arr);
    }
}

AdjustRefreshRateStatus get_adjust_refresh_rate_status(KODI_HANDLE kodiBase) {
    return ADJUST_REFRESHRATE_STATUS_OFF;
}


struct INPUTSTREAM_IDS IDs;
struct {
    int ID;
    int type;
    char *codecName;
} ID_type[INPUTSTREAM_MAX_STREAM_COUNT];
int max_ID_type=0;
bool running = false;

INPUTSTREAM_INFO stream{};


KODI_HANDLE cb_get_stream_transfer(KODI_HANDLE handle,
                                       int streamId,
                                       INPUTSTREAM_INFO* stream) 
{
    printf("get_stream_transfer Stream %d StreamType %d \nCodec %s KeySystem %d  %d-%d PID %d Profil %d Feature %04x Flags %04x ExtraSize %d\n"\
                                                            ,streamId\
                                                            ,stream->m_streamType\
                                                            ,stream->m_codecName\
                                                            ,stream->m_cryptoSession.keySystem\
                                                            ,stream->m_Width\
                                                            ,stream->m_Height\
                                                            ,stream->m_pID\
                                                            ,stream->m_codecProfile\
                                                            ,stream->m_features\
                                                            ,stream->m_flags\
                                                            ,stream->m_ExtraSize);
    std::string codecName(stream->m_codecName);
    const AVCodec* codec = nullptr;

    ID_type[max_ID_type].codecName = strdup(stream->m_codecName);
    ID_type[max_ID_type].ID = streamId;
    ID_type[max_ID_type].type = stream->m_streamType;
    max_ID_type++;
    
    
    if (!running)
        return nullptr;


    if (stream->m_streamType != INPUTSTREAM_TYPE_TELETEXT &&
        stream->m_streamType != INPUTSTREAM_TYPE_RDS && 
        stream->m_streamType != INPUTSTREAM_TYPE_ID3)
    {
        ToLower(codecName);
        codec = avcodec_find_decoder_by_name(codecName.c_str());
        if (!codec) {
            printf("Can't find decoder for %s\n", codecName.c_str());
            return nullptr;
        }
    }
    
    if (stream->m_streamType == INPUTSTREAM_TYPE_VIDEO && stream->m_ExtraData && stream->m_ExtraSize) {
        BitstreamConverterOpen(codec->id, (uint8_t*)stream->m_ExtraData, stream->m_ExtraSize, true);
    } else 
        if (stream->m_streamType == INPUTSTREAM_TYPE_AUDIO) {
           AudioConverterOpen(stream);
        }

    return nullptr;
}

    void AddonHandler::SelectStreams(int *videoid, int*audioid) {
        for (int i=0;i< max_ID_type;i++) {
            if (ID_type[i].type == 1) {
                *videoid = i;
            }
            if (ID_type[i].type == 2 && *audioid == -1 && strcmp("eac3",ID_type[i].codecName)) {
                *audioid = i;
            }
        }
        return;
    }

std::string path;


    AddonHandler::AddonHandler(std::string path_to_kodi) {
        path = path_to_kodi;
    }

    AddonHandler::~AddonHandler() { dlclose(handle); }

    int AddonHandler::LoadAddon()
    {
        int api_version=0;
        kodi.inputstream = new AddonInstance_InputStream;
        kodi.inputstream->props = new AddonProps_InputStream;
        kodi.inputstream->toAddon = new KodiToAddonFuncTable_InputStream;
        kodi.inputstream->toKodi = new AddonToKodiFuncTable_InputStream;
        kodi.functions = new KODI_ADDON_INSTANCE_FUNC();

        KODI_ADDON_INSTANCE_INFO *info = new KODI_ADDON_INSTANCE_INFO;
        info->type = ADDON_INSTANCE_INPUTSTREAM;
        info->parent = nullptr;
        info->functions = new KODI_ADDON_INSTANCE_FUNC_CB();
        info->version = "2.0.2";
        kodi.info = info;

        m_interface.addonBase = nullptr;
        m_interface.globalSingleInstance = nullptr;
        m_interface.firstKodiInstance = &kodi;

        m_interface.toAddon = new KodiToAddonFuncTable_Addon();

        m_interface.toKodi = new AddonToKodiFuncTable_Addon();
        m_interface.toKodi->kodi_addon = new AddonToKodiFuncTable_kodi_addon();
        m_interface.toKodi->kodi_gui = new AddonToKodiFuncTable_kodi_gui();
        m_interface.toKodi->kodi_gui->general = new AddonToKodiFuncTable_kodi_gui_general;
        m_interface.toKodi->kodi_filesystem = new AddonToKodiFuncTable_kodi_filesystem;
        m_interface.toKodi->kodi_filesystem->curl_create = curl_create; 
        m_interface.toKodi->kodi_filesystem->curl_add_option = curl_add_option; 
        m_interface.toKodi->kodi_filesystem->curl_open = curl_open; 
        m_interface.toKodi->kodi_filesystem->get_property_values = get_property_values;
        m_interface.toKodi->kodi_filesystem->close_file = close_file;
        m_interface.toKodi->kodi_filesystem->read_file = read_file;
        m_interface.toKodi->kodi_filesystem->open_file_for_write = open_file_for_write;
        m_interface.toKodi->kodi_filesystem->write_file = write_file;
        m_interface.toKodi->kodi_filesystem->get_file_download_speed = get_file_download_speed;
        m_interface.toKodi->kodi_filesystem->translate_special_protocol = translate_special_protocol;
        m_interface.toKodi->kodi_filesystem->remove_directory = remove_directory;
        m_interface.toKodi->kodi_filesystem->get_directory = get_directory;
        m_interface.toKodi->kodi_filesystem->create_directory = create_directory;
        m_interface.toKodi->kodi_filesystem->free_directory = free_directory;

        m_interface.toKodi->kodi_addon->get_user_path = AddonHandler::get_user_path;
        m_interface.toKodi->kodi_addon->get_addon_info = get_addon_info;

        kodi.inputstream->toKodi->free_demux_packet = cb_free_demux_packet;
        kodi.inputstream->toKodi->allocate_demux_packet = cb_allocate_demux_packet;
        kodi.inputstream->toKodi->allocate_encrypted_demux_packet = cb_allocate_encrypted_demux_packet;

        m_interface.toKodi->addon_log_msg = addon_log_msg;
        m_interface.toKodi->free_string = free_string;
        m_interface.toKodi->free_string_array = free_string_array;
        m_interface.toKodi->kodi_gui->general->get_adjust_refresh_rate_status = get_adjust_refresh_rate_status;

        // serach for inputstream.adaptive lib
        std::string libpath = path + "/addons/inputstream.adaptive";
        std::string addon;

        for (const auto & entry : std::filesystem::directory_iterator(libpath)) {
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

        if (verbose) printf("Open lib %s\n",addon.c_str());

        handle = dlopen(addon.c_str(), RTLD_LAZY);

        if (!handle)
        {
            fprintf(stderr, " Error Open inputstream-adaptive lib: %s\n", dlerror());
            exit(EXIT_FAILURE);
        }

        dlerror(); /* Clear any existing error */

        createfunc = (int (*)(void *))dlsym(handle, "ADDON_Create");
        error = dlerror();
        if (error)
            printf("Error in find ADDON_Create %s\n", error);
        gettypeversionfunc = (char *(*)(int))dlsym(handle, "ADDON_GetTypeVersion");
        error = dlerror();
        if (error)
            printf("Error in find ADDON_GetTypeVersion %s\n", error);
        gettypeminversionfunc = (char *(*)(int))dlsym(handle, "ADDON_GetTypeMinVersion");
        error = dlerror();
        if (error)
            printf("Error in find ADDON_GetTypeMinVersion %s\n", error);
#if 0
        for (int i = 0; i < 6; i++)
        {
            printf("TypeVersion %d %s\n", i, gettypeversionfunc(i));
            printf("TypeMinVersion %d %s\n", i, gettypeminversionfunc(i));
        }
#endif
        int status = createfunc(&m_interface);
        //printf("Create Status %d\n", status);

        if (status == ADDON_STATUS_OK && m_interface.toAddon->create)
        {
            status = m_interface.toAddon->create(m_interface.firstKodiInstance, &m_interface.addonBase);
            if (status == ADDON_STATUS_OK)
            {
                //printf("Create c-api Status %d\n", status);
            }
        }

        m_interface.toKodi->kodi_addon->get_setting_bool = GetSettingBool;
        m_interface.toKodi->kodi_addon->get_setting_int = GetSettingInt;
        m_interface.toKodi->kodi_addon->get_setting_float = GetSettingFloat;
        m_interface.toKodi->kodi_addon->get_setting_string = GetSettingString;

        status = m_interface.toAddon->create_instance(m_interface.addonBase, &kodi);
        //printf("Get instance status %d\n", status);
        return api_version;
    }

    bool AddonHandler::OpenURL(char *url) {

        // Prepare settings from xml File
        std::string xmlfile = path +"/addons/inputstream.adaptive/resources/settings.xml";
        std::string addonpath = path +"/addons/inputstream.adaptive";
        LoadXML(xmlfile);
        AddProp("inputstream.adaptive.stream_selection_type","adaptive");
        AddProp("inputstream.adaptive.manifest_type","mpd");
        //AddProp("inputstream.adaptive.license_flags","force_secure_decoder");
        
        // Make Settings
        std::string decrypt = path+"/cdm";
        AddSettingString(NULL,"DECRYPTERPATH",decrypt.c_str());
        AddSettingString(NULL,"NOSECUREDECODER","false");
        AddSettingString(NULL,"debug.save.license","false");   
        AddSettingString(NULL,"debug.save.manifest","false");  

        props.m_strURL = url;
        props.m_mimeType = "application/dash+xml";
        props.m_profileFolder = strdup(addonpath.c_str());
        props.m_libFolder = strdup(addonpath.c_str());
        //props.m_nCountInfoValues = 0;
        
        bool status = kodi.inputstream->toAddon->open(kodi.inputstream, &props);
        //printf("Open status %d\n", status);
        return status;
    }

    void AddonHandler::SetResolution(unsigned int width,unsigned int height) {
        kodi.inputstream->toAddon->set_video_resolution(kodi.inputstream, width,height,width,height);
    }

    bool AddonHandler::GetStreamIDs() {   
        max_ID_type=0;  // Reset ID Mapping 
        if (kodi.inputstream->toAddon->get_stream_ids(kodi.inputstream,&IDs))
            return true;
        return false;
    }

    void AddonHandler::EnableStream(int streamID, bool enable) {
        running = true;
        kodi.inputstream->toAddon->enable_stream(kodi.inputstream,streamID,enable);
    }

    void AddonHandler::OpenStream(int streamID) {
        kodi.inputstream->toAddon->open_stream(kodi.inputstream,streamID);
    }

    void AddonHandler::Close() {
        kodi.inputstream->toAddon->close(kodi.inputstream);
    }

    void AddonHandler::DemuxFlush() {
        kodi.inputstream->toAddon->demux_flush(kodi.inputstream);
    }

    int AddonHandler::GetCapabilities() {
        struct INPUTSTREAM_CAPABILITIES caps;
        kodi.inputstream->toAddon->get_capabilities(kodi.inputstream,&caps);
        //printf("Capabilities %04x\n",caps.m_mask);
        return caps.m_mask;
    }

    void* AddonHandler::GetStream(int streamId) const
    {
        
        KODI_HANDLE demuxStream = nullptr;
        bool ret = kodi.inputstream->toAddon->get_stream(kodi.inputstream, streamId, &stream,
                                                            &demuxStream, cb_get_stream_transfer);
        if (!ret || stream.m_streamType == INPUTSTREAM_TYPE_NONE)
            return nullptr;

        return &stream;
    }

    struct DEMUX_PACKET* AddonHandler::DemuxRead() 
    {
        struct DEMUX_PACKET* demux;

        demux = kodi.inputstream->toAddon->demux_read(kodi.inputstream);
        return demux;
    }

    void AddonHandler::DenuxReset() {
        if (!kodi.inputstream->toAddon->demux_reset) // does lib provide this function
            return;

        kodi.inputstream->toAddon->demux_reset(kodi.inputstream);
    }

    void AddonHandler::DenuxAbort() {
        if (!kodi.inputstream->toAddon->demux_abort) // does lib provide this function
            return;

        kodi.inputstream->toAddon->demux_abort(kodi.inputstream);
    }

    void AddonHandler::DenuxSetSpeed(int speed) {
        if (!kodi.inputstream->toAddon->demux_set_speed) // does lib provide this function
            return;

        kodi.inputstream->toAddon->demux_set_speed(kodi.inputstream, speed);
    }

    bool AddonHandler::PosTime(int ms) {
        return kodi.inputstream->toAddon->pos_time(kodi.inputstream, ms);
    }

    int64_t AddonHandler::PositionStream() {
        return kodi.inputstream->toAddon->position_stream(kodi.inputstream);
    }

    int64_t AddonHandler::LengthStream() {
        return kodi.inputstream->toAddon->length_stream(kodi.inputstream);
    }

    bool AddonHandler::IsRealTimeStream() {
        return kodi.inputstream->toAddon->is_real_time_stream(kodi.inputstream);
    }

    bool AddonHandler::SeekTime(double time, bool backwards, double *startpts) {
        return kodi.inputstream->toAddon->demux_seek_time(kodi.inputstream, time, backwards, startpts);
    }

    const char* AddonHandler::GetPathList() {
        if (!kodi.inputstream->toAddon->get_path_list) // does lib provide this function
            return nullptr;

        return kodi.inputstream->toAddon->get_path_list(kodi.inputstream);
    }

    bool AddonHandler::GetTimes(struct INPUTSTREAM_TIMES* times) {
        if (!kodi.inputstream->toAddon->get_times) // does lib provide this function
            return false;

        return kodi.inputstream->toAddon->get_times(kodi.inputstream, times);
    }

    int AddonHandler::GetChapter() {
        if (!kodi.inputstream->toAddon->get_chapter) // does lib provide this function
            return -1;

        return kodi.inputstream->toAddon->get_chapter(kodi.inputstream);
    }

    int AddonHandler::GetChapterCount() {
        if (!kodi.inputstream->toAddon->get_chapter_count) // does lib provide this function
            return -1;

        return kodi.inputstream->toAddon->get_chapter_count(kodi.inputstream);
    }

    const char* AddonHandler::GetChapterName(int ch) {
        if (!kodi.inputstream->toAddon->get_chapter_name) // does lib provide this function
            return nullptr;

        return kodi.inputstream->toAddon->get_chapter_name(kodi.inputstream, ch);
    }

    int64_t AddonHandler::GetChapterPos(int ch) {
        if (!kodi.inputstream->toAddon->get_chapter_pos) // does lib provide this function
            return -1;

        return kodi.inputstream->toAddon->get_chapter_pos(kodi.inputstream, ch);
    }

    bool AddonHandler::SeekChapter(int ch) {
        if (!kodi.inputstream->toAddon->seek_chapter) // does lib provide this function
            return -1;

        return kodi.inputstream->toAddon->seek_chapter(kodi.inputstream, ch);
    }

    int AddonHandler::BlockSizeStream() {
        if (!kodi.inputstream->toAddon->block_size_stream) // does lib provide this function
            return false;

        return kodi.inputstream->toAddon->block_size_stream(kodi.inputstream);
    }
    
    int AddonHandler::Read(uint8_t* buf, int buf_size)
    {
        if (!kodi.inputstream->toAddon->read_stream)
            return -2;

        return kodi.inputstream->toAddon->read_stream(kodi.inputstream, buf, buf_size);
    }

    int AddonHandler::GetType(int ID)
    {
        for (int i=0;i<max_ID_type;i++) {
            if (ID_type[i].ID == ID)
                return ID_type[i].type;
        }
        return -1;
    }

    int AddonHandler::GetTotalTime(int streamID, bool enable) {  // Time in Milliseconds
        return kodi.inputstream->toAddon->get_total_time(kodi.inputstream); 
    }

    int AddonHandler::GetTime() {       // Time in Milliseconds
        return kodi.inputstream->toAddon->get_time(kodi.inputstream);
    }

    int64_t AddonHandler::SeekStream(int64_t position, int whence) {
        if (!kodi.inputstream->toAddon->seek_stream) // does lib provide this function
            return -1;

        return kodi.inputstream->toAddon->seek_stream(kodi.inputstream, position, whence);
    }

    char * AddonHandler::get_user_path(void * kodiBase) {
        char *c = strdup(path.c_str());
        return c;
    } 

    bool AddonHandler::AddProp(const char *id, const char *value)
    {
        props.m_ListItemProperties[props.m_nCountInfoValues].m_strKey = strdup(id);
        props.m_ListItemProperties[props.m_nCountInfoValues].m_strValue = strdup(value);
        props.m_nCountInfoValues++;
        return true;
    }

    bool AddonHandler::AddSettingString(void *hdl, char *id, const char * value) {
        settings[n_settings].id = strdup(id);
        settings[n_settings].deflt = strdup(value);
        n_settings++;
        return true;
    }

    int AddonHandler::LoadXML(std::string xmlfile) {
        return ReadXML(xmlfile.c_str());
    }
    

