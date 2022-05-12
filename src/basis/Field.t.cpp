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
namespace dftefe
{
  namespace basis
  {
    template <typename ValueType, utils::MemorySpace memorySpace>
    Field<ValueType, memorySpace>::Field(
      std::shared_ptr<const BasisHandler>   basisHandler,
      const std::string                     constraintsName,
      const linearAlgebra::LinAlgOpContext &linAlgOpContext)
    {
      reinit(basisHandler, constraintsName, linAlgOpContext);
    }

    template <typename ValueType, utils::MemorySpace memorySpace>
    Field<ValueType, memorySpace>::reinit(
      std::shared_ptr<const BasisHandler>   basisHandler,
      const std::string                     constraintsName,
      const linearAlgebra::LinAlgOpContext &linAlgOpContext)
    {
      d_basisHandler     = basisHandler;
      d_constraintsName  = constraintsName;
      d_linAlgOpContext  = linAlgOpContext;
      auto mpiPatternP2P = basisHandler->getMPIPatternP2P(constraintsName);

      //
      // create the vector 
      //
      if(d_basisHandler->isDistributed())
      {
	d_vector = std::make_shared<linearAlgebra::DistributedVector<ValueType, memorySpace>>();

      }
    }

  } // end of namespace basis
} // end of namespace dftefe
