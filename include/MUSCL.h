/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


#ifndef MUSCL_h
#define MUSCL_h

#include <string>

namespace sierra{
namespace nalu{

template<typename T>
void muscl_execute(const T& qL, const T& qR,
                const T& dqL, const T& dqR,
                T& qIpL, T& qIpR,
                const bool useLimiter=true,
                const std::string& limiterType = "van_leer");

} // namespace nalu
} // namespace Sierra

#endif
