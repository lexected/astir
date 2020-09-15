#pragma once

#include <list>
#include <memory>
#include "SymbolGroup.h"

class LLkFirster;
class ILLkFirstable {
public:
	virtual SymbolGroupList first(LLkFirster* firster, const SymbolGroupList& prefix) const = 0;

protected:
	ILLkFirstable() = default;
};

typedef const ILLkFirstable* ILLkFirstableCPtr;