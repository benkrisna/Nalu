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

// ----------------------------------------------------------------------
/*
 * @brief: MUSCL execute function
 * @paramIn: qL, qR: left and right states
 * @paramIn: dqL, dqR: left and right reconstructed gradients * dxL, dxR
 * @paramOut: qIpL, qIpR: left and right MUSCL interpolated states
 * @paramIn: limiterType: type of limiter to apply
 */
// ----------------------------------------------------------------------
template<typename T>
void muscl_execute(const T& qL, const T& qR,
    const T& dqL, const T& dqR,
    T& qIpL, T& qIpR, 
    const bool useLimiter,
    const std::string& limiterType) {
  const T dq = qR - qL;

  T (*limiterFunc)(const T&, const T &, const T&) = nullptr;
  // Determine limiter based on the limiterType input
  if (limiterType == "van_leer") {
    limiterFunc = van_leer_limiter<T>;
  } else if (limiterType == "minmod") {
    limiterFunc = minmod_limiter<T>;
  } else if (limiterType == "superbee") {
    limiterFunc = superbee_limiter<T>;
  } else if (limiterType == "ultrabee") {
    limiterFunc = ultrabee_limiter<T>;
  } else {
    throw std::runtime_error("MUSCL::muscl_execute >> Unknown MUSCL limiter type: " + limiterType);
  }

  T limitL = 1.0; T limitR = 1.0;
  if (useLimiter) {
    const T small = 1e-10;
    limitL = limiterFunc(dqL, dq, small); limitR = limiterFunc(dqR, dq, small);
  }

  // compute the MUSCL interpolated states
  qIpL = qL + 0.5 * limitL * dqL;
  qIpR = qR - 0.5 * limitR * dqR;
} // muscl_execute
// ----------------------------------------------------------------------

template void muscl_execute<double>(const double&, const double&,
    const double&, const double&,
    double&, double&, const bool, const std::string&);
#ifdef STK_HAVE_SIMD
template void muscl_execute<DoubleType>(const DoubleType&, const DoubleType&,
    const DoubleType&, const DoubleType&,
    DoubleType&, DoubleType&, const bool, const std::string&);
#endif

} // namespace nalu
} // namespace Sierra
