#pragma once

#include <memory>
#include <string>

class Location {
public:
	virtual void note(char c) = 0;
	virtual void advance() = 0;
	virtual std::string toString() const = 0;

	virtual std::shared_ptr<Location> clone() const = 0;

protected:
	Location() = default;
};

class InvalidLocation : public Location {
public:
	void note(char c) override;
	void advance() override;
	std::string toString() const override;

	std::shared_ptr<Location> clone() const override;

	InvalidLocation() = default;
};

class TextLocation : public Location {
public:
	unsigned long line;
	unsigned long column;

	TextLocation()
		: line(1), column(0) { }

	TextLocation(unsigned long line, unsigned long column)
		: line(line), column(column) { }

	void note(char c) override;
	void advance() override;
	std::string toString() const override;
	std::shared_ptr<Location> clone() const override;
};

class TextFileLocation : public TextLocation {
public:
	std::string fileName;
	TextFileLocation()
		: TextLocation(0, 0), fileName() { }

	TextFileLocation(const std::string& fileName, unsigned long line, unsigned long column)
		: TextLocation(0, 0), fileName(fileName) { }

	std::string toString() const override;
	std::shared_ptr<Location> clone() const override;
};