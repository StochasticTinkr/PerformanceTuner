#include <string>
#include "StringUtils.h"

bool stringEndsWithIgnoreCase(const std::string& string, const std::string &suffix)
{
	if (string.length() < suffix.length())
	{
		return false;
	}
	for (auto i = suffix.rbegin(), j = string.rbegin(); i != suffix.rend(); ++i, ++j)
	{
		if (tolower(*i) != tolower(*j))
		{
			return false;
		}
	}
	return true;
}


std::string replaceSuffix(const std::string& string, size_t oldSuffixLength, const std::string& newSuffix)
{
	return string.substr(0, string.length() - oldSuffixLength) + newSuffix;
}
