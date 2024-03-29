
#include "bufmgr.h"
#include "frame.h"
#include "lru.h"
#include "mru.h"

//--------------------------------------------------------------------
// Constructor for BufMgr
//
// Input   : bufSize  - number of frames(pages) in the this buffer manager
//           replacementPolicy - a replacement policy, either LRU or MRU            
// Output  : None
// PostCond: All frames are empty.
//           the "replacer" is initiated to LRU or MRU according to the
//           replacement policy.
//--------------------------------------------------------------------
BufMgr::BufMgr(int bufSize, const char* replacementPolicy)
{
	numFrames = bufSize;
	frames = new Frame [numFrames];
	
	if (strcmpi(replacementPolicy, "LRU") == 0) 
		replacer = new LRU();
	else
		replacer = new MRU();

	totalCall = 0;
	totalHit = 0;
	numDirtyPageWrites = 0;
}

//--------------------------------------------------------------------
// Destructor for BufMgr
//
// Input   : None
// Output  : None
//--------------------------------------------------------------------
BufMgr::~BufMgr()
{   
	FlushAllPages();
	delete [] frames;
	delete replacer;
}

//--------------------------------------------------------------------
// BufMgr::PinPage
//
// Input    : pid     - page id of a particular page 
//            isEmpty - (optional, default to false) if true indicate
//                      that the page to be pinned is an empty page.
// Output   : page - a pointer to a page in the buffer pool. (NULL
//            if fail)
// Purpose  : Pin the page with page id = pid to the buffer.  
//            Read the page from disk unless isEmpty is true or unless
//            the page is already in the buffer.
// Condition: Either the page is already in the buffer, or there is at
//            least one frame available in the buffer pool for the 
//            page.
// PostCond : The page with page id = pid resides in the buffer and 
//            is pinned. The number of pin on the page increase by
//            one.
// Return   : OK if operation is successful.  FAIL otherwise.
//--------------------------------------------------------------------
Status BufMgr::PinPage(PageID pid, Page*& page, bool isEmpty)
{
	if(pid == INVALID_PAGE) return FAIL;

	totalCall++;

	// Check if the page is in the buffer pool
	bool inPool = false;
	Frame* currFrame;
	for (int iter = 0; iter < numFrames; iter++) {
		currFrame = &frames[iter];
		if (currFrame->GetPageID() == pid){
			inPool = true;
			totalHit++;
			break;
		}
	}

	if (inPool){
		// Increase its pin count and set output page pointer
		currFrame->Pin();
		page = currFrame->GetPage();
	}
	else {
		
		// Find the first free frame if there is one
		bool foundEmptyFrame = false;
		for (int iter = 0; iter < numFrames; iter++) {
			currFrame = &frames[iter];
			if (!currFrame->IsValid()){
				foundEmptyFrame = true;
				break;
			}
		}
		
		if (!foundEmptyFrame) {
		
			// Find a page to evict based on our replacement policy
			int replacedPageID = replacer->PickVictim();

			// Get a pointer to the frame we will flush
			for (int iter = 0; iter < numFrames; iter++) {
				currFrame = &frames[iter];
				if (currFrame->GetPageID() == replacedPageID){
					break;
				}
			}

			if(FlushPage(replacedPageID) != OK) { 
				page = NULL;
				return FAIL;
			}
		}

		currFrame->SetPageID(pid);
		currFrame->Pin();

		// If the page is not empty, read it in from disk
		if (!isEmpty && currFrame->Read(pid) != OK) {
			page = NULL;
			return FAIL;
		}
		

		page = currFrame->GetPage();
	}

	// Now that the frame is pinned we need to remove it from the ones that can be evicted
	replacer->RemoveFrame(currFrame->GetPageID());
	////std::cout << "pinned page: " << currFrame->GetPageID() <<std::endl;
	return OK;
} 

//--------------------------------------------------------------------
// BufMgr::UnpinPage
//
// Input    : pid     - page id of a particular page 
//            dirty   - indicate whether the page with page id = pid
//                      is dirty or not. (Optional, default to false)
// Output   : None
// Purpose  : Unpin the page with page id = pid in the buffer. Mark 
//            the page dirty if dirty is true.  
// Condition: The page is already in the buffer and is pinned.
// PostCond : The page is unpinned and the number of pin on the
//            page decrease by one. 
// Return   : OK if operation is successful.  FAIL otherwise.
//--------------------------------------------------------------------
Status BufMgr::UnpinPage(PageID pid, bool dirty)
{
	//std::cout << "Unin PageID " << pid << std::endl;
	////std::cout << "Unpinning page  " << pid << " Dirty?: " << dirty << std::endl;
	int frameIndex = FindFrame(pid);
	if (frameIndex == INVALID_FRAME) return FAIL;

	Frame* targetFrame = &frames[frameIndex];
	if (targetFrame->NotPinned()) return FAIL;

	if (dirty) targetFrame->DirtyIt();

	targetFrame->Unpin();

	if (targetFrame->NotPinned()) replacer->AddFrame(targetFrame->GetPageID());

	return OK;
}

//--------------------------------------------------------------------
// BufMgr::NewPage
//
// Input    : howMany - (optional, default to 1) how many pages to 
//                      allocate.
// Output   : firstPid  - the page id of the first page (as output by
//                   DB::AllocatePage) allocated.
//            firstPage - a pointer to the page in memory.
// Purpose  : Allocate howMany number of pages, and pin the first page
//            into the buffer. 
// Condition: howMany > 0 and there is at least one free buffer space
//            to hold a page.
// PostCond : The page with page id = pid is pinned into the buffer.
// Return   : OK if operation is successful.  FAIL otherwise.
// Note     : You can call DB::AllocatePage() to allocate a page.  
//            You should call DB:DeallocatePage() to deallocate the
//            pages you allocate if you failed to pin the page in the
//            buffer.
//--------------------------------------------------------------------
Status BufMgr::NewPage (PageID& firstPid, Page*& firstPage, int howMany)
{
	//std::cout << "New Page Request for " << howMany << " pages." << std::endl;
	// Condition Checks
	if (howMany <= 0) return FAIL;

	bool foundEmptyFrame = false;
	Frame* currFrame;
	for (int iter = 0; iter < numFrames; iter++) {
		currFrame = &frames[iter];
		if (!currFrame->IsValid() || currFrame->NotPinned()){
			foundEmptyFrame = true;
			break;
		}
	}
	//std::cout << "Found empty frame? " << ((foundEmptyFrame) ? "yes" : "no") << "." << std::endl;
	if (!foundEmptyFrame) return FAIL;

	// Allocate the pages

	//std::cout << "Allocating" << std::endl;

	if (MINIBASE_DB->AllocatePage(firstPid, howMany) != OK) {
		firstPid = INVALID_PAGE;
		firstPage = NULL;
		return FAIL;
	}

	//std::cout << "Allocated " << howMany << " pages with fristPid = " << firstPid << std::endl;

	if (PinPage(firstPid,firstPage,true) != OK) {
		MINIBASE_DB->DeallocatePage(firstPid, howMany);
		firstPid = INVALID_PAGE;
		firstPage = NULL;
		return FAIL;
	}

	//std::cout << "Allocated " << howMany << " new pages and pinned pageID: " << firstPid << std::endl;
	return OK;
}

//--------------------------------------------------------------------
// BufMgr::FreePage
//
// Input    : pid     - page id of a particular page 
// Output   : None
// Purpose  : Free the memory allocated for the page with 
//            page id = pid  
// Condition: Either the page is already in the buffer and is pinned
//            no more than once, or the page is not in the buffer.
// PostCond : The page is unpinned, and the frame where it resides in
//            the buffer pool is freed.  Also the page is deallocated
//            from the database. 
// Return   : OK if operation is successful.  FAIL otherwise.
// Note     : You can call MINIBASE_DB->DeallocatePage(pid) to
//            deallocate a page.
//--------------------------------------------------------------------
Status BufMgr::FreePage(PageID pid)
{
	//std::cout << "Free PageID " << pid << std::endl;
	////std::cout << "Free page:  " << pid << std::endl;

	Frame* targetFrame;
	int frameIndex = FindFrame(pid);
	if (frameIndex != INVALID_FRAME) {
		targetFrame = &frames[frameIndex];

		if (targetFrame->GetPinCount() > 1) return FAIL;
		
		UnpinPage(pid, true);
		FlushPage(pid);
	}
	
	return MINIBASE_DB->DeallocatePage(pid);
}





//--------------------------------------------------------------------
// BufMgr::FlushPage
//
// Input    : pid  - page id of a particular page 
// Output   : None
// Purpose  : Flush the page with the given pid to disk.
// Condition: The page with page id = pid must be in the buffer,
//            and is not pinned. pid cannot be INVALID_PAGE.
// PostCond : The page with page id = pid is written to disk if it's dirty. 
//            The frame where the page resides is empty.
// Return   : OK if operation is successful.  FAIL otherwise.
//--------------------------------------------------------------------
Status BufMgr::FlushPage(PageID pid)
{
	//std::cout << "Flush Page" << pid << std::endl;
	////std::cout << "Flush Page  " << pid << std::endl;
	int frameIndex = FindFrame(pid);
	if (frameIndex == INVALID_FRAME) return FAIL;

	Frame* targetFrame = &frames[frameIndex];
	if(!targetFrame->IsValid() || !targetFrame->NotPinned()) return FAIL;

	if (targetFrame->IsDirty()){
		if (targetFrame->Write() != OK) return FAIL;
		numDirtyPageWrites++;
	}
	
	replacer->RemoveFrame(targetFrame->GetPageID());
	targetFrame->EmptyIt();
	//std::cout << "Flush OK " << std::endl;
	return OK;
} 

//--------------------------------------------------------------------
// BufMgr::FlushAllPages
//
// Input    : None
// Output   : None
// Purpose  : Flush all pages in this buffer pool to disk.
// Condition: All pages in the buffer pool must not be pinned.
// PostCond : All dirty pages in the buffer pool are written to 
//            disk (even if some pages are pinned). All frames are empty.
// Return   : OK if operation is successful.  FAIL otherwise.
//--------------------------------------------------------------------

Status BufMgr::FlushAllPages()
{
	//std::cout << "Flush all " << std::endl;
	bool failedOnce = false;
	Frame* currFrame;
	for (int iter = 0; iter < numFrames; iter++) {
		currFrame = &frames[iter];
		if (currFrame->IsValid()) {
			// Check that the frame is not pinned
			if (!currFrame->NotPinned()){
				failedOnce = true;
			}

			if (currFrame->IsDirty()){
				if (currFrame->Write() != OK) failedOnce = true;
				numDirtyPageWrites++;
			}

			replacer->RemoveFrame(currFrame->GetPageID());
			currFrame->EmptyIt();
		}
	}
	return (failedOnce) ? FAIL : OK;
}


//--------------------------------------------------------------------
// BufMgr::GetNumOfUnpinnedFrames
//
// Input    : None
// Output   : None
// Purpose  : Find out how many unpinned locations are in the buffer
//            pool.
// Condition: None
// PostCond : None
// Return   : The number of unpinned buffers in the buffer pool.
//--------------------------------------------------------------------
unsigned int BufMgr::GetNumOfUnpinnedFrames()
{
	int count = 0;
	Frame* currFrame;
	for (int iter = 0; iter < numFrames; iter++) {
		currFrame = &frames[iter];
		if (currFrame->NotPinned()) {
			count++;
		}
	}

	return count;
}

//--------------------------------------------------------------------
// BufMgr::FindFrame
//
// Input    : pid - a page id 
// Output   : None
// Purpose  : Look for the page in the buffer pool, return the frame
//            number if found.
// PreCond  : None
// PostCond : None
// Return   : the frame number if found. INVALID_FRAME otherwise.
//--------------------------------------------------------------------
int BufMgr::FindFrame( PageID pid )
{
	Frame* currFrame;
	for (int iter = 0; iter < numFrames; iter++) {
		currFrame = &frames[iter];
		if (currFrame->GetPageID() == pid) {
			return iter;
		}
	}
	return INVALID_FRAME;
}



void BufMgr::ResetStat() { 
	totalHit = 0; 
	totalCall = 0; 
	numDirtyPageWrites = 0;
}

void  BufMgr::PrintStat() {
	cout<<"**Buffer Manager Statistics**"<<endl;
	cout<<"Number of Dirty Pages Written to Disk: "<<numDirtyPageWrites<<endl;
	cout<<"Number of Pin Page Requests: "<<totalCall<<endl;
	cout<<"Number of Pin Page Request Misses "<<totalCall-totalHit<<endl;
}

