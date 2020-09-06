#pragma once

#include "SemanticTree.h"
#include "Regex.h"
#include "NFA.h"

class Machine;

class NFABuilder {
public:
	NFABuilder(const Machine& context, const MachineComponent* component, const std::string& generationContextPath)
		: m_contextMachine(context), m_contextComponent(component), m_generationContextPath(generationContextPath) { }

	NFA visit(const Category* category) const;
	NFA visit(const Pattern* rule) const;
	NFA visit(const Production* rule) const;

	NFA visit(const DisjunctiveRegex* regex) const;
	NFA visit(const ConjunctiveRegex* regex) const;

	NFA visit(const RepetitiveRegex* regex) const;
	NFA visit(const LookaheadRegex* regex) const;
	
	NFA visit(const AnyRegex* regex) const;
	NFA visit(const ExceptAnyRegex* regex) const;
	NFA visit(const LiteralRegex* regex) const;
	NFA visit(const ArbitrarySymbolRegex* regex) const;
	NFA visit(const ReferenceRegex* regex) const; 
	NFA visit(const LineEndRegex* regex) const;
private:
	const Machine& m_contextMachine;
	const MachineComponent* m_contextComponent;
	const std::string m_generationContextPath;

	std::list<std::shared_ptr<SymbolGroup>> makeLiteralGroups(const AnyRegex* regex) const;
	std::pair<NFAActionRegister, NFAActionRegister> computeActionRegisterEntries(const std::list<RegexAction>& actions) const;
	std::pair<NFAActionRegister, NFAActionRegister> computeActionRegisterEntries(const std::list<RegexAction>& actions, const std::string& payload) const;

	std::shared_ptr<SymbolGroup> createArbitrarySymbolGroup(const NFAActionRegister& actions) const;
};

