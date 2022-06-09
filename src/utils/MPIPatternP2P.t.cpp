
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

#include <utils/Exceptions.h>
#include <utils/MPITags.h>
#include <utils/MPIRequestersBase.h>
#include <utils/MPIRequestersNBX.h>
#include <utils/MPIWrapper.h>
#include <utils/MPIErrorCodeHandler.h>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <memory>
#include <numeric>

namespace dftefe
{
  namespace utils
  {
    namespace mpi
    {
      namespace
      {
        void
        getAllOwnedRanges(const global_size_type         ownedRangeStart,
                          const global_size_type         ownedRangeEnd,
                          std::vector<global_size_type> &allOwnedRanges,
                          const MPIComm &                mpiComm)
        {
          int         nprocs = 1;
          int         err    = MPICommSize(mpiComm, &nprocs);
          std::string errMsg = "Error occured while using MPI_Comm_size. "
                               "Error code: " +
                               std::to_string(err);
          throwException(err == MPISuccess, errMsg);
          std::vector<int> recvCounts(nprocs, 2);
          std::vector<int> displs(nprocs, 0);
          allOwnedRanges.resize(2 * nprocs);
          for (unsigned int i = 0; i < nprocs; ++i)
            displs[i] = 2 * i;

          std::vector<global_size_type> ownedRanges = {ownedRangeStart,
                                                       ownedRangeEnd};
          MPIAllgatherv<MemorySpace::HOST>(&ownedRanges[0],
                                           2,
                                           MPIUnsignedLong,
                                           &allOwnedRanges[0],
                                           &recvCounts[0],
                                           &displs[0],
                                           MPIUnsignedLong,
                                           mpiComm);
        }

        void
        getGhostProcIdToLocalGhostIndicesMap(
          const std::vector<global_size_type> &ghostIndices,
          const std::vector<global_size_type> &allOwnedRanges,
          std::map<size_type, std::vector<size_type>>
            &            ghostProcIdToLocalGhostIndices,
          const MPIComm &mpiComm)
        {
          int         nprocs = 1;
          int         err    = MPICommSize(mpiComm, &nprocs);
          std::string errMsg = "Error occured while using MPI_Comm_size. "
                               "Error code: " +
                               std::to_string(err);
          throwException(err == MPISuccess, errMsg);

          //
          // NOTE: The locally owned ranges need not be ordered as per the
          // processor ranks. That is ranges for processor 0, 1, ...., P-1 given
          // by [N_0,N_1), [N_1, N_2), [N_2, N_3), ..., [N_{P-1},N_P) need not
          // honor the fact that N_0, N_1, ..., N_P are increasing. However, it
          // is more efficient to perform search operations in a sorted vector.
          // Thus, we perform a sort on the end of each locally owned range and
          // also keep track of the indices during the sort
          //

          // vector to store the end of each locally owned ranges
          std::vector<global_size_type> locallyOwnedRangesEnd(nprocs);

          //
          // Vector to keep track of the  indices of locallyOwnedRangesEnd
          // during sort
          std::vector<size_type> locallyOwnedRangesEndProcIds(nprocs);
          for (unsigned int i = 0; i < nprocs; ++i)
            {
              locallyOwnedRangesEnd[i]        = allOwnedRanges[2 * i + 1];
              locallyOwnedRangesEndProcIds[i] = i;
            }

          std::sort(locallyOwnedRangesEndProcIds.begin(),
                    locallyOwnedRangesEndProcIds.end(),
                    [&locallyOwnedRangesEnd](size_type x, size_type y) {
                      return locallyOwnedRangesEnd[x] <
                             locallyOwnedRangesEnd[y];
                    });

          std::sort(locallyOwnedRangesEnd.begin(), locallyOwnedRangesEnd.end());

          const size_type numGhosts = ghostIndices.size();
          for (unsigned int iGhost = 0; iGhost < numGhosts; ++iGhost)
            {
              global_size_type ghostIndex = ghostIndices[iGhost];
              auto        up  = std::upper_bound(locallyOwnedRangesEnd.begin(),
                                         locallyOwnedRangesEnd.end(),
                                         ghostIndex);
              std::string msg = "Ghost index " + std::to_string(ghostIndex) +
                                " not found in any of the processors";
              throwException(up != locallyOwnedRangesEnd.end(), msg);
              size_type upPos =
                std::distance(locallyOwnedRangesEnd.begin(), up);
              size_type procId = locallyOwnedRangesEndProcIds[upPos];
              ghostProcIdToLocalGhostIndices[procId].push_back(iGhost);
            }
        }

        bool
        checkContiguity(const std::vector<size_type> &v)
        {
          const size_type N           = v.size();
          bool            returnValue = true;
          for (unsigned int i = 1; i < N; ++i)
            {
              if ((v[i] - 1) != v[i - 1])
                {
                  returnValue = false;
                  break;
                }
            }
          return returnValue;
        }

        struct RangeMetaData
        {
          global_size_type Id;
          size_type        rangeId;
          bool             isRangeStart;
        };

        bool
        compareRangeMetaData(const RangeMetaData &x, const RangeMetaData &y)
        {
          if (x.Id == y.Id)
            return (!x.isRangeStart);
          else
            return x.Id < y.Id;
        }
        std::vector<size_type>
        getOverlappingRangeIds(const std::vector<global_size_type> &ranges)
        {
          size_type                  numRanges = ranges.size() / 2;
          std::vector<RangeMetaData> rangeMetaDataVec(0);
          for (unsigned int i = 0; i < numRanges; ++i)
            {
              RangeMetaData left;
              left.Id           = ranges[2 * i];
              left.rangeId      = i;
              left.isRangeStart = true;

              RangeMetaData right;
              right.Id           = ranges[2 * i + 1];
              right.rangeId      = i;
              right.isRangeStart = false;

              rangeMetaDataVec.push_back(left);
              rangeMetaDataVec.push_back(right);
            }
          std::sort(rangeMetaDataVec.begin(),
                    rangeMetaDataVec.end(),
                    compareRangeMetaData);
          int                    currentOpen = -1;
          bool                   added       = false;
          std::vector<size_type> returnValue(0);
          for (unsigned int i = 0; i < rangeMetaDataVec.size(); ++i)
            {
              size_type rangeId = rangeMetaDataVec[i].rangeId;
              if (rangeMetaDataVec[i].isRangeStart)
                {
                  if (currentOpen == -1)
                    {
                      currentOpen = rangeId;
                      added       = false;
                    }
                  else
                    {
                      if (!added)
                        {
                          returnValue.push_back(currentOpen);
                          added = true;
                        }
                      returnValue.push_back(rangeId);
                      if (ranges[2 * rangeId + 1] > ranges[2 * currentOpen + 1])
                        {
                          currentOpen = rangeId;
                          added       = true;
                        }
                    }
                }
              else
                {
                  if (rangeId == currentOpen)
                    {
                      currentOpen = -1;
                      added       = false;
                    }
                }
            }
          return returnValue;
        }

      } // namespace

#ifdef DFTEFE_WITH_MPI
      ///
      /// Constructor with MPI
      ///
      template <dftefe::utils::MemorySpace memorySpace>
      MPIPatternP2P<memorySpace>::MPIPatternP2P(
        const std::pair<global_size_type, global_size_type> &locallyOwnedRange,
        const std::vector<dftefe::global_size_type> &        ghostIndices,
        const MPIComm &                                      mpiComm)
        : d_locallyOwnedRange(locallyOwnedRange)
        , d_mpiComm(mpiComm)
        , d_allOwnedRanges(0)
        , d_ghostIndices(0)
        , d_numLocallyOwnedIndices(0)
        , d_numGhostIndices(0)
        , d_numGhostProcs(0)
        , d_ghostProcIds(0)
        , d_numGhostIndicesInGhostProcs(0)
        , d_localGhostIndicesRanges(0)
        , d_numTargetProcs(0)
        , d_flattenedLocalGhostIndices(0)
        , d_targetProcIds(0)
        , d_numOwnedIndicesForTargetProcs(0)
        , d_flattenedLocalTargetIndices(0)
        , d_nGlobalIndices(0)
      {
        d_myRank           = 0;
        d_nprocs           = 1;
        int         err    = MPICommSize(d_mpiComm, &d_nprocs);
        std::string errMsg = "Error occured while using MPI_Comm_size. "
                             "Error code: " +
                             std::to_string(err);
        throwException(err == MPISuccess, errMsg);

        err    = MPICommRank(d_mpiComm, &d_myRank);
        errMsg = "Error occured while using MPI_Comm_rank. "
                 "Error code: " +
                 std::to_string(err);
        throwException(err == MPISuccess, errMsg);

        throwException(
          d_locallyOwnedRange.second >= d_locallyOwnedRange.first,
          "In processor " + std::to_string(d_myRank) +
            ", invalid locally owned range found "
            "(i.e., the second value in the range is less than the first value).");
        d_numLocallyOwnedIndices =
          d_locallyOwnedRange.second - d_locallyOwnedRange.first;
        ///////////////////////////////////////////////////
        //////////// Ghost Data Evaluation Begin //////////
        ///////////////////////////////////////////////////

        //
        // check if the ghostIndices is strictly increasing or nor
        //
        bool isStrictlyIncreasing = std::is_sorted(ghostIndices.begin(),
                                                   ghostIndices.end(),
                                                   std::less_equal<>());
        throwException(
          isStrictlyIncreasing,
          "In processor " + std::to_string(d_myRank) +
            ", the ghost indices passed to MPIPatternP2P is not a strictly increasing set.");

        d_ghostIndices = ghostIndices;

        // copy the ghostIndices to d_ghostIndicesSetSTL
        d_ghostIndicesSetSTL.clear();
        std::copy(ghostIndices.begin(),
                  ghostIndices.end(),
                  std::inserter(d_ghostIndicesSetSTL,
                                d_ghostIndicesSetSTL.end()));
        d_ghostIndicesOptimizedIndexSet =
          OptimizedIndexSet<global_size_type>(d_ghostIndicesSetSTL);

        d_numGhostIndices = ghostIndices.size();

        MemoryTransfer<memorySpace, MemorySpace::HOST> memoryTransfer;

        d_allOwnedRanges.clear();
        getAllOwnedRanges(d_locallyOwnedRange.first,
                          d_locallyOwnedRange.second,
                          d_allOwnedRanges,
                          d_mpiComm);

        std::vector<size_type> overlappingRangeIds =
          getOverlappingRangeIds(d_allOwnedRanges);
        throwException<LogicError>(
          overlappingRangeIds.size() == 0,
          "Detected overlapping ranges among the locallyOwnedRanges passed "
          "to MPIPatternP2P");

        for (unsigned int i = 0; i < d_nprocs; ++i)
          d_nGlobalIndices +=
            d_allOwnedRanges[2 * i + 1] - d_allOwnedRanges[2 * i];

        std::map<size_type, std::vector<size_type>>
          ghostProcIdToLocalGhostIndices;
        getGhostProcIdToLocalGhostIndicesMap(ghostIndices,
                                             d_allOwnedRanges,
                                             ghostProcIdToLocalGhostIndices,
                                             d_mpiComm);

        d_numGhostProcs = ghostProcIdToLocalGhostIndices.size();
        d_ghostProcIds.resize(d_numGhostProcs);
        d_numGhostIndicesInGhostProcs.resize(d_numGhostProcs);
        d_localGhostIndicesRanges.resize(2 * d_numGhostProcs);

        std::vector<size_type> flattenedLocalGhostIndicesTmp(0);
        auto                   it = ghostProcIdToLocalGhostIndices.begin();
        unsigned int           iGhostProc = 0;
        for (; it != ghostProcIdToLocalGhostIndices.end(); ++it)
          {
            d_ghostProcIds[iGhostProc] = it->first;
            const std::vector<size_type> localGhostIndicesInGhostProc =
              it->second;
            bool isContiguous = checkContiguity(localGhostIndicesInGhostProc);
            std::string msg   = "In rank " + std::to_string(d_myRank) +
                              ", the local ghost indices that are owned"
                              " by rank " +
                              std::to_string(d_ghostProcIds[iGhostProc]) +
                              " does not form a contiguous set.";
            throwException<LogicError>(isContiguous, msg);
            d_numGhostIndicesInGhostProcs[iGhostProc] =
              localGhostIndicesInGhostProc.size();

            d_localGhostIndicesRanges[2 * iGhostProc] =
              *(localGhostIndicesInGhostProc.begin());
            d_localGhostIndicesRanges[2 * iGhostProc + 1] =
              *(localGhostIndicesInGhostProc.end() - 1) + 1;
            //
            // Append localGhostIndicesInGhostProc to
            // flattenedLocalGhostIndicesTmp
            //
            std::copy(localGhostIndicesInGhostProc.begin(),
                      localGhostIndicesInGhostProc.end(),
                      back_inserter(flattenedLocalGhostIndicesTmp));
            ++iGhostProc;
          }

        std::string msg = "In rank " + std::to_string(d_myRank) +
                          " mismatch of"
                          " the sizes of ghost indices. Expected size: " +
                          std::to_string(d_numGhostIndices) +
                          " Received size: " +
                          std::to_string(flattenedLocalGhostIndicesTmp.size());
        throwException<DomainError>(flattenedLocalGhostIndicesTmp.size() ==
                                      d_numGhostIndices,
                                    msg);


        d_flattenedLocalGhostIndices.resize(d_numGhostIndices);
        memoryTransfer.copy(d_numGhostIndices,
                            d_flattenedLocalGhostIndices.begin(),
                            &flattenedLocalGhostIndicesTmp[0]);
        ///////////////////////////////////////////////////
        //////////// Ghost Data Evaluation End / //////////
        ///////////////////////////////////////////////////


        ///////////////////////////////////////////////////
        //////////// Target Data Evaluation Begin ////////
        ///////////////////////////////////////////////////
        std::shared_ptr<MPIRequestersBase> mpirequesters =
          std::make_shared<MPIRequestersNBX>(d_ghostProcIds, d_mpiComm);
        d_targetProcIds  = mpirequesters->getRequestingRankIds();
        d_numTargetProcs = d_targetProcIds.size();
        d_numOwnedIndicesForTargetProcs.resize(d_numTargetProcs);

        std::vector<MPIRequest> sendRequests(d_numGhostProcs);
        std::vector<MPIStatus>  sendStatuses(d_numGhostProcs);
        std::vector<MPIRequest> recvRequests(d_numTargetProcs);
        std::vector<MPIStatus>  recvStatuses(d_numTargetProcs);
        const int tag = static_cast<int>(MPITags::MPI_P2P_PATTERN_TAG);
        for (unsigned int iGhost = 0; iGhost < d_numGhostProcs; ++iGhost)
          {
            const size_type numGhostIndicesInProc =
              d_numGhostIndicesInGhostProcs[iGhost];
            const int ghostProcId = d_ghostProcIds[iGhost];
            int       err = MPIIsend<MemorySpace::HOST>(&numGhostIndicesInProc,
                                                  1,
                                                  MPIUnsigned,
                                                  ghostProcId,
                                                  tag,
                                                  d_mpiComm,
                                                  &sendRequests[iGhost]);
            std::string errMsg = "Error occured while using MPI_Isend. "
                                 "Error code: " +
                                 std::to_string(err);
            throwException(err == MPISuccess, errMsg);
          }

        for (unsigned int iTarget = 0; iTarget < d_numTargetProcs; ++iTarget)
          {
            const int targetProcId = d_targetProcIds[iTarget];
            int       err          = MPIIrecv<MemorySpace::HOST>(
              &d_numOwnedIndicesForTargetProcs[iTarget],
              1,
              MPIUnsigned,
              targetProcId,
              tag,
              d_mpiComm,
              &recvRequests[iTarget]);
            std::string errMsg = "Error occured while using MPI_Irecv. "
                                 "Error code: " +
                                 std::to_string(err);
            throwException(err == MPISuccess, errMsg);
          }

        err =
          MPIWaitall(d_numGhostProcs, sendRequests.data(), sendStatuses.data());
        errMsg = "Error occured while using MPI_Waitall. "
                 "Error code: " +
                 std::to_string(err);
        throwException(err == MPISuccess, errMsg);

        err    = MPIWaitall(d_numTargetProcs,
                         recvRequests.data(),
                         recvStatuses.data());
        errMsg = "Error occured while using MPI_Waitall. "
                 "Error code: " +
                 std::to_string(err);
        throwException(err == MPISuccess, errMsg);

        size_type totalOwnedIndicesForTargetProcs =
          std::accumulate(d_numOwnedIndicesForTargetProcs.begin(),
                          d_numOwnedIndicesForTargetProcs.end(),
                          0);
        std::vector<size_type> flattenedLocalTargetIndicesTmp(
          totalOwnedIndicesForTargetProcs);

        size_type startIndex = 0;
        for (unsigned int iGhost = 0; iGhost < d_numGhostProcs; ++iGhost)
          {
            const int numGhostIndicesInProc =
              d_numGhostIndicesInGhostProcs[iGhost];
            const int ghostProcId = d_ghostProcIds[iGhost];

            // We need to send what is the local index in the ghost processor
            // (i.e., the processor that owns the current processor's ghost
            // index)
            std::vector<size_type> localIndicesForGhostProc(
              numGhostIndicesInProc);
            for (unsigned iIndex = 0; iIndex < numGhostIndicesInProc; ++iIndex)
              {
                const size_type ghostLocalIndex =
                  flattenedLocalGhostIndicesTmp[startIndex + iIndex];
                const global_size_type ghostGlobalIndex =
                  ghostIndices[ghostLocalIndex];
                const global_size_type ghostProcOwnedIndicesStart =
                  d_allOwnedRanges[2 * ghostProcId];
                localIndicesForGhostProc[iIndex] =
                  (size_type)(ghostGlobalIndex - ghostProcOwnedIndicesStart);
              }

            int err = MPIIsend<MemorySpace::HOST>(&localIndicesForGhostProc[0],
                                                  numGhostIndicesInProc,
                                                  MPIUnsigned,
                                                  ghostProcId,
                                                  tag,
                                                  d_mpiComm,
                                                  &sendRequests[iGhost]);
            std::string errMsg = "Error occured while using MPI_Isend. "
                                 "Error code: " +
                                 std::to_string(err);
            throwException(err == MPISuccess, errMsg);
            startIndex += numGhostIndicesInProc;
          }

        startIndex = 0;
        for (unsigned int iTarget = 0; iTarget < d_numTargetProcs; ++iTarget)
          {
            const int targetProcId = d_targetProcIds[iTarget];
            const int numOwnedIndicesForTarget =
              d_numOwnedIndicesForTargetProcs[iTarget];
            int err = MPIIrecv<MemorySpace::HOST>(
              &flattenedLocalTargetIndicesTmp[startIndex],
              numOwnedIndicesForTarget,
              MPIUnsigned,
              targetProcId,
              tag,
              d_mpiComm,
              &recvRequests[iTarget]);
            std::string errMsg = "Error occured while using MPI_Irecv. "
                                 "Error code: " +
                                 std::to_string(err);
            throwException(err == MPISuccess, errMsg);
            startIndex += numOwnedIndicesForTarget;
          }

        err =
          MPIWaitall(d_numGhostProcs, sendRequests.data(), sendStatuses.data());
        errMsg = "Error occured while using MPI_Waitall. "
                 "Error code: " +
                 std::to_string(err);
        throwException(err == MPISuccess, errMsg);

        err    = MPIWaitall(d_numTargetProcs,
                         recvRequests.data(),
                         recvStatuses.data());
        errMsg = "Error occured while using MPI_Waitall. "
                 "Error code: " +
                 std::to_string(err);
        throwException(err == MPISuccess, errMsg);


        d_flattenedLocalTargetIndices.resize(totalOwnedIndicesForTargetProcs);
        memoryTransfer.copy(totalOwnedIndicesForTargetProcs,
                            d_flattenedLocalTargetIndices.begin(),
                            &flattenedLocalTargetIndicesTmp[0]);

        ///////////////////////////////////////////////////
        //////////// Target Data Evaluation End ////////
        ///////////////////////////////////////////////////
      }

#else
      ///
      /// Constructor without MPI
      ///
      template <dftefe::utils::MemorySpace memorySpace>
      MPIPatternP2P<memorySpace>::MPIPatternP2P(
        const std::pair<global_size_type, global_size_type> &locallyOwnedRange,
        const std::vector<dftefe::global_size_type> &        ghostIndices,
        const MPIComm &                                      mpiComm)
        : d_locallyOwnedRange(locallyOwnedRange)
        , d_mpiComm(mpiComm)
        , d_allOwnedRanges(0)
        , d_numLocallyOwnedIndices(0)
        , d_numGhostIndices(0)
        , d_ghostIndices(0)
        , d_numGhostProcs(0)
        , d_ghostProcIds(0)
        , d_numGhostIndicesInGhostProcs(0)
        , d_localGhostIndicesRanges(0)
        , d_numTargetProcs(0)
        , d_flattenedLocalGhostIndices(0)
        , d_targetProcIds(0)
        , d_numOwnedIndicesForTargetProcs(0)
        , d_flattenedLocalTargetIndices(0)
        , d_nGlobalIndices(0)
      {
        d_myRank = 0;
        d_nprocs = 1;
        throwException(
          d_locallyOwnedRange.second >= d_locallyOwnedRange.first,
          "In processor " + std::to_string(d_myRank) +
            ", invalid locally owned range found "
            "(i.e., the second value in the range is less than the first value).");
        d_numLocallyOwnedIndices =
          d_locallyOwnedRange.second - d_locallyOwnedRange.first;
        std::vector<global_size_type> d_allOwnedRanges = {
          d_locallyOwnedRange.first, d_locallyOwnedRange.second};
        for (unsigned int i = 0; i < d_nprocs; ++i)
          d_nGlobalIndices +=
            d_allOwnedRanges[2 * i + 1] - d_allOwnedRanges[2 * i];
      }

#endif

      ///
      /// Constructor without MPI
      ///
      template <dftefe::utils::MemorySpace memorySpace>
      MPIPatternP2P<memorySpace>::MPIPatternP2P(
        const std::pair<global_size_type, global_size_type> &locallyOwnedRange)
        : d_locallyOwnedRange(locallyOwnedRange)
        , d_mpiComm(0)
        , d_allOwnedRanges(0)
        , d_numLocallyOwnedIndices(0)
        , d_numGhostIndices(0)
        , d_ghostIndices(0)
        , d_numGhostProcs(0)
        , d_ghostProcIds(0)
        , d_numGhostIndicesInGhostProcs(0)
        , d_localGhostIndicesRanges(0)
        , d_numTargetProcs(0)
        , d_flattenedLocalGhostIndices(0)
        , d_targetProcIds(0)
        , d_numOwnedIndicesForTargetProcs(0)
        , d_flattenedLocalTargetIndices(0)
        , d_nGlobalIndices(0)
      {
        d_myRank = 0;
        d_nprocs = 1;
        throwException(
          d_locallyOwnedRange.second >= d_locallyOwnedRange.first,
          "In processor " + std::to_string(d_myRank) +
            ", invalid locally owned range found "
            "(i.e., the second value in the range is less than the first value).");
        d_numLocallyOwnedIndices =
          d_locallyOwnedRange.second - d_locallyOwnedRange.first;
        std::vector<global_size_type> d_allOwnedRanges = {
          d_locallyOwnedRange.first, d_locallyOwnedRange.second};
        for (unsigned int i = 0; i < d_nprocs; ++i)
          d_nGlobalIndices +=
            d_allOwnedRanges[2 * i + 1] - d_allOwnedRanges[2 * i];
      }

      template <dftefe::utils::MemorySpace memorySpace>
      std::pair<global_size_type, global_size_type>
      MPIPatternP2P<memorySpace>::getLocallyOwnedRange() const
      {
        return d_locallyOwnedRange;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      const std::vector<global_size_type> &
      MPIPatternP2P<memorySpace>::getGhostIndices() const
      {
        return d_ghostIndices;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      const std::vector<size_type> &
      MPIPatternP2P<memorySpace>::getGhostProcIds() const
      {
        return d_ghostProcIds;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      const std::vector<size_type> &
      MPIPatternP2P<memorySpace>::getNumGhostIndicesInProcs() const
      {
        return d_numGhostIndicesInGhostProcs;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      const std::vector<size_type> &
      MPIPatternP2P<memorySpace>::getGhostLocalIndicesRanges() const
      {
        return d_localGhostIndicesRanges;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      size_type
      MPIPatternP2P<memorySpace>::getNumGhostIndicesInProc(
        const size_type procId) const
      {
        auto      itProcIds             = d_ghostProcIds.begin();
        auto      itNumGhostIndices     = d_numGhostIndicesInGhostProcs.begin();
        size_type numGhostIndicesInProc = 0;
        for (; itProcIds != d_ghostProcIds.end(); ++itProcIds)
          {
            numGhostIndicesInProc = *itNumGhostIndices;
            if (procId == *itProcIds)
              break;

            ++itNumGhostIndices;
          }

        std::string msg =
          "The processor Id " + std::to_string(procId) +
          " does not contain any ghost indices for the current processor"
          " (i.e., processor Id " +
          std::to_string(d_myRank) + ")";
        throwException<InvalidArgument>(itProcIds != d_ghostProcIds.end(), msg);

        return numGhostIndicesInProc;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      typename MPIPatternP2P<memorySpace>::SizeTypeVector
      MPIPatternP2P<memorySpace>::getGhostLocalIndices(
        const size_type procId) const
      {
        size_type cumulativeIndices     = 0;
        size_type numGhostIndicesInProc = 0;
        auto      itProcIds             = d_ghostProcIds.begin();
        auto      itNumGhostIndices     = d_numGhostIndicesInGhostProcs.begin();
        for (; itProcIds != d_ghostProcIds.end(); ++itProcIds)
          {
            numGhostIndicesInProc = *itNumGhostIndices;
            if (procId == *itProcIds)
              break;

            cumulativeIndices += numGhostIndicesInProc;
            ++itNumGhostIndices;
          }

        std::string msg =
          "The processor Id " + std::to_string(procId) +
          " does not contain any ghost indices for the current processor"
          " (i.e., processor Id " +
          std::to_string(d_myRank) + ")";
        throwException<InvalidArgument>(itProcIds != d_ghostProcIds.end(), msg);

        SizeTypeVector returnValue(numGhostIndicesInProc);
        MemoryTransfer<memorySpace, memorySpace>::copy(
          numGhostIndicesInProc,
          returnValue.begin(),
          d_flattenedLocalGhostIndices.begin() + cumulativeIndices);

        return returnValue;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      const std::vector<size_type> &
      MPIPatternP2P<memorySpace>::getTargetProcIds() const
      {
        return d_targetProcIds;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      const std::vector<size_type> &
      MPIPatternP2P<memorySpace>::getNumOwnedIndicesForTargetProcs() const
      {
        return d_numOwnedIndicesForTargetProcs;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      const typename MPIPatternP2P<memorySpace>::SizeTypeVector &
      MPIPatternP2P<memorySpace>::getOwnedLocalIndicesForTargetProcs() const
      {
        return d_flattenedLocalTargetIndices;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      size_type
      MPIPatternP2P<memorySpace>::getNumOwnedIndicesForTargetProc(
        const size_type procId) const
      {
        auto      itProcIds         = d_targetProcIds.begin();
        auto      itNumOwnedIndices = d_numOwnedIndicesForTargetProcs.begin();
        size_type numOwnedIndicesForProc = 0;
        for (; itProcIds != d_targetProcIds.end(); ++itProcIds)
          {
            numOwnedIndicesForProc = *itNumOwnedIndices;
            if (procId == *itProcIds)
              break;

            ++itNumOwnedIndices;
          }

        std::string msg = "There are no owned indices for "
                          " target processor Id " +
                          std::to_string(procId) +
                          " in the current processor"
                          " (i.e., processor Id " +
                          std::to_string(d_myRank) + ")";
        throwException<InvalidArgument>(itProcIds != d_targetProcIds.end(),
                                        msg);
        return numOwnedIndicesForProc;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      typename MPIPatternP2P<memorySpace>::SizeTypeVector
      MPIPatternP2P<memorySpace>::getOwnedLocalIndices(
        const size_type procId) const
      {
        size_type cumulativeIndices      = 0;
        size_type numOwnedIndicesForProc = 0;
        auto      itProcIds              = d_targetProcIds.begin();
        auto      itNumOwnedIndices = d_numOwnedIndicesForTargetProcs.begin();
        for (; itProcIds != d_targetProcIds.end(); ++itProcIds)
          {
            numOwnedIndicesForProc = *itNumOwnedIndices;
            if (procId == *itProcIds)
              break;

            cumulativeIndices += numOwnedIndicesForProc;
            ++itNumOwnedIndices;
          }

        std::string msg = "There are no owned indices for "
                          " target processor Id " +
                          std::to_string(procId) +
                          " in the current processor"
                          " (i.e., processor Id " +
                          std::to_string(d_myRank) + ")";
        throwException<InvalidArgument>(itProcIds != d_targetProcIds.end(),
                                        msg);

        SizeTypeVector returnValue(numOwnedIndicesForProc);
        MemoryTransfer<memorySpace, memorySpace>::copy(
          numOwnedIndicesForProc,
          returnValue.begin(),
          d_flattenedLocalTargetIndices.begin() + cumulativeIndices);

        return returnValue;
      }



      template <dftefe::utils::MemorySpace memorySpace>
      const MPIComm &
      MPIPatternP2P<memorySpace>::mpiCommunicator() const
      {
        return d_mpiComm;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      size_type
      MPIPatternP2P<memorySpace>::nmpiProcesses() const
      {
        return d_nprocs;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      size_type
      MPIPatternP2P<memorySpace>::thisProcessId() const
      {
        return d_myRank;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      global_size_type
      MPIPatternP2P<memorySpace>::nGlobalIndices() const
      {
        return d_nGlobalIndices;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      size_type
      MPIPatternP2P<memorySpace>::localOwnedSize() const
      {
        return d_numLocallyOwnedIndices;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      size_type
      MPIPatternP2P<memorySpace>::localGhostSize() const
      {
        return d_numGhostIndices;
      }


      template <dftefe::utils::MemorySpace memorySpace>
      global_size_type
      MPIPatternP2P<memorySpace>::localToGlobal(const size_type localId) const
      {
        global_size_type returnValue = 0;
        if (localId < d_numLocallyOwnedIndices)
          {
            returnValue = d_locallyOwnedRange.first + localId;
          }
        else if (localId < (d_numLocallyOwnedIndices + d_numGhostIndices))
          {
            auto it =
              d_ghostIndices.begin() + (localId - d_numLocallyOwnedIndices);
            returnValue = *it;
          }
        else
          {
            std::string msg =
              "In processor " + std::to_string(d_myRank) +
              ", the local index " + std::to_string(localId) +
              " passed to localToGlobal() in MPIPatternP2P is"
              " larger than number of locally owned plus ghost indices.";
            throwException<InvalidArgument>(false, msg);
          }
        return returnValue;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      size_type
      MPIPatternP2P<memorySpace>::globalToLocal(
        const global_size_type globalId) const
      {
        size_type returnValue = 0;
        if (globalId >= d_locallyOwnedRange.first &&
            globalId < d_locallyOwnedRange.second)
          {
            returnValue = globalId - d_locallyOwnedRange.first;
          }
        else
          {
            bool found = false;
            d_ghostIndicesOptimizedIndexSet.getPosition(globalId,
                                                        returnValue,
                                                        found);
            std::string msg =
              "In processor " + std::to_string(d_myRank) +
              ", the global index " + std::to_string(globalId) +
              " passed to globalToLocal() in MPIPatternP2P is"
              " neither present in its locally owned range nor in its "
              " ghost indices.";
            throwException<InvalidArgument>(found, msg);
          }

        return returnValue;
      }

      template <dftefe::utils::MemorySpace memorySpace>
      bool
      MPIPatternP2P<memorySpace>::inLocallyOwnedRange(
        const global_size_type globalId) const
      {
        return (globalId >= d_locallyOwnedRange.first &&
                globalId < d_locallyOwnedRange.second);
      }

      template <dftefe::utils::MemorySpace memorySpace>
      bool
      MPIPatternP2P<memorySpace>::isGhostEntry(
        const global_size_type globalId) const
      {
        bool      found = false;
        size_type localId;
        d_ghostIndicesOptimizedIndexSet.getPosition(globalId, localId, found);
        return found;
      }
    } // end of namespace mpi
  }   // end of namespace utils
} // end of namespace dftefe
