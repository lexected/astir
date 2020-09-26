cmake_minimum_required (VERSION 3.15)

project ("Astir" VERSION 1.0.0 LANGUAGES CXX)

# C++ standard
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_STANDARD 17)

# To Check if we need filesystem
include(CheckCXXSymbolExists)
CHECK_CXX_SYMBOL_EXISTS(std::filesystem::path::preferred_separator filesystem cxx17fs)

if(NOT cxx17fs)
    target_link_libraries("Astir" PRIVATE stdc++fs)
endif()

# Add source to this project's executable.
add_executable ("astir"
    "CharType.h"
	"CppGenerationVisitor.cpp"
	"CppGenerationVisitor.h"
	"CppLLkParserGenerator.cpp"
	"CppLLkParserGenerator.h"
	"CppNFAGenerationHelper.cpp"
	"CppNFAGenerationHelper.h"
	"DimCli/libs/dimcli/cli.h"
	"DimCli/libs/dimcli/cli.cpp"
    "Exception.h"
	"Field.cpp"
	"Field.h"
    "FileLocation.h"
	"FiniteAutomatonDefinition.cpp"
	"FiniteAutomatonDefinition.h"
    "GenerationException.cpp"
	"GenerationException.h"
    "GenerationHelper.cpp"
	"GenerationHelper.h"
	"GenerationVisitor.h"
    "IActing.h"
	"IFileLocalizable.h"
	"ILLkBuilding.h"
	"ILLkFirstable.h"
	"ILLkParserGenerable.h"
	"IndentedStringStream.cpp"
	"IndentedStringStream.h"
    "INFABuildable.h"
	"IReferencing.cpp"
	"IReferencing.h"
	"ISemanticEntity.h"
	"ISyntacticEntity.cpp"
	"ISyntacticEntity.h"
    "LexicalAnalysisException.h"
    "LexicalAnalyzer.cpp"
    "LexicalAnalyzer.h"
    "LLkBuilder.cpp"
    "LLkBuilder.h"
    "LLkFirster.cpp"
    "LLkFirster.h"
    "LLkParserDefinition.cpp"
    "LLkParserDefinition.h"
    "LLkParserGenerator.h"
    "MachineDefinition.cpp"
    "MachineDefinition.h"
    "MachineStatement.cpp"
    "MachineStatement.h"
    "main.cpp"
    "NFA.cpp"
    "NFA.h"
    "NFAAction.cpp"
    "NFAAction.h"
    "NFABuilder.cpp"
    "NFABuilder.h"
    "Regex.cpp"
    "Regex.h"
    "RegexAction.h"
    "SemanticAnalysisException.cpp"
    "SemanticAnalysisException.h"
    "SymbolGroup.cpp"
    "SymbolGroup.h"
    "SyntacticAnalysisException.h"
    "SyntacticAnalyzer.cpp"
    "SyntacticAnalyzer.h"
    "SyntacticTree.cpp"
    "SyntacticTree.h"
    "TestingSwitch.h"
    "Token.cpp"
    "Token.h"
)
