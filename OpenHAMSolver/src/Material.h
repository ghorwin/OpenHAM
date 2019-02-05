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

#ifndef MaterialH
#define MaterialH

#include <IBK_LinearSpline.h>
#include <IBK_Parameter.h>
#include <IBK_Path.h>

/*! Implements material functions.
	For each material layer, an instance of this class is initialized.
	For built-in materials, the material object is initialized by a call to init().
	For material data to be read from file, the material object is initialized
	with readFromFile().
*/
class Material {
public:
	/*! Default constructor initializes an invalid material, id = -1. */
	Material();

	/*! Initializes material data for built-in materials. */
	void init(int index);

	/*! Initializes material data from material files. */
	void readFromFile(const IBK::Path & m6FilePath);

	/*! MRC: Returns moisture content [m3/m3] for capillary pressure [Pa]. */
	double Ol_pc(double pc) const;
	/*! Returns thermal conductivity [J/msK] for moisture content [m3/m3]. */
	double lambda_Ol(double Ol) const;
	/*! Returns liquid conductivity [log10(s)] for moisture content [m3/m3]. */
	double Kl_Ol(double Ol) const;
	/*! Returns vapor permeability [s] for moisture content [m3/m3]. */
	double Kv_Ol(double T, double Ol) const;

	/*! Creates some plots for the material functions in this material, useful for generating reports. */
	void createPlots(const IBK::Path & plotDir, unsigned int layerIdx) const;

	int				m_i;		///< Material index number
	IBK::Path		m_filepath;	///< The file path that the material was read from (set in readFromFile()).

	std::string		m_name;		///< Some descriptive name.
	std::string		m_flags;	///< Parametrization flags;

	bool			m_isAir;	///< Set to true for air materials (different sorption isotherm)
	double			m_rho; 		///< Bulk density [kg/m3]
	double			m_cT;		///< Heat capacity [J/kgK]
	double			m_lambda;	///< Thermal conductivity [J/msK]
	IBK::Parameter	m_mew;		///< Mu-dry-value [---], if given.
	double			m_Oeff;		///< Effective saturation moisture content in [m3/m3]; w_sat = Oeff*1000 kg/m3


	/*! Moisture storage function: \f$\theta_\ell\left(p_C\right)\f$ */
	IBK::LinearSpline	m_Ol_pC_Spline;
	/*! Liquid transport function: \f$lgK_\ell\left(\theta_\ell\right)\f$
		\note Stores log10-values of liquid conductivity, and interpolates in logspace.
	*/
	IBK::LinearSpline	m_lgKl_Ol_Spline;
	/*! Vapour permeability function: \f$lgK_v\left(\theta_\ell\right)\f$
		\note Stores log10-values of vapor permeability, and interpolates in logspace.
	*/
	IBK::LinearSpline	m_lgKv_Ol_Spline;

	/*! Thermal conductivity spline: \f$\lambda\left(\theta_\ell\right)\f$
		If empty, default thermal conductivity spline is being used.
	*/
	IBK::LinearSpline	m_lambda_Ol_Spline;
};

#endif // MaterialH
