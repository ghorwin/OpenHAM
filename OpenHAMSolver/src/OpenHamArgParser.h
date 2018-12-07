#ifndef OpenHAMArgParserH
#define OpenHAMArgParserH

#include <IBK_SolverArgsParser.h>

/*! Command line parser class, extends the generic solver args parser class with custom
	functionality (actually removes arguments not supported by OpenHAM.
*/
class OpenHAMArgParser : public IBK::SolverArgsParser {
public:
	/*! Constructor. */
	OpenHAMArgParser();

	/*! Prints help page to std-output.
		\param out	Stream buffer of the help output.
	*/
	virtual void printHelp(std::ostream & out) const;

	/*! Prints man page to std-output.
		\param out	Stream buffer of the manual output.
	*/
	virtual void printManPage(std::ostream & out) const;
};

#endif // OpenHAMArgParserH
