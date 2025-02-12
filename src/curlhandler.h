#pragma once

#include <string>
#include "kodi/c-api/filesystem.h"

extern bool verbose;

int get_curl_instance();
extern const char * GetValue(const char *,struct ch *);

/* curl calls this routine to get more data */
size_t write_callback(char *buffer,
                      size_t size,
                      size_t nitems,
                      void *userp);

double get_file_download_speed(void* Kodibase, void *curl);
ssize_t read_file(void* KodiBase, void* curl, void *ptr, size_t size);
bool ParseLine(const std::string& headerLine,struct ch *c);
size_t header_callback(void *ptr, size_t size, size_t nmemb, void *curl);
std::string GetHeader(struct ch *c);
const char * GetValue(const char *name,struct ch *c);
void * curl_create(void * base, const char *url);
bool curl_open(void *base, void *curl, unsigned int flags);
void Decode(const char* input, unsigned int length, std::string &output);
bool curl_add_option(void* kodiBase, void* curl, int type, const char* name, const char* value);
void * open_file_for_write(void* kodi, const char *filename, bool overwrite);
ssize_t write_file(void *kodi, void* curl, const void  *p, size_t size);
void close_file (void* kodiBase, void* curl);
char ** get_property_values(void* kodiBase, void* curl, int type, const char* name, int* numvalues);
char * translate_special_protocol(void * kodiBase, const char *proto);
bool remove_directory(void * kodiBase, const char *dir);
bool create_directory(void * kodiBase, const char *dir);
char * get_addon_info(void * kodiBase, const char *item);
void SplitFilename (const std::string& str);

bool get_directory(void * kodiBase,
                   const char *path,
                   const char* mask,
                   struct VFSDirEntry** items,
                   unsigned int* num_items);

void free_directory(void* kodiBase, struct VFSDirEntry* items, unsigned int num_items);




