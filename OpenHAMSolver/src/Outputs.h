#ifndef OUTPUTS_H
#define OUTPUTS_H

#include "Model.h"

#include <DATAIO_DataIO.h>
#include <IBK_UnitVector.h>


/*! Handles outputs from model. */
class Outputs {
public:
	/*! Constructor, needs model to work. */
	Outputs(const Model * model);

	/*! Creates output files. */
	void setupOutputFiles(const IBK::Path & outputRootDir);

	/*! Appends outputs using current state of model. */
	void appendOutputs();

	/*! Writes geometry file. */
	void writeGeofile();

	/*! Adds a profile output. */
	void addProfileOutput(const std::string & fname,
						  const std::string & quantity,
						  const std::string & valueUnit);


	/*! Holds pointer to model instance (not owned). */
	const Model *	m_model;

	IBK::Path						m_outputRootPath;

	std::string						m_geoFileName;
	unsigned int					m_geoFileHash;

	/*! DataIO containers. */
	std::vector<DATAIO::DataIO*>	m_dataIOs;

	/*! Workspace vector for converting outputs and writing profiles. */
	IBK::UnitVector					m_profileVector;
};

#endif // OUTPUTS_H
