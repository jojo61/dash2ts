#include <string>
#include <vector>
#include <stdarg.h>

enum {
    ADDON_LOG_DEBUG,
    ADDON_LOG_INFO,
    ADDON_LOG_ERROR,
    ADDON_LOG_WARNING
};

#define PVR_ADDON_NAME_STRING_LENGTH 1024
#define PVR_ADDON_URL_STRING_LENGTH 1024
#define PVR_ADDON_DESC_STRING_LENGTH 1024
#define PVR_ADDON_INPUT_FORMAT_STRING_LENGTH 32
#define PVR_ADDON_EDL_LENGTH 64
#define PVR_ADDON_TIMERTYPE_ARRAY_SIZE 32
#define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE 512
#define PVR_ADDON_TIMERTYPE_VALUES_ARRAY_SIZE_SMALL 128
#define PVR_ADDON_TIMERTYPE_STRING_LENGTH 128
#define PVR_ADDON_ATTRIBUTE_DESC_LENGTH 128
#define PVR_ADDON_ATTRIBUTE_VALUES_ARRAY_SIZE 512
#define PVR_ADDON_DESCRAMBLE_INFO_STRING_LENGTH 64
#define PVR_ADDON_DATE_STRING_LENGTH 32
#define PVR_ADDON_COUNTRIES_STRING_LENGTH 128
#define PVR_ADDON_LANGUAGES_STRING_LENGTH 128

typedef struct PVR_CHANNEL
  {
    unsigned int iUniqueId;
    bool bIsRadio;
    unsigned int iChannelNumber;
    unsigned int iSubChannelNumber;
    char strChannelName[PVR_ADDON_NAME_STRING_LENGTH];
    char strMimeType[PVR_ADDON_INPUT_FORMAT_STRING_LENGTH];
    unsigned int iEncryptionSystem;
    char strIconPath[PVR_ADDON_URL_STRING_LENGTH];
    bool bIsHidden;
    bool bHasArchive;
    int iOrder;
    int iClientProviderUid;
  } PVR_CHANNEL;

typedef enum PVR_CONNECTION_STATE
  {
    /// @brief __0__ : Unknown state (e.g. not yet tried to connect).
    PVR_CONNECTION_STATE_UNKNOWN = 0,

    /// @brief __1__ : Backend server is not reachable (e.g. server not existing or
    /// network down).
    PVR_CONNECTION_STATE_SERVER_UNREACHABLE = 1,

    /// @brief __2__ : Backend server is reachable, but there is not the expected type of
    /// server running (e.g. HTSP required, but FTP running at given server:port).
    PVR_CONNECTION_STATE_SERVER_MISMATCH = 2,

    /// @brief __3__ : Backend server is reachable, but server version does not match
    /// client requirements.
    PVR_CONNECTION_STATE_VERSION_MISMATCH = 3,

    /// @brief __4__ : Backend server is reachable, but denies client access (e.g. due
    /// to wrong credentials).
    PVR_CONNECTION_STATE_ACCESS_DENIED = 4,

    /// @brief __5__ : Connection to backend server is established.
    PVR_CONNECTION_STATE_CONNECTED = 5,

    /// @brief __6__ : No connection to backend server (e.g. due to network errors or
    /// client initiated disconnect).
    PVR_CONNECTION_STATE_DISCONNECTED = 6,

    /// @brief __7__ : Connecting to backend.
    PVR_CONNECTION_STATE_CONNECTING = 7,
  } PVR_CONNECTION_STATE;
typedef enum ADDON_STATUS
  {
    /// @brief For everything OK and no error
    ADDON_STATUS_OK,

    /// @brief A needed connection was lost
    ADDON_STATUS_LOST_CONNECTION,

    /// @brief Addon needs a restart inside Kodi
    ADDON_STATUS_NEED_RESTART,

    /// @brief Necessary settings are not yet set
    ADDON_STATUS_NEED_SETTINGS,

    /// @brief Unknown and incomprehensible error
    ADDON_STATUS_UNKNOWN,

    /// @brief Permanent failure, like failing to resolve methods
    ADDON_STATUS_PERMANENT_FAILURE,

    /* internal used return error if function becomes not used from child on
    * addon */
    ADDON_STATUS_NOT_IMPLEMENTED
  } ADDON_STATUS;

typedef enum PVR_ERROR
  {
    /// @brief __0__ : No error occurred.
    PVR_ERROR_NO_ERROR = 0,

    /// @brief __-1__ : An unknown error occurred.
    PVR_ERROR_UNKNOWN = -1,

    /// @brief __-2__ : The method that Kodi called is not implemented by the add-on.
    PVR_ERROR_NOT_IMPLEMENTED = -2,

    /// @brief __-3__ : The backend reported an error, or the add-on isn't connected.
    PVR_ERROR_SERVER_ERROR = -3,

    /// @brief __-4__ : The command was sent to the backend, but the response timed out.
    PVR_ERROR_SERVER_TIMEOUT = -4,

    /// @brief __-5__ : The command was rejected by the backend.
    PVR_ERROR_REJECTED = -5,

    /// @brief __-6__ : The requested item can not be added, because it's already present.
    PVR_ERROR_ALREADY_PRESENT = -6,

    /// @brief __-7__ : The parameters of the method that was called are invalid for this
    /// operation.
    PVR_ERROR_INVALID_PARAMETERS = -7,

    /// @brief __-8__ : A recording is running, so the timer can't be deleted without
    /// doing a forced delete.
    PVR_ERROR_RECORDING_RUNNING = -8,

    /// @brief __-9__ : The command failed.
    PVR_ERROR_FAILED = -9,
  } PVR_ERROR;
//============================================================================
  /// @ingroup cpp_kodi_addon_pvr_Defs_epg
  /// @brief Special @ref kodi::addon::PVREPGTag::SetSeriesNumber(), @ref kodi::addon::PVREPGTag::SetEpisodeNumber()
  /// and @ref kodi::addon::PVREPGTag::SetEpisodePartNumber() value to indicate
  /// it is not to be used.
  ///
#define EPG_TAG_INVALID_SERIES_EPISODE -1

typedef std::pair<std::string, std::string> StreamParamValue;
typedef std::vector<StreamParamValue> PVRStreamProperty;
  //----------------------------------------------------------------------------

void Log(int,const char *, ...);
//void Log(int,const char *);
std::string GetUserPath(std::string );
std::string GetAddonPath(std::string );
std::string UserPath();
bool FileExists(std::string , bool);

class  CSettingValue
{
public:
  explicit CSettingValue(const std::string& settingValue) : str(settingValue) {}

  bool empty() const { return str.empty(); }

  /// @defgroup cpp_kodi_addon_addonbase_Defs_CSettingValue_Help Value Help
  /// @ingroup cpp_kodi_addon_addonbase_Defs_CSettingValue
  ///
  /// <b>The following table contains values that can be set with @ref cpp_kodi_addon_addonbase_Defs_CSettingValue :</b>
  /// | Name | Type | Get call
  /// |------|------|----------
  /// | **Settings value as string** | `std::string` | @ref CSettingValue::GetString "GetString"
  /// | **Settings value as integer** | `int` | @ref CSettingValue::GetInt "GetInt"
  /// | **Settings value as unsigned integer** | `unsigned int` | @ref CSettingValue::GetUInt "GetUInt"
  /// | **Settings value as boolean** | `bool` | @ref CSettingValue::GetBoolean "GetBoolean"
  /// | **Settings value as floating point** | `float` | @ref CSettingValue::GetFloat "GetFloat"
  /// | **Settings value as enum** | `enum` | @ref CSettingValue::GetEnum "GetEnum"

  /// @addtogroup cpp_kodi_addon_addonbase_Defs_CSettingValue
  ///@{

  /// @brief To get settings value as string.
  std::string GetString() const { return str; }

  /// @brief To get settings value as integer.
  int GetInt() const { return std::atoi(str.c_str()); }

  /// @brief To get settings value as unsigned integer.
  unsigned int GetUInt() const { return std::atoi(str.c_str()); }

  /// @brief To get settings value as boolean.
  bool GetBoolean() const { return std::atoi(str.c_str()) > 0; }

  /// @brief To get settings value as floating point.
  float GetFloat() const { return static_cast<float>(std::atof(str.c_str())); }

  /// @brief To get settings value as enum.
  /// @note Inside settings.xml them stored as integer.
  template<typename enumType>
  enumType GetEnum() const
  {
    return static_cast<enumType>(GetInt());
  }

  ///@}

private:
  const std::string str;
};


