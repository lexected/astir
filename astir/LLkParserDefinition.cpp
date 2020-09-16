#include "LLkParserDefinition.h"

void LLkParserDefinition::initialize() {
	if (initialized()) { // really necessary
		return;
	}

	this->MachineDefinition::initialize();

	auto roots = this->getRoots();
	for (const auto& statementPtr : roots) {
		ILLkBuildingCPtr statementAsBuildable = statementPtr.get();
		statementAsBuildable->accept(&m_builder);
	}
}

void LLkParserDefinition::accept(GenerationVisitor* visitor) const {
	//TODO: implement
}
