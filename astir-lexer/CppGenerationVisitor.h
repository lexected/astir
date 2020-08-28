#pragma once

#include "GenerationVisitor.h"

class CppGenerationVisitor : public GenerationVisitor {
public:
	CppGenerationVisitor(const std::string& folderPath)
		: GenerationVisitor(folderPath) { }

	void setup() const override;

	void visit(const SemanticTree* tree) override;
	void visit(const FiniteAutomatonMachine* tree) override;
private:
	static std::string generateTypeDeclarations(const std::list<const MachineComponent*>& relevantComponents);
	static std::string generateStateMap(const NFA& fa);
	static std::string generateStateFinality(const NFA& fa);
};

