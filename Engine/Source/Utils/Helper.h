#pragma once
#include <string>
#include <sstream>
#include <algorithm>

[[maybe_unused]] static bool IsFloat(const std::string& text) {
	std::istringstream iss(text);
	float f;
	iss >> std::noskipws >> f; // noskipws considers leading whitespace invalid
	// Check the entire string was consumed and if either failbit or badbit is set
	return iss.eof() && !iss.fail();
}

[[maybe_unused]] static bool IsInt(const std::string& text)
{
	std::string s(text);
	return !s.empty() && std::find_if(s.begin(), s.end(), [](unsigned char c) { return !std::isdigit(c); }) == s.end();
}

[[maybe_unused]] static bool IsBool(const std::string& text)
{
	if (IsInt(text) || text == "true" || text == "false")
		return true;
	else
		return false;
}

[[maybe_unused]] static bool ToBool(const std::string& text)
{
	if (text != "0" || text == "true")
		return true;
	else
		return false;
}