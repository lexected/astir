#include <iostream>
#include <fstream>

#include "LexicalAnalyzer.h"
#include "Parser.h"

int main(int argc, char* argv[]) {
	std::fstream inputFile("ExampleTokenizer.alex");

	LexicalAnalyzer analyzer;
	auto tokenList = analyzer.process(inputFile);

	for (const Token& token : tokenList) {
		std::cout << "[" << token.locationString() << "] "
			<< token.typeString()
			<< ": \"" << token.string << "\""
			<< std::endl
			;
	}

	Parser parser;
	auto specification = parser.parse(tokenList);

	return 0;
}