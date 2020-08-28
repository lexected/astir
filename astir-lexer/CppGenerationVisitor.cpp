#include "CppGenerationVisitor.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "GenerationHelper.h"

void CppGenerationVisitor::setup() const {
	if(!std::filesystem::is_directory("Output")) {
		std::filesystem::create_directory("Output");
	}
	std::filesystem::copy_file("Resources/RawStream.h", "Output/RawStream.h");
	std::filesystem::copy_file("Resources/RawStream.cpp", "Output/RawStream.cpp");
}

void CppGenerationVisitor::visit(const SemanticTree* tree) {
	for (const auto machinePair : tree->machines) {
		machinePair.second->accept(this);
	}
}

void CppGenerationVisitor::visit(const FiniteAutomatonMachine* machine) {
	std::ifstream specimenFaHeader("Resources/SpecimenFiniteAutomaton.sh");
	std::string specimenFaHeaderContents((std::istreambuf_iterator<char>(specimenFaHeader)), (std::istreambuf_iterator<char>()));

	std::ifstream specimenFaCode("Resources/SpecimenFiniteAutomaton.cpp");
	std::string specimenFaCodeContents((std::istreambuf_iterator<char>(specimenFaCode)), (std::istreambuf_iterator<char>()));

	// do constructive magic
	std::map<std::string, std::string> macros;
	// in particular, we need to
	//  - register all the name macros
	macros.emplace("${{MachineName}}", machine->name);
	
	//  - enumerate all the terminal types
	auto terminalTypeComponents = machine->getTerminalTypeComponents();
	std::stringstream ss;
	for (auto componentPtr : terminalTypeComponents) {
		ss << componentPtr->name << "," << std::endl;
	}
	macros.emplace("${{TerminalTypesEnumerated}}", ss.str());

	//  - generate type declarations
	auto typeComponents = machine->getTypeComponents();
	const std::string& generatedClasses = generateTypeDeclarations(typeComponents);
	macros.emplace("${{TypeDeclarations}}", generatedClasses);

	//  - set the DFA table parameter macros
	macros.emplace("${{StateCount}}", std::to_string(machine->getNFA().states.size()));
	macros.emplace("${{TransitionCount}}", "256");

	//  - generate action context declarations
	macros.emplace("${{ActionContextsDeclarations}}", "");

	//  - generate transition action decs&defs&adds
	macros.emplace("${{TransitionActionsDeclarations}}", "");
	macros.emplace("${{TransitionActionsDefinitions}}", "");
	macros.emplace("${{TransitionActionAddressesEnumerated}}", "");

	//  - generate state action decs&defs&adds
	macros.emplace("${{StateActionsDeclarations}}", "");
	macros.emplace("${{StateActionsDefinitions}}", "");
	macros.emplace("${{StateActionAddressesEnumerated}}", "");

	//  - generate state map
	macros.emplace("${{StateMapEnumerated}}", generateStateMap(machine->getNFA()));

	//  - generate state finality
	macros.emplace("${{StateFinalityEnumerated}}", generateStateFinality(machine->getNFA()));

	std::ofstream faHeader("Output/" + machine->name + ".h");
	std::ofstream faCode("Output/" + machine->name + ".cpp");

	GenerationHelper::macroWrite(specimenFaHeaderContents, macros, faHeader);
	GenerationHelper::macroWrite(specimenFaCodeContents, macros, faCode);
}

std::string CppGenerationVisitor::generateTypeDeclarations(const std::list<const MachineComponent*>& components) {
	std::stringstream ss;

	//TODO: add internal field generation

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
		ss << "\tEOS(const std::shared_ptr<RawStreamLocation>& location)" << std::endl;
		ss << "\t\t: Production(location)";
		if (mc->isTerminal()) {
			ss << ", Terminal(TerminalType::" << mc->name << ")";
		}
		ss << " { }" << std::endl;
		ss << "};" << std::endl;
	}

	return ss.str();
}

std::string CppGenerationVisitor::generateStateMap(const NFA& fa) {
	std::stringstream ss;

	for (State state = 0; state < fa.states.size(); ++state) {
		const auto& stateObject = fa.states[state];
		State transitionTableLine[256];
		std::fill(std::begin(transitionTableLine), std::end(transitionTableLine), (State)(-1));

		for (const auto& transition : stateObject.transitions) {
			auto symbolPtr = std::dynamic_pointer_cast<LiteralSymbolGroup>(transition.condition);
			std::fill(transitionTableLine+symbolPtr->rangeStart, transitionTableLine + symbolPtr->rangeEnd, transition.target);
		}

		ss << "{ ";
		for (const State& transitionTableEntry : transitionTableLine) {
			ss << transitionTableEntry << ", ";
		}
		ss << "},\n";
	}

	return ss.str();
}

std::string CppGenerationVisitor::generateStateFinality(const NFA& fa) {
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
