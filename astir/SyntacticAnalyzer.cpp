#include "SyntacticAnalyzer.h"

#include <set>

using namespace std;

#include "SemanticAnalysisException.h"

/*
	A VERY IMPORTANT RULE:
	 --- IF YOU'RE RETURNING NULLPTR, YOU ARE RESPONSIBLE FOR MAKING SURE IT POINTS TO WHERE IT WAS POINTING WHEN IT WAS PASSED TO YOU ---
*/

std::unique_ptr<SyntacticTree> SyntacticAnalyzer::process(const std::list<Token>& tokens) const {
	auto it = tokens.cbegin();

	unique_ptr<SyntacticTree> specificationFile = make_unique<SyntacticTree>();
	while (it->type != TokenType::EOS) {
		auto savedIt = it;
		
		unique_ptr<UsesStatement> usesStatement;
		if ((usesStatement = SyntacticAnalyzer::parseUsesStatement(it))) {
			specificationFile->usesStatements.push_back(move(usesStatement));
			continue;
		} 

		unique_ptr<MachineDefinition> machineDefinition;
		if ((machineDefinition = SyntacticAnalyzer::parseMachineDefinition(it))) {
			const std::string& incomingMachineDefinitionName = machineDefinition->name;
			auto fit = specificationFile->machineDefinitions.find(incomingMachineDefinitionName);
			if (fit != specificationFile->machineDefinitions.end()) {
				throw SemanticAnalysisException("A machine with the name '" + incomingMachineDefinitionName + "' has already been defined in the current context", *fit->second);
			}

			specificationFile->machineDefinitions.emplace(incomingMachineDefinitionName, move(machineDefinition));
			continue;
		}

		throw SyntacticAnalysisException("Unexpected input on specification file level, expected a machine definition or a 'uses' statement", *it, *savedIt);
	}
	
	return specificationFile;
}

std::unique_ptr<UsesStatement> SyntacticAnalyzer::parseUsesStatement(std::list<Token>::const_iterator& it) const {
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

std::unique_ptr<MachineDefinition> SyntacticAnalyzer::parseMachineDefinition(std::list<Token>::const_iterator & it) const {
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

	if (it->type == TokenType::KW_ON) {
		++it;

		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier for the target of 'on' (i.e. underlying/input machine)", "for machine declaration", *savedIt);
		}

		machineDefinition->on = std::make_pair<std::string, std::shared_ptr<MachineDefinition>>(std::string(it->string), nullptr);
		++it;
	}

	if (it->type == TokenType::KW_USES) {
		++it;
		if (it->type != TokenType::IDENTIFIER) {
			throw UnexpectedTokenException(*it, "an identifier for the machine to \"use\" to follow the 'uses' keyword", "for machine declaration", *savedIt);
		}
		machineDefinition->uses.emplace(it->string, nullptr);

		while (it->type == TokenType::OP_COMMA) {
			++it;

			if (it->type != TokenType::IDENTIFIER) {
				throw UnexpectedTokenException(*it, "an identifier for the machine to \"use\" in the 'uses' clause", "for machine declaration", *savedIt);
			}

			machineDefinition->uses.emplace(it->string, nullptr);
			++it;
		}
	} 

	if (it->type == TokenType::OP_SEMICOLON) {
		++it;
		return machineDefinition;
	}

	if (it->type != TokenType::CURLY_LEFT) {
		throw UnexpectedTokenException(*it, "definition body opening bracket '{'", "for machine definition", *savedIt);
	}
	++it;

	std::shared_ptr<MachineStatement> statement;
	while (true) {
		statement = SyntacticAnalyzer::parseMachineStatement(it);
		if(statement) {
			const std::string& incomingStatementName = statement->name;
			auto fit = machineDefinition->statements.find(incomingStatementName);
			if (fit != machineDefinition->statements.end()) {
				throw SemanticAnalysisException("A statement with the name '" + incomingStatementName + "' has already been defined in the current context", *fit->second);
			}

			machineDefinition->statements.emplace(incomingStatementName, statement);
		} else {
			break;
		}
	}

	if (it->type != TokenType::CURLY_RIGHT) {
		throw UnexpectedTokenException(*it, "a statement or definition body opening bracket '}'", "for machine definition", *savedIt);
	}
	++it;

	return machineDefinition;
}

bool SyntacticAnalyzer::tryParseMachineFlag(std::list<Token>::const_iterator& it, std::map<MachineFlag, MachineDefinitionAttribute>& attributes) const {
	std::pair<MachineFlag, bool> setting;
	if (it->type == TokenType::KW_PRODUCTIONS_TERMINAL_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::ProductionsTerminalByDefault, true);
	} else if (it->type == TokenType::KW_PRODUCTIONS_NONTERMINAL_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::ProductionsTerminalByDefault, false);
	} else if (it->type == TokenType::KW_PRODUCTIONS_ROOT_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::ProductionsRootByDefault, true);
	} else if (it->type == TokenType::KW_PRODUCTIONS_NONROOT_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::ProductionsRootByDefault, false);
	} else if (it->type == TokenType::KW_CATEGORIES_ROOT_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::CategoriesRootByDefault, true);
	} else if (it->type == TokenType::KW_CATEGORIES_NONROOT_BY_DEFAULT) {
		setting = make_pair<MachineFlag, bool>(MachineFlag::CategoriesRootByDefault, false);
	} else {
		return false;
	}
	
	
	if (attributes[setting.first].set) {
		throw SyntacticAnalysisException("The attribute setting '" + it->string + "' in 'with' clause of a machine definition attempts to configure machine on an attribute that has already been explicitly set - check for repetitive or contradictory use of attribute settings", *it);
	} else {
		attributes[setting.first].set = false;
		attributes[setting.first].value = setting.second;
	}

	++it;
	return true;
}

std::unique_ptr<MachineDefinition> SyntacticAnalyzer::parseMachineType(std::list<Token>::const_iterator& it) const {
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

std::unique_ptr<MachineStatement> SyntacticAnalyzer::parseMachineStatement(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;

	Rootness rootness = Rootness::Unspecified;
	if (it->type == TokenType::KW_IGNORED) {
		++it;

		if (it->type != TokenType::KW_ROOT) {
			throw UnexpectedTokenException(*it, "keyword 'root'", "machine statement", *savedIt);
		}

		rootness = Rootness::IgnoreRoot;
		++it;
	} else if (it->type == TokenType::KW_ROOT) {
		++it;
		rootness = Rootness::AcceptRoot;
	}

	if (it->type == TokenType::KW_CATEGORY) {
		++it;
		return parseCategoryStatement(it, rootness);
	} else if (it->type == TokenType::KW_PATTERN) {
		if (rootness != Rootness::Unspecified) {
			throw UnexpectedTokenException(*it, "'category', 'production', or terminality [+ 'production']", "machine statement", *savedIt);
		}

		++it;
		return parsePatternStatement(it);
	} else if (it->type == TokenType::KW_REGEX) {
		if (rootness != Rootness::Unspecified) {
			throw UnexpectedTokenException(*it, "'category', 'production', or terminality [+ 'production']", "machine statement", *savedIt);
		}

		++it;
		return parseRegexStatement(it);
	} else {
		Terminality terminality = Terminality::Unspecified;
		if (it->type == TokenType::KW_TERMINAL) {
			++it;
			terminality = Terminality::Terminal;
		} else if (it->type == TokenType::KW_NONTERMINAL) {
			++it;
			terminality = Terminality::Nonterminal;
		}

		if(it->type == TokenType::KW_PRODUCTION) {
			++it;
		}

		if(it->type == TokenType::IDENTIFIER) {
			return parseProductionStatement(it, rootness, terminality);
		} else {
			if (rootness != Rootness::Unspecified) {
				throw UnexpectedTokenException(*it, "an identifier, or 'production' + an identifier, or 'category' + an identifier to follow the rootness specification", "machine statement", *savedIt);
			} else if (terminality != Terminality::Unspecified) {
				throw UnexpectedTokenException(*it, "an identifier, or 'production' + an identifier to follow the terminality specification", "machine statement", *savedIt);
			} else {
				it = savedIt;
				return nullptr;
			}
		}
	}
}

std::unique_ptr<CategoryStatement> SyntacticAnalyzer::parseCategoryStatement(std::list<Token>::const_iterator& it, Rootness rootness) const {
	auto savedIt = it;

	std::unique_ptr<CategoryStatement> categoryStatement = make_unique<CategoryStatement>();
	categoryStatement->copyLocation(*savedIt);
	categoryStatement->rootness = rootness;

	if (it->type != TokenType::IDENTIFIER) {
		throw UnexpectedTokenException(*it, "an identifier for the category name", "for category declaration", *savedIt);
	}
	categoryStatement->name = it->string;
	++it;

	// category is also an attributed statement
	parseInAttributedStatement(savedIt, it, *categoryStatement, "category");

	if (it->type != TokenType::OP_SEMICOLON) {
		throw UnexpectedTokenException(*it, "the terminal semicolon ';'", "for category definition", *savedIt);
	}
	++it;

	return categoryStatement;
}

void SyntacticAnalyzer::parseInAttributedStatement(std::list<Token>::const_iterator& productionStartIt, std::list<Token>::const_iterator& it, AttributedStatement& statement, const std::string& attributedStatementType) const {
	if (it->type == TokenType::OP_COLON) {
		++it;
		do {
			if (it->type != TokenType::IDENTIFIER) {
				throw UnexpectedTokenException(*it, "a " + attributedStatementType + " name identifier", "for inheritance in " + attributedStatementType + " declaration", *productionStartIt);
			}
			statement.categories.emplace(std::string(it->string), nullptr);
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
			lastDeclaration = SyntacticAnalyzer::parseMemberDeclaration(it);
			if (lastDeclaration) {
				statement.fields.push_back(std::move(lastDeclaration));
			} else {
				it = savedIt;
				break;
			}
		} while (true);

		if (it->type != TokenType::CURLY_RIGHT) {
			throw UnexpectedTokenException(*it, "a token for member declaration or the matching closing right curly bracket '}'", "for " + attributedStatementType + " definition", *productionStartIt);
		}
		++it;
	}
}

void SyntacticAnalyzer::parseInRuleStatement(std::list<Token>::const_iterator& productionStartIt, std::list<Token>::const_iterator& it, RuleStatement& statement, const std::string& attributedStatementType) const {
	if (it->type != TokenType::OP_EQUALS) {
		throw UnexpectedTokenException(*it, "'=' followed by a list of qualified names", "for " + attributedStatementType + " declaration", *productionStartIt);
	}
	++it;

	auto prevIt = it;
	std::shared_ptr<DisjunctiveRegex> disjunction = SyntacticAnalyzer::parseDisjunctiveRegex(it);
	if (!disjunction) {
		throw SyntacticAnalysisException("Expected an alternative (i.e. a valid, possibly disjunctive, astir regex) to follow the '=' on the intial token - none found.", *it, *prevIt);
	}
	statement.regex = move(disjunction);

	if (it->type != TokenType::OP_SEMICOLON) {
		throw UnexpectedTokenException(*it, "the terminal semicolon ';'", "for " + attributedStatementType + " definition", *productionStartIt);
	}
	++it;
}

std::unique_ptr<ProductionStatement> SyntacticAnalyzer::parseProductionStatement(std::list<Token>::const_iterator& it, Rootness rootness, Terminality terminality) const {
	auto savedIt = it;
	std::unique_ptr<ProductionStatement> productionStatement = make_unique<ProductionStatement>();
	productionStatement->copyLocation(*savedIt);
	productionStatement->rootness = rootness;
	productionStatement->terminality = terminality;

	if (it->type != TokenType::IDENTIFIER) {
		throw UnexpectedTokenException(*it, "an identifier to serve as production name", " for production declaration", *savedIt);
	}
	productionStatement->name = it->string;
	++it;

	// production is also an attributed statement
	parseInAttributedStatement(savedIt, it, *productionStatement, "production");

	// production is also a rule statement
	parseInRuleStatement(savedIt, it, *productionStatement, "production");

	return productionStatement;
}

std::unique_ptr<PatternStatement> SyntacticAnalyzer::parsePatternStatement(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	std::unique_ptr<PatternStatement> patternStatement = make_unique<PatternStatement>();
	patternStatement->copyLocation(*savedIt);

	if (it->type != TokenType::IDENTIFIER) {
		throw UnexpectedTokenException(*it, "an identifier to serve as pattern name", " for pattern declaration", *savedIt);
	}
	patternStatement->name = it->string;
	++it;

	// pattern is also an attributed statement
	parseInAttributedStatement(savedIt, it, *patternStatement, "pattern");

	// pattern is also a rule statement
	parseInRuleStatement(savedIt, it, *patternStatement, "pattern");

	return patternStatement;
}

std::unique_ptr<RegexStatement> SyntacticAnalyzer::parseRegexStatement(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	std::unique_ptr<RegexStatement> regexStatement = make_unique<RegexStatement>();
	regexStatement->copyLocation(*savedIt);

	if (it->type != TokenType::IDENTIFIER) {
		throw UnexpectedTokenException(*it, "an identifier to serve as regex name", " for regex declaration", *savedIt);
	}
	regexStatement->name = it->string;
	++it;

	// production is also a rule statement
	parseInRuleStatement(savedIt, it, *regexStatement, "regex");

	return regexStatement;
}

std::unique_ptr<Field> SyntacticAnalyzer::parseMemberDeclaration(std::list<Token>::const_iterator& it) const {
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

std::unique_ptr<RootRegex> SyntacticAnalyzer::parseRootRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;

	unique_ptr<RootRegex> rr;
	if ((rr = parseRepetitiveRegex(it)) || (rr = parseAtomicRegex(it))) {
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

			rr->actions.push_back(move(atp));
		}

		return rr;
	}  else {
		return nullptr;
	}
}

std::unique_ptr<RepetitiveRegex> SyntacticAnalyzer::parseRepetitiveRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	unique_ptr<AtomicRegex> atomicRegex = parseAtomicRegex(it);
	if (!atomicRegex) {
		// throw UnexpectedTokenException(*it, "token for action-atomic regex", "for repetitive regex as an alternative of root regex", *productionStartIt);
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
			throw SyntacticAnalysisException("The number for minimum repetitions in the regex is strictly larger than the number for maximum repetitions", *it, *savedIt);
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

std::unique_ptr<PrimitiveRegex> SyntacticAnalyzer::parsePrimitiveRegex(std::list<Token>::const_iterator& it) const {
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
		ret = make_unique<ArbitrarySymbolRegex>();
	} else if (it->type == TokenType::IDENTIFIER) {
		auto rr = make_unique<ReferenceRegex>();
		rr->referenceName = it->string;
		ret = move(rr);
	} else {
		return nullptr; // needs to be like this
	}
	++it;

	ret->copyLocation(*savedIt);

	return ret;
}

RegexActionType SyntacticAnalyzer::parseRegexAction(std::list<Token>::const_iterator& it) const {
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

std::unique_ptr<AtomicRegex> SyntacticAnalyzer::parseAtomicRegex(std::list<Token>::const_iterator& it) const {
	auto savedIt = it;
	if (it->type == TokenType::PAR_LEFT) {
		++it;

		if (it->type == TokenType::PAR_RIGHT) {
			++it;
			return std::make_unique<EmptyRegex>();
		}

		auto dr = parseDisjunctiveRegex(it);
		if (!dr) {
			throw UnexpectedTokenException(*it, "a beginning token for a disjunctive regex or an empty regex", "for atomic regex", *savedIt);
		}

		if (it->type != TokenType::PAR_RIGHT) {
			throw UnexpectedTokenException(*it, "a matching closing right parenthesis ')'", "for bracketed disjunctive regex as a part of atomic regex", *savedIt);
		}
		++it;

		return dr;
	}

	auto primitiveRegexPtr = SyntacticAnalyzer::parsePrimitiveRegex(it);
	if (!primitiveRegexPtr) {
		return nullptr;
	}

	return primitiveRegexPtr;
}

bool SyntacticAnalyzer::tryParseAnyRegex(std::list<Token>::const_iterator& it, std::unique_ptr<AnyRegex>& anyRegexPtr) const {
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
				throw SyntacticAnalysisException("In regex range capture, the end character was found to precede the beginning character", *it, *savedIt);
			}
			
			ar->ranges.push_back({ (CharType)firstString[0], (CharType)it->string[0] });
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

std::unique_ptr<ConjunctiveRegex> SyntacticAnalyzer::parseConjunctiveRegex(std::list<Token>::const_iterator& it) const {
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

std::unique_ptr<DisjunctiveRegex> SyntacticAnalyzer::parseDisjunctiveRegex(std::list<Token>::const_iterator& it) const {
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
