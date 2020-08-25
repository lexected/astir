#include <iostream>
#include <string>

namespace ExampleTokenizer {
	using CategoryBitArray = unsigned long;

	enum class TokenType {
		IDENTIFIER,
		EQUALS,
		NUMBER,
		STRING,

		OP_ADD,
		OP_SUB
	};

	class Token {
	public:
		TokenType type;
		std::string string;
		CategoryBitArray categories;
	protected:
		Token(TokenType type)
			: type(type), string(""), categories(0) { }
	};

	class NumberToken : public Token {
	public:
		unsigned long long value;

		NumberToken()
			: Token(TokenType::NUMBER), value(0) { }
	};

	class IdentifierToken : public Token {
		std::string identifiedName;

		IdentifierToken()
			: Token(TokenType::IDENTIFIER) { }
	};
};

int fakemain() {
	std::cout << "sizeof(Token)" << sizeof(ExampleTokenizer::Token) << std::endl;
	std::cout << "sizeof(NumberToken)" << sizeof(ExampleTokenizer::NumberToken) << std::endl;
	std::cout << "sizeof(IdentifierToken)" << sizeof(ExampleTokenizer::IdentifierToken) << std::endl;

	return 0;
}

