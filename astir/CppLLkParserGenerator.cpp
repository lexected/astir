#include "CppLLkParserGenerator.h"
#include "GenerationException.h"

CppLLkParserGenerator::CppLLkParserGenerator(LLkBuilder& builder)
	: LLkParserGenerator(builder) { }

void CppLLkParserGenerator::visit(const CategoryStatement* category) {
	m_declarations.push_back("std::shared_ptr<" + category->name + "> parse_" + category->name + "(InputStream& is) const;");

	// TODO: Handle the case when category.references.empty() == true!!!

	// definition preamble
	m_output.put("std::shared_ptr<");
	m_output << category->name << "> " << m_builder.contextMachine().name << "::parse_" << category->name << "(InputStream& is) const {" << std::endl;
	m_output.increaseIndentation();

	// core
	bool isFirst = true;
	m_output.put(""); // to indent
	for (const auto& categoryReferencePair : category->references) {
		auto decisionPoint = m_builder.getDecisionTree(categoryReferencePair.second.statement);
		if (isFirst) {
			isFirst = false;
		} else {
			m_output << " else ";
		}
		
		// if header
		m_output << "if(";
		outputConditionTesting(decisionPoint);
		m_output << ") {";
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
	m_output.putln("error();");
	m_output.decreaseIndentation();
	m_output.putln("}");

	// definition postamble
	m_output.decreaseIndentation();
	m_output.putln("}");
}

void CppLLkParserGenerator::visit(const PatternStatement* rule) {
	handleRuleBody(rule);
}

void CppLLkParserGenerator::visit(const ProductionStatement* production) {
	m_declarations.push_back("std::shared_ptr<" + production->name + "> parse_" + production->name + "(InputStream& is) const;");

	// definition preamble
	m_output.put("std::shared_ptr<");
	m_output << production->name << "> " << m_builder.contextMachine().name << "::parse_" << production->name << "(InputStream& is) const {" << std::endl;
	m_output.increaseIndentation();

	handleRuleBody(production);

	m_output.decreaseIndentation();
	// else error
	m_output.putln("} else {");
	m_output.increaseIndentation();
	m_output.putln("error();");
	m_output.decreaseIndentation();
	m_output.putln("}");

	// definition postamble
	m_output.decreaseIndentation();
	m_output.putln("}");
}

void CppLLkParserGenerator::visit(const RegexStatement* rule) {
	handleRuleBody(rule);
}

void CppLLkParserGenerator::visit(const DisjunctiveRegex* regex) {
	// core
	bool isFirst = true;
	m_output.put(""); // to indent
	for (const auto& conjunctiveRegexPtr : regex->disjunction) {
		auto decisionPoint = m_builder.getDecisionTree(conjunctiveRegexPtr.get());
		if (isFirst) {
			isFirst = false;
		} else {
			m_output << " else ";
		}

		// if header
		m_output << "if(";
		outputConditionTesting(decisionPoint);
		m_output << ") {";
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
	m_output.putln("error();");
	m_output.decreaseIndentation();
	m_output.putln("}");
}

void CppLLkParserGenerator::visit(const ConjunctiveRegex* regex) {
	for (const auto& rootRegexPtr : regex->conjunction) {
		ILLkFirstableCPtr rootRegexAsFirstablePtr = dynamic_cast<ILLkFirstableCPtr>(rootRegexPtr.get());
		auto decisionPoint = m_builder.getDecisionTree(rootRegexAsFirstablePtr);

		// if header
		m_output << "if(";
		outputConditionTesting(decisionPoint);
		m_output << ") {";
		m_output.increaseIndentation();

		// core handling
		ILLkParserGenerableCPtr asGenerable = dynamic_cast<ILLkParserGenerableCPtr>(rootRegexPtr.get());
		asGenerable->accept(this);

		// if footer
		m_output.decreaseIndentation();
		m_output.putln("} else {");
		m_output.increaseIndentation();
		m_output.putln("error();");
		m_output.decreaseIndentation();
		m_output.putln("}");
	}
}

void CppLLkParserGenerator::visit(const RepetitiveRegex* regex) {
	ILLkFirstableCPtr repRegexAsFirstablePtr = dynamic_cast<ILLkFirstableCPtr>(regex);
	auto decisionPoint = m_builder.getDecisionTree(repRegexAsFirstablePtr);

	//TODO: repetitive core
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
	m_output.putln("is.consume();");
}

void CppLLkParserGenerator::visit(const ReferenceRegex* regex) {
	if (regex->referenceStatementMachine == &m_builder.contextMachine()) {
		m_output.putln("parse_" + regex->referenceName + "(is);");
	} else if (regex->referenceStatementMachine != m_builder.contextMachine().on.second.get()) {
		// i.e. if this comes from a "uses" machine
		// TODO
	} else {
		// it comes from an "on" machine
		m_output.putln("is.consume();");
	}
}

void CppLLkParserGenerator::handleRuleBody(const RuleStatement* rule) {
	m_output.put("if(");
	outputConditionTesting(m_builder.getDecisionTree(rule));
	m_output << ") {" << std::endl;
	m_output.increaseIndentation();

	rule->regex->accept(this);

	m_output.decreaseIndentation();
	m_output.putln("} else {");
	m_output.increaseIndentation();
	m_output.putln("error();");
	m_output.decreaseIndentation();
	m_output.putln("}");
}

void CppLLkParserGenerator::outputConditionTesting(const LLkDecisionPoint& dp, unsigned long depth) {
	if (dp.transitions.empty()) {
		m_output << "true";
		return;
	}

	for (auto it = dp.transitions.cbegin(); it != dp.transitions.cend();++it) {
		if (it != dp.transitions.cbegin()) {
			m_output << " || ";
		}

		const auto& transitionPtr = *it;
		outputCondition(transitionPtr->condition, depth);
		m_output << " && (";
		outputConditionTesting(transitionPtr->point, depth+1);
		m_output << ")";
	}
}

void CppLLkParserGenerator::outputCondition(const std::shared_ptr<SymbolGroup>& sgPtr, unsigned long depth) {
	const SymbolGroup* rawPtr = sgPtr.get();
	
	const LiteralSymbolGroup* literalPtr = dynamic_cast<const LiteralSymbolGroup*>(rawPtr);
	if (literalPtr != nullptr) {
		m_output << "is.peek(" << depth << ")->raw() == " << literalPtr->literal;
		return;
	}

	const StatementSymbolGroup* ssgPtr = dynamic_cast<const StatementSymbolGroup*>(rawPtr);
	if (ssgPtr != nullptr) {
		m_output << "dynamic_cast<" << ssgPtr->statement->name << ">(is.peek(" << depth << ")) != nullptr";
		return;
	}

	throw GenerationException("Unknown symbol group encountered");
}
