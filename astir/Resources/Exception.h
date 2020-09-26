#pragma once

#include <string>
#include <exception>

class Exception : public std::exception {
public:
	Exception() = default;
	Exception(const std::string& message)
		: std::exception(message.c_str()) { }

	virtual ~Exception() = default;
};