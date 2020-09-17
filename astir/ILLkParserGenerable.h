#pragma once

class LLkParserGenerator;
class ILLkParserGenerable {
public:
	virtual void accept(LLkParserGenerator* generator) const = 0;

protected:
	ILLkParserGenerable() = default;
};

typedef const ILLkParserGenerable* ILLkParserGenerableCPtr;