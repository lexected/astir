#pragma once

#include <sstream>
#include <set>

#include "GenerationVisitor.h"
#include "NFA.h"

using ActionRegisterId = unsigned long;

class CppGenerationVisitor : public GenerationVisitor {
public:
	CppGenerationVisitor(const std::string& folderPath)
		: GenerationVisitor(folderPath), m_actionRegistersUsed(0) { }

	void setup() const override;

	void visit(const SemanticTree* tree) override;
	void visit(const FiniteAutomatonMachine* tree) override;

	void visit(const FlagField* flagField) override;
	void visit(const RawField* rawField) override;
	void visit(const ItemField * itemField) override;
	void visit(const ListField * listField) override;

	void visit(const NFAActionRegister* actionRegister) override;
	void visit(const NFAAction* actionRegister) override;
private:
	std::string generateTypeDeclarations(const std::list<const MachineComponent*>& relevantComponents);
	void generateAutomatonMechanicsMaps(const std::string& machineName, const NFA& fa, std::string& stateMap, std::string& actionRegisterDeclarations, std::string& actionRegisterDefinitions, std::string& transitionActionMap, std::string& stateActionMap);
	std::string generateAutomatonContextDeclarations(const NFA& fa) const;
	std::string generateStateFinality(const NFA& fa) const;
	std::string generateActionRegisterDefinition(const std::string& machineName, ActionRegisterId registerId, const NFAActionRegister& nar, bool isStateAction);

	std::stringstream m_output;
	void resetOutput();
	std::string outputAndReset();

	ActionRegisterId m_actionRegistersUsed;
};

