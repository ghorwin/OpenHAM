#ifndef MATERIAL_H
#define MATERIAL_H

#include <IBK_LinearSpline.h>

/*! Implements material functions. */
class Material {
public:
	Material(int index);

	/*! MRC: Returns moisture content [m3/m3] for capillary pressure [Pa]. */
	double Ol_pc(double pc) const;
	/*! Returns thermal conductivity [J/msK] for moisture content [m3/m3]. */
	double lambda_Ol(double Ol) const;
	/*! Returns liquid conductivity [log10(s)] for moisture content [m3/m3]. */
	double Kl_Ol(double Ol) const;
	/*! Returns vapor permeability [s] for moisture content [m3/m3]. */
	double Kv_Ol(double T, double Ol) const;

	int				m_i;		///< Material index number

	std::string		m_name;		///< Some descriptive name.
	bool			m_isAir;
	double			m_rho; 		///< Bulk density [kg/m3]
	double			m_cT;		///< Heat capacity [J/kgK]
	double			m_lambda;	///< Thermal conductivity [J/msK]
	double			m_Opor;		///< Open porosity in [m3/m3]
	double			m_Oeff;		///< Effective saturation in [m3/m3]

	IBK::LinearSpline	m_klSpline;
};

#endif // MATERIAL_H
