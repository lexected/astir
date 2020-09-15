#include "LLkParserDefinition.h"

void LLkParserDefinition::initialize() {
	if (initialized()) { // really necessary
		return;
	}

	this->MachineDefinition::initialize();
}
