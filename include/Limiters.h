/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/
#include<FieldTypeDef.h>

namespace sierra {
  namespace nalu {
    double van_leer_limiter(const DoubleType &dqm, const DoubleType &dqp);
  } // namespace nalu
} // namespace sierra
