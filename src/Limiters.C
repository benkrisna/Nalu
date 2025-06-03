/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/
#include "Limiters.h"

namespace sierra {
  namespace nalu {
    double van_leer_limiter(const DoubleType &dqm, const DoubleType &dqp) {
      DoubleType limit = (2.0*(dqm*dqp+stk::math::abs(dqm*dqp))) /
        ((dqm+dqp)*(dqm+dqp)+small_);
      return limit;
    }
  } // namespace nalu
} // namespace sierra
