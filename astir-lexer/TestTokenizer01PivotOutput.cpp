#include <iostream>
#include <string>
#include <list>
#include <memory>

#include "RawStream.h"
	
namespace TestTokenizer01 {
	enum class TerminalType {
		EOS,

		p1,
		p2,
		p3
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

	class p1 : public Terminal {
	public:
		p1(const std::shared_ptr<RawStreamLocation>& location)
			: Terminal(TerminalType::p1, location) { }
	};

	class p2 : public Terminal {
	public:
		p2(const std::shared_ptr<RawStreamLocation>& location)
			: Terminal(TerminalType::p2, location) { }
	};

	class p3 : public Terminal {
	public:
		p3(const std::shared_ptr<RawStreamLocation>& location)
			: Terminal(TerminalType::p3, location) { }
	};

	using State = size_t;
	class TestTokenizer01 {
	public:
		TestTokenizer01()
			: m_currentState(0) { }

		std::shared_ptr<Terminal> apply(RawStream& rs);
		std::list<std::shared_ptr<Terminal>> process(RawStream& rs);

		bool lastApplicationSuccessful() const { return m_currentState == 0; }
	private:
		// state-switching internals
		State m_currentState;
		static State m_stateMap[6][256];
		static bool m_stateFinality[6];

		// contexts
		std::shared_ptr<Terminal> m_token;
	};

	std::shared_ptr<Terminal> TestTokenizer01::apply(RawStream& rs) {
		char currentCharacter;
		State lastAcceptingState = (State)(-1);

		rs.pin();
		while (true) {
			State stateToGoTo;
			if (!rs.get(currentCharacter)) {
				stateToGoTo = (State)(-1);
			} else {
				stateToGoTo = m_stateMap[m_currentState][currentCharacter];
			}
			
			if (stateToGoTo == (State)(-1)) {
				if (lastAcceptingState == (State)(-1)) {
					if(m_currentState > 0) {
						// failure, we are not in the first state and we have not reached any accepting state so far, so we will return nullptr. The users are then encouraged to make use of lastApplicationSuccessful() to identify the issue
						return nullptr;
					} else {
						// all done, lastApplicationSuccessful!
						return nullptr;
					}
				} else {
					// accept, reset m_currentState to 0, and backtrack input to where the lastAcceptingState was hit
					rs.resetToPin();
					// then return m_token;
				}
			} else {
				if (m_stateFinality[stateToGoTo]) {
					lastAcceptingState = stateToGoTo;
					rs.pin();
				}
				m_currentState = stateToGoTo;
			}
		}

		return std::shared_ptr<Terminal>();
	}

	std::list<std::shared_ptr<Terminal>> TestTokenizer01::process(RawStream& rs) {
		std::list<std::shared_ptr<Terminal>> ret;

		while (lastApplicationSuccessful() /* && the end of */) {
			auto terminalPtr = apply(rs);
			if(terminalPtr) {
				ret.push_back(terminalPtr);
			}
		}

		if (lastApplicationSuccessful()) {
			auto eosLocationPtr = rs.pinLocation()->clone();
			eosLocationPtr->advance();
			ret.push_back(std::make_shared<EOS>(eosLocationPtr));
		}

		return ret;
	}
	
	State TestTokenizer01::m_stateMap[6][256];
	bool TestTokenizer01::m_stateFinality[6] = { false, false, false, false, false, false };
};

int fakeemain() {

	return 0;
}
