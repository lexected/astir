#pragma once

#include <sstream>
#include <set>

#include "GenerationVisitor.h"
#include "FiniteAutomatonDefinition.h"

class CppGenerationVisitor : public GenerationVisitor {
public:
	CppGenerationVisitor(const std::string& folderPath)
		: GenerationVisitor(folderPath), m_hasIncludedRawStreamFiles(false) { }

	void setup() const override;

	void visit(const SyntacticTree* tree) override;
	void visit(const FiniteAutomatonDefinition* tree) override;
	void visit(const LLkParserDefinition* llkParserDefinition) override;

	void visit(const TypeFormingStatement* component) override;
	void visit(const FlagField* flagField) override;
	void visit(const RawField* rawField) override;
	void visit(const ItemField * itemField) override;
	void visit(const ListField * listField) override;

private:
	void buildUniversalMachineMacros(std::map<std::string, std::string>& macros, const MachineDefinition* machine);
	std::string combineForwardDeclarationsAndClear();

	std::stringstream m_output;
	std::set<std::string> m_typeFormingStatementsVisited;
	void resetOutput();
	std::string outputAndReset();

	bool m_hasIncludedRawStreamFiles;
};

