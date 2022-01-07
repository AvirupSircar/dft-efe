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

#include <utils/OptimizedIndexSet.h>
#include <iterator>
namespace dftefe {

  namespace utils {

    OptimizedIndexSet::OptimizedIndexSet(const std::set<global_size_type> & inputSet,
	const double numRangesToSetSizeTol /*=0.1*/):
      d_set(inputSet),
      d_defaultToSTLSet(false),
      d_contiguousRanges(0)
    {
    
      std::set<global_size_type>::const_iterator itLastRange = d_set.begin();
      std::set<global_size_type>::const_iterator itPrev = d_set.begin(); 
      std::set<global_size_type>::const_iterator it = itPrev; 
      it++; 
      for(; it != d_set.end(); ++it)
      {
	bool isContiguous = ((*it - 1) == *(itPrev));
	if(!isContiguous)
	{
	  d_contiguousRanges.push_back(*itLastRange);
	  d_contiguousRanges.push_back(*(itPrev)+1);
	  itLastRange = it;
	}
	itPrev= it;
      }
      d_contiguousRanges.push_back(*itLastRange);
      d_contiguousRanges.push_back(*(itPrev)+1);

      d_numContiguousRanges = d_contiguousRanges.size()/2;
      d_numEntriesBefore.resize(d_numContiguousRanges,0);
      size_type cumulativeEntries = 0;
      for(unsigned int i = 0; i < d_numContiguousRanges; ++i)
      {
	d_numEntriesBefore[i] = cumulativeEntries;
	cumulativeEntries += d_contiguousRanges[2*i+1]-d_contiguousRanges[2*i];
      }

      if((d_numContiguousRanges+0.0)/d_set.size() > numRangesToSetSizeTol)
	d_defaultToSTLSet = true;

    }

    void
    OptimizedIndexSet::getPosition(const global_size_type index,
	size_type & pos,
	bool & found)
    {
      found = false;
      if(d_defaultToSTLSet)
      { 
	auto it = d_set.find(index);
	if(it != d_set.end())
	{
	  found = true;
	  pos = std::distance(d_set.begin(),it);
	}
      }

      else
      { 
	/*
	 * The logic used for finding an index is as follows:
	 * 1. Find the position of the element in d_contiguousRanges 
	 *    which is greater than (strictly greater) the input index.
	 *    Let's call this position as upPos and the value at upPos as
	 *    upVal. The complexity of finding it is 
	 *    O(log(size of d_contiguousRanges)) 
	 * 2. Since d_contiguousRanges stores pairs of startId and endId
	 *    (endId not inclusive) of contiguous ranges in d_set, 
	 *    any index for which upPos is even (i.e., it corresponds to a startId) 
	 *    cannot belong to d_set. Why?
	 *    Consider two consequtive ranges [k1,k2) and [k3,k4) where
	 *    k1 < k2 < k3 < k4. If upVal for index corresponds to k3 
	 *    (i.e., startId of a range), then
	 *    (a) index does not lie in the [k3,k4) as index < upVal (=k3). 
	 *    (b) index cannot lie in [k1,k2), because if index lies in [k1,k2), then
	 *    upVal should be k2 (not k3)
	 *  3. If upPos is odd (i.e, it corresponds to an endId), we find the relative 
	 *     position of index in that range. Subsequently, we determine the global 
	 *     position of index in d_set by adding the relative position to
	 *     the number of entries in d_set prior to the range where index lies
	 */
	   
	auto up = std::upper_bound(d_contiguousRanges.begin(), 
	    d_contiguousRanges.end(), index);
	size_type upPos = std::distance(d_contiguousRanges.begin(), up);
	if(upPos % 2 == 1)
	{
	  found = true;
	  size_type rangeId = upPos/2;
	  pos = d_numEntriesBefore[rangeId] + index - 
	    d_contiguousRanges[upPos-1];
	}
      }
    }

  } // end of namespace utils

} // end of namespace dftefe
