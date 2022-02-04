/******************************************************************************
 * Copyright (c) 2021.                                                        *
 * The Regents of the University of Michigan and DFT-EFE developers.          *
 *                                                                            *
 * This file is part of the DFT-EFE code.                                     *
 *                                                                            *
 * DFT-EFE is free software: you can redistribute it and/or modify            *
 *   it under the terms of the Lesser GNU General Public License as           *
 *   published by the Free Software Foundation, either version 3 of           *
 *   the License, or (at your option) any later version.                      *
 *                                                                            *
 * DFT-EFE is distributed in the hope that it will be useful, but             *
 *   WITHOUT ANY WARRANTY; without even the implied warranty                  *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.                     *
 *   See the Lesser GNU General Public License for more details.              *
 *                                                                            *
 * You should have received a copy of the GNU Lesser General Public           *
 *   License at the top level of DFT-EFE distribution.  If not, see           *
 *   <https://www.gnu.org/licenses/>.                                         *
 ******************************************************************************/

/*
 * @author Ian C. Lin, Sambit Das.
 */

#ifndef dftefeVectorStorage_h
#define dftefeVectorStorage_h

#include <utils/MemoryManager.h>
#include <utils/TypeConfig.h>
#include <vector>
namespace dftefe
{
  namespace utils
  {
    template <typename ValueType, dftefe::utils::MemorySpace memorySpace>
    class VectorStorage
    {
      /**
       * @brief A class template to provide an interface that can act similar to
       * STL vectors but with different MemorySpace---
       * HOST (cpu) , DEVICE (gpu), etc,.
       *
       * @tparam ValueType The underlying value type for the VectorStorage
       *  (e.g., int, double, complex<double>, etc.)
       * @tparam memorySpace The memory space in which the VectorStorage needs
       *  to reside
       *
       */
      typedef ValueType        value_type;
      typedef ValueType *      pointer;
      typedef ValueType &      reference;
      typedef const ValueType &const_reference;
      typedef ValueType *      iterator;
      typedef const ValueType *const_iterator;

    public:
      VectorStorage() = default;

      /**
       * @brief Copy constructor for a VectorStorage
       * @param[in] u VectorStorage object to copy from
       */
      VectorStorage(const VectorStorage &u);

      /**
       * @brief Move constructor for a VectorStorage
       * @param[in] u VectorStorage object to move from
       */
      VectorStorage(VectorStorage &&u) noexcept;

      /**
       * @brief Constructor for VectorStorage with size and initial value arguments
       * @param[in] size size of the VectorStorage
       * @param[in] initVal initial value of elements of the VectorStorage
       */
      explicit VectorStorage(size_type size, ValueType initVal = 0);

      /**
       * @brief Destructor
       */
      ~VectorStorage();


      /**
       * @brief Return iterator pointing to the beginning of VectorStorage
       * data.
       *
       * @returns Iterator pointing to the beginning of VectorStorage.
       */
      iterator
      begin();

      /**
       * @brief Return iterator pointing to the beginning of VectorStorage
       * data.
       *
       * @returns Constant iterator pointing to the beginning of
       * VectorStorage.
       */
      const_iterator
      begin() const;

      /**
       * @brief Return iterator pointing to the end of VectorStorage data.
       *
       * @returns Iterator pointing to the end of VectorStorage.
       */
      iterator
      end();

      /**
       * @brief Return iterator pointing to the end of VectorStorage data.
       *
       * @returns Constant iterator pointing to the end of
       * VectorStorage.
       */
      const_iterator
      end() const;


      /**
       * @brief Copy assignment operator
       * @param[in] rhs the rhs VectorStorage from which to copy
       * @returns reference to the lhs VectorStorage
       */
      VectorStorage &
      operator=(const VectorStorage &rhs);


      /**
       * @brief Move assignment constructor
       * @param[in] rhs the rhs VectorStorage from which to move
       * @returns reference to the lhs VectorStorage
       */
      VectorStorage &
      operator=(VectorStorage &&rhs) noexcept;

      //      // This part does not work for GPU version, will work on this
      //      until
      //      // having cleaner solution.
      //      /**
      //       * @brief Operator to get a reference to a element of the Vector
      //       * @param[in] i is the index to the element of the Vector
      //       * @returns reference to the element of the Vector
      //       * @throws exception if i >= size of the Vector
      //       */
      //      reference
      //      operator[](size_type i);
      //
      //      /**
      //       * @brief Operator to get a const reference to a element of the Vector
      //       * @param[in] i is the index to the element of the Vector
      //       * @returns const reference to the element of the Vector
      //       * @throws exception if i >= size of the Vector
      //       */
      //      const_reference
      //      operator[](size_type i) const;

      /**
       * @brief Deallocates and then resizes VectorStorage with new size
       * and initial value arguments
       * @param[in] size size of the VectorStorage
       * @param[in] initVal initial value of elements of the VectorStorage
       */
      void
      resize(size_type size, ValueType initVal = ValueType());

      /**
       * @brief Returns the dimension of the VectorStorage
       * @returns size of the VectorStorage
       */
      size_type
      size() const;

      /**
       * @brief Return the raw pointer to the VectorStorage
       * @return pointer to data
       */
      ValueType *
      data();

      /**
       * @brief Return the raw pointer to the VectorStorage without modifying the values
       * @return pointer to const data
       */
      const ValueType *
      data() const;

      /**
       * @brief Copies the data to a VectorStorage object in a different memory space.
       * This provides a seamless interface to copy back and forth between
       * memory spaces , including between the same memory spaces.
       *
       * @note The destination VectorStorage must be pre-allocated appropriately
       *
       * @tparam memorySpaceDst memory space of the destination VectorStorage
       * @param[in] dstVectorStorage reference to the destination
       *  VectorStorage. It must be pre-allocated appropriately
       * @param[out] dstVectorStorage reference to the destination
       *  VectorStorage with the data copied into it
       */
      template <dftefe::utils::MemorySpace memorySpaceDst>
      void
      copyTo(VectorStorage<ValueType, memorySpaceDst> &dstVectorStorage) const;

      /**
       * @brief Copies the data to a VectorStorage object in a different memory space.
       * This provides a seamless interface to copy back and forth between
       * memory spaces , including between the same memory spaces. This is a
       * more granular version of the above copyTo function as it provides
       * transfer from a specific portion of the source VectorStorage to a
       * specific portion of the destination VectorStorage.
       *
       * @note The destination VectorStorage must be pre-allocated appropriately
       *
       * @tparam memorySpaceDst memory space of the destination VectorStorage
       * @param[in] dstVectorStorage reference to the destination
       *  VectorStorage. It must be pre-allocated appropriately
       * @param[in] N number of entries of the source VectorStorage
       *  that needs to be copied to the destination VectorStorage
       * @param[in] srcOffset offset relative to the start of the source
       *  VectorStorage from which we need to copy data
       * @param[in] dstOffset offset relative to the start of the destination
       *  VectorStorage to which we need to copy data
       * @param[out] dstVectorStorage reference to the destination
       *  VectorStorage with the data copied into it
       */
      template <dftefe::utils::MemorySpace memorySpaceDst>
      void
      copyTo(VectorStorage<ValueType, memorySpaceDst> &dstVectorStorage,
             const size_type                           N,
             const size_type                           srcOffset,
             const size_type                           dstOffset) const;

      /**
       * @brief Copies data from a VectorStorage object in a different memory space.
       * This provides a seamless interface to copy back and forth between
       * memory spaces, including between the same memory spaces.
       *
       * @note The VectorStorage must be pre-allocated appropriately
       *
       * @tparam memorySpaceSrc memory space of the source VectorStorage
       *  from which to copy
       * @param[in] srcVectorStorage reference to the source
       *  VectorStorage
       */
      template <dftefe::utils::MemorySpace memorySpaceSrc>
      void
      copyFrom(
        const VectorStorage<ValueType, memorySpaceSrc> &srcVectorStorage);

      /**
       * @brief Copies data from a VectorStorage object in a different memory space.
       * This provides a seamless interface to copy back and forth between
       * memory spaces, including between the same memory spaces.
       * This is a more granular version of the above copyFrom function as it
       * provides transfer from a specific portion of the source VectorStorage
       * to a specific portion of the destination VectorStorage.
       *
       * @note The VectorStorage must be pre-allocated appropriately
       *
       * @tparam memorySpaceSrc memory space of the source VectorStorage
       *  from which to copy
       * @param[in] srcVectorStorage reference to the source
       *  VectorStorage
       * @param[in] N number of entries of the source VectorStorage
       *  that needs to be copied to the destination VectorStorage
       * @param[in] srcOffset offset relative to the start of the source
       *  VectorStorage from which we need to copy data
       * @param[in] dstOffset offset relative to the start of the destination
       *  VectorStorage to which we need to copy data
       */
      template <dftefe::utils::MemorySpace memorySpaceSrc>
      void
      copyFrom(VectorStorage<ValueType, memorySpaceSrc> &srcVectorStorage,
               const size_type                           N,
               const size_type                           srcOffset,
               const size_type                           dstOffset);

    private:
      ValueType *d_data = nullptr;
      size_type  d_size = 0;
    };

  } // namespace utils
} // end of namespace dftefe

#include "VectorStorage.t.cpp"

#endif
