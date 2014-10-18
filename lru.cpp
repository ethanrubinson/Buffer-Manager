#include "lru.h"


// SCHEMA FOR LEAST RECENTLY USED POLICY
// Uses a list to maintain elements.
// The head element is the least recently used.
// New elements are added to the tail and old duplicate entries removed if present

LRU::LRU() {
	frameChain = new std::list<int>();
}

LRU::~LRU() {
	std::list<int>().swap(*frameChain); /* Not sure if I need this line but too afraid to remove.
										   It creates a temporary empty list to swap the chain into
										   effectively freeing both of their memories allocated to
										   store their respective elements in a single statement. */
	frameChain->clear();
	delete frameChain;
}

int LRU::PickVictim() {

	if(frameChain->empty()) return INVALID_PAGE;
	int victimFrameID = frameChain->front();
	frameChain->pop_front();
	return victimFrameID;
}

void LRU::AddFrame(int f) {
	frameChain->remove(f);
	frameChain->push_back(f);
}

void LRU::RemoveFrame(int f) {
	frameChain->remove(f);
} 
