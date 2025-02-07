#include "kodi.h"

#include "Utils.h"


#include <algorithm>
#include <iomanip>
#include <iterator>
#include <sstream>

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <filesystem>
//#include <kodi/Filesystem.h>
//#include <kodi/General.h>


std::string Utils::GetFilePath(const std::string &strPath, bool bUserPath)
{
  return bUserPath ? GetUserPath(strPath) : GetAddonPath(strPath);
}

// http://stackoverflow.com/a/17708801
std::string Utils::UrlEncode(const std::string &value)
{
  std::ostringstream escaped;
  escaped.fill('0');
  escaped << std::hex;

  for (char c : value) {
      // Keep alphanumeric and other accepted characters intact
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' || c == '!') // Exclamation mark should not be here but Zattoo does not correctly encode it
    {
      escaped << c;
      continue;
    }

    // Any other characters are percent-encoded
    escaped << '%' << std::setw(2) << int((unsigned char) c);
  }

  return escaped.str();
}

double Utils::StringToDouble(const std::string &value)
{
  std::istringstream iss(value);
  double result;

  iss >> result;

  return result;
}

int Utils::StringToInt(const std::string &value)
{
  return (int) StringToDouble(value);
}

std::vector<std::string> Utils::SplitString(const std::string &str,
    const char &delim, int maxParts)
{
  typedef std::string::const_iterator iter;
  iter beg = str.begin();
  std::vector < std::string > tokens;

  while (beg != str.end())
  {
    if (maxParts == 1)
    {
      tokens.emplace_back(beg, str.end());
      break;
    }
    maxParts--;
    iter temp = find(beg, str.end(), delim);
    if (beg != str.end())
      tokens.emplace_back(beg, temp);
    beg = temp;
    while ((beg != str.end()) && (*beg == delim))
      beg++;
  }

  return tokens;
}

bool Utils::DirectoryExists(const std::string& name) {
  return std::filesystem::exists(name);
}

bool Utils::CreateDirectory(const std::string& name) {
  return std::filesystem::create_directory(name);
}

bool Utils::FileExists (const std::string& name, bool cache) {
  struct stat buffer;   
  return (stat (name.c_str(), &buffer) == 0); 
}

bool Utils::GetDirectory(std::string& path, std::vector<std::string> *items) {
  
    for (const auto & entry : std::filesystem::directory_iterator(path)) {
        items->push_back(entry.path());
    }
    return true;
}

bool Utils::DeleteFile(std::string& path) {
    return std::filesystem::remove(path);
}

std::string Utils::ReadFile(const std::string& path)
{
  int fp = open(path.c_str(),O_RDONLY);
  
  if (fp < 0)
  {
    Log(ADDON_LOG_ERROR, "Failed to open file [%s].", path.c_str());
    return "";
  }

  char buf[1025];
  ssize_t nbRead;
  std::string content;
  while ((nbRead = read(fp,buf, 1024)) > 0)
  {
    buf[nbRead] = 0;
    content.append(buf);
  }
  close(fp);
  return content;

}

time_t Utils::StringToTime(const std::string &timeString)
{
  struct tm tm{};

  int year, month, day, h, m, s, tzh, tzm;
  if (sscanf(timeString.c_str(), "%d-%d-%dT%d:%d:%d%d", &year, &month, &day, &h,
      &m, &s, &tzh) < 7)
  {
    tzh = 0;
  }
  tzm = tzh % 100;
  tzh = tzh / 100;

  tm.tm_year = year - 1900;
  tm.tm_mon = month - 1;
  tm.tm_mday = day;
  tm.tm_hour = h - tzh;
  tm.tm_min = m - tzm;
  tm.tm_sec = s;

  time_t ret = timegm(&tm);
  return ret;
}

int Utils::GetChannelId(const char * strChannelName)
{
  int iId = 0;
  int c;
  while ((c = *strChannelName++))
    iId = ((iId << 5) + iId) + c; /* iId * 33 + c */
  return abs(iId);
}

std::string Utils::GetImageUrl(const std::string& imageToken) {
  return "https://images.zattic.com/cms/" + imageToken + "/format_640x360.jpg";
}

std::string Utils::JsonStringOrEmpty(const rapidjson::Value& jsonValue, const char* fieldName)
{
  if (!jsonValue.HasMember(fieldName) || !jsonValue[fieldName].IsString())
  {
    return "";
  }
  return jsonValue[fieldName].GetString();
}

int Utils::JsonIntOrZero(const rapidjson::Value& jsonValue, const char* fieldName)
{
  if (!jsonValue.HasMember(fieldName) || !jsonValue[fieldName].IsInt())
  {
    return 0;
  }
  return jsonValue[fieldName].GetInt();
}

bool Utils::JsonBoolOrFalse(const rapidjson::Value& jsonValue, const char* fieldName)
{
  if (!jsonValue.HasMember(fieldName))
  {
    return false;
  }

  if (jsonValue[fieldName].IsBool())
  {
    return jsonValue[fieldName].GetBool();
  }

  if (jsonValue[fieldName].IsInt())
  {
    return jsonValue[fieldName].GetInt() != 0;
  }

  return false;
}

bool Utils::RunsOnLinux()
{
    #ifdef __linux__
    return true;
    #else
    return false;
    #endif
}
