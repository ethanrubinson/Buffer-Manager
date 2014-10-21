#include "mru.h"

MRU::MRU() {
	frameChain = new std::list<int>();
}

MRU::~MRU() {
	std::list<int>().swap(*frameChain); /* Not sure if I need this line but too afraid to remove.
										   It creates a temporary empty list to swap the chain into
										   effectively freeing both of their memories allocated to
										   store their respective elements in a single statement. */
	frameChain->clear();
	delete frameChain;
}

int MRU::PickVictim() {
	if(frameChain->empty()) return INVALID_PAGE;
	int victimFrameID = frameChain->back();
	frameChain->pop_back();
	return victimFrameID;
}

void MRU::AddFrame(int f) {
	frameChain->remove(f);
	frameChain->push_back(f);
}

void MRU::RemoveFrame(int f) {
	frameChain->remove(f);
} 
