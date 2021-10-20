#ifndef dftefeMemoryManager_h
#define dftefeMemoryManager_h

#include "TypeConfig.h"

namespace dftefe
{
  namespace utils
  {
    //
    // MemorySpace
    //
    enum class MemorySpace
    {
      HOST,  //
      DEVICE //
    };

    //
    // MemoryManager
    //
    template <typename NumType, MemorySpace memorySpace>
    class MemoryManager
    {
    public:
      static NumType *
      allocate(size_type size);

      static void
      deallocate(NumType *ptr);
    };

    template <typename NumType>
    class MemoryManager<NumType, MemorySpace::HOST>
    {
    public:
      static NumType *
      allocate(size_type size);

      static void
      deallocate(NumType *ptr);
    };

    template <typename NumType>
    class MemoryManager<NumType, MemorySpace::DEVICE>
    {
    public:
      static NumType *
      allocate(size_type size);

      static void
      deallocate(NumType *ptr);
    };
  } // namespace utils

} // namespace dftefe

#include "MemoryManager.t.cpp"

#endif
