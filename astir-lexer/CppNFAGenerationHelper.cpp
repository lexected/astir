#include "CppNFAGenerationHelper.h"

#include <sstream>

void CppNFAGenerationHelper::generateMechanicsMaps(std::string& stateMap, std::string& actionRegisterDeclarations, std::string& actionRegisterDefinitions, std::string& transitionActionMap, std::string& stateActionMap) const {
	std::stringstream stateMapStream;
	std::stringstream actionRegisterDeclarationStream;
	std::stringstream actionRegisterDefinitionStream;
	std::stringstream transitionActionMapStream;

	// counter for action registers
	ActionRegisterId actionRegistersUsed = 0;

	// the entire state action-register map, 0 (to be nullptr) by default
	std::vector<ActionRegisterId> stateActionRegisterMap(m_fa.states.size(), (ActionRegisterId)0);

	for (State state = 0; state < m_fa.states.size(); ++state) {
		const auto& stateObject = m_fa.states[state];

		// a single line of the state transition map, (State)-1 by default
		std::vector<std::vector<State>> transitionStateMapLine(256, std::vector<State>());

		// a single line of transition action-register map, 0 (to be nullptr) by default
		std::vector<std::vector<ActionRegisterId>> transitionActionRegisterMapLine(256, std::vector<ActionRegisterId>());

		// handle the action register of state actions
		const NFAActionRegister& snar = stateObject.actions;
		if (snar.size() > 0) {
			ActionRegisterId registerId = ++actionRegistersUsed;
			actionRegisterDeclarationStream << generateActionRegisterDeclaration(registerId, snar);
			actionRegisterDefinitionStream << generateActionRegisterDefinition(registerId, snar);

			stateActionRegisterMap[state] = registerId;
		}

		for (const auto& transition : stateObject.transitions) {
			const auto conditionSymbolIndices = transition.condition->retrieveSymbolIndices();
			for (SymbolIndex symbolIndex : *conditionSymbolIndices) {
				auto offsetMapLine = transitionStateMapLine.begin() + symbolIndex;
				offsetMapLine->push_back(transition.target);
			}

			// handle the action register of transition actions
			const NFAActionRegister& tnar = transition.condition->actions;
			ActionRegisterId createdRegisterId;
			if (tnar.size() > 0) {
				createdRegisterId = ++actionRegistersUsed;
				actionRegisterDeclarationStream << generateActionRegisterDeclaration(createdRegisterId, tnar);
				actionRegisterDefinitionStream << generateActionRegisterDefinition(createdRegisterId, tnar);
			} else {
				createdRegisterId = 0;
			}

			for (SymbolIndex symbolIndex : *conditionSymbolIndices) {
				auto offsetMapLine = transitionActionRegisterMapLine.begin() + symbolIndex;
				offsetMapLine->push_back(createdRegisterId);
			}
		}

		stateMapStream << "{ ";
		for (const std::vector<State>& transitionStateMapEntryVector : transitionStateMapLine) {
			stateMapStream << "{ ";
			for (auto it = transitionStateMapEntryVector.crbegin(); it != transitionStateMapEntryVector.crend(); ++it) {
				stateMapStream << *it << ", ";
			}
			stateMapStream << "}, ";
		}
		stateMapStream << "},\n";

		transitionActionMapStream << "{ ";
		for (const std::vector<ActionRegisterId>& actionMapEntryVector : transitionActionRegisterMapLine) {
			transitionActionMapStream << "{ ";
			for (auto it = actionMapEntryVector.crbegin(); it != actionMapEntryVector.crend(); ++it) {
				if (*it == (ActionRegisterId)0) {
					transitionActionMapStream << "nullptr, ";
				} else {
					transitionActionMapStream << "&actionRegister" << *it << ", ";
				}
			}
			transitionActionMapStream << "}, ";
		}
		transitionActionMapStream << "},\n";
	}

	std::stringstream stateActionMapStream;
	for (State state = 0; state < m_fa.states.size(); ++state) {
		if (stateActionRegisterMap[state] == (ActionRegisterId)0) {
			stateActionMapStream << "nullptr, ";
		} else {
			stateActionMapStream << "&actionRegister" << stateActionRegisterMap[state] << ", ";
		}
	}

	stateMap = stateMapStream.str();
	actionRegisterDeclarations = actionRegisterDeclarationStream.str();
	actionRegisterDefinitions = actionRegisterDefinitionStream.str();
	transitionActionMap = transitionActionMapStream.str();
	stateActionMap = stateActionMapStream.str();

}

std::string CppNFAGenerationHelper::generateContextDeclarations() const {
	std::stringstream ss;

	for (const auto& contextParentChildPair : m_fa.contexts) {
		ss << "std::shared_ptr<" << contextParentChildPair.second << "> " << contextParentChildPair.first << "__" << contextParentChildPair.second << ';' << std::endl;
	}

	return ss.str();
}

std::string CppNFAGenerationHelper::generateStateFinality() const {
	std::stringstream ss;

	for (State state = 0; state < m_fa.states.size(); ++state) {
		if (m_fa.finalStates.contains(state)) {
			ss << "true, ";
		} else {
			ss << "false, ";
		}
	}

	return ss.str();
}

std::string CppNFAGenerationHelper::generateActionRegisterDeclaration(ActionRegisterId registerId, const NFAActionRegister& nar) const {
	std::stringstream actionRegisterDeclarationStream;

	actionRegisterDeclarationStream
		<< "void actionRegister" << registerId
		<< "(size_t position, const std::deque<InputTerminalPtr>& input, const std::shared_ptr<Location>& location);"
		<< std::endl
		;

	return actionRegisterDeclarationStream.str();
}

std::string CppNFAGenerationHelper::generateActionRegisterDefinition(ActionRegisterId registerId, const NFAActionRegister& nar) const {
	std::stringstream ss;
	ss << "void " << m_machineName << "::" << "actionRegister" << registerId << "(size_t position, const std::deque<InputTerminalPtr>& input, const std::shared_ptr<Location>& location) {" << std::endl;

	for (const auto& action : nar) {
		ss << generateActionOperation(action);
	}

	ss << "}" << std::endl << std::endl;

	return ss.str();
}

std::string CppNFAGenerationHelper::generateActionOperation(const NFAAction& na) const {
	std::stringstream output;
	output << '\t';
	switch (na.type) {
		case NFAActionType::Flag:
			output << na.contextPath << "->" << na.targetName << " = true;" << std::endl;
			break;
		case NFAActionType::Unflag:
			output << na.contextPath << "->" << na.targetName << " = false;" << std::endl;
			break;

		case NFAActionType::InitiateCapture:
			output << "m_captureStack.push(position);" << std::endl;
			break;
		case NFAActionType::Capture:
			output << "{" << std::endl;
			output << "\t\tauto stackPos = m_captureStack.top();" << std::endl;
			output << "\t\tm_captureStack.pop();" << std::endl;
			output << "\t\tstd::stringstream ss;" << std::endl;
			output << "\t\tfor(auto it = input.cbegin()+stackPos;it < input.cbegin()+position+1;++it) {" << std::endl;
			output << "\t\t\tss << (*it)->raw;" << std::endl;
			output << "\t\t}" << std::endl;
			output << "\t\t" << na.contextPath << "->" << na.targetName << " = ss.str();" << std::endl; // +1 is to recognize the fact that position points at the current character payload and not beyond it
			output << "\t}" << std::endl;
			break;
		case NFAActionType::Empty:
			output << na.contextPath << "->" << na.targetName << ".clear();" << std::endl;
			break;
		case NFAActionType::Append:
			output << "{" << std::endl;
			output << "\t\tauto stackPos = m_captureStack.top();" << std::endl;
			output << "\t\tm_captureStack.pop();" << std::endl;
			output << "\t\tstd::stringstream ss;" << std::endl;
			output << "\t\tfor(auto it = input.cbegin()+stackPos;it < input.cbegin()+position+1;++it) {" << std::endl;
			output << "\t\t\tss << (*it)->raw;" << std::endl;
			output << "\t\t}" << std::endl;
			output << "\t\t" << na.contextPath << "->" << na.targetName << ".append(ss.str());" << std::endl; // +1 is to recognize the fact that position points at the current character payload and not beyond it
			output << "\t}" << std::endl;
			break;
		case NFAActionType::Prepend:
			output << "{" << std::endl;
			output << "\t\tauto stackPos = m_captureStack.top();" << std::endl;
			output << "\t\tm_captureStack.pop();" << std::endl;
			output << "\t\tstd::stringstream ss;" << std::endl;
			output << "\t\tfor(auto it = input.cbegin()+stackPos;it < input.cbegin()+position+1;++it) {" << std::endl;
			output << "\t\t\tss << (*it)->raw;" << std::endl;
			output << "\t\t}" << std::endl;
			output << "\t\t" << na.contextPath << "->" << na.targetName << ".insert(0, ss.str());" << std::endl; // +1 is to recognize the fact that position points at the current character payload and not beyond it
			output << "\t}" << std::endl;
			break;

		case NFAActionType::CreateContext:
			output << na.contextPath << "__" << na.targetName << " = std::make_shared<" << na.targetName << ">(location);" << std::endl;
			output << "\tm_captureStack.push(position);" << std::endl;
			break;
		case NFAActionType::TerminalizeContext:
			output << "{" << std::endl;
			output << "\t\tauto stackPos = m_captureStack.top();" << std::endl;
			output << "\t\tm_captureStack.pop();" << std::endl;
			output << "\t\tstd::stringstream ss;" << std::endl;
			output << "\t\tfor(auto it = input.cbegin()+stackPos;it < input.cbegin()+position+1;++it) {" << std::endl;
			output << "\t\t\tss << (*it)->raw;" << std::endl;
			output << "\t\t}" << std::endl;
			output << "\t\t" << na.contextPath << "__" << na.targetName << "->raw = ss.str();" << std::endl; // +1 is to recognize the fact that position points at the current character payload and not beyond it
			output << "\t}" << std::endl;
			break;
		case NFAActionType::ElevateContext:
			output << na.contextPath << " = " << na.contextPath << "__" << na.targetName << ';' << std::endl;
			break;
		case NFAActionType::Set:
			output << na.contextPath << "->" << na.targetName << " = " << na.payload << ';' << std::endl;
			break;
		case NFAActionType::Unset:
			output << na.contextPath << "->" << na.targetName << " = nullptr;" << std::endl;
			break;
		case NFAActionType::Push:
			output << na.contextPath << "->" << na.targetName << ".push_back(" << na.payload << ");" << std::endl;
			break;
		case NFAActionType::Pop:
			output << na.contextPath << "->" << na.targetName << ".pop_back();" << std::endl;
			break;
		case NFAActionType::Clear:
			output << na.contextPath << "->" << na.targetName << ".clear();" << std::endl;
			break;
	}

	return output.str();
}
