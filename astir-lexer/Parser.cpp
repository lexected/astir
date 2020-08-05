#include "Parser.h"

#include <set>

using namespace std;

std::unique_ptr<Specification> Parser::parse(const std::list<Token>& tokens) const {
	auto it = tokens.begin();

	unique_ptr<Specification> specification(new Specification);
	while (it->type != TokenType::EOS) {
		auto savedIt = it;
		auto machineDefinition = Parser::parseMachineDefinition(it);
		if (machineDefinition == nullptr) {
			it = savedIt;
			// throw
		}

		specification->machineDefinitions.push_back(std::move(machineDefinition));
	}

	return specification;
}

std::unique_ptr<MachineDefinition> Parser::parseMachineDefinition(std::list<Token>::const_iterator & it) const {
	auto savedIt = it;
	if (it->type == TokenType::KW_DETERMINISTIC || it->type == TokenType::KW_NONDETERMINISTIC || it->type == TokenType::KW_FINITE) {
		std::unique_ptr<FADefinition> faDef = std::make_unique<FADefinition>();

		FAType faType = FAType::Nondeterministic;
		if (it->type != TokenType::KW_FINITE) {
			faType = (it->type == TokenType::KW_DETERMINISTIC ? FAType::Deterministic : FAType::Nondeterministic);
			++it;
		}
		faDef->type = faType;

		if (it->type != TokenType::KW_FINITE) {
			throw UnexpectedTokenException(*it, "the keyword 'finite'", "for finite automaton declaration", *savedIt);
		}
		++it;

		if (it->type != TokenType::KW_AUTOMATON) {
			throw UnexpectedTokenException(*it, "the keyword 'automaton'", "for finite automaton declaration", *savedIt);
		}
		++it;

		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier", "for finite automaton declaration", *savedIt);
		}
		faDef->machineName = it->string;
		++it;

		if (it->type == TokenType::KW_WITH) {
			++it;
			std::set<TokenType> usedAttributes;
			if (it->type == TokenType::KW_GROUPED_STRING_LITERALS) {
				if (usedAttributes.find(TokenType::KW_GROUPED_STRING_LITERALS) != usedAttributes.cend()) { 
					throw UnexpectedTokenException(*it, "a previously unused attribute", "finite automaton declaration", *savedIt);
				} else if(usedAttributes.find(TokenType::KW_INDIVIDUAL_STRING_LITERALS) != usedAttributes.cend()) {
					throw UnexpectedTokenException(*it, "an attribute that does not contradict the previously used 'individual_string_literals'", "finite automaton declaration", *savedIt);
				} else {
					usedAttributes.insert(TokenType::KW_GROUPED_STRING_LITERALS);
					faDef->attributes[FAFlag::GroupedStringLiterals] = true;
				}
			} else if (it->type == TokenType::KW_INDIVIDUAL_STRING_LITERALS) {
				if (usedAttributes.find(TokenType::KW_GROUPED_STRING_LITERALS) != usedAttributes.cend()) {
					throw UnexpectedTokenException(*it, "an attribute that does not contradict the previously used 'grouped_string_literals'", "finite automaton declaration", *savedIt);
				} else if (usedAttributes.find(TokenType::KW_INDIVIDUAL_STRING_LITERALS) != usedAttributes.cend()) {
					throw UnexpectedTokenException(*it, "a previously unused attribute", "finite automaton declaration", *savedIt);
				} else {
					usedAttributes.insert(TokenType::KW_INDIVIDUAL_STRING_LITERALS);
					faDef->attributes[FAFlag::GroupedStringLiterals] = false;
				}
			} else if (it->type == TokenType::KW_TABLE_LOOKUP) {
				if (usedAttributes.find(TokenType::KW_TABLE_LOOKUP) != usedAttributes.cend()) {
					throw UnexpectedTokenException(*it, "a previously unused attribute", "finite automaton declaration", *savedIt);
				} else if (usedAttributes.find(TokenType::KW_MACHINE_LOOKUP) != usedAttributes.cend()) {
					throw UnexpectedTokenException(*it, "an attribute that does not contradict the previously used 'machine_lookup'", "finite automaton declaration", *savedIt);
				} else {
					usedAttributes.insert(TokenType::KW_TABLE_LOOKUP);
					faDef->attributes[FAFlag::TableLookup] = true;
				}
			} else if (it->type == TokenType::KW_MACHINE_LOOKUP) {
				if (usedAttributes.find(TokenType::KW_TABLE_LOOKUP) != usedAttributes.cend()) {
					throw UnexpectedTokenException(*it, "an attribute that does not contradict the previously used 'table_lookup'", "finite automaton declaration", *savedIt);
				} else if (usedAttributes.find(TokenType::KW_MACHINE_LOOKUP) != usedAttributes.cend()) {
					throw UnexpectedTokenException(*it, "a previously unused attribute", "finite automaton declaration", *savedIt);
				} else {
					usedAttributes.insert(TokenType::KW_MACHINE_LOOKUP);
					faDef->attributes[FAFlag::TableLookup] = false;
				}
			} else {
				throw UnexpectedTokenException(*it, "a valid finite automaton attribute", "finite automaton declaration", *savedIt);
			}
		}

		if (it->type == TokenType::KW_EXTENDS) {
			++it;
			if (it->type != TokenType::IDENTIFIER) {
				throw UnexpectedTokenException(*it, "an identifier for the target of 'extends'", "for finite automaton declaration", *savedIt);
			}
			faDef->extends = it->string;
			++it;
		} else if (it->type == TokenType::KW_FOLLOWS) {
			++it;
			if (it->type != TokenType::IDENTIFIER) {
				throw UnexpectedTokenException(*it, "an identifier for the target of 'follows'", "for finite automaton declaration", *savedIt);
			}
			faDef->follows = it->string;
			++it;
		}

		if (it->type == TokenType::OP_SEMICOLON) {
			++it;
			return faDef;
		}

		if (it->type != TokenType::CURLY_LEFT) {
			throw UnexpectedTokenException(*it, "definition body opening bracket '{'", "for finite automaton definition", *savedIt);
		}
		++it;

		std::unique_ptr<Statement> lastStatement;
		do {
			auto savedIt = it;
			lastStatement = Parser::parseStatement(it);
			if (lastStatement != nullptr) {
				faDef->statements.push_back(std::move(lastStatement));
			} else {
				it = savedIt;
				break;
			}
		} while (true);

		if (it->type != TokenType::CURLY_RIGHT) {
			throw UnexpectedTokenException(*it, "a statement or definition body opening bracket '}'", "for finite automaton definition", *savedIt);
		}
		++it;

		// no need for move, according to 'internet'
		return faDef;
	}

	return nullptr;
}

std::unique_ptr<Statement> Parser::parseStatement(std::list<Token>::const_iterator& it) const {
	auto initIt = it;
	if (it->type == TokenType::KW_CATEGORY) {
		++it;
		std::unique_ptr<CategoryStatement> catstat = make_unique<CategoryStatement>();

		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier for the category name", "for category declaration", *initIt);
		}
		catstat->name = it->string;
		++it;
		
		if (it->type == TokenType::OP_COLON) {
			++it;
			do {
				if (it->type != TokenType::IDENTIFIER) {
					throw UnexpectedTokenException(*it, "an category name identifier", "for inheritance in category declaration", *initIt);
				}
				catstat->categories.push_back(it->string);
				++it;

				if (it->type != TokenType::OP_COMMA) {
					break;
				}
				++it;
			} while (true);
		}

		if (it->type == TokenType::OP_SEMICOLON) {
			return catstat;
		}

		if (it->type != TokenType::OP_EQUALS) {
			throw UnexpectedTokenException(*it, "'=' followed by a list of qualified names", "for category declaration", *initIt);
		}

		std::unique_ptr<QualifiedName> lastName;
		do {
			auto savedIt = it;
			lastName = Parser::parseQualifiedName(it);
			if (lastName != nullptr) {
				catstat->qualifiedNames.push_back(std::move(lastName));
			} else {
				it = savedIt;
				break;
			}
		} while (true);

		if (it->type != TokenType::OP_SEMICOLON) {
			throw UnexpectedTokenException(*it, "the terminal semicolon ';'", "for category definition", *initIt);
		}

		return catstat;
	} else if (it->type == TokenType::KW_REGEX) {
		
	} else if (it->type == TokenType::KW_TOKEN) {
		
	} else if (it->type == TokenType::KW_RULE) {
		
	} else if (it->type == TokenType::KW_PRODUCTION) {
		
	} else {
		return nullptr;
	}
}

std::unique_ptr<QualifiedName> Parser::parseQualifiedName(std::list<Token>::const_iterator& it) const {
	std::unique_ptr<QualifiedName> qname = make_unique<QualifiedName>();
	if (it->type != TokenType::IDENTIFIER) {
		return nullptr;
	}
	qname->queriedCategories.push_back(it->string);
	++it;

	do {
		if (it->type != TokenType::OP_AMPERSAND) {
			break;
		}
		++it;

		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier to follow the ampersand qualification operator", "for qualified name");
		}
		qname->queriedCategories.push_back(it->string);
		++it;
	} while (true);

	if (it->type == TokenType::OP_AT) {
		++it;
		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier for instance name", "for qualified name");
		}
		qname->instanceName = it->string;
		++it;
	}

	return qname;
}
