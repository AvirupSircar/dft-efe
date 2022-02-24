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
 * @author Sambit Das
 */

#ifndef dftefeMPICommunicatorP2PKernels_h
#define dftefeMPICommunicatorP2PKernels_h

#include <utils/MemorySpaceType.h>
#include <utils/MemoryStorage.h>
#include <utils/TypeConfig.h>

namespace dftefe
{
  namespace utils
  {
    template <typename ValueType, dftefe::utils::MemorySpace memorySpace>
    class MPICommunicatorP2PKernels
    {
      public:

      using SizeTypeVector = utils::MemoryStorage<size_type, memorySpace>;

      /**
       * @brief Function template for architecture adaptable gather kernel to send buffer
       * @tparam ValueType the type of the number
       * @tparam memorySpace
       * @param[in] dataArray data array with locally owned entries
       * @param[in] ownedLocalIndicesForTargetProcs
       * @param[in] numOwnedIndicesForTargetProcs
       * @param[in] blockSize
       * @param[out] sendBuffer
       */
      static void
      gatherLocallyOwnedEntriesToSendBuffer(const MemoryStorage<ValueType, memorySpace> &dataArray,
                                            const SizeTypeVector & ownedLocalIndicesForTargetProcs,
                                            const SizeTypeVector & numOwnedIndicesForTargetProcs,
                                            const size_type blockSize,
                                            MemoryStorage<ValueType, memorySpace> &sendBuffer);

      /**
       * @brief Function template for architecture adaptable accumlate kernel from recv buffer
       * @tparam ValueType the type of the number
       * @tparam memorySpace
       * @param[in] recvBuffer 
       * @param[in] ownedLocalIndicesForTargetProcs
       * @param[in] numOwnedIndicesForTargetProcs
       * @param[in] blockSize 
       * @param[out] dataArray
       */
      static void
      accumulateAddRecvBufferToLocallyOwnedEntries(const MemoryStorage<ValueType, memorySpace> &recvBuffer,
                                            const SizeTypeVector & ownedLocalIndicesForTargetProcs,
                                            const SizeTypeVector & numOwnedIndicesForTargetProcs,
                                            const size_type blockSize,
                                            MemoryStorage<ValueType, memorySpace> &dataArray);

    };

  } // namespace utils
} // namespace dftefe


#endif
