#pragma once

#include "Token.h"
#include <list>
#include <memory>

#include "Specification.h"

class Parser {
public:
	Parser() = default;

	std::unique_ptr<Specification> parse(const std::list<Token>& tokens) const;
	std::unique_ptr<MachineDefinition> parseMachineDefinition(std::list<Token>::const_iterator & it) const;
	std::unique_ptr<MachineDefinition> parseStatement(std::list<Token>::const_iterator& it) const;
};

