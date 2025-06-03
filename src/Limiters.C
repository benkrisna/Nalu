/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


#include "Limiters.h"
#include "SimdInterface.h"

// basic c++
#include <algorithm>
#include <cmath>

namespace sierra{
namespace nalu{
  template<typename T>
  T van_leer_limiter(const T& dq, const T& dm, const T& small)
  {
    // van Leer limiter
    T limit = (2.0*(dm*dq+stk::math::abs(dm*dq))) /
      ((dm+dq)*(dm+dq)+small);
    return limit;
  }

#ifdef STK_HAVE_SIMD
template DoubleType van_leer_limiter<DoubleType>(const DoubleType&, const DoubleType&, const DoubleType&);
#endif

} // namespace nalu
} // namespace Sierra
