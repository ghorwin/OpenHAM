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

#ifndef IntegratorH
#define IntegratorH

#include "Model.h"
#include "Outputs.h"
#include "BandMatrix.h"
#include "SolverFeedback.h"

/*! The time integrator (Implicit Euler) */
class Integrator {
public:
	Integrator();
	~Integrator();

	/*! Allocates memory/resizes vectors */
	void setupMemory();

	/*! Main solver start function. */
	void run();

	/*! Do an integration step and return new integrator time point. */
	void step();

	/*! Solve Newton iteration.
		Solves the root finding problem based on old states m_yn at time m_t and new solution
		variables m_z at m_tNext.
		\return Returns true on success, false on convergence failure.
	*/
	bool solveNewton();

	/*! Generates Jacobian based on next time point m_tNext and states using DQ approximation. */
	void generateJacobian();

	/*! Computes a WRMS norm using the values vector to generate weights and
		the diff vector for the norm calculation.
	*/
	double norm(const std::vector<double> & values, const std::vector<double> & diff) const;

	/*! Copies content of vector efficiently. */
	static void copyVec(const std::vector<double> & src, std::vector<double> & target);

	/*! Physical model implementation. */
	Model					m_model;
	/*! Handles output writing. */
	Outputs					m_outputs;
	/*! Prints simulation progress. */
	SolverFeedback			m_feedback;

	/*! If true, statistics are written after each solver step into stats.tsv, otherwise only
		at output time points.
	*/
	bool					m_solverStepStats;

	/*! Current time point in [s], the time point that was accepted in last successful call to step(). */
	double					m_t;
	/*! Proposed time point in [s], set in step() and used in generateJacobian(). */
	double					m_tNext;

	/*! Proposed time step for next integration step in [s]. */
	double					m_dt;

	/*! Number of unknowns in integrator. */
	unsigned int			m_n;


	/*! Holds solution variables at last time point. */
	std::vector<double>		m_zn;
	/*! Holds current solution variables. */
	std::vector<double>		m_z;
	/*! Holds conserved quantities at last time point. */
	std::vector<double>		m_yn;
	/*! Holds current estimates of conserved variables. */
	std::vector<double>		m_y;
	/*! Holds time derivatives of current estimates of conserved variables. */
	std::vector<double>		m_ydot;
	/*! Holds residuals of F(z) function. */
	std::vector<double>		m_res;

	/*! Holds residual norm computed after iteration step. */
	double					m_resNorm;
	/*! Holds difference norm computed after iteration step. */
	double					m_deltaNorm;

	// Workspace vectors for numeric Jacobian generation

	std::vector<double>		m_zJac;
	std::vector<double>		m_yJac;
	std::vector<double>		m_ydotJac;
	std::vector<double>		m_resJac;

	BandMatrix				m_jacobian;

	/*! Counter for non-linear iterations in last step, needed for dt adjustment. */
	unsigned int			m_nonlinIters;

	unsigned int			m_statRhsEvals;
	unsigned int			m_statNonLinIters;
	unsigned int			m_statSteps;
	unsigned int			m_statJacEvals;

	std::ostream			*m_logFileStream;

};

#endif // IntegratorH
