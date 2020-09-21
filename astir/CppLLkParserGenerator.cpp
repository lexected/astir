#include "CppLLkParserGenerator.h"
#include "GenerationException.h"

#include <algorithm>

CppLLkParserGenerator::CppLLkParserGenerator(LLkBuilder& builder)
	: LLkParserGenerator(builder) { }

void CppLLkParserGenerator::visitTypeFormingStatements(const std::list<std::shared_ptr<TypeFormingStatement>>& typeFormingStatements) {
	for (const auto& typeFormingStatementSPtr : typeFormingStatements) {
		auto tfsAsParserGenerable = std::dynamic_pointer_cast<ILLkParserGenerable>(typeFormingStatementSPtr);
		tfsAsParserGenerable->accept(this);
	}
}

void CppLLkParserGenerator::visitRootDisjunction(const std::list<std::shared_ptr<TypeFormingStatement>>& rootDisjunction) {
	m_declarations.push_back("std::shared_ptr<OutputProduction> parse_root(InputStream& is);");

	if (rootDisjunction.empty()) {
		throw GenerationException("A machine with no root productions can not be generated");
	}

	// definition preamble
	m_output.put("std::shared_ptr<OutputProduction> ");
	m_output << m_builder.contextMachine()->name << "::parse_root(InputStream & is) {" << std::endl;
	m_output.increaseIndentation();
	m_output.putln("auto productionStartLocation = is.peek(0)->location()->clone();");
	m_output.putln("size_t cumulativePeekCorrection = 0;");

	// core
	std::vector<LLkDecisionPoint> decisionPoints;
	decisionPoints.reserve(rootDisjunction.size());

	bool isFirst = true;
	m_output.put(""); // to indent
	for (const auto& tfsSPtr : rootDisjunction) {
		decisionPoints.push_back(m_builder.getDecisionTree(tfsSPtr.get()));
		if (isFirst) {
			isFirst = false;
		} else {
			m_output << " else ";
		}

		// if header
		m_output << "if(";
		m_output << makeConditionTesting(decisionPoints.back());
		m_output << ") {" << std::endl;
		m_output.increaseIndentation();

		// core handling
		if (tfsSPtr->rootness == Rootness::IgnoreRoot) {
			m_output.putln("parse_" + tfsSPtr->name + "(is);");
			m_output.putln("return nullptr;");
		} else if (tfsSPtr->rootness == Rootness::AcceptRoot) {
			m_output.putln("return parse_" + tfsSPtr->name + "(is);");
		}

		// if footer
		m_output.decreaseIndentation();
		m_output.put("}");
	}

	// else error
	m_output << " else {" << std::endl;
	m_output.increaseIndentation();
	m_output.putln("error(\"" + makeExpectationMessage(decisionPoints) + "\");");
	m_output.putln("return nullptr; // to suppress the warning");
	m_output.decreaseIndentation();
	m_output.putln("}");

	// definition postamble
	handleTypeFormingPostamble();
}

void CppLLkParserGenerator::visit(const CategoryStatement* category) {
	m_declarations.push_back("std::shared_ptr<" + category->name + "> parse_" + category->name + "(InputStream& is);");

	// definition preamble
	handleTypeFormingPreamble(category->name);

	// core
	if (category->references.empty()) {
		m_output.putln("return std::make_shared<" + category->name + ">(productionStartLocation);");
	} else {
		std::vector<LLkDecisionPoint> decisionPoints;
		decisionPoints.reserve(category->references.size());

		bool isFirst = true;
		m_output.put(""); // to indent
		for (const auto& categoryReferencePair : category->references) {
			decisionPoints.push_back(m_builder.getDecisionTree(categoryReferencePair.second.statement));
			if (isFirst) {
				isFirst = false;
			} else {
				m_output << " else ";
			}

			// if header
			m_output << "if(";
			m_output << makeConditionTesting(decisionPoints.back());
			m_output << ") {" << std::endl;
			m_output.increaseIndentation();

			// core handling
			m_output.putln("return parse_" + categoryReferencePair.first + "(is);");

			// if footer
			m_output.decreaseIndentation();
			m_output.put("}");
		}

		// else error
		m_output << " else {" << std::endl;
		m_output.increaseIndentation();
		m_output.putln("error(\"" + makeExpectationMessage(decisionPoints) + "\");");
		m_output.decreaseIndentation();
		m_output.putln("}");
	}

	// definition postamble
	m_output.putln("");
	m_output.putln("return nullptr; // to suppress the warning");
	handleTypeFormingPostamble();
}

void CppLLkParserGenerator::visit(const PatternStatement* rule) {
	handleRuleBody(rule);
}

void CppLLkParserGenerator::visit(const ProductionStatement* production) {
	m_declarations.push_back("std::shared_ptr<" + production->name + "> parse_" + production->name + "(InputStream& is);");

	// definition preamble
	handleTypeFormingPreamble(production->name);

	handleRuleBody(production);

	m_output.putln("return std::make_shared<" + production->name + ">(productionStartLocation);");

	handleTypeFormingPostamble();
}

void CppLLkParserGenerator::visit(const RegexStatement* rule) {
	handleRuleBody(rule);
}

void CppLLkParserGenerator::visit(const DisjunctiveRegex* regex) {
	std::vector<LLkDecisionPoint> decisionPoints;
	decisionPoints.reserve(regex->disjunction.size());

	// core
	bool isFirst = true;
	m_output.put(""); // to indent
	for (const auto& conjunctiveRegexPtr : regex->disjunction) {
		decisionPoints.push_back(m_builder.getDecisionTree(conjunctiveRegexPtr.get()));
		if (isFirst) {
			isFirst = false;
		} else {
			m_output << " else ";
		}

		// if header
		m_output << "if(";
		m_output << makeConditionTesting(decisionPoints.back());
		m_output << ") {" << std::endl;
		m_output.increaseIndentation();

		// core handling
		conjunctiveRegexPtr->accept(this);

		// if footer
		m_output.decreaseIndentation();
		m_output.put("}");
	}

	// else error
	m_output << " else {" << std::endl;
	m_output.increaseIndentation();
	m_output.putln("error(\"" + makeExpectationMessage(decisionPoints) + "\");");
	m_output.decreaseIndentation();
	m_output.putln("}");
}

void CppLLkParserGenerator::visit(const ConjunctiveRegex* regex) {
	const auto end = regex->conjunction.cend();
	const auto begin = regex->conjunction.cbegin();
	for (auto it = begin; it != end; ++it) {
		if (it != begin) {
			m_output.putln("");
		}

		const auto& rootRegexPtr = *it;
		ILLkFirstableCPtr rootRegexAsFirstablePtr = dynamic_cast<ILLkFirstableCPtr>(rootRegexPtr.get());
		auto decisionPoint = m_builder.getDecisionTree(rootRegexAsFirstablePtr);

		// if header
		m_output.put("if(");
		m_output << makeConditionTesting(decisionPoint);
		m_output << ") {" << std::endl;
		m_output.increaseIndentation();

		// core handling
		ILLkParserGenerableCPtr asGenerable = dynamic_cast<ILLkParserGenerableCPtr>(rootRegexPtr.get());
		asGenerable->accept(this);

		// if footer
		m_output.decreaseIndentation();
		m_output.putln("} else {");
		m_output.increaseIndentation();
		m_output.putln("error(\""+makeExpectationMessage(decisionPoint)+"\");");
		m_output.decreaseIndentation();
		m_output.putln("}");
	}
}

void CppLLkParserGenerator::visit(const RepetitiveRegex* regex) {
	ILLkParserGenerableCPtr repeatedRegexAsGenerablePtr = dynamic_cast<ILLkParserGenerableCPtr>(regex->regex.get());
	ILLkFirstableCPtr repeatedRegexAsFirstablePtr = dynamic_cast<ILLkFirstableCPtr>(regex->regex.get());
	auto decisionPoint = m_builder.getDecisionTree(repeatedRegexAsFirstablePtr);

	for (unsigned long it = 0; it < regex->minRepetitions; ++it) {
		// if header
		m_output.put("if(");
		m_output << makeConditionTesting(decisionPoint);
		m_output << ") {" << std::endl;
		m_output.increaseIndentation();

		// core handling
		repeatedRegexAsGenerablePtr->accept(this);

		// if footer
		m_output.decreaseIndentation();
		m_output.putln("} else {");
		m_output.increaseIndentation();
		m_output.putln("error(\"" + makeExpectationMessage(decisionPoint) + "\");");
		m_output.decreaseIndentation();
		m_output.putln("}");
	}

	if (regex->minRepetitions == regex->maxRepetitions) {
		return;
	}
	if (regex->minRepetitions > 0) {
		m_output.putln(""); // nothing to see here, just some formatting :)
	}

	if (regex->maxRepetitions != regex->INFINITE_REPETITIONS) {
		m_output.putln("{");
		m_output.increaseIndentation();
		m_output.putln("unsigned long counter = " + std::to_string(regex->maxRepetitions - regex->minRepetitions) + ";");
		m_output.put("while(counter > 0 && (");
		m_output << makeConditionTesting(decisionPoint);
		m_output << ")) {" << std::endl;
		m_output.increaseIndentation(); 
		repeatedRegexAsGenerablePtr->accept(this);
		m_output.putln("--counter;");
		m_output.decreaseIndentation();
		m_output.putln("}");
		m_output.decreaseIndentation();
		m_output.putln("}");
	} else {
		m_output.put("while(");
		m_output << makeConditionTesting(decisionPoint);
		m_output << ") {" << std::endl;
		m_output.increaseIndentation();
		repeatedRegexAsGenerablePtr->accept(this);
		m_output.decreaseIndentation();
		m_output.putln("}");
	}
}

void CppLLkParserGenerator::visit(const EmptyRegex* regex) {
	// do nothing, or at least until the actions come into play
}

void CppLLkParserGenerator::visit(const AnyRegex* regex) {
	m_output.putln("is.consume();");
}

void CppLLkParserGenerator::visit(const ExceptAnyRegex* regex) {
	m_output.putln("is.consume();");
}

void CppLLkParserGenerator::visit(const LiteralRegex* regex) {
	m_output.putln("is.consume();");
}

void CppLLkParserGenerator::visit(const ArbitrarySymbolRegex* regex) {
	m_output.putln("is.consume();"); // even with uses machines, this is the intended and correct behaviour!!
}

void CppLLkParserGenerator::visit(const ReferenceRegex* regex) {
	if (regex->referenceStatementMachine == m_builder.contextMachine()) {
		m_output.putln("parse_" + regex->referenceName + "(is);");
	} else if (regex->referenceStatementMachine != m_builder.contextMachine()->on.second.get()) {
		// i.e. if this comes from a "uses" machine
		m_output.putln("m_" + regex->referenceStatementMachine->name + ".consume(is);");
	} else {
		// it comes from an "on" machine
		m_output.putln("is.consume();");
	}
}

std::string CppLLkParserGenerator::parsingDeclarations() const {
	std::stringstream ss;
	for (const auto& declarationString : m_declarations) {
		ss << declarationString << std::endl;
	}

	return ss.str();
}

void CppLLkParserGenerator::handleTypeFormingPreamble(const std::string& typeName) {
	m_output.put("std::shared_ptr<");
	m_output << typeName << "> " << m_builder.contextMachine()->name << "::parse_" << typeName << "(InputStream& is) {" << std::endl;
	m_output.increaseIndentation();
	m_output.putln("auto productionStartLocation = is.peek(0)->location()->clone();");
	m_output.putln("size_t cumulativePeekCorrection = 0;");
}

void CppLLkParserGenerator::handleTypeFormingPostamble() {
	m_output.decreaseIndentation();
	m_output.putln("}");
	m_output.putln("");
}

void CppLLkParserGenerator::handleRuleBody(const RuleStatement* rule) {
	/*m_output.put("if(");
	m_output << makeConditionTesting(m_builder.getDecisionTree(rule));
	m_output << ") {" << std::endl;
	m_output.increaseIndentation();*/

	rule->regex->accept(this);

	/*m_output.decreaseIndentation();
	m_output.putln("} else {");
	m_output.increaseIndentation();
	m_output.putln("error();");
	m_output.decreaseIndentation();
	m_output.putln("}");*/
}

std::string CppLLkParserGenerator::makeConditionTesting(const LLkDecisionPoint& dp, unsigned long depth, bool needsUnpeeking) const {
	std::stringstream output;
	if (dp.transitions.empty()) {
		return "true";
	}

	
	for (auto it = dp.transitions.cbegin(); it != dp.transitions.cend(); ++it) {
		if (it != dp.transitions.cbegin()) {
			output << " || ";
		}

		const auto& transitionPtr = *it;
		const bool doesThisConditionHaveContinuation = !transitionPtr->point.transitions.empty();

		std::string unpeekingPostamble;
		auto condition = makeCondition(transitionPtr->condition, unpeekingPostamble, depth);
		const bool doesThisOneNeedUnpeeking = !unpeekingPostamble.empty();
		const bool needsUnpeekingDisambiguationParenthesisStructureAroundCondition = doesThisOneNeedUnpeeking && doesThisConditionHaveContinuation;
		if (needsUnpeekingDisambiguationParenthesisStructureAroundCondition) {
			output << '(';
		}
		output << condition;

		if(doesThisConditionHaveContinuation) {
			output << " && ";
			const bool needParentheses = transitionPtr->point.transitions.size() > 1;
			if (needParentheses) {
				output << '(';
			}
			
			output << makeConditionTesting(transitionPtr->point, depth+(doesThisOneNeedUnpeeking ? 0 : 1), doesThisOneNeedUnpeeking || needsUnpeeking);
			
			if (needParentheses) {
				output << ')';
			}
		} else if (doesThisOneNeedUnpeeking || needsUnpeeking) {
			output << " && !(cumulativePeekCorrection = 0)";
		}

		if (needsUnpeekingDisambiguationParenthesisStructureAroundCondition) {
			output << " || " << unpeekingPostamble << ')';
		}
	}
	
	return output.str();
}

std::string CppLLkParserGenerator::makeCondition(const std::shared_ptr<SymbolGroup>& sgPtr, std::string& postamble, unsigned long depth) const {
	std::stringstream output;
	const SymbolGroup* rawPtr = sgPtr.get();

	const ByteSymbolGroup* bytePtr = dynamic_cast<const ByteSymbolGroup*>(rawPtr);
	if (bytePtr != nullptr) {
		output << "is.peek(cumulativePeekCorrection+" << depth << ")->raw.length() == 1 && is.peek(cumulativePeekCorrection+" << depth << ")->raw[0] >= " << bytePtr->rangeStart << " && is.peek(cumulativePeekCorrection+" << depth << ")->raw[0] <= " << bytePtr->rangeEnd;
		return output.str();
	}

	const LiteralSymbolGroup* literalPtr = dynamic_cast<const LiteralSymbolGroup*>(rawPtr);
	if (literalPtr != nullptr) {
		output << "is.peek(cumulativePeekCorrection+" << depth << ")->raw == \"" << literalPtr->literal << "\"";
		return output.str();
	}

	const StatementSymbolGroup* ssgPtr = dynamic_cast<const StatementSymbolGroup*>(rawPtr);
	if (ssgPtr != nullptr) {
		if (ssgPtr->statementMachine == this->m_builder.contextMachine()->on.second.get()) {
			// the input comes from "on"
			output << "std::dynamic_pointer_cast<" << (ssgPtr->statementMachine != m_builder.contextMachine() ? ssgPtr->statementMachine->name + "::" : "") << ssgPtr->statement->name << ">(is.peek(cumulativePeekCorrection+" << depth << "))";
		} else {
			// the input comes from an "uses" machine
			output << "m_" << ssgPtr->statementMachine->name << ".peekCast<";
			output << (ssgPtr->statementMachine != m_builder.contextMachine() ? ssgPtr->statementMachine->name + "::" : "");
			output << ssgPtr->statement->name << ">(is, cumulativePeekCorrection+" << depth << ", cumulativePeekCorrection)";
			postamble = "m_" + ssgPtr->statementMachine->name + ".unpeekIfApplicable(is, cumulativePeekCorrection)";
		}

		return output.str();
	}

	throw GenerationException("Unknown symbol group encountered");
}


std::string CppLLkParserGenerator::makeExpectationMessage(const LLkDecisionPoint& dp) {
	return makeExpectationMessage(std::vector<LLkDecisionPoint>({ dp }));
}

std::string CppLLkParserGenerator::makeExpectationMessage(const std::vector<LLkDecisionPoint>& dps) {
	size_t maxDepth = 0;
	for (const auto& dp : dps) {
		maxDepth = std::max(maxDepth, dp.maxDepth());
	}

	std::stringstream ss;
	ss << "Unexpected token ";

	if (maxDepth > 1) {
		ss << "sequence ";
	}
	ss << "\\\"\"";
	for (size_t it = 0; it < maxDepth; ++it) {
		if (it > 0) {
			ss << " + \" \"";
		}
		ss << " + is.peek(" << it << ")->stringForError()";
	}
	ss << " + \"\\\" encountered at \" + is.peek(0)->locationString() + \"; expected ";
	for (auto dpIt = dps.cbegin(); dpIt != dps.cend();++dpIt) {
		if (dpIt != dps.cbegin()) {
			ss << ", or ";
		}
		ss << "\\\"";
		ss << makeExpectationGrammar(*dpIt);
		ss << "\\\"";
	}

	return ss.str();
}

std::string CppLLkParserGenerator::makeExpectationGrammar(const LLkDecisionPoint& dp) {
	if (dp.transitions.empty()) {
		return "nothing";
	}

	std::stringstream ss;
	if (dp.transitions.size() > 1) {
		ss << "(";
	}

	for (auto it = dp.transitions.cbegin(); it != dp.transitions.cend(); ++it) {
		if (it != dp.transitions.cbegin()) {
			ss << " | ";
		}

		const auto& transition = *it;
		const SymbolGroup* rawPtr = transition->condition.get();
		const LiteralSymbolGroup* literalPtr = dynamic_cast<const LiteralSymbolGroup*>(rawPtr);
		if (literalPtr != nullptr) {
			ss << "'" << literalPtr->literal << "'";
		} else {
			const StatementSymbolGroup* ssgPtr = dynamic_cast<const StatementSymbolGroup*>(rawPtr);
			if (ssgPtr != nullptr) {
				ss << ssgPtr->statement->name;
			} else {
				throw GenerationException("Unknown symbol group encountered");
			}
		}

		if (!transition->point.transitions.empty()) {
			ss << ' ';
			ss << makeExpectationGrammar(transition->point);
		}
	}

	if (dp.transitions.size() > 1) {
		ss << ")";
	}

	return ss.str();
}
