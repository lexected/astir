#pragma once

class LLkBuilder;

class ILLkBuilding {
public:
	virtual void accept(LLkBuilder* nfaBuilder) const = 0;

	virtual ~ILLkBuilding() = default;
protected:
	ILLkBuilding() = default;
};

typedef const ILLkBuilding* ILLkBuildingCPtr;