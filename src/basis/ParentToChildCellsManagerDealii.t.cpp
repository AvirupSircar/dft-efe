#include <utils/Exceptions.h>
#include <utils/Point.h>
#include <utils/DealiiConversions.h>
namespace dftefe
{
  namespace basis
  {
    template <unsigned int dim>
    ParentToChildCellsManagerDealii::ParentToChildCellsManagerDealii()
      : d_triangulationDealiiSerialVector(0)
    {}

    template <unsigned int dim>
    ParentToChildCellsManagerDealii::~ParentToChildCellsManagerDealii()
    {}

    template <unsigned int dim>
    std::vector<std::shared_ptr<const TriangulationCellBase>>
    ParentToChildCellsManagerDealii::createChildCells(
      const TriangulationCellBase &parentCell)
    {
      std::vector<utils::Point> vertices(0, Point(dim, 0.0));
      parentCell.getVertices(vertices);
      auto triangulationDealiiSerial =
        std::make_shared<TriangulationDealiiSerial<dim>>();
      d_triangulationDealiiSerialVector.push_back(triangulationDealiiSerial);
      triangulationDealiiSerial.initializeTriangulationConstruction();
      triangulationDealiiSerial.createSingleCellTriangulation(vertices);
      triangulationDealiiSerial.refineGlobal(1);
      triangulationDealiiSerial.finalizeTriangulationConstruction();
      const size_type numberCells = triangulationDealiiSerial.nLocalCells();
      std::vector<std::shared_ptr<const TriangulationCellBase>> returnValue(
        numberCells);

      TriangulationCellBase::const_cellIterator cellIter =
        triangulationDealiiSerial.beginLocal();
      unsigned int iCell = 0;
      for (; cellIter != triangulationDealiiSerial.endLocal(); ++cellIter)
        {
          returnValue[iCell] = *cellIter;
          iCell++;
        }

      return returnValue;
    }

    template <unsigned int dim>
    void
    ParentToChildCellsManagerDealii::popLast()
    {
      d_triangulationDealiiSerialVector.pop_back();
    }

  } // end of namespace basis

} // end of namespace dftefe
