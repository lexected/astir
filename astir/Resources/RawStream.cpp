#include "ProductionStream.h"
#include "RawStream.h"

bool RawStream::streamGet(std::shared_ptr<RawTerminal>& c) {
    char payload;
    bool ret = bool{ m_underlyingStream.get(payload) };

    if (ret) {
        m_currentStreamLocation->note(payload);
        c = std::make_shared<RawTerminal>(payload, m_currentStreamLocation->clone());
    }

    return ret;
}

bool RawStream::streamGood() const {
    return m_underlyingStream.good();
}

TextFileStream::TextFileStream(const std::string& fileName)
    : m_fileStream(fileName), RawStream(m_fileStream, std::make_shared<TextFileLocation>(fileName, 1, 0)) { }
