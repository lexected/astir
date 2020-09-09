#include <iostream>
#include <fstream>

#include "LexicalAnalyzer.h"
#include "SyntacticAnalyzer.h"
#include "CppGenerationVisitor.h"

#include "dimcli/libs/dimcli/cli.h"

void printTokenList(const std::list<Token>& tokenList);

#include "TestingSwitch.h"
int main(int argc, char* argv[]) {
#ifndef TESTING
	Dim::Cli cli;
	auto& grammarFilePath = cli.opt<std::string>("<grammarFilePath>").desc("The path to the containing the grammar specification that is to be processed");
	auto& outputDirectoryPath = cli.opt<std::string>("outputDirectory", ".").desc("The directory where the generated files are meant to go.");
	if (!cli.parse(std::cerr, argc, argv))
		return cli.exitCode();

	std::fstream grammarFile(*grammarFilePath);

	LexicalAnalyzer lexicalAnalyzer;
	auto tokenList = lexicalAnalyzer.process(grammarFile);

	SyntacticAnalyzer syntacticAnalyzer;
	std::shared_ptr<SyntacticTree> syntacticTree = syntacticAnalyzer.process(tokenList);
	syntacticTree->initialize();

	CppGenerationVisitor generationVisitor(*outputDirectoryPath);
	generationVisitor.setup();
	generationVisitor.visit(syntacticTree.get());

#else
	std::vector<std::string> testsToRun = {
		"Test01",
		"Test02",
		"Test03",
		"Test04",
		"Test05",
		"Test06",
		"Test07",
	};
	for (const std::string& testName : testsToRun) {
		std::fstream inputFile("Tests/" + testName + "/" + testName + ".alex");

		LexicalAnalyzer analyzer;
		auto tokenList = analyzer.process(inputFile);

		SyntacticAnalyzer parser;
		std::shared_ptr<SyntacticTree> syntacticTree = parser.process(tokenList);
		syntacticTree->initialize();

		CppGenerationVisitor generationVisitor("Tests/" + testName + "/Output");
		generationVisitor.setup();
		generationVisitor.visit(syntacticTree.get());
	}
#endif

	return 0;
}

void printTokenList(const std::list<Token>& tokenList) {
	for (const Token& token : tokenList) {
		std::cout << "[" << token.locationString() << "] "
			<< token.typeString()
			<< ": \"" << token.string << "\""
			<< std::endl
			;
	}
}