#include <basis/TriangulationBase.h>
#include <basis/TriangulationDealiiParallel.h>
#include <basis/CellMappingBase.h>
#include <basis/LinearCellMappingDealii.h>
#include <basis/EFEBasisManagerDealii.h>
#include <basis/FEConstraintsDealii.h>
#include <basis/EFEBasisDataStorageDealii.h>
#include <basis/FEBasisOperations.h>
#include <basis/EFEBasisHandlerDealii.h>
#include <quadrature/QuadratureAttributes.h>
#include <quadrature/QuadratureRuleGauss.h>
#include <utils/Point.h>
#include <utils/TypeConfig.h>
#include <utils/MemorySpaceType.h>
#include <utils/MemoryStorage.h>
#include <vector>
#include <fstream>
#include <memory>

#include <deal.II/grid/tria.h>
#include <deal.II/grid/grid_generator.h>
#include <deal.II/dofs/dof_handler.h>
#include <deal.II/fe/fe_q.h>
#include <deal.II/fe/fe_values.h>
#include <deal.II/base/quadrature_lib.h>
#include <deal.II/fe/mapping_q1.h>
#include <deal.II/dofs/dof_tools.h>

#include <iostream>

double rho(double x, double y, double z, double Rx, double Ry, double Rz, double rc)
{
  // The function should have homogeneous dirichlet BC
  double r = std::sqrt( (x-Rx)*(x-Rx) + (y-Ry)*(y-Ry) + (z-Rz)*(z-Rz) );
  double ret = 0;
  if( r > rc )
    return 0;
  else
    return -21*std::pow((r-rc),3)*(6*r*r + 3*r*rc + rc*rc)/(5*M_PI*std::pow(rc,8))*4*M_PI;
}

double potential(double x, double y, double z, double Rx, double Ry, double Rz, double rc)
{
  // The function should have inhomogeneous dirichlet BC
  double r = std::sqrt( (x-Rx)*(x-Rx) + (y-Ry)*(y-Ry) + (z-Rz)*(z-Rz) );
  if( r > rc )
    return 1/r;
  else
    return (9*std::pow(r,7)-30*std::pow(r,6)*rc
      +28*std::pow(r,5)*std::pow(rc,2)-14*std::pow(r,2)*std::pow(rc,5)
        +12*std::pow(rc,7))/(5*std::pow(rc,8));
}

int main()
{

  std::cout<<" Entering test poisson problem enrichement\n";

  //initialize MPI

  int mpiInitFlag = 0;
  dftefe::utils::mpi::MPIInitialized(&mpiInitFlag);
  if(!mpiInitFlag)
  {
    dftefe::utils::mpi::MPIInit(NULL, NULL);
  }

  dftefe::utils::mpi::MPIComm comm = dftefe::utils::mpi::MPICommWorld;

  int blasQueue = 0;
  dftefe::linearAlgebra::blasLapack::BlasQueue<dftefe::utils::MemorySpace::HOST> *blasQueuePtr = &blasQueue;

  std::shared_ptr<dftefe::linearAlgebra::LinAlgOpContext<dftefe::utils::MemorySpace::HOST>> linAlgOpContext = 
    std::make_shared<dftefe::linearAlgebra::LinAlgOpContext<dftefe::utils::MemorySpace::HOST>>(blasQueuePtr);

  // Set up Triangulation
  const unsigned int dim = 3;
  std::shared_ptr<dftefe::basis::TriangulationBase> triangulationBase =
    std::make_shared<dftefe::basis::TriangulationDealiiParallel<dim>>(comm);
  std::vector<unsigned int>         subdivisions = {10, 10, 10};
  std::vector<bool>                 isPeriodicFlags(dim, false);
  std::vector<dftefe::utils::Point> domainVectors(dim,
                                                  dftefe::utils::Point(dim, 0.0));


  double xmax = 10.0;
  double ymax = 10.0;
  double zmax = 10.0;

  domainVectors[0][0] = xmax;
  domainVectors[1][1] = ymax;
  domainVectors[2][2] = zmax;

  // initialize the triangulation
  triangulationBase->initializeTriangulationConstruction();
  triangulationBase->createUniformParallelepiped(subdivisions,
                                                 domainVectors,
                                                 isPeriodicFlags);
  triangulationBase->finalizeTriangulationConstruction();
 
  triangulationBase->executeCoarseningAndRefinement();
  triangulationBase->finalizeTriangulationConstruction();

  // Enrichment data file consisting of g(r,\theta,\phi) = f(r)*Y_lm(\theta, \phi)
  std::string sourceDir = "/home/avirup/dft-efe/test/basis/src/";
  std::string atomDataFile = "AtomData.in";
  std::string inputFileName = sourceDir + atomDataFile;
  std::fstream fstream;

  fstream.open(inputFileName, std::fstream::in);
  
  // read the input file and create atomsymbol vector and atom coordinates vector.
  std::vector<dftefe::utils::Point> atomCoordinatesVec;
  std::vector<double> coordinates;
  coordinates.resize(dim,0.);
  std::vector<std::string> atomSymbol;
  std::string symbol;
  atomSymbol.resize(0);
  std::string line;
  while (std::getline(fstream, line)){
      std::stringstream ss(line);
      ss >> symbol; 
      for(unsigned int i=0 ; i<dim ; i++){
          ss >> coordinates[i]; 
      }
      atomCoordinatesVec.push_back(coordinates);
      atomSymbol.push_back(symbol);
  }
  dftefe::utils::mpi::MPIBarrier(comm);
      
  std::map<std::string, std::string> atomSymbolToFilename;
  for (auto i:atomSymbol )
  {
      atomSymbolToFilename[i] = sourceDir + i + ".xml";
  }

  std::vector<std::string> fieldNames{ "density", "vhartree", "vnuclear", "vtotal", "orbital" };
  std::vector<std::string> metadataNames{ "symbol", "Z", "charge", "NR", "r" };
  std::shared_ptr<dftefe::atoms::AtomSphericalDataContainer>  atomSphericalDataContainer = 
      std::make_shared<dftefe::atoms::AtomSphericalDataContainer>(atomSymbolToFilename,
                                                      fieldNames,
                                                      metadataNames);

  std::string fieldName = "vnuclear";
  double atomPartitionTolerance = 1e-6;


  // initialize the basis Manager

  unsigned int feDegree = 2;

  // make FEBasisManagerDealii for the constraints
  std::shared_ptr<dftefe::basis::FEBasisManager> febasisManager =   std::make_shared<dftefe::basis::FEBasisManagerDealii<dim>>(
      triangulationBase ,
      feDegree); 

  std::shared_ptr<dftefe::basis::FEBasisManager> basisManager =   std::make_shared<dftefe::basis::EFEBasisManagerDealii<dim>>(
      triangulationBase ,
      atomSphericalDataContainer ,
      feDegree ,
      atomPartitionTolerance,
      atomSymbol,
      atomCoordinatesVec,
      fieldName,
      comm);
  std::map<dftefe::global_size_type, dftefe::utils::Point> dofCoords;
  basisManager->getBasisCenters(dofCoords);

  std::cout << "Locally owned cells : " << basisManager->nLocallyOwnedCells() << "\n";
  std::cout << "Total Number of dofs : " << basisManager->ndofs() << "\n";
  
  // Set the constraints

  std::string constraintHanging = "HangingNodeConstraint"; //give BC to rho
  std::string constraintHomwHan = "HomogeneousWithHanging"; // use this to solve the laplace equation
  std::string constraintPotential = "InHomogeneosWithHangingPotential"; // this is for getting analytical solution
  std::vector<std::shared_ptr<dftefe::basis::FEConstraintsBase<double, dftefe::utils::MemorySpace::HOST>>>
    constraintsVec;
  constraintsVec.resize(3);
  for ( unsigned int i=0 ;i < constraintsVec.size() ; i++ )
   constraintsVec[i] = std::make_shared<dftefe::basis::FEConstraintsDealii<double, dftefe::utils::MemorySpace::HOST, dim>>();

  constraintsVec[0]->clear();
  constraintsVec[0]->makeHangingNodeConstraint(basisManager);
  constraintsVec[0]->close();

  constraintsVec[1]->clear();
  constraintsVec[1]->makeHangingNodeConstraint(basisManager);
  constraintsVec[1]->setHomogeneousDirichletBC();
  constraintsVec[1]->close();

  constraintsVec[2]->clear();
  constraintsVec[2]->makeHangingNodeConstraint(basisManager);
  const unsigned int dofs_per_cell =
    basisManager->nCellDofs(0);
  const unsigned int faces_per_cell =
    dealii::GeometryInfo<dim>::faces_per_cell;
  const unsigned int dofs_per_face =
    std::pow((basisManager->getFEOrder(0)+1),2);
  std::vector<dftefe::global_size_type> cellGlobalDofIndices(dofs_per_cell);
  std::vector<dftefe::global_size_type> iFaceGlobalDofIndices(dofs_per_face);
  std::vector<bool> dofs_touched(basisManager->nGlobalNodes(), false);
  auto              icell = basisManager->beginLocallyOwnedCells();
  dftefe::utils::Point basisCenter(dim, 0);
  for (; icell != basisManager->endLocallyOwnedCells(); ++icell)
    {
      (*icell)->cellNodeIdtoGlobalNodeId(cellGlobalDofIndices);
      for (unsigned int iFace = 0; iFace < faces_per_cell; ++iFace)
        {
          (*icell)->getFaceDoFGlobalIndices(iFace, iFaceGlobalDofIndices);
          const dftefe::size_type boundaryId = (*icell)->getFaceBoundaryId(iFace);
          if (boundaryId == 0)
            {
              for (unsigned int iFaceDof = 0; iFaceDof < dofs_per_face;
                    ++iFaceDof)
                {
                  const dftefe::global_size_type nodeId =
                    iFaceGlobalDofIndices[iFaceDof];
                  if (dofs_touched[nodeId])
                    continue;
                  dofs_touched[nodeId] = true;
                  if (!constraintsVec[2]->isConstrained(nodeId))
                    {
                      basisCenter = dofCoords.find(nodeId)->second;
                      double constraintValue = potential(basisCenter[0], basisCenter[1], basisCenter[2], Rx, Ry, Rz, rc);
                      constraintsVec[2]->setInhomogeneity(nodeId, constraintValue);
                    } // non-hanging node check
                }     // Face dof loop
            }
        } // Face loop
    }     // cell locally owned
  constraintsVec[2]->close();

  std::map<std::string,
           std::shared_ptr<const dftefe::basis::Constraints<double, dftefe::utils::MemorySpace::HOST>>> constraintsMap;

  constraintsMap[constraintHanging] = constraintsVec[0];
  constraintsMap[constraintHomwHan] = constraintsVec[1];
  constraintsMap[constraintPotential] = constraintsVec[2];

  // Set up the quadrature rule
  unsigned int num1DGaussSize =4;

  dftefe::quadrature::QuadratureRuleAttributes quadAttr(dftefe::quadrature::QuadratureFamily::GAUSS,true,num1DGaussSize);

  dftefe::basis::BasisStorageAttributesBoolMap basisAttrMap;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreValues] = true;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreGradient] = false;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreHessian] = false;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreOverlap] = false;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreGradNiGradNj] = false;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreJxW] = false;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreQuadRealPoints] = false;

  // Set up the FE Basis Data Storage
  std::shared_ptr<dftefe::basis::BasisDataStorage<double, dftefe::utils::MemorySpace::HOST>> feBasisData =
    std::make_shared<dftefe::basis::EFEBasisDataStorageDealii<double, dftefe::utils::MemorySpace::HOST,dim>>
    (basisManager, quadAttr, basisAttrMap);

  // // evaluate basis data
  feBasisData->evaluateBasisData(quadAttr, basisAttrMap);

  // Set up BasisHandler
  std::shared_ptr<dftefe::basis::BasisHandler<double, dftefe::utils::MemorySpace::HOST>> basisHandler =
    std::make_shared<dftefe::basis::EFEBasisHandlerDealii<double, dftefe::utils::MemorySpace::HOST,dim>>
    (basisManager, constraintsMap, comm);

  // std::vector<std::pair<dftefe::global_size_type, dftefe::global_size_type>> vec = basisHandler->getLocallyOwnedRanges(constraintName);
  // for (auto i:vec )
  // {
  //   std::cout<< i.first << "," << i.second << "\n" ;
  // }

  // // Set up basis Operations
  dftefe::basis::FEBasisOperations<double, double, dftefe::utils::MemorySpace::HOST,dim> feBasisOp(feBasisData,50);

  // set up MPIPatternP2P for the constraints
  auto mpiPatternP2PHanging = basisHandler->getMPIPatternP2P(constraintHanging);
  auto mpiPatternP2PHomwHan = basisHandler->getMPIPatternP2P(constraintHomwHan);
  auto mpiPatternP2PPotential = basisHandler->getMPIPatternP2P(constraintPotential);

  // set up different multivectors - rho, vh with inhomogeneous BC, vh
  std::shared_ptr<dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>
   dens = std::make_shared<
    dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>(
      mpiPatternP2PHomwHan, linAlgOpContext, numComponents, double());

  std::shared_ptr<dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>
   vh = std::make_shared<
    dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>(
      mpiPatternP2PPotential, linAlgOpContext, numComponents, double());

  std::shared_ptr<dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>
   vhNHDB = std::make_shared<
    dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>(
      mpiPatternP2PHanging, linAlgOpContext, numComponents, double());

  std::shared_ptr<dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>
   solution = std::make_shared<
    dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>(
      mpiPatternP2PHanging, linAlgOpContext, numComponents, double());

  std::shared_ptr<dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>
   error = std::make_shared<
    dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>(
      mpiPatternP2PPotential, linAlgOpContext, numComponents, double());

  //populate the value of the Density at the nodes for interpolating to quad points
  auto numLocallyOwnedCells  = basisManager->nLocallyOwnedCells();
  auto itField  = dens->begin();
  dftefe::utils::Point nodeLoc(dim,0.0);
  for (dftefe::size_type iCell = 0; iCell < numLocallyOwnedCells ; iCell++)
    {
      // get cell dof global ids
      std::vector<dftefe::global_size_type> cellGlobalNodeIds;
      basisManager->getCellDofsGlobalIds(iCell, cellGlobalNodeIds);

      // loop over nodes of a cell
      for ( dftefe::size_type iNode = 0 ; iNode < cellGlobalNodeIds.size() ; iNode++)
      {
        // If node not constrained then get the local id and coordinates of the node
        dftefe::global_size_type globalId = cellGlobalNodeIds[iNode];
        if( !basisHandler->getConstraints(constraintHomwHan).isConstrained(globalId))
        {
          dftefe::size_type localId = basisHandler->globalToLocalIndex(globalId,constraintHomwHan) ;
          basisHandler->getBasisCenters(localId,constraintHomwHan,nodeLoc);
          *(itField + localId )  = rho(nodeLoc[0], nodeLoc[1], nodeLoc[2], Rx, Ry, Rz, rc);
        }
      }
    }
  dens->updateGhostValues();
  basisHandler->getConstraints(constraintHomwHan).distributeParentToChild(*dens, numComponents);


  // vector for lhs

  numLocallyOwnedCells  = basisManager->nLocallyOwnedCells();
  itField  = vhNHDB->begin();
  dofs_touched.clear();
  dofs_touched.resize(basisManager->nGlobalNodes(), false);
  icell = basisManager->beginLocallyOwnedCells();
  for (; icell != basisManager->endLocallyOwnedCells(); ++icell)
    {
      (*icell)->cellNodeIdtoGlobalNodeId(cellGlobalDofIndices);
      for (unsigned int iFace = 0; iFace < faces_per_cell; ++iFace)
        {
          (*icell)->getFaceDoFGlobalIndices(iFace, iFaceGlobalDofIndices);
          const dftefe::size_type boundaryId = (*icell)->getFaceBoundaryId(iFace);
          if (boundaryId == 0)
            {
              for (unsigned int iFaceDof = 0; iFaceDof < dofs_per_face;
                    ++iFaceDof)
                {
                  const dftefe::global_size_type globalId =
                    iFaceGlobalDofIndices[iFaceDof];
                  if (dofs_touched[globalId])
                    continue;
                  dofs_touched[globalId] = true;
                  if (!basisHandler->getConstraints(constraintHanging).isConstrained(globalId))
                    {
                      dftefe::size_type localId = basisHandler->globalToLocalIndex(globalId,constraintHanging) ;
                      basisHandler->getBasisCenters(localId,constraintHanging,nodeLoc);
                      *(itField + localId )  = potential(nodeLoc[0], nodeLoc[1], nodeLoc[2], Rx, Ry, Rz, rc);
                    } // non-hanging node check
                }     // Face dof loop
            }
        } // Face loop
    }     // cell locally owned
  vhNHDB->updateGhostValues();
  basisHandler->getConstraints(constraintHanging).distributeParentToChild(*vhNHDB, numComponents);


  //populate the value of the Potential at the nodes for the analytic expressions

  numLocallyOwnedCells  = basisManager->nLocallyOwnedCells();
  itField  = vh->begin();
  for (dftefe::size_type iCell = 0; iCell < numLocallyOwnedCells ; iCell++)
    {
      // get cell dof global ids
      std::vector<dftefe::global_size_type> cellGlobalNodeIds;
      basisManager->getCellDofsGlobalIds(iCell, cellGlobalNodeIds);

      // loop over nodes of a cell
      for ( dftefe::size_type iNode = 0 ; iNode < cellGlobalNodeIds.size() ; iNode++)
        {
          // If node not constrained then get the local id and coordinates of the node
          dftefe::global_size_type globalId = cellGlobalNodeIds[iNode];
         if( !basisHandler->getConstraints(constraintPotential).isConstrained(globalId))
         {
            dftefe::size_type localId = basisHandler->globalToLocalIndex(globalId,constraintPotential) ;
            basisHandler->getBasisCenters(localId,constraintPotential,nodeLoc);
            *(itField + localId )  = potential(nodeLoc[0], nodeLoc[1], nodeLoc[2], Rx, Ry, Rz, rc);
         }
        }
    }
  vh->updateGhostValues();
  basisHandler->getConstraints(constraintPotential).distributeParentToChild(*vh, numComponents);

  // create the quadrature Value Container

  std::shared_ptr<dftefe::quadrature::QuadratureRule> quadRule =
    std::make_shared<dftefe::quadrature::QuadratureRuleGauss>(dim, num1DGaussSize);

  dftefe::basis::LinearCellMappingDealii<dim> linearCellMappingDealii;
  dftefe::quadrature::QuadratureRuleContainer quadRuleContainer( quadAttr, quadRule, triangulationBase,
                                                                 linearCellMappingDealii);

  dftefe::quadrature::QuadratureValuesContainer<double, dftefe::utils::MemorySpace::HOST> quadValuesContainer(quadRuleContainer, numComponents);

  for(dftefe::size_type i = 0 ; i < quadValuesContainer.nCells() ; i++)
  {
    dftefe::size_type quadId = 0;
    for (auto j : quadRuleContainer.getCellRealPoints(i))
    {
      double a = rho( j[0], j[1], j[2], Rx, Ry, Rz, rc);
      double *b = &a;
      quadValuesContainer.setCellQuadValues<dftefe::utils::MemorySpace::HOST> (i, quadId, b);
      quadId = quadId + 1;
    }
  }

  //feBasisOp.interpolate( *dens, constraintHomwHan, *basisHandler, quadAttr, quadValuesContainer);

  std::shared_ptr<dftefe::linearAlgebra::LinearSolverFunction<double,
                                                   double,
                                                   dftefe::utils::MemorySpace::HOST>> linearSolverFunction =
    std::make_shared<dftefe::physics::PoissonLinearSolverFunctionFE<double,
                                                   double,
                                                   dftefe::utils::MemorySpace::HOST,
                                                   dim>>
                                                   (basisHandler,
                                                    feBasisOp,
                                                    feBasisData,
                                                    quadValuesContainer,
                                                    quadAttr,
                                                    constraintHanging,
                                                    constraintHomwHan,
                                                    *vhNHDB,
                                                    dftefe::linearAlgebra::PreconditionerType::JACOBI,
                                                    linAlgOpContext,
                                                    50);

  dftefe::linearAlgebra::LinearAlgebraProfiler profiler;

  std::shared_ptr<dftefe::linearAlgebra::LinearSolverImpl<double,
                                                   double,
                                                   dftefe::utils::MemorySpace::HOST>> CGSolve =
    std::make_shared<dftefe::linearAlgebra::CGLinearSolver<double,
                                                   double,
                                                   dftefe::utils::MemorySpace::HOST>>
                                                   ( maxIter,
                                                  absoluteTol,
                                                  relativeTol,
                                                  divergenceTol,
                                                  profiler);

  CGSolve->solve(*linearSolverFunction);

  linearSolverFunction->getSolution(*solution);

  std::vector<double> ones(0);
  ones.resize(numComponents, (double)1.0);
  std::vector<double> nOnes(0);
  nOnes.resize(numComponents, (double)-1.0);

  std::cout<<"solution norm: "<<solution->l2Norms()[0]<<", potential analytical norm: "<<vh->l2Norms()[0]<<"\n";

  dftefe::linearAlgebra::add(ones, *vh, nOnes, *solution, *error);

  std::cout<<"No of dofs: "<< basisManager->nGlobalNodes() <<", error norm: "<<error->l2Norms()[0]<<", relative error: "<<(error->l2Norms()[0]/vh->l2Norms()[0])<<"\n";

  int mpiFinalFlag = 0;
  dftefe::utils::mpi::MPIFinalized(&mpiFinalFlag);
  if(!mpiFinalFlag)
  {
    dftefe::utils::mpi::MPIFinalize();
  }

}
