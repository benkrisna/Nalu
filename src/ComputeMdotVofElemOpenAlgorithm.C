/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


// nalu
#include "ComputeMdotVofElemOpenAlgorithm.h"
#include "Algorithm.h"

#include "FieldTypeDef.h"
#include "Realm.h"
#include "SolutionOptions.h"
#include "master_element/MasterElement.h"

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
// ComputeMdotVofElemOpenAlgorithm - bf mdot continuity open bc
//==========================================================================
//--------------------------------------------------------------------------
//-------- constructor -----------------------------------------------------
//--------------------------------------------------------------------------
ComputeMdotVofElemOpenAlgorithm::ComputeMdotVofElemOpenAlgorithm(
  Realm &realm,
  stk::mesh::Part *part,
  const SolutionOptions &solnOpts)
  : Algorithm(realm, part),
    velocityRTM_(NULL),
    Gpdx_(NULL),
    coordinates_(NULL),
    pressure_(NULL),
    density_(NULL),
    interfaceCurvature_(NULL),
    surfaceTension_(NULL),
    vof_(NULL),
    exposedAreaVec_(NULL),
    dynamicPressure_(NULL),
    pressureBc_(NULL),
    shiftMdot_(solnOpts.cvfemShiftMdot_),
    shiftedGradOp_(solnOpts.get_shifted_grad_op("pressure")),
    penaltyFac_(2.0),
    buoyancyWeight_(solnOpts.buoyancyPressureStab_ ? 1.0 : 0.0),
    n_(realm_.solutionOptions_->localVofN_),
    m_(realm_.solutionOptions_->localVofM_),
    c_(realm_.solutionOptions_->localVofC_)
{
  // save off fields
  stk::mesh::MetaData & meta_data = realm_.meta_data();
  if ( solnOpts.does_mesh_move() )
    velocityRTM_ = meta_data.get_field<double>(stk::topology::NODE_RANK, "velocity_rtm");
  else
    velocityRTM_ = meta_data.get_field<double>(stk::topology::NODE_RANK, "velocity");
  Gpdx_ = meta_data.get_field<double>(stk::topology::NODE_RANK, "dpdx");
  coordinates_ = meta_data.get_field<double>(stk::topology::NODE_RANK, realm_.get_coordinates_name());
  pressure_ = meta_data.get_field<double>(stk::topology::NODE_RANK, "pressure");
  density_ = meta_data.get_field<double>(stk::topology::NODE_RANK, "density");
  interfaceCurvature_ = meta_data.get_field<double>(stk::topology::NODE_RANK, "interface_curvature");
  surfaceTension_ = meta_data.get_field<double>(stk::topology::NODE_RANK, "surface_tension");
  vof_ = meta_data.get_field<double>(stk::topology::NODE_RANK, "volume_of_fluid");
  exposedAreaVec_ = meta_data.get_field<double>(meta_data.side_rank(), "exposed_area_vector");
  dynamicPressure_ = meta_data.get_field<double>(meta_data.side_rank(), "dynamic_pressure");
  openMassFlowRate_ = meta_data.get_field<double>(meta_data.side_rank(), "open_mass_flow_rate");
  openVolumeFlowRate_ = meta_data.get_field<double>(meta_data.side_rank(), "open_volume_flow_rate");
  pressureBc_ = meta_data.get_field<double>(stk::topology::NODE_RANK, "pressure_bc");
  gravity_ = realm_.solutionOptions_->gravity_;
}

//--------------------------------------------------------------------------
//-------- destructor ------------------------------------------------------
//--------------------------------------------------------------------------
ComputeMdotVofElemOpenAlgorithm::~ComputeMdotVofElemOpenAlgorithm()
{
  // does nothing
}

//--------------------------------------------------------------------------
//-------- execute ---------------------------------------------------------
//--------------------------------------------------------------------------
void
ComputeMdotVofElemOpenAlgorithm::execute()
{

  stk::mesh::BulkData & bulk_data = realm_.bulk_data();
  stk::mesh::MetaData & meta_data = realm_.meta_data();

  const int nDim = meta_data.spatial_dimension();
  
  // ip values; both boundary and opposing surface
  std::vector<double> uBip(nDim);
  std::vector<double> GpdxBip(nDim);
  std::vector<double> dpdxBip(nDim);

  // pointers to fixed values
  double *p_uBip = &uBip[0];
  double *p_GpdxBip = &GpdxBip[0];
  double *p_dpdxBip = &dpdxBip[0];

  // nodal fields to gather
  std::vector<double> ws_coordinates;
  std::vector<double> ws_face_pressure;
  std::vector<double> ws_pressure;
  std::vector<double> ws_vrtm;
  std::vector<double> ws_Gpdx;
  std::vector<double> ws_density;
  std::vector<double> ws_kappa;
  std::vector<double> ws_sigma;
  std::vector<double> ws_vof;
  std::vector<double> ws_bcPressure;

  // master element
  std::vector<double> ws_face_shape_function;
  std::vector<double> ws_dndx; 
  std::vector<double> ws_det_j;

  // projection time scale based on time step
  const double dt = realm_.get_time_step();
  const double gamma1 = realm_.get_gamma1();
  const double projTimeScale = dt/gamma1;

  // set accumulation variables
  double mdotOpen = 0.0;

  // deal with state
  ScalarFieldType &densityNp1 = density_->field_of_state(stk::mesh::StateNP1);

  // define vector of parent topos; should always be UNITY in size
  std::vector<stk::topology> parentTopo;

  // define some common selectors
  stk::mesh::Selector s_locally_owned_union = meta_data.locally_owned_part()
    &stk::mesh::selectUnion(partVec_);

  stk::mesh::BucketVector const& face_buckets =
    realm_.get_buckets( meta_data.side_rank(), s_locally_owned_union );
  for ( stk::mesh::BucketVector::const_iterator ib = face_buckets.begin();
        ib != face_buckets.end() ; ++ib ) {
    stk::mesh::Bucket & b = **ib ;

    // extract connected element topology
    b.parent_topology(stk::topology::ELEMENT_RANK, parentTopo);
    STK_ThrowAssert ( parentTopo.size() == 1 );
    stk::topology theElemTopo = parentTopo[0];

    // volume master element
    MasterElement *meSCS = sierra::nalu::MasterElementRepo::get_surface_master_element(theElemTopo);
    const int nodesPerElement = meSCS->nodesPerElement_;
   
    // face master element
    MasterElement *meFC = sierra::nalu::MasterElementRepo::get_surface_master_element(b.topology());
    const int nodesPerFace = b.topology().num_nodes();
    const int numScsBip = meFC->numIntPoints_;

    // algorithm related; element (exposed face and element)
    ws_coordinates.resize(nodesPerElement*nDim);
    ws_face_pressure.resize(nodesPerFace);
    ws_pressure.resize(nodesPerElement);
    ws_vrtm.resize(nodesPerFace*nDim);
    ws_Gpdx.resize(nodesPerFace*nDim);
    ws_density.resize(nodesPerFace);
    ws_kappa.resize(nodesPerFace);
    ws_sigma.resize(nodesPerFace);
    ws_vof.resize(nodesPerElement);
    ws_bcPressure.resize(nodesPerFace);
    ws_face_shape_function.resize(numScsBip*nodesPerFace);
    ws_dndx.resize(nDim*numScsBip*nodesPerElement);
    ws_det_j.resize(numScsBip);    

    // pointers
    double *p_coordinates = &ws_coordinates[0];
    double *p_face_pressure = &ws_face_pressure[0];
    double *p_pressure = &ws_pressure[0];
    double *p_vrtm = &ws_vrtm[0];
    double *p_Gpdx = &ws_Gpdx[0];
    double *p_density = &ws_density[0];
    double *p_kappa = &ws_kappa[0];
    double *p_sigma = &ws_sigma[0];
    double *p_vof = &ws_vof[0];
    double *p_bcPressure = &ws_bcPressure[0];
    double *p_face_shape_function = &ws_face_shape_function[0];
    double *p_dndx = &ws_dndx[0];

    // shape functions; boundary
    if ( shiftMdot_ )
      meFC->shifted_shape_fcn(&p_face_shape_function[0]);
    else
      meFC->shape_fcn(&p_face_shape_function[0]);
    
    const stk::mesh::Bucket::size_type length   = b.size();

    for ( stk::mesh::Bucket::size_type k = 0 ; k < length ; ++k ) {

      // get face
      stk::mesh::Entity face = b[k];

      //======================================
      // gather nodal data off of face
      //======================================
      stk::mesh::Entity const * face_node_rels = bulk_data.begin_nodes(face);
      int num_face_nodes = bulk_data.num_nodes(face);
      // sanity check on num nodes
      STK_ThrowAssert( num_face_nodes == nodesPerFace );
      for ( int ni = 0; ni < num_face_nodes; ++ni ) {
        stk::mesh::Entity node = face_node_rels[ni];

        // gather scalars
        p_face_pressure[ni]    = *stk::mesh::field_data(*pressure_, node);
        p_bcPressure[ni] = *stk::mesh::field_data(*pressureBc_, node);
        p_density[ni]    = *stk::mesh::field_data(densityNp1, node);
        p_kappa[ni]  = *stk::mesh::field_data(*interfaceCurvature_, node);
        p_sigma[ni]  = *stk::mesh::field_data(*surfaceTension_, node);
        
        // gather vectors
        double * vrtm = stk::mesh::field_data(*velocityRTM_, node);
        double * Gjp = stk::mesh::field_data(*Gpdx_, node);
        const int offSet = ni*nDim;
        for ( int j=0; j < nDim; ++j ) {
          p_vrtm[offSet+j] = vrtm[j];
          p_Gpdx[offSet+j] = Gjp[j];
        }
      }

      // pointer to face data
      const double * areaVec = stk::mesh::field_data(*exposedAreaVec_, face);
      const double * dynamicP = stk::mesh::field_data(*dynamicPressure_, b, k);
      double * mdot = stk::mesh::field_data(*openMassFlowRate_, face);
      double * vdot = stk::mesh::field_data(*openVolumeFlowRate_, face);

      // extract the connected element to this exposed face; should be single in size!
      const stk::mesh::Entity* face_elem_rels = bulk_data.begin_elements(face);
      STK_ThrowAssert( bulk_data.num_elements(face) == 1 );

      // get element; its face ordinal number and populate face_node_ordinals
      stk::mesh::Entity element = face_elem_rels[0];
      const int face_ordinal = bulk_data.begin_element_ordinals(face)[0];
      const int *face_node_ordinals = meSCS->side_node_ordinals(face_ordinal);

      //======================================
      // gather nodal data off of element
      //======================================
      stk::mesh::Entity const * elem_node_rels = bulk_data.begin_nodes(element);
      int num_nodes = bulk_data.num_nodes(element);
      // sanity check on num nodes
      STK_ThrowAssert( num_nodes == nodesPerElement );
      for ( int ni = 0; ni < num_nodes; ++ni ) {
        stk::mesh::Entity node = elem_node_rels[ni];

        // gather scalars
        p_pressure[ni] = *stk::mesh::field_data(*pressure_, node);
        p_vof[ni]  = *stk::mesh::field_data(*vof_, node);
        
        // gather vectors
        double * coords = stk::mesh::field_data(*coordinates_, node);
        const int offSet = ni*nDim;
        for ( int j=0; j < nDim; ++j ) {
          p_coordinates[offSet+j] = coords[j];
        }
      }

      // compute dndx
      double scs_error = 0.0;
      if ( shiftedGradOp_ )
        meSCS->shifted_face_grad_op(1, face_ordinal, &p_coordinates[0], &p_dndx[0], &ws_det_j[0], &scs_error);
      else
        meSCS->face_grad_op(1, face_ordinal, &p_coordinates[0], &p_dndx[0], &ws_det_j[0], &scs_error);

      // loop over boundary ips
      for ( int ip = 0; ip < numScsBip; ++ip ) {

        // zero out vector quantities; form aMag
        double aMag = 0.0;
        for ( int j = 0; j < nDim; ++j ) {
          p_uBip[j] = 0.0;
          p_GpdxBip[j] = 0.0;
          p_dpdxBip[j] = 0.0;
          const double axj = areaVec[ip*nDim+j];
          aMag += axj*axj;
        }
        aMag = std::sqrt(aMag);

        // form L^-1
        double inverseLengthScale = 0.0;
        for ( int ic = 0; ic < nodesPerFace; ++ic ) {
          const int faceNodeNumber = face_node_ordinals[ic];
          const int offSetDnDx = nDim*nodesPerElement*ip + faceNodeNumber*nDim;
          for ( int j = 0; j < nDim; ++j ) {
            inverseLengthScale += p_dndx[offSetDnDx+j]*areaVec[ip*nDim+j];
          }
        }        
        inverseLengthScale /= aMag;

        // interpolate to bip
        double pBip = 0.0;
        double pbcBip = -dynamicP[ip];
        double rhoBip = 0.0;
        double sigmaKappaBip = 0.0;
        double vofBip = 0.0;
        const int offSetSF_face = ip*nodesPerFace;
        for ( int ic = 0; ic < nodesPerFace; ++ic ) {
          const double r = p_face_shape_function[offSetSF_face+ic];
          pBip += r*p_face_pressure[ic];
          pbcBip += r*p_bcPressure[ic];
          rhoBip += r*p_density[ic];
          sigmaKappaBip += r*p_sigma[ic]*p_kappa[ic];
          vofBip += r*p_vof[ic];
          const int icNdim = ic*nDim;
          for ( int j = 0; j < nDim; ++j ) {
            p_uBip[j] += r*p_vrtm[icNdim+j];
            p_GpdxBip[j] += r*p_Gpdx[icNdim+j];
          }
        }

        // form dpdxBip and dvofdaBip
        double dvofdaBip = 0.0;
        for ( int ic = 0; ic < nodesPerElement; ++ic ) {
          const int offSetDnDx = nDim*nodesPerElement*ip + ic*nDim;
          const double pIc = p_pressure[ic];
          const double vofIc = p_vof[ic];
          for ( int j = 0; j < nDim; ++j ) {
            const double dxj = p_dndx[offSetDnDx+j];
            p_dpdxBip[j] += dxj*pIc;
            dvofdaBip += dxj*vofIc*areaVec[ip*nDim+j];
          }
        }
     
        // correct for localized approach
        sigmaKappaBip *= c_*std::pow(vofBip,n_)*std::pow(1.0-vofBip,m_);
        
        // form vdot; uj*Aj - projTS*(dpdxj/rhoBip - Gjp)*Aj + penaltyFac_/rhoBip*projTS*invL*(pBip - pbcBip)*aMag + BF
        double tvdot = penaltyFac_*projTimeScale/rhoBip*inverseLengthScale*(pBip - pbcBip)*aMag
          + projTimeScale*sigmaKappaBip*dvofdaBip/rhoBip;
        for ( int j = 0; j < nDim; ++j ) {
          const double axj = areaVec[ip*nDim+j];
          tvdot += (p_uBip[j] - projTimeScale*((p_dpdxBip[j]-buoyancyWeight_*rhoBip*gravity_[j])/rhoBip - p_GpdxBip[j]))*axj;
        }
        
        // scatter to vdot and mdot; accumulate
        vdot[ip] = tvdot;
        mdot[ip] = rhoBip*tvdot;
        mdotOpen += rhoBip*tvdot;
      }
    }
  }
  // scatter back to solution options; not thread safe
  realm_.solutionOptions_->mdotAlgOpen_ += mdotOpen;
}

} // namespace nalu
} // namespace Sierra
