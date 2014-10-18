#include "frame.h"
#include "page.h"
#include "db.h"

class Frame 
{
  private:

    PageID pid;
    Page* data;
    int pinCount;
    bool dirty;

  public:

    Frame::Frame() {
      pinCount = 0;
      dirty = false;
      pid = INVALID_PAGE;
      data = new Page(); //This will always point to something. 

    }

    Frame::~Frame() {
      delete data;
    }

    void Frame::Pin() {pinCount++;} //Do we need to check if this has a page?

    void Unpin() {pinCount--;}

    int Frame::GetPinCount() {return pinCount;}

    void Frame::EmptyIt() {
      pid = INVALID_PAGE;
      pinCount = 0;
      dirty = false;
    }

    void Frame::DirtyIt(){dirty = true;}

    void Frame::SetPageID(PageID pid){pid = pid;}

    bool Frame::IsDirty(){return dirty;}

    // Returns false if Frame is empty
    bool Frame::IsValid(){
      return !(pid == INVALID_PAGE);
    }
    
    // TODO
    Status Frame::Write(){
      db::WritePage(pid,"DISK"); //What page constitutes disk?
      EmptyIt();
    }

    Status Frame::Read(PageID pid){ 
      SetPageID(pid);
      db::ReadPage(pid,data);
      DirtyIt();
      return OK;}
    
    // Returns true if not pinned
    bool Frame::NotPinned(){return pinCount == 0;}

    PageID Frame::GetPageID(){return pid}

    Page *Frame::GetPage(){return data}
