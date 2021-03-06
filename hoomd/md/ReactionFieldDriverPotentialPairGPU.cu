// Copyright (c) 2009-2018 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.

/*! \file ReactionFieldDriverPotentialPairGPU.cu
    \brief Defines the driver functions for computing all types of pair forces on the GPU
*/

#include "EvaluatorPairReactionField.h"
#include "AllDriverPotentialPairGPU.cuh"
cudaError_t gpu_compute_reaction_field_forces(const pair_args_t & args,
                                                const Scalar3 *d_params)
    {
    return gpu_compute_pair_forces<EvaluatorPairReactionField>(args,
                                                     d_params);
    }

