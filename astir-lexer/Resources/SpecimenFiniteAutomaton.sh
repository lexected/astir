#include <iostream>
#include <string>
#include <list>
#include <memory>
#include <vector>

#include "RawStream.h"

namespace ${{MachineName}} {
	class Production : IRawStreamLocalizable {
	public:
		Production(const std::shared_ptr<RawStreamLocation>& occurenceLocation)
			: m_location(occurenceLocation) { }
		Production(const IRawStreamLocalizable& underlyingEntity)
			: m_location(underlyingEntity.location()) { }

		const std::shared_ptr<RawStreamLocation>& location() const override;
	private:
		std::shared_ptr<RawStreamLocation> m_location;
	};

	enum class TerminalType {
		EOS,

		${{TerminalTypesEnumerated}}
	};

	class Terminal {
	public:
		TerminalType type;
		std::string raw;
	protected:
		Terminal(TerminalType type)
			: type(type), raw("") { }
		Terminal(TerminalType type, const std::string& str)
			: type(type), raw(str) { }
	};

	class EOS : public Production, public Terminal {
	public:
		EOS(const std::shared_ptr<RawStreamLocation>& location)
			: Production(location), Terminal(TerminalType::EOS) { }
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
		static std::vector<State> m_stateMap[${{StateCount}}][${{TransitionCount}}];
		static bool m_stateFinality[${{StateCount}}];
		static std::vector<void (${{MachineName}}::*)(char)> m_transitionActions[${{StateCount}}][${{TransitionCount}}];
		static void (${{MachineName}}::* m_stateActions[${{StateCount}}])(;

		// action contexts
		std::shared_ptr<Terminal> m_token;
		${{ActionContextsDeclarations}}
		// actions
		${{ActionDeclarations}}
	};
};
