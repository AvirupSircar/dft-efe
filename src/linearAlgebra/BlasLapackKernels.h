#ifndef dftefeKernels_h
#define dftefeKernels_h

#include <utils/MemoryManager.h>
#include <linearAlgebra/BlasLapackTypedef.h>
#include <linearAlgebra/LinAlgOpContext.h>
#include <blas.hh>
#include <vector>

namespace dftefe
{
  namespace linearAlgebra
  {
    namespace blasLapack
    {
      /**
       * @brief namespace class for BlasLapack kernels not present in blaspp.
       */
      template <typename ValueType1,
                typename ValueType2,
                dftefe::utils::MemorySpace memorySpace>
      class KernelsTwoValueTypes
      {
      public:
        /**
         * @brief Template for performing \f$ z = \alpha x$
         * @param[in] size size of the array
         * @param[in] \f$ alpha \f$ scalar
         * @param[in] x array
         * @param[out] z array
         */
        static void
        ascale(size_type                            size,
               ValueType1                           alpha,
               const ValueType2 *                   x,
               scalar_type<ValueType1, ValueType2> *z);

        /**
         * @brief Template for performing \f$ z_i = x_i * y_i$
         * @param[in] size size of the array
         * @param[in] x array
         * @param[in] y array
         * @param[out] z array
         */
        static void
        hadamardProduct(size_type                            size,
                        const ValueType1 *                   x,
                        const ValueType2 *                   y,
                        scalar_type<ValueType1, ValueType2> *z);

        /**
         * @brief Template for performing \f$ {\bf Z}={\bf A} \odot {\bf B} = a_1 \otimes b_1
         * \quad a_2 \otimes b_2 \cdots \a_K \otimes b_K \f$, where \f${\bf
         * A}\f$ is  \f$I \times K\f$ matrix, \f${\bf B}\f$ is \f$J \times K\f$,
         * and \f$ {\bf Z} \f$ is \f$ (IJ)\times K \f$ matrix. All the matrices
         * are assumed to be stored in column major format
         * @param[in] size size I
         * @param[in] size size J
         * @param[in] size size K
         * @param[in] X array
         * @param[in] Y array
         * @param[out] Z array
         */
        static void
        khatriRaoProduct(size_type                            sizeI,
                         size_type                            sizeJ,
                         size_type                            sizeK,
                         const ValueType1 *                   A,
                         const ValueType2 *                   B,
                         scalar_type<ValueType1, ValueType2> *Z);

        /**
         * @brief Template for performing \f$ z = \alpha x + \beta y \f$
         * @param[in] size size of the array
         * @param[in] \f$ alpha \f$ scalar
         * @param[in] x array
         * @param[in] \f$ beta \f$ scalar
         * @param[in] y array
         * @param[out] z array
         */
        static void
        axpby(size_type                            size,
              scalar_type<ValueType1, ValueType2>  alpha,
              const ValueType1 *                   x,
              scalar_type<ValueType1, ValueType2>  beta,
              const ValueType2 *                   y,
              scalar_type<ValueType1, ValueType2> *z);

        /**
         * @brief Template for computing dot products numVec vectors in a multi Vector
         * @param[in] vecSize size of each vector
         * @param[in] numVec number of vectors in the multi Vector
         * @param[in] multiVecDataX multi vector data in row major format i.e.
         * vector index is the fastest index
         * @param[in] multiVecDataY multi vector data in row major format i.e.
         * vector index is the fastest index
         * @param[out] multiVecDotProduct multi vector dot product of size
         * numVec
         *
         */
        static void
        dotMultiVector(size_type                            vecSize,
                       size_type                            numVec,
                       const ValueType1 *                   multiVecDataX,
                       const ValueType2 *                   multiVecDataY,
                       scalar_type<ValueType1, ValueType2> *multiVecDotProduct,
                       LinAlgOpContext<memorySpace> &       context);
      };

      template <typename ValueType, dftefe::utils::MemorySpace memorySpace>
      class KernelsOneValueType
      {
      public:
        /**
         * @brief Template for computing \f$ l_{\inf} \f$ norms of all the numVec vectors in a multi Vector
         * @param[in] vecSize size of each vector
         * @param[in] numVec number of vectors in the multi Vector
         * @param[in] multiVecData multi vector data in row major format i.e.
         * vector index is the fastest index
         *
         * @return \f$ l_{\inf} \f$  norms of all the vectors
         */
        static std::vector<double>
        amaxsMultiVector(size_type        vecSize,
                         size_type        numVec,
                         const ValueType *multiVecData);

        /**
         * @brief Template for computing \f$ l_2 \f$ norms of all the numVec vectors in a multi Vector
         * @param[in] vecSize size of each vector
         * @param[in] numVec number of vectors in the multi Vector
         * @param[in] multiVecData multi vector data in row major format i.e.
         * vector index is the fastest index
         *
         * @return \f$ l_2 \f$  norms of all the vectors
         */
        static std::vector<double>
        nrms2MultiVector(size_type                     vecSize,
                         size_type                     numVec,
                         const ValueType *             multiVecData,
                         LinAlgOpContext<memorySpace> &context);
      };

#ifdef DFTEFE_WITH_DEVICE
      template <typename ValueType1, typename ValueType2>
      class KernelsTwoValueTypes<ValueType1,
                                 ValueType2,
                                 dftefe::utils::MemorySpace::DEVICE>
      {
      public:
        static void
        ascale(size_type                            size,
               ValueType1                           alpha,
               const ValueType2 *                   x,
               scalar_type<ValueType1, ValueType2> *z);


        static void
        hadamardProduct(size_type                            size,
                        const ValueType1 *                   x,
                        const ValueType2 *                   y,
                        scalar_type<ValueType1, ValueType2> *z);


        static void
        khatriRaoProduct(size_type                            sizeI,
                         size_type                            sizeJ,
                         size_type                            sizeK,
                         const ValueType1 *                   A,
                         const ValueType2 *                   B,
                         scalar_type<ValueType1, ValueType2> *Z);

        static void
        axpby(size_type                            size,
              scalar_type<ValueType1, ValueType2>  alpha,
              const ValueType1 *                   x,
              scalar_type<ValueType1, ValueType2>  beta,
              const ValueType2 *                   y,
              scalar_type<ValueType1, ValueType2> *z);

        static void
        dotMultiVector(
          size_type                            vecSize,
          size_type                            numVec,
          const ValueType1 *                   multiVecDataX,
          const ValueType2 *                   multiVecDataY,
          scalar_type<ValueType1, ValueType2> *multiVecDotProduct,
          LinAlgOpContext<dftefe::utils::MemorySpace::DEVICE> &context);
      };

      template <typename ValueType>
      class KernelsOneValueType<ValueType, dftefe::utils::MemorySpace::DEVICE>
      {
      public:
        static std::vector<double>
        amaxsMultiVector(size_type        vecSize,
                         size_type        numVec,
                         const ValueType *multiVecData);


        static std::vector<double>
        nrms2MultiVector(
          size_type                                            vecSize,
          size_type                                            numVec,
          const ValueType *                                    multiVecData,
          LinAlgOpContext<dftefe::utils::MemorySpace::DEVICE> &context);
      };

#endif

    } // namespace blasLapack
  }   // namespace linearAlgebra
} // namespace dftefe

#endif // dftefeKernels_h
