#include "IndentedStringStream.h"

void IndentedStringStream::put(const std::string& s) {
	if (s.empty()) {
		return;
	}

	_put(s);
}

void IndentedStringStream::putln(const std::string& s) {
	if (s.empty()) {
		return;
	}

	_put(s);
	*this << std::endl;
}

void IndentedStringStream::newline() {
	*this << m_indentation << std::endl;
}

void IndentedStringStream::indent() {
	*this << m_indentation;
}

void IndentedStringStream::increaseIndentation() {
	m_indentation += '\t';
}

void IndentedStringStream::decreaseIndentation() {
	m_indentation.resize(m_indentation.size() - 1);
}

void IndentedStringStream::_put(const std::string& s) {
	size_t nextStop = 0;
	while (nextStop < s.length()) {
		size_t lastStop = nextStop;
		nextStop = s.find('\n', lastStop);
		if(nextStop != std::string::npos) {
			++nextStop;
			*this << m_indentation << s.substr(lastStop, nextStop-lastStop);
		} else {
			*this << m_indentation << s.substr(lastStop);
			break;
		}
	}
}
