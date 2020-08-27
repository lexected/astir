#include <iostream>
#include <string>
#include <list>
#include <memory>

#include "RawStream.h"

namespace ${{MachineName}} {
	enum class TerminalType {
		${{TerminalTypesEnumerated}}
	};

	class Terminal : public IRawStreamLocalizable {
	public:
		TerminalType type;
		std::string string;
	protected:
		Terminal(TerminalType type, const std::shared_ptr<RawStreamLocation>& occurenceLocation)
			: type(type), string(""), m_location(occurenceLocation) { }
		Terminal(TerminalType type, const IRawStreamLocalizable& underlyingEntity)
			: type(type), string(""), m_location(underlyingEntity.location()) { }

		const std::shared_ptr<RawStreamLocation>& location() const override;
	private:
		std::shared_ptr<RawStreamLocation> m_location;
	};

	const std::shared_ptr<RawStreamLocation>& Terminal::location() const {
		return m_location;
	}

	class EOS : public Terminal {
	public:
		EOS(const std::shared_ptr<RawStreamLocation>& location)
			: Terminal(TerminalType::EOS, location) { }
	};

	${{TypeDeclarations}}

	using State = size_t;
	class ${{MachineName}} {
	public:
		${{MachineName}}()
			: m_currentState(0) { }

		std::shared_ptr<Terminal> apply(RawStream& rs);
		std::list<std::shared_ptr<Terminal>> process(RawStream& rs);

		bool lastApplicationSuccessful() const { return m_currentState == 0; }
		void reset();

	private:
		// state-switching internals
		State m_currentState;
		static State m_stateMap[${{StateCount}}][${{TransitionCount}}];
		static bool m_stateFinality[${{StateCount}}];
		static void (${{MachineName}}::* m_transitionActions[${{StateCount}}][${{TransitionCount}}])(char c);
		static void (${{MachineName}}::* m_stateActions[${{StateCount}}])();

		// action contexts
		std::shared_ptr<Terminal> m_token;
		${{ActionContextsDeclarations}}

		// transition actions
		${{TransitionActionsDeclarations}}

		// state actions
		${{StateActionsDeclarations}}
	};
};
