#ifndef SRC_EPG_EPGPROVIDER_H_
#define SRC_EPG_EPGPROVIDER_H_

#include "../ZatChannel.h"
//#include <kodi/addon-instance/PVR.h>
#include <ctime>
#include <mutex>

class EpgProvider
{
public:

  EpgProvider(void *addon) {} 
  virtual ~EpgProvider() {};

  //virtual bool LoadEPGForChannel( time_t iStart, time_t iEnd) = 0;
protected:

  static std::mutex sendEpgToKodiMutex;
#if 0
  void SendEpg(kodi::addon::PVREPGTag &tag) {
    m_addon->EpgEventStateChange(tag, EPG_EVENT_CREATED);
  }
  kodi::addon::CInstancePVRClient *m_addon;
#endif
};

#endif /* SRC_EPG_EPGPROVIDER_H_ */

