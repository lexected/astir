#pragma once

#include <set>
#include <deque>
#include <vector>
#include <list>
#include <map>

#include "Regex.h"
#include "NFAAction.h"
#include "SymbolGroup.h"

#include "AFA.h"

struct Transition : public AFATransition<SymbolGroup> {
	NFAActionRegister actions;

	Transition(AFAState target)
		: AFATransition<SymbolGroup>(target) { }
	Transition(AFAState target, const std::shared_ptr<SymbolGroup>& condition, const NFAActionRegister& actions, bool doNotOptimizeTargetIntoConditionClosure = false)
		: AFATransition<SymbolGroup>(target, condition, doNotOptimizeTargetIntoConditionClosure), actions(actions) { }
	Transition(AFAState target, const std::shared_ptr<SymbolGroup>& condition)
		: Transition(target, condition, NFAActionRegister()) { }

	bool equals(const AFATransition<SymbolGroup>& transition) const override;
	bool canBeMerged() const override;

	bool alignedSymbolWise(const Transition& rhs) const;
	std::list<Transition> disjoinFrom(const Transition& rhs);
};

using TransitionList = std::list<Transition>;

class NFAStateObject : public AFAStateObject<Transition> {
public:
	NFAActionRegister actions;

	const NFAStateObject& operator+=(const NFAStateObject& rhs);
	void copyPayloadIn(const NFAStateObject& rhs);
	void copyPayloadIn(const Transition& rhs) override;
	void copyPayloadOut(Transition& rhs) const override;

protected:
	const AFAStateObject<Transition>& operator+=(const AFAStateObject<Transition>& rhs) override;
	void copyPayloadIn(const AFAStateObject<Transition>& rhs) override;
};

using CapturePointId = size_t;

class NFA : public AFA<NFAStateObject, unsigned int> {
public:
	std::list<std::pair<std::string, std::string>> contexts; // parent context name, subcontext name (also the type)

	NFA() = default;
	NFA(const NFA& rhs) = default;
	NFA(const AFA<NFAStateObject, unsigned int>& rhs);
	NFA(AFA<NFAStateObject, unsigned int>&& rhs);

	void orNFA(const NFA& rhs, bool preventSymbolClosureOptimisation);
	void andNFA(const NFA& rhs, bool preventSymbolClosureOptimisation);
	void operator|=(const NFA& rhs);
	void operator&=(const NFA& rhs);

	Transition& addEmptyTransition(AFAState state, AFAState target);
	Transition& addEmptyTransition(AFAState state, AFAState target, const NFAActionRegister& ar);
	void registerContext(const std::string& parentContextName, const std::string& name);

	void addFinalActions(const NFAActionRegister& actions);
	void addInitialActions(const NFAActionRegister& actions);
	AFAState concentrateFinalStates();
	AFAState concentrateFinalStates(const NFAActionRegister& actions);

	NFA buildPseudoDFA() const;

	static void calculateDisjointTransitions(std::list<Transition>& symbolGroups);
	static std::list<std::shared_ptr<ByteSymbolGroup>> makeComplementSymbolGroups(const std::list<std::shared_ptr<SymbolGroup>>& symbolGroups);

private:
	void mergeInContexts(const NFA& rhs);

	std::list<Transition> calculateTransitions(const std::set<AFAState>& states) const override;
};
