#pragma once

#include "SyntacticTree.h"
#include "MachineDefinition.h"

#include "LLkBuilder.h"
#include "LLkFirster.h"

#include <memory>

struct LLkParserDefinition : public MachineDefinition {
	LLkParserDefinition()
		: MachineDefinition({
				{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::ProductionsRootByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::CategoriesRootByDefault, MachineDefinitionAttribute(false) },
			}), m_builder(std::make_unique<LLkBuilder>(10, *this)) { }

	void initialize() override;

	void accept(GenerationVisitor* visitor) const override;

	LLkBuilder& builder() const { return *m_builder; }
private:
	std::unique_ptr<LLkBuilder> m_builder;
};