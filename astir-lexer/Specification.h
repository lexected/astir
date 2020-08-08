#pragma once

#include <map>
#include <string>
#include <memory>
#include <list>

#include "Exception.h"
#include "SpecificationFile.h"

class SemanticAnalysisException : public Exception {
public:
	SemanticAnalysisException(const std::string& message)
		: Exception(message) { }
};

class Machine;
class Specification {
public:
	std::map<std::string, std::shared_ptr<Machine>> machines;

	void initializeFromFile(const SpecificationFile& specificationFile);
private:
	bool containsDefinitionRecursion(const std::map<std::string, MachineDefinition*>& definitions, std::list<std::string>& namesEncountered, std::string nameConsidered) const;
};

class Machine {
public:
	const std::string name;

	std::shared_ptr<Machine> follows;
	std::shared_ptr<Machine> extends;

	Machine(const std::string& name)
		: name(name) { }

	virtual void initializeFromDefinition(const MachineDefinition* definition);
};