#ifndef INTEGRATOR_H
#define INTEGRATOR_H

#include "Model.h"
#include "Outputs.h"
#include "BandMatrix.h"

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

#endif // INTEGRATOR_H
