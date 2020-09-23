#pragma once

#include <istream>

#include "ProductionStream.h"
#include "Terminal.h"

class RawTerminal : public Terminal<char> {
public:
	RawTerminal()
		: Terminal<char>('\0', nullptr) { }
	//^ something distinguishably invalid (is it tho?), should never be referred to in practice if not initialized in a valid fashion

	RawTerminal(char c, const std::shared_ptr<Location>& occurenceLocation)
		: Terminal<char>(c, std::string({ c }), occurenceLocation) { }
};

class RawStream : public ProductionStream<RawTerminal> {
protected:
	RawStream(std::istream& underlyingStream, const std::shared_ptr<Location>& startingStreamLocation)
		: m_underlyingStream(underlyingStream), m_currentStreamLocation(startingStreamLocation), ProductionStream<RawTerminal>(startingStreamLocation) { }

	bool streamGet(std::shared_ptr<RawTerminal>& c) override;
	bool streamGood() const override;

private:
	std::istream& m_underlyingStream;

	std::shared_ptr<Location> m_currentStreamLocation;
};

class TextFileStream : public RawStream {
public:
	TextFileStream(const std::string& fileName, std::istream& underlyingStream)
		: RawStream(underlyingStream, std::make_shared<TextFileLocation>(fileName, 1, 0)) { }
};