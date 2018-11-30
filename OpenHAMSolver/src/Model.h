/*	Copyright (c) 2001-2017, Institut für Bauklimatik, TU Dresden, Germany

	Written by Andreas Nicolai
	All rights reserved.

	This file is part of the OpenHAM software.

	Redistribution and use in source and binary forms, with or without modification,
	are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	3. Neither the name of the copyright holder nor the names of its contributors
	   may be used to endorse or promote products derived from this software without
	   specific prior written permission.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
	ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
	WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
	ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
	(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
	LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
	ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
	(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
	SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifndef ModelH
#define ModelH

#include <string>
#include <vector>
#include <map>

#include <IBK_SolverArgsParser.h>

#include <DELPHIN_Project.h>

#include "Material.h"
#include "Directories.h"

class Outputs;

/*! The physical model implementation. */
class Model {
public:
	enum VariableMapping {
		yz_Tpc,
		yz_Trh,
		yz_TpC,
		yz_Trhowv,
		yz_urhowv
	};

	/*! Init model from project file. */
	void init(const IBK::SolverArgsParser & args, Outputs & outputs);

	/*! Reads CCD file into linear spline.
		Values are converted into base IO unit upon read.
	*/
	void readCCD(const IBK::Path &ccdFile, IBK::LinearSpline & spline);


	/*! Compute converved quantities from iteration variable z and store result in y.
		Mind that for compute y[0] you need z[0] and z[1] and so on.
	*/
	void y_z(const std::vector<double> & z, std::vector<double> & y);

	/*! Update model to given time point t and variable z, compute derivatives and store result in ydot. */
	void ydot_z(double t, const std::vector<double> & z, std::vector<double> & y, std::vector<double> & ydot);

	/*! Updates all model variables based on solution variables m_zU and m_zRhowv.
		This implementation works for the variable mappings: yz_Tpc, yz_Trh and yz_TpC
		\param e Element index (0..m_nElements-1).
	*/
	void updateStatesTx(unsigned int e);

	/*! Computes boundary fluxes for current time and state of model. */
	void updateBoundaryConditions();

	/*! Computes internal fluxes. */
	void updateFluxes();



	// Member variables controling the integrator

	/*! Command line arguments. */
	IBK::SolverArgsParser	m_args;

	/*! File paths to log, result and project directory. */
	Directories				m_dirs;

	/*! Project file. */
	DELPHIN_LIGHT::Project	m_project;


	/*! Which potential shall be mapped to solution variables z. */
	VariableMapping			m_variableMapping;

	/*! Maximum number of allowed non-linear iterations. */
	unsigned int			m_maxNonLinIters;

	/*! Maximum time step size in [s]. */
	double					m_maxDt;
	/*! Minimum time step size in [s]. */
	double					m_minDt;

	/*! Current simulation time point (state of model) in [s]. */
	double					m_t;

	/*! Simulation duration in [s]. */
	double					m_tEnd;

	/*! Initial time step size in [s]. */
	double					m_dt0;

	/*! Output step size in [s]. */
	double					m_dtOutput;



	// Member variables controling the grid

	/*! Number of elements. */
	unsigned int				m_nElements;

	/*! Material index for each element (size m_nElements). */
	std::vector<unsigned int>	m_matIdx;

	/*! Materials used in model. */
	std::vector<Material>		m_materials;

	/*! Element/grid sizes in [m] from left to right (size m_nElements). */
	std::vector<double>			m_dx;


	// Member variables defining current boundary conditions

	/*! Left ambient air temperature in [K] */
	double						m_TLeft;
	/*! Left ambient air relative humidity in [0..1] */
	double						m_rhLeft;
	/*! Right ambient air temperature in [K] */
	double						m_TRight;
	/*! Right ambient air relative humidity in [0..1] */
	double						m_rhRight;

	// Member variables holdings profile data
	// All vectors have size m_nElements

	/*! Conserved quantity: energy density in [J/m3] */
	std::vector<double>		m_u;
	/*! Conserved quantity: moisture mass density in [kg/m3] */
	std::vector<double>		m_rhowv;

	/*! Solution variable (mapping to T, or u is done by model). */
	std::vector<double>		m_zU;
	/*! Solution variable (mapping to rh, pc, pC, or rhowv is done by model). */
	std::vector<double>		m_zRhowv;

	/*! Temperature in [K] */
	std::vector<double>		m_T;
	/*! Relative humidity in [%] */
	std::vector<double>		m_rh;
	std::vector<double>		m_pc;
	std::vector<double>		m_pC;
	std::vector<double>		m_pv;
	std::vector<double>		m_pvsat;

	/*! Liquid conductivity in [s] */
	std::vector<double>		m_Kl;
	/*! Vapour conductivity in [s] */
	std::vector<double>		m_Kv;
	/*! Thermal conductivity in [W/mK] */
	std::vector<double>		m_lambda;

	/*! Heat conduction flux [W/m2] */
	std::vector<double>		m_q;
	/*! Vapor diffusion mass flux density [kg/m2s] */
	std::vector<double>		m_jv;
	/*! Enthalpy flux associated with vapor diffusion in [W/m2] */
	std::vector<double>		m_hv;
	/*! Liquid convection mass flux density [kg/m2s] */
	std::vector<double>		m_jw;
	/*! Enthalpy flux associated with liquid conduction in [W/m2] */
	std::vector<double>		m_hw;


	// Boundary fluxes

	// left side - outside, heat conduction, vapor diffusion, rain

	double						m_qLeft;
	double						m_jvLeft;
	double						m_hvLeft;
	double						m_jwLeft;
	double						m_hwLeft;

	// right side - internal side, no rain

	double						m_qRight;
	double						m_jvRight;
	double						m_hvRight;


private:
	/*! Utility function to parse command line arguments.
		\todo Move to command line parser at some point.
	*/
	void extractDiscretizationOptions(bool & variableDisc, double & dx, double & stretch);
};

#endif // ModelH
