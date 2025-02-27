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

#include <linearAlgebra/LinearAlgebraTypes.h>
#include <utils/Exceptions.h>
namespace dftefe
{
  namespace linearAlgebra
  {
    const std::map<Error, std::string> ErrorMsg::d_errToMsgMap = {
      {Error::SUCCESS, "Success"},
      {Error::FAILED_TO_CONVERGE, "Failed to converge"},
      {Error::RESIDUAL_DIVERGENCE, "Residual diverged"},
      {Error::DIVISON_BY_ZERO, "Division by zero encountered"},
      {Error::OTHER_ERROR, "Other error encountered"}};

    std::pair<bool, std::string>
    ErrorMsg::isSuccessAndMsg(const Error &error)
    {
      std::pair<bool, std::string> returnValue(false, "");
      auto                         it = d_errToMsgMap.find(error);
      if (it != d_errToMsgMap.end())
        returnValue = std::make_pair(error == Error::SUCCESS, it->second);

      else
        {
          utils::throwException<utils::InvalidArgument>(
            false, "Invalid linearAlgebra::Error passed.");
        }

      return returnValue;
    }
  } // namespace linearAlgebra
} // namespace dftefe
