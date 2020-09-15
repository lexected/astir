#pragma once

#include "SyntacticTree.h"
#include "MachineDefinition.h"

struct LLkParserDefinition : public MachineDefinition {
	LLkParserDefinition()
		: MachineDefinition({
				{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::ProductionsRootByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::CategoriesRootByDefault, MachineDefinitionAttribute(false) },
			}) { }

	void initialize() override;

	void accept(GenerationVisitor* visitor) const override;
private:
};