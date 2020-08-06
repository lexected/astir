#include "Parser.h"

#include <set>

using namespace std;

/*
	A VERY IMPORTANT RULE:
	 --- IF YOU'RE RETURNING NULLPTR, YOU ARE RESPONSIBLE FOR MAKING SURE IT POINTS TO WHERE IT WAS POINTING WHEN IT WAS PASSED TO YOU ---
*/

std::unique_ptr<Specification> Parser::parse(const std::list<Token>& tokens) const {
	auto it = tokens.begin();

	unique_ptr<Specification> specification(new Specification);
	while (it->type != TokenType::EOS) {
		auto savedIt = it;
		unique_ptr<SpecificationStatement> specificationStatement;
		if ((specificationStatement = Parser::parseUsingStatement(it)) != nullptr) {
			specification->statements.push_back(move(specificationStatement));
		} else if ((specificationStatement = Parser::parseMachineDefinition(it)) != nullptr) {
			specification->statements.push_back(move(specificationStatement));
		} else {
			throw ParserException("Unexpected input on specification level, expected a machine definition or a 'using' statement", *it, *savedIt);
		}
	}

	return specification;
}

std::unique_ptr<UsingStatement> Parser::parseUsingStatement(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	if (it->type != TokenType::KW_USING) {
		return nullptr;
	}
	++it;

	unique_ptr<UsingStatement> usingStatement = make_unique<UsingStatement>();
	if (it->type != TokenType::STRING) {
		throw UnexpectedTokenException(*it, "a string with file path", "for 'using' statement", *savedIt);
	}
	usingStatement->filePath = it->string;
	++it;

	if (it->type != TokenType::OP_SEMICOLON) {
		throw UnexpectedTokenException(*it, "terminal semicolon", "for 'using' statement", *savedIt);
	}
	++it;

	return usingStatement;
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

		std::unique_ptr<MachineStatement> lastStatement;
		do {
			auto savedIt = it;
			lastStatement = Parser::parseMachineStatement(it);
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

std::unique_ptr<MachineStatement> Parser::parseMachineStatement(std::list<Token>::const_iterator& it) const {
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
			++it;
			return catstat;
		}

		if (it->type != TokenType::CURLY_LEFT) {
			throw UnexpectedTokenException(*it, "'{' followed by member declarations", "for category definition", *initIt);
		}
		++it;

		std::unique_ptr<MemberDeclaration> lastDeclaration;
		do {
			auto savedIt = it;
			lastDeclaration = Parser::parseMemberDeclaration(it);
			if (lastDeclaration != nullptr) {
				catstat->members.push_back(std::move(lastDeclaration));
			} else {
				it = savedIt;
				break;
			}
		} while (true);

		if (it->type != TokenType::CURLY_RIGHT) {
			throw UnexpectedTokenException(*it, "a token for member declaration or the matching closing right curly bracket '}'", "for category definition", *initIt);
		}
		++it;

		if (it->type != TokenType::OP_SEMICOLON) {
			throw UnexpectedTokenException(*it, "the terminal semicolon ';'", "for category definition", *initIt);
		}
		++it;

		return catstat;
	} else if (it->type == TokenType::KW_REGEX || it->type == TokenType::KW_TOKEN || it->type == TokenType::KW_RULE || it->type == TokenType::KW_PRODUCTION) {
		string grammarStatementType = it->toHumanString();
		std::unique_ptr<GrammarStatement> grastat = make_unique<GrammarStatement>();
		switch (it->type) {
			case TokenType::KW_REGEX:
				grastat->type = GrammarStatementType::Regex;
				break;
			case TokenType::KW_TOKEN:
				grastat->type = GrammarStatementType::Token;
				break;
			case TokenType::KW_RULE:
				grastat->type = GrammarStatementType::Rule;
				break;
			case TokenType::KW_PRODUCTION:
				grastat->type = GrammarStatementType::Production;
				break;
			default:
				throw ParserException("Unrecognized token '" + grammarStatementType + "' accepted as a grammar statement type at " +it->locationString());
		}
		++it;

		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier for the "+ grammarStatementType +" name", "for " + grammarStatementType + " declaration", *initIt);
		}
		grastat->name = it->string;
		++it;

		if (it->type == TokenType::OP_COLON) {
			++it;
			do {
				if (it->type != TokenType::IDENTIFIER) {
					throw UnexpectedTokenException(*it, "an " + grammarStatementType + " name identifier", "for inheritance in " + grammarStatementType + " declaration", *initIt);
				}
				grastat->categories.push_back(it->string);
				++it;

				if (it->type != TokenType::OP_COMMA) {
					break;
				}
				++it;
			} while (true);
		}

		if (it->type == TokenType::CURLY_LEFT) {
			++it;

			std::unique_ptr<MemberDeclaration> lastDeclaration;
			do {
				auto savedIt = it;
				lastDeclaration = Parser::parseMemberDeclaration(it);
				if (lastDeclaration != nullptr) {
					grastat->members.push_back(std::move(lastDeclaration));
				} else {
					it = savedIt;
					break;
				}
			} while (true);

			if (it->type != TokenType::CURLY_RIGHT) {
				throw UnexpectedTokenException(*it, "a token for member declaration or the matching closing right curly bracket '}'", "for "+ grammarStatementType +" definition", *initIt);
			}
			++it;
		}

		if (it->type != TokenType::OP_EQUALS) {
			throw UnexpectedTokenException(*it, "'=' followed by a list of qualified names", "for " + grammarStatementType + " declaration", *initIt);
		}
		++it;

		auto savedIt = it;
		std::unique_ptr<Alternative> lastAlternative = Parser::parseAlternative(it);
		if (lastAlternative == nullptr) {
			throw ParserException("Expected an alternative to follow the '=' on the intial token - none found.", *it, *savedIt);
		}
		
		while(it->type == TokenType::OP_OR) {
			++it;

			lastAlternative = Parser::parseAlternative(it);
			if (lastAlternative == nullptr) {
				throw UnexpectedTokenException(*it, "an alternative to follow the alternative operator '|'", "for " + grammarStatementType + " definition body", *initIt);
			}

			grastat->alternatives.push_back(move(lastAlternative));
		}

		if (it->type != TokenType::OP_SEMICOLON) {
			throw UnexpectedTokenException(*it, "the terminal semicolon ';'", "for " + grammarStatementType + " definition", *initIt);
		}
		++it;

		return grastat;
	} else {
		return nullptr;
	}
}

std::unique_ptr<MemberDeclaration> Parser::parseMemberDeclaration(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	MemberDeclaration* md;
	if (it->type == TokenType::KW_FLAG || it->type == TokenType::KW_RAW) {
		std::string typeForANicePersonalizedExceptionMessage = it->string;
		md = it->type == TokenType::KW_FLAG ? dynamic_cast<MemberDeclaration *>(new FlagDeclaration) : dynamic_cast<MemberDeclaration*>(new RawDeclaration);
		++it;

		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier for member name to follow '" + typeForANicePersonalizedExceptionMessage + "'", "member declaration", *savedIt);
		}
		md->name = it->string;
		++it;
	} else {
		if (it->type != TokenType::IDENTIFIER) {
			return nullptr;
		}
		std::string typeName = it->string;
		++it;

		auto vtd = new VariablyTypedDeclaration;
		if (it->type == TokenType::KW_LIST) {
			md = new ListDeclaration;
			++it;
		} else if (it->type == TokenType::KW_ITEM) {
			md = new ItemDeclaration;
			++it;
		} else if (it->type == TokenType::IDENTIFIER) {
			md = new ItemDeclaration;
		} else {
			throw UnexpectedTokenException(*it, "'list', 'item', or an identifier for item (implicitly assumed) name", "for member declaration");
		}

		vtd->name = it->string;
		vtd->type = std::move(typeName);
		md = vtd;
		++it;
	}

	if (it->type != TokenType::OP_SEMICOLON) {
		throw UnexpectedTokenException(*it, "the terminal semicolon ';'", "for member declaration");
	}
	++it;

	return std::unique_ptr<MemberDeclaration>(md);
}

std::unique_ptr<Alternative> Parser::parseAlternative(std::list<Token>::const_iterator& it) const {
	std::unique_ptr<Alternative> alternative = make_unique<Alternative>();

	unique_ptr<RootRegex> lastRootRegex = parseRootRegex(it);
	while (lastRootRegex != nullptr) {
		alternative->rootRegexes.push_back(move(lastRootRegex));
		lastRootRegex = parseRootRegex(it);
	}
	
	return alternative;
}

std::unique_ptr<RootRegex> Parser::parseRootRegex(std::list<Token>::const_iterator& it) const {
	unique_ptr<RootRegex> rr;
	if ((rr = parseRepetitiveRegex(it)) != nullptr) {
		return rr;
	} else if ((rr = parseLookaheadRegex(it)) != nullptr) {
		return rr;
	} else if ((rr = parseActionAtomicRegex(it)) != nullptr) {
		return rr;
	} else {
		return nullptr;
	}
}

std::unique_ptr<RepetitiveRegex> Parser::parseRepetitiveRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	unique_ptr<ActionAtomicRegex> aar = parseActionAtomicRegex(it);
	if (aar == nullptr) {
		throw UnexpectedTokenException(*it, "token for action-atomic regex", "for repetitive regex as an alternative of root regex", *savedIt);
	}

	if (it->type == TokenType::OP_QM) {
		++it;
		unique_ptr<RepetitiveRegex> rr = make_unique<RepetitiveRegex>();
		rr->minRepetitions = 0;
		rr->maxRepetitions = 1;
		rr->actionAtomicRegex = move(aar);
		return rr;
	} else if (it->type == TokenType::OP_STAR) {
		++it;
		unique_ptr<RepetitiveRegex> rr = make_unique<RepetitiveRegex>();
		rr->minRepetitions = 0;
		rr->maxRepetitions = rr->INFINITE_REPETITIONS;
		rr->actionAtomicRegex = move(aar);
		return rr;
	} else if (it->type == TokenType::OP_PLUS) {
		++it;
		unique_ptr<RepetitiveRegex> rr = make_unique<RepetitiveRegex>();
		rr->minRepetitions = 1;
		rr->maxRepetitions = rr->INFINITE_REPETITIONS;
		rr->actionAtomicRegex = move(aar);
		return rr;
	} else if (it->type == TokenType::CURLY_LEFT) {
		++it;

		unique_ptr<RepetitiveRegex> rr = make_unique<RepetitiveRegex>();
		if (it->type != TokenType::NUMBER) {
			throw UnexpectedTokenException(*it, "a number", "for repetition range regex", *savedIt);
		}
		rr->minRepetitions = (unsigned long)std::stoul(it->string);
		++it;

		if (it->type != TokenType::OP_COMMA) {
			throw UnexpectedTokenException(*it, "a comma separating range numbers", "for repetition range regex", *savedIt);
		}
		++it;

		if (it->type != TokenType::NUMBER) {
			throw UnexpectedTokenException(*it, "a number", "for repetition range regex", *savedIt);
		}
		rr->maxRepetitions = (unsigned long)std::stoul(it->string);
		if (rr->minRepetitions > rr->maxRepetitions) {
			throw ParserException("The number for minimum repetitions in the regex is strictly larger than the number for maximum repetitions", *it, *savedIt);
		}
		++it;

		if (it->type != TokenType::CURLY_RIGHT) {
			throw UnexpectedTokenException(*it, "a curly right bracket '}'", "for repetition range regex", *savedIt);
		}
		++it;
		
		rr->actionAtomicRegex = move(aar);
		return rr;
	} else {
		it = savedIt;
		return nullptr;
	}
}

std::unique_ptr<LookaheadRegex> Parser::parseLookaheadRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	unique_ptr<ActionAtomicRegex> aar = parseActionAtomicRegex(it);
	if (aar == nullptr) {
		throw UnexpectedTokenException(*it, "token for action-atomic regex", "for lookahead regex as an alternative of root regex", *savedIt);
	}

	if (it->type == TokenType::OP_FWDSLASH) {
		++it;

		unique_ptr<LookaheadRegex> lr = make_unique<LookaheadRegex>();
		unique_ptr<AtomicRegex> ar = parseAtomicRegex(it);
		if (ar == nullptr) {
			throw UnexpectedTokenException(*it, "a token for atomic regex to follow '\\'", "for lookahead match regex", *savedIt);
		}
		lr->match = move(aar);
		lr->lookahead = move(ar);
		return lr;
	} else {
		return nullptr;
	}
}

std::unique_ptr<ActionAtomicRegex> Parser::parseActionAtomicRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	if (it->type == TokenType::PAR_LEFT) {
		++it;
		auto aar = parseActionAtomicRegex(it);
		
		if (it->type != TokenType::PAR_RIGHT) {
			throw UnexpectedTokenException(*it, "a matching closing right parenthesis ')'", "for action-atomic regex", *savedIt);
		}
		++it;
	}

	auto ar = parseAtomicRegex(it);
	unique_ptr<ActionAtomicRegex> aar = make_unique<ActionAtomicRegex>();
	aar->regex = move(ar);
	while (it->type == TokenType::OP_AT) {
		++it;

		ActionTargetPair atp;
		atp.action = parseRegexAction(it);

		if (it->type != TokenType::OP_COLON) {
			// throw
		}
		++it;

		if (it->type != TokenType::IDENTIFIER) {
			// throw
		}
		atp.target = it->string;
		++it;

		aar->actionTargetPairs.push_back(move(atp));
	}

	return aar;
}

RegexAction Parser::parseRegexAction(std::list<Token>::const_iterator& it) const {
	RegexAction ret;
	switch (it->type) {
		case TokenType::KW_SET:
			ret = RegexAction::Set;
		case TokenType::KW_UNSET:
			ret = RegexAction::Unset;
		case TokenType::KW_FLAG:
			ret = RegexAction::Flag;
		case TokenType::KW_UNFLAG:
			ret = RegexAction::Unflag;
		case TokenType::KW_APPEND:
			ret = RegexAction::Append;
		case TokenType::KW_PREPEND:
			ret = RegexAction::Prepend;
		case TokenType::KW_CLEAR:
			ret = RegexAction::Clear;
		case TokenType::KW_LEFT_TRIM:
			ret = RegexAction::LeftTrim;
		case TokenType::KW_RIGHT_TRIM:
			ret = RegexAction::RightTrim;
		default:
			throw UnexpectedTokenException(*it, "a valid action type keyword to follow the action operator '@'", "for action-atomic regex");
	}
	++it;

	return ret;
}

std::unique_ptr<AtomicRegex> Parser::parseAtomicRegex(std::list<Token>::const_iterator& it) const {
	/* perhaps rather surprisingly, this will be a lookahead bit of this parser */
	auto savedIt = it; // for throwing purposes, heh
	if (it->type == TokenType::PAR_LEFT) {
		++it;

		unique_ptr<DisjunctiveRegex> dr = parseDisjunctiveRegex(it);
		if (dr == nullptr) {
			throw UnexpectedTokenException(*it, "a beginning token for a disjunctive regex", "for atomic regex", *savedIt);
		}

		if (it->type != TokenType::PAR_RIGHT) {
			throw UnexpectedTokenException(*it, "a matching closing right parenthesis ')'", "for bracketed disjunctive regex as a part of atomic regex", *savedIt);
		}
		++it;

		return dr;
	} else if (it->type == TokenType::SQUARE_LEFT) {
		return parseAnyRegex(it);
	} else if (it->type == TokenType::STRING) {
		auto lr = make_unique<LiteralRegex>();
		lr->literal = it->string;
		return lr;
	} else if (it->type == TokenType::OP_DOT) {
		return make_unique<ArbitraryLiteralRegex>();
	} else if (it->type == TokenType::OP_CARET) {
		return make_unique<LineBeginRegex>();
	} else if (it->type == TokenType::OP_DOLLAR) {
		return make_unique<LineEndRegex>();
	} else if (it->type == TokenType::IDENTIFIER) {
		auto rr = make_unique<ReferenceRegex>();
		rr->referenceName = it->string;
		return rr;
	} else {
		throw UnexpectedTokenException(*it, "a token to begin an atomic regex", "for atomic regex", *savedIt);
	}
}

std::unique_ptr<AnyRegex> Parser::parseAnyRegex(std::list<Token>::const_iterator& it) const {
	if (it->type != TokenType::SQUARE_LEFT) {
		return nullptr;
	}
	auto savedIt = it;
	++it;
	
	AnyRegex* ar;
	if (it->type == TokenType::OP_CARET) {
		ar = new ExceptAnyRegex;
		++it;
	} else {
		ar = new AnyRegex;
	}

	while (it->type == TokenType::STRING) {
		std::string firstString = it->string;
		++it;

		if (it->type == TokenType::OP_DASH) {
			++it;

			if (it->type != TokenType::STRING) {
				throw UnexpectedTokenException(*it, "a string literal for end of range to follow '-'", "for 'any/none-of' regex");
			}

			if (firstString[0] > it->string[0]) {
				throw ParserException("In regex range capture, the end character was found to precede the beginning character", *it, *savedIt);
			}
			
			ar->ranges.push_back({ firstString[0], it->string[0] });
			++it;
		} else {
			ar->literals.push_back(move(firstString));
		}
	}

	if (it->type != TokenType::SQUARE_RIGHT) {
		throw UnexpectedTokenException(*it, "literal, literal range, or the matching right square bracket ']'", "for 'any/none-of' regex", *savedIt);
	}
	++it;

	return std::unique_ptr<AnyRegex>(ar);
}

std::unique_ptr<ConjunctiveRegex> Parser::parseConjunctiveRegex(std::list<Token>::const_iterator& it) const {
	auto rr = parseRootRegex(it);
	if (rr == nullptr) {
		return nullptr;
	}

	auto cr = make_unique<ConjunctiveRegex>();
	while (rr != nullptr) {
		cr->conjunction.push_back(move(rr));

		rr = parseRootRegex(it);
	}

	return cr;
}

std::unique_ptr<DisjunctiveRegex> Parser::parseDisjunctiveRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	auto cr = parseConjunctiveRegex(it);
	if (cr == nullptr) {
		return nullptr;
	}

	auto dr = make_unique<DisjunctiveRegex>();
	dr->disjunction.push_back(move(cr));

	while (it->type == TokenType::OP_OR) {
		++it;

		cr = parseConjunctiveRegex(it);
		if (cr == nullptr) {
			throw UnexpectedTokenException(*it, "a beginning token for a conjunctive regex", "for disjunctive regex", *savedIt);
		}
		dr->disjunction.push_back(move(cr));
	}

	return dr;
}
