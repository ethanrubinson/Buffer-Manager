#include "frame.h"

Frame::Frame() {
	pid = INVALID_PAGE;
	data = new Page();
	pinCount = 0;
	dirty = false;
}

Frame::~Frame() {
	delete data;
}

void Frame::Pin() {
	pinCount++;
}

void Frame::Unpin() {
	pinCount--;
}

int Frame::GetPinCount() {
	return pinCount;
}

void Frame::EmptyIt() {
    pid = INVALID_PAGE;
    pinCount = 0;
    dirty = false;
	// Do we need to wipe the data?
}

void Frame::DirtyIt() {
	dirty = true;
}

void Frame::SetPageID(PageID pid) {
	this->pid = pid;
}

bool Frame::IsDirty() {
	return dirty;
}

bool Frame::IsValid() {
    return (pid != INVALID_PAGE);
}
    
Status Frame::Write() {
   return MINIBASE_DB->WritePage(pid, data);
}

Status Frame::Read(PageID pid){ 
   return MINIBASE_DB->ReadPage(pid, data);
}
    
bool Frame::NotPinned() {
	return pinCount == 0;
}

PageID Frame::GetPageID() { 
	return pid;
}

Page *Frame::GetPage(){
	return data;
}
