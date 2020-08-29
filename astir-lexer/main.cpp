#include <iostream>
#include <fstream>

#include "LexicalAnalyzer.h"
#include "Parser.h"
#include "CppGenerationVisitor.h"

int main(int argc, char* argv[]) {
	std::fstream inputFile("Tests/Test03/Test03.alex");

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

	CppGenerationVisitor generationVisitor("Tests/Test03/Output");
	generationVisitor.setup();
	generationVisitor.visit(semanticTree.get());

	return 0;
}