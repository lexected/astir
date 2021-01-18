#pragma once

#include "SyntacticTree.h"
#include "MachineDefinition.h"

#include "LLkBuilder.h"
#include "LLkFirster.h"

#include <memory>

struct LLkParserDefinition : public MachineDefinition {
public:
	LLkParserDefinition(unsigned long k);

	void initialize() override;

	void accept(GenerationVisitor* visitor) const override;

	LLkBuilder& builder() const { return *m_builder; }
	unsigned long k() const { return m_k; }
private:
	unsigned long m_k;
	std::unique_ptr<LLkBuilder> m_builder;
};