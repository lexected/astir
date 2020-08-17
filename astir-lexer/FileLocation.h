#pragma once

#include <string>

struct FileLocation {
	unsigned long line;
	unsigned long column;

	FileLocation()
		: line(0), column(0) { }

	FileLocation(unsigned long line, unsigned long column)
		: line(line), column(column) { }

	std::string toString() const {
		return std::to_string(line) + ":" + std::to_string(column);
	}
};

class IFileLocalizable {
public:
	const FileLocation& location() const { return m_location; }
	std::string locationString() const { return m_location.toString(); }

	void copyLocation(const IFileLocalizable& anotherLocalizableThingy) {
		m_location = anotherLocalizableThingy.m_location;
	}
protected:
	IFileLocalizable() = default;
	IFileLocalizable(const FileLocation& location)
		: m_location(location) { }
	IFileLocalizable(unsigned long line, unsigned long column)
		: m_location(line, column) { }

	void setLocation(const FileLocation& location) {
		m_location = location;
	}
	void setLocation(unsigned long line, unsigned long column) {
		m_location.line = line;
		m_location.column = column;
	}

	// virtual ~ILocalizable() = default; // commented out because it does not appear to be strictly needed
private:
	FileLocation m_location;
};