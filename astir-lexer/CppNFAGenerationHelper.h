#pragma once

#include <string>

#include "NFA.h"

using ActionRegisterId = unsigned long;

class CppNFAGenerationHelper {
public:
	CppNFAGenerationHelper(const std::string& machineName, const NFA& fa, const size_t inputTerminalCount)
		: m_machineName(machineName), m_fa(fa), m_inputTerminalCount(inputTerminalCount) { }

	void generateMechanicsMaps(std::string& stateMap, std::string& actionRegisterDeclarations, std::string& actionRegisterDefinitions, std::string& transitionActionMap, std::string& stateActionMap) const;
	std::string generateContextDeclarations() const;
	std::string generateStateFinality() const;
private:
	const std::string& m_machineName;
	const NFA& m_fa;
	const size_t m_inputTerminalCount;

	std::string generateActionRegisterDeclaration(ActionRegisterId registerId, const NFAActionRegister& nar) const;
	std::string generateActionRegisterDefinition(ActionRegisterId registerId, const NFAActionRegister& nar) const;
	std::string generateActionOperation(const NFAAction& na) const;
};

