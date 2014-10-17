#include "frame.h"

Frame::Frame() {}
Frame::~Frame() {}
void Frame::Pin() {}
void Unpin() {}
int Frame::GetPinCount() {return 0;}
void Frame::EmptyIt() {}
void Frame::DirtyIt(){}
void Frame::SetPageID(PageID pid){}
bool Frame::IsDirty(){return true;}
bool Frame::IsValid(){return true;}
Status Frame::Write(){return OK;}
Status Frame::Read(PageID pid){ return OK;}
bool Frame::NotPinned(){return true;}
PageID Frame::GetPageID(){return 1;}
Page *Frame::GetPage(){return NULL;}