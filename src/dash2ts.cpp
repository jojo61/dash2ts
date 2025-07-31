

#include <unistd.h>
#include <signal.h>
#include "streamplayer.h"
#include "addonhandler.h"

static const char *const VERSION = "1.00"
#ifdef GIT_REV
    "-GIT-" GIT_REV
#endif
    ;

extern std::string headers;
extern bool verbose;

std::string& Trim(std::string &, const char* const );

void usage() {
    printf("dash2ts Version %s\n",VERSION);
    printf("Usage: dash2ts -u url_to_manifest.mpd\n");
    printf("               -p portnr\n");
    printf("              [-k path_to_kodi]\n");
    printf("              [-h http_headers]\n");
    printf("              [-d drm_token] or [-w widevine_url]\n");
    printf("              [-v] \n");
    printf("       -v Enable Debug Verbose\n");
    printf("       path_to_kodi default is /storage/.kodi\n");
    printf("       http_headers default is %s\n",headers.c_str());
    printf("       only -d or -w is allowed. Not both\n");
    exit(0);
}

StreamPlayer *p;
AddonHandler *h;

void signal_callback_handler(int signum)
{
    printf("Caught signal %d\n",signum);
    // Cleanup and close up stuff here
    // Terminate program
    if (p)
        delete p;
    if (h)
        delete h;
    exit(signum);
}

int
main(int argc, char *argv[])
{

    std::string path_to_kodi;
    
    
#if 0
    char *test1 = "https://livesim.dashif.org/livesim/chunkdur_1/ato_7/testpic4_8s/Manifest.mpd";
    char *test2 = "https://apasfiis.sf.apa.at/dash/cms-worldwide/2024-12-08_1734_tl_02_Advent-in-Vorar_____14254423__o__1040094888__s15775910_0__ORF2BHD_17335613P_18215516P_QXB.mp4/manifest.mpd";
    char *test3 = "http://localhost/video/manifest.mpd";
    char *test4 = "https://orf3-247.mdn.ors.at/orf/orf3/qxa-247/manifest.mpd";
    char *test5 = "https://orf2-247.mdn.ors.at/orf/orf2/qxa-247/manifest.mpd";
    char *test6 = "https://media.axprod.net/TestVectors/v6.1-MultiDRM-MultiKey/Manifest_1080p.mpd";
#endif
    
    char *url = NULL;
    int api_version = 0;
    int portno;
    
    std::string orf_widevine_url ="https://drm.ors.at/acquire-license/widevine?BrandGuid=13f2e056-53fe-4469-ba6d-999970dbe549&userToken=";
    std::string cenc = "com.widevine.alpha";
    std::string drm_token;
    std::string widevine_url;
    
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

    // Register signal and signal handler
    signal(SIGINT, signal_callback_handler);

    h = new AddonHandler(path_to_kodi); // Init AddonHandler
    p = new StreamPlayer(portno);       // Init Player 

    // Load the inputstream.adaptive Library and get the API Version
    api_version = h->LoadAddon();

    // Make Properties for inputstream-adaptive
    if (widevine_url.size()) {
#if 0
        std::string prop = cenc + "|" + widevine_url + "|" + headers;
        if (api_version >= 1) {
            h->AddProp("inputstream.adaptive.drm_legacy",prop.c_str());
        } else {
        
            prop = widevine_url + "|" + headers + "|R{SSM}|R";
#endif
            std::string prop = widevine_url;
            h->AddProp("inputstream.adaptive.license_key",prop.c_str());
            h->AddProp("inputstream.adaptive.license_type",cenc.c_str());
        //} 
    } else {
        std::string prop = cenc + "|" + orf_widevine_url + drm_token + "|" + headers;
        if (api_version >= 1) {
            h->AddProp("inputstream.adaptive.drm_legacy",prop.c_str());
        } else {
            prop = orf_widevine_url + drm_token + "|" + headers + "|R{SSM}|R";
            h->AddProp("inputstream.adaptive.license_key",prop.c_str());
            h->AddProp("inputstream.adaptive.license_type",cenc.c_str());
        }
    }
    
    h->SetResolution(1920,1080);

    // Finaly open the URL 
    bool ret = h->OpenURL(url);

    if (ret) {
        p->StreamPlay(h);  // Get Stream and send it to TCP Port
    }
    else {
        printf("Can't Open URL\n");
    }
    delete h;
    delete p;

    exit(EXIT_SUCCESS);
}
