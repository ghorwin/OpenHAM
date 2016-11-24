#include "Outputs.h"

#include <DataIO.h>
#include <IBK_FileUtils.h>
#include <IBK_crypt.h>

Outputs::Outputs(const Model * model) :
	m_model(model)
{
}


void Outputs::setupOutputFiles(const IBK::Path & outputRootPath) {
	const char * const FUNC_ID = "[Outputs::setupOutputFiles]";

	// create output directory if it does not exist yet
	if (!outputRootPath.exists())
		IBK::Path::makePath(outputRootPath);

	m_outputRootPath = outputRootPath;

	// populate geoFile with data and write geometry file
	writeGeofile();

	// create output files
	// we need files for:
	// surface conditions on either side: T, rh, pv
	// surface fluxes on either side: q, j_v, j_w, h_v, h_w
	// profiles: T, rh, pv, pvsat, w, w_hyg, w_ovhg

	// index 0 - temperature profile
	addProfileOutput("Temperature", "Temperature", "C");
	// index 1 - temperature profile
	addProfileOutput("RelativeHumidity", "Relative Humidity", "%");

	m_profileVector.resize(m_model->m_nElements);
}


void Outputs::appendOutputs() {

	// write profile outputs

	// index 0 - temperature
	m_profileVector.m_unit.set("K");
	m_profileVector.m_data = m_model->m_T;
	m_profileVector.convert(IBK::Unit("C"));
	std::string errmsg;
	m_dataIOs[0]->appendData(m_model->m_t, &m_profileVector.m_data[0], errmsg);

	// index 1 - relative humidity
	m_profileVector.m_unit.set("---");
	m_profileVector.m_data = m_model->m_rh;
	m_profileVector.convert(IBK::Unit("%"));
	m_dataIOs[1]->appendData(m_model->m_t, &m_profileVector.m_data[0], errmsg);

}


void Outputs::writeGeofile() {
	const char * const FUNC_ID = "[FUNC_ID]";
	// create new DATAIO::GeoFile instance
	DATAIO::GeoFile geofile;
	// specify file format (true for binary)
	geofile.m_isBinary = false;

	// *** Populate data tables ***

	// compose material data table (the vector materials is from your code)
	for (unsigned int i=0; i<m_model->m_materials.size(); ++i) {
		// GeoFile::Material is a class holding all information of a single material definition
		DATAIO::GeoFile::Material mat;
		mat.name  = m_model->m_materials[i].m_name; // some descriptive name
		//mat.color = ...; // some color, encoded as integer value
		mat.ID    = i; // unique material ID, referenced in elements table below
		// add material definition to list of materials in geofile
		geofile.m_matDefs.push_back(mat);
	}

	// compose element data table (the vector elements is from your code)
	double x = 0;
	for (unsigned int i=0; i<m_model->m_nElements; ++i) {
		// GeoFile::Element holds all element-specific information
		DATAIO::GeoFile::Element el;
		// specify material ID, this ID should be one of the IDs given to materials in
		// the geofile.m_matDefs vector.
		el.matnr = m_model->m_matIdx[i];
		// specify unique element number
		el.n = i;
		// specify grid coordinates (3D grid, coordinate system starting left-top-front)
		el.i = i;
		el.j = 0;
		el.k = 0;
		// specify element center coordinates (typically in [m], 3D grid, coordinate system
		// starting left-bottom-back)
		x += m_model->m_dx[i]*0.5;
		el.x = x;
		el.y = 0.5;
		el.z = 0.5;
		x += m_model->m_dx[i]*0.5;
		// add element information to vector with elements
		geofile.m_elementsVec.push_back(el);
	}

	// compose sides data table (vector sides is from your code)
	x = 0;
	for (unsigned int i=0; i<m_model->m_nElements+1; ++i) {
		// GeoFile::Side holds all side-specific information
		DATAIO::GeoFile::Side sid;
		// specify unique side number
		sid.n = i;
		// specify side orientation
		sid.orientation = 0; // x-direction
		// specify grid line coordinates (see above)
		sid.i = i;
		sid.j = 0;
		sid.k = 0;
		// specify grid line center coordinates (typically in [m], see above)
		sid.x = x;
		sid.y = 0.5;
		sid.z = 0.5;
		if (i< m_model->m_nElements)
			x += m_model->m_dx[i];
		// add side information to vector with sides
		geofile.m_sidesVec.push_back(sid);
	}

	// compose grid data, basically all column, row, stack widths
	geofile.m_grid.xwidths = m_model->m_dx;
	geofile.m_grid.ywidths = std::vector<double>(1,1);
	geofile.m_grid.zwidths = std::vector<double>(1,1);

	// *** Compose file name according to naming convention ***

	IBK::Path projectFilePath(m_model->m_projectFile);
	std::string projectContent  = IBK::file2String(projectFilePath);
	// suppose the string projectContent holds the relevant data of
	// the project file, at least material references, grid, and assignments

	// compute project file hash code
	unsigned int projectFileHashCode = IBK::SuperFastHash(projectContent);
	std::string projectFileHash = IBK::val2string<unsigned int>(projectFileHashCode);

	// generate relative path to geometry file
	IBK::Path projectFilenameWoExt  = projectFilePath.withoutExtension();
	IBK::Path geofileName = IBK::Path(projectFilenameWoExt.str() + "_" + projectFileHash);
	// append extension based on binary/ASCII format
	if (geofile.m_isBinary)		geofileName.addExtension(".g6b");
	else						geofileName.addExtension(".g6a");
	// and store relative path
	m_geoFileName = geofileName.filename().str();

	// store full path
	geofile.m_filename = m_outputRootPath / geofileName.filename();

	// *** write geometry file ***
	std::string errmsg;
	if (!geofile.write(errmsg, NULL)) {
		throw IBK::Exception(errmsg + "\nError writing geometry file.", FUNC_ID);
	}

	// compute hash code and remember geofileHash
	// for writing of DataIO containers
	m_geoFileHash = geofile.hashCode();
}


void Outputs::addProfileOutput(const std::string & fname,
							   const std::string & quantity,
							   const std::string & valueUnit)
{
	const char * const FUNC_ID = "[Outputs::addProfileOutput]";

	m_dataIOs.push_back(new DATAIO::DataIO);
	DATAIO::DataIO & d = *m_dataIOs.back();

	// specify filename
	d.m_filename = m_outputRootPath / (fname + ".d6o");
	// specify binary format (for ASCII set false)
	d.m_isBinary = false;
	// specify start year
	d.m_startYear = 2000;
	// set the file data type
	d.m_type = DATAIO::T_FIELD; // used for the keyword ELEMENTS or SIDES and in the header
	// store project file name (optional)
	d.m_projectFileName = m_model->m_projectFile;
	// store geometry file and hash of the geometry file content
	d.m_geoFileName = m_geoFileName;
	d.m_geoFileHash = m_geoFileHash;
	// store quantity information
	d.m_quantity = quantity;
	d.m_quantityKeyword = fname;
	// store data format
	d.m_spaceType = DATAIO::ST_SINGLE;
	d.m_timeType  = DATAIO::TT_NONE;
	// store IO units
	d.m_timeUnit  = "h";
	d.m_valueUnit = valueUnit;
	// store number of values and element numbers (or side numbers, if writing fluxes)
	std::vector<unsigned int> nums(m_model->m_nElements);
	for (unsigned int i=0; i<m_model->m_nElements; ++i)
		nums[i] = i;
	d.m_nums = nums;

	// finally write the file header
	std::string errmsg;
	bool success = d.writeHeader(errmsg);
	if (!success)
		throw IBK::Exception("Cannot write output file.", FUNC_ID);
}
