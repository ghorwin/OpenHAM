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

#include "Mesh.h"

#include <cmath>
#include <algorithm>
#include <stdexcept>


Mesh::Mesh(double density, double ratio) : m_density(density), m_ratio(ratio)
{
}


void Mesh::generate(int n, std::vector<double> & ds_vec) {
	double s = 0;
	ds_vec.resize(n);
	double A = std::sqrt(m_ratio);
	for (int i=1; i<n; ++i) {
		double xi = double(i)/n;
		double s_next;
		double u = 0.5*(1 + tanh(m_density*(xi-0.5))/tanh(m_density/2));
		s_next = u/(A + (1-A)*u);
		ds_vec[i-1] = s_next-s;
		s = s_next;
	}
	// calculate last element such that sum becomes = 1
	ds_vec[n-1] = 1 - s;
}


void Mesh::generate(const int n, const double d, std::vector<double> & dx_vec) {
	generate(n, dx_vec); 	// dx_vec contains normalized element widths ds
	// scale to absolute thickness
	for (int i=0; i<n; ++i)
		dx_vec[i] *= d;
}

