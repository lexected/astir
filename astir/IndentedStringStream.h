#pragma once

#include <sstream>

class IndentedStringStream : public std::stringstream {
public:
	void put(const std::string& s);
	void putln(const std::string& s);

	void increaseIndentation();
	void decreaseIndentation();

private:
	std::string m_indentation;
};