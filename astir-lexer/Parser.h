#pragma once

#include "Token.h"
#include <list>
#include <memory>

#include "Specification.h"

class Parser {
public:
	Parser() = default;

	std::unique_ptr<Specification> parse(const std::list<Token>& tokens) const;
};

