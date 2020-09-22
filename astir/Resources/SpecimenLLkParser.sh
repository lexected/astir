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
#include "Machine.h"

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

	class Exception : public std::exception {
	public:
		Exception(const std::string& message)
			: std::exception(message.c_str()) {}
		virtual ~Exception() = default;
	};

	class ${{MachineName}} : public Machine<InputStream, OutputProduction> {
	public:
		${{MachineName}}()
			: m_lastApplicationSuccessful(false), m_lastException(nullptr) { }

		std::shared_ptr<OutputProduction> apply(InputStream& rs) override;

		std::shared_ptr<OutputProduction> parse(InputStream& rs);
		std::shared_ptr<OutputProduction> parseWithIgnorance(InputStream& rs);
		std::list<std::shared_ptr<OutputProduction>> parseStream(InputStream& rs);
		std::list<std::shared_ptr<OutputProduction>> parseStreamWithIgnorance(InputStream& rs);

		bool lastApplicationSuccessful() const override { return m_lastApplicationSuccessful; }
		void reset() override;
		std::string lastError() const;
		
	private:
		bool m_lastApplicationSuccessful;
		std::unique_ptr<Exception> m_lastException;
		void error(const std::string& message) const;

		// helper methods
		${{CombineRawDeclaration}}
		
		// dependency machines
		${{DependencyMachineFields}}
		// parsing declarations
		${{ParsingDeclarations}}
	};
};
