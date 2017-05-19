#pragma once
#include <stdint.h>
#include <vector>

//#define DS_STATEMACHINE_IMPLEMENTATION
// ---------------------------------------------------------------
// EventStream
// This is a simple Blob that stores events. Usually events
// are just simple numbers. But you can also add additional 
// data. 
// This stream is used in the StateMachine and passed along
// to every active state. It is reset at the beginning of every
// frame.
// ---------------------------------------------------------------
class EventStream {

	struct EventHeader {
		uint32_t id;
		uint32_t type;
		size_t size;
	};

public:
	EventStream();
	virtual ~EventStream();
	void reset();
	void add(uint32_t type);
	void add(uint32_t type, void* p, size_t size);
	const bool get(uint32_t index, void* p) const;
	const int getType(uint32_t index) const;
	const bool containsType(uint32_t type) const;
	const uint32_t num() const {
		return _mappings.size();
	}
private:
	void addHeader(uint32_t type, size_t size);
	EventStream(const EventStream& orig) {}
	char* _data;
	std::vector<uint32_t> _mappings;
	uint32_t _index;
};


// ---------------------------------------------------------------
// GameState
// ---------------------------------------------------------------
class GameState {

public:
	GameState(const char* name);
	virtual ~GameState() {}
	virtual int tick(float dt, EventStream* stream) = 0;
	virtual void render() = 0;
	virtual void activate() = 0;
	virtual void deactivate() = 0;
	const uint32_t& getHash() const {
		return _hash;
	}
	const bool isActive() const {
		return _active;
	}
protected:
	bool _active;
	uint32_t _hash;	
};

// ---------------------------------------------------------------
// StateMachine
// ---------------------------------------------------------------
class StateMachine {

public:
	StateMachine();
	~StateMachine();
	void add(GameState* state);
	void activate(const char* name);
	void deactivate(const char* name);
	virtual void tick(float dt);
	virtual void render();
	bool hasEvents() const;
	uint32_t numEvents() const;
	const bool getEvent(uint32_t index, void* p) const;
	const int getEventType(uint32_t index) const;
	const bool containsEventType(uint32_t type) const;
protected:
	GameState* find(const char* name);
	std::vector<GameState*> _states;
	EventStream _stream;
};

#ifdef DS_STATEMACHINE_IMPLEMENTATION

const int EVENT_HEADER_SIZE = 12;

const uint32_t FNV_Prime = 0x01000193; //   16777619
const uint32_t FNV_Seed = 0x811C9DC5; // 2166136261

inline uint32_t fnv1a(const char* text, uint32_t hash = FNV_Seed) {
	const unsigned char* ptr = (const unsigned char*)text;
	while (*ptr) {
		hash = (*ptr++ ^ hash) * FNV_Prime;
	}
	return hash;
}

// ---------------------------------------------------------------
// EventStream
// ---------------------------------------------------------------
EventStream::EventStream() {
	_data = new char[4096];
	reset();
}

EventStream::~EventStream() {
	delete[] _data;
}

// -------------------------------------------------------
// reset
// -------------------------------------------------------
void EventStream::reset() {
	_mappings.clear();
	_index = 0;
}

// -------------------------------------------------------
// add event
// -------------------------------------------------------
void EventStream::add(uint32_t type, void* p, size_t size) {
	addHeader(type, size);
	char* data = _data + _index + EVENT_HEADER_SIZE;
	memcpy(data, p, size);
	_mappings.push_back(_index);
	_index += EVENT_HEADER_SIZE + size;
}

// -------------------------------------------------------
// add event
// -------------------------------------------------------
void EventStream::add(uint32_t type) {
	addHeader(type, 0);
	char* data = _data + _index;
	_mappings.push_back(_index);
	_index += EVENT_HEADER_SIZE;
}

// -------------------------------------------------------
// add header
// -------------------------------------------------------
void EventStream::addHeader(uint32_t type, size_t size) {
	EventHeader header;
	header.id = _mappings.size();;
	header.size = size;
	header.type = type;
	char* data = _data + _index;
	memcpy(data, &header, EVENT_HEADER_SIZE);
}

// -------------------------------------------------------
// get
// -------------------------------------------------------
const bool EventStream::get(uint32_t index, void* p) const {
	int lookup = _mappings[index];
	char* data = _data + lookup;
	EventHeader* header = (EventHeader*)data;
	data += EVENT_HEADER_SIZE;
	memcpy(p, data, header->size);
	return true;
}

// -------------------------------------------------------
// get type
// -------------------------------------------------------
const int EventStream::getType(uint32_t index) const {
	int lookup = _mappings[index];
	char* data = _data + lookup;
	EventHeader* header = (EventHeader*)data;
	return header->type;
}

// -------------------------------------------------------
// contains type
// -------------------------------------------------------
const bool EventStream::containsType(uint32_t type) const {
	for (int i = 0; i < _mappings.size(); ++i) {
		if (getType(i) == type) {
			return true;
		}
	}
	return false;
}


GameState::GameState(const char* name) : _active(false) {
	_hash = fnv1a(name);
}

// ---------------------------------------------------------------
// GameStateManager
// ---------------------------------------------------------------
StateMachine::StateMachine() {
	_stream.reset();
}

StateMachine::~StateMachine() {

}

void StateMachine::tick(float dt) {
	_stream.reset();
	for (size_t i = 0; i < _states.size(); ++i) {
		if (_states[i]->isActive()) {
			_states.at(i)->tick(dt, &_stream);
		}
	}

}

GameState* StateMachine::find(const char* name) {
	uint32_t h = fnv1a(name);
	for (size_t i = 0; i < _states.size(); ++i) {
		if (_states[i]->getHash() == h) {
			return _states[i];
		}
	}
	return 0;
}

void StateMachine::activate(const char* name) {
	GameState* state = find(name);
	if (state != 0) {
		state->activate();
	}
}

void StateMachine::deactivate(const char* name) {
	GameState* state = find(name);
	if (state != 0) {
		state->deactivate();
	}
}

void StateMachine::render() {
	for (size_t i = 0; i < _states.size(); ++i) {
		if (_states[i]->isActive()) {
			_states.at(i)->render();
		}
	}
}

void StateMachine::add(GameState* state) {
	_states.push_back(state);
}

bool StateMachine::hasEvents() const {
	return _stream.num() > 0;
}

uint32_t StateMachine::numEvents() const {
	return _stream.num();

}

const bool StateMachine::getEvent(uint32_t index, void* p) const {
	return _stream.get(index, p);
}

const int StateMachine::getEventType(uint32_t index) const {
	return _stream.getType(index);
}

const bool StateMachine::containsEventType(uint32_t type) const {
	return _stream.containsType(type);
}
#endif