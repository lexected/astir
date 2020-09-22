#pragma once

#include <deque>
#include <list>

#include "Location.h"
#include "Production.h"
#include "Terminal.h"
#include "Exception.h"

#include <stack>

class ProductionStreamException : public Exception {
public:
	ProductionStreamException() = default;
	ProductionStreamException(const std::string& message)
		: Exception(message) { }
};

template <class ProductionType>
class ProductionStream {
	static_assert(std::is_base_of<Production, ProductionType>::value);
public:
	typedef ProductionType StreamElement;
	typedef std::shared_ptr<StreamElement> StreamElementPtr;

	bool get(StreamElementPtr& c);
	StreamElementPtr peek(size_t ahead = 0);
	StreamElementPtr consume(size_t howMany = 1);
	bool good() const;

	void pin();
	std::deque<StreamElementPtr> bufferSincePin() const;
	void resetToPin();
	void unpin();

	void setBufferFixed(bool bufferFixed = true) { m_bufferFixed = bufferFixed; }
	size_t currentPosition() const;
	void resetToPosition(size_t newPosition);

	std::shared_ptr<Location> lastPinLocation() const;
	const std::shared_ptr<Location>& lastLocation() const { return m_lastLocation; }

protected:
	ProductionStream(const std::shared_ptr<Location>& startingStreamLocation)
		: m_buffer(), m_nextProductionToGive(0), m_bufferFixed(false), m_lastLocation(startingStreamLocation), m_bufferStartLocation(startingStreamLocation) { }

	virtual bool streamGet(StreamElementPtr& c) = 0;
	virtual bool streamGood() const = 0;

private:
	bool m_bufferFixed;
	size_t m_nextProductionToGive;
	std::deque<std::shared_ptr<ProductionType>> m_buffer;
	struct Pin {
		size_t bufferOffset;
		std::shared_ptr<Location> location;

		Pin(size_t offset, const std::shared_ptr<Location>& location)
			: bufferOffset(offset), location(location) { }
	};
	std::stack<Pin> m_pins;
	
	std::shared_ptr<Location> m_lastLocation;
	std::shared_ptr<Location> m_bufferStartLocation;
};

template <class ProductionType>
class ListProductionStream : public ProductionStream<ProductionType> {
public:
	ListProductionStream(const std::list<std::shared_ptr<ProductionType>>& inputProductionList)
		: ProductionStream<ProductionType>(inputProductionList.empty() ? std::make_shared<InvalidLocation>() : inputProductionList.front()->location()),
		m_list(inputProductionList), m_iterator(m_list.cbegin()) { }

protected:
	bool streamGet(std::shared_ptr<ProductionType>& c) override;
	bool streamGood() const override;

private:
	const std::list<std::shared_ptr<ProductionType>>& m_list;
	typename std::list<std::shared_ptr<ProductionType>>::const_iterator m_iterator;
};

template<class ProductionType>
inline bool ProductionStream<ProductionType>::get(StreamElementPtr& c) {
	if (m_nextProductionToGive < m_buffer.size()) {
		c = m_buffer[m_nextProductionToGive];
		++m_nextProductionToGive;
		m_lastLocation = c->location();

		return true;
	} else {
		bool ret = streamGet(c);

		if (ret) {
			m_buffer.push_back(c);
			++m_nextProductionToGive;
			m_lastLocation = c->location();
		}

		return ret;
	}
}

template<class ProductionType>
inline std::shared_ptr<ProductionType> ProductionStream<ProductionType>::peek(size_t ahead) {
	while (m_nextProductionToGive+ahead >= m_buffer.size()) { 
		std::shared_ptr<ProductionType> c;
		bool status = streamGet(c);

		if (status) {
			m_buffer.push_back(c);
		} else {
			return nullptr;
		}
	}

	return m_buffer[m_nextProductionToGive+ahead];
}

template<class ProductionType>
inline std::shared_ptr<ProductionType> ProductionStream<ProductionType>::consume(size_t howMany) {
	StreamElementPtr ret;
	size_t counter = 0;
	for(; counter < howMany;++counter) {
		auto outcome = get(ret);
		if (!outcome) {
			break;
		}
	}
	return ret;
}

template<class ProductionType>
inline bool ProductionStream<ProductionType>::good() const {
	return m_nextProductionToGive < m_buffer.size() || streamGood();
}

template<class ProductionType>
inline void ProductionStream<ProductionType>::pin() {
	m_pins.push(Pin(m_nextProductionToGive, m_lastLocation));
}

template<class ProductionType>
inline std::deque<std::shared_ptr<ProductionType>> ProductionStream<ProductionType>::bufferSincePin() const {
	if (m_pins.empty()) {
		throw ProductionStreamException("Invalid call to bufferSincePin() -- the pin stack is empty");
	}
	const auto& latestPin = m_pins.top();

	return std::deque<std::shared_ptr<ProductionType>>(m_buffer.cbegin() + latestPin.bufferOffset, m_buffer.cbegin() + m_nextProductionToGive);
}

template<class ProductionType>
inline void ProductionStream<ProductionType>::resetToPin() {
	if (m_pins.empty()) {
		throw ProductionStreamException("Invalid call to resetToPin() -- the pin stack is empty");
	}

	const auto& top = m_pins.top();
	m_nextProductionToGive = top.bufferOffset;
	m_lastLocation = top.location;
	auto newBufferStartLocation__atLeastMaybe = top.location;
	m_pins.pop();

	if (m_pins.empty() && !m_bufferFixed) {
		if (m_nextProductionToGive > 0) {
			m_buffer.erase(m_buffer.cbegin(), m_buffer.cbegin() + m_nextProductionToGive);
			m_nextProductionToGive = 0;
			m_bufferStartLocation = newBufferStartLocation__atLeastMaybe;
		}
	}
}

template<class ProductionType>
inline void ProductionStream<ProductionType>::unpin() {
	if (m_pins.empty()) {
		throw ProductionStreamException("Invalid unpin -- the pin stack is empty");
	}

	auto newBufferStartLocation = m_pins.top().location;
	m_pins.pop();

	if (m_pins.empty() && !m_bufferFixed) {
		if (m_nextProductionToGive > 0) {
			m_buffer.erase(m_buffer.cbegin(), m_buffer.cbegin() + m_nextProductionToGive);
			m_nextProductionToGive = 0;
			m_bufferStartLocation = newBufferStartLocation;
		}
	}
}

template<class ProductionType>
inline size_t ProductionStream<ProductionType>::currentPosition() const {
	return m_nextProductionToGive;
}

template<class ProductionType>
inline void ProductionStream<ProductionType>::resetToPosition(size_t newPosition) {
	m_nextProductionToGive = newPosition;

	if (newPosition > 0) {
		m_lastLocation = m_buffer[newPosition-1]->location();
	} else {
		m_lastLocation = m_bufferStartLocation;
	}
}

template<class ProductionType>
inline std::shared_ptr<Location> ProductionStream<ProductionType>::lastPinLocation() const {
	if (m_pins.empty()) {
		return nullptr;
	}

	const Pin& topPin = m_pins.top();
	return topPin.location;
}

template<class ProductionType>
inline bool ListProductionStream<ProductionType>::streamGet(std::shared_ptr<ProductionType>& c) {
	if (m_iterator != m_list.cend()) {
		c = *m_iterator;
		++m_iterator;

		return true;
	}

	return false;
}

template<class ProductionType>
inline bool ListProductionStream<ProductionType>::streamGood() const {
	return m_iterator != m_list.cend();
}
