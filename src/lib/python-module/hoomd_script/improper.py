# Highly Optimized Object-Oriented Molecular Dynamics (HOOMD) Open
# Source Software License
# Copyright (c) 2008 Ames Laboratory Iowa State University
# All rights reserved.

# Redistribution and use of HOOMD, in source and binary forms, with or
# without modification, are permitted, provided that the following
# conditions are met:

# * Redistributions of source code must retain the above copyright notice,
# this list of conditions and the following disclaimer.

# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.

# * Neither the name of the copyright holder nor the names HOOMD's
# contributors may be used to endorse or promote products derived from this
# software without specific prior written permission.

# Disclaimer

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER AND
# CONTRIBUTORS ``AS IS''  AND ANY EXPRESS OR IMPLIED WARRANTIES,
# INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
# AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 

# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS  BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.

# $Id$
# $URL$

import force;
import globals;
import math;
import hoomd;
import sys;
import util;

## \package hoomd_script.improper
# \brief Commands that specify %improper forces
#
# Impropers add forces between specified quaduplets of particles and are typically used to 
# model rotation about chemical bonds without having bonds to connect the atoms. Impropers between particles are set when an input XML file is read
# (init.read_xml) or when an another initializer creates them (like init.create_random_polymers)
#
# By themselves, impropers that have been specified in an input file do nothing. Only when you 
# specify an improper force (i.e. improper.harmonic), are forces actually calculated between the 
# listed particles.

## Harmonic %improper forces
#
# The command improper.harmonic specifies a %harmonic improper potential energy between every qudruplet of particles
# in the simulation. 
# \f[ V(r) = k \left( chi - chi_{0}(r)  \right )^2 \f]
# where \f$ chi \f$ is angle between two sides of the improper
#
# Coefficients \f$ k \f$ and \f$ chi_0 \f$ must be set for each type of %improper in the simulation using
# set_coeff().
#
# \note Specifying the improper.harmonic command when no impropers are defined in the simulation results in an error.
class harmonic(force._force):
	## Specify the %harmonic %improper %force
	#
	# \b Example:
	# \code
	# harmonic = improper.harmonic()
	# \endcode
	def __init__(self):
		util.print_status_line();
		# check that some impropers are defined
		if globals.particle_data.getImproperData().getNumImpropers() == 0:
			print >> sys.stderr, "\n***Error! No impropers are defined.\n";
			raise RuntimeError("Error creating improper forces");		
		
		# initialize the base class
		force._force.__init__(self);
		
		# create the c++ mirror class
		if globals.particle_data.getExecConf().exec_mode == hoomd.ExecutionConfiguration.executionMode.CPU:
			self.cpp_force = hoomd.HarmonicImproperForceCompute(globals.particle_data);
		elif globals.particle_data.getExecConf().exec_mode == hoomd.ExecutionConfiguration.executionMode.GPU:
			self.cpp_force = hoomd.HarmonicImproperForceComputeGPU(globals.particle_data);
		else:
			print >> sys.stderr, "\n***Error! Invalid execution mode\n";
			raise RuntimeError("Error creating improper forces");

		globals.improper_compute = self.cpp_force;

		globals.system.addCompute(self.cpp_force, self.force_name);
		
		# variable for tracking which improper type coefficients have been set
		self.improper_types_set = [];
	
	## Sets the %harmonic %improper coefficients for a particular %improper type
	#
	# \param improper_type Improper type to set coefficients for
	# \param k Coefficient \f$ k \f$ in the %force
	# \param chi Coefficient \f$ chi \f$ in the %force
	#
	# Using set_coeff() requires that the specified %improper %force has been saved in a variable. i.e.
	# \code
	# harmonic = improper.harmonic()
	# \endcode
	#
	# \b Examples:
	# \code
	# harmonic.set_coeff('heme-ang', k=30.0, chi=1.57)
	# harmonic.set_coeff(hdyro-bond', k=20.0, chi=1.57)
	# \endcode
	#
	# The coefficients for every %improper type in the simulation must be set 
	# before the run() can be started.
	def set_coeff(self, improper_type, k, chi):
		util.print_status_line();
		
		# set the parameters for the appropriate type
		self.cpp_force.setParams(globals.particle_data.getImproperData().getTypeByName(improper_type), k, chi);
		
		# track which particle types we have set
		if not improper_type in self.improper_types_set:
			self.improper_types_set.append(improper_type);
		
	def update_coeffs(self):
		# get a list of all improper types in the simulation
		ntypes = globals.particle_data.getImproperData().getNImproperTypes();
		type_list = [];
		for i in xrange(0,ntypes):
			type_list.append(globals.particle_data.getImproperData().getNameByType(i));
			
		# check to see if all particle types have been set
		for cur_type in type_list:
			if not cur_type in self.improper_types_set:
				print >> sys.stderr, "\n***Error:", cur_type, " coefficients missing in improper.harmonic\n";
				raise RuntimeError("Error updating coefficients");
