#pragma once

#include <list>

#include "AFA.h"
#include "SymbolGroup.h"
#include "WeakContext.h"

class LRATransition : public AFATransition<SymbolGroup> {
public:
	LRATransition(AFAState target)
		: AFATransition(target) { }
	LRATransition(AFAState target, const std::shared_ptr<SymbolGroup>& condition)
		: AFATransition(target, condition) { }
	LRATransition(AFAState target, const std::shared_ptr<SymbolGroup>& condition, bool doNotOptimizeTargetIntoConditionClosure)
		: AFATransition(target, condition, doNotOptimizeTargetIntoConditionClosure) { }
};

class LRAAction {
public:
	std::shared_ptr<WeakContext> context;
};

class LRAShift : public LRAAction {
public:
};

class LRAReduce : public LRAAction {
public:
	unsigned int count;
};

class LRTag {
public:
	const TypeFormingStatement* statement;
	std::shared_ptr<WeakContext> context;
};

class LRAStateObject : public AFAStateObject<LRATransition> {
public:
	std::list<std::shared_ptr<LRAAction>> actions;

	const LRAStateObject& operator+=(const LRAStateObject& rhs);
	void copyPayloadIn(const LRAStateObject& rhs);
	void copyPayloadIn(const LRATransition& rhs) override;
	void copyPayloadOut(LRATransition& rhs) const override;
};

class LRA : public AFA<LRAStateObject, LRTag> {
public:
	LRATransition& addTransition(AFAState sourceState, AFAState targetState, const std::shared_ptr<SymbolGroup>& condition);
};