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
  double van_leer_limiter(const double& dq, const double& dm, const double& small)
  {
    // van Leer limiter
    // https://en.wikipedia.org/wiki/Van_Leer_limiter
    if (std::abs(dq) < small && std::abs(dm) < small) {
      return 1.0;
    }
    else if (dm * dq > 0.0) {
      return 2.0 * dq / (dq + dm);
    }
    else {
      return 0.0;
    }
  }
} // namespace nalu
} // namespace Sierra
