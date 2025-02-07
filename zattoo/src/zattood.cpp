#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include "kodi.h"
#include "Session.h"
#include "http/HttpClient.h"
#include "ZatData.h"
//#include "http/Curl.h"

#define DAEMON

bool verbose=false;
bool enable_cache = false;
std::string path;
std::string myfifo = "/tmp/zattoofifo";
std::string CACHE_DIR = "/userdata/addon_data/pvr.zattoo/cache/";

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

void usage() {
    printf("Use: zattood -k <path_to_kodi> [-u <uniqueID>] [-c] [-v]\n");
    printf("                  -c Print Channel List\n");
}

std::string GetEnv( const std::string & var ) {
     const char * val = std::getenv( var.c_str() );
     if ( val == nullptr ) { // invalid to assign nullptr to std::string
         return "";
     }
     else {
         return val;
     }
}

std::string widevine_url,headers;

int main(int argc, char *argv[]) {
    
    PVRStreamProperty properties;
    bool print_channels = false;
    int uniqueID=0;
    std::string url;
    std::string license;
    char request[50];
    std::string home = "HOME";
    
    home = GetEnv(home);
    printf(" Home %s\n",home.c_str());
    path = home + "/.kodi";

    int c;
    while ((c = getopt (argc, argv, "u:k:w:h:C:vc")) != -1) {
        switch (c) {
            
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
            case 'C': // Set cache directory
                CACHE_DIR.clear();
                CACHE_DIR.append(optarg);
                CACHE_DIR.append("/zattoocache/");
                enable_cache = true;
                continue;
            case 'c': // Print channellist
                print_channels = true;
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

    if (argc < 1) {
        usage();
        exit(0);
    }

    if (verbose) printf("-------Start---------\n");
    
    std::string pathdb = path + "/userdata/addon_data/pvr.zattoo/";

    auto zatData = new ZatData();

    bool login = zatData->Create();
    
    if (login) {
        std::this_thread::sleep_for(std::chrono::seconds(2));  // Give time to connect
        while (zatData->GetState() != PVR_CONNECTION_STATE_CONNECTED) {
            printf("wait for connection\n");
            std::this_thread::sleep_for(std::chrono::seconds(3));
        }
        zatData->LoadChannels();
        std::string channels = zatData->GetChannels();

        if (print_channels) {
            printf("%s",channels.c_str());
            exit(0);
        }
        

        mkfifo(myfifo.c_str(), 0666);
        int fd;
        for (;;) {
            fd = open(myfifo.c_str(), O_RDONLY);
            ssize_t len = read(fd,request,sizeof(request));  // Wait for request
            close(fd);
            uniqueID = atoi(request);

            if (verbose) printf(" Channel ID %d",uniqueID);
            zatData->GetChannelStreamProperties(uniqueID,properties); // get stream properties
            std::vector<StreamParamValue>::iterator it;
            for (it = properties.begin(); it != properties.end(); ++it)
            {
                if (!strcmp(it->first.c_str(),"streamurl")) {
                    url = it->second;
                }
                if (!strcmp(it->first.c_str(),"inputstream.adaptive.license_key")) {
                    license = it->second;
                }
                printf(">%s< >%s<\n",it->first.c_str(),it->second.c_str());
                
            }

            if (url.size()) {
                std::string reply = url+";"+license+";"+path+";";
                //printf("reply %s\n",reply.c_str());
                int fd = open(myfifo.c_str(), O_WRONLY);
                write(fd,reply.c_str(),reply.size());       // Reply stream properties
                close(fd);
            }
            url.clear();
            url.append("ERROR");
            properties.clear();
        }
    }
    exit(0);
}