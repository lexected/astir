#pragma once

#include "Token.h"
#include <list>

class Parser {
public:
	Parser() = default;

	void parse(const std::list<Token>& tokens) const;
};

