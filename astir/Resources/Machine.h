#pragma once

#include <memory>
#include <list>
#include <map>

#include "Exception.h"

class MachineException : public Exception {
public:
	MachineException() = default;
	MachineException(const std::string& message)
		: Exception(message) { }
};

template <typename InputStreamType, typename OutputProductionType>
class Machine {
public:
	virtual std::shared_ptr<OutputProductionType> apply(InputStreamType& rs) = 0;

	std::shared_ptr<OutputProductionType> applyWithIgnorance(InputStreamType& rs);
	std::shared_ptr<OutputProductionType> tryApply(InputStreamType& rs);
	std::shared_ptr<OutputProductionType> tryApplyWithIgnorance(InputStreamType& rs);

	std::list<std::shared_ptr<OutputProductionType>> processStream(InputStreamType& rs);
	std::list<std::shared_ptr<OutputProductionType>> processStreamWithIgnorance(InputStreamType& rs);
	std::list<std::shared_ptr<OutputProductionType>> tryProcessStream(InputStreamType& rs);
	std::list<std::shared_ptr<OutputProductionType>> tryProcessStreamWithIgnorance(InputStreamType& rs);

	virtual bool lastApplicationSuccessful() const = 0;
	virtual void reset() = 0;

	// peeking mechanism
	std::shared_ptr<OutputProductionType> peek(InputStreamType& rs, size_t ahead, size_t& cumulativePeekCorrection);
	template <typename DesiredType>
	std::shared_ptr<DesiredType> peekCast(InputStreamType& rs, size_t ahead, size_t& cumulativePeekCorrection);
	std::shared_ptr<OutputProductionType> consume(InputStreamType& rs);
	bool unpeekIfApplicable(InputStreamType& rs, size_t& cumulativePeekCorrection);
	bool unpeekAll(InputStreamType& rs, size_t& cumulativePeekCorrection);

protected:
	Machine() = default;

private:
	struct PeekCacheEntry {
		std::shared_ptr<OutputProductionType> production;
		size_t inputStreamSpan;

		PeekCacheEntry(const std::shared_ptr<OutputProductionType>& production, size_t inputStreamSpan)
			: production(production), inputStreamSpan(inputStreamSpan) { }
	};
	std::map<size_t, PeekCacheEntry> m_peekCache;
};

template<typename InputStreamType, typename OutputProductionType>
inline std::shared_ptr<OutputProductionType> Machine<InputStreamType, OutputProductionType>::applyWithIgnorance(InputStreamType& rs) {
	do {
		auto terminalPtr = apply(rs);
		if (terminalPtr) {
			return terminalPtr;
		}
	} while (lastApplicationSuccessful() && rs.good());
}

template<typename InputStreamType, typename OutputProductionType>
inline std::shared_ptr<OutputProductionType> Machine<InputStreamType, OutputProductionType>::tryApply(InputStreamType& rs) {
	rs.pin();
	auto terminalPtr = apply(rs);
	if (lastApplicationSuccessful()) {
		rs.unpin();
		reset();
		return terminalPtr;
	} else {
		reset();
		rs.resetToPin();
		return nullptr;
	}
}

template<typename InputStreamType, typename OutputProductionType>
inline std::shared_ptr<OutputProductionType> Machine<InputStreamType, OutputProductionType>::tryApplyWithIgnorance(InputStreamType& rs) {
	do {
		rs.pin();
		auto terminalPtr = apply(rs);
		if (lastApplicationSuccessful()) {
			rs.unpin();
			reset();
			if (terminalPtr) {
				return terminalPtr;
			}
		} else {
			rs.resetToPin();
			reset();
			return nullptr;
		}
	} while (rs.good());

	return nullptr; // TODO: maybe replace with an exception for consistency, or something ... but that would be difficult to catch I guess... unless you create a general (Machine)Exception exception all particular Exception classes will inherit from
}

template<typename InputStreamType, typename OutputProductionType>
inline std::list<std::shared_ptr<OutputProductionType>> Machine<InputStreamType, OutputProductionType>::processStream(InputStreamType& rs) {
	std::list<std::shared_ptr<OutputProductionType>> ret;

	bool wasLastApplicationSuccessful;
	do {
		auto terminalPtr = apply(rs);
		ret.push_back(terminalPtr);
		wasLastApplicationSuccessful = lastApplicationSuccessful();
		if (wasLastApplicationSuccessful) {
			reset();
		}
	} while (wasLastApplicationSuccessful && rs.good());

	return ret;
}

template<typename InputStreamType, typename OutputProductionType>
inline std::list<std::shared_ptr<OutputProductionType>> Machine<InputStreamType, OutputProductionType>::processStreamWithIgnorance(InputStreamType& rs) {
	std::list<std::shared_ptr<OutputProductionType>> ret;

	bool wasLastApplicationSuccessful;
	do {
		auto terminalPtr = apply(rs);
		if (terminalPtr) {
			ret.push_back(terminalPtr);
		}
		wasLastApplicationSuccessful = lastApplicationSuccessful();
		if (wasLastApplicationSuccessful) {
			reset();
		}
	} while (wasLastApplicationSuccessful && rs.good());

	return ret;
}

template<typename InputStreamType, typename OutputProductionType>
inline std::list<std::shared_ptr<OutputProductionType>> Machine<InputStreamType, OutputProductionType>::tryProcessStream(InputStreamType& rs) {
	std::list<std::shared_ptr<OutputProductionType>> ret;

	bool wasLastApplicationSuccessful;
	do {
		rs.pin();
		auto terminalPtr = apply(rs);
		ret.push_back(terminalPtr);
		wasLastApplicationSuccessful = lastApplicationSuccessful();
		reset();
		if (!wasLastApplicationSuccessful) {
			rs.resetToPin();
		} else {
			rs.unpin();
		}
	} while (wasLastApplicationSuccessful && rs.good());

	return ret;
}

template<typename InputStreamType, typename OutputProductionType>
inline std::list<std::shared_ptr<OutputProductionType>> Machine<InputStreamType, OutputProductionType>::tryProcessStreamWithIgnorance(InputStreamType& rs) {
	std::list<std::shared_ptr<OutputProductionType>> ret;

	bool wasLastApplicationSuccessful;
	do {
		rs.pin();
		auto terminalPtr = apply(rs);
		if (terminalPtr) {
			ret.push_back(terminalPtr);
		}
		wasLastApplicationSuccessful = lastApplicationSuccessful();
		reset();
		if (!wasLastApplicationSuccessful) {
			rs.resetToPin();
		} else {
			rs.unpin();
		}
	} while (wasLastApplicationSuccessful && rs.good());

	return ret;
}

template<typename InputStreamType, typename OutputProductionType>
inline std::shared_ptr<OutputProductionType> Machine<InputStreamType, OutputProductionType>::peek(InputStreamType& rs, size_t ahead, size_t& cumulativePeekCorrection) {
	size_t targetStartPosition = rs.currentPosition() + ahead;
	auto fit = m_peekCache.find(targetStartPosition);
	if (fit != m_peekCache.end()) {
		cumulativePeekCorrection += fit->second.inputStreamSpan;
		return fit->second.production;
	}

	rs.pin();
	rs.consume(ahead);

	size_t startingPosition = rs.currentPosition();
	auto production = tryApplyWithIgnorance(rs);
	size_t endingPosition = rs.currentPosition();
	if (production) {
		size_t deltaPosition = endingPosition - startingPosition;
		rs.setBufferFixed(true);
		m_peekCache.emplace(startingPosition, PeekCacheEntry(production, deltaPosition));
		cumulativePeekCorrection += deltaPosition;
	}

	rs.resetToPin();
	return production;
}

template<typename InputStreamType, typename OutputProductionType>
inline std::shared_ptr<OutputProductionType> Machine<InputStreamType, OutputProductionType>::consume(InputStreamType& rs) {
	size_t targetStartPosition = rs.currentPosition();
	auto fit = m_peekCache.find(targetStartPosition);
	if (fit != m_peekCache.end()) {
		rs.consume(fit->second.inputStreamSpan);
		auto ret = fit->second.production;
		m_peekCache.erase(fit);
		if (m_peekCache.empty()) {
			rs.setBufferFixed(false);
		}
		return ret;
	} else {
		throw std::exception("Peeking indices do not correspond to consumption points");
		return nullptr; // should really throw exception here instead
	}
}

template<typename InputStreamType, typename OutputProductionType>
inline bool Machine<InputStreamType, OutputProductionType>::unpeekIfApplicable(InputStreamType& rs, size_t& cumulativePeekCorrection) {
	size_t targetEndPosition = rs.currentPosition() + cumulativePeekCorrection;
	auto fit = std::lower_bound(m_peekCache.cbegin(), m_peekCache.cend(), targetEndPosition, [](const auto& cachePair, const auto& cmpVal) {
		return cachePair.first + cachePair.second.inputStreamSpan < cmpVal;
	});
	if (fit != m_peekCache.end() && fit->first + fit->second.inputStreamSpan == targetEndPosition) {
		cumulativePeekCorrection -= fit->second.inputStreamSpan;
		m_peekCache.erase(fit);
		if (m_peekCache.empty()) {
			rs.setBufferFixed(false);
		}
	}

	return false;
}

template<typename InputStreamType, typename OutputProductionType>
inline bool Machine<InputStreamType, OutputProductionType>::unpeekAll(InputStreamType& rs, size_t& cumulativePeekCorrection) {
	m_peekCache.clear();
	rs.setBufferFixed(false);

	cumulativePeekCorrection = 0;

	return false;
}

template<typename InputStreamType, typename OutputProductionType>
template<typename DesiredType>
inline std::shared_ptr<DesiredType> Machine<InputStreamType, OutputProductionType>::peekCast(InputStreamType& rs, size_t ahead, size_t& cumulativePeekCorrection) {
	auto cast = std::dynamic_pointer_cast<DesiredType>(peek(rs, ahead, cumulativePeekCorrection));
	if (!cast) {
		unpeekIfApplicable(rs, cumulativePeekCorrection);
	}
	return cast;
}
