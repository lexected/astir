#include "LLkParserDefinition.h"
#include "GenerationVisitor.h"

void LLkParserDefinition::initialize() {
	if (initialized()) { // really necessary
		return;
	}

	this->MachineDefinition::initialize();

	auto roots = this->getRoots();
	m_builder->visitRootDisjunction(roots);
}

void LLkParserDefinition::accept(GenerationVisitor* visitor) const {
	visitor->visit(this);
}
