#include "ZattooEpgProvider.h"
#include "rapidjson/document.h"
//#include <kodi/AddonBase.h>
#include "../kodi.h"
#include "../Utils.h"
#include <ctime>
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include "../http/Cache.h"
#include "../md5.h"
#include "../ZatData.h"

extern bool enable_cache;

using namespace rapidjson;

std::mutex ZattooEpgProvider::loadedTimeslotsMutex;

ZattooEpgProvider::ZattooEpgProvider(
    void *addon,
    std::string providerUrl,
    //EpgDB &epgDB,
    HttpClient &httpClient,
    //Categories &categories,
    std::map<std::string, ZatChannel> &visibleChannelsByCid,
    std::string powerHash
  ):
  EpgProvider(addon),
  //m_epgDB(epgDB),
  m_httpClient(httpClient),
  //m_categories(categories),
  m_powerHash(powerHash),
  m_providerUrl(providerUrl),
  m_visibleChannelsByCid(visibleChannelsByCid)
{
  time(&lastCleanup);
  m_detailsThreadRunning = true;
  m_detailsThread = std::thread([&] { DetailsThread(); });
}

ZattooEpgProvider::~ZattooEpgProvider() {
  m_detailsThreadRunning = false;
  if (m_detailsThread.joinable())
    m_detailsThread.join();
}

std::string ZattooEpgProvider::svdrpsend(std::string& cmd, std::string& data , bool reuse) {

    // Open output Socket
    static int sockfd = -1;
    char buffer[1000];
    std::string result = "";
    std::string mycmd;
    if (data.size())
      mycmd = cmd+"\n";
    else
      mycmd = cmd+"\nquit\n";
      
    if (sockfd == -1) {
        struct sockaddr_in serv_addr;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        
        if (sockfd < 0) {
            printf("ERROR %d opening socket\n",sockfd);
            std::this_thread::sleep_for(std::chrono::seconds(4));
            return result;
        }
        

        struct hostent *server = gethostbyname("127.0.0.1");
        if (server == NULL) {
            printf("ERROR, no such host\n");
            close(sockfd);
            sockfd = 1;
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
            close(sockfd);
            sockfd = -1;
            return result;
        }

    }
    
    write(sockfd,mycmd.c_str(),mycmd.size());
    //printf("svdrpsend write cmd %d %d\n",mycmd.size(),r);
    if (data.size()) {
       write(sockfd,data.c_str(),data.size());
       //printf("svdrpsend write data  %d %d\n",data.size(),r);
       if (!reuse) {
          mycmd = "quit\n";
          write(sockfd,mycmd.c_str(),mycmd.size());
       }
    }
    
    ssize_t i;
    if (!reuse) { 
      do {
        i = read(sockfd,buffer,sizeof(buffer));      
        if (i > 0)
          result.append(buffer,i);
      } while (i > 0);
    }

    if (!reuse) {
      //printf("Close socket\n");
      close (sockfd);
      sockfd = -1;
    }

    return result;

}

std::string ZattooEpgProvider::GetDetails(int ProgrammId, time_t validUntil) {
    std::ostringstream urlStream;
    std::string description = "";

    urlStream << m_providerUrl << "/zapi/v2/cached/program/power_details/"
        << m_powerHash << "?complete=True&program_ids=" << std::to_string(ProgrammId);
    
    int statusCode;
    std::string jsonString = m_httpClient.HttpGetCached(urlStream.str(), 86400, statusCode, false);

    Document detailDoc;
    detailDoc.Parse(jsonString.c_str());
    if (detailDoc.GetParseError() || !detailDoc["success"].GetBool())
    {
      return description;
    }
    else
    {
      const Value& programs = detailDoc["programs"];
      for (Value::ConstValueIterator progItr = programs.Begin();
          progItr != programs.End(); ++progItr)
      {
        const Value &program = *progItr;
        description.append(Utils::JsonStringOrEmpty(program, "d"));
        
        //epgDBInfo->season = program.HasMember("s_no") && !program["s_no"].IsNull() ? program["s_no"].GetInt() : -1;
        //epgDBInfo->episode = program.HasMember("e_no") && !program["e_no"].IsNull() ? program["e_no"].GetInt() : -1;

      }
      if (description.size() && enable_cache) {  // Cache only if description is avail.
          std::string cacheKey = md5(urlStream.str());
          //time_t validUntil;
          //time(&validUntil);
          //validUntil += 86400;
          Cache::Write(cacheKey, jsonString, validUntil);
      }
    }
    return description;
}

void ZattooEpgProvider::DetailsThread()
{
  time_t start,end;
  std::string data;
  std::this_thread::sleep_for(std::chrono::seconds(10));
  Log(ADDON_LOG_DEBUG, "EPG thread started");
  Cache::Cleanup();   // Cleanup old cache files on start
  std::string cmd = "lstc";
  vdr_channels.clear();
  while (!vdr_channels.size()) {
    vdr_channels = svdrpsend(cmd,data, false);
    if (!vdr_channels.size()) {
      std::this_thread::sleep_for(std::chrono::seconds(4));
    }
  }
  while (m_detailsThreadRunning)
  {
    printf("Start EPG Detail Update\n");
    start = time(0);
    end = start + 8 * 3600;  // 8 hours
    ZattooEpgProvider::LoadEPGForChannel(start,end, true);  // Read 8h epg with Details
    printf("Start EPG additional Update\n");
    for (int i = 0;i< 3;i++) {                              // Read additional 24h EPG without Details (is anyway not available)
      start = end;
      end = end + 8 * 3600;
      ZattooEpgProvider::LoadEPGForChannel(start,end, false);
    }
    printf("Finished update wait for 1 h\n");
    std::this_thread::sleep_for(std::chrono::hours(1));  // Sleep 1h
    //std::this_thread::sleep_for(std::chrono::minutes(10));  // Sleep 1h
    Cache::Cleanup();
  }
}

bool ZattooEpgProvider::LoadEPGForChannel( time_t iStart, time_t iEnd, bool readdetails) {
  
  PVRStreamProperty properties;
  ZatChannel channel;
  std::string VDRid;
  std::string uniqueID;
  std::string epgdata;
  std::string cmd,details,data;
  
  bool more_details = true;
  //CleanupAlreadyLoaded();
  //time_t tempStart = iStart - (iStart % (3600 / 2)) - 86400;
  time_t tempStart = iStart; //SkipAlreadyLoaded(tempStart, iEnd);
  time_t tempEnd = iEnd; //tempStart + 3600 * 5; //Add 5 hours
  while (tempStart < iEnd)
  {
    if (tempEnd > iEnd) {
      tempEnd = iEnd;
    }
    
    std::ostringstream urlStream;
    urlStream << m_providerUrl << "/zapi/v3/cached/" + m_powerHash + "/guide"
        << "?end=" << tempEnd << "&start=" << tempStart
        << "&format=json";

    int statusCode;
    std::string jsonString = m_httpClient.HttpGet(urlStream.str(),statusCode);

    Document doc;
    doc.Parse(jsonString.c_str());
    if (doc.GetParseError())
    {
      Log(ADDON_LOG_ERROR, "Loading epg failed from %lu to %lu", iStart, iEnd);
      return false;
    }
    //RegisterAlreadyLoaded(tempStart, tempEnd);
    const Value& channels = doc["channels"];
    
    //std::lock_guard<std::mutex> lock(sendEpgToKodiMutex);
    //m_epgDB.BeginTransaction();
    
    for (Value::ConstMemberIterator iter = channels.MemberBegin(); iter != channels.MemberEnd(); ++iter) {
      std::string cid = iter->name.GetString();
      

      channel = m_visibleChannelsByCid[cid];
      VDRid = "I-"+std::to_string(channel.iUniqueId)+"-80-1";
      
      if (vdr_channels.find("A="+std::to_string(channel.iUniqueId)+":") == std::string::npos) {
        continue;
      }
 
      Log(ADDON_LOG_DEBUG,"Channel %s",cid.c_str());
      //printf("Channel %s\n",cid.c_str());
      //std::this_thread::sleep_for(std::chrono::milliseconds(100));
      epgdata = "C "+VDRid+"\n";
      more_details = true;
      const Value& programs = iter->value;
      for (Value::ConstValueIterator itr1 = programs.Begin();
          itr1 != programs.End(); ++itr1)
      {
        const Value& program = (*itr1);

        const Type& checkType = program["t"].GetType();
        if (checkType != kStringType)
          continue;

        time_t endTime = program["e"].GetInt();
        
        int programId = program["id"].GetInt();
        if (more_details && readdetails) {
          details = GetDetails(programId,endTime);
          if (details.size() == 0) {
            more_details = false;
          }
        }
 
        const Value& genres = program["g"];
        std::string genreString;
        for (Value::ConstValueIterator itr2 = genres.Begin();
            itr2 != genres.End(); ++itr2)
        {
          genreString = (*itr2).GetString();
          break;
        }
        //printf("Genre >%s< ",genreString.c_str());
        std::string title = Utils::JsonStringOrEmpty(program, "t"); 
        std::string subtitle = Utils::JsonStringOrEmpty(program, "et");
        time_t startTime = program["s"].GetInt();

        epgdata.append("E "+ std::to_string(programId) + " " + std::to_string(startTime) + " " + std::to_string(endTime - startTime) + "\n");  // set Event data
        epgdata.append("T "+ title + "\n");
        epgdata.append("S "+ subtitle + "\n");
        epgdata.append("D "+ details + "\n");
        epgdata.append("e \n");

        //printf("Title %s Subtitle %s from %d to %d\n",title.c_str(),subtitle.c_str(),startTime,endTime);
#if 0        
        epgDBInfo.programId = program["id"].GetInt();
        epgDBInfo.recordUntil = Utils::JsonIntOrZero(program, "rg_u");
        epgDBInfo.replayUntil = Utils::JsonIntOrZero(program, "sr_u");
        epgDBInfo.restartUntil = Utils::JsonIntOrZero(program, "ry_u");
        epgDBInfo.startTime = program["s"].GetInt();
        epgDBInfo.endTime = program["e"].GetInt();
        epgDBInfo.title = Utils::JsonStringOrEmpty(program, "t"); 
        epgDBInfo.subtitle = Utils::JsonStringOrEmpty(program, "et");
        epgDBInfo.genre = genreString;
        epgDBInfo.imageToken = Utils::JsonStringOrEmpty(program, "i_t");
        epgDBInfo.cid = cid;
        m_epgDB.Insert(epgDBInfo);
        
        SendEpgDBInfo(epgDBInfo);
#endif
      }
      epgdata.append("c \n.\n");
      cmd = "pute";
      svdrpsend(cmd,epgdata,true);
    }
    cmd = "quit";
    svdrpsend(cmd,epgdata,false);
    tempStart = tempEnd;
    tempEnd = tempStart + 3600 * 5; //Add 5 hours
  }
  return true;
}
#if 0
void ZattooEpgProvider::SendEpgDBInfo(EpgDBInfo &epgDBInfo) {
  
  if (m_visibleChannelsByCid.count(epgDBInfo.cid) == 0) {
    return;
  }
  
  int uniqueChannelId = Utils::GetChannelId(epgDBInfo.cid.c_str());
  
  kodi::addon::PVREPGTag tag;
  tag.SetUniqueBroadcastId(static_cast<unsigned int>(epgDBInfo.programId));
  tag.SetTitle(epgDBInfo.title);
  tag.SetUniqueChannelId(static_cast<unsigned int>(uniqueChannelId));
  tag.SetStartTime(epgDBInfo.startTime);
  tag.SetEndTime(epgDBInfo.endTime);
  tag.SetPlotOutline(epgDBInfo.description);
  tag.SetPlot(epgDBInfo.description);
  tag.SetEpisodeName(epgDBInfo.subtitle);
  tag.SetOriginalTitle(""); /* not supported */
  tag.SetCast(""); /* not supported */
  tag.SetDirector(""); /*SA not supported */
  tag.SetWriter(""); /* not supported */
  tag.SetYear(0); /* not supported */
  tag.SetIMDBNumber(""); /* not supported */
  tag.SetIconPath(Utils::GetImageUrl(epgDBInfo.imageToken));
  tag.SetParentalRating(0); /* not supported */
  tag.SetStarRating(0); /* not supported */
  tag.SetSeriesNumber(epgDBInfo.season);
  tag.SetEpisodeNumber(epgDBInfo.episode);  
  tag.SetEpisodePartNumber(EPG_TAG_INVALID_SERIES_EPISODE); /* not supported */
  
  std::string genreStr = epgDBInfo.genre;
  int genre = m_categories.Category(genreStr);
  if (genre)
  {
    tag.SetGenreSubType(genre & 0x0F);
    tag.SetGenreType(genre & 0xF0);
  }
  else
  {
    tag.SetGenreType(EPG_GENRE_USE_STRING);
    tag.SetGenreSubType(0); /* not supported */
    tag.SetGenreDescription(genreStr);
  }

  if (m_detailsThreadRunning) {
    SendEpg(tag);  
  }
}

void ZattooEpgProvider::CleanupAlreadyLoaded() {
  time_t now;
  time(&now);
  if (lastCleanup + 60 > now) {
    return;
  }
  lastCleanup = now;
  std::lock_guard<std::mutex> lock(loadedTimeslotsMutex);
  m_loadedTimeslots.erase(
      std::remove_if(m_loadedTimeslots.begin(), m_loadedTimeslots.end(),
          [&now](const LoadedTimeslots & o) { return o.loaded < now - 60; }),
          m_loadedTimeslots.end());
}

void ZattooEpgProvider::RegisterAlreadyLoaded(time_t startTime, time_t endTime) {
  LoadedTimeslots slot;
  slot.start = startTime;
  slot.end = endTime;
  time(&slot.loaded);
  std::lock_guard<std::mutex> lock(loadedTimeslotsMutex);
  m_loadedTimeslots.push_back(slot);
}

time_t ZattooEpgProvider::SkipAlreadyLoaded(time_t startTime, time_t endTime) {
  time_t newStartTime = startTime;
  std::lock_guard<std::mutex> lock(loadedTimeslotsMutex);
  std::vector<LoadedTimeslots> slots(m_loadedTimeslots.begin(), m_loadedTimeslots.end());
  for (LoadedTimeslots slot: slots) {
    if (slot.start <= newStartTime && slot.end > newStartTime) {
      newStartTime = slot.end;
      if (newStartTime > endTime) {
        break;
      }
    }
  }
  return newStartTime;
}

void ZattooEpgProvider::DetailsThread()
{
  std::this_thread::sleep_for(std::chrono::seconds(10));
  Log(ADDON_LOG_DEBUG, "Details thread started");
  while (m_detailsThreadRunning)
  {
    std::list<EpgDBInfo> epgDBInfos = m_epgDB.GetWithWhere("DETAILS_LOADED=0 order by abs(strftime('%s','now')-END_TIME) limit 20;");
    Log(ADDON_LOG_DEBUG, "Loading details for %d epg entries.", epgDBInfos.size());
    if (epgDBInfos.size() > 0) {
      std::lock_guard<std::mutex> lock(sendEpgToKodiMutex);
      m_epgDB.BeginTransaction();
      std::vector<EpgDBInfo> infos(epgDBInfos.begin(), epgDBInfos.end());
      std::ostringstream ids;
      std::map<int, EpgDBInfo*> epgDBInfoById;

      bool first = true;
      for (EpgDBInfo &epgDBInfo: infos) {
        if (first) {
          first = false;
        } else {
          ids << ",";
        }
        ids << epgDBInfo.programId;
        epgDBInfoById[epgDBInfo.programId] = &epgDBInfo;

      }
      std::ostringstream urlStream;
      urlStream << m_providerUrl << "/zapi/v2/cached/program/power_details/"
          << m_powerHash << "?complete=True&program_ids=" << ids.str();
      
      int statusCode;
      std::string jsonString = m_httpClient.HttpGet(urlStream.str(), statusCode);

      Document detailDoc;
      detailDoc.Parse(jsonString.c_str());
      if (detailDoc.GetParseError() || !detailDoc["success"].GetBool())
      {
        Log(ADDON_LOG_ERROR, "Failed to load details for program.");
        m_detailsThreadRunning = false;
        
      }
      else
      {
        const Value& programs = detailDoc["programs"];
        for (Value::ConstValueIterator progItr = programs.Begin();
            progItr != programs.End(); ++progItr)
        {
          const Value &program = *progItr;
          int programId = program["id"].GetInt();
          EpgDBInfo *epgDBInfo = epgDBInfoById[programId];
          epgDBInfo->description = Utils::JsonStringOrEmpty(program, "d");
          epgDBInfo->season = program.HasMember("s_no") && !program["s_no"].IsNull() ? program["s_no"].GetInt() : -1;
          epgDBInfo->episode = program.HasMember("e_no") && !program["e_no"].IsNull() ? program["e_no"].GetInt() : -1;

          epgDBInfo->detailsLoaded = 1;
          if (!m_epgDB.Update(*epgDBInfo)) {
            Log(ADDON_LOG_ERROR, "Failed to update epg data.");
          }
          SendEpgDBInfo(*epgDBInfo);
        }
      }
      for (EpgDBInfo &epgDBInfo: infos) {
        if (!epgDBInfo.detailsLoaded) {
          epgDBInfo.detailsLoaded = 1;
          m_epgDB.Update(epgDBInfo);
        }
      }
      m_epgDB.EndTransaction();
    }

    for (int i = 0; i < 10; i++) {
      if (!m_detailsThreadRunning) {
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  Log(ADDON_LOG_DEBUG, "Details thread stopped");
}
#endif
