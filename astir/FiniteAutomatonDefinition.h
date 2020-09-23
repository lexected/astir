#pragma once

#include "SyntacticTree.h"
#include "MachineDefinition.h"

struct FiniteAutomatonDefinition : public MachineDefinition {
	FiniteAutomatonDefinition()
		: MachineDefinition({
				{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(true) },
				{ MachineFlag::ProductionsRootByDefault, MachineDefinitionAttribute(true) },
				{ MachineFlag::CategoriesRootByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::AmbiguityResolvedByPrecedence, MachineDefinitionAttribute(false) }
			}) { }

	void initialize() override;

	const NFA& getNFA() const { return m_nfa; }

	void accept(GenerationVisitor* visitor) const override;
private:
	std::shared_ptr<const FiniteAutomatonDefinition> m_finiteAutomatonDefinition;
	NFA m_nfa;
};