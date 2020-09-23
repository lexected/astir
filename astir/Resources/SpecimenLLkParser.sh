#pragma once

#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <stack>
#include <vector>

// stream
#include "${{AppropriateStreamHeader}}"

// general dependencies
#include "Terminal.h"
#include "Parser.h"

// particular dependencies
${{DependencyHeaderIncludes}}

namespace ${{MachineName}} {
	enum class ${{MachineName}}TerminalType {
		${{OutputTerminalTypesEnumerated}}
		EOS = 0
	};

	typedef ${{MachineName}}TerminalType OutputTerminalType;

	class ${{MachineName}}Terminal : public Terminal<OutputTerminalType> {
	public:
		${{MachineName}}Terminal()
			: ${{MachineName}}Terminal(OutputTerminalType::EOS, nullptr) { }
			// something distinguishably invalid, should never be referred to in practice if not initialized in a valid fashion

	protected:
		${{MachineName}}Terminal(OutputTerminalType type, const std::shared_ptr<Location>& occurenceLocation)
			: Terminal(type, occurenceLocation) { }
		${{MachineName}}Terminal(OutputTerminalType type, const std::string& str, const std::shared_ptr<Location>& occurenceLocation)
			: Terminal(type, str, occurenceLocation) { }
	};

	typedef ${{MachineName}}Terminal OutputTerminal;
	typedef std::shared_ptr<OutputTerminal> OutputTerminalPtr;
	typedef ${{OutputType}} OutputProduction;
	typedef std::shared_ptr<OutputProduction> OutputProductionPtr;

	class EOS : public OutputTerminal {
	public:
		EOS(const std::shared_ptr<Location>& location)
			: OutputTerminal(OutputTerminalType::EOS, location) { }
	};

	${{TypeForwardDeclarations}}
	${{TypeDeclarations}}
	typedef ${{InputStreamTypeName}} InputStream;
	typedef ${{InputTypeName}} InputType;
	typedef std::shared_ptr<${{InputTypeName}}> InputTypePtr;

	class ${{MachineName}} : public Parser<InputStream, OutputProduction> {
	public:
		${{MachineName}}() = default;

	protected:
		// helper methods
		${{CombineRawDeclaration}}
		
		// dependency machines
		${{DependencyMachineFields}}
		// parsing declarations
		${{ParsingDeclarations}}
	};
};
