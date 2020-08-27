#pragma once

#include <istream>
#include <deque>
#include <memory>
#include <string>

class RawStreamLocation {
public:
	virtual void note(char c) = 0;
	virtual void advance() = 0;
	virtual std::string toString() const = 0;

	virtual std::shared_ptr<RawStreamLocation> clone() const = 0;
protected:
	RawStreamLocation() = default;
};

class TextLocation : public RawStreamLocation {
public:
	unsigned long line;
	unsigned long column;

	TextLocation()
		: line(0), column(0) { }

	TextLocation(unsigned long line, unsigned long column)
		: line(line), column(column) { }

	void note(char c) override;
	void advance() override;
	std::string toString() const override;
	std::shared_ptr<RawStreamLocation> clone() const override;
};

class TextFileLocation : public TextLocation {
public:
	std::string fileName;
	TextFileLocation()
		: TextLocation(0, 0), fileName() { }

	TextFileLocation(const std::string& fileName, unsigned long line, unsigned long column)
		: TextLocation(0, 0), fileName(fileName) { }

	std::string toString() const override;
	std::shared_ptr<RawStreamLocation> clone() const override;
};

class IRawStreamLocalizable {
public:
	virtual const std::shared_ptr<RawStreamLocation>& location() const = 0;
	std::string locationString() const { return location()->toString(); }

protected:
	IRawStreamLocalizable() = default;
	virtual ~IRawStreamLocalizable() = default;
};

class RawStream {
public:
	bool get(char& c);
	bool good() const;

	void pin();
	void resetToPin();
	void unpin();

	const std::shared_ptr<RawStreamLocation>& pinLocation() const { return m_pinLocation; }
	const std::shared_ptr<RawStreamLocation>& currentLocation() const { return m_currentStreamLocation; }
protected:
	RawStream(std::istream& underlyingStream, const std::shared_ptr<RawStreamLocation>& startingStreamLocation)
		: m_underlyingStream(underlyingStream), m_buffer(), m_nextByteToGive(0), m_currentStreamLocation(startingStreamLocation) { }

private:
	std::istream& m_underlyingStream;
	size_t m_nextByteToGive;
	std::deque<char> m_buffer;

	std::shared_ptr<RawStreamLocation> m_currentStreamLocation;
	std::shared_ptr<RawStreamLocation> m_pinLocation;
};

class TextFileStream : public RawStream {
public:
	TextFileStream(const std::string& fileName, std::istream& underlyingStream)
		: RawStream(underlyingStream, std::make_shared<TextFileLocation>(fileName, 1, 0)) { }
};