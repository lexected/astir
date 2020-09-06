#pragma once

#include <istream>

#include "ProductionStream.h"
#include "Terminal.h"

class RawTerminal : public Terminal<char> {
public:
	RawTerminal(char c, const std::shared_ptr<Location>& occurenceLocation)
		: Terminal<char>(c, std::string({ c }), occurenceLocation) { }
};

class RawStream : public ProductionStream<RawTerminal> {
protected:
	RawStream(std::istream& underlyingStream, const std::shared_ptr<Location>& startingStreamLocation)
		: m_underlyingStream(underlyingStream), m_currentStreamLocation(startingStreamLocation), ProductionStream<RawTerminal>(startingStreamLocation) { }

	bool streamGet(RawTerminal& c) override;
	bool streamGood() override;

private:
	std::istream& m_underlyingStream;

	std::shared_ptr<Location> m_currentStreamLocation;
};

class TextFileStream : public RawStream {
public:
	TextFileStream(const std::string& fileName, std::istream& underlyingStream)
		: RawStream(underlyingStream, std::make_shared<TextFileLocation>(fileName, 1, 0)) { }
};