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

template<typename T>
T van_leer_limiter(const T& dq, const T& dm, const T& small=1e-10);

template<typename T>
T minmod_limiter(const T& dq, const T& dm, const T& small=1e-10);

template<typename T>
T superbee_limiter(const T& dq, const T& dm, const T& small=1e-10);

template<typename T>
T ultrabee_limiter(const T& dq, const T& dm, const T& small=1e-10);

} // namespace nalu
} // namespace Sierra

#endif
