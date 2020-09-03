#include "CppGenerationVisitor.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "GenerationHelper.h"

void CppGenerationVisitor::setup() const {
	// TODO: improve error handling here... what if m_folderPath is actually a file?? throw an exception
	if(!std::filesystem::is_directory(m_folderPath)) {
		std::filesystem::create_directory(m_folderPath);
	}
	std::filesystem::copy_file("Resources/RawStream.h", m_folderPath / "RawStream.h", std::filesystem::copy_options::overwrite_existing);
	std::filesystem::copy_file("Resources/RawStream.cpp", m_folderPath / "RawStream.cpp", std::filesystem::copy_options::overwrite_existing);
}

void CppGenerationVisitor::visit(const SemanticTree* tree) {
	for (const auto machinePair : tree->machines) {
		machinePair.second->accept(this);
	}
}

void CppGenerationVisitor::visit(const FiniteAutomatonMachine* machine) {
	std::ifstream specimenFaHeader("Resources/SpecimenFiniteAutomaton.sh");
	std::string specimenFaHeaderContents((std::istreambuf_iterator<char>(specimenFaHeader)), (std::istreambuf_iterator<char>()));

	std::ifstream specimenFaCode("Resources/SpecimenFiniteAutomaton.scpp");
	std::string specimenFaCodeContents((std::istreambuf_iterator<char>(specimenFaCode)), (std::istreambuf_iterator<char>()));

	// do constructive magic
	std::map<std::string, std::string> macros;
	// in particular, we need to
	//  - register all the name macros
	macros.emplace("MachineName", machine->name);
	
	//  - enumerate all the terminal types
	auto terminalTypeComponents = machine->getTerminalTypeComponents();
	std::stringstream ss;
	for (auto componentPtr : terminalTypeComponents) {
		ss << componentPtr->name << "," << std::endl;
	}
	macros.emplace("TerminalTypesEnumerated", ss.str());

	//  - generate type declarations
	auto typeComponents = machine->getTypeComponents();
	const std::string& generatedClasses = generateTypeDeclarations(typeComponents);
	macros.emplace("TypeDeclarations", generatedClasses);

	//  - set the DFA table parameter macros
	macros.emplace("StateCount", std::to_string(machine->getNFA().states.size()));
	macros.emplace("TransitionCount", "256");

	//  - generate action context declarations
	macros.emplace("ActionContextsDeclarations", generateAutomatonContextDeclarations(machine->getNFA()));

	// generate this bulk by traversing the NFA only once
	std::string stateMap;
	std::string actionRegisterDeclarations;
	std::string actionRegisterDefinitions;
	std::string transitionActionMap;
	std::string stateActionMap;
	generateAutomatonMechanicsMaps(machine->name, machine->getNFA(), stateMap, actionRegisterDeclarations, actionRegisterDefinitions, transitionActionMap, stateActionMap);

	//  - generate state map
	macros.emplace("StateMapEnumerated", stateMap);

	//  - generate action decs&defs&maps
	macros.emplace("ActionDeclarations", actionRegisterDeclarations);
	macros.emplace("ActionDefinitions", actionRegisterDefinitions);
	macros.emplace("TransitionActionMapEnumerated", transitionActionMap);
	macros.emplace("StateActionMapEnumerated", stateActionMap);

	//  - generate state finality
	macros.emplace("StateFinalityEnumerated", generateStateFinality(machine->getNFA()));

	std::ofstream faHeader(m_folderPath / (machine->name + ".h"));
	std::ofstream faCode(m_folderPath / (machine->name + ".cpp"));

	GenerationHelper::macroWrite(specimenFaHeaderContents, macros, faHeader);
	GenerationHelper::macroWrite(specimenFaCodeContents, macros, faCode);
}

void CppGenerationVisitor::visit(const FlagField* flagField) {
	m_output << "bool " << flagField->name << ";" << std::endl;
}

void CppGenerationVisitor::visit(const RawField* rawField) {
	m_output << "std::string " << rawField->name << ";" << std::endl;
}

void CppGenerationVisitor::visit(const ItemField* itemField) {
	m_output << "std::shared_ptr<" << itemField->type << "> " << itemField->name << ";" << std::endl;
}

void CppGenerationVisitor::visit(const ListField* listField) {
	m_output << "std::list<std::shared_ptr<" << listField->type << ">> " << listField->name << ";" << std::endl;
}

void CppGenerationVisitor::visit(const NFAActionRegister* actionRegister) {
	for (const NFAAction& action : *actionRegister) {
		m_output << '\t';
		action.accept(this);
		m_output << std::endl;
	}
}

void CppGenerationVisitor::visit(const NFAAction* action) {
	switch (action->type) {
		case NFAActionType::Flag:
			m_output << action->contextPath << "->" << action->targetName << " = true;";
			break;
		case NFAActionType::Unflag:
			m_output << action->contextPath << "->" << action->targetName << " = false;";
			break;

		case NFAActionType::Capture:
			// TODO: implement
			break;
		case NFAActionType::Empty:
			m_output << action->contextPath << "->" << action->targetName << ".clear();";
			break;
		case NFAActionType::Append:
			m_output << action->contextPath << "->" << action->targetName << ".append(1, c);";
			break;
		case NFAActionType::Prepend:
			m_output << action->contextPath << "->" << action->targetName << ".insert(0, 1, c);";
			break;
		
		case NFAActionType::CreateContext:
			m_output << action->contextPath << "__" << action->targetName << " = std::make_shared<" << action->targetName << ">(stream.currentLocation());";
			break;
		case NFAActionType::TerminalizeContext:
			m_output << action->contextPath << "__" << action->targetName << ".raw = stream.rawSincePin();";
			break;
		case NFAActionType::ElevateContext:
			m_output << action->contextPath << " = " << action->contextPath << "__" << action->targetName << ';';
			break;
		case NFAActionType::Set:
			m_output << action->contextPath << "->" << action->targetName << " = " << action->payload << ';';
			break;
		case NFAActionType::Unset:
			m_output << action->contextPath << "->" << action->targetName << " = nullptr;";
			break;
		case NFAActionType::Push:
			m_output << action->contextPath << "->" << action->targetName << ".push_back(" << action->payload << ");";
			break;
		case NFAActionType::Pop:
			m_output << action->contextPath << "->" << action->targetName << ".pop_back();";
			break;
		case NFAActionType::Clear:
			m_output << action->contextPath << "->" << action->targetName << ".clear();";
			break;
	}
}

std::string CppGenerationVisitor::generateTypeDeclarations(const std::list<const MachineComponent*>& components) {
	std::stringstream ss;

	for (auto mc : components) {
		ss << "class " << mc->name << " : public Production";
		if (mc->isTerminal()) {
			ss << ", public Terminal";
		}
		for (auto catPtr : mc->categories) {
			ss << ", public " << catPtr->name;
		}
		ss <<"{" << std::endl;

		ss << "public:" << std::endl;
		ss << '\t' << mc->name << "(const std::shared_ptr<RawStreamLocation>& location)" << std::endl;
		ss << "\t\t: Production(location)";
		if (mc->isTerminal()) {
			ss << ", Terminal(TerminalType::" << mc->name << ")";
		}
		ss << " { }" << std::endl;
		ss << '\t' << std::endl;
		for (auto fieldPtr : mc->fields) {
			ss << '\t';
			fieldPtr->accept(this);
			ss << outputAndReset();
		}
		ss << "};" << std::endl;
	}

	return ss.str();
}

void CppGenerationVisitor::generateAutomatonMechanicsMaps(const std::string& machineName, const NFA& fa, std::string& stateMap, std::string& actionRegisterDeclarations, std::string& actionRegisterDefinitions, std::string& transitionActionMap, std::string& stateActionMap) {
	std::stringstream stateMapStream;
	std::stringstream actionRegisterDeclarationStream;
	std::stringstream actionRegisterDefinitionStream;
	std::stringstream transitionActionMapStream;

	// the entire state action-register map, 0 (to be nullptr) by default
	std::vector<ActionRegisterId> stateActionRegisterMap(fa.states.size(), (ActionRegisterId)0);

	for (State state = 0; state < fa.states.size(); ++state) {
		const auto& stateObject = fa.states[state];

		// a single line of the state transition map, (State)-1 by default
		std::vector<std::vector<State>> transitionStateMapLine(256, std::vector<State>());

		// a single line of transition action-register map, 0 (to be nullptr) by default
		std::vector<std::vector<ActionRegisterId>> transitionActionRegisterMapLine(256, std::vector<ActionRegisterId>());
		
		// handle the action register of state actions
		const NFAActionRegister& snar = stateObject.actions;
		if (snar.size() > 0) {
			ActionRegisterId registerId = ++m_actionRegistersUsed;
			actionRegisterDeclarationStream << "void actionRegister" << registerId << "(const RawStream& stream);" << std::endl;
			actionRegisterDefinitionStream << generateActionRegisterDefinition(machineName, registerId, snar, true);

			stateActionRegisterMap[state] = registerId;
		}

		for (const auto& transition : stateObject.transitions) {
			auto symbolPtr = std::dynamic_pointer_cast<LiteralSymbolGroup>(transition.condition);
			for (auto it = transitionStateMapLine.begin() + symbolPtr->rangeStart; it <= transitionStateMapLine.begin() + symbolPtr->rangeEnd; ++it) {
				it->push_back(transition.target);
			}
			
			// handle the action register of transition actions
			const NFAActionRegister& tnar = transition.condition->actions;
			if (tnar.size() > 0) {
				ActionRegisterId registerId = ++m_actionRegistersUsed;
				actionRegisterDeclarationStream << "void actionRegister" << registerId << "(char c);" << std::endl;
				actionRegisterDefinitionStream << generateActionRegisterDefinition(machineName, registerId, tnar, false);

				for (auto it = transitionActionRegisterMapLine.begin() + symbolPtr->rangeStart; it <= transitionActionRegisterMapLine.begin() + symbolPtr->rangeEnd; ++it) {
					it->push_back(registerId);
				}
			}
		}

		stateMapStream << "{ ";
		for (const std::vector<State>& transitionStateMapEntryVector : transitionStateMapLine) {
			stateMapStream << "{ ";
			for (const State transitionStateMapEntry : transitionStateMapEntryVector) {
				stateMapStream << transitionStateMapEntry << ", ";
			}
			stateMapStream << "}, ";
		}
		stateMapStream << "},\n";

		transitionActionMapStream << "{ ";
		for (const std::vector<ActionRegisterId>& actionMapEntryVector : transitionActionRegisterMapLine) {
			transitionActionMapStream << "{ ";
			for (const ActionRegisterId actionMapEntry : actionMapEntryVector) {
				transitionActionMapStream << "&actionRegister" << actionMapEntry << ", ";
			}
			transitionActionMapStream << "}, ";
		}
		transitionActionMapStream << "},\n";
	}

	std::stringstream stateActionMapStream;
	for (State state = 0; state < fa.states.size(); ++state) {
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

std::string CppGenerationVisitor::generateAutomatonContextDeclarations(const NFA& fa) const {
	std::stringstream ss;

	for (const auto& contextParentChildPair : fa.contexts) {
		ss << "std::shared_ptr<" << contextParentChildPair.second << "> " << contextParentChildPair.first << "__"  << contextParentChildPair.second << ';' << std::endl;
	}
	
	return ss.str();
}

std::string CppGenerationVisitor::generateStateFinality(const NFA& fa) const {
	std::stringstream ss;

	for (State state = 0; state < fa.states.size(); ++state) {
		if (fa.finalStates.contains(state)) {
			ss << "true, ";
		} else {
			ss << "false, ";
		}
	}

	return ss.str();
}

std::string CppGenerationVisitor::generateActionRegisterDefinition(const std::string& machineName, ActionRegisterId registerId, const NFAActionRegister& nar, bool isStateAction) {
	std::stringstream ss;
	ss << "void " << machineName << "::" << "actionRegister" << registerId << (isStateAction ? "(const RawStream& stream)" : "(char c)") << " {" << std::endl;

	nar.accept(this);
	ss << outputAndReset();

	ss << "}" << std::endl << std::endl;

	return ss.str();
}

void CppGenerationVisitor::resetOutput() {
	std::stringstream().swap(m_output);
}

std::string CppGenerationVisitor::outputAndReset() {
	std::string ret = m_output.str();
	resetOutput();
	return ret;
}
