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
  T van_leer_limiter(const T& dq, const T& dm, const T& small) {
    // van Leer limiter
    T limit = (2.0*(dm*dq+stk::math::abs(dm*dq))) /
      ((dm+dq)*(dm+dq)+small);
    return limit;
  }

  template<typename T>
  T minmod_limiter(const T& dq, const T& dm, const T& small) {
    // Minmod limiter
    const T r = dq / (dm + small);
    return stk::math::max(T(0.0), stk::math::min(T(1.0), r));
  }

  template<typename T>
  T superbee_limiter(const T& dq, const T& dm, const T& small) {
    // superbee_limiter
    const T r = dq / (dm + small);
    const T a = stk::math::min(2.0 * r, T(1.0));
    const T b = stk::math::min(r, T(2.0));
    return stk::math::max(T(0.0), stk::math::max(a, b));
  }

  template<typename T>
  T ultrabee_limiter(const T& dq, const T& dm, const T& small) {
    // ultrabee_limiter
    const T r = dq / (dm + small);
    const T limit = stk::math::min(T(2.0), stk::math::min(2.0 * r, 1.0 + r));
    return stk::math::if_then_else(r > 0.0, limit, T(0.0));
  }

template double van_leer_limiter<double>(const double&, const double&, const double&);
#ifdef STK_HAVE_SIMD
template DoubleType van_leer_limiter<DoubleType>(const DoubleType&, const DoubleType&, const DoubleType&);
#endif

template double minmod_limiter<double>(const double&, const double&, const double&);
#ifdef STK_HAVE_SIMD
template DoubleType minmod_limiter<DoubleType>(const DoubleType&, const DoubleType&, const DoubleType&);
#endif

template double superbee_limiter<double>(const double&, const double&, const double&);
#ifdef STK_HAVE_SIMD
template DoubleType superbee_limiter<DoubleType>(const DoubleType&, const DoubleType&, const DoubleType&);
#endif

template double ultrabee_limiter<double>(const double&, const double&, const double&);
#ifdef STK_HAVE_SIMD
template DoubleType ultrabee_limiter<DoubleType>(const DoubleType&, const DoubleType&, const DoubleType&);
#endif

} // namespace nalu
} // namespace Sierra
