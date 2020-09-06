#pragma once

#include <set>
#include <deque>
#include <vector>
#include <list>
#include <map>

#include "Regex.h"
#include "NFAAction.h"

class Machine;
class MachineComponent;
class Production;

using State = size_t;
using SymbolIndex = size_t;

struct SymbolGroup {
public:
	virtual ~SymbolGroup() = default;

	// a necessary part of SymbolGroup I am afraid, the SymbolGroup is the literal payload for the action
	NFAActionRegister actions;

	virtual bool contains(const SymbolGroup* rhs) const = 0;
	virtual bool equals(const SymbolGroup* rhs) const = 0;
	virtual bool disjoint(const SymbolGroup* rhs) const = 0;
	virtual std::list<std::shared_ptr<SymbolGroup>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) = 0;

	virtual std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const = 0;

protected:
	SymbolGroup() = default;
	SymbolGroup(const NFAActionRegister& actions)
		: actions(actions) { }
};

struct EmptySymbolGroup : public SymbolGroup {
public:
	EmptySymbolGroup()
		: SymbolGroup() { }
	EmptySymbolGroup(const NFAActionRegister & actions)
		: SymbolGroup(actions) { }

	bool contains(const SymbolGroup* rhs) const override;
	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::shared_ptr<SymbolGroup>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
protected:
};

struct LiteralSymbolGroup : public SymbolGroup {
	LiteralSymbolGroup()
		: LiteralSymbolGroup(0, 0) { }
	LiteralSymbolGroup(CharType rangeStart, CharType rangeEnd)
		: rangeStart(rangeStart), rangeEnd(rangeEnd), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }
	LiteralSymbolGroup(CharType rangeStart, CharType rangeEnd, const NFAActionRegister& actions)
		: rangeStart(rangeStart), rangeEnd(rangeEnd), SymbolGroup(actions), m_symbolIndicesFlyweight(std::make_shared<std::list<SymbolIndex>>()) { }
	LiteralSymbolGroup(const LiteralSymbolGroup& lsg, const NFAActionRegister& actions)
		: LiteralSymbolGroup(lsg.rangeStart, lsg.rangeEnd, actions) { }
	
	bool contains(const SymbolGroup* rhs) const override;
	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::shared_ptr<SymbolGroup>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	CharType rangeStart;
	CharType rangeEnd;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
private:
	std::shared_ptr<std::list<SymbolIndex>> m_symbolIndicesFlyweight;
};

struct TerminalSymbolGroup : public SymbolGroup {
	std::list<const Production*> referencedProductions;
	TerminalSymbolGroup(const std::list<const Production*>& referencedProductions)
		: referencedProductions(referencedProductions) { }
	TerminalSymbolGroup(const std::list<const Production*>& referencedProductions, const NFAActionRegister& actions)
		: referencedProductions(referencedProductions), SymbolGroup(actions) { }

	bool contains(const SymbolGroup* rhs) const override;
	bool equals(const SymbolGroup* rhs) const override;
	bool disjoint(const SymbolGroup* rhs) const override;
	std::list<std::shared_ptr<SymbolGroup>> disjoinFrom(const std::shared_ptr<SymbolGroup>& rhs) override;

	std::shared_ptr<std::list<SymbolIndex>> retrieveSymbolIndices() const override;
private:
	std::shared_ptr<std::list<SymbolIndex>> m_symbolIndicesFlyweight;
};

struct Transition {
	std::shared_ptr<SymbolGroup> condition;
	State target;

	Transition(State target)
		: target(target), condition(nullptr) { }
	Transition(State target, const std::shared_ptr<SymbolGroup>& condition)
		: target(target), condition(condition) { }
};

using TransitionList = std::list<Transition>;

struct NFAState {
	TransitionList transitions;
	NFAActionRegister actions;
};

using CapturePointId = size_t;

class NFA {
public:
	std::set<State> finalStates;
	std::vector<NFAState> states; // 0th element of this vector is by default the initial state
	std::list<std::pair<std::string, std::string>> contexts; // parent context name, subcontext name (also the type)

	NFA();

	void operator|=(const NFA& rhs);
	void operator&=(const NFA& rhs);

	State addState();
	void addTransition(State state, const Transition& transition);
	Transition& addEmptyTransition(State state, State target);
	Transition& addEmptyTransition(State state, State target, const NFAActionRegister& ar);
	void registerContext(const std::string& parentContextName, const std::string& name);

	void addFinalActions(const NFAActionRegister& actions);
	void addInitialTransitionActions(const NFAActionRegister& actions);
	State concentrateFinalStates();
	State concentrateFinalStates(const NFAActionRegister& actions);

	NFA buildPseudoDFA() const;

	static void calculateDisjointSymbolGroups(std::list<std::shared_ptr<SymbolGroup>>& symbolGroups);
	static std::list<std::shared_ptr<LiteralSymbolGroup>> makeComplementSymbolGroups(const std::list<std::shared_ptr<SymbolGroup>>& symbolGroups);

private:
	void mergeInContexts(const NFA& rhs);

	struct DFAState {
		std::set<State> nfaStates;
		bool marked;
		NFAActionRegister actions;

		DFAState()
			: marked(false) { }
		DFAState(const std::set<State>& nfaStates, const NFAActionRegister& actions)
			: nfaStates(nfaStates), marked(false), actions(actions) { }
	};

	DFAState calculateEpsilonClosure(const std::set<State>& states) const;
	std::set<State> calculateSymbolClosure(const std::set<State>& states, const SymbolGroup* symbolOnTransition) const;
	std::list<std::shared_ptr<SymbolGroup>> calculateTransitionSymbols(const std::set<State>& states) const;
	

	State findUnmarkedState(const std::deque<DFAState>& stateMap) const;
	State findStateByNFAStateSet(const std::deque<DFAState>& stateMap, const std::set<State>& nfaSet) const;
};
