/*------------------------------------------------------------------------*/
/*  Copyright 2014 Sandia Corporation.                                    */
/*  This software is released under the license detailed                  */
/*  in the file, LICENSE, which is located in the top-level Nalu          */
/*  directory structure                                                   */
/*------------------------------------------------------------------------*/


#include <SolutionOptions.h>
#include <Enums.h>
#include <NaluEnv.h>
#include <MeshMotionInfo.h>
#include <FixPressureAtNodeInfo.h>

// basic c++
#include <stdexcept>
#include <utility>

namespace sierra{
namespace nalu{


//==========================================================================
// Class Definition
//==========================================================================
// SolutionOptions - holder for user options at the realm scope
//==========================================================================
//--------------------------------------------------------------------------
//-------- constructor -----------------------------------------------------
//--------------------------------------------------------------------------
SolutionOptions::SolutionOptions()
  : hybridDefault_(0.0),
    alphaDefault_(0.0),
    alphaUpwDefault_(1.0),
    kappaMusclDefault_(-1.0),
    upwDefault_(1.0),
    lamScDefault_(1.0),
    turbScDefault_(1.0),
    turbPrDefault_(1.0),
    musclDefault_(false),
    nocDefault_(true),
    shiftedGradOpDefault_(false),
    skewSymmetricDefault_(false),
    tanhFormDefault_("classic"),
    tanhTransDefault_(2.0),
    tanhWidthDefault_(4.0),
    referenceDensity_(0.0),
    referenceTemperature_(298.0),
    thermalExpansionCoeff_(1.0),
    stefanBoltzmann_(5.6704e-8),
    includeDivU_(0.0),
    isTurbulent_(false),
    turbulenceModel_(LAMINAR),
    dynamicTurbulenceProcedure_(false),
    dynamicTurbulenceClippingFac_(0.0),
    meshMotion_(false),
    meshMotionIncludesSixDof_(false),
    meshDeformation_(false),
    externalMeshDeformation_(false),
    initialMeshDisplacement_(false),
    errorIndicatorActive_(false),
    errorIndicatorType_(EIT_NONE),
    errorIndicatorFrequency_(100),
    ncAlgGaussLabatto_(true),
    ncAlgUpwindAdvection_(true),
    ncAlgIncludePstab_(true),
    ncAlgDetailedOutput_(false),
    ncAlgCoincidentNodesErrorCheck_(false),
    ncAlgCurrentNormal_(false),
    ncAlgPngPenalty_(true),
    cvfemShiftMdot_(false),
    cvfemReducedSensPoisson_(false),
    inputVariablesRestorationTime_(1.0e8),
    inputVariablesInterpolateInTime_(false),
    inputVariablesPeriodicTime_(0.0),
    consistentMMPngDefault_(false),
    useConsolidatedSolverAlg_(false),
    useConsolidatedBcSolverAlg_(false),
    eigenvaluePerturb_(false),
    eigenvaluePerturbDelta_(0.0),
    eigenvaluePerturbBiasTowards_(3),
    eigenvaluePerturbTurbKe_(0.0),
    localVofJm_(0.003),
    localVofM_(0.0),
    localVofN_(0.0),
    localVofC_(1.0),
    vofCalpha_(0.05),
    vofDensityPhaseOne_(1.0),
    vofDensityPhaseTwo_(1.2e-3),
    mdotAlgAccumulation_(0.0),
    mdotAlgInflow_(0.0),
    mdotAlgOpen_(0.0),
    scaleGeometricProps_(false),
    quadType_("GaussLegendre"),
    accousticallyCompressible_(false),
    balancedForce_(false),
    volumetricSrc_(0.0),
    wettedWallInit_(false),
    wettedWallDistance_(-1.0)
{
  // nothing to do
}

//--------------------------------------------------------------------------
//-------- destructor ------------------------------------------------------
//--------------------------------------------------------------------------
SolutionOptions::~SolutionOptions()
{
  std::map<std::string, MeshMotionInfo *>::iterator it;
  for ( it = meshMotionInfoMap_.begin(); it!= meshMotionInfoMap_.end(); ++it ) {
    MeshMotionInfo *info = it->second;
    delete info;
  }
  for ( it = initialMeshDisplacementInfoMap_.begin(); it!= initialMeshDisplacementInfoMap_.end(); ++it ) {
    MeshMotionInfo *info = it->second;
    delete info;
  }
}

//--------------------------------------------------------------------------
//-------- load ------------------------------------------------------------
//--------------------------------------------------------------------------
void
SolutionOptions::load(const YAML::Node & y_node)
{
  const bool optional=true;

  const YAML::Node y_solution_options = expect_map(y_node,"solution_options", optional);
  if(y_solution_options)
  {
    get_required(y_solution_options, "name", name_);
    
    // divU factor for stress
    get_if_present(y_solution_options, "divU_stress_scaling", includeDivU_, includeDivU_);
    
    // external mesh motion expected
    get_if_present(y_solution_options, "externally_provided_mesh_deformation", externalMeshDeformation_, externalMeshDeformation_);

    // shift mdot for continuity (CVFEM)
    get_if_present(y_solution_options, "shift_cvfem_mdot", cvfemShiftMdot_, cvfemShiftMdot_);

    // DEPRECATED shifted CVFEM pressure poisson;
    if ( y_solution_options["shift_cvfem_poisson"] ) {
      throw std::runtime_error("shift_cvfem_poisson line command is deprecated to the generalized solution options block command, shifted_gradient_operator");
    }

    // Reduce sensitivities for CVFEM PPE: M delta_p = b - A p; M has quadrature at edge midpoints of the element
    get_if_present(y_solution_options, "reduced_sens_cvfem_poisson", cvfemReducedSensPoisson_, cvfemReducedSensPoisson_);

    // check for consolidated solver alg (AssembleSolver)
    get_if_present(y_solution_options, "use_consolidated_solver_algorithm", useConsolidatedSolverAlg_, useConsolidatedSolverAlg_);

    // check for consolidated face-elem bc alg
    get_if_present(y_solution_options, "use_consolidated_face_elem_bc_algorithm", useConsolidatedBcSolverAlg_, useConsolidatedBcSolverAlg_);

    // eigenvalue purturbation; over all dofs...
    get_if_present(y_solution_options, "eigenvalue_perturbation", eigenvaluePerturb_);
    get_if_present(y_solution_options, "eigenvalue_perturbation_delta", eigenvaluePerturbDelta_);
    get_if_present(y_solution_options, "eigenvalue_perturbation_bias_towards", eigenvaluePerturbBiasTowards_);
    get_if_present(y_solution_options, "eigenvalue_perturbation_turbulent_ke", eigenvaluePerturbTurbKe_);
    
    // evaporation and localized VOF
    get_if_present(y_solution_options, "local_vof_jm", localVofJm_, localVofJm_);
    get_if_present(y_solution_options, "local_vof_m", localVofM_, localVofM_);
    get_if_present(y_solution_options, "local_vof_n", localVofN_, localVofN_);
    get_if_present(y_solution_options, "local_vof_c", localVofC_, localVofC_);

    // quadrature type for high order
    get_if_present(y_solution_options, "high_order_quadrature_type", quadType_);

    // accoustically compressible algorith
    get_if_present(y_solution_options, "use_accoustically_compressible_algorithm", accousticallyCompressible_);

    // balanced-force area and inverse density scaling
    get_if_present_no_default(y_solution_options, "activate_balanced_force_algorithm", balancedForce_);

    // Buoyancy pressure stabilization 
    if(balancedForce_)
      get_if_present(y_solution_options, "activate_buoyancy_pressure_stabilization", buoyancyPressureStab_, true);
    else
      get_if_present(y_solution_options, "activate_buoyancy_pressure_stabilization", buoyancyPressureStab_, false);

    // volumetric source term
    get_if_present(y_solution_options, "volumetric_source", volumetricSrc_, volumetricSrc_);
          
    // wetted wall active
    get_if_present(y_solution_options, "activate_wetted_wall_initialization", wettedWallInit_, wettedWallInit_);
    get_if_present(y_solution_options, "wetted_wall_initialization_distance", wettedWallDistance_, wettedWallDistance_);

    // scale inverse props
    get_if_present(y_solution_options, "scale_geometric_properties", scaleGeometricProps_, scaleGeometricProps_);

    // extract turbulence model; would be nice if we could parse an enum..
    std::string specifiedTurbModel;
    std::string defaultTurbModel = "laminar";
    get_if_present(y_solution_options,
        "turbulence_model", specifiedTurbModel, defaultTurbModel);

    bool matchedTurbulenceModel = false;
    for ( int k=0; k < TurbulenceModel_END; ++k ) {
      if (case_insensitive_compare(specifiedTurbModel, TurbulenceModelNames[k])) {
        turbulenceModel_ = TurbulenceModel(k);
        matchedTurbulenceModel = true;
        break;
      }
    }

    if (!matchedTurbulenceModel) {
      std::string msg = "Turbulence model `" + specifiedTurbModel +
          "' not implemented.\n  Available turbulence models are ";

     for ( int k=0; k < TurbulenceModel_END; ++k ) {
       msg += "`" + TurbulenceModelNames[k] + "'";
       if ( k != TurbulenceModel_END-1) { msg += ", "; }
     }
      throw std::runtime_error(msg);
    }
    if ( turbulenceModel_ != LAMINAR ) {
      isTurbulent_ = true;
    }

    // initialize turbulence constants since some laminar models may need such variables, e.g., kappa
    initialize_turbulence_constants();

    // check for dynamic procedure; error checks
    get_if_present(y_solution_options,
                   "activate_dynamic_turbulence_procedure", dynamicTurbulenceProcedure_, dynamicTurbulenceProcedure_);
    if ( dynamicTurbulenceProcedure_ ) {
      if ( turbulenceModel_ != KSGS && turbulenceModel_ != LRKSGS ) {
        throw std::runtime_error("Error: Dynamic procedure is only implemented for the kSGS family of turbulence models");
      }
    }

    get_if_present(y_solution_options,
                   "dynamic_turbulence_clipping_factor", dynamicTurbulenceClippingFac_, dynamicTurbulenceClippingFac_);

    // extract possible copy from input fields restoration time
    get_if_present(y_solution_options, "input_variables_from_file_restoration_time",
      inputVariablesRestorationTime_, inputVariablesRestorationTime_);

    // choice of interpolation or snapping to closest in the data base
    get_if_present(y_solution_options, "input_variables_interpolate_in_time",
      inputVariablesInterpolateInTime_, inputVariablesInterpolateInTime_);

    // allow for periodic sampling in time
    get_if_present(y_solution_options, "input_variables_from_file_periodic_time",
      inputVariablesPeriodicTime_, inputVariablesPeriodicTime_);

    // first set of options; hybrid, source, etc.
    const YAML::Node y_options = expect_sequence(y_solution_options, "options", optional);
    if (y_options) {
      for (size_t ioption = 0; ioption < y_options.size(); ++ioption)
      {
        const YAML::Node y_option = y_options[ioption] ;
        if (expect_map(y_option, "hybrid_factor", optional)) {
          y_option["hybrid_factor"] >> hybridMap_ ;
        }
        else if (expect_map(y_option, "alpha", optional)) {
          y_option["alpha"] >> alphaMap_ ;
        }
        else if (expect_map(y_option, "alpha_upw", optional)) {
          y_option["alpha_upw"] >> alphaUpwMap_;
        }
        else if (expect_map(y_option, "upw_factor", optional)) {
          y_option["upw_factor"] >> upwMap_ ;
        }
        else if (expect_map(y_option, "muscl", optional)) {
          y_option["muscl"] >> useMusclMap_;
        }
        else if (expect_map(y_option, "kappa_muscl", optional)) {
          y_option["kappa_muscl"] >>  kappaMusclMap_;
        }
        else if (expect_map(y_option, "limiter", optional)) {
          y_option["limiter"] >> limiterMap_ ;
        }
        else if (expect_map(y_option, "limiter_type", optional)) {
          y_option["limiter_type"] >> limiterTypeMap_ ;
        }
        else if (expect_map( y_option, "laminar_schmidt", optional)) {
          y_option["laminar_schmidt"] >> lamScMap_ ;
        }
        else if (expect_map( y_option, "laminar_prandtl", optional)) {
          y_option["laminar_prandtl"] >> lamPrMap_ ;
        }
        else if (expect_map( y_option, "turbulent_schmidt", optional)) {
          y_option["turbulent_schmidt"] >> turbScMap_ ;
        }
        else if (expect_map( y_option, "turbulent_prandtl", optional)) {
          y_option["turbulent_prandtl"] >> turbPrMap_ ;
        }
        else if (expect_map( y_option, "source_terms", optional)) {
          const YAML::Node ySrc = y_option["source_terms"];
          ySrc >> srcTermsMap_ ;
        }
        else if (expect_map( y_option, "element_source_terms", optional)) {
          const YAML::Node ySrc = y_option["element_source_terms"];
          ySrc >> elemSrcTermsMap_ ;
        }
        else if (expect_map( y_option, "source_term_parameters", optional)) {
          y_option["source_term_parameters"] >> srcTermParamMap_ ;
        }
        else if (expect_map( y_option, "element_source_term_parameters", optional)) {
          y_option["element_source_term_parameters"] >> elemSrcTermParamMap_ ;
        }
        else if (expect_map( y_option, "projected_nodal_gradient", optional)) {
          y_option["projected_nodal_gradient"] >> nodalGradMap_ ;
        }
        else if (expect_map( y_option, "noc_correction", optional)) {
          y_option["noc_correction"] >> nocMap_ ;
        }
        else if (expect_map( y_option, "shifted_gradient_operator", optional)) {
          y_option["shifted_gradient_operator"] >> shiftedGradOpMap_ ;
        }
        else if (expect_map( y_option, "skew_symmetric_advection", optional)) {
          y_option["skew_symmetric_advection"] >> skewSymmetricMap_;
        }
        else if (expect_map( y_option, "input_variables_from_file", optional)) {
          y_option["input_variables_from_file"] >> inputVarFromFileMap_ ;
        }
        else if (expect_map( y_option, "turbulence_model_constants", optional)) {
          std::map<std::string, double> turbConstMap;
          y_option["turbulence_model_constants"] >> turbConstMap ;
          // iterate the parsed map
          std::map<std::string, double>::iterator it;
          for ( it = turbConstMap.begin(); it!= turbConstMap.end(); ++it ) {
            std::string theConstName = it->first;
            double theConstValue = it->second;
            // find the enum and set the value
            bool foundIt = false;
            for ( int k=0; k < TM_END; ++k ) {
              if ( theConstName == TurbulenceModelConstantNames[k] ) {
                TurbulenceModelConstant theConstEnum = TurbulenceModelConstant(k);
                turbModelConstantMap_[theConstEnum] = theConstValue;
                foundIt = true;
                break;
              }
            }
            // error check..
            if ( !foundIt ) {
              NaluEnv::self().naluOutputP0() << "Sorry, turbulence model constant with name " << theConstName << " was not found " << std::endl;
              NaluEnv::self().naluOutputP0() << "List of turbulence model constant names are as follows:" << std::endl;
              for ( int k=0; k < TM_END; ++k ) {
                NaluEnv::self().naluOutputP0() << TurbulenceModelConstantNames[k] << std::endl;
              }
            }
          }
        }
        else if (expect_map( y_option, "user_constants", optional)) {
          const YAML::Node y_user_constants = y_option["user_constants"];
          get_if_present(y_user_constants, "reference_density",  referenceDensity_, referenceDensity_);
          get_if_present(y_user_constants, "reference_temperature",  referenceTemperature_, referenceTemperature_);
          get_if_present(y_user_constants, "thermal_expansion_coefficient",  thermalExpansionCoeff_, thermalExpansionCoeff_);
          get_if_present(y_user_constants, "stefan_boltzmann",  stefanBoltzmann_, stefanBoltzmann_);
        
          if (expect_sequence( y_user_constants, "gravity", optional) ) {
            const int gravSize = y_user_constants["gravity"].size();
            for (int i = 0; i < gravSize; ++i ) {
              gravity_[i] = y_user_constants["gravity"][i].as<double>() ;
            }
          }
        }
        else if (expect_map( y_option, "non_conformal", optional)) {
          const YAML::Node y_nc = y_option["non_conformal"];
          get_if_present(y_nc, "gauss_labatto_quadrature",  ncAlgGaussLabatto_, ncAlgGaussLabatto_);
          get_if_present(y_nc, "upwind_advection",  ncAlgUpwindAdvection_, ncAlgUpwindAdvection_);
          get_if_present(y_nc, "include_pstab",  ncAlgIncludePstab_, ncAlgIncludePstab_);
          get_if_present(y_nc, "detailed_output",  ncAlgDetailedOutput_, ncAlgDetailedOutput_);
          get_if_present(y_nc, "activate_coincident_node_error_check",  ncAlgCoincidentNodesErrorCheck_, ncAlgCoincidentNodesErrorCheck_);
          get_if_present(y_nc, "current_normal",  ncAlgCurrentNormal_, ncAlgCurrentNormal_);
          get_if_present(y_nc, "include_png_penalty",  ncAlgPngPenalty_, ncAlgPngPenalty_);
        }
        else if (expect_map( y_option, "peclet_function_form", optional)) {
          y_option["peclet_function_form"] >> tanhFormMap_ ;
        }
        else if (expect_map( y_option, "peclet_function_tanh_transition", optional)) {
          y_option["peclet_function_tanh_transition"] >> tanhTransMap_ ;
        }
        else if (expect_map( y_option, "peclet_function_tanh_width", optional)) {
          y_option["peclet_function_tanh_width"] >> tanhWidthMap_ ;
        }
        // overload line command, however, push to the same tanh data structure
        else if (expect_map( y_option, "blending_function_form", optional)) {
          y_option["blending_function_form"] >> tanhFormMap_ ;
        }
        else if (expect_map( y_option, "tanh_transition", optional)) {
          y_option["tanh_transition"] >> tanhTransMap_ ;
        }
        else if (expect_map( y_option, "tanh_width", optional)) {
          y_option["tanh_width"] >> tanhWidthMap_ ;
        }
        else if (expect_map( y_option, "consistent_mass_matrix_png", optional)) {
          y_option["consistent_mass_matrix_png"] >> consistentMassMatrixPngMap_ ;
        }
        else {
          if (!NaluEnv::self().parallel_rank())
          {
            std::cout << "Error: parsing at " << NaluParsingHelper::info(y_option)
              //<< "... at parent ... " << NaluParsingHelper::info(y_node)
                      << std::endl;
          }
          throw std::runtime_error("unknown solution option: "+ NaluParsingHelper::info(y_option));
        }
      }
    }

    // second set of options: mesh motion... this means that the Realm will expect to provide rotation-based motion
    const YAML::Node y_mesh_motion = expect_sequence(y_solution_options, "mesh_motion", optional);
    if (y_mesh_motion) {
      // mesh motion is active
      meshMotion_ = true;
      
      // has a user stated that mesh motion is external?
      if ( meshDeformation_ ) {
        NaluEnv::self().naluOutputP0() << "mesh motion set to external (will prevail over mesh motion specification)!" << std::endl;
      }
      else {        
        for (size_t ioption = 0; ioption < y_mesh_motion.size(); ++ioption) {
          const YAML::Node &y_option = y_mesh_motion[ioption];

          // extract mesh motion name
          std::string motionName = "na";
          get_required(y_option, "name", motionName);
          double omega = 0.0;

          // Check for 6 DOF
          bool sixDOF = false;
          get_if_present(y_option, "include_six_dof", sixDOF, sixDOF);

          // set a flag for specialized six-dof mesh motion
          if ( sixDOF )
            meshMotionIncludesSixDof_ = true;

          // Assume straight rotation if no sixDOF included          
          if ( !sixDOF )
            get_required(y_option, "omega", omega);

          // now fill in name
          std::vector<std::string> meshMotionBlock;
          const YAML::Node &targets = y_option["target_name"];
          if (targets.Type() == YAML::NodeType::Scalar) {
            meshMotionBlock.resize(1);
            meshMotionBlock[0] = targets.as<std::string>() ;
          }
          else {
            meshMotionBlock.resize(targets.size());
            for (size_t i=0; i < targets.size(); ++i) {
              meshMotionBlock[i] = targets[i].as<std::string>() ;
            }
          }
          
          // look for centroid coordinates; optional, provide a default
          std::vector<double> cCoordsVec(3,0.0); 
          const YAML::Node coordsVecNode = y_option["centroid_coordinates"];
          if ( coordsVecNode ) {
            for ( size_t i = 0; i < coordsVecNode.size(); ++i )
              cCoordsVec[i] = coordsVecNode[i].as<double>();
          }
          
          // check for calculation of centroid
          bool computeCentroid = false;
          get_if_present(y_option, "compute_centroid", computeCentroid, computeCentroid);
          // user specified prevails
          if ( coordsVecNode && computeCentroid ) {
            NaluEnv::self().naluOutputP0() 
              << "centroid_coordinates and compute_centroid both active, user-specified centroid will prevail" << std::endl;
            computeCentroid = false;
          }


          // look for unit vector; provide default
          std::vector<double> unitVec(3,0.0); 
          if ( !sixDOF ) {
            const YAML::Node uV = y_option["unit_vector"];
            if ( uV ) {
              for ( size_t i = 0; i < uV.size(); ++i )
                unitVec[i] = uV[i].as<double>() ;
            }
            else {
              NaluEnv::self().naluOutputP0() << "SolutionOptions::load() unit_vector not supplied; will use 0,0,1" << std::endl;
              unitVec[2] = 1.0;
            }
          }

          if ( sixDOF ) {

            std::vector<std::string> forceSurface;
            std::vector<double> bodyAngle(3,0.0);
            std::vector<double> bodyOmega(3,0.0);
            std::vector<double> appliedForce(3,0.0);
            std::vector<double> bodyPrincInertia(3,0.0);
            std::vector<double> bodyVel(3,0.0);
            std::vector<double> bodyDispCC(3,0.0);

            std::vector<std::array<double,9>> tetherGeom;

            // Check for all 6-DOF related inputs
            const YAML::Node fS = y_option["forcing_surface"];
            if ( fS ) {
              for ( size_t i = 0; i < fS.size(); ++i )
                forceSurface.push_back(fS[i].as<std::string>()) ;
            }

            const YAML::Node bA = y_option["body_angle"];
            if ( bA ) {
              for ( size_t i = 0; i < bA.size(); ++i )
                bodyAngle[i] = bA[i].as<double>() ;
            }

            const YAML::Node bdCC = y_option["body_cc_disp"];
            if ( bdCC ) {
              for ( size_t i = 0; i < bdCC.size(); ++i )
                bodyDispCC[i] = bdCC[i].as<double>() ;
            }

            const YAML::Node bO = y_option["body_omega"];
            if ( bO ) {
              for ( size_t i = 0; i < bO.size(); ++i )
                bodyOmega[i] = bO[i].as<double>() ;
            }

            const YAML::Node bF = y_option["applied_force"];
            if ( bF ) {
              for ( size_t i = 0; i < bF.size(); ++i )
                appliedForce[i] = bF[i].as<double>() ; 
            }

            const YAML::Node bFI = y_option["principal_moments_inertia"];
            if ( bFI ) {
              for ( size_t i = 0; i < bFI.size(); ++i )
                bodyPrincInertia[i] = bFI[i].as<double>() ; 
            }

            const YAML::Node bV = y_option["body_velocity"];
            if ( bV ) {
              for ( size_t i = 0; i < bV.size(); ++i )
                bodyVel[i] = bV[i].as<double>() ;
            }

            double bodyMass = 0.0;
            get_if_present(y_option, "body_mass", bodyMass, bodyMass);

            double bodyDen = 0.0;
            get_if_present(y_option, "body_density", bodyDen, bodyDen);

            const YAML::Node bTether = y_option["tethers"];

            if ( bTether ) {
              if ( bTether.size() % 9 != 0 ) 
                throw std::runtime_error("Incorrect tether format! Format should be :: " \
                  " allow_slack (0 or 1), relaxed length, spring constant," \
                  " x1 relative to center of mass (RCOM), y1 RCOM, z1 RCOM, x2 fixed on domain (FOD)," \
                  " y2 FOD, z2 FOD :: For multiple tethers repeat same format for each tether.");
              
              size_t numTethers = bTether.size()/9;
              for ( size_t i = 0; i < numTethers; ++i ) {
                tetherGeom.emplace_back(std::array<double,9>{});
                auto &&currentTether = tetherGeom.back();
                for ( size_t tetherInfo = 0; tetherInfo < 9; ++tetherInfo )
                  currentTether[tetherInfo] = bTether[9*i+tetherInfo].as<double>();
              }
            }

            MeshMotionInfo *meshInfo = new MeshMotionInfo(meshMotionBlock, forceSurface, bodyDispCC, bodyAngle, bodyOmega, 
              bodyPrincInertia, cCoordsVec, bodyVel, bodyMass, bodyDen, appliedForce, computeCentroid, tetherGeom);

            // set the map
            meshMotionInfoMap_[motionName] = meshInfo;

          }
          else {

            MeshMotionInfo *meshInfo = new MeshMotionInfo(meshMotionBlock, omega, cCoordsVec, unitVec, computeCentroid);
            // set the map
            meshMotionInfoMap_[motionName] = meshInfo;

          }
        }
      }
    }
    
    // second set of options: initial mesh displacement 
    const YAML::Node y_initial_mesh_displacement = expect_sequence(y_solution_options, "initial_mesh_displacement", optional);
    if (y_initial_mesh_displacement) {
      // initial displacement is activated
      initialMeshDisplacement_ = true;
      
      for (size_t ioption = 0; ioption < y_initial_mesh_displacement.size(); ++ioption) {
	const YAML::Node &y_option = y_initial_mesh_displacement[ioption];
        
	// extract mesh motion name
	std::string motionName = "na";
	get_required(y_option, "name", motionName);

	// dummy omega
	const double omega = 0.0;
	
	// now fill in name
	std::vector<std::string> meshMotionBlock;
	const YAML::Node &targets = y_option["target_name"];
	if (targets.Type() == YAML::NodeType::Scalar) {
	  meshMotionBlock.resize(1);
	  meshMotionBlock[0] = targets.as<std::string>() ;
	}
	else {
	  meshMotionBlock.resize(targets.size());
	  for (size_t i=0; i < targets.size(); ++i) {
	    meshMotionBlock[i] = targets[i].as<std::string>() ;
	  }
	}
        
	// look for centroid coordinates; optional, provide a default
	std::vector<double> cCoordsVec(3,0.0); 
	const YAML::Node coordsVecNode = y_option["centroid_coordinates"];
	if ( coordsVecNode ) {
	  for ( size_t i = 0; i < coordsVecNode.size(); ++i )
	    cCoordsVec[i] = coordsVecNode[i].as<double>();
	}
	
	// check for calculation of centroid
	bool computeCentroid = false;
	get_if_present(y_option, "compute_centroid", computeCentroid, computeCentroid);
	// user specified prevails
	if ( coordsVecNode && computeCentroid ) {
	  NaluEnv::self().naluOutputP0() 
	    << "centroid_coordinates and compute_centroid both active, user-specified centroid will prevail" << std::endl;
	  computeCentroid = false;
	}
	
	// look for unit vector; provide default
	std::vector<double> unitVec(3,0.0); 
	const YAML::Node uV = y_option["unit_vector"];
	if ( uV ) {
	  for ( size_t i = 0; i < uV.size(); ++i )
	    unitVec[i] = uV[i].as<double>() ;
	}
	else {
	  NaluEnv::self().naluOutputP0() << "SolutionOptions::load() unit_vector not supplied; will use 0,0,1" << std::endl;
	  unitVec[2] = 1.0;
	}
	
	// extract angle
	double theAngle = 0.0;
	get_if_present(y_option, "angle", theAngle, theAngle);
        
	MeshMotionInfo *meshInfo = new MeshMotionInfo(meshMotionBlock, omega, cCoordsVec, unitVec, computeCentroid, theAngle);
	// set the map
	initialMeshDisplacementInfoMap_[motionName] = meshInfo;
      }
    }
  
    const YAML::Node fix_pressure = expect_map(y_solution_options, "fix_pressure_at_node", optional);
    if (fix_pressure) {
      needPressureReference_ = true;
      fixPressureInfo_.reset(new FixPressureAtNodeInfo);

      fixPressureInfo_->refPressure_ = fix_pressure["value"].as<double>();
      if (fix_pressure["node_lookup_type"])
      {
        std::string lookupTypeName = fix_pressure["node_lookup_type"].as<std::string>();
        if (lookupTypeName == "stk_node_id")
          fixPressureInfo_->lookupType_ = FixPressureAtNodeInfo::STK_NODE_ID;
        else if (lookupTypeName == "spatial_location")
          fixPressureInfo_->lookupType_ = FixPressureAtNodeInfo::SPATIAL_LOCATION;
        else
          throw std::runtime_error("FixPressureAtNodeInfo: Invalid option specified for "
                                   "'node_lookup_type' in input file.");
      }

      if (fixPressureInfo_->lookupType_ == FixPressureAtNodeInfo::SPATIAL_LOCATION) {
        fixPressureInfo_->location_ = fix_pressure["location"].as<std::vector<double>>();
        fixPressureInfo_->searchParts_ =
          fix_pressure["search_target_part"].as<std::vector<std::string>>();
        if (fix_pressure["search_method"]) {
          std::string searchMethodName = fix_pressure["search_method"].as<std::string>();
          if (searchMethodName != "stk_kdtree")
            NaluEnv::self().naluOutputP0() << "FixPressureAtNodeInfo::search_method only supports stk_kdtree"
                                           << std::endl;
        }
      }
      else {
        fixPressureInfo_->stkNodeId_ = fix_pressure["node_identifier"].as<unsigned int>();
      }
    }

    // error indicator option
    const YAML::Node y_error_indicator = expect_map(y_solution_options, "error_indicator", optional);
    if (y_error_indicator) {
      
      errorIndicatorActive_ = true;

      std::string type = "pstab";
      get_if_present(y_error_indicator, "type", type, type);
      if (type == "pstab")
        errorIndicatorType_ = EIT_PSTAB;
      else if (type == "limiter")
        errorIndicatorType_ = EIT_LIMITER;
      else if (type == "simple.vorticity_dx")
        errorIndicatorType_ = EIT_SIMPLE_VORTICITY_DX;
      else if (type == "simple.vorticity")
        errorIndicatorType_ = EIT_SIMPLE_VORTICITY;
      else if (type == "simple.dudx2")
        errorIndicatorType_ = EIT_SIMPLE_DUDX2;
    
      get_if_present(y_error_indicator, "frequency", errorIndicatorFrequency_, errorIndicatorFrequency_);

      NaluEnv::self().naluOutputP0() << std::endl;
      NaluEnv::self().naluOutputP0() << "Error Indicator Options Review: " << std::endl;
      NaluEnv::self().naluOutputP0() << "================================" << std::endl;
      NaluEnv::self().naluOutputP0() << " Type: " << type << std::endl;
      NaluEnv::self().naluOutputP0() << " Frequency: " << errorIndicatorFrequency_ << std::endl;
    }

    NaluEnv::self().naluOutputP0() << std::endl;
    NaluEnv::self().naluOutputP0() << "Turbulence Model Review:   " << std::endl;
    NaluEnv::self().naluOutputP0() << "===========================" << std::endl;
    NaluEnv::self().naluOutputP0() << "Turbulence Model is: "
                                   << TurbulenceModelNames[turbulenceModel_] << " " << isTurbulent_ <<std::endl;
    
    // over view PPE specifications
    NaluEnv::self().naluOutputP0() << std::endl;
    NaluEnv::self().naluOutputP0() << "PPE review:   " << std::endl;
    NaluEnv::self().naluOutputP0() << "===========================" << std::endl;
    
    if ( cvfemShiftMdot_ )
      NaluEnv::self().naluOutputP0() << "Shifted CVFEM mass flow rate" << std::endl;
    if ( cvfemReducedSensPoisson_)
      NaluEnv::self().naluOutputP0() << "Reduced sensitivities CVFEM Poisson" << std::endl;
    
    // sanity checks; if user asked for shifted Poisson, then user will have reduced sensitivities
    if ( get_shifted_grad_op("pressure") ) {
      if ( !cvfemReducedSensPoisson_) {
        NaluEnv::self().naluOutputP0() << "Reduced sensitivities CVFEM Poisson will be set since reduced grad_op is requested" << std::endl;
        cvfemReducedSensPoisson_ = true;
      }
    }
    
    // overview gradient operator for CVFEM
    if ( shiftedGradOpMap_.size() > 0 ) {
      NaluEnv::self().naluOutputP0() << std::endl;
      NaluEnv::self().naluOutputP0() << "CVFEM gradient operator review:   " << std::endl;
      NaluEnv::self().naluOutputP0() << "===========================" << std::endl;
      for ( const auto& shiftIt : shiftedGradOpMap_ ) {
        NaluEnv::self().naluOutputP0() << " dof: " << shiftIt.first
                                       << " shifted: " << (shiftIt.second ? "yes" : "no") << std::endl; 
      }
    }
  } 
}

//--------------------------------------------------------------------------
//-------- initialize_turbulence_constants ---------------------------------
//--------------------------------------------------------------------------
void
SolutionOptions::initialize_turbulence_constants() 
{
  // set the default map values; resize to max turbulence model enum
  turbModelConstantMap_[TM_cMu] = 0.09; 
  turbModelConstantMap_[TM_kappa] = 0.41;
  turbModelConstantMap_[TM_cDESke] = 0.61; 
  turbModelConstantMap_[TM_cDESkw] = 0.78;
  turbModelConstantMap_[TM_tkeProdLimitRatio] = (turbulenceModel_ == SST || turbulenceModel_ == SST_DES) ? 10.0 : 500.0;
  turbModelConstantMap_[TM_cmuEps] = 0.0856; 
  turbModelConstantMap_[TM_cEps] = 0.845;
  turbModelConstantMap_[TM_betaStar] = 0.09;
  turbModelConstantMap_[TM_aOne] = 0.31;
  turbModelConstantMap_[TM_betaOne] = 0.075;
  turbModelConstantMap_[TM_betaTwo] = 0.0828;
  turbModelConstantMap_[TM_gammaOne] = 5.0/9.0;
  turbModelConstantMap_[TM_gammaTwo] = 0.44;
  turbModelConstantMap_[TM_sigmaKOne] = 0.85;
  turbModelConstantMap_[TM_sigmaKTwo] = 1.0;
  turbModelConstantMap_[TM_sigmaWOne] = 0.50;
  turbModelConstantMap_[TM_sigmaWTwo] = 0.856;
  turbModelConstantMap_[TM_cmuCs] = 0.17;
  turbModelConstantMap_[TM_Cw] = 0.325;
  turbModelConstantMap_[TM_CbTwo] = 0.35;
  turbModelConstantMap_[TM_SDRWallFactor] = 1.0;
  turbModelConstantMap_[TM_zCV] = 0.5;
  turbModelConstantMap_[TM_ci] = 0.9;
  turbModelConstantMap_[TM_elog] = 9.8;
  turbModelConstantMap_[TM_yplus_crit] = 11.63;
  turbModelConstantMap_[TM_cEpsOne] = 1.44;
  turbModelConstantMap_[TM_cEpsTwo] = 1.92;
}


double
SolutionOptions::get_alpha_factor(const std::string& dofName) const
{
  double factor = alphaDefault_;
  auto iter = alphaMap_.find(dofName);

  if (iter != alphaMap_.end())
    factor = iter->second;

  return factor;
}

double
SolutionOptions::get_alpha_upw_factor(const std::string& dofName) const
{
  double factor = alphaUpwDefault_;
  auto iter = alphaUpwMap_.find(dofName);

  if (iter != alphaUpwMap_.end())
    factor = iter->second;

  return factor;
}

double
SolutionOptions::get_upw_factor(const std::string& dofName) const
{
  double factor = upwDefault_;
  auto iter = upwMap_.find(dofName);

  if (iter != upwMap_.end())
    factor = iter->second;

  return factor;
}

bool
SolutionOptions::primitive_uses_limiter(const std::string& dofName) const
{
  bool usesIt = false;
  auto iter = limiterMap_.find(dofName);
  if (iter != limiterMap_.end())
    usesIt = iter->second;

  return usesIt;
}

std::string
SolutionOptions::limiter_type(const std::string& dofName) const
{
  std::string limiterType = "default";
  auto iter = limiterTypeMap_.find(dofName);
  if (iter != limiterTypeMap_.end())
    limiterType = iter->second;

  if (limiterType != "van_leer" && limiterType != "minmod" &&
      limiterType != "superbee" && limiterType != "ultrabee" &&
      limiterType != "default"
      ) {
    throw std::runtime_error("SolutionOptions::limiter_type() called with unknown limiter type: " + limiterType);
  }
  NaluEnv::self().naluOutputP0() << "SolutionOptions::limiter_type() for dofName: " << dofName
                                 << " limiter type: " << limiterType << std::endl;

  return limiterType;
}



bool
SolutionOptions::get_shifted_grad_op(const std::string& dofName) const
{
  bool factor = shiftedGradOpDefault_;
  auto iter = shiftedGradOpMap_.find(dofName);

  if (iter != shiftedGradOpMap_.end())
    factor = iter->second;

  return factor;
}

bool
SolutionOptions::get_skew_symmetric(const std::string& dofName) const
{
  bool factor = skewSymmetricDefault_;
  auto iter = skewSymmetricMap_.find(dofName);

  if (iter != skewSymmetricMap_.end())
    factor = iter->second;

  return factor;
}

std::array<double, 3> 
SolutionOptions::get_gravity_vector() const
{
  return gravity_;
}

//--------------------------------------------------------------------------
//-------- get_turb_model_constant() ------------------------------------------
//--------------------------------------------------------------------------
double
SolutionOptions::get_turb_model_constant(
   TurbulenceModelConstant turbModelEnum) const
{
  std::map<TurbulenceModelConstant, double>::const_iterator it
    = turbModelConstantMap_.find(turbModelEnum);
  if ( it != turbModelConstantMap_.end() ) {
    return it->second;
  }
  else {
    throw std::runtime_error("unknown (not found) turbulence model constant");
  }
}

bool
SolutionOptions::get_noc_usage(
  const std::string &dofName ) const
{
  bool factor = nocDefault_;
  std::map<std::string, bool>::const_iterator iter
    = nocMap_.find(dofName);
  if (iter != nocMap_.end()) {
    factor = (*iter).second;
  }
  return factor;
}

double
SolutionOptions::get_turb_prandtl(
  const std::string &dofName ) const
{
  double factor = turbPrDefault_;
  std::map<std::string, double>::const_iterator iter
    = turbPrMap_.find(dofName);
  if (iter != turbPrMap_.end()) {
    factor = (*iter).second;
  }
  return factor;
}

void SolutionOptions::set_consolidated_bc_solver_alg()
{
  useConsolidatedBcSolverAlg_ = true;
}

// MUSCL
//--------------------------------------------------------------------------
//-------- get_muscl_usage() ------------------------------------------
//--------------------------------------------------------------------------
bool
SolutionOptions::get_muscl_usage(
  const std::string &dofName ) const
{
  bool useMuscl = musclDefault_;
  std::map<std::string, bool>::const_iterator iter
    = useMusclMap_.find(dofName);
  if (iter != useMusclMap_.end()) {
     useMuscl = (*iter).second;
  }
  return useMuscl;
}
//--------------------------------------------------------------------------
//-------- get_kappa_muscl_factor() ----------------------------------------
//--------------------------------------------------------------------------
double
SolutionOptions::get_kappa_muscl_factor(
   const std::string& dofName) const {
  double factor = kappaMusclDefault_;
  std::map<std::string, double>::const_iterator iter = kappaMusclMap_.find(dofName);
  if (iter != kappaMusclMap_.end()) {
    factor = (*iter).second;
  }
  NaluEnv::self().naluOutputP0() << "SolutionOptions::get_kappa_muscl_factor() for dofName: " << dofName
                                 << " kappa_muscl factor: " << factor << std::endl;

  return factor;
} // get_kappa_muscl_factor

} // namespace nalu
} // namespace Sierra
