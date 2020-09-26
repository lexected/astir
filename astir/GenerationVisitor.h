#pragma once

#include <string>
#include "Field.h"
#include "SyntacticTree.h"
#include "FiniteAutomatonDefinition.h"
#include "LLkParserDefinition.h"
#include "MachineStatement.h"

#include <filesystem>

class GenerationVisitor {
public:
	virtual void setup() const = 0;

	virtual void visit(const SyntacticTree* tree) = 0;
	virtual void visit(const FiniteAutomatonDefinition* fad) = 0;
	virtual void visit(const LLkParserDefinition* llkParserDefinition) = 0;

	virtual void visit(const TypeFormingStatement* statement) = 0;
	virtual void visit(const FlagField* flagField) = 0;
	virtual void visit(const RawField* rawField) = 0;
	virtual void visit(const ItemField* itemField) = 0;
	virtual void visit(const ListField* listField) = 0;
protected:
	GenerationVisitor(const std::string& path)
		: m_folderPath(path) { }

	const std::filesystem::path m_folderPath;
};

