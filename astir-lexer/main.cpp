#include <iostream>
#include <fstream>

#include "LexicalAnalyzer.h"
#include "Parser.h"

int main(int argc, char* argv[]) {
	std::fstream inputFile("TestTokenizer01.alex");

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
	std::shared_ptr<ISemanticallyProcessable<SemanticTree>> syntacticTree = parser.parse(tokenList);
	auto semanticTree = syntacticTree->makeSemanticEntity(syntacticTree);
	semanticTree->initialize();

	return 0;
}