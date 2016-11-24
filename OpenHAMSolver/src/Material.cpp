#include "Material.h"

#include <IBK_math.h>

Material::Material(int index) {
//	const char * const FUNC_ID = "[Material::Material(int index)]";
	m_i = index;
	std::string logKlSplineOl;
	std::string logKlSplineLog10Kl;
	switch (index) {
		// Hamstad Brick
		case 0 :
			m_name		= "Hamstad Brick";
			m_rho		= 2005;
			m_cT		= 840;
			m_lambda	= 0.5;
			m_Opor		= 0.157;
			m_Oeff		= 0.157;
			m_isAir		= false;
			logKlSplineOl =
				"0                8.03206e-05      9.61229e-05      0.000115034      0.000137666      0.000164751      0.000197164      0.000235954      0.000282375      0.00033793       0.000404414      0.000483978      0.000579195      0.000693143      0.000829509      0.000992699      0.00118799       0.00142168       0.00170133       0.00203595       0.00243632       0.00291528       0.00348815       0.00417313       0.00499174       0.00596927       0.00713511       0.00852281       0.0101696        0.0121147        0.014396         0.017045         0.0200829        0.0235369        0.027555         0.0329148        0.042885         0.0670305        0.109759         0.140851         0.151343         0.154359         0.155487         0.156406         0.156622         0.157            ";
			logKlSplineLog10Kl =
				"-20.5031         -20.5031         -20.1831         -19.8631         -19.5431         -19.2231         -18.9031         -18.5831         -18.2631         -17.9431         -17.6231         -17.3031         -16.9831         -16.6631         -16.3431         -16.0231         -15.8054         -15.5378         -15.3401         -15.158          -14.636          -14.4122         -14.1042         -13.834          -13.4157         -13.2141         -13.0024         -12.7079         -12.5578         -12.4636         -12.3941         -12.2614         -12.0959         -12.0408         -11.8986         -11.8845         -11.773          -11.1372         -10.1465         -9.37365         -8.92297         -8.82647         -8.72144         -8.72144         -8.71975         -8.71975";

			break;

		// Hamstad Finishing Material
		case 1 :
			m_name		= "Hamstad Finishing Material";
			m_rho		= 790;
			m_cT		= 870;
			m_lambda	= 0.2;
			m_Opor		= 0.209;
			m_Oeff		= 0.209;
			m_isAir		= false;
			break;
		// invalid number
		default: m_i = -1;
	}
	if (!logKlSplineOl.empty()) {
		m_klSpline.read(logKlSplineOl, logKlSplineLog10Kl);
	}
}


double Material::Ol_pc(double pc) const {
	double Ol = 0;
	switch (m_i) {
		// Hamstad Brick
		case 0 :
		{
			const double reta[2] = {-1.25e-5, -1.8e-5};
			const double retn[2] = {1.65, 6};
			const double retm[2] = {0.39394, 0.83333};
			const double retw[2] = {0.3, 0.7};
			for (int i=0; i<2; ++i) {
				double tmp = IBK::f_pow(reta[i]*pc, retn[i]);
				double tmp2 = IBK::f_pow(1.0 + tmp, retm[i]);
				Ol += retw[i] / tmp2;
			}
			Ol *= m_Oeff;
		} break;

		// Hamstad Finishing Material
		case 1 :
		{
			double tmp = IBK::f_pow(-2e-6*pc, 1.27);
			double tmp2 = IBK::f_pow(1.0 + tmp, 0.21260);
			Ol = m_Oeff / tmp2;
		} break;
	}
	return Ol;
}


double Material::lambda_Ol(double Ol) const {
	switch (m_i) {
		// Hamstad Brick
		case 0 : return m_lambda + 4.5 * Ol; // 4.5
		// Hamstad Finishing Material
		case 1 : return m_lambda + 4.5 * Ol;
	}
	return 0.5;
}


double Material::Kl_Ol(double Ol) const {
	switch (m_i) {
		// Hamstad Brick
		case 0 : return IBK::f_pow10(m_klSpline.value(Ol));

		// Hamstad Finishing Material
		case 1 :
		{
			double tmp = (Ol-0.120)*1000;
			tmp = -33.0 + 7.04e-2*tmp - 1.742e-4*tmp*tmp - 2.7953e-6*tmp*tmp*tmp
				  -1.1566e-7*tmp*tmp*tmp*tmp + 2.5969e-9*tmp*tmp*tmp*tmp*tmp;
			tmp *= 0.4342944819032518;
			return IBK::f_pow10(tmp);
		} break;
	}
	return 0;
}


double Material::Kv_Ol(double T, double Ol) const {
	switch (m_i) {
		// Hamstad Brick
		case 0 :
		{
			const double mew = 30.0;
			double tmp = 1.0 - Ol/m_Oeff;
			double Kv = 2.61e-5/(461.89*T*mew) * tmp/(0.503*tmp*tmp + 0.497);
			return Kv;
		} break;

		// Hamstad Finishing Material
		case 1 :
		{
			const double mew = 3.0;
			double tmp = 1.0 - Ol/m_Oeff;
			double Kv = 2.61e-5/(461.89*T*mew) * tmp/(0.503*tmp*tmp + 0.497);
			return Kv;
		} break;
	}
	return 0;
}

