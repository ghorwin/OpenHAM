#include "Integrator.h"

#include <cstring>

Integrator::Integrator() :
	m_outputs(&m_model),
	m_logFileStream(NULL)
{
}


Integrator::~Integrator() {
	delete m_logFileStream;
}


void Integrator::setupMemory() {
	m_n = m_model.m_nElements * 2; // two balance equations per element

	m_zn.resize(m_n);
	m_z.resize(m_n);

	m_yn.resize(m_n);
	m_y.resize(m_n);
	m_ydot.resize(m_n);
	m_res.resize(m_n);

	m_zJac.resize(m_n);
	m_yJac.resize(m_n);
	m_ydotJac.resize(m_n);
	m_resJac.resize(m_n);

	m_jacobian.resize(m_n, 3, 3, 6);
}


void Integrator::run() {

	// resize vectors
	setupMemory();

	// store initial conserved quantities
	for (unsigned int i=0; i<m_model.m_nElements; ++i) {
		m_yn[i*2] = m_model.m_u[i];
		m_yn[i*2+1] = m_model.m_rhowv[i];
		m_zn[i*2] = m_model.m_zU[i];
		m_zn[i*2+1] = m_model.m_zRhowv[i];
	}

	// we use constant prediction, z = z_n, to get first iterative values
	copyVec(m_zn, m_z);
	copyVec(m_yn, m_y);


	// write initial conditions to file
	IBK::Path resRootDir = IBK::Path(m_model.m_projectFile).withoutExtension();
	IBK::Path outputRootDir = resRootDir / "results";
	m_outputs.setupOutputFiles(outputRootDir);
	m_outputs.appendOutputs();

	IBK::Path logPath = resRootDir / "log";
	if (!logPath.exists())
		IBK::Path::makePath(logPath);
	logPath = logPath / "screenlog.txt";
	m_logFileStream = new std::ofstream(logPath.str().c_str());
	*m_logFileStream << std::setw(10) << "Time [h]" << '\t'
					 << std::setw(10) << "dt [s]" << '\t'
					 << std::setw(5) << "step" << '\t'
					 << std::setw(5) << "iter" << '\t'
					 << std::setw(10) << "resNorm" << '\t'
					 << std::setw(10) << "deltaNorm" << std::endl;

	m_dt = m_model.m_dt0;

	m_statRhsEvals = 0;
	m_statNonLinIters = 0;
	m_statSteps = 0;
	m_statJacEvals = 0;

	// time loop, integrator time always starts at 0, mapping to real time is done
	// in climate data loader and output handler

	m_t = 0;
	double nextOutputTime = m_model.m_dtOutput;
	while (m_t < m_model.m_tEnd) {

		// determine length of integration step, use suggestion in model as bases, but reduce if
		// output time point is within next step
		if (m_t+m_dt > nextOutputTime)
			m_dt = nextOutputTime - m_t; // to hit directly the output time point

		// integrate step, this will advance our simulation time point and also set a new proposed time step m_dt
		step();
		// Note: m_z == m_zn, m_y == m_yn, m_t := m_t + m_dt

		// if we have reached the output time point, write outputs (use fuzzy comparison)
		if (m_t > nextOutputTime - 1e-8) { /// \note fuzzy comparison needed to compensate for rounding errors
			m_outputs.appendOutputs();
			nextOutputTime += m_model.m_dtOutput; /// \todo improve in case of significant round-off errors
#ifndef ENABLE_STEP_STATS
		std::cout << std::setw(10) << m_t/3600 << '\t'
						 << std::setw(10) << m_dt << '\t'
						 << std::setw(5) << m_statSteps << '\t'
						 << std::setw(5) << m_statNonLinIters << std::endl;
		*m_logFileStream << std::setw(10) << m_t << '\t'
						 << std::setw(10) << m_dt << '\t'
						 << std::setw(5) << m_statSteps << '\t'
						 << std::setw(5) << m_nonlinIters << '\t'
						 << std::setw(10) << m_resNorm << '\t'
						 << std::setw(10) << m_deltaNorm << std::endl;
#endif // ENABLE_STEP_STATS
		}
	}

}


void Integrator::step() {
	const char * const FUNC_ID = "[Integrator::step]";
	// do an integration step from current time point m_t to m_t + m_dt

	// compute next time point (the time point we solve for)
	m_tNext = m_t + m_dt;

	bool errorTestPassed = false;

	// error test reduction loop
	while (!errorTestPassed) {

		// time step reduction loop, if iteration does not converge
		bool converged = false;
		while (!converged) {

			// Implicit Euler method, iteration loop, either Picard iteration or Newton

			m_nonlinIters = 0;
			converged = solveNewton();
#ifdef ENABLE_STEP_STATS
			std::cout << std::setw(10) << std::right << m_tNext/3600
					  << std::setw(10) << std::right << m_dt
					  << "  " << (converged ? "Converged" : "Failed") << std::endl;
#endif // ENABLE_STEP_STATS

			// if not converged
			if (!converged) {
				m_dt *= 0.5;
				if (m_dt < m_model.m_minDt)
					throw IBK::Exception( IBK::FormatString("m_dt = %1 < %2 (minimum allowed time step)")
										  .arg(m_dt)
										  .arg(m_model.m_minDt), FUNC_ID);
				m_tNext = m_t + m_dt;
				// reset iterative variable
				copyVec(m_zn, m_z);
			}

		} // converged


		/// \todo Error test
		errorTestPassed = true;

	} // while errorTestPassed

	// store new time point
	m_t = m_tNext;

	// store compute states
	copyVec(m_z, m_zn);
	copyVec(m_y, m_yn);

	++m_statSteps;
	m_statNonLinIters += m_nonlinIters;

	// compute new proposal for next iteration step
	m_dt *= 1.1 + (m_nonlinIters*0.8/m_model.m_maxNonLinIters);
	m_dt = std::min(m_dt, m_model.m_maxDt);

}


bool Integrator::solveNewton() {
	// 1. Iteration equation is given by: 0 = y_n+1 - y_n - dt*f(t_n+1, y_n+1)
	//
	// 2. Solution variable is z, with model-specified mapping of y_n+1 = y(z_n+1)
	//
	// 3. Modified iteration equation is:
	//      0 = y(z_n+1) - y_n - dt*f(t_n+1, z_n+1)
	//            or
	//      0 = y(z) - y_n - dt*f(t_n+1, z)
	//
	//    Mind that we define now f as function of f(t,z)!!!
	//
	// 4. Root finding equation reads:
	//     F(z) = 0 = y(z) - y_n - dt*f(t_n+1, z)
	//
	//    Introduce iteration level m (versus time level n)
	//    Newton scheme:
	//     dF/dz|m * (z_m+1 - z_m) = - F(z_m)

	// 5. Start iteration with z_m=0 == z_n  stored in m_z

	// We update Jacobian in every iteration step
	bool converged = false;
	while (!converged && m_nonlinIters < m_model.m_maxNonLinIters) {
		++m_nonlinIters;

		// evaluate -F(z_m) function (residuals)

		// compute rhs: y(z_m)        -> m_y
		//              ydot(t, z_m)  -> m_ydot
		try {
			m_model.ydot_z(m_tNext, m_z, m_y, m_ydot);
		}
		catch (IBK::Exception & ex) {
			ex.writeMsgStackToError();
			return false;
		}

		// compute residuals: res = y(z) - y_n - dt*f(t_n+1, z)
		for (unsigned int i=0; i<m_n; ++i) {
			m_res[i] = m_y[i] - m_yn[i] - m_dt*m_ydot[i];
		}
		m_resNorm = norm(m_y, m_res);

		// compute Jacobian
		try {
			generateJacobian();
		}
		catch (IBK::Exception & ex) {
			ex.writeMsgStackToError();
			return false;
		}

		// compose rhs: -F(z)
		// solve equation system (delta z)
		for (unsigned int i=0; i<m_n; ++i) {
			m_resJac[i] = -m_res[i];
		}
		m_jacobian.backsolve(&m_resJac[0]);

		// compute residual norm of differences

		/// \note It makes a difference whether difference norm is computed based on z-difference
		/// and/or y differences.
		m_deltaNorm = norm(m_z, m_resJac);
#ifdef ENABLE_STEP_STATS
		std::cout << std::setw(20) << " "
				  << std::setw(5)  << std::right << m_statNonLinIters << "  "
				  << std::setw(5)  << std::right << m_nonlinIters << "  "
				  << std::setw(10) << std::right << m_resNorm << "  "
				  << std::setw(10) << std::right << m_deltaNorm << std::endl;
		*m_logFileStream << std::setw(10) << m_t << '\t'
						 << std::setw(10) << m_dt << '\t'
						 << std::setw(5) << m_statSteps << '\t'
						 << std::setw(5) << m_nonlinIters << '\t'
						 << std::setw(10) << m_resNorm << '\t'
						 << std::setw(10) << m_deltaNorm << std::endl;
#endif // ENABLE_STEP_STATS

		// add delta z to m_z
		for (unsigned int i=0; i<m_n; ++i)
			m_z[i] += m_resJac[i];

		if (m_resNorm < 1)
			converged = true;
	}


	return converged;
}



void Integrator::generateJacobian() {
	// create backup copy of residuals and z
	copyVec(m_res, m_resJac);
	copyVec(m_z, m_zJac);

	// loop over all groups
	unsigned int m = 7; // block-tridiagonal structure with 2x2 block dimension

	// Use Curtis-Powel-Reid algorithm, modify m_zJac in groups
	for (unsigned int i=0; i<7; ++i) {

		// modify m_zJac[] with stride m
		for (unsigned int j=i; j<m_n; j += m) {
			/// \note The adjustment can be computed based on values of z, or values of y and converted to z
			double diff = std::fabs(m_z[j])*1e-8 + 1e-12;
			m_zJac[j] += diff;
		}

		// calculate modified right hand side
		m_model.ydot_z(m_tNext, m_zJac, m_yJac, m_ydotJac);
		for (unsigned int k=0; k<m_n; ++k) {
			m_resJac[k] = m_yJac[k] - m_yn[k] - m_dt*m_ydotJac[k];
		}

		// compute Jacobian elements in groups
		for (unsigned int j=i; j<m_n; j += m) {
			double diff = std::fabs(m_z[j])*1e-8 + 1e-12;
			// restrict jacobian assembly to the filled positions of the band
			unsigned int kl = std::max( 0, (int)(j - 3));
			unsigned int ku = std::min(j + 3, m_n - 1 );
			// F = y - yn - dt*ydot,
			for (unsigned int k=kl; k<=ku; ++k) {
				// compute finite-differences column j in row k
				double resDiff = m_resJac[k] - m_res[k];
				m_jacobian(k,j) = resDiff/diff;
			}
		}
		// Jacobian matrix now holds df/dz
		// update solver statistics
		++m_statRhsEvals;

		// restore original vector
		for (unsigned int j=i; j<m_n; j += m) {
			// restore y vector
			m_zJac[j] = m_z[j];
		} // for j

		// restore residuals
		copyVec(m_res, m_resJac);

	} // for i

//#define DUMP_JACOBIAN
#ifdef DUMP_JACOBIAN
	std::ofstream jacdump("jacobian.txt");
	m_jacobian.write(jacdump, NULL, false, 14);
#endif // DUMP_JACOBIAN

	// perform LU-factorization of the jacobian
	m_jacobian.lu();

	// update solver statistics
	++m_statJacEvals;
}


double Integrator::norm(const std::vector<double> & values, const std::vector<double> & diff) const {
	double norm = 0;
	for (unsigned int i=0; i<m_n; ++i) {
		double weight = std::fabs(values[i])*1e-8 + 1e-10;
		double res = diff[i]/weight;
		norm += res*res;
	}
	norm = IBK::f_sqrt(norm);
	return norm;
}


void Integrator::copyVec(const std::vector<double> & src, std::vector<double> & target) {
	std::memcpy(&target[0], &src[0], target.size()*sizeof(double));
}