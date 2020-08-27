#pragma once

#include <set>
#include <vector>
#include <list>

#include "Regex.h"

class Machine;
class MachineComponent;
struct Field;

using State = size_t;

enum class NFAActionType : unsigned char {
	Set = 1,
	Unset = 2,
	Flag = 3,
	Unflag = 4,
	Append = 5,
	Prepend = 6,
	Clear = 7,
	LeftTrim = 8,
	RightTrim = 9,

	CreateContext = 101,
	AssignContext = 102,
	AppendContext = 103,
	PrependContext = 104,

	None = 255
};

struct NFAAction {
	NFAActionType type;
	std::string contextPath;
	std::string targetPath;

	/*NFAAction(RegexActionType regexAction)
		: action((NFAActionType)regexAction), targetComponent(nullptr), contextPath() { }
	NFAAction(NFAActionType faAction)
		: action(faAction), targetComponent(nullptr), contextPath() { }
	NFAAction(NFAActionType faAction, const Field* targetComponent)
		: action(faAction), targetComponent(targetComponent), contextPath() { }*/
	NFAAction(NFAActionType faAction, const std::string& contextPath, const std::string& targetPath)
		: type(faAction), contextPath(contextPath), targetPath(targetPath) { }
};

class NFAActionRegister : public std::list<NFAAction> {
public:
	NFAActionRegister() = default;

	NFAActionRegister operator+(const NFAActionRegister& rhs) const;
	const NFAActionRegister& operator+=(const NFAActionRegister& rhs);
};

struct SymbolGroup {
public:
	virtual ~SymbolGroup() = default;

	// a necessary part of SymbolGroup I am afraid, the SymbolGroup is the literal payload for the action
	NFAActionRegister actions;

	virtual bool contains(const SymbolGroup* symbol) const = 0;

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

	bool contains(const SymbolGroup* symbol) const override;
protected:
};

struct LiteralSymbolGroup : public SymbolGroup {
	LiteralSymbolGroup()
		: rangeStart(0), rangeEnd(0) { }
	LiteralSymbolGroup(unsigned char rangeStart, unsigned  char rangeEnd)
		: rangeStart(rangeStart), rangeEnd(rangeEnd) { }
	LiteralSymbolGroup(unsigned char rangeStart, unsigned  char rangeEnd, const NFAActionRegister& actions)
		: rangeStart(rangeStart), rangeEnd(rangeEnd), SymbolGroup(actions) { }
	LiteralSymbolGroup(const LiteralSymbolGroup& lsg, const NFAActionRegister& actions)
		: rangeStart(lsg.rangeStart), rangeEnd(lsg.rangeEnd), SymbolGroup(actions) { }

	bool contains(const SymbolGroup* symbol) const override;
	bool equals(const LiteralSymbolGroup& rhs) const;
	bool disjoint(const LiteralSymbolGroup& lhs) const;
	static void disjoin(std::list<LiteralSymbolGroup>& symbolGroups, const LiteralSymbolGroup& lhs, const LiteralSymbolGroup& rhs);

	unsigned char rangeStart;
	unsigned char rangeEnd;
};

struct ArbitrarySymbolGroup : public LiteralSymbolGroup {
	ArbitrarySymbolGroup()
		: LiteralSymbolGroup(0, (char)255) { }
	ArbitrarySymbolGroup(const NFAActionRegister& actions)
		: LiteralSymbolGroup(0, (char)255, actions) { }
};

struct ProductionSymbolGroup : public SymbolGroup {
	const MachineComponent* referencedComponent;
	ProductionSymbolGroup(const MachineComponent* referencedComponent)
		: referencedComponent(referencedComponent) { }
	ProductionSymbolGroup(const MachineComponent* referencedComponent, const NFAActionRegister& actions)
		: referencedComponent(referencedComponent), SymbolGroup(actions) { }

	bool contains(const SymbolGroup* symbol) const override;
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
};

class NFA {
public:
	std::set<State> finalStates;
	std::vector<NFAState> states;
	// 0th element of this vector is by default the initial state

	NFA();

	void operator|=(const NFA& rhs);
	void operator&=(const NFA& rhs);

	void addContextedAlternative(const NFA& rhs, const std::string& targetGenerationPath, const std::string& sourceGenerationPath);

	State addState();
	void addTransition(State state, const Transition& transition);
	Transition& addEmptyTransition(State state, State target);
	Transition& addEmptyTransition(State state, State target, const NFAActionRegister& ar);
	State concentrateFinalStates();
	void addFinalActions(const NFAActionRegister& actions);

	NFA buildDFA() const;

	static void calculateDisjointLiteralSymbolGroups(std::list<LiteralSymbolGroup>& symbolGroups);
	static std::list<LiteralSymbolGroup> negateLiteralSymbolGroups(const std::list<LiteralSymbolGroup>& symbolGroups);

private:
	std::set<State> calculateEpsilonClosure(const std::set<State>& states) const;
	std::set<State> calculateSymbolClosure(const std::set<State>& states, const SymbolGroup* symbolOnTransition) const;
	std::list<std::shared_ptr<SymbolGroup>> calculateTransitionSymbols(const std::set<State>& states) const;
	static void calculateDisjointProductionSymbolGroups(std::list<ProductionSymbolGroup>& symbolGroups);

	struct DFAState {
		std::set<State> nfaStates;
		bool marked;

		DFAState()
			: marked(false) { }
		DFAState(const std::set<State>& nfaStates)
			: nfaStates(nfaStates), marked(false) { }
	};
	State findUnmarkedState(const std::vector<DFAState>& stateMap) const;
	State findStateByNFAStateSet(const std::vector<DFAState>& stateMap, const std::set<State>& nfaSet) const;
};
