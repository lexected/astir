#pragma once

#include <sstream>
#include <set>

#include "GenerationVisitor.h"

class CppGenerationVisitor : public GenerationVisitor {
public:
	CppGenerationVisitor(const std::string& folderPath)
		: GenerationVisitor(folderPath), m_hasIncludedRawStreamFiles(false) { }

	void setup() const override;

	void visit(const SemanticTree* tree) override;
	void visit(const FiniteAutomatonMachine* tree) override;

	void visit(const MachineComponent* component) override;
	void visit(const FlagField* flagField) override;
	void visit(const RawField* rawField) override;
	void visit(const ItemField * itemField) override;
	void visit(const ListField * listField) override;

private:
	std::stringstream m_output;
	void resetOutput();
	std::string outputAndReset();

	bool m_hasIncludedRawStreamFiles;
};

