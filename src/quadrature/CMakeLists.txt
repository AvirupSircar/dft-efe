if (NOT TARGET dft-efe-quadrature)
find_package(deal.II 9.3.0 REQUIRED)
include_directories(${DEAL_II_INCLUDE_DIRS})

if(NOT DEFINED CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()


if(ENABLE_MPI)
	add_compile_definitions(DFTEFE_WITH_MPI)
	if (NOT MPI_FOUND)
		find_package(MPI REQUIRED)
		target_include_directories(dft-efe-quadrature PUBLIC ${MPI_CXX_INCLUDE_PATH})
		set(DFTEFE_MPI_LIBRARIES ${MPI_CXX_LIBRARIES})
	endif()
endif()

# Add source files here
set(DFT-EFE-QUADRATURE-SOURCES
	QuadratureAttributes.cpp
	QuadratureRule.cpp
	QuadratureRuleContainer.cpp
        QuadratureRuleGauss.cpp
	QuadratureRuleGLL.cpp
	QuadratureRuleAdaptive.cpp
	#QuadratureValuesContainer.cpp
	Integrate.cpp)

add_library(dft-efe-quadrature SHARED ${DFT-EFE-QUADRATURE-SOURCES})

if (NOT TARGET dft-efe-utils)
  add_subdirectory(../utils ${CMAKE_CURRENT_BINARY_DIR}/buildUtils)
endif()

include_directories(../)
#target_link_libraries(dft-efe-quadrature PUBLIC dft-efe-utils dft-efe-linalg ${DFTEFE_MPI_LIBRARIES})
target_link_libraries(dft-efe-quadrature PUBLIC dft-efe-utils ${DFTEFE_MPI_LIBRARIES})
endif()
