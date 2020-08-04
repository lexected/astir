#include "Parser.h"

using namespace std;

std::unique_ptr<Specification> Parser::parse(const std::list<Token>& tokens) const {
	auto it = tokens.begin();

	list<unique_ptr<MachineDefinition>> definitions;

	while (it->type != TokenType::EOS) {
		auto savedIt = it;
		auto machineDefinition = Parser::parseMachineDefinition(it);
		if (machineDefinition == nullptr) {
			it = savedIt;
			// throw
		}

		definitions.push_back(machineDefinition);
	}

	return std::unique_ptr<Specification>(new Specification(definitions));
}

std::unique_ptr<MachineDefinition> Parser::parseMachineDefinition(std::list<Token>::const_iterator & it) const {
	if (it->type == TokenType::KW_DETERMINISTIC || it->type == TokenType::KW_NONDETERMINISTIC || it->type == TokenType::KW_FINITE) {
		std::unique_ptr<FADefinition> faDef = std::make_unique<FADefinition>();

		FAType faType = FAType::Nondeterministic;
		if (it->type != TokenType::KW_FINITE) {
			faType = (it->type == TokenType::KW_DETERMINISTIC ? FAType::Deterministic : FAType::Nondeterministic);
			++it;
		}
		faDef->type = faType;

		if (it->type != TokenType::KW_FINITE) {
			// throw
		}
		++it;

		if (it->type != TokenType::KW_AUTOMATON) {
			// throw
		}
		++it;

		if (it->type != TokenType::IDENTIFIER) {
			//throw
		}
		faDef->machineName = it->string;
		++it;

		if (it->type == TokenType::KW_WITH) {
			// handle the with clauses
		}

		if (it->type == TokenType::KW_EXTENDS) {
			++it;
			if (it->type != TokenType::IDENTIFIER) {
				// throw
			}
			faDef->extends = it->string;
			++it;
		} else if (it->type == TokenType::KW_FOLLOWS) {
			++it;
			if (it->type != TokenType::IDENTIFIER) {
				// throw
			}
			faDef->follows = it->string;
			++it;
		}

		if (it->type == TokenType::OP_SEMICOLON) {
			++it;
			return faDef;
		}

		if (it->type != TokenType::CURLY_LEFT) {
			// throw
		}
		++it;

		std::unique_ptr<Statement> lastStatement;
		do {
			auto savedIt = it;
			lastStatement = Parser::parseStatement(it);
			if (lastStatement != nullptr) {
				faDef->statements.push_back(lastStatement);
			} else {
				it = savedIt;
				break;
			}
		} while (true);

		return faDef;
	}

	return nullptr;
}

std::unique_ptr<MachineDefinition> Parser::parseStatement(std::list<Token>::const_iterator& it) const {
	// implement
	return std::unique_ptr<MachineDefinition>();
}
