#pragma once

class LRABuilder;
class LRA;

#include "AFA.h"
#include "SymbolGroup.h"

class ILRABuilding {
public:
	virtual void accept(const LRABuilder* lraBuilder, LRA* lra, AFAState startingState, const SymbolGroupPtrVector& lookahead) const = 0;

	virtual ~ILRABuilding() = default;
protected:
	ILRABuilding() = default;
};

typedef const ILRABuilding* ILRABuildingCPtr;