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

#include<utils/Exceptions.h>
#include<atoms/AtomSphericalData.h>
#include<string>
#include<iostream>
int main()
{
  std::string atomFileName = "TestAtom.xml";
  std::vector<std::string> fieldNames{ "density", "vhartree", "vnuclear", "vtotal", "orbital" };
  std::vector<std::string> metadataNames{ "symbol", "Z", "charge", "NR", "r" };
  std::vector<int> qNumbers{2, 1, -1};
  dftefe::atoms::AtomSphericalData atomTest(atomFileName, fieldNames, metadataNames);
  dftefe::atoms::SphericalData sphericalDataObj = atomTest.getSphericalData("orbital", qNumbers);
  std::vector<double> pointvec{2, 2, 0.};
  std::vector<double> originvec{0. ,0. ,0.};
  dftefe::utils::Point point(pointvec);
  dftefe::utils::Point origin(originvec);
  double polarAngleTolerance = 1e-6;
  double  cutoffTolerance = 1e-6;
  sphericalDataObj.initSpline();
  std::cout<<sphericalDataObj.getValue<3>(point,origin,polarAngleTolerance)<<"\n";
  std::vector<double> x = sphericalDataObj.getDerivativeValue<3>(point,origin,polarAngleTolerance, cutoffTolerance);
  std::cout<<x[0]<<","<<x[1]<<","<<x[2];
}
