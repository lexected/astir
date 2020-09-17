#include "IndentedStringStream.h"

void IndentedStringStream::put(const std::string& s) {
	*this << s;
}

void IndentedStringStream::putln(const std::string& s) {
	*this << s << std::endl;
}

void IndentedStringStream::increaseIndentation() {
	m_indentation += '\t';
}

void IndentedStringStream::decreaseIndentation() {
	m_indentation.resize(m_indentation.size() - 1);
}
