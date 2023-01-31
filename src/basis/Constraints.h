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
 * @author Vishal Subramanian
 */

#ifndef dftefeConstraints_h
#define dftefeConstraints_h

#include <utils/TypeConfig.h>
#include <utils/MPIPatternP2P.h>

#include <linearAlgebra/Vector.h>
namespace dftefe
{
  namespace basis
  {
    /**
     * An abstract class to handle the constraints related to a basis
     */
    template <typename ValueTypeBasisCoeff,
              dftefe::utils::MemorySpace memorySpace>
    class Constraints
    {
    public:
      ~Constraints() = default;
      virtual void
      clear() = 0;
      virtual void
      setInhomogeneity(global_size_type    basisId,
                       ValueTypeBasisCoeff constraintValue) = 0;
      virtual void
      close() = 0;
      virtual bool
      isClosed() const = 0;
      virtual void
      setHomogeneousDirichletBC() = 0;
      virtual bool
      isConstrained(global_size_type basisId) const = 0;

      virtual const std::vector<
        std::pair<global_size_type, ValueTypeBasisCoeff>> *
      getConstraintEntries(const global_size_type lineDof) const = 0;

      virtual bool
      isInhomogeneouslyConstrained(const global_size_type index) const = 0;

      virtual ValueTypeBasisCoeff
      getInhomogeneity(const global_size_type lineDof) const = 0;

      virtual void
      copyConstraintsData(
        const Constraints<ValueTypeBasisCoeff, memorySpace> &constraintsDataIn,
        const utils::mpi::MPIPatternP2P<memorySpace> &       mpiPattern) = 0;
      virtual void
      populateConstraintsData(
        const utils::mpi::MPIPatternP2P<memorySpace> &mpiPattern) = 0;

      virtual void
      distributeChildToParent(
        linearAlgebra::Vector<ValueTypeBasisCoeff, memorySpace> &vectorData,
        size_type blockSize = 1) const = 0;
      virtual void
      distributeParentToChild(
        linearAlgebra::MultiVector<ValueTypeBasisCoeff, memorySpace>
          &       vectorData,
        size_type blockSize) const = 0;
      virtual void
      setConstrainedNodesToZero(
        linearAlgebra::Vector<ValueTypeBasisCoeff, memorySpace> &vectorData,
        size_type blockSize = 1) const = 0;

      virtual void
      setConstrainedNodesToZero(
        linearAlgebra::MultiVector<ValueTypeBasisCoeff, memorySpace>
          &       vectorData,
        size_type blockSize) const = 0;
    };

  } // namespace basis
} // namespace dftefe

#endif // dftefeConstraints_h
