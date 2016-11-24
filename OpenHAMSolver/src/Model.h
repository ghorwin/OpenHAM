#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <vector>
#include <map>

#include "Material.h"

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


	Model();

	/*! Init model from project file. */
	void init(const std::string & projectFile);

	/*! Reads project file.
		\param projectFile The path to the project file.
		\param keyValuePairs Holds all key-value pairs defined in project file
	*/
	void readProjectFile(const std::string & projectFile, std::map<std::string, std::string> & keyValuePairs);

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

	std::string			m_projectFile;

	VariableMapping		m_variableMapping;

	/*! Maximum number of allowed non-linear iterations. */
	unsigned int		m_maxNonLinIters;

	/*! Maximum time step size in [s]. */
	double				m_maxDt;
	/*! Minimum time step size in [s]. */
	double				m_minDt;

	/*! Current simulation time point (state of model) in [s]. */
	double				m_t;

	/*! Simulation duration in [s]. */
	double				m_tEnd;

	/*! Initial time step size in [s]. */
	double				m_dt0;

	/*! Output step size in [s]. */
	double				m_dtOutput;



	// Member variables controling the grid

	/*! Number of elements. */
	unsigned int				m_nElements;

	/*! Material index for each element. */
	std::vector<unsigned int>	m_matIdx;

	/*! Materials used in model. */
	std::vector<Material>		m_materials;

	/*! Element sizes in [m] from left to right. */
	std::vector<double>			m_dx;


	// Member variables defining boundary conditions
	double						m_alphaLeft;
	double						m_betaLeft;

	double						m_alphaRight;
	double						m_betaRight;

	IBK::LinearSpline			m_TLeftSpline;
	double						m_TLeft;
	IBK::LinearSpline			m_rhLeftSpline;
	double						m_rhLeft;

	IBK::LinearSpline			m_TRightSpline;
	double						m_TRight;
	IBK::LinearSpline			m_rhRightSpline;
	double						m_rhRight;

	IBK::LinearSpline			m_gRainSpline;

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
	std::vector<double>		m_jv;
	std::vector<double>		m_hv;
	std::vector<double>		m_jw;
	std::vector<double>		m_hw;

	// Boundary fluxes

	double						m_qLeft;
	double						m_jvLeft;
	double						m_hvLeft;
	double						m_jwLeft;
	double						m_hwLeft;

	double						m_qRight;
	double						m_jvRight;
	double						m_hvRight;

};

#endif // MODEL_H
