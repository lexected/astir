#include "RawStream.h"

bool RawStream::get(char& c) {
    if (m_nextByteToGive < m_buffer.size()) {
        c = m_buffer[m_nextByteToGive];
        ++m_nextByteToGive;
        m_currentStreamLocation->note(c);

        return true;
    } else {
        bool ret = bool{ m_underlyingStream.get(c) };

        if (ret) {
            m_buffer.push_back(c);
            ++m_nextByteToGive;
            m_currentStreamLocation->note(c);
        }

        return ret;
    }
}

bool RawStream::good() const {
    return m_nextByteToGive < m_buffer.size() || m_underlyingStream.good();
}

void RawStream::pin() {
    if (m_nextByteToGive > 0) {
        m_buffer.erase(m_buffer.cbegin(), m_buffer.cbegin() + m_nextByteToGive);
        m_nextByteToGive = 0;
    }

    m_pinLocation = m_currentStreamLocation;
}

std::string RawStream::rawSincePin() const {
    std::string ret(m_buffer.cbegin(), m_buffer.cbegin() + m_nextByteToGive);
    return ret;
}

void RawStream::resetToPin() {
    m_nextByteToGive = 0;
    m_currentStreamLocation = m_pinLocation;
}

void RawStream::unpin() {
    m_buffer.clear();
    m_nextByteToGive = 0;
    m_pinLocation = nullptr;
}

size_t RawStream::currentPosition() const {
    return m_nextByteToGive;
}

void RawStream::resetToPosition(size_t newPosition) {
    m_nextByteToGive = newPosition;

    m_currentStreamLocation = m_pinLocation->clone();
    for (size_t it = 0; it < newPosition; ++it) {
        m_currentStreamLocation->note(m_buffer[it]);
    }
}

void TextLocation::note(char c) {
    ++column;
    if(c == '\n') {
        ++line;
        column = 0;
    }
}

void TextLocation::advance() {
    ++column;
}

std::string TextLocation::toString() const {
    return std::to_string(line) + ":" + std::to_string(column);
}

std::shared_ptr<RawStreamLocation> TextLocation::clone() const {
    return std::make_shared<TextLocation>(*this);
}

std::string TextFileLocation::toString() const {
    return fileName + ":" + this->TextLocation::toString();
}

std::shared_ptr<RawStreamLocation> TextFileLocation::clone() const {
    return std::make_shared<TextFileLocation>(*this);
}
