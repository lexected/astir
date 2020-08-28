#include <iostream>
#include <string>
#include <list>
#include <memory>

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
		std::string string;
	protected:
		Terminal(TerminalType type)
			: type(type), string("") { }
		Terminal(TerminalType type, const std::string& str)
			: type(type), string(str) { }
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
		static State m_stateMap[${{StateCount}}][${{TransitionCount}}];
		static bool m_stateFinality[${{StateCount}}];
		static void (${{MachineName}}::* m_transitionActions[${{StateCount}}][${{TransitionCount}}])(char c);
		static void (${{MachineName}}::* m_stateActions[${{StateCount}}])(const RawStream& stream);

		// action contexts
		std::shared_ptr<Terminal> m_token;
		${{ActionContextsDeclarations}}
		// actions
		${{ActionDeclarations}}
	};
};
