/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


// nalu
#include <AssemblePNGPressureBoundarySolverAlgorithm.h>
#include <EquationSystem.h>
#include <FieldTypeDef.h>
#include <LinearSystem.h>
#include <Realm.h>
#include <SolutionOptions.h>
#include <TimeIntegrator.h>
#include <master_element/MasterElement.h>

// stk_mesh/base/fem
#include <stk_mesh/base/BulkData.hpp>
#include <stk_mesh/base/Field.hpp>
#include <stk_mesh/base/GetBuckets.hpp>
#include <stk_mesh/base/GetEntities.hpp>
#include <stk_mesh/base/MetaData.hpp>
#include <stk_mesh/base/Part.hpp>

namespace sierra{
namespace nalu{

//==========================================================================
// Class Definition
//==========================================================================
// AssemblePNGPressureBoundarySolverAlgorithm - lhs for scalar projected 
//                                              nodal gradient, specialized
//                                              for open pressure
//==========================================================================
//--------------------------------------------------------------------------
//-------- constructor -----------------------------------------------------
//--------------------------------------------------------------------------
AssemblePNGPressureBoundarySolverAlgorithm::AssemblePNGPressureBoundarySolverAlgorithm(
  Realm &realm,
  stk::mesh::Part *part,
  EquationSystem *eqSystem,
  std::string independentDofName)
  : SolverAlgorithm(realm, part, eqSystem),
    scalarQ_(NULL),
    exposedAreaVec_(NULL)
{
  // save off fields 
  stk::mesh::MetaData & meta_data = realm_.meta_data();
  scalarQ_ = meta_data.get_field<double>(stk::topology::NODE_RANK, independentDofName);
  dynamicP_ = meta_data.get_field<double>(meta_data.side_rank(), "dynamic_pressure");
  exposedAreaVec_ = meta_data.get_field<double>(meta_data.side_rank(), "exposed_area_vector");
}

//--------------------------------------------------------------------------
//-------- initialize_connectivity -----------------------------------------
//--------------------------------------------------------------------------
void
AssemblePNGPressureBoundarySolverAlgorithm::initialize_connectivity()
{
  eqSystem_->linsys_->buildFaceToNodeGraph(partVec_);
}

//--------------------------------------------------------------------------
//-------- execute ---------------------------------------------------------
//--------------------------------------------------------------------------
void
AssemblePNGPressureBoundarySolverAlgorithm::execute()
{
  stk::mesh::MetaData & meta_data = realm_.meta_data();

  const int nDim = meta_data.spatial_dimension();

  // space for LHS/RHS; nodesPerFace*nDim*nodesPerFace*nDim and nodesPerFace*nDim
  std::vector<double> lhs;
  std::vector<double> rhs;
  std::vector<int> scratchIds;
  std::vector<double> scratchVals;
  std::vector<stk::mesh::Entity> connected_nodes;

  // nodal fields to gather
  std::vector<double> ws_scalarQ;

  // master element
  std::vector<double> ws_face_shape_function;

  // define some common selectors
  stk::mesh::Selector s_locally_owned_union = meta_data.locally_owned_part()
    &stk::mesh::selectUnion(partVec_);

  stk::mesh::BucketVector const& face_buckets =
    realm_.get_buckets( meta_data.side_rank(), s_locally_owned_union );
  for ( stk::mesh::BucketVector::const_iterator ib = face_buckets.begin();
        ib != face_buckets.end() ; ++ib ) {
    stk::mesh::Bucket & b = **ib ;

    // face master element
    MasterElement *meFC = sierra::nalu::MasterElementRepo::get_surface_master_element(b.topology());
    const int nodesPerFace = meFC->nodesPerElement_;
    const int numScsBip = meFC->numIntPoints_;
    const int *faceIpNodeMap = meFC->ipNodeMap();

    // resize some things; matrix related
    const int lhsSize = nodesPerFace*nDim*nodesPerFace*nDim;
    const int rhsSize = nodesPerFace*nDim;
    lhs.resize(lhsSize);
    rhs.resize(rhsSize);
    scratchIds.resize(rhsSize);
    scratchVals.resize(rhsSize);
    connected_nodes.resize(nodesPerFace);

    // algorithm related; element
    ws_scalarQ.resize(nodesPerFace);
    ws_face_shape_function.resize(numScsBip*nodesPerFace);
  
    // pointers
    double *p_lhs = &lhs[0];
    double *p_rhs = &rhs[0];
    double *p_scalarQ = &ws_scalarQ[0];
    double *p_face_shape_function = &ws_face_shape_function[0];

    // zero lhs; always zero
    for ( int p = 0; p < lhsSize; ++p )
      p_lhs[p] = 0.0;
  
    // shape function
    meFC->shape_fcn(&p_face_shape_function[0]);

    const stk::mesh::Bucket::size_type length   = b.size();

    for ( stk::mesh::Bucket::size_type k = 0 ; k < length ; ++k ) {

      // zero rhs only since LHS never contributes, never touched
      for ( int p = 0; p < rhsSize; ++p )
        p_rhs[p] = 0.0;

      //======================================
      // gather nodal data off of face
      //======================================
      stk::mesh::Entity const * face_node_rels = b.begin_nodes(k);
      int num_face_nodes = b.num_nodes(k);
      // sanity check on num nodes
      STK_ThrowAssert( num_face_nodes == nodesPerFace );
      for ( int ni = 0; ni < num_face_nodes; ++ni ) {
        // get the node and form connected_node
        stk::mesh::Entity node = face_node_rels[ni];
        connected_nodes[ni] = node;
        // gather scalars
        p_scalarQ[ni] = *stk::mesh::field_data(*scalarQ_, node);
      }

      // pointer to face data
      const double * areaVec = stk::mesh::field_data(*exposedAreaVec_, b, k);
      const double * dynamicP = stk::mesh::field_data(*dynamicP_, b, k);

      // start the assembly
      for ( int ip = 0; ip < numScsBip; ++ip ) {
        
        // nearest node to ip
        const int localFaceNode = faceIpNodeMap[ip];

        // save off some offsets for this ip
        const int nnNdim = localFaceNode*nDim;
        const int offSetSF_face = ip*nodesPerFace;

        // interpolate to bip
        double scalarQBip = -dynamicP[ip];
        for ( int ic = 0; ic < nodesPerFace; ++ic ) {
          const double r = p_face_shape_function[offSetSF_face+ic];
          scalarQBip += r*p_scalarQ[ic];
        }

        // assemble to RHS; rhs -= a negative contribution => +=
        for ( int i = 0; i < nDim; ++i ) {
          p_rhs[nnNdim+i] += scalarQBip*areaVec[ip*nDim+i];
        }
      }
      
      apply_coeff(connected_nodes, scratchIds, scratchVals, rhs, lhs, __FILE__);

    }
  }
}

} // namespace nalu
} // namespace Sierra
