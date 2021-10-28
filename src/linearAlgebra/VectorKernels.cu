#ifdef DFTEFE_WITH_DEVICE_CUDA
#  include <DeviceKernelLauncher.h>
#  include "DeviceDataTypeOverloads.cuh"
#  include "VectorKernels.h"
namespace dftefe
{
  namespace linearAlgebra
  {
    namespace
    {
      template <typename ValueType>
      __global__ void
      addCUDAKernel(size_type size, const ValueType *u, ValueType *v)
      {
        const unsigned int globalThreadId =
          blockIdx.x * blockDim.x + threadIdx.x;
        for (unsigned int i = globalThreadId; i < size;
             i += blockDim.x * gridDim.x)
          {
            v[i] = dftefe::utils::add(v[i], u[i]);
          }
      }

      template <typename ValueType>
      __global__ void
      subCUDAKernel(size_type size, const ValueType *u, ValueType *v)
      {
        const unsigned int globalThreadId =
          blockIdx.x * blockDim.x + threadIdx.x;
        for (unsigned int i = globalThreadId; i < size;
             i += blockDim.x * gridDim.x)
          {
            v[i] = dftefe::utils::sub(v[i], u[i]);
          }
      }

      template <typename ValueType>
      __global__ void
      addCUDAKernel(size_type        size,
                    const ValueType  a,
                    const ValueType *u,
                    const ValueType  b,
                    const ValueType *v,
                    ValueType       *w)
      {
        const unsigned int globalThreadId =
          blockIdx.x * blockDim.x + threadIdx.x;
        for (unsigned int i = globalThreadId; i < size;
             i += blockDim.x * gridDim.x)
          {
            w[i] = dftefe::utils::add(dftefe::utils::mult(a, u[i]),
                                      dftefe::utils::mult(b, v[i]));
          }
      }

    } // namespace

    template <typename ValueType>
    void
    VectorKernels<ValueType, dftefe::utils::MemorySpace::DEVICE>::add(
      const size_type  size,
      const ValueType *u,
      ValueType       *v)
    {
      addCUDAKernel<<<size / dftefe::utils::BLOCK_SIZE + 1,
                      dftefe::utils::BLOCK_SIZE>>>(
        size,
        dftefe::utils::makeDataTypeDeviceCompatible(u),
        dftefe::utils::makeDataTypeDeviceCompatible(v));
    }

    template <typename ValueType>
    void
    VectorKernels<ValueType, dftefe::utils::MemorySpace::DEVICE>::sub(
      const size_type  size,
      const ValueType *u,
      ValueType       *v)
    {
      subCUDAKernel<<<size / dftefe::utils::BLOCK_SIZE + 1,
                      dftefe::utils::BLOCK_SIZE>>>(
        size,
        dftefe::utils::makeDataTypeDeviceCompatible(u),
        dftefe::utils::makeDataTypeDeviceCompatible(v));
    }


    template <typename ValueType>
    void
    VectorKernels<ValueType, dftefe::utils::MemorySpace::DEVICE>::add(
      size_type        size,
      ValueType        a,
      const ValueType *u,
      ValueType        b,
      const ValueType *v,
      ValueType       *w)
    {
      addCUDAKernel<<<size / dftefe::utils::BLOCK_SIZE + 1,
                      dftefe::utils::BLOCK_SIZE>>>(
        size,
        dftefe::utils::makeDataTypeDeviceCompatible(a),
        dftefe::utils::makeDataTypeDeviceCompatible(u),
        dftefe::utils::makeDataTypeDeviceCompatible(b),
        dftefe::utils::makeDataTypeDeviceCompatible(v),
        dftefe::utils::makeDataTypeDeviceCompatible(w));
    }

    template class VectorKernels<size_type, dftefe::utils::MemorySpace::DEVICE>;
    template class VectorKernels<int, dftefe::utils::MemorySpace::DEVICE>;
    template class VectorKernels<double, dftefe::utils::MemorySpace::DEVICE>;
    template class VectorKernels<float, dftefe::utils::MemorySpace::DEVICE>;
    template class VectorKernels<std::complex<double>,
                                 dftefe::utils::MemorySpace::DEVICE>;
    template class VectorKernels<std::complex<float>,
                                 dftefe::utils::MemorySpace::DEVICE>;
  } // namespace linearAlgebra
} // namespace dftefe
#endif
