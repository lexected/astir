#pragma once

#include <memory>

#include "IFileLocalizable.h"
#include "ISyntacticEntity.h"

class ISemanticEntity {
public:
	virtual void initialize() {
		m_initialized = true;
	}

	bool initialized() const { return m_initialized; }
protected:
	ISemanticEntity()
		: m_initialized(false) { }
	virtual ~ISemanticEntity() = default;
private:
	bool m_initialized;
};