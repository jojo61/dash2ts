
#define DMX_SPECIALID_STREAMINFO -10
#define DMX_SPECIALID_STREAMCHANGE -11



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
} ID_type[INPUTSTREAM_MAX_STREAM_COUNT];
int max_ID_type=0;

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

    ID_type[max_ID_type].ID = streamId;
    ID_type[max_ID_type].type = stream->m_streamType;
    max_ID_type++;

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
        videoconvert = BitstreamConverterOpen(codec->id, (uint8_t*)stream->m_ExtraData, stream->m_ExtraSize, true);
        myfile.write((const char *) stream->m_ExtraData, stream->m_ExtraSize);
    } else 
        if (stream->m_streamType == INPUTSTREAM_TYPE_AUDIO) {
           AudioConverterOpen(stream);
        }

    return nullptr;
}

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
    // void addon_log_msg(const KODI_ADDON_BACKEND_HDL hdl, const int level, const char *msg);
    // void OpenAddon(char * lib);
    AddonHandler() {}
    ~AddonHandler() { dlclose(handle); }

    void LoadAddon(std::string addonlib)
    {

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

        m_interface.toKodi->kodi_addon->get_user_path = get_user_path;

        kodi.inputstream->toKodi->free_demux_packet = cb_free_demux_packet;
        kodi.inputstream->toKodi->allocate_demux_packet = cb_allocate_demux_packet;
        kodi.inputstream->toKodi->allocate_encrypted_demux_packet = cb_allocate_encrypted_demux_packet;

        m_interface.toKodi->addon_log_msg = addon_log_msg;
        m_interface.toKodi->free_string = free_string;
        m_interface.toKodi->free_string_array = free_string_array;
        m_interface.toKodi->kodi_gui->general->get_adjust_refresh_rate_status = get_adjust_refresh_rate_status;
        
        handle = dlopen(addonlib.c_str(), RTLD_LAZY);
        if (!handle)
        {
            fprintf(stderr, " Error Open inputstream-adaptive lib: %s\n", dlerror());
            exit(EXIT_FAILURE);
        }

        dlerror(); /* Clear any existing error */

        // cosine = (double (*)(double)) dlsym(handle, "open");

        createfunc = (int (*)(void *))dlsym(handle, "ADDON_Create");
        error = dlerror();
        if (error)
            printf("Error %s\n", error);
        gettypeversionfunc = (char *(*)(int))dlsym(handle, "ADDON_GetTypeVersion");
        error = dlerror();
        if (error)
            printf("Error %s\n", error);
        gettypeminversionfunc = (char *(*)(int))dlsym(handle, "ADDON_GetTypeMinVersion");
        error = dlerror();
        if (error)
            printf("Error %s\n", error);
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

    }

    bool OpenAddon(char *url) {
        
        props.m_strURL = url;
        props.m_mimeType = "";
        //props.m_nCountInfoValues = 0;
        
        bool status = kodi.inputstream->toAddon->open(kodi.inputstream, &props);
        //printf("Open status %d\n", status);
        return status;
    }

    void SetResolution(unsigned int width,unsigned int height) {
        kodi.inputstream->toAddon->set_video_resolution(kodi.inputstream, width,height,width,height);
    }

    bool GetStreamIDs() {   
        max_ID_type=0;  // Reset ID Mapping 
        if (kodi.inputstream->toAddon->get_stream_ids(kodi.inputstream,&IDs))
            return true;
        return false;
    }

    void EnableStream(int streamID, bool enable) {
        kodi.inputstream->toAddon->enable_stream(kodi.inputstream,streamID,enable);
    }

    void OpenStream(int streamID) {
        kodi.inputstream->toAddon->open_stream(kodi.inputstream,streamID);
    }

    void Close() {
        kodi.inputstream->toAddon->close(kodi.inputstream);
    }

    void DemuxFlush() {
        kodi.inputstream->toAddon->demux_flush(kodi.inputstream);
    }

    int GetCapabilities() {
        struct INPUTSTREAM_CAPABILITIES caps;
        kodi.inputstream->toAddon->get_capabilities(kodi.inputstream,&caps);
        printf("Capabilities %04x\n",caps.m_mask);
        return caps.m_mask;
    }

    INPUTSTREAM_INFO* GetStream(int streamId) const
    {
        
        KODI_HANDLE demuxStream = nullptr;
        bool ret = kodi.inputstream->toAddon->get_stream(kodi.inputstream, streamId, &stream,
                                                            &demuxStream, cb_get_stream_transfer);
        if (!ret || stream.m_streamType == INPUTSTREAM_TYPE_NONE)
            return nullptr;

        //printf("Getstream Extra Data %p Size %d\n",stream.m_ExtraData,stream.m_ExtraSize);

        return &stream;
    }

    struct DEMUX_PACKET* DemuxRead() 
    {
        struct DEMUX_PACKET* demux;

        demux = kodi.inputstream->toAddon->demux_read(kodi.inputstream);
        return demux;
    }

    int Read(uint8_t* buf, int buf_size)
    {
        if (!kodi.inputstream->toAddon->read_stream)
            return -2;

        return kodi.inputstream->toAddon->read_stream(kodi.inputstream, buf, buf_size);
    }

    int GetType(int ID)
    {
        for (int i=0;i<max_ID_type;i++) {
            if (ID_type[i].ID == ID)
                return ID_type[i].type;
        }
        return -1;
    }

    bool AddProp(const char *id, const char *value)
    {
        props.m_ListItemProperties[props.m_nCountInfoValues].m_strKey = strdup(id);
        props.m_ListItemProperties[props.m_nCountInfoValues].m_strValue = strdup(value);
        props.m_nCountInfoValues++;
        return true;
    }
};
