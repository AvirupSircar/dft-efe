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

#ifndef dftefeOperatorContext_h
#define dftefeOperatorContext_h

#include <utils/MemorySpaceType.h>
#include <linearAlgebra/BlasLapackTypedef.h>
#include <linearAlgebra/LinAlgOpContext.h>
#include <linearAlgebra/Vector.h>
#include <linearAlgebra/MultiVector.h>
#include <linearAlgebra/AbstractMatrix.h>
namespace dftefe
{
  namespace linearAlgebra
  {
    /**
     *@brief Abstract class to encapsulate the action of a discrete operator on vectors, matrices, etc.
     *
     * @tparam ValueTypeOperator The datatype (float, double, complex<double>, etc.) for the underlying operator
     * @tparam ValueTypeOperand The datatype (float, double, complex<double>, etc.) of the vector, matrices, etc.
     * on which the operator will act
     * @tparam memorySpace The meory sapce (HOST, DEVICE, HOST_PINNES, etc.) in which the data of the operator
     * and its operands reside
     *
     */
    template <typename ValueTypeOperator,
              typename ValueTypeOperand,
              utils::MemorySpace memorySpace>
    class OperatorContext
    {
    public:
      /**
       *@brief Default Destructor
       *
       */
      ~OperatorContext() = default;

      virtual void
      apply(const Vector<ValueTypeOperand, memorySpace> &x,
            Vector<blasLapack::scalar_type<ValueTypeOperator, ValueTypeOperand>,
                   memorySpace> &                        y) const = 0;

      virtual void
      apply(const MultiVector<ValueTypeOperand, memorySpace> &X,
            MultiVector<
              blasLapack::scalar_type<ValueTypeOperator, ValueTypeOperand>,
              memorySpace> &Y) const = 0;

      //
      // TODO: Uncomment the following and implement in all the derived classes
      //

      // virtual
      //  apply(const AbstractMatrix<ValueTypeOperand, memorySpace> & X,
      //    AbstractMatrix<blasLapack::scalar_type<ValueTypeOperator,
      //    ValueTypeOperand>, memorySpace> & Y) const = 0;
    };
  } // end of namespace linearAlgebra
} // end of namespace dftefe
#endif // dftefeOperatorContext_h
