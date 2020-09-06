#include "CppGenerationVisitor.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "GenerationHelper.h"
#include "CppNFAGenerationHelper.h"

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

	const NFA& nfa = machine->getNFA();
	CppNFAGenerationHelper cngh(machine->name, nfa);

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
	for (const auto& machineComponentPair : machine->components) {
		machineComponentPair.second->accept(this);
	}
	macros.emplace("TypeDeclarations", this->outputAndReset());

	//  - set the DFA table parameter macros
	macros.emplace("StateCount", std::to_string(machine->getNFA().states.size()));
	macros.emplace("TransitionCount", "256");

	//  - generate action context declarations
	macros.emplace("ActionContextsDeclarations", cngh.generateContextDeclarations());

	// generate this bulk by traversing the NFA only once
	std::string stateMap;
	std::string actionRegisterDeclarations;
	std::string actionRegisterDefinitions;
	std::string transitionActionMap;
	std::string stateActionMap;
	cngh.generateMechanicsMaps(stateMap, actionRegisterDeclarations, actionRegisterDefinitions, transitionActionMap, stateActionMap);

	//  - generate state map
	macros.emplace("StateMapEnumerated", stateMap);

	//  - generate action decs&defs&maps
	macros.emplace("ActionDeclarations", actionRegisterDeclarations);
	macros.emplace("ActionDefinitions", actionRegisterDefinitions);
	macros.emplace("TransitionActionMapEnumerated", transitionActionMap);
	macros.emplace("StateActionMapEnumerated", stateActionMap);

	//  - generate state finality
	macros.emplace("StateFinalityEnumerated", cngh.generateStateFinality());

	std::ofstream faHeader(m_folderPath / (machine->name + ".h"));
	std::ofstream faCode(m_folderPath / (machine->name + ".cpp"));

	GenerationHelper::macroWrite(specimenFaHeaderContents, macros, faHeader);
	GenerationHelper::macroWrite(specimenFaCodeContents, macros, faCode);
}

void CppGenerationVisitor::visit(const MachineComponent* mc) {
	if (!mc->isTypeForming()) {
		return;
	}

	m_output << "class " << mc->name << " : public Production";
	if (mc->isTerminal()) {
		m_output << ", public Terminal";
	}
	for (auto catPtr : mc->categories) {
		m_output << ", public " << catPtr->name;
	}
	m_output << "{" << std::endl;

	m_output << "public:" << std::endl;
	m_output << '\t' << mc->name << "(const std::shared_ptr<RawStreamLocation>& location)" << std::endl;
	m_output << "\t\t: Production(location)";
	if (mc->isTerminal()) {
		m_output << ", Terminal(TerminalType::" << mc->name << ")";
	}
	m_output << " { }" << std::endl;
	m_output << '\t' << std::endl;
	for (auto fieldPtr : mc->fields) {
		m_output << '\t';
		fieldPtr->accept(this);
		// m_output << outputAndReset(); not needed actually since we are working on m_output atm
	}
	m_output << "};" << std::endl;
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

void CppGenerationVisitor::resetOutput() {
	std::stringstream().swap(m_output);
}

std::string CppGenerationVisitor::outputAndReset() {
	std::string ret = m_output.str();
	resetOutput();
	return ret;
}
