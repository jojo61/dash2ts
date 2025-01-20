using namespace tinyxml2;

#define MAXPROP 40

#ifndef XMLCheckResult
	#define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { printf("Error: %i\n", a_eResult); return a_eResult; }
#endif

    struct {
        char *id;
        char *level;
        char *deflt;
    } settings[MAXPROP];
    int n_settings;

    int ReadXML(const char * xmlfile) {

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
            n_settings = j+1;
        }
        
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


    