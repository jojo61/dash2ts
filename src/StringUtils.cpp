#include "StringUtils.h"

template<typename Str, typename Fn>
void transformString(const Str& input, Str& output, Fn fn)
{
  std::transform(input.begin(), input.end(), output.begin(), fn);
}

void ToLower(std::string &str)
{
  transformString(str, str, ::tolower);
}

std::string& TrimLeft(std::string &str, const char* const chars)
{
  size_t nidx = str.find_first_not_of(chars);
  str.erase(0, nidx);
  return str;
}

std::string& TrimRight(std::string &str, const char* const chars)
{
  size_t nidx = str.find_last_not_of(chars);
  str.erase(str.npos == nidx ? 0 : ++nidx);
  return str;
}

std::string& Trim(std::string &str, const char* const chars)
{
  TrimLeft(str, chars);
  return TrimRight(str, chars);
}
