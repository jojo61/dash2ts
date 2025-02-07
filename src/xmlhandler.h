using namespace tinyxml2;

#define MAXPROP 40

#ifndef XMLCheckResult
	#define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { printf("XML Error: %i\n", a_eResult); return a_eResult; }
#endif

    struct {
        char *id;
        char *level;
        char *deflt;
    } settings[MAXPROP];
    int n_settings;

    int ReadXML(const char * xmlfile) {
#if 0
        XMLDocument doc;
        int j=0;

        XMLError eResult = doc.LoadFile(xmlfile);
        XMLCheckResult(eResult);
        
        std::vector<const char*> elems = {"level","default"};

        XMLElement * p_root_element = doc.FirstChildElement();
        
        XMLElement * p_setting = p_root_element->FirstChildElement("section"); 
        p_setting = p_setting->FirstChildElement("category"); 
        p_setting = p_setting->FirstChildElement("group");
        p_setting = p_setting->FirstChildElement("setting"); 
        const char *text = p_setting->Attribute("id");
        settings[j].id = strdup(text);

        while(p_setting && n_settings < MAXPROP){
            for (std::size_t i{}; i < elems.size(); ++i){
                tinyxml2::XMLElement * ptr = p_setting->FirstChildElement(elems[i]); 
                //std::cout << i << elems[i] << ": " << ptr->GetText() << '\n';
                if (i==0) {
                    settings[j].level = strdup(ptr->GetText());
                }
                if (i==1) {
                    settings[j].deflt = strdup(ptr->GetText());
                }
            }
            p_setting = p_setting->NextSiblingElement("setting");
            if (p_setting) {
                text = p_setting->Attribute("id");
                settings[++j].id = strdup(text);
            }
            printf("XML ID >%s< Value >%s<\n",settings[j-1].id,settings[j-1].deflt);
            n_settings = j+1;
        }
        
        
#else
    int j = 0;
    settings[j].id      = strdup("adaptivestream.res.max");
    settings[j++].deflt = strdup("auto");
    settings[j].id      = strdup("adaptivestream.type");
    settings[j++].deflt = strdup("default");
    settings[j].id      = strdup("adaptivestream.res.secure.max");
    settings[j++].deflt = strdup("auto");
    settings[j].id      = strdup("adaptivestream.bandwidth.init.auto");
    settings[j++].deflt = strdup("true"); 
    settings[j].id      = strdup("adaptivestream.bandwidth.init");
    settings[j++].deflt = strdup("4000");  
    settings[j].id      = strdup("adaptivestream.bandwidth.min");
    settings[j++].deflt = strdup("0");
    settings[j].id      = strdup("adaptivestream.bandwidth.max");
    settings[j++].deflt = strdup("1000000");
    settings[j].id      = strdup("adaptivestream.streamselection.mode");
    settings[j++].deflt = strdup("manual-v");
    settings[j].id      = strdup("adaptivestream.test.mode");
    settings[j++].deflt = strdup("switch-segments");
    settings[j].id      = strdup("MEDIATYPE");
    settings[j++].deflt = strdup("0");
    settings[j].id      = strdup("MAXBUFFERDURATION");
    settings[j++].deflt = strdup("120");
    settings[j].id      = strdup("ASSUREDBUFFERDURATION");
    settings[j++].deflt = strdup("60");
    settings[j].id      = strdup("HDCPOVERRIDE");
    settings[j++].deflt = strdup("false");
    n_settings = j;
#endif

        return EXIT_SUCCESS;
    }

    int FindId(const char* id) {
        for (int i=0;i<n_settings;i++) {
            if (!strcmp(settings[i].id,id)) {
                return i;
            }
        }
        printf("%s not found\n",id);
        return -1;
    }

    bool GetSettingString(void* hdl, const char *id, char **value)
    {
        int idx = FindId(id);
        if (idx >= 0) {
            *value = settings[idx].deflt;
            return true;
        }
        return false;
    }

    bool GetSettingInt(void* hdl, const char *id, int *value)
    {
        int idx = FindId(id);
        if ( idx >= 0) {
            *value = atoi(settings[idx].deflt);
            return true;
        }
        return false;
    }

    bool GetSettingFloat(void* hdl, const char *id, float *value)
    {
        int idx = FindId(id);
        if ( idx >= 0) {
            *value = atof(settings[idx].deflt);
            return true;
        }
        return false;
    }


    bool GetSettingBool(void * hdl, const char *id, bool *value)
    {
        *value = false;
        int idx = FindId(id);
        if ( idx >= 0) {
            if (!strcmp(settings[idx].deflt,"true")) {
                *value = true;
                return true;
            }
            else {
                return false;
            }
        }
        return false;
    }


    