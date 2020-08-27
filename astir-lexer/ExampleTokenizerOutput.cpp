#include <iostream>
#include <string>
#include <list>

#include "FileLocation.h"
#include "IFileLocalizable.h"

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

	class Terminal : public IFileLocalizable {
	public:
		TerminalType type;
		std::string string;
	protected:
		Terminal(TerminalType type, const FileLocation& occurenceLocation)
			: type(type), string(""), m_location(occurenceLocation) { }
		Terminal(TerminalType type, const IFileLocalizable& underlyingEntity)
			: type(type), string(""), m_location(underlyingEntity.location()) { }

	private:
		FileLocation m_location;
	};

	class EOS : public Terminal {
	public:
		EOS(const FileLocation& location)
			: Terminal(TerminalType::EOS, location) { }
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
		std::list<std::shared_ptr<Terminal>> process(std::istream& is);
	private:
		
	};
};

int fakemain() {
	
	return 0;
}

