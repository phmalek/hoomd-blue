// Copyright (c) 2009-2016 The Regents of the University of Michigan
// This file is part of the HOOMD-blue project, released under the BSD 3-Clause License.


// this include is necessary to get MPI included before anything else to support intel MPI
#include "hoomd/ExecutionConfiguration.h"

#include <iostream>

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#include "hoomd/md/Enforce2DUpdater.h"
#include "hoomd/md/AllPairPotentials.h"
#include "hoomd/md/NeighborListBinned.h"
#include "hoomd/md/TwoStepNVTMTK.h"
#include "hoomd/ComputeThermo.h"

#ifdef ENABLE_CUDA
#include "hoomd/md/Enforce2DUpdaterGPU.h"
#endif

#include "hoomd/md/IntegratorTwoStep.h"

#include "hoomd/extern/saruprng.h"

#include <math.h>

using namespace std;
using namespace boost;

//! label the boost test module
#define BOOST_TEST_MODULE Enforce2DUpdaterTests
#include "boost_utf_configure.h"

/*! \file enforce2d_updater_test.cc
    \brief Unit tests for the Enforce2DUpdater class
    \ingroup unit_tests
*/

//! Typedef'd Enforce2DUpdater factory
typedef boost::function<boost::shared_ptr<Enforce2DUpdater> (boost::shared_ptr<SystemDefinition> sysdef)> enforce2d_creator;

//! boost test case to verify proper operation of Enforce2DUpdater
void enforce2d_basic_test(enforce2d_creator creator, boost::shared_ptr<ExecutionConfiguration> exec_conf)
    {
    BoxDim box(20.0, 20.0, 1.0);
    boost::shared_ptr<SystemDefinition> sysdef(new SystemDefinition(100, box, 1, 0, 0, 0, 0, exec_conf));

    sysdef->setNDimensions(2);
    boost::shared_ptr<ParticleData> pdata = sysdef->getParticleData();
    boost::shared_ptr<ParticleSelector> selector_all(new ParticleSelectorTag(sysdef, 0, pdata->getN()-1));
    boost::shared_ptr<ParticleGroup> group_all(new ParticleGroup(sysdef, selector_all));

    Saru saru(11, 21, 33);

    // setup a simple initial state
    Scalar tiny = 1e-3;
    for (unsigned int i=0; i<10; i++)
        for (unsigned int j=0; j<10; j++)
            {
            unsigned int k = i*10 + j;
            Scalar3 pos;
            pos.x = Scalar(2*i)-10.0 + tiny;
            pos.y = Scalar(2*j)-10.0 + tiny;
            pos.z = 0.0;
            pdata->setPosition(k, pos);
            Scalar3 vel;
            vel.x = saru.f(-1.0, 1.0);
            vel.y= saru.f(-1.0, 1.0);
            vel.z = 0.0;
            pdata->setVelocity(k, vel);
            }

    boost::shared_ptr<Variant> T(new VariantConst(1.0));
    boost::shared_ptr<ComputeThermo> thermo(new ComputeThermo(sysdef, group_all));
    thermo->setNDOF(2*group_all->getNumMembers()-2);
    boost::shared_ptr<TwoStepNVTMTK> two_step_nvt(new TwoStepNVTMTK(sysdef, group_all, thermo, 0.5, T));

    Scalar deltaT = Scalar(0.005);
    boost::shared_ptr<IntegratorTwoStep> nve_up(new IntegratorTwoStep(sysdef, deltaT));
    nve_up->addIntegrationMethod(two_step_nvt);

    boost::shared_ptr<NeighborListBinned> nlist(new NeighborListBinned(sysdef, Scalar(2.5), Scalar(0.3)));
    nlist->setStorageMode(NeighborList::half);

    boost::shared_ptr<PotentialPairLJ> fc(new PotentialPairLJ(sysdef, nlist));

    // setup some values for alpha and sigma
    Scalar epsilon = Scalar(1.0);
    Scalar sigma = Scalar(1.0);
    Scalar alpha = Scalar(1.0);
    Scalar lj1 = Scalar(4.0) * epsilon * pow(sigma,Scalar(12.0));
    Scalar lj2 = alpha * Scalar(4.0) * epsilon * pow(sigma,Scalar(6.0));

    // specify the force parameters
    fc->setParams(0,0,make_scalar2(lj1,lj2));
    fc->setRcut(0,0,Scalar(2.5));
    fc->setShiftMode(PotentialPairLJ::shift);

    nve_up->addForceCompute(fc);
    nve_up->prepRun(0);

    // verify that the atoms leave the xy plane if no contstraints are present
    // and random forces are added (due to roundoff error in a long simulation)s
    unsigned int np = pdata->getN();

    for (int t = 0; t < 1000; t++)
        {
        if (t%100 == 0) {
            ArrayHandle<Scalar4> h_vel(pdata->getVelocities(), access_location::host, access_mode::readwrite);
            ArrayHandle<Scalar3> h_accel(pdata->getAccelerations(), access_location::host, access_mode::readwrite);
            for (unsigned int i=0; i<np; i++)
                {
                h_accel.data[i].z += saru.f(-0.001, 0.002);
                h_vel.data[i].z += saru.f(-0.002, 0.001);
                }
            }
        nve_up->update(t);
        }

    Scalar total_deviation = Scalar(0.0);
    for (unsigned int i=0; i<np; i++)
        total_deviation += fabs(pdata->getPosition(i).z);

    //make sure the deviation is large (should be >> tol)
    BOOST_CHECK(total_deviation > tol);

    // re-initialize the initial state
    for (unsigned int i=0; i<10; i++)
        for (unsigned int j=0; j<10; j++)
            {
            unsigned int k = i*10 + j;
            Scalar3 pos;
            pos.x = Scalar(2*i)-10.0 + tiny;
            pos.y = Scalar(2*j)-10.0 + tiny;
            pos.z = 0.0;
            pdata->setPosition(k,pos);
            Scalar3 vel;
            vel.x = saru.f(-1.0, 1.0);
            vel.y = saru.f(-1.0, 1.0);
            vel.z = 0.0;
            pdata->setVelocity(k,vel);
            }

    pdata->notifyParticleSort();

    boost::shared_ptr<Enforce2DUpdater> enforce2d = creator(sysdef);

    // verify that the atoms never leave the xy plane if contstraint is present:
    for (int t = 0; t < 1000; t++)
        {
        if (t%100 == 0) {
            ArrayHandle<Scalar4> h_vel(pdata->getVelocities(), access_location::host, access_mode::readwrite);
            ArrayHandle<Scalar3> h_accel(pdata->getAccelerations(), access_location::host, access_mode::readwrite);
            for (unsigned int i=0; i<np; i++)
                {
                h_accel.data[i].z += saru.f(-0.01, 0.02);
                h_vel.data[i].z += saru.f(-0.1, 0.2);
                }
            }
        enforce2d->update(t);
        nve_up->update(t);
        }

    total_deviation = Scalar(0.0);
    for (unsigned int i=0; i<np; i++)
        {
        total_deviation += fabs(pdata->getPosition(i).z);
        }

    MY_BOOST_CHECK_CLOSE(total_deviation, 0.0, tol);

    }

//! Enforce2DUpdater creator for unit tests
boost::shared_ptr<Enforce2DUpdater> base_class_enforce2d_creator(boost::shared_ptr<SystemDefinition> sysdef)
    {
    return boost::shared_ptr<Enforce2DUpdater>(new Enforce2DUpdater(sysdef));
    }

#ifdef ENABLE_CUDA
//! Enforce2DUpdaterGPU creator for unit tests
boost::shared_ptr<Enforce2DUpdater> gpu_enforce2d_creator(boost::shared_ptr<SystemDefinition> sysdef)
    {
    return boost::shared_ptr<Enforce2DUpdater>(new Enforce2DUpdaterGPU(sysdef));
    }
#endif

//! boost test case for basic enforce2d tests
BOOST_AUTO_TEST_CASE( Enforce2DUpdater_basic )
    {
    enforce2d_creator creator = bind(base_class_enforce2d_creator, _1);
   enforce2d_basic_test(creator, boost::shared_ptr<ExecutionConfiguration>(new ExecutionConfiguration(ExecutionConfiguration::CPU)));
    }

#ifdef ENABLE_CUDA
//! boost test case for basic enforce2d tests
BOOST_AUTO_TEST_CASE( Enforce2DUpdaterGPU_basic )
    {
    enforce2d_creator creator = bind(gpu_enforce2d_creator, _1);
    enforce2d_basic_test(creator, boost::shared_ptr<ExecutionConfiguration>(new ExecutionConfiguration(ExecutionConfiguration::GPU)));
    }
#endif
