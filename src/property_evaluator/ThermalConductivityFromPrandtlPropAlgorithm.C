/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


#include <Algorithm.h>
#include <property_evaluator/ThermalConductivityFromPrandtlPropAlgorithm.h>
#include <FieldTypeDef.h>
#include <Realm.h>

#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/GetBuckets.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/Selector.hpp>

namespace sierra{
namespace nalu{

//==========================================================================
// Class Definition
//==========================================================================
// ThermalConductivityFromPrandtlPropAlgorithm - compute k from Pr
//==========================================================================
//--------------------------------------------------------------------------
//-------- constructor -----------------------------------------------------
//--------------------------------------------------------------------------
ThermalConductivityFromPrandtlPropAlgorithm::ThermalConductivityFromPrandtlPropAlgorithm(
  Realm & realm,
  stk::mesh::Part * part,
  ScalarFieldType *thermalCond,
  ScalarFieldType *specHeat,
  ScalarFieldType *viscosity,
  const double Pr)
  : Algorithm(realm, part),
    thermalCond_(thermalCond),
    specHeat_(specHeat),
    viscosity_(viscosity),
    Pr_(Pr)
{
  // does nothing
}

//--------------------------------------------------------------------------
//-------- execute ---------------------------------------------------------
//--------------------------------------------------------------------------
void
ThermalConductivityFromPrandtlPropAlgorithm::execute()
{
  // make sure that partVec_ is size one
  STK_ThrowAssert( partVec_.size() == 1 );

  stk::mesh::Selector selector = stk::mesh::selectUnion(partVec_);

  stk::mesh::BucketVector const& node_buckets =
    realm_.get_buckets( stk::topology::NODE_RANK, selector );

  for ( stk::mesh::BucketVector::const_iterator ib = node_buckets.begin();
        ib != node_buckets.end() ; ++ib ) {
    stk::mesh::Bucket & b = **ib ;
    const stk::mesh::Bucket::size_type length   = b.size();

    double *thermalCond = stk::mesh::field_data(*thermalCond_, b);
    const double *specHeat = stk::mesh::field_data(*specHeat_, b );
    const double *viscosity = stk::mesh::field_data(*viscosity_, b );
    
    for ( stk::mesh::Bucket::size_type k = 0 ; k < length ; ++k ) {
      thermalCond[k] = specHeat[k]*viscosity[k]/Pr_;
    }
  }
}

} // namespace nalu
} // namespace Sierra
