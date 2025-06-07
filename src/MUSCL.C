/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


#include "Limiters.h"
#include "MUSCL.h"
#include "SimdInterface.h"

// basic c++
#include <algorithm>
#include <cmath>

namespace sierra{
namespace nalu{
  template<typename T>
    T muscl_execute(const T& qL, const T& qR,
        const T& dqL, const T& dqR,
        T& qIpL, T& qIpR) {
    }

template double muscl_execute<double>(const double&, const double&,
    const double&, const double&,
    double&, double&);
#ifdef STK_HAVE_SIMD
template DoubleType muscl_execute<DoubleType>(const DoubleType&, const DoubleType&,
    const DoubleType&, const DoubleType&,
    DoubleType&, DoubleType&);
#endif

} // namespace nalu
} // namespace Sierra
