#include <curl/curl.h>
#include <string>
#include "kodi/c-api/filesystem.h"
#include "StringUtils.h"

#define BUFFERMAX 2000000
#define MAX_INSTANCE 10

typedef std::pair<std::string, std::string> HeaderParamValue;
typedef std::vector<HeaderParamValue> HeaderParams;
typedef HeaderParams::iterator HeaderParamsIter;

static constexpr int CURL_OFF = 0L;
static constexpr int CURL_ON = 1L;

struct ch {
struct curl_slist *m_header=NULL;
std::string   m_protoLine;
HeaderParams m_params;
unsigned char *streambuffer=NULL;
int maxstreambuffer;
int writeptr,readptr;
void *curl;
std::string m_postdata;
int post;
int fp;
char *useragent;
};

struct ch curlhandler[10];

int get_curl_instance() {
    for (int i =0;i<MAX_INSTANCE;i++)
        if (curlhandler[i].curl == NULL)
            return i;
    return -1;
}

extern const char * GetValue(const char *,struct ch *);
/* curl calls this routine to get more data */
size_t write_callback(char *buffer,
               size_t size,
               size_t nitems,
               void *userp)
{
    struct ch *c = (struct ch*) userp;
    if(userp == NULL) return 0;
    if (c->writeptr == 0) {  // First call after open needs to get buffer
        int anz;
        const char *value;

        if (c->streambuffer)
            free(c->streambuffer);

        value = GetValue((const char *)"content-length",c);
        if (value) {
            anz = atoi((char*)value);
            if (anz > BUFFERMAX) {
                c->streambuffer = (unsigned char*)malloc(anz);
                c->maxstreambuffer = anz;
            }
            else {
                c->streambuffer = (unsigned char*)malloc(BUFFERMAX);
                c->maxstreambuffer = BUFFERMAX;
            }
            //printf("Open for %d Bytes\n",maxstreambuffer);
        }
        else {
            c->streambuffer = (unsigned char*)malloc(BUFFERMAX);
            c->maxstreambuffer = BUFFERMAX;
        }
    }
    int total = size * nitems;

    if (total + c->writeptr > c->maxstreambuffer) {
        printf("Bufferoverflow on write_callback\n");
        c->writeptr = c->maxstreambuffer + 1;
        return total;
    }
    
    memcpy(c->streambuffer + c->writeptr,buffer,total);
    c->writeptr += total;

    //printf("Got %d Bytes\n",size * nitems);
    return total; 
}

double get_file_download_speed(void* Kodibase, void *curl){
    struct ch *c = (struct ch *)curl;
    curl_off_t speed;
    double fspeed;
    curl_easy_getinfo(c->curl, CURLINFO_SPEED_DOWNLOAD_T, &speed);
    //printf(" Download Speed %ld bps\n",speed);
    fspeed = speed;
    return fspeed;
}

ssize_t read_file(void* KodiBase, void* curl, void *ptr, size_t size) {
    struct ch *c = (struct ch *)curl;
    int readsize = c->writeptr - c->readptr;

    if (c->writeptr == c->maxstreambuffer + 1)
        return -1;              // Error in Fileread
    if (readsize == 0) 
        return 0;               // EOF
    if (readsize > (int)size)
        readsize = size;       // no enough readbuffer avail.
    if (c->readptr + readsize > c->maxstreambuffer) 
        return -1;              // should not happen
    memcpy(ptr,c->streambuffer+c->readptr,readsize);
    c->readptr += readsize;
    return readsize;
}

bool ParseLine(const std::string& headerLine,struct ch *c)
{
  const size_t valueStart = headerLine.find(':');

  if (valueStart != std::string::npos)
  {
    std::string strParam(headerLine, 0, valueStart);
    std::string strValue(headerLine, valueStart + 1);
    std::string XVAL("x-vxpl");

    Trim(strParam, " \t");
    ToLower(strParam);

    Trim(strValue, " \t\r\n");

    if (strParam == XVAL)
        return true;

    if (!strParam.empty() && !strValue.empty()) {
        for (HeaderParams::const_iterator iter = c->m_params.begin(); iter != c->m_params.end(); ++iter) {
            if (iter->first.c_str() && iter->first == strParam) {
                c->m_params.erase(iter);
                --iter;
            }
        }
        c->m_params.emplace_back(strParam, strValue);
    }
    else
        return false;
  }
  else 
    if (strlen(headerLine.c_str()) > 2) {
        c->m_protoLine = headerLine;
    }
    


  return true;
}

size_t header_callback(void *ptr, size_t size, size_t nmemb, void *curl)
{
  struct ch *c = (struct ch *)curl;
  
  std::string inString;
  // libcurl doc says that this info is not always \0 terminated
  const char* strBuf = (const char*)ptr;
  const size_t iSize = size * nmemb;
  if (strBuf[iSize - 1] == 0)
    inString.assign(strBuf, iSize - 1); // skip last char if it's zero
  else
    inString.append(strBuf, iSize);

  ParseLine(inString,c);

  return iSize;
}

std::string GetHeader(struct ch *c)
{   
  if (c->m_protoLine.empty() && c->m_params.empty())
    return "";

  std::string strHeader(c->m_protoLine + "\r\n");

  for (HeaderParams::const_iterator iter = c->m_params.begin(); iter != c->m_params.end(); ++iter)
    strHeader += ((*iter).first + ": " + (*iter).second + "\r\n");

  strHeader += "\r\n";
  return strHeader;
}

const char * GetValue(const char *name,struct ch *c) {
    for (HeaderParams::const_iterator iter = c->m_params.begin(); iter != c->m_params.end(); ++iter) {
        if (!strcmp(iter->first.c_str(),name)) {
           return iter->second.c_str();
        }
    }
    //printf("Tag %s not found\n",name);
    return nullptr;;
}

void * curl_create(void * base, const char *url) {

    
    int i = get_curl_instance();
    if (i == -1)
        return nullptr;
    if (verbose) printf("Open Handle %d URL %s\n",i,url);
    struct ch *c = &curlhandler[i];    
    c->curl = curl_easy_init();
    if (c->curl) {
        curl_easy_setopt(c->curl, CURLOPT_URL, url);
        c->readptr = 0;
        c->writeptr = 0;
        c->fp = -1;
    }
    return c;

}

bool curl_open(void *base, void *curl, unsigned int flags) {


    //printf("curl open\n");
    struct ch *c = (struct ch *)curl;
    if (c->curl) {
        //curl_easy_setopt(c->curl, CURLOPT_URL, url);
        curl_easy_setopt(c->curl, CURLOPT_NOPROGRESS, 1L);
        curl_easy_setopt(c->curl, CURLOPT_USERAGENT, c->useragent);
        curl_easy_setopt(c->curl, CURLOPT_MAXREDIRS, 50L);
        curl_easy_setopt(c->curl, CURLOPT_TCP_KEEPALIVE, 1L);
        curl_easy_setopt(c->curl, CURLOPT_HTTPHEADER, c->m_header);
        curl_easy_setopt(c->curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(c->curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(c->curl, CURLOPT_ACCEPT_ENCODING, "");


        std::string response_string;
        std::string header_string;
        curl_easy_setopt(c->curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(c->curl, CURLOPT_HEADERFUNCTION, header_callback);
        curl_easy_setopt(c->curl, CURLOPT_WRITEDATA, c);
        curl_easy_setopt(c->curl, CURLOPT_HEADERDATA, c);
        if (c->post) {
            curl_easy_setopt(c->curl, CURLOPT_POST,1);
            curl_easy_setopt(c->curl, CURLOPT_POSTFIELDS, c->m_postdata.c_str());
            curl_easy_setopt(c->curl, CURLOPT_POSTFIELDSIZE, c->m_postdata.length());
        }
        //curl_easy_setopt(c->curl, CURLOPT_DEBUGFUNCTION, debug_callback);
        curl_easy_setopt(c->curl, CURLOPT_VERBOSE, CURL_OFF);
        curl_easy_setopt(c->curl, CURLOPT_COOKIEFILE, "");
        curl_easy_setopt(c->curl, CURLOPT_COOKIELIST, "FLUSH");
        curl_easy_setopt(c->curl, CURLOPT_REFERER, NULL);
        curl_easy_setopt(c->curl, CURLOPT_AUTOREFERER, CURL_OFF);
        curl_easy_setopt(c->curl, CURLOPT_CONNECTTIMEOUT, 10L);

        curl_easy_perform(c->curl);

        char *eurl;
        char *scheme;
        long response_code;
        double elapsed;
        curl_easy_getinfo(c->curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_getinfo(c->curl, CURLINFO_TOTAL_TIME, &elapsed);
        curl_easy_getinfo(c->curl, CURLINFO_EFFECTIVE_URL, &eurl);
        curl_easy_getinfo(c->curl, CURLINFO_SCHEME, &scheme);

        //printf("eurl %s\n",eurl);
        //printf("Scheme %s\n",scheme);
        //printf("Response %ld\n",response_code);
    }
    return true;
}
const std::string m_characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                         "abcdefghijklmnopqrstuvwxyz"
                                         "0123456789+/";

void Decode(const char* input, unsigned int length, std::string &output)
{
  if (input == NULL || length == 0)
    return;

  long l;
  output.clear();

  for (unsigned int index = 0; index < length; index++)
  {
    if (input[index] == '=')
    {
      length = index;
      break;
    }
  }

  output.reserve(length - ((length + 2) / 4));

  for (unsigned int i = 0; i < length; i += 4)
  {
    l = ((((unsigned long) m_characters.find(input[i])) & 0x3F) << 18);
    l |= (((i + 1) < length) ? ((((unsigned long) m_characters.find(input[i + 1])) & 0x3F) << 12) : 0);
    l |= (((i + 2) < length) ? ((((unsigned long) m_characters.find(input[i + 2])) & 0x3F) <<  6) : 0);
    l |= (((i + 3) < length) ? ((((unsigned long) m_characters.find(input[i + 3])) & 0x3F) <<  0) : 0);

    output.push_back((char)((l >> 16) & 0xFF));
    if (i + 2 < length)
      output.push_back((char)((l >> 8) & 0xFF));
    if (i + 3 < length)
      output.push_back((char)((l >> 0) & 0xFF));
  }
}

bool curl_add_option(void* kodiBase, void* curl, int type, const char* name, const char* value){
    struct ch *c = (struct ch *)curl;
    switch (type) {
        case ADDON_CURL_OPTION_HEADER:
        case ADDON_CURL_OPTION_PROTOCOL:
            char tmp[3000];     
            if (!strcmp("postdata",name)) {
                std::string out;
                c->post = 1;
                Decode(value,strlen(value),c->m_postdata);
                //printf("Post %d Bytes\n",strlen(value));
                break;
            }
            else if (!strcmp("User-Agent",name)) {
                c->useragent = strdup(value);
                break;
            }
            else {
                //printf("ADD Header %s: %s\n",name,value);
                sprintf(tmp,"%s: %s",name,value); 
                c->m_header = curl_slist_append(c->m_header, (const char *)tmp);
            }
            //printf("Option: %s\n",tmp);
            break;
        default:
            printf("Add Unknown Option Type %d Name %s Value %s\n",type,name,value);
            return false;
            break;
    }
    return true;
}

void * open_file_for_write(void* kodi, const char *filename, bool overwrite) {
    int i = get_curl_instance();
    if (i == -1)
        return nullptr;
    if (verbose) printf("Open Handle %d File %s\n",i,filename);
    struct ch *c = &curlhandler[i]; 
    if (overwrite)
        c->fp = open(filename,O_CREAT|O_RDWR, 0644);
    else
        c->fp = open(filename,O_APPEND|O_RDWR, 0644);
    c->curl = (void *)1;
    return c;
}

ssize_t write_file(void *kodi, void* curl, const void  *p, size_t size) {
    struct ch *c = (struct ch *)curl;
    //printf("Filewrite %d bytes\n",size);
    return write(c->fp,p,size);
}

void close_file (void* kodiBase, void* curl) {
    
    struct ch *c = (struct ch *)curl;

    //printf("Close %p Curl %p fp %d\n",c,c->curl,c->fp);
    if (c->fp >= 0 && c->curl == (void*)1) {
        close(c->fp);
        c->fp = -1;
        c->curl = NULL;
        return;
    }
    curl_easy_cleanup(c->curl);
    curl_slist_free_all(c->m_header);
    if (c->streambuffer)
        free(c->streambuffer);
    c->streambuffer = NULL;
    c->m_header=NULL;
    c->curl = NULL;
    c->m_params.clear();
    //printf("close_file\n");
}

char ** get_property_values(void* kodiBase, void* curl, int type, const char* name, int* numvalues)
{
    
    char** ret = static_cast<char**>(malloc(sizeof(char*) * 1));
    const char * r;
    struct ch *c = (struct ch *)curl;

    //printf("GetProperty Type %d >%s< \n",type,name);
    switch (type) {
        case ADDON_FILE_PROPERTY_RESPONSE_PROTOCOL:        
            //printf("Get Protoline %s\n",m_protoLine.c_str());
            ret[0] = strdup(c->m_protoLine.c_str());
            *numvalues = 1;
            return ret;
            break;
        case ADDON_FILE_PROPERTY_RESPONSE_HEADER:
            r = GetValue(name,c);
            //printf("get response header %s -> %s\n",name,r);
            if (r) {
                ret[0] = strdup(r);
                *numvalues = 1;
                return ret;
            } else {
              return nullptr;
            }
            break;
        case ADDON_FILE_PROPERTY_CONTENT_TYPE:
            printf("Get contenttype\n");
            break;
        case ADDON_FILE_PROPERTY_CONTENT_CHARSET:
            break;
        case ADDON_FILE_PROPERTY_MIME_TYPE:
            break;
        case ADDON_FILE_PROPERTY_EFFECTIVE_URL:
            char *eurl;
            curl_easy_getinfo(c->curl, CURLINFO_EFFECTIVE_URL, &eurl);
            //printf("Get eurl %s\n",eurl);
            ret[0] = strdup(eurl);
            *numvalues = 1;
            return ret;
            break;
        default:
            printf("Wrong GetProperty Type %d\n",type);
            break;
    }

    return nullptr;
}

extern std::string path;
char * translate_special_protocol(void * kodiBase, const char *proto) {
    char *c = strdup("");
    if (!strcmp("special://xbmcbinaddons/inputstream.adaptive/",proto)) {
        std::string newpath = path + "/addons/inputstream.adaptive";
        c = strdup(newpath.c_str());
        printf("new path %s\n",c);
        return c;
    }
    //printf("translate >%s<\n",proto);
    c = strdup(proto);
    return c;
}


bool remove_directory(void * kodiBase, const char *dir) {
    if (verbose)
        printf("Remove Directory %s\n",dir);
    return true;
} 

bool create_directory(void * kodiBase, const char *dir) {
    if (verbose)
        printf("Test and Create Directory %s\n",dir);
    return true;
} 

char * get_addon_info(void * kodiBase, const char *item) {
    if (verbose)
        printf("Get Addon Info Item %s \n",item);
    return nullptr;
} 

std::string mypath,myfile;
void SplitFilename (const std::string& str)
{
  
  std::size_t found = str.find_last_of("/\\");
  mypath =  str.substr(0,found);
  myfile =  str.substr(found+1);
}

bool get_directory(void * kodiBase, 
                    const char *path,
                    const char* mask, 
                    struct VFSDirEntry** items, 
                    unsigned int* num_items) {
    if (verbose)
        printf("Read Directory %s\n",path);

    static VFSDirEntry item;
    std::string dirpath = path;
    std::size_t pos;
    *num_items=0;
    *items = &item;
    if (!strcmp(path,""))
       return true;

    for (const auto & entry : std::filesystem::directory_iterator(dirpath)) {
        std::string tmp = entry.path();
        if ( tmp.find("ssd_") != std::string::npos) {
            SplitFilename(tmp);
            item.label = strdup(myfile.c_str());
            item.path = strdup(tmp.c_str());
            if (verbose) printf("Got File %s in %s\n",item.label,item.path);
            *num_items = 1;
            *items = &item;
            return true;
        }
    }
    return false;
} 

void free_directory(void* kodiBase, struct VFSDirEntry* items, unsigned int num_items) {
    return ;
}





