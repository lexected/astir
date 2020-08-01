#include <iostream>
#include <fstream>

#include "LexicalAnalyzer.h"

int main(int argc, char* argv[]) {
	std::fstream inputFile("ExampleFile.alex");

	LexicalAnalyzer analyzer;
	auto tokenList = analyzer.process(inputFile);

	for (const Token& token : tokenList) {
		std::cout << "[" << token.line << ":" << token.column << "] "
			<< LexicalAnalyzer::tokenTypeToString(token.type)
			<< ": \"" << token.string << "\""
			<< std::endl
			;
	}
}