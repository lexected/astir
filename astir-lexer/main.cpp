#include <iostream>
#include <fstream>

#include "LexicalAnalyzer.h"

int main(int argc, char* argv[]) {
	std::fstream inputFile("ExampleFile.astir");

	LexicalAnalyzer analyzer;
	auto tokenList = analyzer.process(inputFile);

	for (const Token& token : tokenList) {
		std::cout << "[" << token.line << ":" << token.column << "] "
			<< token.typeString()
			<< ": \"" << token.string << "\""
			<< std::endl
			;
	}
}