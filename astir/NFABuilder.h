#pragma once

#include "SyntacticTree.h"
#include "Regex.h"
#include "NFA.h"

class NFABuilder {
public:
	NFABuilder(const MachineDefinition& context, const MachineStatement* statement, const std::string& generationContextPath)
		: m_contextMachine(context), m_contextStatement(statement), m_generationContextPath(generationContextPath) { }

	NFA visit(const CategoryStatement* category) const;
	NFA visit(const PatternStatement* rule) const;
	NFA visit(const ProductionStatement* rule) const;
	NFA visit(const RegexStatement* rule) const;

	NFA visit(const DisjunctiveRegex* regex) const;
	NFA visit(const ConjunctiveRegex* regex) const;

	NFA visit(const RepetitiveRegex* regex) const;
	
	NFA visit(const EmptyRegex* regex) const;
	NFA visit(const AnyRegex* regex) const;
	NFA visit(const ExceptAnyRegex* regex) const;
	NFA visit(const LiteralRegex* regex) const;
	NFA visit(const ArbitrarySymbolRegex* regex) const;
	NFA visit(const ReferenceRegex* regex) const;

private:
	const MachineDefinition& m_contextMachine;
	const MachineStatement* m_contextStatement;
	const std::string m_generationContextPath;

	std::list<std::shared_ptr<SymbolGroup>> makeLiteralGroups(const AnyRegex* regex) const;
	std::pair<NFAActionRegister, NFAActionRegister> computeActionRegisterEntries(const std::list<RegexAction>& actions) const;
	std::pair<NFAActionRegister, NFAActionRegister> computeActionRegisterEntries(const std::list<RegexAction>& actions, const std::string& payload) const;

	std::shared_ptr<SymbolGroup> createArbitrarySymbolGroup() const;
};

