#pragma once

#include <sstream>

class IndentedStringStream : public std::stringstream {
public:
	void put(const std::string& s);
	void putln(const std::string& s);
	
	void newline();
	void indent();

	void increaseIndentation();
	void decreaseIndentation();

private:
	void _put(const std::string& s);
	std::string m_indentation;
};