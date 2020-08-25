#pragma once

#include <memory>

#include "IFileLocalizable.h"
#include "ISyntacticEntity.h"

class ISemanticEntity : public IFileLocalizable {
public:
	virtual std::shared_ptr<const ISyntacticEntity> underlyingSyntacticEntity() const = 0;

	const FileLocation& location() const override {
		return underlyingSyntacticEntity()->location();
	}

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