#pragma once

#include <string>
#include "SemanticTree.h"
#include "NFA.h"

class GenerationVisitor {
public:
	virtual void setup() const = 0;

	virtual void visit(const SemanticTree* tree) = 0;
	virtual void visit(const FiniteAutomatonMachine* tree) = 0;
protected:
	GenerationVisitor(const std::string& path)
		: m_folderPath(path) { }

	std::string m_folderPath;
};

