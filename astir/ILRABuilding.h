#pragma once

class LRABuilder;
class LRA;

#include "AFA.h"

class ILRABuilding {
public:
	virtual void accept(LRABuilder* nfaBuilder, LRA* lra, AFAState startingState) const = 0;

	virtual ~ILRABuilding() = default;
protected:
	ILRABuilding() = default;
};

typedef const ILRABuilding* ILRABuildingCPtr;