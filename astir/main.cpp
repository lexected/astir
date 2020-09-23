#include <iostream>
#include <fstream>
#include <map>
#include <string>

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

	try {
		LexicalAnalyzer lexicalAnalyzer;
		std::cout << "Tokenizing grammar file" << std::endl;
		auto tokenList = lexicalAnalyzer.process(grammarFile);

		SyntacticAnalyzer syntacticAnalyzer;
		std::cout << "Parsing grammar file" << std::endl;
		std::shared_ptr<SyntacticTree> syntacticTree = syntacticAnalyzer.process(tokenList);
		
		std::cout << "Semantically processing the grammar" << std::endl;
		syntacticTree->initialize();

		CppGenerationVisitor generationVisitor(*outputDirectoryPath);
		generationVisitor.setup();
		std::cout << "Generating output code" << std::endl;
		generationVisitor.visit(syntacticTree.get());
	} catch (const Exception& exception) {
		std::cerr << "Error: " << exception.what() << std::endl;
	} 

#else
	std::map<std::string, std::string> testsToRun = {
		{ "Test01", "Test01" },
		{ "Test02", "Test02" },
		{ "Test03", "Test03" },
		{ "Test04", "Test04" },
		{ "Test05", "Test05" },
		{ "Test06", "Test06" },
		{ "Test07", "Test07" },
		{ "Test08", "Test08" },
		{ "Test09", "Test09" },
		{ "Test10", "Test10" },/*
		{ "Test11", "Test11" },
		{ "Test12", "Test12" },
		{ "Test13", "Test13" },*/
		//{ "Hello Binary", "BinaryRecognizer" },
	};
	for (const auto& folderFilePair : testsToRun) {
		std::fstream inputFile("Tests/" + folderFilePair.first + "/" + folderFilePair.second + ".astir");

		LexicalAnalyzer analyzer;
		auto tokenList = analyzer.process(inputFile);
		// printTokenList(tokenList);

		SyntacticAnalyzer parser;
		std::shared_ptr<SyntacticTree> syntacticTree = parser.process(tokenList);
		syntacticTree->initialize();

		CppGenerationVisitor generationVisitor("Tests/" + folderFilePair.first + "/Output");
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