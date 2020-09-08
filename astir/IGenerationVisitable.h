#pragma once

class GenerationVisitor;

class IGenerationVisitable {
public:
	virtual void accept(GenerationVisitor* visitor) const = 0;

protected:
	IGenerationVisitable() = default;
};