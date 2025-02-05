#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 

//#include "kodi.h"

void usage() {
    printf("Use: zattoohandler -p <port> -u <uniqueID> [-v]\n");
    
}

std::string svdrpsend(std::string& cmd, std::string& data) {

    // Open output Socket
    char buffer[1000];
    std::string result = "";
    std::string mycmd;
    if (data.size())
      mycmd = cmd+"\n";
    else
      mycmd = cmd+"\nquit\n";
    struct sockaddr_in serv_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (sockfd < 0) {
        printf("ERROR opening socket\n");
        return result;
    }

    struct hostent *server = gethostbyname("127.0.0.1");
    if (server == NULL) {
        printf("ERROR, no such host\n");
        return result;
    }
    
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
    serv_addr.sin_port = htons(6419);

    int flags = fcntl(sockfd, F_GETFL);
    fcntl(sockfd, F_SETFL, flags&~SOCK_NONBLOCK);

    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) {
        //printf("ERROR connecting\n");
        return result;
    }
    ssize_t r = write(sockfd,mycmd.c_str(),mycmd.size());
    if (data.size()) {
       r = write(sockfd,data.c_str(),data.size());
       mycmd = "quit\n";
       r = write(sockfd,mycmd.c_str(),mycmd.size());
    }
    
    ssize_t i;
    do {
      i = read(sockfd,buffer,sizeof(buffer));      
      if (i > 0)
        result.append(buffer,i);
    } while (i > 0);
    //printf(" Result %s\n",result.c_str());
    close (sockfd);
    return result;
}

#ifndef EXTSHELL
#define EXTSHELL "/bin/bash"
#endif

std::string path ;
int verbose;
std::string myfifo = "/tmp/zattoofifo";

int main(int argc, char *argv[]) {
    
    int uniqueID=0;
    std::string url;
    std::string widevine;
    std::string portno;
    char reply[1000];
    int c;
    while ((c = getopt (argc, argv, "u:p:k:v")) != -1) {
        switch (c) {
            
            case 'p': // Portnr
                portno.append(optarg);
                continue;
            case 'u':
                uniqueID = atoi(optarg);
                continue;
            case 'v': // Verbose
                verbose = true;
                continue;
            case 'k': // Legacy not used anymore
                continue;                
            default:
                usage();
                exit(0);
        }
        break;
    }

    if (argc < 3) {
        usage();
        exit(0);
    }

    mkfifo(myfifo.c_str(), 0666);

    int fd = open(myfifo.c_str(), O_WRONLY);
    std::string request = std::to_string(uniqueID)+" ";
    write (fd,request.c_str(),request.size()); // send Unique ID
    close (fd);
    fd = open(myfifo.c_str(), O_RDONLY);
    ssize_t ret = read(fd,reply,sizeof(reply)); // wait for Stream URL and Widevine URL
    close(fd);
    url.append(strtok(reply,";"));
    widevine.append(strtok(NULL,";"));
    path.append(strtok(NULL,";"));
    //printf("URL %s Widevine %s\n",url.c_str(),widevine.c_str());

    if (url != "ERROR") {        
        std::string cmd;
        cmd = "dash2ts -u "+url+" -p "+portno+" -k "+path+ " -w \""+widevine+"\"";
        if (verbose) cmd.append(" -v");
        //printf("Start %s\n",cmd.c_str());
        execl(EXTSHELL, "sh", "-c",cmd.c_str(), (char *) nullptr);        
    } else {
        std::string st = "mesg Kanal derzeit nicht verfügbar";
        std::string d;
        svdrpsend(st,d);
        exit(0);
    }
}