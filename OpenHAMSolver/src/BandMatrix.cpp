#include "BandMatrix.h"

void BandMatrix::resize(unsigned int n, unsigned int mu, unsigned int ml, unsigned int smu) {
	m_n = n;
	m_mu = mu;
	m_ml = ml;
	m_smu = smu;
	m_colSize = smu + ml + 1;
	m_dataStorage.resize(n*m_colSize);
	m_data.resize(n);
	m_data[0] = &m_dataStorage[0];
	// store pointers to columns
	for (unsigned int i=1; i<n; ++i)
		m_data[i] = &m_dataStorage[0] + i*m_colSize;
	m_pivots.resize(n);
}


int BandMatrix::lu() {
	const char * const FUNC_ID = "[BandMatrix::lu]";
	if (m_n == 0)
		throw IBK::Exception("Matrix is empty.", FUNC_ID);
	unsigned int n_1 = static_cast<unsigned int>(m_n) - 1;
	if (m_pivots.size() != m_n)
		throw IBK::Exception(IBK::FormatString("Mismatching sizes of matrix (n = %1) and pivots vector (size = %2).")
							 .arg(m_n).arg((unsigned int)m_pivots.size()),
							 FUNC_ID);
	unsigned int *p = &m_pivots[0];
	double ** a = &m_data[0];

	// zero out the first smu - mu rows of the rectangular array a
	unsigned int num_rows = m_smu - m_mu;
	if (num_rows > 0) {
		for (unsigned int c=0; c<m_n; ++c) {
			double * a_c = a[c];
			for (unsigned int r=0; r<num_rows; ++r)
				a_c[r] = 0;
		}
	}

	// k = elimination step number
	for (unsigned int k=0; k<n_1; ++k, ++p) {
		double * col_k               = a[k];
		double * diag_k              = col_k + m_smu;
		double * sub_diag_k          = diag_k + 1;

		unsigned int last_row_k = std::min<unsigned int>(n_1,k+m_ml);

		// find l = pivot row number
		unsigned int l=k;
		double max = std::fabs(*diag_k);
		double * kptr = sub_diag_k;
		for (unsigned int i=k+1; i<=last_row_k; ++i, ++kptr)
			if (std::fabs(*kptr) > max) {
				l=i;
				max = std::fabs(*kptr);
			}

		unsigned int storage_l = l-k+m_smu;  // ROW(l,k,smu) : calculate row index in column
		*p = l;                             // store row of pivot element

		// check for zero pivot element
		if (col_k[storage_l] == 0) {
			return k+1;
		}

		// swap a(l,k) and a(k,k) if necessary
		bool swap = (l != k);
		if (swap)
			std::swap(col_k[storage_l], *diag_k);

		// Scale the elements below the diagonal in
		// column k by -1.0 / a(k,k). After the above swap,
		// a(k,k) holds the pivot element. This scaling
		// stores the pivot row multipliers -a(i,k)/a(k,k)
		// in a(i,k), i=k+1, ..., MIN(n-1,k+ml).

		double mult = -1.0 / (*diag_k);
		kptr = sub_diag_k;
		for (unsigned int i=k+1; i<=last_row_k; ++i, ++kptr)
			if (*kptr != 0)
				*kptr *= mult;

		// row_i = row_i - [a(i,k)/a(k,k)] row_k, i=k+1, ..., MIN(n-1,k+ml)
		// row k is the pivot row after swapping with row l.
		// The computation is done one column at a time,
		// column j=k+1, ..., MIN(k+smu,n-1).

		double * col_j;
		unsigned int last_col_k = std::min<unsigned int>(k+m_smu, n_1);
		unsigned int storage_k;
		double a_kj;
		for (unsigned int j=k+1; j<=last_col_k; ++j) {
			col_j = a[j];
			storage_l = l-j+m_smu; // ROW(l,j,smu)
			storage_k = k-j+m_smu; // ROW(k,j,smu)
			a_kj = col_j[storage_l];

			// Swap the elements a(k,j) and a(k,l) if l!=k.
			if (swap) {
				col_j[storage_l] = col_j[storage_k];
				col_j[storage_k] = a_kj;
			}

			// a(i,j) = a(i,j) - [a(i,k)/a(k,k)]*a(k,j)
			// a_kj = a(k,j), *kptr = - a(i,k)/a(k,k), *jptr = a(i,j)

			if (a_kj != 0.0) {
				kptr=sub_diag_k;
				// pointer to the element in row k+1 and column j: ROW(k+1,j,smu)
				double * jptr = col_j + k+1-j+m_smu;
				for (unsigned int i=k+1; i<=last_row_k; ++i, ++kptr, ++jptr)
					*jptr += a_kj * *kptr;
			}
		}
	}

	// set the last pivot row to be n-1 and check for a zero pivot
	*p = n_1;
	if (a[n_1][m_smu] == 0.0)
		return 2*m_n;

	return 0;
}


void BandMatrix::backsolve(double * b) const {
	// get direct access to data memory in vectors
	unsigned int n_1 = static_cast<unsigned int>(m_n)-1;
	IBK_ASSERT(m_pivots.size() == m_n);
	const unsigned int *p = &m_pivots[0];
	const double *const *a = &m_data[0];

	// Solve Ly = b, store solution y in b
	for (unsigned int k=0; k < n_1; ++k) {
		unsigned int l = p[k];
		double mult = b[l];
		if (l != k) {
			b[l] = b[k];
			b[k] = mult;
		}
		const double *diag_k = a[k]+m_smu;
		unsigned int last_row_k = std::min<unsigned int>(n_1,k+m_ml);
		for (unsigned int i=k+1; i<=last_row_k; ++i)
			b[i] += mult * diag_k[i-k];
	}

	// Solve Ux = y, store solution x in b
	for (int k=n_1; k>=0; --k) {
		const double * diag_k = a[k]+m_smu;      // pointer to the diagonal element of column/row k
		int first_row_k = std::max<int>(0, k-static_cast<int>(m_smu));
		b[k] /= (*diag_k);
		double mult = -b[k];
		for (int i=first_row_k; i<=k-1; ++i)
			b[i] += mult*diag_k[i-k];
	}
}


void BandMatrix::multiply(const double * b, double * r) const {
	// first initialize target vector with 0
	std::memset(r, 0, sizeof(double)*m_n);
	// now loop over all columns of the matrix
	for (unsigned int j=0; j<m_n; ++j) {
		// now we loop over all rows in this column and add the corresponding contribution
		// to the resultant vector
		for (unsigned int i=std::max<int>(0,j-m_mu); (int)i<=std::min<int>(m_n-1, j+m_ml); ++i) {
			r[i] += m_data[j][i - j + m_smu]*b[j];
		}
	}
}


void BandMatrix::write(std::ostream & out, double * b, bool matlabFormat,
					   unsigned int width) const
{
	if (width < 1)	width = 10;
	else 			--width; // subtract one for automatically padded " "
	if (matlabFormat) {
	}
	else {
		// screen format
		for (unsigned int i=0; i<m_n; ++i) {
			out << "[ ";
			for (unsigned int j=0; j<m_n; ++j) {
				out << std::setw(width) << std::right;
				if (j+m_ml<i || j>i+m_mu)
#define BAND_MATRIX_WRITE_ZEROS
#ifdef BAND_MATRIX_WRITE_ZEROS
					out << 0;
#else //  BAND_MATRIX_WRITE_ZEROS
					out << " ";
#endif //  BAND_MATRIX_WRITE_ZEROS
				else						out << operator()(i,j);
				out << " ";
			}
			out << " ]";
			if (b != NULL)
				out << "  [ " << std::setw(width) << std::right << b[i] << " ]";
			out << std::endl;
		}
	}
}
