#pragma once

#include <string>

#include "Machine.h"

class ParserException : public MachineException {
public:
	ParserException() = default;
	ParserException(const std::string& message)
		: MachineException(message) { }
};

template <typename InputStreamType, typename OutputProductionType>
class Parser : public Machine<InputStreamType, OutputProductionType> {
public:
	Parser()
		: m_lastApplicationSuccessful(false), m_lastException(nullptr) { }

	std::shared_ptr<OutputProductionType> apply(InputStreamType& rs) override;

	std::shared_ptr<OutputProductionType> parse(InputStreamType& rs);
	std::shared_ptr<OutputProductionType> parseWithIgnorance(InputStreamType& rs);
	std::list<std::shared_ptr<OutputProductionType>> parseStream(InputStreamType& rs);
	std::list<std::shared_ptr<OutputProductionType>> parseStreamWithIgnorance(InputStreamType& rs);

	bool lastApplicationSuccessful() const override { return m_lastApplicationSuccessful; }
	void reset() override;
	std::string lastError() const;

protected:
	virtual std::shared_ptr<OutputProductionType> parse_root(InputStreamType& is) = 0;
	void error(const std::string& message) const;

private:
	bool m_lastApplicationSuccessful;
	std::unique_ptr<Exception> m_lastException;
};

template<typename InputStreamType, typename OutputProductionType>
inline std::shared_ptr<OutputProductionType> Parser<InputStreamType, OutputProductionType>::apply(InputStreamType& rs) {
	try {
		m_lastApplicationSuccessful = true;
		return parse_root(rs);
	} catch (const Exception& ex) {
		m_lastException = std::make_unique<Exception>(ex);
		m_lastApplicationSuccessful = false;
		return nullptr;
	}
}

template<typename InputStreamType, typename OutputProductionType>
inline std::shared_ptr<OutputProductionType> Parser<InputStreamType, OutputProductionType>::parse(InputStreamType& rs) {
	return parse_root(rs);
}

template<typename InputStreamType, typename OutputProductionType>
inline std::shared_ptr<OutputProductionType> Parser<InputStreamType, OutputProductionType>::parseWithIgnorance(InputStreamType& rs) {
	while (rs.good()) {
		auto ret = parse(rs);
		if (ret) {
			return ret;
		}
	}

	throw ParserException("Parse with ignorance so far unsuccessful but the input stream is no longer good()");
	return nullptr;
}

template<typename InputStreamType, typename OutputProductionType>
inline std::list<std::shared_ptr<OutputProductionType>> Parser<InputStreamType, OutputProductionType>::parseStream(InputStreamType& rs) {
	std::list<std::shared_ptr<OutputProductionType>> ret;

	while (rs.good()) {
		auto p = parse(rs);
		ret.push_back(p);
	}

	return ret;
}

template<typename InputStreamType, typename OutputProductionType>
inline std::list<std::shared_ptr<OutputProductionType>> Parser<InputStreamType, OutputProductionType>::parseStreamWithIgnorance(InputStreamType& rs) {
	std::list<std::shared_ptr<OutputProductionType>> ret;

	while (rs.good()) {
		auto p = parse(rs);
		if (p) {
			ret.push_back(p);
		}
	}

	return ret;
}

template<typename InputStreamType, typename OutputProductionType>
inline void Parser<InputStreamType, OutputProductionType>::reset() {
	m_lastApplicationSuccessful = false;
}

template<typename InputStreamType, typename OutputProductionType>
inline std::string Parser<InputStreamType, OutputProductionType>::lastError() const {
	if (m_lastException) {
		return m_lastException->what();
	} else {
		return "";
	}
}

template<typename InputStreamType, typename OutputProductionType>
inline void Parser<InputStreamType, OutputProductionType>::error(const std::string& message) const {
	throw ParserException(message);
}