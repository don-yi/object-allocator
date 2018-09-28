#include "ObjectAllocator.h"

ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig& config)
  : PageList_(nullptr), FreeList_(nullptr), 
  ObjectSize_(ObjectSize),
  PageSize_(config.ObjectsPerPage_ * ObjectSize + sizeof(void*)
    + (config.ObjectsPerPage_ * 2 * config.PadBytes_)
    + (config.ObjectsPerPage_ * config.HBlockInfo_.size_)
  ),
  PadBytes_(config.PadBytes_),
  ObjectsPerPage_(config.ObjectsPerPage_),
  MaxPages_(config.MaxPages_),
  Alignment_(config.Alignment_),
  LeftAlignSize_(config.LeftAlignSize_),
  InterAlignSize_(config.InterAlignSize_),
  HBlockInfo_(config.HBlockInfo_)
{
  allocate_new_page();

  // Handle exception
  if (!PageList_)
  {
      throw OAException(OAException::E_NO_MEMORY,
        "out of physical memory (operator new fails)"
      );
  }
}

ObjectAllocator::~ObjectAllocator()
{
  GenericObject* pagewalker;
  GenericObject* nextpage;

  pagewalker = PageList_;
  nextpage = nullptr;

  // While a page left to delete, delete the page and walk to next page
  while (pagewalker)
  {
    nextpage = pagewalker->Next;

    delete[] pagewalker;

    pagewalker = nextpage;
  }
}

// todo: adjust free list and mark allocated signature
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

  // Fill in the allocated signature to the object and link the free list
  GenericObject* nextfree;
  char* object;

  nextfree = FreeList_->Next;
  object = reinterpret_cast<char*>(FreeList_);

  for (auto i = 0; i < ObjectSize_; ++i)
  {
    object[i] = ALLOCATED_PATTERN;
  }

  FreeList_ = nextfree;

  // Handle private stats
  ++ObjectsInUse_;
  --FreeObjects_;
  ++Allocations_;

  if (MostObjects_ < ObjectsInUse_)
  {
    MostObjects_ = ObjectsInUse_;
  }

  return reinterpret_cast<void*>(object);
}

void ObjectAllocator::Free(void* Object)
{
  GenericObject* castedobject;
  castedobject = reinterpret_cast<GenericObject*>(Object);

  // Make sure this object hasn't been freed yet
  if (IsOnFreeList(castedobject))
  {
    throw OAException(OAException::E_MULTIPLE_FREE,
      "FreeObject: Object has already been freed."
    );
  }

  // Make sure this object is not on bad boundary
  if (IsOnBadBoundary(castedobject))
  {
    throw OAException(OAException::E_BAD_BOUNDARY,
      "block address is on a page, but not on any block-boundary"
    );
  }

  // Fill in freed memory signature
  char* objectfiller;

  objectfiller = reinterpret_cast<char*>(Object);

  for (auto i = 0; i < ObjectSize_; ++i)
  {
    objectfiller[i] = FREED_PATTERN;
  }

  // Link the free list
  castedobject->Next = FreeList_;
  FreeList_ = castedobject;

  // Handle private stats
  --ObjectsInUse_;
  ++FreeObjects_;
  ++Deallocations_;
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
  return reinterpret_cast<const void*>(PageList_);
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
  stats.FreeObjects_ = FreeObjects_;
  stats.Allocations_ = Allocations_;
  stats.Deallocations_ = Deallocations_;
  stats.MostObjects_ = MostObjects_;

  return stats;
}

void ObjectAllocator::allocate_new_page()
{
  char* newpage;
  GenericObject* castedpage;

  try
  {
    // If new throws an exception, catch it, and throw our own type of exception
    newpage = new char[PageSize_];
    // Fill in unallocated memory signature
    for (auto i = 0; i < PageSize_; ++i)
    {
      newpage[i] = UNALLOCATED_PATTERN;
    }

    // Fill in padding memory signature
    if (PadBytes_)
    {
      char* padwalker;
      char* pageend;

      padwalker = newpage + sizeof(GenericObject*);
      pageend = newpage + PageSize_;

      while (padwalker < pageend)
      {
        for (unsigned i = 0; i < PadBytes_; ++i)
        {
          padwalker[i] = PAD_PATTERN;
        }

        padwalker += ObjectSize_ + PadBytes_;

        for (unsigned i = 0; i < PadBytes_; ++i)
        {
          padwalker[i] = PAD_PATTERN;
        }

        padwalker += PadBytes_;
      }
    }

    castedpage = reinterpret_cast<GenericObject*>(newpage);

    // Link the page list
    GenericObject* nextpage = PageList_;
    PageList_ = castedpage;
    PageList_->Next = nextpage;
  }
  catch (std::bad_alloc &)
  {
    throw OAException(OAException::E_NO_MEMORY, "allocate_new_page: No system memory available.");
  }

  // Link free list
  // TODO: handle allignments
  char* placeholder;
  GenericObject* freelistwalker;

  placeholder = newpage + sizeof(GenericObject*) + PadBytes_;
  freelistwalker = reinterpret_cast<GenericObject*>(placeholder);
  FreeList_ = freelistwalker;
  FreeList_->Next = nullptr;

  for (unsigned i = 0; i < ObjectsPerPage_ - 1; ++i)
  {
    placeholder = placeholder + ObjectSize_ + (2 * PadBytes_);
    freelistwalker = reinterpret_cast<GenericObject*>(placeholder);
    freelistwalker->Next = FreeList_;
    FreeList_ = freelistwalker;
  }

  // Handle private stats
  ++PagesInUse_;
  for (unsigned i = 0; i < ObjectsPerPage_; ++i)
  {
    ++FreeObjects_;
  }
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

// Make sure this object hasn't been freed yet
// todo: make it constant time
bool ObjectAllocator::IsOnFreeList(GenericObject * object) const
{
  GenericObject* freelistwalker;
  freelistwalker = FreeList_;
    
  while (freelistwalker)
  {
    if (freelistwalker == object)
    {
      return true;
    }
    freelistwalker = freelistwalker->Next;
  }

  return false;
}

// Make sure this object is not on bad boundary
bool ObjectAllocator::IsOnBadBoundary(GenericObject * object) const
{
  GenericObject* pagewalker;
  char* pageend;
  GenericObject* castedpageend;
  char* firstobjpos;
  //char* lastobjpos;

  pagewalker = PageList_;

  // Check the page which the object is on
  while (true)
  {
    pageend = reinterpret_cast<char*>(pagewalker) + PageSize_;
    castedpageend = reinterpret_cast<GenericObject*>(pageend);

    firstobjpos = reinterpret_cast<char*>(pagewalker) + sizeof(GenericObject*) +
      PadBytes_
    ;

    // If the object is in a page
    if (reinterpret_cast<GenericObject*>(firstobjpos) <= object &&
      object < castedpageend
      )
    {
      break;
    }

    // If the object is not in a page
    pagewalker = pagewalker->Next;
    if (!pagewalker)
    {
      return true;
    }
  }


  //// Good boundary: The object at first or last position in a page
  //lastobjpos = pageend - ObjectSize_ - PadBytes_;
  //// todo: consider allignments
  //if (object == reinterpret_cast<GenericObject*>(firstobjpos) ||
  //  object == reinterpret_cast<GenericObject*>(lastobjpos)
  //  )
  //{
  //  return false;
  //}

  // The object in between
  size_t disttoobj;

  disttoobj = reinterpret_cast<char*>(object) - firstobjpos;
  return (disttoobj % (ObjectSize_ + 2 * PadBytes_)) != 0;
}

  //GenericObject* content;
  //char* castedcontent;

  //content = freelistwalker + 1;
  //castedcontent = reinterpret_cast<char*>(content);
  //auto contentsize = ObjectSize_ - sizeof(GenericObject*);
  //for (auto i = 0; i < contentsize; ++i)
  //{
  //  castedcontent[i] = UNALLOCATED_PATTERN;
  //}
