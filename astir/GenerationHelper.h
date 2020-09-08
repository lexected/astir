#pragma once

#include <string>
#include <map>
#include <ostream>

class GenerationHelper {
public:
	static void macroWrite(const std::string& sourceString, const std::map<std::string, std::string>& macroPairs, std::ostream& m_output);

private:
	GenerationHelper() = default;
};

