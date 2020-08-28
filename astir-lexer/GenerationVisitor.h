#pragma once

#include <string>
#include "Field.h"
#include "SemanticTree.h"

#include <filesystem>

class GenerationVisitor {
public:
	virtual void setup() const = 0;

	virtual void visit(const SemanticTree* tree) = 0;
	virtual void visit(const FiniteAutomatonMachine* tree) = 0;

	virtual void visit(const FlagField* flagField) = 0;
	virtual void visit(const RawField* rawField) = 0;
	virtual void visit(const ItemField* itemField) = 0;
	virtual void visit(const ListField* listField) = 0;

	virtual void visit(const NFAActionRegister* actionRegister) = 0;
	virtual void visit(const NFAAction* actionRegister) = 0;
protected:
	GenerationVisitor(const std::string& path)
		: m_folderPath(path) { }

	const std::filesystem::path m_folderPath;
};

