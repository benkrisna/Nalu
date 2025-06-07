/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


#ifndef MUSCL_h
#define MUSCL_h

namespace sierra{
namespace nalu{

template<typename T>
T muscl_execute(const T& qL, const T& qR,
                const T& dqL, const T& dqR,
                T& qIpL, T& qIpR);

} // namespace nalu
} // namespace Sierra

#endif
