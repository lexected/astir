#include "ProductionStream.h"
#include "RawStream.h"

bool RawStream::streamGet(RawTerminal& c) {
    char payload;
    bool ret = bool{ m_underlyingStream.get(payload) };

    if (ret) {
        m_currentStreamLocation->note(payload);
        c = RawTerminal(payload, m_currentStreamLocation);
        m_currentStreamLocation->note(payload);
    }

    return ret;
}

bool RawStream::streamGood() const {
    return m_underlyingStream.good();
}
