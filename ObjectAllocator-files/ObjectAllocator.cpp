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

  // Fill in the allocated signature to the object and move the free list
  char* object;

  object = reinterpret_cast<char*>(FreeList_);
  FreeList_ = FreeList_->Next;

  for (auto i = 0; i < ObjectSize_; ++i)
  {
    object[i] = ALLOCATED_PATTERN;
  }

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
  // Fill in freed memory signature
  GenericObject* castedobject;
  GenericObject* nextfreespace;
  char* objectfiller;

  castedobject = reinterpret_cast<GenericObject*>(Object);
  nextfreespace = castedobject->Next;
  objectfiller = reinterpret_cast<char*>(Object);

  for (auto i = 0; i < ObjectSize_; ++i)
  {
    objectfiller[i] = FREED_PATTERN;
  }

  FreeList_ = nextfreespace;

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

// todo: link new pg to the head not the tail
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

  // Link free list
  GenericObject* freelistwalker;

  // TODO: handle allignments and pads
  freelistwalker = castedpage + 1;
  freelistwalker->Next = nullptr;

  for (unsigned i = 0; i < ObjectsPerPage_ - 1; ++i)
  {
    GenericObject* nextfreespace;
    char* placeholder;

    nextfreespace = freelistwalker;
    placeholder = reinterpret_cast<char*>(freelistwalker) + ObjectSize_;

    freelistwalker = reinterpret_cast<GenericObject*>(placeholder);
    freelistwalker->Next = nextfreespace;
    FreeList_ = freelistwalker;
  }

  // Handle private stats
  ++PagesInUse_;
  for (unsigned i = 0; i < ObjectsPerPage_; ++i)
  {
    ++FreeObjects_;
  }

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
    //GenericObject* content;
    //char* castedcontent;

    //content = freelistwalker + 1;
    //castedcontent = reinterpret_cast<char*>(content);
    //auto contentsize = ObjectSize_ - sizeof(GenericObject*);
    //for (auto i = 0; i < contentsize; ++i)
    //{
    //  castedcontent[i] = UNALLOCATED_PATTERN;
    //}

