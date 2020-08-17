#pragma once

#include <string>

struct FileLocation {
	unsigned long line;
	unsigned long column;

	FileLocation()
		: line(0), column(0) { }

	FileLocation(unsigned long line, unsigned long column)
		: line(line), column(column) { }

	std::string toString() const {
		return std::to_string(line) + ":" + std::to_string(column);
	}
};