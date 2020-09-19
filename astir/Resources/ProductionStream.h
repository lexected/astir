#pragma once

#include <deque>
#include <list>

#include "Location.h"
#include "Production.h"
#include "Terminal.h"

template <class ProductionType>
class ProductionStream {
	static_assert(std::is_base_of<Production, ProductionType>::value);
public:
	typedef ProductionType StreamElement;
	typedef std::shared_ptr<StreamElement> StreamElementPtr;

	bool get(StreamElementPtr& c);
	StreamElementPtr peek(size_t ahead = 0);
	bool consume();
	bool good() const;

	void pin();
	std::deque<StreamElementPtr> bufferSincePin() const;
	void resetToPin();
	void unpin();

	size_t currentPosition() const;
	void resetToPosition(size_t newPosition);

	const std::shared_ptr<Location>& pinLocation() const { return m_pinLocation; }
	const std::shared_ptr<Location>& lastLocation() const { return m_lastLocation; }
protected:
	ProductionStream(const std::shared_ptr<Location>& startingStreamLocation)
		: m_buffer(), m_nextProductionToGive(0), m_pinLocation(startingStreamLocation), m_lastLocation(startingStreamLocation) { }

	virtual bool streamGet(StreamElementPtr& c) = 0;
	virtual bool streamGood() const = 0;
private:
	size_t m_nextProductionToGive;
	std::deque<std::shared_ptr<ProductionType>> m_buffer;

	std::shared_ptr<Location> m_pinLocation;
	std::shared_ptr<Location> m_lastLocation;
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

	return m_buffer[m_nextProductionToGive];
}

template<class ProductionType>
inline bool ProductionStream<ProductionType>::consume() {
	StreamElementPtr __discard;
	return get(__discard);
}

template<class ProductionType>
inline bool ProductionStream<ProductionType>::good() const {
	return m_nextProductionToGive < m_buffer.size() || streamGood();
}

template<class ProductionType>
inline void ProductionStream<ProductionType>::pin() {
	if (m_nextProductionToGive > 0) {
		m_buffer.erase(m_buffer.cbegin(), m_buffer.cbegin() + m_nextProductionToGive);
		m_nextProductionToGive = 0;
	}

	m_pinLocation = m_lastLocation;
}

template<class ProductionType>
inline std::deque<std::shared_ptr<ProductionType>> ProductionStream<ProductionType>::bufferSincePin() const {
	return std::deque<std::shared_ptr<ProductionType>>(m_buffer.cbegin(), m_buffer.cbegin() + m_nextProductionToGive);
}

template<class ProductionType>
inline void ProductionStream<ProductionType>::resetToPin() {
	m_nextProductionToGive = 0;
	m_lastLocation = m_pinLocation;
}

template<class ProductionType>
inline void ProductionStream<ProductionType>::unpin() {
	m_buffer.clear();
	m_nextProductionToGive = 0;
	m_pinLocation = nullptr;
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
		m_lastLocation = m_pinLocation;
	}
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
