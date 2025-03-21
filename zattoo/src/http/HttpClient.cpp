#include "../kodi.h"
#include "HttpClient.h"
#include "Cache.h"
#include <random>
#include "../md5.h"
//#include <kodi/AddonBase.h>

static const std::string USER_AGENT = std::string("Kodi/")
    + std::string("21.1.0") + std::string(" pvr.zattoo/")
    + std::string("21.0.4");

HttpClient::HttpClient(ParameterDB *parameterDB):
  m_parameterDB(parameterDB)
{
  Log(ADDON_LOG_INFO, "Using useragent: %s", USER_AGENT.c_str());

  m_uuid = m_parameterDB->Get("uuid");
  m_beakerSessionId = m_parameterDB->Get("beakerSessionId");  
}

HttpClient::~HttpClient()
{
  
}

void HttpClient::ClearSession() {
  m_uuid = GetUUID();
  //m_beakerSessionId = "";  
}

std::string HttpClient::GetUUID()
{
  if (!m_uuid.empty())
  {
    return m_uuid;
  }
  m_uuid = GenerateUUID();
  m_parameterDB->Set("uuid", m_uuid);
  return m_uuid;
}

std::string HttpClient::GenerateUUID()
{
    std::string tmp_s;
    static const char alphanum[] =
        "0123456789"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "-";
    
    srand( (unsigned) time(NULL));
    
    for (int i = 0; i < 21; ++i)
    {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }
    
    return tmp_s;
}

std::string HttpClient::HttpGetCached(const std::string& url, time_t cacheDuration, int &statusCode, bool store)
{

  std::string content;

  std::string cacheKey = md5(url);
  statusCode = 200;
  if (!Cache::Read(cacheKey, content))
  {
    content = HttpGet(url, statusCode);
    if (!content.empty() && store)
    {
      time_t validUntil;
      time(&validUntil);
      validUntil += cacheDuration;
      Cache::Write(cacheKey, content, validUntil);
    }
  }
  return content;

}

std::string HttpClient::HttpGet(const std::string& url, int &statusCode)
{
  //Log(ADDON_LOG_INFO, "GET URL >%s< ",url.c_str());
  return HttpRequest("GET", url, "", statusCode);
}

std::string HttpClient::HttpDelete(const std::string& url, int &statusCode)
{
  return HttpRequest("DELETE", url, "", statusCode);
}

std::string HttpClient::HttpPost(const std::string& url, const std::string& postData, int &statusCode)
{
  //Log(ADDON_LOG_INFO, "POST URL >%s< Data >%s<",url.c_str(),postData.c_str());
  return HttpRequest("POST", url, postData, statusCode);
}

std::string HttpClient::HttpRequest(const std::string& action, const std::string& url, const std::string& postData, int &statusCode)
{
  Curl curl;

  curl.AddOption("acceptencoding", "gzip,deflate");
  
  std::string cookie = "";

  if (!m_beakerSessionId.empty())
  {
    cookie += "beaker.session.id=" + m_beakerSessionId + "; ";
  }

  if (!m_uuid.empty())
  {
    cookie += "uuid=" + m_uuid + "; ";
  }
  
  if (!cookie.empty()) {
    curl.AddOption("Cookie", cookie);
  }

  curl.AddHeader("User-Agent", USER_AGENT);

  std::string content = HttpRequestToCurl(curl, action, url, postData, statusCode);

  if (statusCode >= 400 || statusCode < 200) {
    Log(ADDON_LOG_ERROR, "Open URL failed with %i.", statusCode);
    if (m_statusCodeHandler != nullptr) {
      m_statusCodeHandler->ErrorStatusCode(statusCode);
    }
    return "";
  }
  std::string sessionId = curl.GetCookie("beaker.session.id");
  if (!sessionId.empty() && m_beakerSessionId != sessionId)
  {
    Log(ADDON_LOG_DEBUG, "Got new beaker.session.id: %s..",
        sessionId.substr(0, 5).c_str());
    m_beakerSessionId = sessionId;
    m_parameterDB->Set("beakerSessionId", sessionId);
  }

  return content;
}

std::string HttpClient::HttpRequestToCurl(Curl &curl, const std::string& action,
    const std::string& url, const std::string& postData, int &statusCode)
{
  //Log(ADDON_LOG_DEBUG, "Http-Request: %s %s.", action.c_str(), url.c_str());
  std::string content;
  if (action == "POST")
  {
    content = curl.Post(url, postData, statusCode);
  }
  else if (action == "DELETE")
  {
    content = curl.Delete(url, statusCode);
  }
  else
  {
    content = curl.Get(url, statusCode);
  }
  return content;

}
