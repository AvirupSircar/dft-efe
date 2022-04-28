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
 * @author Bikash Kanungo
 */

#ifndef dftefeBasisPartitioner_h
#define dftefeBasisPartitioner_h

#include <utils/TypeConfig.h>
#include <utils/MemoryStorage.h>
#include <string>
namespace dftefe
{
  namespace basis
  {

    /**
     * @brief An abstract class to encapsulate the partitioning 
     * of a basis across multiple processors
     * 
     * @tparam template parameter memorySpace defines the MemorySpace (i.e., HOST or
     * DEVICE) in which the data must reside.
     */
    template <dftefe::utils::MemorySpace memorySpace>
      class BasisPartitioner {

	//
	// typedefs
	//
	public:
	  using SizeTypeVector = utils::MemoryStorage<size_type, memorySpace>;
	  using GlobalSizeTypeVector =
	    utils::MemoryStorage<global_size_type, memorySpace>;

	public:
	  ~FEBasisPartitioner() = default;

	  std::pair<global_size_type, global_size_type>
	    getLocallyOwnedRange(const std::string constraintsName) const  = 0;

	  const GlobalSizeTypeVector & 
	    getGhostIndices(const std::string constraintsName) const = 0;

	  size_type
	    nLocalSize(const std::string constraintsName) const  = 0;

	  size_type
	    nLocallyOwnedSize(const std::string constraintsName) const  = 0;

	  size_type
	    nGhostSize(const std::string constraintsName) const  = 0;

	  bool
	    inLocallyOwnedRange(const global_size_type, const std::string constraintsName) const = 0;

	  bool
	    isGhostEntry(const global_size_type, const std::string constraintsName) const = 0;

	  size_type
	    globalToLocal(const global_size_type, const std::string constraintsName) const = 0;

	  global_size_type
	    localToGlobal(const size_type, const std::string constraintsName) const = 0;
      };

  } // end of namespace basis
} // end of namespace dftefe
#endif // dftefeBasisPartitioner_h
