#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <stack>
#include <vector>
#include <functional>

#include "${{ApplicableStreamHeader}}"
#include "Terminal.h"

namespace ${{MachineName}} {
	enum class ${{MachineName}}TerminalType {
		${{TerminalTypesEnumerated}}
		EOS = 0
	};

	typedef ${{MachineName}}TerminalType RelevantTerminalType;

	class ${{MachineName}}Terminal : public Terminal<RelevantTerminalType> {
	protected:
		Terminal(RelevantTerminalType type, const std::shared_ptr<Location>& occurenceLocation)
			: Terminal(type, occurenceLocation) { }
		Terminal(TerminalType type, const std::string& str, const std::shared_ptr<Location>& occurenceLocation)
			: Terminal(type, str, occurenceLocation) { }
	};

	typedef ${{MachineName}}Terminal RelevantTerminal;

	class EOS : public RelevantTerminal {
	public:
		EOS(const std::shared_ptr<Location>& location)
			: RelevantTerminal(RelevantTerminalType::EOS) { }
	};

	${{TypeDeclarations}}
	using State = size_t;

	typedef ${{RelevantStreamTypeName}} RelevantStream;

	class ${{MachineName}};
	typedef void (${{MachineName}}::* ActionMethodPointer)(size_t, const std::string&, const std::shared_ptr<Location>&);
	class ${{MachineName}} {
	public:
		${{MachineName}}()
			: m_currentState(0) { }

		std::shared_ptr<Terminal> apply(RelevantStream& rs);
		std::list<std::shared_ptr<Terminal>> process(RelevantStream& rs);

		bool lastApplicationSuccessful() const { return m_stateFinality[m_currentState]; }
		void reset();

	private:
		// state-switching internals
		State m_currentState;
		static std::vector<State> m_stateMap[${{StateCount}}][${{TransitionCount}}];
		static bool m_stateFinality[${{StateCount}}];
		static std::vector<void (${{MachineName}}::*)(size_t, const std::string&, const std::shared_ptr<Location>&)> m_transitionActions[${{StateCount}}][${{TransitionCount}}];
		static void (${{MachineName}}::* m_stateActions[${{StateCount}}])(size_t, const std::string&, const std::shared_ptr<Location>&);

		// raw-capture internals
		std::stack<size_t> m_captureStack;

		// action contexts
		std::shared_ptr<Terminal> m_token;
		${{ActionContextsDeclarations}}
		// actions
		${{ActionDeclarations}}
	};
};
