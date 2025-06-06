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
    const T prod = dm * dq;
    const T minAbs = stk::math::min(stk::math::abs(dm), stk::math::abs(dq));
    const T limiter = stk::math::if_else(prod > 0.0, minAbs, T(0.0));
    return limiter / (stk::math::abs(dm) + small);
  }

  template<typename T>
  T superbee_limiter(const T& dq, const T& dm, const T& small) {
    // superbee_limiter
    const T prod = dm * dq;
    const T two_dq = 2.0 * stk::math::abs(dq);
    const T two_dm = 2.0 * stk::math::abs(dm);

    const T min1 = stk::math::min(two_dq, stk::math::abs(dm));
    const T min2 = stk::math::min(stk::math::abs(dq), two_dm);
    const T maxMin = stk::math::max(min1, min2);

    const T limiter = stk::math::if_else(prod > 0.0, maxMin, T(0.0));
    return limiter / (stk::math::abs(dm) + small);
  }

  template<typename T>
  T ultrabee_limiter(const T& dq, const T& dm, const T& small) {
    // ultrabee_limiter
    const T prod = dm * dq;
    const T abs_dm = stk::math::abs(dm);
    const T abs_dq = stk::math::abs(dq);

    const T two_dm = 2.0 * abs_dm;
    const T two_dq = 2.0 * abs_dq;
    const T one_plus = abs_dm + abs_dq;

    const T minAll = stk::math::min(stk::math::min(two_dm, two_dq), one_plus);
    const T limiter = stk::math::if_else(prod > 0.0, minAll, T(0.0));
    return limiter / (abs_dm + small);
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
