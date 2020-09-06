#include "ProductionStream.h"
#include "RawStream.h"

bool RawStream::streamGet(std::shared_ptr<RawTerminal>& c) {
    char payload;
    bool ret = bool{ m_underlyingStream.get(payload) };

    if (ret) {
        m_currentStreamLocation->note(payload);
        c = std::make_shared<RawTerminal>(payload, m_currentStreamLocation);
        m_currentStreamLocation->note(payload);
    }

    return ret;
}

bool RawStream::streamGood() const {
    return m_underlyingStream.good();
}
