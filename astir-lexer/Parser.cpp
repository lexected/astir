#include "Parser.h"

#include <set>

using namespace std;

/*
	A VERY IMPORTANT RULE:
	 --- IF YOU'RE RETURNING NULLPTR, YOU ARE RESPONSIBLE FOR MAKING SURE IT POINTS TO WHERE IT WAS POINTING WHEN IT WAS PASSED TO YOU ---
*/

std::unique_ptr<SyntacticTree> Parser::parse(const std::list<Token>& tokens) const {
	auto it = tokens.cbegin();

	unique_ptr<SyntacticTree> specificationFile = make_unique<SyntacticTree>();
	while (it->type != TokenType::EOS) {
		auto savedIt = it;
		
		unique_ptr<UsesStatement> usesStatement;
		if ((usesStatement = Parser::parseUsesStatement(it))) {
			specificationFile->usesStatements.push_back(move(usesStatement));
			continue;
		} 

		unique_ptr<MachineDefinition> machineDefinition;
		if ((machineDefinition = Parser::parseMachineDefinition(it))) {
			specificationFile->machineDefinitions.push_back(move(machineDefinition));
			continue;
		}

		throw ParserException("Unexpected input on specification file level, expected a machine definition or a 'uses' statement", *it, *savedIt);
	}
	
	return specificationFile;
}

std::unique_ptr<UsesStatement> Parser::parseUsesStatement(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	if (it->type != TokenType::KW_USES) {
		return nullptr;
	}
	++it;

	unique_ptr<UsesStatement> usingStatement = make_unique<UsesStatement>();
	usingStatement->copyLocation(*savedIt);
	if (it->type != TokenType::STRING) {
		throw UnexpectedTokenException(*it, "a string with file path", "for 'uses' statement", *savedIt);
	}
	usingStatement->filePath = it->string;
	++it;

	if (it->type != TokenType::OP_SEMICOLON) {
		throw UnexpectedTokenException(*it, "terminal semicolon", "for 'uses' statement", *savedIt);
	}
	++it;

	return usingStatement;
}

std::unique_ptr<MachineDefinition> Parser::parseMachineDefinition(std::list<Token>::const_iterator & it) const {
	auto savedIt = it;
	auto machineDefinition = parseMachineType(it);
	if (!machineDefinition) {
		return nullptr;
	}

	if (it->type != TokenType::IDENTIFIER) {
		throw UnexpectedTokenException(*it, "an identifier", "for machine declaration", *savedIt);
	}
	machineDefinition->name = it->string;
	++it;

	if (it->type == TokenType::KW_WITH) {
		++it;
		
		if (!tryParseMachineFlag(it, machineDefinition->attributes)) {
			throw UnexpectedTokenException(*it, "an attribute-setting keyword to follow 'with'", "for machine declaration", *savedIt);
		}
		while (it->type == TokenType::OP_COMMA) {
			++it;
			if (!tryParseMachineFlag(it, machineDefinition->attributes)) {
				throw UnexpectedTokenException(*it, "an attribute-setting keyword to follow comma in the 'with' clause", "for machine declaration", *savedIt);
			}
		}
	}

	if (it->type == TokenType::KW_USES) {
		++it;
		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier for the machine to \"use\" to follow the 'uses' keyword", "for machine declaration", *savedIt);
		}
		machineDefinition->uses.push_back(it->string);

		while (it->type == TokenType::OP_COMMA) {
			++it;

			if (it->type != TokenType::IDENTIFIER) {
				throw UnexpectedTokenException(*it, "an identifier for the machine to \"use\" in the 'uses' clause", "for machine declaration", *savedIt);
			}

			machineDefinition->uses.push_back(it->string);
			++it;
		}
	} else if (it->type == TokenType::KW_ON) {
		++it;

		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier for the target of 'on' (i.e. underlying/input machine)", "for machine declaration", *savedIt);
		}

		machineDefinition->on = it->string;
		++it;
	}

	if (it->type == TokenType::OP_SEMICOLON) {
		++it;
		return machineDefinition;
	}

	if (it->type != TokenType::CURLY_LEFT) {
		throw UnexpectedTokenException(*it, "definition body opening bracket '{'", "for machine definition", *savedIt);
	}
	++it;

	std::unique_ptr<MachineStatement> lastStatement;
	do {
		std::shared_ptr<CategoryStatement> categoryStatement;
		if (categoryStatement = Parser::parseCategoryStatement(it)) {
			machineDefinition->categoryStatements.push_back(categoryStatement);
			continue;
		}

		std::shared_ptr<RuleStatement> ruleStatement;
		if (ruleStatement = Parser::parseRuleStatement(it)) {
			machineDefinition->ruleStatements.push_back(ruleStatement);
			continue;
		}

		break;
	} while (true);

	if (it->type != TokenType::CURLY_RIGHT) {
		throw UnexpectedTokenException(*it, "a statement or definition body opening bracket '}'", "for machine definition", *savedIt);
	}
	++it;

	return machineDefinition;
}

bool Parser::tryParseMachineFlag(std::list<Token>::const_iterator& it, std::map<MachineFlag, MachineDefinitionAttribute>& attributes) const {
	std::pair<MachineFlag, bool> setting;
	if (it->type == TokenType::KW_PRODUCTIONS_TERMINAL_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::ProductionsTerminalByDefault, true);
	} else if (it->type == TokenType::KW_PRODUCTIONS_NONTERMINAL_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::ProductionsTerminalByDefault, false);
	} else if (it->type == TokenType::KW_RULES_PRODUCTIONS_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::RulesProductionsByDefault, true);
	} else if (it->type == TokenType::KW_RULES_PATTERNS_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::ProductionsTerminalByDefault, false);
	} else {
		return false;
	}
	
	
	if (attributes[setting.first].set) {
		throw ParserException("The attribute setting '" + it->string + "' in 'with' clause of a machine definition attempts to configure machine on an attribute that has already been explicitly set - check for repetitive or contradictory use of attribute settings", *it);
	} else {
		attributes[setting.first].set = false;
		attributes[setting.first].value = setting.second;
	}

	++it;
	return true;
}

std::unique_ptr<MachineDefinition> Parser::parseMachineType(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	if (it->type == TokenType::KW_FINITE) {
		std::unique_ptr<FiniteAutomatonDefinition> faDef = std::make_unique<FiniteAutomatonDefinition>();
		faDef->copyLocation(*savedIt);
		++it;

		if (it->type != TokenType::KW_AUTOMATON) {
			throw UnexpectedTokenException(*it, "the keyword 'automaton'", "for finite automaton declaration", *savedIt);
		}
		++it;

		return faDef;
	} else {
		return nullptr;
	}
}

std::unique_ptr<CategoryStatement> Parser::parseCategoryStatement(std::list<Token>::const_iterator& it) const {
	if (it->type != TokenType::KW_CATEGORY) {
		return nullptr;
	}

	auto savedIt = it;
	++it;
	std::unique_ptr<CategoryStatement> catstat = make_unique<CategoryStatement>();
	catstat->copyLocation(*savedIt);

	if (it->type != TokenType::IDENTIFIER) {
		throw UnexpectedTokenException(*it, "an identifier for the category name", "for category declaration", *savedIt);
	}
	catstat->name = it->string;
	++it;

	if (it->type == TokenType::OP_COLON) {
		++it;
		do {
			if (it->type != TokenType::IDENTIFIER) {
				throw UnexpectedTokenException(*it, "an category name identifier", "for inheritance in category declaration", *savedIt);
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
		throw UnexpectedTokenException(*it, "'{' followed by member declarations", "for category definition", *savedIt);
	}
	++it;

	std::unique_ptr<Field> lastDeclaration;
	do {
		auto savedIt = it;
		lastDeclaration = Parser::parseMemberDeclaration(it);
		if (lastDeclaration) {
			catstat->fields.push_back(std::move(lastDeclaration));
		} else {
			it = savedIt;
			break;
		}
	} while (true);

	if (it->type != TokenType::CURLY_RIGHT) {
		throw UnexpectedTokenException(*it, "a token for member declaration or the matching closing right curly bracket '}'", "for category definition", *savedIt);
	}
	++it;

	if (it->type != TokenType::OP_SEMICOLON) {
		throw UnexpectedTokenException(*it, "the terminal semicolon ';'", "for category definition", *savedIt);
	}
	++it;

	return catstat;
}

std::unique_ptr<RuleStatement> Parser::parseRuleStatement(std::list<Token>::const_iterator& it) const {
	if (it->type != TokenType::KW_TERMINAL
		&& it->type != TokenType::KW_NONTERMINAL
		&& it->type != TokenType::KW_PATTERN
		&& it->type != TokenType::KW_PRODUCTION
		&& it->type != TokenType::IDENTIFIER) {
		return nullptr;
	}

	auto savedIt = it;
	std::unique_ptr<RuleStatement> grastat = make_unique<RuleStatement>();
	grastat->copyLocation(*savedIt);

	if (it->type == TokenType::KW_TERMINAL) {
		grastat->terminalitySpecified = true;
		grastat->terminality = true;
		++it;
	} else if(it->type == TokenType::KW_NONTERMINAL) {
		grastat->terminalitySpecified = true;
		grastat->terminality = false;
		++it;
	}

	std::string grammarStatementType = "production/pattern";
	if (it->type == TokenType::KW_PATTERN) {
		grastat->typeSpecified = true;
		grastat->type = RuleStatementType::Pattern;
		++it;

		grammarStatementType = "pattern";
	} else if (it->type == TokenType::KW_PRODUCTION) {
		grastat->typeSpecified = true;
		grastat->type = RuleStatementType::Production;
		++it;

		grammarStatementType = "production";
	}

	if (it->type != TokenType::IDENTIFIER) {
		throw UnexpectedTokenException(*it, "an identifier to serve as "+ grammarStatementType + " name", " for " + grammarStatementType + " declaration", *savedIt);
	}
	grastat->name = it->string;
	++it;

	if (it->type == TokenType::OP_COLON) {
		++it;
		do {
			if (it->type != TokenType::IDENTIFIER) {
				throw UnexpectedTokenException(*it, "a " + grammarStatementType + " name identifier", "for inheritance in " + grammarStatementType + " declaration", *savedIt);
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

		std::unique_ptr<Field> lastDeclaration;
		do {
			auto savedIt = it;
			lastDeclaration = Parser::parseMemberDeclaration(it);
			if (lastDeclaration) {
				grastat->fields.push_back(std::move(lastDeclaration));
			} else {
				it = savedIt;
				break;
			}
		} while (true);

		if (it->type != TokenType::CURLY_RIGHT) {
			throw UnexpectedTokenException(*it, "a token for member declaration or the matching closing right curly bracket '}'", "for " + grammarStatementType + " definition", *savedIt);
		}
		++it;
	}

	if (it->type != TokenType::OP_EQUALS) {
		throw UnexpectedTokenException(*it, "'=' followed by a list of qualified names", "for " + grammarStatementType + " declaration", *savedIt);
	}
	++it;

	auto prevIt = it;
	std::shared_ptr<DisjunctiveRegex> disjunction = Parser::parseDisjunctiveRegex(it);
	if (!disjunction) {
		throw ParserException("Expected an alternative (i.e. a valid, possibly disjunctive, astir regex) to follow the '=' on the intial token - none found.", *it, *prevIt);
	}
	grastat->disjunction = move(disjunction);

	if (it->type != TokenType::OP_SEMICOLON) {
		throw UnexpectedTokenException(*it, "the terminal semicolon ';'", "for " + grammarStatementType + " definition", *savedIt);
	}
	++it;

	return grastat;
}

std::unique_ptr<Field> Parser::parseMemberDeclaration(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	Field* md;
	if (it->type == TokenType::KW_FLAG || it->type == TokenType::KW_RAW) {
		std::string typeForANicePersonalizedExceptionMessage = it->string;
		md = it->type == TokenType::KW_FLAG ? dynamic_cast<Field *>(new FlagField) : dynamic_cast<Field*>(new RawField);
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

		VariablyTypedField* vtd;
		
		if (it->type == TokenType::KW_LIST) {
			vtd = new ListField;
			++it;
		} else if (it->type == TokenType::KW_ITEM) {
			vtd = new ItemField;
			++it;
		} else if (it->type == TokenType::IDENTIFIER) {
			vtd = new ItemField;
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

	md->copyLocation(*savedIt);
	return std::unique_ptr<Field>(md);
}

std::unique_ptr<RootRegex> Parser::parseRootRegex(std::list<Token>::const_iterator& it) const {
	unique_ptr<RootRegex> rr;
	if ((rr = parseRepetitiveRegex(it))) {
		return rr;
	} else if ((rr = parseLookaheadRegex(it))) {
		return rr;
	} else if ((rr = parseAtomicRegex(it))) {
		return rr;
	} else {
		return nullptr;
	}
}

std::unique_ptr<RepetitiveRegex> Parser::parseRepetitiveRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	unique_ptr<AtomicRegex> atomicRegex = parseAtomicRegex(it);
	if (!atomicRegex) {
		// throw UnexpectedTokenException(*it, "token for action-atomic regex", "for repetitive regex as an alternative of root regex", *savedIt);
		return nullptr;
	}

	if (it->type == TokenType::OP_QM) {
		++it;
		unique_ptr<RepetitiveRegex> rr = make_unique<RepetitiveRegex>();
		rr->minRepetitions = 0;
		rr->maxRepetitions = 1;
		rr->regex = move(atomicRegex);
		rr->copyLocation(*savedIt);
		return rr;
	} else if (it->type == TokenType::OP_STAR) {
		++it;
		unique_ptr<RepetitiveRegex> rr = make_unique<RepetitiveRegex>();
		rr->minRepetitions = 0;
		rr->maxRepetitions = rr->INFINITE_REPETITIONS;
		rr->regex = move(atomicRegex);
		rr->copyLocation(*savedIt);
		return rr;
	} else if (it->type == TokenType::OP_PLUS) {
		++it;
		unique_ptr<RepetitiveRegex> rr = make_unique<RepetitiveRegex>();
		rr->minRepetitions = 1;
		rr->maxRepetitions = rr->INFINITE_REPETITIONS;
		rr->regex = move(atomicRegex);
		rr->copyLocation(*savedIt);
		return rr;
	} else if (it->type == TokenType::CURLY_LEFT) {
		++it;

		unique_ptr<RepetitiveRegex> rr = make_unique<RepetitiveRegex>();
		rr->copyLocation(*savedIt);
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
		
		rr->regex = move(atomicRegex);
		return rr;
	} else {
		it = savedIt;
		return nullptr;
	}
}

std::unique_ptr<LookaheadRegex> Parser::parseLookaheadRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	unique_ptr<AtomicRegex> atomicRegex = parseAtomicRegex(it);
	if (!atomicRegex) {
		// throw UnexpectedTokenException(*it, "token for action-atomic regex", "for lookahead regex as an alternative of root regex", *savedIt);
		return nullptr;
	}

	if (it->type == TokenType::OP_FWDSLASH) {
		++it;

		unique_ptr<LookaheadRegex> lr = make_unique<LookaheadRegex>();
		lr->copyLocation(*savedIt);
		unique_ptr<PrimitiveRegex> ar = parsePrimitiveRegex(it);
		if (!ar) {
			throw UnexpectedTokenException(*it, "a token for atomic regex to follow '\\'", "for lookahead match regex", *savedIt);
		}
		lr->match = move(atomicRegex);
		lr->lookahead = move(ar);
		return lr;
	} else {
		it = savedIt;
		return nullptr;
	}
}

std::unique_ptr<PrimitiveRegex> Parser::parsePrimitiveRegex(std::list<Token>::const_iterator& it) const {
	/* perhaps rather surprisingly, this will be a lookahead bit of this parser */
	auto savedIt = it; // for throwing purposes, heh

	if (it->type == TokenType::SQUARE_LEFT) {
		unique_ptr<AnyRegex> ret;
		if (tryParseAnyRegex(it, ret)) {
			return ret;
		}
	}

	unique_ptr<PrimitiveRegex> ret;
	if (it->type == TokenType::STRING) {
		auto lr = make_unique<LiteralRegex>();
		lr->literal = it->string;
		ret = move(lr);
	} else if (it->type == TokenType::OP_DOT) {
		ret = make_unique<ArbitraryLiteralRegex>();
	} /*else if (it->type == TokenType::OP_CARET) {
		ret = make_unique<LineBeginRegex>();
	}*/ else if (it->type == TokenType::OP_DOLLAR) {
		ret = make_unique<LineEndRegex>();
	} else if (it->type == TokenType::IDENTIFIER) {
		auto rr = make_unique<ReferenceRegex>();
		rr->referenceName = it->string;
		ret = move(rr);
	} else {
		return nullptr; // needs to be like this
	}
	++it;

	ret->copyLocation(*savedIt);
	
	while (it->type == TokenType::OP_AT) {
		auto interimIt = it;
		++it;

		RegexAction atp;
		atp.type = parseRegexAction(it);

		if (it->type != TokenType::OP_COLON) {
			throw UnexpectedTokenException(*it, "a colon ':' to separate action from target", "for action-atomic regex", *savedIt);
		}
		++it;

		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier to specify the action target", "for action-atomic regex", *savedIt);
		}
		atp.target = it->string;
		atp.copyLocation(*interimIt);
		++it;

		ret->actions.push_back(move(atp));
	}

	return ret;
}

RegexActionType Parser::parseRegexAction(std::list<Token>::const_iterator& it) const {
	RegexActionType ret;
	switch (it->type) {
		case TokenType::KW_FLAG:
			ret = RegexActionType::Flag;
			break;
		case TokenType::KW_UNFLAG:
			ret = RegexActionType::Unflag;
			break;

		case TokenType::KW_CAPTURE:
			ret = RegexActionType::Capture;
			break;
		case TokenType::KW_EMPTY:
			ret = RegexActionType::Empty;
			break;
		case TokenType::KW_APPEND:
			ret = RegexActionType::Append;
			break;
		case TokenType::KW_PREPEND:
			ret = RegexActionType::Prepend;
			break;

		case TokenType::KW_SET:
			ret = RegexActionType::Set;
			break;
		case TokenType::KW_UNSET:
			ret = RegexActionType::Unset;
			break;
		case TokenType::KW_PUSH:
			ret = RegexActionType::Push;
			break;
		case TokenType::KW_POP:
			ret = RegexActionType::Pop;
			break;
		case TokenType::KW_CLEAR:
			ret = RegexActionType::Clear;
			break;
		default:
			throw UnexpectedTokenException(*it, "a valid action type keyword to follow the action operator '@'", "for action-atomic regex");
	}
	++it;
	
	return ret;
}

std::unique_ptr<AtomicRegex> Parser::parseAtomicRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	if (it->type == TokenType::PAR_LEFT) {
		++it;

		auto dr = parseDisjunctiveRegex(it);
		if (!dr) {
			throw UnexpectedTokenException(*it, "a beginning token for a disjunctive regex", "for atomic regex", *savedIt);
		}

		if (it->type != TokenType::PAR_RIGHT) {
			throw UnexpectedTokenException(*it, "a matching closing right parenthesis ')'", "for bracketed disjunctive regex as a part of atomic regex", *savedIt);
		}
		++it;

		return dr;
	}

	auto primitiveRegexPtr = Parser::parsePrimitiveRegex(it);
	if (!primitiveRegexPtr) {
		return nullptr;
	}

	return primitiveRegexPtr;
}

bool Parser::tryParseAnyRegex(std::list<Token>::const_iterator& it, std::unique_ptr<AnyRegex>& anyRegexPtr) const {
	if (it->type != TokenType::SQUARE_LEFT) {
		return false;
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
	ar->copyLocation(*savedIt);

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

	anyRegexPtr = std::unique_ptr<AnyRegex>(ar);

	return true;
}

std::unique_ptr<ConjunctiveRegex> Parser::parseConjunctiveRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;

	auto rr = parseRootRegex(it);
	if (!rr) {
		return nullptr;
	}

	auto cr = make_unique<ConjunctiveRegex>();
	cr->copyLocation(*savedIt);
	while (rr) {
		cr->conjunction.push_back(move(rr));

		rr = parseRootRegex(it);
	}

	return cr;
}

std::unique_ptr<DisjunctiveRegex> Parser::parseDisjunctiveRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	auto cr = parseConjunctiveRegex(it);
	if (!cr) {
		return nullptr;
	}

	auto dr = make_unique<DisjunctiveRegex>();
	dr->disjunction.push_back(move(cr));
	dr->copyLocation(*savedIt);

	while (it->type == TokenType::OP_OR) {
		++it;

		cr = parseConjunctiveRegex(it);
		if (!cr) {
			throw UnexpectedTokenException(*it, "a beginning token for a conjunctive regex", "for disjunctive regex", *savedIt);
		}
		dr->disjunction.push_back(move(cr));
	}

	return dr;
}
