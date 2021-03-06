// Copyright (c) 2009-2018 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.

/*! \file DPDThermoDriverPotentialPairGPU.cu
    \brief Defines the driver functions for computing all types of pair forces on the GPU
*/

#include "PotentialPairDPDThermoGPU.cuh"
#include "EvaluatorPairDPDThermo.h"
#include "AllDriverPotentialPairGPU.cuh"

cudaError_t gpu_compute_dpdthermodpd_forces(const dpd_pair_args_t& args,
                                            const Scalar2 *d_params)
    {
    return gpu_compute_dpd_forces<EvaluatorPairDPDThermo>(args,
                                                          d_params);
    }


cudaError_t gpu_compute_dpdthermo_forces(const pair_args_t& pair_args,
                                         const Scalar2 *d_params)
    {
    return gpu_compute_pair_forces<EvaluatorPairDPDThermo>(pair_args,
                                                           d_params);
    }


