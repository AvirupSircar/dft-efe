    #include <basis/TriangulationBase.h>
    #include <basis/TriangulationDealiiParallel.h>
    #include <basis/CellMappingBase.h>
    #include <basis/LinearCellMappingDealii.h>
    #include <basis/EFEBasisManagerDealii.h>
    #include <basis/EFEConstraintsDealii.h>
    #include <basis/EFEBasisDataStorageDealii.h>
    #include <basis/FEBasisOperations.h>
    #include <basis/EFEBasisHandlerDealii.h>
    #include <quadrature/QuadratureAttributes.h>
    #include <quadrature/QuadratureRuleGauss.h>
    #include <quadrature/QuadratureRuleAdaptive.h>
    #include <atoms/AtomSevereFunction.h>
    #include <utils/Point.h>
    #include <utils/TypeConfig.h>
    #include <utils/MemorySpaceType.h>
    #include <utils/MemoryStorage.h>
    #include <vector>
    #include <fstream>
    #include <memory>
    #include <linearAlgebra/LinearSolverFunction.h>
    #include <physics/PoissonLinearSolverFunctionFE.h>
    #include <linearAlgebra/LinearAlgebraProfiler.h>
    #include <linearAlgebra/CGLinearSolver.h>

    #include <deal.II/grid/tria.h>
    #include <deal.II/grid/grid_generator.h>
    #include <deal.II/dofs/dof_handler.h>
    #include <deal.II/fe/fe_q.h>
    #include <deal.II/fe/fe_values.h>
    #include <deal.II/base/quadrature_lib.h>
    #include <deal.II/fe/mapping_q1.h>
    #include <deal.II/dofs/dof_tools.h>

    #include <iostream>


// operator - nabla^2 in weak form
// operand - V_H
// memoryspace - HOST

template<typename T>
T readParameter(std::string ParamFile, std::string param)
{
  T t(0);
  std::string line;
  std::fstream fstream;
  fstream.open(ParamFile, std::fstream::in);
  int count = 0;
  while (std::getline(fstream, line))
  {
    for (int i = 0; i < line.length(); i++)
    {
        if (line[i] == ' ')
        {
            line.erase(line.begin() + i);
            i--;
        }
    }
    std::istringstream iss(line);
    std::string type;
    std::getline(iss, type, '=');
    if (type.compare(param) == 0)
    {
      iss >> t;
      count = 1;
      break;
    }
  }
  if(count == 0)
  {
    dftefe::utils::throwException(false, "The parameter is not found: "+ param);
  }
  fstream.close();
  return t;
}


double rho(dftefe::utils::Point &point, std::vector<dftefe::utils::Point> &origin, double rc)
{
  double ret = 0;
  // The function should have homogeneous dirichlet BC
  for (unsigned int i = 0 ; i < origin.size() ; i++ )
  {
    double r = 0;
    for (unsigned int j = 0 ; j < point.size() ; j++ )
    {
      r += std::pow((point[j]-origin[i][j]),2);
    }
    r = std::sqrt(r);
    if( r > rc )
      ret += 0;
    else
      ret += -21*std::pow((r-rc),3)*(6*r*r + 3*r*rc + rc*rc)/(5*M_PI*std::pow(rc,8))*4*M_PI;
  }
  return ret;
}

double potential(dftefe::utils::Point &point, std::vector<dftefe::utils::Point> &origin, double rc)
{
  double ret = 0;
  // The function should have homogeneous dirichlet BC
  for (unsigned int i = 0 ; i < origin.size() ; i++ )
  {
    double r = 0;
    for (unsigned int j = 0 ; j < point.size() ; j++ )
    {
      r += std::pow((point[j]-origin[i][j]),2);
    }
    r = std::sqrt(r);
    if( r > rc )
      ret += 1/r;
    else
      ret += (9*std::pow(r,7)-30*std::pow(r,6)*rc
        +28*std::pow(r,5)*std::pow(rc,2)-14*std::pow(r,2)*std::pow(rc,5)
        +12*std::pow(rc,7))/(5*std::pow(rc,8));
  }
  return ret;
}

int main(int argc, char** argv)
{
  // argv[1] = "PoissonProblemEnrichmentTwoBody/param.in"

  std::cout<<" Entering test two body interaction \n";

  // Required to solve : \nabla^2 V_H = g(r,r_c) Solve using CG in linearAlgebra
  // In the weak form the eqn is:
  // (N_i,N_j)*V_H = (N_i, g(r,r_c))
  // Input to CG are : linearSolverFnction. Reqd to create a derived class of the base.
  // For the nabla : LaplaceOperatorContextFE to get \nabla^2(A)*x = y

  //initialize MPI

  int mpiInitFlag = 0;
  dftefe::utils::mpi::MPIInitialized(&mpiInitFlag);
  if(!mpiInitFlag)
  {
    dftefe::utils::mpi::MPIInit(NULL, NULL);
  }

  dftefe::utils::mpi::MPIComm comm = dftefe::utils::mpi::MPICommWorld;

  int rank;
  dftefe::utils::mpi::MPICommRank(comm, &rank);

  int blasQueue = 0;
  dftefe::linearAlgebra::blasLapack::BlasQueue<dftefe::utils::MemorySpace::HOST> *blasQueuePtr = &blasQueue;

  std::shared_ptr<dftefe::linearAlgebra::LinAlgOpContext<dftefe::utils::MemorySpace::HOST>> linAlgOpContext =
    std::make_shared<dftefe::linearAlgebra::LinAlgOpContext<dftefe::utils::MemorySpace::HOST>>(blasQueuePtr);


  // Read the parameter files and atom coordinate files
  std::string sourceDir = "/home/avirup/dft-efe/analysis/classicalEnrichmentComparison/";
  std::string atomDataFile = "TwoSmearedCharge.in";
  std::string paramDataFile = argv[1];
  std::string inputFileName = sourceDir + atomDataFile;
  std::string parameterInputFileName = sourceDir + paramDataFile;

  double xmax = readParameter<double>(parameterInputFileName, "xmax");
  double ymax = readParameter<double>(parameterInputFileName, "ymax");
  double zmax = readParameter<double>(parameterInputFileName, "zmax");
  unsigned int subdivisionx = readParameter<unsigned int>(parameterInputFileName, "subdivisionx");
  unsigned int subdivisiony = readParameter<unsigned int>(parameterInputFileName, "subdivisiony");
  unsigned int subdivisionz = readParameter<unsigned int>(parameterInputFileName, "subdivisionz");
  double rc = readParameter<double>(parameterInputFileName, "rc");
  double hMin = readParameter<double>(parameterInputFileName, "hMin");
  unsigned int maxIter = readParameter<unsigned int>(parameterInputFileName, "maxIter");
  double absoluteTol = readParameter<double>(parameterInputFileName, "absoluteTol");
  double relativeTol = readParameter<double>(parameterInputFileName, "relativeTol");
  double divergenceTol = readParameter<double>(parameterInputFileName, "divergenceTol");
  double refineradius = readParameter<double>(parameterInputFileName, "refineradius");
  unsigned int num1DGaussSize = readParameter<unsigned int>(parameterInputFileName, "num1DQuadratureSize");
  unsigned int feOrder = readParameter<unsigned int>(parameterInputFileName, "feOrder");
  double atomPartitionTolerance = readParameter<double>(parameterInputFileName, "atomPartitionTolerance");
  double smallestCellVolume = readParameter<double>(parameterInputFileName, "smallestCellVolume");
  unsigned int maxRecursion = readParameter<unsigned int>(parameterInputFileName, "maxRecursion");
  double adaptiveQuadTolerance = readParameter<double>(parameterInputFileName, "adaptiveQuadTolerance");
  double integralThreshold = readParameter<double>(parameterInputFileName, "integralThreshold");
  unsigned int numComponents = 3;

  // Set up Triangulation
  const unsigned int dim = 3;
    std::shared_ptr<dftefe::basis::TriangulationBase> triangulationBase =
        std::make_shared<dftefe::basis::TriangulationDealiiParallel<dim>>(comm);
  std::vector<unsigned int>         subdivisions = {subdivisionx, subdivisiony ,subdivisionz};
  std::vector<bool>                 isPeriodicFlags(dim, false);
  std::vector<dftefe::utils::Point> domainVectors(dim,
                                                  dftefe::utils::Point(dim, 0.0));

  domainVectors[0][0] = xmax;
  domainVectors[1][1] = ymax;
  domainVectors[2][2] = zmax;

  // initialize the triangulation
  triangulationBase->initializeTriangulationConstruction();
  triangulationBase->createUniformParallelepiped(subdivisions,
                                                 domainVectors,
                                                 isPeriodicFlags);
  triangulationBase->finalizeTriangulationConstruction();

  std::fstream fstream;
  fstream.open(inputFileName, std::fstream::in);
  
  // read the input file and create atomsymbol vector and atom coordinates vector.
  std::vector<dftefe::utils::Point> atomCoordinatesVec(0,dftefe::utils::Point(dim, 0.0));
  std::vector<dftefe::utils::Point> atomCoordinates1(0,dftefe::utils::Point(dim, 0.0));
  std::vector<dftefe::utils::Point> atomCoordinates2(0,dftefe::utils::Point(dim, 0.0));
  std::vector<double> coordinates;
  coordinates.resize(dim,0.);
  std::vector<std::string> atomSymbolVec;
  std::string symbol;
  atomSymbolVec.resize(0);
  std::string line;
  while (std::getline(fstream, line)){
      std::stringstream ss(line);
      ss >> symbol; 
      for(unsigned int i=0 ; i<dim ; i++){
          ss >> coordinates[i]; 
      }
      atomCoordinatesVec.push_back(coordinates);
      atomSymbolVec.push_back(symbol);
  }
  dftefe::utils::mpi::MPIBarrier(comm);
  atomCoordinates1.push_back(atomCoordinatesVec[0]);
  atomCoordinates2.push_back(atomCoordinatesVec[1]);

  std::map<std::string, std::string> atomSymbolToFilename;
  for (auto i:atomSymbolVec )
  {
      atomSymbolToFilename[i] = sourceDir + i + ".xml";
  }

  std::vector<std::string> fieldNames{"vnuclear"};
  std::vector<std::string> metadataNames{ "symbol", "Z", "charge", "NR", "r" };
  std::shared_ptr<dftefe::atoms::AtomSphericalDataContainer>  atomSphericalDataContainer = 
      std::make_shared<dftefe::atoms::AtomSphericalDataContainer>(atomSymbolToFilename,
                                                      fieldNames,
                                                      metadataNames);

  std::string fieldName = "vnuclear";

  int flag = 1;
  int mpiReducedFlag = 1;
  bool radiusRefineFlag = true;
  while(mpiReducedFlag)
  {
    flag = 0;
    auto triaCellIter = triangulationBase->beginLocal();
    for( ; triaCellIter != triangulationBase->endLocal(); triaCellIter++)
    {
      radiusRefineFlag = false;
      (*triaCellIter)->clearRefineFlag();
      dftefe::utils::Point centerPoint(dim, 0.0); 
      (*triaCellIter)->center(centerPoint);
      for ( unsigned int i=0 ; i<atomCoordinatesVec.size() ; i++)
      {
        double dist = 0;
        for (unsigned int j = 0 ; j < dim ; j++ )
        {
          dist += std::pow((centerPoint[j]-atomCoordinatesVec[i][j]),2);
        }
        dist = std::sqrt(dist);
        if(dist < refineradius)
          radiusRefineFlag = true;
      }
      if (radiusRefineFlag && (*triaCellIter)->diameter() > hMin)
      {
        (*triaCellIter)->setRefineFlag();
        flag = 1;
      }
    }
    triangulationBase->executeCoarseningAndRefinement();
    triangulationBase->finalizeTriangulationConstruction();
    // Mpi_allreduce that all the flags are 1 (mpi_max)
    int err = dftefe::utils::mpi::MPIAllreduce<dftefe::utils::MemorySpace::HOST>(
      &flag,
      &mpiReducedFlag,
      1,
      dftefe::utils::mpi::MPIInt,
      dftefe::utils::mpi::MPIMax,
      comm);
    std::pair<bool, std::string> mpiIsSuccessAndMsg =
      dftefe::utils::mpi::MPIErrIsSuccessAndMsg(err);
    dftefe::utils::throwException(mpiIsSuccessAndMsg.first,
                          "MPI Error:" + mpiIsSuccessAndMsg.second);
  }

  // initialize the basis Manager

  std::shared_ptr<dftefe::basis::FEBasisManager> basisManager =   std::make_shared<dftefe::basis::EFEBasisManagerDealii<dim>>(
      triangulationBase ,
      atomSphericalDataContainer ,
      feOrder,
      atomPartitionTolerance,
      atomSymbolVec,
      atomCoordinatesVec,
      fieldName,
      comm);
  std::map<dftefe::global_size_type, dftefe::utils::Point> dofCoords;
  basisManager->getBasisCenters(dofCoords);

  std::cout << "Locally owned cells : " << basisManager->nLocallyOwnedCells() << "\n";
  std::cout << "Total Number of dofs : " << basisManager->nGlobalNodes() << "\n";

  // Set the constraints

  std::string constraintHanging = "HangingNodeConstraint"; //give BC to rho
  std::string constraintHomwHan = "HomogeneousWithHanging"; // use this to solve the laplace equation

  std::vector<std::shared_ptr<dftefe::basis::FEConstraintsBase<double, dftefe::utils::MemorySpace::HOST>>>
    constraintsVec;
  constraintsVec.resize(2);
  for ( unsigned int i=0 ;i < constraintsVec.size() ; i++ )
   constraintsVec[i] = std::make_shared<dftefe::basis::EFEConstraintsDealii<double, dftefe::utils::MemorySpace::HOST, dim>>();

  constraintsVec[0]->clear();
  constraintsVec[0]->makeHangingNodeConstraint(basisManager);
  constraintsVec[0]->close();

  constraintsVec[1]->clear();
  constraintsVec[1]->makeHangingNodeConstraint(basisManager);
  constraintsVec[1]->setHomogeneousDirichletBC();
  constraintsVec[1]->close();

  std::map<std::string,
           std::shared_ptr<const dftefe::basis::Constraints<double, dftefe::utils::MemorySpace::HOST>>> constraintsMap;

  constraintsMap[constraintHanging] = constraintsVec[0];
  constraintsMap[constraintHomwHan] = constraintsVec[1];

        // Set up quad attribute
        dftefe::quadrature::QuadratureRuleAttributes quadAttr(dftefe::quadrature::QuadratureFamily::ADAPTIVE,false);

  dftefe::basis::BasisStorageAttributesBoolMap basisAttrMap;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreValues] = true;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreGradient] = false;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreHessian] = false;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreOverlap] = false;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreGradNiGradNj] = true;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreJxW] = true;
  basisAttrMap[dftefe::basis::BasisStorageAttributes::StoreQuadRealPoints] = true;

  // Set up the FE Basis Data Storage
        std::shared_ptr<dftefe::basis::FEBasisDataStorage<double, dftefe::utils::MemorySpace::HOST>> feBasisData =
        std::make_shared<dftefe::basis::EFEBasisDataStorageDealii<double, dftefe::utils::MemorySpace::HOST,dim>>
        (basisManager, quadAttr, basisAttrMap);

        std::shared_ptr<const dftefe::basis::EFEBasisManagerDealii<dim>> efeBM =
        std::dynamic_pointer_cast<const dftefe::basis::EFEBasisManagerDealii<dim>>(basisManager);

        // Set up the vector of scalarSpatialRealFunctions for adaptive quadrature
        std::vector<std::shared_ptr<const dftefe::utils::ScalarSpatialFunctionReal>> functionsVec(0);
        unsigned int numfun = 2;
        functionsVec.resize(numfun); // First and second derivative of the enrichemnt function
        std::vector<double> tolerances(numfun);
        std::vector<double> integralThresholds(numfun);
        for ( unsigned int i=0 ;i < functionsVec.size() ; i++ )
        {
            functionsVec[i] = std::make_shared<dftefe::atoms::AtomSevereFunction<dim>>(        
                efeBM->getEnrichmentIdsPartition(),
                atomSphericalDataContainer,
                atomSymbolVec,
                atomCoordinatesVec,
                fieldName,
                i);
            tolerances[i] = adaptiveQuadTolerance;
            integralThresholds[i] = integralThreshold;
        }

        // Set up base quadrature rule for adaptive quadrature 
        std::shared_ptr<dftefe::quadrature::QuadratureRule> quadRule =
        std::make_shared<dftefe::quadrature::QuadratureRuleGauss>(dim, num1DGaussSize);
        
        // evaluate basis data
        feBasisData->evaluateBasisData(quadAttr,  quadRule, functionsVec,
            tolerances, integralThresholds, smallestCellVolume, maxRecursion, basisAttrMap);

        // Set up BasisHandler
        std::shared_ptr<dftefe::basis::FEBasisHandler<double, dftefe::utils::MemorySpace::HOST, dim>> basisHandler =
        std::make_shared<dftefe::basis::EFEBasisHandlerDealii<double, dftefe::utils::MemorySpace::HOST,dim>>
        (basisManager, constraintsMap, comm);

  // Set up basis Operations
  dftefe::basis::FEBasisOperations<double, double, dftefe::utils::MemorySpace::HOST,dim> feBasisOp(feBasisData,50);

  // set up MPIPatternP2P for the constraints
  auto mpiPatternP2PHanging = basisHandler->getMPIPatternP2P(constraintHanging);
  auto mpiPatternP2PHomwHan = basisHandler->getMPIPatternP2P(constraintHomwHan);

  // set up different multivectors - rho, vh with inhomogeneous BC, vh

  std::shared_ptr<dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>
   vhNHDB = std::make_shared<
    dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>(
      mpiPatternP2PHanging, linAlgOpContext, numComponents, double());

  std::shared_ptr<dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>
   solution = std::make_shared<
    dftefe::linearAlgebra::MultiVector<double, dftefe::utils::MemorySpace::HOST>>(
      mpiPatternP2PHanging, linAlgOpContext, numComponents, double());

  // vector for lhs
  dftefe::utils::Point nodeLoc(dim,0.0);
  auto itField  = vhNHDB->begin();
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
                  const dftefe::global_size_type globalId =
                    iFaceGlobalDofIndices[iFaceDof];
                  if (dofs_touched[globalId])
                    continue;
                  dofs_touched[globalId] = true;
                  if (!basisHandler->getConstraints(constraintHanging).isConstrained(globalId))
                    {
                      dftefe::size_type localId = basisHandler->globalToLocalIndex(globalId,constraintHanging) ;
                      basisHandler->getBasisCenters(localId,constraintHanging,nodeLoc);
                      *(itField + (localId)*(numComponents))  = potential(nodeLoc, atomCoordinates1, rc);
                      *(itField + (localId)*(numComponents) + 1)  = potential(nodeLoc, atomCoordinates2, rc);
                      *(itField + (localId)*(numComponents) + 2)  = potential(nodeLoc, atomCoordinatesVec, rc);          
                    } // non-hanging node check
                }     // Face dof loop
            }
        } // Face loop
    }     // cell locally owned

  // create the quadrature Value Container

        dftefe::basis::LinearCellMappingDealii<dim> linearCellMappingDealii;
        dftefe::quadrature::QuadratureRuleContainer quadRuleContainer =  
                    feBasisData->getQuadratureRuleContainer(quadAttr);

        dftefe::quadrature::QuadratureValuesContainer<double, dftefe::utils::MemorySpace::HOST> quadValuesContainer(quadRuleContainer, numComponents);
        dftefe::quadrature::QuadratureValuesContainer<double, dftefe::utils::MemorySpace::HOST> quadValuesContainerNumerical(quadRuleContainer, numComponents);
        dftefe::quadrature::QuadratureValuesContainer<double, dftefe::utils::MemorySpace::HOST> quadValuesContainerAnalytical(quadRuleContainer, numComponents);


  for(dftefe::size_type i = 0 ; i < quadValuesContainer.nCells() ; i++)
  {
    dftefe::size_type quadId = 0;
    for (auto j : quadRuleContainer.getCellRealPoints(i))
    {
      std::vector<double> a(numComponents, 0);
      a[0] = rho( j, atomCoordinates1, rc);
      a[1] = rho( j, atomCoordinates2, rc);
      a[2] = rho( j, atomCoordinatesVec, rc);
      double *b = a.data();
      quadValuesContainer.setCellQuadValues<dftefe::utils::MemorySpace::HOST> (i, quadId, b);
      quadId = quadId + 1;
    }
  }

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

  // perform integral rho vh 

  feBasisOp.interpolate( *solution, constraintHanging, *basisHandler, quadAttr, quadValuesContainerNumerical);

  for(dftefe::size_type i = 0 ; i < quadValuesContainerAnalytical.nCells() ; i++)
  {
      dftefe::size_type quadId = 0;
      for (auto j : quadRuleContainer.getCellRealPoints(i))
      {
          std::vector<double> a(numComponents, 0);
          a[0] = potential( j, atomCoordinates1, rc);
          a[1] = potential( j, atomCoordinates2, rc);
          a[2] = potential( j, atomCoordinatesVec, rc);
          double *b = a.data();
          quadValuesContainerAnalytical.setCellQuadValues<dftefe::utils::MemorySpace::HOST> (i, quadId, b);
          quadId = quadId + 1;
      }
  }

  auto iter1 = quadValuesContainer.begin();
  auto iter2 = quadValuesContainerNumerical.begin();
  auto iter3 = quadValuesContainerAnalytical.begin();
  dftefe::size_type numQuadraturePoints = quadRuleContainer.nQuadraturePoints(), mpinumQuadraturePoints=0;
  const std::vector<double> JxW = quadRuleContainer.getJxW();
  std::vector<double> integral(9, 0.0), mpiReducedIntegral(integral.size(), 0.0);
  for (unsigned int i = 0 ; i < numQuadraturePoints ; i++ )
  {
    for (unsigned int j = 0 ; j < numComponents ; j++ )
    {
      integral[j] += *(i*numComponents+j+iter1) * *(i*numComponents+j+iter2) * JxW[i] * 0.5/(4*M_PI);
    }
    integral[3] += *(i*numComponents+0+iter1) * *(i*numComponents+1+iter2) * JxW[i] * 0.5/(4*M_PI);
    integral[4] += *(i*numComponents+1+iter1) * *(i*numComponents+0+iter2) * JxW[i] * 0.5/(4*M_PI);
    integral[5] += *(i*numComponents+0+iter1) * JxW[i]/(4*M_PI);
    integral[6] += *(i*numComponents+1+iter1) * JxW[i]/(4*M_PI);
    integral[7] += *(i*numComponents+0+iter1) * *(i*numComponents+0+iter3) * JxW[i] * 0.5/(4*M_PI);
    integral[8] += *(i*numComponents+1+iter1) * *(i*numComponents+1+iter3) * JxW[i] * 0.5/(4*M_PI);
  }

        dftefe::utils::mpi::MPIAllreduce<dftefe::utils::MemorySpace::HOST>(
            &numQuadraturePoints,
            &mpinumQuadraturePoints,
            1,
            dftefe::utils::mpi::MPIUnsigned,
            dftefe::utils::mpi::MPISum,
            comm);

  dftefe::utils::mpi::MPIAllreduce<dftefe::utils::MemorySpace::HOST>(
        integral.data(),
        mpiReducedIntegral.data(),
        integral.size(),
        dftefe::utils::mpi::MPIDouble,
        dftefe::utils::mpi::MPISum,
        comm);

  double Ig = 10976./(17875*rc);
  double vg0 = potential(atomCoordinates1[0], atomCoordinates1, rc);
  double analyticalSelfPotantial = 0.5 * (Ig - vg0) ;

    double dist = 0;
    for (unsigned int j = 0 ; j < dim ; j++ )
    {
      dist += std::pow((atomCoordinates1[0][j]-atomCoordinates2[0][j]),2);
    }
    dist = std::sqrt(dist);
    
    std::cout << "The error in electrostatic energy: " << (mpiReducedIntegral[2] + 2*analyticalSelfPotantial) - 1.0/dist << "\n";

        if(rank == 0)
        {
        std::ofstream myfile;
        std::stringstream ss;
        ss << "EFE"<<subdivisionx<<"x"<<subdivisiony<<"x"<<subdivisionz<<
        "feOrder_"<<feOrder<<"hMin_"<<hMin<<"adapTol_"<<adaptiveQuadTolerance<<".out";
        std::string outputFile = ss.str();
        myfile.open (outputFile, std::ios::out | std::ios::trunc);
          myfile << "Total Number of dofs : " << basisManager->nGlobalNodes() << "\n";
          myfile << "No. of quad points: "<< mpinumQuadraturePoints << "\n";
          myfile << "Integral of b s' over volume: "<< mpiReducedIntegral[5] << "," << mpiReducedIntegral[6] << "\n";
          myfile << "The electrostatic energy (analy/num) : "<< (mpiReducedIntegral[2] + 2*analyticalSelfPotantial) << "," << (mpiReducedIntegral[2] - (mpiReducedIntegral[0] + mpiReducedIntegral[1]))  << "\n";
          myfile << "The error in electrostatic energy from analytical self potential: " << (mpiReducedIntegral[2] + 2*analyticalSelfPotantial) - 1.0/dist << " Relative error: "<<((mpiReducedIntegral[2] + 2*analyticalSelfPotantial) - 1.0/dist)*dist<<"\n";
          myfile << "The error in electrostatic energy from numerical self potntial : " << (mpiReducedIntegral[2] - (mpiReducedIntegral[0] + mpiReducedIntegral[1])) - 1.0/dist << " Relative error: "<<((mpiReducedIntegral[2] - (mpiReducedIntegral[0] + mpiReducedIntegral[1])) - 1.0/dist)*dist<<"\n";
          myfile << "The error in electrostatic energy from analytical self potential using adaptive quad: " << (mpiReducedIntegral[2] - (mpiReducedIntegral[7] + mpiReducedIntegral[8])) - 1.0/dist << " Relative error: "<<((mpiReducedIntegral[2] - (mpiReducedIntegral[7] + mpiReducedIntegral[8])) - 1.0/dist)*dist<<"\n";
        myfile.close();
        }        

  //gracefully end MPI

  int mpiFinalFlag = 0;
  dftefe::utils::mpi::MPIFinalized(&mpiFinalFlag);
  if(!mpiFinalFlag)
  {
    dftefe::utils::mpi::MPIFinalize();
  }
}
