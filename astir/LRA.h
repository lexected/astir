#pragma once

#include <list>

#include "AFA.h"
#include "SymbolGroup.h"

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
	SymbolGroupPtrVector lookahead;

	LRTag(const TypeFormingStatement* statement, const SymbolGroupPtrVector& lookahead);
	LRTag(const TypeFormingStatement* statement, const SymbolGroupPtrVector&& lookahead);

	bool operator<(const LRTag& rhs) const;
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