/*
 *  Copyright (C) 2020 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "Settings.h"
//#include <kodi/General.h>
#include <tinyxml2.h>
#include "kodi.h"

using namespace tinyxml2;

#ifndef XMLCheckResult
	#define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { printf("XML Error: %i\n", a_eResult); return a_eResult; }
#endif

bool CSettings::Load()
{
  XMLDocument doc;

  std::string xmlfile = UserPath() + "settings.xml";

  XMLError eResult = doc.LoadFile(xmlfile.c_str());
  XMLCheckResult(eResult);

  XMLElement * p_root_element = doc.FirstChildElement();
  
  XMLElement * p_setting = p_root_element->FirstChildElement("setting"); 
  const char *text;
  while (p_setting && (text = p_setting->Attribute("id"))) {
    if (!strcmp(text,"username")) {
      m_zatUsername = strdup(p_setting->GetText());
    }
    if (!strcmp(text,"password")) {
      m_zatPassword = strdup(p_setting->GetText());
    }
    if (!strcmp(text,"provider")) {
      m_provider = atoi(p_setting->GetText());
    }
    if (!strcmp(text,"drmLevel")) {
      m_drmLevel = atoi(p_setting->GetText());
    }
    p_setting = p_setting->NextSiblingElement("setting");
  }
  m_parentalPin = "";
  m_smartTV = false;
  return true;
}

#if 0
int CSettings::SetSetting(const std::string& settingName,
                                   const kodi::addon::CSettingValue& settingValue)
{

  if (settingName == "username")
  {
    std::string tmp_sUsername;
    Log(ADDON_LOG_DEBUG, "Changed Setting 'username'");
    tmp_sUsername = m_zatUsername;
    m_zatUsername = settingValue.GetString();
    if (tmp_sUsername != m_zatUsername)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "password")
  {
    std::string tmp_sPassword;
    Log(ADDON_LOG_DEBUG, "Changed Setting 'password'");
    tmp_sPassword = m_zatPassword;
    m_zatPassword = settingValue.GetString();
    if (tmp_sPassword != m_zatPassword)
      return ADDON_STATUS_NEED_RESTART;
  }
  else if (settingName == "favoritesonly")
  {
    Log(ADDON_LOG_DEBUG, "Changed Setting 'favoritesonly' from %u to %u", m_zatFavoritesOnly, settingValue.GetBoolean());
    if (m_zatFavoritesOnly != settingValue.GetBoolean())
    {
      m_zatFavoritesOnly = settingValue.GetBoolean();
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (settingName == "smarttv")
  {
    Log(ADDON_LOG_DEBUG, "Changed Setting 'smarttv' from %u to %u", m_smartTV, settingValue.GetBoolean());
    if (m_smartTV != settingValue.GetBoolean())
    {
      m_smartTV = settingValue.GetBoolean();
      return ADDON_STATUS_NEED_RESTART;
    }
  }  else if (settingName == "enableDolby")
  {
    Log(ADDON_LOG_DEBUG, "Changed Setting 'enableDolby' from %u to %u", m_zatEnableDolby, settingValue.GetBoolean());
    if (m_zatEnableDolby != settingValue.GetBoolean())
    {
      m_zatEnableDolby = settingValue.GetBoolean();
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (settingName == "skipStart")
  {
    Log(ADDON_LOG_DEBUG, "Changed Setting 'skipStart' from %u to %u", m_skipStartOfProgramme, settingValue.GetBoolean());
    if (m_skipStartOfProgramme != settingValue.GetBoolean())
    {
      m_skipStartOfProgramme = settingValue.GetBoolean();
      return ADDON_STATUS_OK;
    }
  }
  else if (settingName == "skipEnd")
  {
    Log(ADDON_LOG_DEBUG, "Changed Setting 'skipEnd' from %u to %u", m_skipEndOfProgramme, settingValue.GetBoolean());
    if (m_skipEndOfProgramme != settingValue.GetBoolean())
    {
      m_skipEndOfProgramme = settingValue.GetBoolean();
      return ADDON_STATUS_OK;
    }
  }  else if (settingName == "parentalPin")
  {
    std::string tmp_sParentalPin;
    Log(ADDON_LOG_DEBUG, "Changed Setting 'parentalPin'");
    tmp_sParentalPin = m_parentalPin;
    m_parentalPin = settingValue.GetString();
    if (tmp_sParentalPin != m_parentalPin)
      return ADDON_STATUS_OK;
  }
  else if (settingName == "provider")
  {
    Log(ADDON_LOG_DEBUG, "Changed Setting 'provider'");
    if (m_provider != settingValue.GetInt())
    {
      m_provider = settingValue.GetInt();
      return ADDON_STATUS_NEED_RESTART;
    }
  }
  else if (settingName == "drmLevel")
  {
    Log(ADDON_LOG_DEBUG, "Changed Setting 'drmLevel' from %u to %u", m_drmLevel, settingValue.GetInt());
    if (m_drmLevel != settingValue.GetInt())
    {
      m_drmLevel = settingValue.GetInt();
      return ADDON_STATUS_OK;
    }
  }

  return ADDON_STATUS_OK;
}
#endif
bool CSettings::VerifySettings() {
#if 0
  std::string username = GetZatUsername();
  std::string password = GetZatPassword();
  if (username.empty() || password.empty()) {
    Log(ADDON_LOG_INFO, "Username or password not set.");
    kodi::QueueNotification(QUEUE_WARNING, "", kodi::addon::GetLocalizedString(30200));

    return false;
  }
#endif
  return true;
}
