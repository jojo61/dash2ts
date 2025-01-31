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
  //m_detailsThread = std::thread([&] { DetailsThread(); });
}

ZattooEpgProvider::~ZattooEpgProvider() {
  m_detailsThreadRunning = false;
  //if (m_detailsThread.joinable())
  //  m_detailsThread.join();
}

std::string ZattooEpgProvider::svdrpsend(std::string& cmd) {
  
  char buffer[1000];
    std::string result = "";
    std::string mycmd = "svdrpsend "+cmd+"\n";
    FILE* pipe = popen(mycmd.c_str(), "r");
    if (!pipe) throw std::runtime_error("popen() failed!");
    try {
        while (fgets(buffer, sizeof (buffer), pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        throw;
    }
    pclose(pipe);
    return result;

}

std::string ZattooEpgProvider::GetDetails(int ProgrammId) {
    std::ostringstream urlStream;
    std::string description = "";

    urlStream << m_providerUrl << "/zapi/v2/cached/program/power_details/"
        << m_powerHash << "?complete=True&program_ids=" << std::to_string(ProgrammId);
    
    int statusCode;
    std::string jsonString = m_httpClient.HttpGet(urlStream.str(), statusCode);

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
    }
    return description;
}

bool ZattooEpgProvider::LoadEPGForChannel(ZatChannel &notused, time_t iStart, time_t iEnd) {
  char tmp[10];
  ZatChannel channel;
  std::string VDRid;
  std::string uniqueID;
  std::string epgdata;
  std::string cmd,details;
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
    std::string jsonString = m_httpClient.HttpGetCached(urlStream.str(), 86400, statusCode);

    Document doc;
    doc.Parse(jsonString.c_str());
    if (doc.GetParseError())
    {
      Log(ADDON_LOG_ERROR, "Loading epg failed from %lu to %lu", iStart, iEnd);
      return false;
    }
    //RegisterAlreadyLoaded(tempStart, tempEnd);
    const Value& channels = doc["channels"];
    
    std::lock_guard<std::mutex> lock(sendEpgToKodiMutex);
    //m_epgDB.BeginTransaction();
    
    for (Value::ConstMemberIterator iter = channels.MemberBegin(); iter != channels.MemberEnd(); ++iter) {
      std::string cid = iter->name.GetString();
      

      channel = m_visibleChannelsByCid[cid];
      VDRid = "I-"+std::to_string(channel.iUniqueId)+"-80-1";
      cmd = "lstc "+VDRid;
      std::string result = svdrpsend(cmd);
      if (result.find("not defined") != std::string::npos) {
        continue;
      }
      printf ("Channel %s\n",cid.c_str());

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
        
        int programId = program["id"].GetInt();
        if (more_details)
          details = GetDetails(programId);
        if (details.size() == 0) {
           more_details = false;
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
        time_t endTime = program["e"].GetInt();
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
      epgdata.append("c \n");
      int fp = open("/tmp/epg",O_WRONLY|O_CREAT|O_TRUNC,0644);
      ssize_t s = write(fp,epgdata.c_str(),epgdata.size());
      if (s != epgdata.size())
         printf("error in write\n");
      close(fp);
      //cmd = "clre "+VDRid;
      //svdrpsend(cmd);
      cmd = "pute /tmp/epg";
      svdrpsend(cmd);
    }
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
