#ifndef _MRU_H
#define _MRU_H

#include "db.h"
#include "replacer.h"
#include <list>

// MRU Buffer Replacement
class MRU : public Replacer {
public:
	MRU();
	virtual ~MRU();

	virtual int PickVictim();
	virtual void AddFrame(int f);
	virtual void RemoveFrame(int f);

private:
	std::list<int>* frameChain;

};

#endif // MRU