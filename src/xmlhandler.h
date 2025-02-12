#pragma once

#include <tinyxml2.h>

using namespace tinyxml2;

#define MAXPROP 40

#ifndef XMLCheckResult
#define XMLCheckResult(a_eResult) if (a_eResult != XML_SUCCESS) { printf("XML Error: %i\n", a_eResult); return a_eResult; }
#endif

struct settingsdef {
    char *id;
    char *level;
    char *deflt;
};

extern settingsdef settings[MAXPROP];
extern int n_settings;

int ReadXML(const char * xmlfile);
int FindId(const char* id);
bool GetSettingString(void* hdl, const char *id, char **value);
bool GetSettingInt(void* hdl, const char *id, int *value);
bool GetSettingFloat(void* hdl, const char *id, float *value);
bool GetSettingBool(void * hdl, const char *id, bool *value);
