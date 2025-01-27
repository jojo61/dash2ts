#ifndef ZATDATA
#define ZATDATA
#include "UpdateThread.h"
//#include "categories.h"
#include <map>
#include <thread>
#include <mutex>
#include "rapidjson/document.h"
#include "ZatChannel.h"
#include "Settings.h"
#include "sql/EpgDB.h"
#include "sql/RecordingsDB.h"
#include "sql/ParameterDB.h"
#include "http/HttpClient.h"
#include "epg/EpgProvider.h"
#include "Session.h"

class CZattooTVAddon;

struct ZatRecordingDetails
{
  std::string genre;
  std::string description;
  int seriesNumber = EPG_TAG_INVALID_SERIES_EPISODE;
  int episodeNumber = EPG_TAG_INVALID_SERIES_EPISODE;
};

struct PVRZattooChannelGroup
{
  std::string name;
  std::vector<ZatChannel> channels;
};

class ZatData 
{
public:
  bool Create();
  ZatData();
  ~ZatData();
  bool Initialize();
  bool LoadChannels();

  void myTrim( std::string &str, char* charsToRemove );
  //PVR_ERROR GetCapabilities(PVRCapabilities& capabilities);
  PVR_ERROR GetBackendName(std::string& name);
  PVR_ERROR GetBackendVersion(std::string& version);
  PVR_ERROR GetBackendHostname(std::string& hostname);
  PVR_ERROR GetConnectionString(std::string& connection);

  PVR_ERROR GetChannelsAmount(int& amount);
  std::string GetChannels();
  PVR_ERROR GetChannelGroupsAmount(int& amount);
  //PVR_ERROR GetChannelGroups(bool radio, PVRChannelGroupsResultSet& results);
  //PVR_ERROR GetChannelGroupMembers(const PVRChannelGroup& group,
  //                                 PVRChannelGroupMembersResultSet& results);
  //PVR_ERROR GetEPGForChannel(int channelUid, time_t start, time_t end, PVREPGTagsResultSet& results) override;
  PVR_ERROR GetChannelStreamProperties(const int uniqueID,
                                       PVRStreamProperty& properties);
#if 0
  PVR_ERROR GetTimerTypes(std::vector<PVRTimerType>& types) override;
  PVR_ERROR GetTimers(PVRTimersResultSet& results) override;
  PVR_ERROR GetTimersAmount(int& amount) override;
  PVR_ERROR AddTimer(const PVRTimer& timer) override;
  PVR_ERROR DeleteTimer(const PVRTimer& timer, bool forceDelete) override;
  PVR_ERROR GetRecordings(bool deleted, PVRRecordingsResultSet& results) override;
  PVR_ERROR GetRecordingsAmount(bool deleted,  int& amount) override;
  PVR_ERROR GetRecordingStreamProperties(const PVRRecording& recording,
                                         std::vector<PVRStreamProperty>& properties) override;
  PVR_ERROR DeleteRecording(const PVRRecording& recording) override;
  PVR_ERROR SetRecordingPlayCount(const PVRRecording& recording, int count) override;
  PVR_ERROR SetRecordingLastPlayedPosition(const PVRRecording& recording,
                                           int lastplayedposition) override;
  PVR_ERROR GetRecordingLastPlayedPosition(const PVRRecording& recording, int& position) override;
  PVR_ERROR IsEPGTagPlayable(const PVREPGTag& tag, bool& isPlayable) override;
  PVR_ERROR IsEPGTagRecordable(const PVREPGTag& tag, bool& isRecordable) override;
  PVR_ERROR GetEPGTagStreamProperties(const PVREPGTag& tag, std::vector<PVRStreamProperty>& properties) override;
  PVR_ERROR GetEPGTagEdl(const PVREPGTag& tag, std::vector<PVREDLEntry>& edl) override;
  PVR_ERROR GetRecordingEdl(const PVRRecording& recording, std::vector<PVREDLEntry>& edl) override;  

  int GetRecallSeconds(const PVREPGTag& tag);
  void GetEPGForChannelAsync(int uniqueChannelId, time_t iStart, time_t iEnd);
#endif
  bool RecordingEnabled()
  {
    return false; // m_session->IsRecordingEnabled();
  }
  void UpdateConnectionState(const std::string& connectionString, PVR_CONNECTION_STATE newState, const std::string& message);
  
  //ADDON_STATUS SetSetting(const std::string& settingName,
  //                        const CSettingValue& settingValue) override;

  bool SessionInitialized();
private:
  std::vector<PVRZattooChannelGroup> m_channelGroups;
  std::map<int, ZatChannel> m_channelsByUid;
  std::map<std::string, ZatChannel> m_channelsByCid;
  std::map<std::string, ZatChannel> m_visibleChannelsByCid;
  std::vector<UpdateThread*> m_updateThreads;
  //Categories m_categories;
  //EpgDB *m_epgDB;
  RecordingsDB *m_recordingsDB;
  ParameterDB *m_parameterDB;
  HttpClient *m_httpClient;
  //EpgProvider *m_epgProvider = nullptr;
  CSettings* m_settings;
  Session *m_session;

  bool ReadDataJson();
  rapidjson::Document Login();
  bool InitSession(bool isReinit);
  bool ReinitSession();
  ZatChannel* FindChannel(int uniqueId);
  PVRZattooChannelGroup* FindGroup(const std::string& strName);
  std::string GetStreamTypeString(bool withDrm);
  bool IsDrmLimitApplied(rapidjson::Document& doc);
  std::string GetStreamUrl(rapidjson::Document& doc, PVRStreamProperty& properties);
  std::string GetBasicStreamParameters(bool requiresDrm);
  std::string GetQualityStreamParameter(const std::string& cid, bool withoutDrm, bool& requiresDrm);
  int GetDrmLevel();
  //bool ParseRecordingsTimers(const rapidjson::Value& recordings, std::map<int, ZatRecordingDetails>& detailsById);
  //void AddTimerType(std::vector<PVRTimerType>& types, int idx, int attributes);
  //bool Record(int programId, bool series);

  void SetStreamProperties(PVRStreamProperty& properties, const std::string& url);
#if 0
  std::string GetStreamUrlForProgram(const std::string& cid, int programId, std::vector<PVRStreamProperty>& properties);
#endif
  bool TryToReinitIf403(int statusCode);
};
#endif