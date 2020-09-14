#pragma once

#include <list>
#include <memory>
#include "SymbolGroup.h"

class ILLkNonterminal {
public:
	virtual SymbolGroupList first(std::list<std::shared_ptr<SymbolGroup>> prefix) const = 0;

protected:
	ILLkNonterminal() = default;
};

typedef const ILLkNonterminal* ILLkNonterminalCPtr;