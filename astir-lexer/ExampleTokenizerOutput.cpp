#include <iostream>
#include <string>
#include <list>

namespace ExampleTokenizer {
	using CategoryBitArray = unsigned long;

	enum class TerminalType {
		EOS,

		IDENTIFIER,
		EQUALS,
		NUMBER,
		STRING,

		OP_ADD,
		OP_SUB
	};

	class Terminal {
	public:
		TerminalType type;
		std::string string;
	protected:
		Terminal(TerminalType type)
			: type(type), string("") { }
	};

	class EOS : public Terminal {
	public:
		EOS()
			: Terminal(TerminalType::EOS) { }
	};

	class Number {
	public:
		bool isNegative;
	};

	class Integer : public Number {
	public:
		std::string value;
	};

	class PositiveInteger : public Terminal, public Integer { };
	class NegativeInteger : public Terminal, public Integer { };

	class Float : public Terminal, public Number {
	public:
		std::shared_ptr<PositiveInteger> mantissaWholes;
		std::shared_ptr<PositiveInteger> mantissaDecimals;
		std::shared_ptr<PositiveInteger> exponent;
	};

	class Identifier {
	public:
		std::string name;
	};

	class Operator {
	public:
	};

	class ExampleTokenizer {
	public:
		std::shared_ptr<Terminal> apply(std::istream& is);
		std::list<std::shared_ptr<Terminal>> processStream(std::istream& is);
	private:
		
	};
};

int fakemain() {
	
	return 0;
}

