#include "ObjectAllocator.h"

ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig& config)
  : PageList_(nullptr), FreeList_(nullptr), 
  ObjectSize_(ObjectSize),
  PageSize_(config.ObjectsPerPage_ * ObjectSize + sizeof(void*)),
  PadBytes_(config.PadBytes_),
  ObjectsPerPage_(config.ObjectsPerPage_),
  MaxPages_(config.MaxPages_),
  Alignment_(config.Alignment_),
  LeftAlignSize_(config.LeftAlignSize_),
  InterAlignSize_(config.InterAlignSize_),
  HBlockInfo_(config.HBlockInfo_)
{
    allocate_new_page();
}

ObjectAllocator::~ObjectAllocator()
{
  GenericObject* pagetodelete;
  GenericObject* nextpage;

  pagetodelete = PageList_;
  nextpage = nullptr;

  // While a page left to delete, delete the page and walk to next page
  while (pagetodelete)
  {
    nextpage = pagetodelete->Next;

    delete[] pagetodelete;

    pagetodelete = nextpage;
  }
}

void* ObjectAllocator::Allocate(const char* label)
{
  // If no free space, allocate a new page
  if (!FreeList_)
  {
    // If max page, throw exception
    if (PagesInUse_ >= MaxPages_)
    {
      throw OAException(OAException::E_NO_PAGES,
        "out of logical memory (max pages has been reached)"
      );
    }
    allocate_new_page();
  }

  // Take the object to the first space in the list
  void* object;
  GenericObject* nextfreelist;

  object = reinterpret_cast<void*>(FreeList_);
  nextfreelist = FreeList_->Next;

  FreeList_ = nextfreelist;

  ++ObjectsInUse_;

  return object;
}

void ObjectAllocator::Free(void* Object)
{
}

unsigned ObjectAllocator::DumpMemoryInUse(DUMPCALLBACK fn) const
{
  return 0;
}

unsigned ObjectAllocator::ValidatePages(VALIDATECALLBACK fn) const
{
  return 0;
}

unsigned ObjectAllocator::FreeEmptyPages()
{
  return 0;
}

bool ObjectAllocator::ImplementedExtraCredit()
{
  return false;
}

void ObjectAllocator::SetDebugState(bool State)
{
}

const void* ObjectAllocator::GetFreeList() const
{
  return nullptr;
}

const void* ObjectAllocator::GetPageList() const
{
  return PageList_;
}

OAConfig ObjectAllocator::GetConfig() const
{
  // Make new stats object and copy member variables from private
  OAConfig config;

  config.PadBytes_ = PadBytes_;
  config.ObjectsPerPage_ = ObjectsPerPage_;
  config.MaxPages_ = MaxPages_;
  config.Alignment_ = Alignment_;
  config.LeftAlignSize_ = LeftAlignSize_;
  config.InterAlignSize_ = InterAlignSize_;
  config.HBlockInfo_ = HBlockInfo_;

  return config;
}

OAStats ObjectAllocator::GetStats() const
{
  // Make new stats object and copy member variables from private
  OAStats stats;

  stats.ObjectSize_ = ObjectSize_;
  stats.PageSize_ = PageSize_;
  stats.PagesInUse_ = PagesInUse_;
  stats.ObjectsInUse_ = ObjectsInUse_;

  return stats;
}

void ObjectAllocator::allocate_new_page()
{
  try
  {
    // If new throws an exception, catch it, and throw our own type of exception
    char* newpage;
    GenericObject* castedpage;

    newpage = new char[PageSize_];
    castedpage = reinterpret_cast<GenericObject*>(newpage);
    castedpage->Next = nullptr;

    // If first page, set page list
    if (!PageList_)
    {
      PageList_ = castedpage;
    }
    // If not first page, Walk to last page and set the new page in the next
    else
    {
      GenericObject* pagewalker = PageList_;
      while (pagewalker->Next)
      {
        pagewalker = pagewalker->Next;
      }
      pagewalker->Next = castedpage;
    }
  }
  catch (std::bad_alloc &)
  {
    throw OAException(OAException::E_NO_MEMORY, "allocate_new_page: No system memory available.");
  }

  // Devide the page into objects and link free list
  GenericObject* freelistwalker;

  // fix when allignments and pads!@#!@#!#
  freelistwalker = PageList_ + 1;
  freelistwalker->Next = nullptr;

  for (unsigned i = 0; i < ObjectsPerPage_ - 1; ++i)
  {
    GenericObject* prevfreelist;
    char* placeholder;

    prevfreelist = freelistwalker;
    placeholder = reinterpret_cast<char*>(freelistwalker) + ObjectSize_;
    freelistwalker = reinterpret_cast<GenericObject*>(placeholder);
    freelistwalker->Next = prevfreelist;
    FreeList_ = freelistwalker;
  }

  ++PagesInUse_;

  //if ( /* Object is on a valid page boundary */)
  //  // put it on the free list
  //else
  //  throw OAException(OAException::E_BAD_BOUNDARY, "validate_object: Object not on a boundary.");
}

void ObjectAllocator::put_on_freelist(void* Object)
{
}

ObjectAllocator::ObjectAllocator(const ObjectAllocator& oa)
{
}

ObjectAllocator& ObjectAllocator::operator=(const ObjectAllocator& oa)
{
  return *this;
}
