#pragma once

#include "ISemanticEntity.h"
#include "IGenerationVisitable.h"

struct Field : public ISyntacticEntity, public IGenerationVisitable {
	std::string name;
};

struct FlagField : public Field {
	void accept(GenerationVisitor* visitor) const override;
};

struct RawField : public Field {
	void accept(GenerationVisitor* visitor) const override;
};

struct MachineDefinition;
struct VariablyTypedField : public Field {
	std::string type;
	const MachineDefinition* machineOfTheType;

	VariablyTypedField()
		: type(), machineOfTheType(nullptr) { }
};

struct ItemField : public VariablyTypedField {
	void accept(GenerationVisitor* visitor) const override;
};

struct ListField : public VariablyTypedField {
	void accept(GenerationVisitor* visitor) const override;
};
