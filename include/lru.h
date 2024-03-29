#ifndef _LRU_H
#define _LRU_H

#include "db.h"
#include "replacer.h"
#include <list>

// LRU Buffer Replacement
class LRU : public Replacer {
public:
	LRU();
	virtual ~LRU();

	virtual int PickVictim();
	virtual void AddFrame(int f);
	virtual void RemoveFrame(int f); 

private:
	std::list<int>* frameChain;

};

#endif // LRU