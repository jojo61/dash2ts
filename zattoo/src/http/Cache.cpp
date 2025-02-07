//#include <kodi/AddonBase.h>
#include "Cache.h"
//#include <kodi/Filesystem.h>
#include "../Utils.h"
#include "../kodi.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"


using namespace rapidjson;

extern std::string CACHE_DIR;

time_t Cache::m_lastCleanup = 0;

bool Cache::Read(const std::string& key, std::string& data)
{
  std::string cacheFile = CACHE_DIR + key;
  if (!Utils::FileExists(cacheFile, true))
  {
    return false;
  }
  //printf("Read cachefile %s\n",key.c_str());
  std::string jsonString = Utils::ReadFile(cacheFile);
  if (jsonString.empty())
  {
    return false;
  }
  Document doc;
  doc.Parse(jsonString.c_str());
  if (doc.GetParseError())
  {
    if (Utils::FileExists(cacheFile, true))
    {
      Log(ADDON_LOG_ERROR, "Parsing cache file [%s] failed.", cacheFile.c_str());
    }
    return false;
  }

  if (!IsStillValid(doc))
  {
    Log(ADDON_LOG_DEBUG, "Ignoring cache file [%s] due to expiry.",
        cacheFile.c_str());
    return false;
  }

  Log(ADDON_LOG_DEBUG, "Load from cache file [%s].", cacheFile.c_str());
  data = doc["data"].GetString();
  return !data.empty();
}

void * open_file_for_write(void* kodi, const char *filename, bool overwrite);
ssize_t write_file(void *kodi, void* curl, const void  *p, size_t size);
void close_file (void* kodiBase, void* curl);

void Cache::Write(const std::string& key, const std::string& data, time_t validUntil)
{
  if (!Utils::DirectoryExists(CACHE_DIR))
  {
    if (!Utils::CreateDirectory(CACHE_DIR))
    {
      Log(ADDON_LOG_ERROR, "Could not crate cache directory [%s].", CACHE_DIR);
      return;
    }
  }
  //printf("Write cachefile %s\n",key.c_str());
  std::string cacheFile = CACHE_DIR + key;
  void *file = open_file_for_write(0,cacheFile.c_str(), true);
  if (file == nullptr)
  {
    Log(ADDON_LOG_ERROR, "Could not write to cache file [%s].",
        cacheFile.c_str());
    return;
  }

  Document d;
  d.SetObject();
  d.AddMember("validUntil", static_cast<uint64_t>(validUntil), d.GetAllocator());
  Value value;
  value.SetString(data.c_str(), static_cast<SizeType>(data.length()), d.GetAllocator());
  d.AddMember("data", value, d.GetAllocator());

  StringBuffer buffer;
  Writer<StringBuffer> writer(buffer);
  d.Accept(writer);
  const char* output = buffer.GetString();
  write_file(0,file,output, strlen(output));
  close_file(0,file);

}

void Cache::Cleanup()
{
  time_t currTime;
  time(&currTime);
  
  if (m_lastCleanup + 60 * 60 > currTime)
  {
   return;
  }
  
  m_lastCleanup = currTime;
  if (!Utils::DirectoryExists(CACHE_DIR))
  {
    Log(ADDON_LOG_ERROR, "Did not find cache directory.");
    return;
  }

  std::vector<std::string> items;
  if (!Utils::GetDirectory(CACHE_DIR,  &items))
  {
    Log(ADDON_LOG_ERROR, "Could not get cache directory.");
    return;
  }

  for (const auto& item : items)
  {
  #if 0
    if (item.IsFolder())
    {
      continue;
    }
    std::string path = item.Path();
  #endif
  
    std::string path = item;
    std::string jsonString = Utils::ReadFile(path);
    if (jsonString.empty())
    {
      continue;
    }
    Document doc;
    doc.Parse(jsonString.c_str());
    if (doc.GetParseError())
    {
      Log(ADDON_LOG_ERROR, "Parsing cache file [%s] failed. -> Delete", path.c_str());
      Utils::DeleteFile(path);
    }

    if (!IsStillValid(doc))
    {
      Log(ADDON_LOG_DEBUG, "Deleting expired cache file [%s].", path.c_str());
      if (!Utils::DeleteFile(path))
      {
        Log(ADDON_LOG_DEBUG, "Deletion of file [%s] failed.", path.c_str());
      }
    }
  }
}

bool Cache::IsStillValid(const Value& cache)
{
  time_t validUntil = static_cast<time_t>(cache["validUntil"].GetUint64());
  time_t current_time;
  time(&current_time);
  return validUntil >= current_time;
}
