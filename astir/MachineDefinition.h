#pragma once

#include "ISyntacticEntity.h"
#include "ISemanticEntity.h"
#include "IGenerationVisitable.h"

#include "MachineStatement.h"

#include <string>
#include <memory>
#include <map>
#include <list>

enum class MachineFlag {
	ProductionsTerminalByDefault,
	ProductionsRootByDefault,
	CategoriesRootByDefault,
	AmbiguityResolvedByPrecedence
};

struct MachineDefinitionAttribute {
	bool set;
	bool value;

	MachineDefinitionAttribute()
		: set(false), value(false) { }
	MachineDefinitionAttribute(bool value)
		: set(false), value(value) { }
};

struct MachineDefinition : public ISyntacticEntity, public ISemanticEntity, public IGenerationVisitable {
public:
	std::string name;
	std::map<MachineFlag, MachineDefinitionAttribute> attributes;
	std::map<std::string, std::shared_ptr<MachineDefinition>> uses;
	std::pair<std::string, std::shared_ptr<MachineDefinition>> on;
	std::map<std::string, std::shared_ptr<MachineStatement>> statements;

	void initialize() override;

	std::shared_ptr<MachineStatement> findMachineStatement(const std::string& name, const MachineDefinition** sourceMachine = nullptr) const; // anyone calling this function shall not take up even a partial ownership of the component, normal pointer suffices
	std::list<std::shared_ptr<ProductionStatement>> getTerminalProductions() const;
	std::list<std::shared_ptr<TypeFormingStatement>> getTypeFormingStatements() const;
	bool hasPurelyTerminalRoots() const;
	std::list<std::shared_ptr<TypeFormingStatement>> getRoots() const;
	std::list<const ProductionStatement*> getUnderlyingProductionsOfRoots() const;
	std::list<std::shared_ptr<ProductionStatement>> getTerminalRoots() const;
	TerminalTypeIndex terminalProductionCount() const { return m_terminalCount; };

	void completeCategoryReferences(std::list<std::string> namesEncountered, const std::shared_ptr<AttributedStatement>& attributedStatement, bool mustBeACategory = false) const;

	SymbolGroupList computeArbitrarySymbolGroupList() const;
	bool isOnTerminalInput() const { return m_isOnTerminalInput; }

protected:
	MachineDefinition()
		: attributes({
			{ MachineFlag::ProductionsTerminalByDefault, MachineDefinitionAttribute(false) },
			{ MachineFlag::ProductionsRootByDefault, MachineDefinitionAttribute(false) },
			{ MachineFlag::CategoriesRootByDefault, MachineDefinitionAttribute(false) },
			{ MachineFlag::AmbiguityResolvedByPrecedence, MachineDefinitionAttribute(false) }
			}), m_terminalCount((TerminalTypeIndex)0), m_isOnTerminalInput(false) { }

	MachineDefinition(const std::map<MachineFlag, MachineDefinitionAttribute>& attributes);

private:
	TerminalTypeIndex m_terminalCount;

	void mergeInAttributes(const std::map<MachineFlag, MachineDefinitionAttribute>& attributes);
	bool m_isOnTerminalInput;
};
