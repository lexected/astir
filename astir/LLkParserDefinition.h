#pragma once

#include "SyntacticTree.h"
#include "MachineDefinition.h"

#include "LLkBuilder.h"
#include "LLkFirster.h"

struct LLkParserDefinition : public MachineDefinition {
	LLkParserDefinition()
		: MachineDefinition({
				{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::ProductionsRootByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::CategoriesRootByDefault, MachineDefinitionAttribute(false) },
			}), m_builder(10, *this), m_firster(*this) { }

	void initialize() override;

	void accept(GenerationVisitor* visitor) const override;
private:
	LLkBuilder m_builder;
	LLkFirster m_firster;
};