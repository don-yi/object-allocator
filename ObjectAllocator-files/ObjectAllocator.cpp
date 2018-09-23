#include "ObjectAllocator.h"

ObjectAllocator::ObjectAllocator(size_t ObjectSize, const OAConfig& config)
  : PageList_(nullptr), FreeList_(nullptr)
{
}

ObjectAllocator::~ObjectAllocator()
{
}

void* ObjectAllocator::Allocate(const char* label)
{
  return nullptr;
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
  return nullptr;
}

OAConfig ObjectAllocator::GetConfig() const
{
  return {};
}

OAStats ObjectAllocator::GetStats() const
{
  return {};
}

void ObjectAllocator::allocate_new_page()
{
}

void ObjectAllocator::put_on_freelist(void* Object)
{
}

ObjectAllocator::ObjectAllocator(const ObjectAllocator& oa)
  : PageList_(nullptr), FreeList_(nullptr)
{
}

ObjectAllocator& ObjectAllocator::operator=(const ObjectAllocator& oa)
{
  return *this;
}
