#pragma once

#include "ISemanticEntity.h"
#include "IGenerationVisitable.h"

struct Field : public ISyntacticEntity, public IGenerationVisitable {
	std::string name;

	virtual bool flaggable() const = 0;
	virtual bool settable() const = 0;
	virtual bool listable() const = 0;
};

struct FlagField : public Field {
	bool flaggable() const override;
	bool settable() const override;
	bool listable() const override;

	void accept(GenerationVisitor* visitor) const override;
};

struct RawField : public Field {
	bool flaggable() const override;
	bool settable() const override;
	bool listable() const override;

	void accept(GenerationVisitor* visitor) const override;
};

struct VariablyTypedField : public Field {
	std::string type;
};

struct ItemField : public VariablyTypedField {
	bool flaggable() const override;
	bool settable() const override;
	bool listable() const override;

	void accept(GenerationVisitor* visitor) const override;
};

struct ListField : public VariablyTypedField {
	bool flaggable() const override;
	bool settable() const override;
	bool listable() const override;

	void accept(GenerationVisitor* visitor) const override;
};
