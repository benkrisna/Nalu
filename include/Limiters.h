/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


#ifndef Limiters_h
#define Limiters_h

namespace sierra{
namespace nalu{

double van_leer_limiter(const double& dq, const double& dm, const double& small=1e-10);

} // namespace nalu
} // namespace Sierra

#endif
