#pragma once

class LLkBuilder;

class ILLkBuildable {
public:
	virtual void accept(LLkBuilder* nfaBuilder) const = 0;

	virtual ~ILLkBuildable() = default;

protected:
	ILLkBuildable() = default;
};

typedef const ILLkBuildable* ILLkBuildableCPtr;