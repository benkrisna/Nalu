/*------------------------------------------------------------------------*/
/*  Copyright 2014 National Renewable Energy Laboratory.                  */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/

#include "kernels/UnitTestKernelUtils.h"
#include "UnitTestUtils.h"
#include "UnitTestHelperObjects.h"

#include "kernel/MomentumBuoyancyBoussinesqSrcElemKernel.h"

#include <random>

TEST_F(MomentumKernelHex8Mesh, buoyancy_boussinesq)
{
  std::mt19937 rng;
  rng.seed(0); // fixed seed
  std::uniform_real_distribution<double> ref_densities(0.8,1.3);

  fill_mesh_and_init_fields();

  // Setup solution options for default advection kernel
  solnOpts_.meshMotion_ = false;
  solnOpts_.meshDeformation_ = false;
  solnOpts_.externalMeshDeformation_ = false;
  solnOpts_.gravity_[2] = -9.81;
  solnOpts_.referenceDensity_ = ref_densities(rng);
  solnOpts_.referenceTemperature_ = 298;
  solnOpts_.thermalExpansionCoeff_ = 1.0;

  unit_test_utils::HelperObjects helperObjs(bulk_, stk::topology::HEX_8, 3, partVec_[0]);

  // Initialize the kernel
  std::unique_ptr<sierra::nalu::Kernel> kernel(
    new sierra::nalu::MomentumBuoyancyBoussinesqSrcElemKernel<sierra::nalu::AlgTraitsHex8>(
      *bulk_, solnOpts_, helperObjs.assembleElemSolverAlg->dataNeededByKernels_));

  // Add to kernels to be tested
  helperObjs.assembleElemSolverAlg->activeKernels_.push_back(kernel.get());

  helperObjs.assembleElemSolverAlg->execute();

  EXPECT_EQ(helperObjs.linsys->lhs_.extent(0), 24u);
  EXPECT_EQ(helperObjs.linsys->lhs_.extent(1), 24u);
  EXPECT_EQ(helperObjs.linsys->rhs_.extent(0), 24u);

  double expFac = solnOpts_.referenceDensity_ * solnOpts_.thermalExpansionCoeff_;

  // Exact solution
  std::vector<double> rhsExact(24,0.0);
  for (size_t i=2; i < 24; i += 3)
    rhsExact[i] = -0.125 * solnOpts_.gravity_[2] * expFac * (300.0 - solnOpts_.referenceTemperature_);

  unit_test_kernel_utils::expect_all_near(helperObjs.linsys->rhs_,rhsExact.data());
}

