#ifndef __STRING_UTILS_H__
#define __STRING_UTILS_H__

bool stringEndsWithIgnoreCase(const std::string& string, const std::string &suffix);
std::string replaceSuffix(const std::string& string, size_t oldSuffixLength, const std::string& newSuffix);

#endif
