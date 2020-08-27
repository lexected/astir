#pragma once

#include "GenerationVisitor.h"

class CppGenerationVisitor : public GenerationVisitor {
public:
	CppGenerationVisitor(const std::string& folderPath)
		: GenerationVisitor(folderPath) { }

	void setup() const override;

	void visit(const SemanticTree* tree) override;
	void visit(const FiniteAutomatonMachine* tree) override;
};

