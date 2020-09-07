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
	std::filesystem::copy_file("Resources/Location.h", m_folderPath / "Location.h", std::filesystem::copy_options::overwrite_existing);
	std::filesystem::copy_file("Resources/Location.cpp", m_folderPath / "Location.cpp", std::filesystem::copy_options::overwrite_existing);
	std::filesystem::copy_file("Resources/Production.h", m_folderPath / "Production.h", std::filesystem::copy_options::overwrite_existing);
	std::filesystem::copy_file("Resources/Terminal.h", m_folderPath / "Terminal.h", std::filesystem::copy_options::overwrite_existing);
	std::filesystem::copy_file("Resources/ProductionStream.h", m_folderPath / "ProductionStream.h", std::filesystem::copy_options::overwrite_existing);
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

	// do constructive magic
	std::map<std::string, std::string> macros;
	// in particular, we need to
	//  - register all the name macros
	macros.emplace("MachineName", machine->name);
	//	- choose appropriate input stream type
	if (!machine->on) {
		if(!m_hasIncludedRawStreamFiles) {
			std::filesystem::copy_file("Resources/RawStream.h", m_folderPath / "RawStream.h", std::filesystem::copy_options::overwrite_existing);
			std::filesystem::copy_file("Resources/RawStream.cpp", m_folderPath / "RawStream.cpp", std::filesystem::copy_options::overwrite_existing);

			m_hasIncludedRawStreamFiles = true;
		}
		macros.emplace("AppropriateStreamHeader", "RawStream.h");
		macros.emplace("InputTerminalTypeName", "RawTerminal");
		macros.emplace("InputStreamTypeName", "RawStream");
		macros.emplace("DependencyHeaderInclude", "");
	} else {
		macros.emplace("AppropriateStreamHeader", "ProductionStream.h");
		macros.emplace("InputTerminalTypeName", machine->on->name + "::OutputTerminal");
		macros.emplace("InputStreamTypeName", "ProductionStream<" + machine->on->name + "::OutputTerminal>");
		macros.emplace("DependencyHeaderInclude", "#include \"" + machine->on->name + ".h\"");
	}
	
	//  - enumerate all the production roots
	auto terminalRootProductions = machine->getTerminalRoots();
	std::stringstream ss;
	for (auto productionPtr : terminalRootProductions) {
		ss << productionPtr->name << " = " << productionPtr->typeIndex << "," << std::endl;
	}
	macros.emplace("OutputTerminalTypesEnumerated", ss.str());
	macros.emplace("OutputTerminalTypeCount", std::to_string(machine->terminalProductionCount()));

	//  - generate type declarations
	for (const auto& machineComponentPair : machine->components) {
		machineComponentPair.second->accept(this);
	}
	macros.emplace("TypeDeclarations", this->outputAndReset());

	//	- set the type of out the output to the highest resolution possible
	macros.emplace("OutputType", machine->hasPurelyTerminalRoots() ? "OutputTerminal" : "Production");

	//  - set the DFA table parameter macros
	macros.emplace("StateCount", std::to_string(machine->getNFA().states.size()));
	size_t numberOfInputTerminals;
	if (machine->on) {
		numberOfInputTerminals = machine->on->terminalProductionCount() + 1; //+1 comes from having EOS (TerminalTypeIndex = 0) there implicitly
	} else {
		numberOfInputTerminals = 256;
	}
	macros.emplace("TransitionSymbolCount", std::to_string(numberOfInputTerminals));

	CppNFAGenerationHelper cngh(machine->name, nfa, numberOfInputTerminals);
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

	m_output << "class " << mc->name << " : ";
	if (mc->isTerminal()) {
		m_output << "public OutputTerminal";
	} else {
		m_output << "public Production";
	}
	for (auto catPtr : mc->categories) {
		m_output << ", public " << catPtr->name;
	}
	m_output << " {" << std::endl;

	m_output << "public:" << std::endl;
	m_output << '\t' << mc->name << "(const std::shared_ptr<Location>& location)" << std::endl;
	m_output << "\t\t: ";
	if (mc->isTerminal()) {
		m_output << "OutputTerminal(OutputTerminalType::" << mc->name << ", location)";
	} else {
		m_output << "Production(location)";
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
	m_output << "std::shared_ptr<" << itemField->machineOfTheType->name << "::" << itemField->type << "> " << itemField->name << ";" << std::endl;
}

void CppGenerationVisitor::visit(const ListField* listField) {
	m_output << "std::list<std::shared_ptr<" << listField->machineOfTheType->name << "::" << listField->type << ">> " << listField->name << ";" << std::endl;
}

void CppGenerationVisitor::resetOutput() {
	std::stringstream().swap(m_output);
}

std::string CppGenerationVisitor::outputAndReset() {
	std::string ret = m_output.str();
	resetOutput();
	return ret;
}
