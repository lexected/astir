#include <iostream>
#include <fstream>

#include "LexicalAnalyzer.h"
#include "SyntacticAnalyzer.h"
#include "CppGenerationVisitor.h"

int main(int argc, char* argv[]) {
	std::fstream inputFile("Tests/Test06/Test06.alex");

	LexicalAnalyzer analyzer;
	auto tokenList = analyzer.process(inputFile);

	for (const Token& token : tokenList) {
		std::cout << "[" << token.locationString() << "] "
			<< token.typeString()
			<< ": \"" << token.string << "\""
			<< std::endl
			;
	}

	SyntacticAnalyzer parser;
	std::shared_ptr<SyntacticTree> syntacticTree = parser.parse(tokenList);
	syntacticTree->initialize();

	CppGenerationVisitor generationVisitor("Tests/Test06/Output");
	generationVisitor.setup();
	generationVisitor.visit(syntacticTree.get());

	return 0;
}