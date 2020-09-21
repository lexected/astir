#include "LLkParserDefinition.h"
#include "GenerationVisitor.h"

LLkParserDefinition::LLkParserDefinition(unsigned long k)
	: MachineDefinition({
				{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::ProductionsRootByDefault, MachineDefinitionAttribute(false) },
				{ MachineFlag::CategoriesRootByDefault, MachineDefinitionAttribute(false) },
	}), m_builder(std::make_unique<LLkBuilder>(this)), m_k(k) { }

void LLkParserDefinition::initialize() {
	if (initialized()) { // really necessary
		return;
	}

	this->MachineDefinition::initialize();

	for (const auto& statementPair : statements) {
		auto statementAsLLkBuilding = dynamic_cast<ILLkBuildingCPtr>(statementPair.second.get());
		statementAsLLkBuilding->accept(m_builder.get());
	}

	auto roots = this->getRoots();
	m_builder->visitRootDisjunction(roots);
}

void LLkParserDefinition::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}
