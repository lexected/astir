#include "LLkParserDefinition.h"
#include "GenerationVisitor.h"

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
