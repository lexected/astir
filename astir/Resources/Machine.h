#pragma once

#include <memory>
#include <list>

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

protected:
	Machine() = default;
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
