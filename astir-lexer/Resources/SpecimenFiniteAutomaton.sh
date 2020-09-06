#pragma once

#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <stack>
#include <vector>
#include <functional>

#include "${{AppropriateStreamHeader}}"
${{DependencyHeaderInclude}}
#include "Terminal.h"

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
	typedef ${{OutputType}} OutputProduction;

	class EOS : public OutputTerminal {
	public:
		EOS(const std::shared_ptr<Location>& location)
			: OutputTerminal(OutputTerminalType::EOS, location) { }
	};

	${{TypeDeclarations}}
	using State = size_t;

	typedef ${{InputStreamTypeName}} InputStream;
	typedef ${{InputTerminalTypeName}} InputTerminal;
	typedef std::shared_ptr<${{InputTerminalTypeName}}> InputTerminalPtr;

	class ${{MachineName}};
	typedef void (${{MachineName}}::* ActionMethodPointer)(size_t, const std::deque<InputTerminalPtr>&, const std::shared_ptr<Location>&);
	class ${{MachineName}} {
	public:
		${{MachineName}}()
			: m_currentState(0) { }

		std::shared_ptr<OutputProduction> apply(InputStream& rs);
		std::list<std::shared_ptr<OutputProduction>> process(InputStream& rs);

		bool lastApplicationSuccessful() const { return m_stateFinality[m_currentState]; }
		void reset();

	private:
		// state-switching internals
		State m_currentState;
		static std::vector<State> m_stateMap[${{StateCount}}][${{TransitionSymbolCount}}];
		static bool m_stateFinality[${{StateCount}}];
		static std::vector<void (${{MachineName}}::*)(size_t, const std::deque<InputTerminalPtr>&, const std::shared_ptr<Location>&)> m_transitionActions[${{StateCount}}][${{TransitionSymbolCount}}];
		static void (${{MachineName}}::* m_stateActions[${{StateCount}}])(size_t, const std::deque<InputTerminalPtr>&, const std::shared_ptr<Location>&);

		// raw-capture internals
		std::stack<size_t> m_captureStack;

		// action contexts
		std::shared_ptr<OutputProduction> m_token;
		${{ActionContextsDeclarations}}
		// actions
		${{ActionDeclarations}}
	};
};
