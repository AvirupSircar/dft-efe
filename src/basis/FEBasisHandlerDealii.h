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
 * @author Bikash Kanungo, Vishal Subramanian
 */

#ifndef dftefeFEBasisHandlerDealii_h
#define dftefeFEBasisHandlerDealii_h

#include <basis/FEBasisHandler.h>
#include <basis/BasisManager.h>
#include <basis/FEBasisManagerDealii.h>
#include <basis/Constraints.h>
#include <basis/FEConstraintsDealii.h>
#include <utils/MPIPatternP2P.h>
#include <memory>
#include <map>
#include <deal.II/matrix_free/matrix_free.h>
namespace dftefe
{
  namespace basis
  {
    /**
     * @brief An abstract class to encapsulate the partitioning
     * of a finite element basis across multiple processors
     */
    template <typename ValueType,
              dftefe::utils::MemorySpace memorySpace,
              size_type                  dim>
    class FEBasisHandlerDealii
      : public FEBasisHandler<ValueType, memorySpace, dim>
    {
      //
      // typedefs
      //
    public:
      using SizeTypeVector = utils::MemoryStorage<size_type, memorySpace>;
      using GlobalSizeTypeVector =
        utils::MemoryStorage<global_size_type, memorySpace>;
      using LocalIndexIter       = typename SizeTypeVector::iterator;
      using const_LocalIndexIter = typename SizeTypeVector::const_iterator;
      using GlobalIndexIter      = typename GlobalSizeTypeVector::iterator;
      using const_GlobalIndexIter =
        typename GlobalSizeTypeVector::const_iterator;

    public:
      FEBasisHandlerDealii(
        std::shared_ptr<const BasisManager> basisManager,
        std::map<std::string,
                 std::shared_ptr<const Constraints<ValueType, memorySpace>>>
                                   constraintsMap,
        const utils::mpi::MPIComm &mpiComm);
      void
      reinit(
        std::shared_ptr<const BasisManager> basisManager,
        std::map<std::string,
                 std::shared_ptr<const Constraints<ValueType, memorySpace>>>
                                   constraintsMap,
        const utils::mpi::MPIComm &mpiComm);

      FEBasisHandlerDealii(
        std::shared_ptr<const BasisManager> basisManager,
        std::map<std::string,
                 std::shared_ptr<const Constraints<ValueType, memorySpace>>>
          constraintsMap);
      void
      reinit(
        std::shared_ptr<const BasisManager> basisManager,
        std::map<std::string,
                 std::shared_ptr<const Constraints<ValueType, memorySpace>>>
          constraintsMap);

      ~FEBasisHandlerDealii() = default;

      const BasisManager &
      getBasisManager() const override;

      bool
      isDistributed() const override;

      const Constraints<ValueType, memorySpace> &
      getConstraints(const std::string constraintsName) const override;

      std::shared_ptr<const utils::mpi::MPIPatternP2P<memorySpace>>
      getMPIPatternP2P(const std::string constraintsName) const override;

      std::pair<global_size_type, global_size_type>
      getLocallyOwnedRange(const std::string constraintsName) const override;

      size_type
      nLocallyOwned(const std::string constraintsName) const override;


      const GlobalSizeTypeVector &
      getGhostIndices(const std::string constraintsName) const override;

      size_type
      nLocal(const std::string constraintsName) const override;


      size_type
      nGhost(const std::string constraintsName) const override;

      bool
      inLocallyOwnedRange(const global_size_type globalId,
                          const std::string constraintsName) const override;

      bool
      isGhostEntry(const global_size_type ghostId,
                   const std::string      constraintsName) const override;

      size_type
      globalToLocalIndex(const global_size_type globalId,
                         const std::string      constraintsName) const override;

      global_size_type
      localToGlobalIndex(const size_type   localId,
                         const std::string constraintsName) const override;

      void
      getBasisCenters(const size_type       localId,
                      const std::string     constraintsName,
                      dftefe::utils::Point &basisCenter) const override;

      //
      // FE specific functions
      //
      size_type
      nLocallyOwnedCells() const override;

      size_type
      nLocallyOwnedCellDofs(const size_type cellId) const override;

      size_type
      nCumulativeLocallyOwnedCellDofs() const override;

      const_GlobalIndexIter
      locallyOwnedCellGlobalDofIdsBegin(
        const std::string constraintsName) const override;

      const_GlobalIndexIter
      locallyOwnedCellGlobalDofIdsBegin(
        const size_type   cellId,
        const std::string constraintsName) const override;

      const_GlobalIndexIter
      locallyOwnedCellGlobalDofIdsEnd(
        const size_type   cellId,
        const std::string constraintsName) const override;

      const_LocalIndexIter
      locallyOwnedCellLocalDofIdsBegin(
        const std::string constraintsName) const override;

      const_LocalIndexIter
      locallyOwnedCellLocalDofIdsBegin(
        const size_type   cellId,
        const std::string constraintsName) const override;

      const_LocalIndexIter
      locallyOwnedCellLocalDofIdsEnd(
        const size_type   cellId,
        const std::string constraintsName) const override;

      //
      // dealii specific functions
      //

    private:
      std::shared_ptr<const FEBasisManagerDealii<dim>> d_feBMDealii;
      std::map<
        std::string,
        std::shared_ptr<const FEConstraintsDealii<ValueType, memorySpace, dim>>>
                                                    d_feConstraintsDealiiOptMap;
      utils::mpi::MPIComm                           d_mpiComm;
      bool                                          d_isDistributed;
      std::pair<global_size_type, global_size_type> d_locallyOwnedRange;
      std::vector<size_type>                        d_locallyOwnedCellStartIds;
      GlobalSizeTypeVector   d_locallyOwnedCellGlobalIndices;
      std::vector<size_type> d_numLocallyOwnedCellDofs;

      // constraints dependent data
      std::map<std::string, std::shared_ptr<GlobalSizeTypeVector>>
        d_ghostIndicesMap;
      std::map<std::string,
               std::shared_ptr<utils::mpi::MPIPatternP2P<memorySpace>>>
        d_mpiPatternP2PMap;
      std::map<std::string, std::shared_ptr<SizeTypeVector>>
        d_locallyOwnedCellLocalIndicesMap;

      std::map<global_size_type, dealii::Point<dim>> d_supportPoints;
    };

  } // end of namespace basis
} // end of namespace dftefe
#include <basis/FEBasisHandlerDealii.t.cpp>
#endif // dftefeFEBasisHandlerDealii_h
