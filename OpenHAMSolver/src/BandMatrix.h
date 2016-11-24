#ifndef BandMatrixH
#define BandMatrixH

#include <vector>
#include <stdexcept>
#include <numeric>
#include <utility>
#include <cmath>
#include <cstring>

#include <IBK_assert.h>

/*! A banded matrix implementation.

	The implementation is aimed at easy and intuitive usage of the
	matrix and internal storage. A decomposition and backsolving
	algorithm is also provided. When creating or resizing a band
	matrix you need to pass the number of elements in the matrix n and
	the number of upper and lower off-diagonal elements mu and ml.
	If you are using the decomposition algorithm you have to ensure
	that there is enough storage space available, because the pivoting
	requires a larger bandwidth than the original matrix has. The
	parameter smu gives the storage upper bandwidth used for
	the storage of the decomposed matrix and is calculated as in the
	following example:
	\code
	// Defined size of matrix and number of off-diagonal elements
	unsigned int n = 10;
	unsigned int mu = 3;	// 3 upper off-diagonal elements
	unsigned int ml = 2;	// 2 lower off-diagonal elements
	// Compute the storage upper bandwidth needed for decomposition.
	// If decomposition is not used, set msu = mu
	unsigned int smu = std::min(n - 1, mu + ml);
	// Now create a banded matrix
	BandMatrix mat(n, mu, ml, smu);
	// or resize an existing matrix object
	mat.resize(n, mu, ml, msu);
	\endcode
*/
class BandMatrix {
public:
	/*! Default constructor (creates an empty matrix). */
	BandMatrix() : m_n(0), m_mu(0), m_ml(0), m_smu(0), m_colSize(0) {}

	/*! Constructor, see class documentation for description of parameters. */
	BandMatrix(unsigned int n, unsigned int mu, unsigned int ml, unsigned int smu) {
		resize(n, mu, ml, smu);
	}

	/*! Copy-Constructor. */
	BandMatrix(const BandMatrix & other) { *this=other; }

	/*! Assignment operator. */
	const BandMatrix & operator=(const BandMatrix & other) {
		resize(other.m_n, other.m_mu, other.m_ml, other.m_smu);
		// copy data from other matrix
		std::memcpy(m_data[0], other.m_data[0], sizeof(double)*m_n*m_colSize);
		// also copy pivots
		std::copy(other.m_pivots.begin(), other.m_pivots.end(), m_pivots.begin());
		return *this;
	}

	/*! Returns a reference to the element at the coordinates [row, col]. */
	double &  operator() (unsigned int row, unsigned int col) { return m_data[col][row-col+m_smu]; }

	/*! Returns a constant reference to the element at the coordinates [col,row]. */
	double operator() (unsigned int row, unsigned int col) const {
		IBK_ASSERT(col < m_n);
		IBK_ASSERT(row < m_n);
		if (col+m_ml<row || col>row+m_mu)	return 0.0;
		else								return m_data[col][row-col+m_smu];
	}

	/*! Resizes the matrix (all data will be lost). */
	void resize(unsigned int n, unsigned int mu, unsigned int ml, unsigned int smu);

	/*! Returns the number of rows/cols in the matrix. */
	unsigned int  n() const { return m_n; }

	/*! Returns the upper band width. */
	unsigned int  mu() const { return m_mu; }

	/*! Returns the lower band width. */
	unsigned int  ml() const { return m_ml; }

	/*! Returns the extended upper band width. */
	unsigned int  smu() const { return m_smu; }

	/*! Clears the matrix. */
	void clear() {
		m_data.clear();
		m_dataStorage.clear();
		m_pivots.clear();
		m_n = m_mu = m_ml = m_smu = 0;
	}

	/*! Fills the matrix with zeros. */
	void setZero() { std::memset(m_data[0], 0, m_n*m_colSize*sizeof(double)); }
	/*! Returns the raw data. */
	std::vector<double>& data() { return m_dataStorage; }

	/*! Performs an in-place LU factorization of the matrix with partial pivoting.
		\return Returns 0 if successful.
			- If during the first elimination step a column is found to be completely zero
			(and thus also the pivot element), the column number (1..m_n) is returned.
			- If in the second step the pivot element becomes zero, the function returns the
			row of that element as value between m_n...2*m_n-1.
			- If the final pivot element in cell (m_n-1, m_n-1) is zero, the function returns 2*m_n.
	*/
	int lu();

	/*! Solves the equation system: Ax = b, using the 'pivots'
		from the factorization, the solution is stored in the vector 'b'.
	*/
	void backsolve(double * b) const;

	/*! Computes r = A*b.
		\param b Vector of size n() (read-only).
		\param r Vector of size n(), holds results.
	*/
	void multiply(const double * b, double * r) const;

	/*! Writes the matrix and optionally a right-hand-side vector to output stream or file. */
	void write(std::ostream & out, double * b = NULL, bool matlabFormat = false,
			   unsigned int width = 4) const;

private:
	/*! Dimension of the matrix. */
	unsigned int m_n;
	/*! Size of the upper diagonal band (without center diagonal). */
	unsigned int m_mu;
	/*! Size of the lower diagonal band (without center diagonal). */
	unsigned int m_ml;
	/*! Size of the upper diagonal band including space for factorisation. */
	unsigned int m_smu;
	/*! Number of elements per storage column. */
	unsigned int m_colSize;
	/*! This vector of size n contains pointers to the individual columns of the matrix.
		The first element points to the first column, and thus also to the begin
		of the whole linear memory array of size n*m_colSize.
	*/
	std::vector<double*>		m_data;
	/*! This vector holds the actual data. */
	std::vector<double>			m_dataStorage;
	/*! Pivot vector for lu() and backsolve() functions. */
	std::vector<unsigned int>	m_pivots;
};

#endif // BandMatrixH
