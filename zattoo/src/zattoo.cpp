#include <sys/stat.h>
#include <unistd.h>
#include "kodi.h"
#include "Session.h"
#include "http/HttpClient.h"
#include "ZatData.h"
//#include "http/Curl.h"


bool verbose=false;
std::string path = "/home/jojo/.kodi";

   // template<typename... Args>
    void Log(int level,const char *format, ...)
    {
        va_list args;
        if (!verbose)
            return;

        va_start(args, format);
        vprintf(format, args);
        va_end(args);
        printf("\n");
    }
#if 0
   void Log(int level,const char *format)
    {
        printf(format);
        printf("\n");
    }
#endif
    std::string GetUserPath(std::string path) {
        std::string mp = path;
        return mp;
    }

    std::string GetAddonPath(std::string path) {
        std::string mp = path;
        return mp;
    }

    std::string UserPath() {
        std::string mp = path + "/userdata/addon_data/pvr.zattoo/"; // path+"/addons/pvr.zattoo";
        return mp;
    }

    bool FileExists(std::string name, bool flag) {
        struct stat buffer;   
        return (stat (name.c_str(), &buffer) == 0); 
    }




std::string portno;
std::string widevine_url,headers;


int main(int argc, char *argv[]) {
    
    PVRStreamProperty properties;
    bool print_channels = false;
    bool get_epg = false;
    int uniqueID=0;
    std::string url;
    std::string license;
    
    int c;
    while ((c = getopt (argc, argv, "u:p:k:w:h:vce")) != -1) {
        switch (c) {
            
            case 'p': // Portnr
                portno.append(optarg);
                continue;
            case 'k': // got new Path to Kodi
                path.clear();
                path.append(optarg);
                continue;
            case 'u':
                uniqueID = atoi(optarg);
                continue;
            case 'w': // Widevine URL
                widevine_url.append(optarg);
                //Trim(widevine_url,"\"");
                continue;
            case 'h': // Set new Headers
                headers.clear();
                headers.append(optarg);
                continue;
            case 'c': // Print channellist
                print_channels = true;
                continue; 
            case 'e': // Get EPG
                get_epg = true;
                continue; 
            case 'v': // Verbose
                verbose = true;
                continue;             
            default:
                //usage();
                exit(0);
        }
        break;
    }
    if (verbose) printf("-------Start---------\n");
    //auto curl = new Curl();
    std::string pathdb = path + "/userdata/addon_data/pvr.zattoo/";
#if 0
    auto parameterDB = new ParameterDB(pathdb);
    auto httpClient = new HttpClient(parameterDB);
    auto settings = new CSettings();
    auto zatData = new ZatData();
    auto session = new Session(httpClient,zatData,settings,parameterDB);
#else
    auto zatData = new ZatData();
#endif

    //settings->Load();
    bool login = zatData->Create();
    //bool login = session->LoginThread();


    if (login) {
        zatData->LoadChannels();
        std::string channels = zatData->GetChannels();

        if (print_channels) {
            printf("%s",channels.c_str());
            exit(0);
        }
        if (get_epg) {
            time_t start = time(0);
            time_t end = start + 8 * 3600;  // 4 hours
            zatData->GetEPGForChannelAsync(0,start,end);
            exit(0);
        }
        if (verbose) printf(" Channel ID %d",uniqueID);
        zatData->GetChannelStreamProperties(uniqueID,properties);
        std::vector<StreamParamValue>::iterator it;
        for (it = properties.begin(); it != properties.end(); ++it)
        {
            if (!strcmp(it->first.c_str(),"streamurl")) {
                url = it->second;
            }
            if (!strcmp(it->first.c_str(),"inputstream.adaptive.license_key")) {
                license = it->second;
            }
            printf(">%s< >%s<",it->first.c_str(),it->second.c_str());
            
        }
        if (url.size()) {
            int pid;
            pid = fork();
            if (pid < 0) exit (0); //fork failed
            if (pid == 0) {
                std::string cmd;
                cmd = "dash2ts -u "+url+" -p "+portno+" -k "+path+ " -w \""+license+"\"";
                printf("Start %s\n",cmd.c_str());
                system(cmd.c_str());
            
            } 
        }
    }

    exit(0);
}