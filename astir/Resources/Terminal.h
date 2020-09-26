#pragma once

#include <memory>

#include "Production.h"

template <typename TerminalTypeType>
class Terminal : public Production {
public:
	TerminalTypeType type;
	std::string raw;

	std::string stringForError() const override { return '\'' + raw + '\''; };
protected:
	Terminal(TerminalTypeType type, const std::shared_ptr<Location>& occurenceLocation)
		: type(type), raw(), Production(occurenceLocation) { }
	Terminal(TerminalTypeType type, const std::string& raw, const std::shared_ptr<Location>& occurenceLocation)
		: type(type), raw(raw), Production(occurenceLocation) { }
};