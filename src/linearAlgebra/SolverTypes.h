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

#ifndef dftefeSolverTypes_h
#define dftefeSolverTypes_h

namespace dftefe
{
  namespace linearAlgebra
  {
    enum class LinearSolverType
    {
      CG
    };

    enum class PreconditionerType
    {
      NONE,
      JACOBI
    };

    enum class NonLinearSolverType
    {
      CG,
      LBFGS
    };

    enum class Error
    {
      SUCCESS,
      FAILED_TO_CONVERGE,
      DIVISON_BY_ZERO,
      OTHER_ERROR
    };

  } // end of namespace linearAlgebra
} // end of namespace dftefe
#endif // dftefeSolverTypes_h
