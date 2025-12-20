//#define CATCH_CONFIG_MAIN
#define CATCH_CONFIG_RUNNER

#include "catch2/catch.hpp"

#include "module_reg.h"
#include "mt_pcie_f.h"


int main( int argc, char* argv[] )
{
	Catch::Session session; // There must be exactly one instance

	std::string cfg;

	// Build a new parser on top of Catch's
	using namespace Catch::clara;
	auto cli 
		= session.cli() // Get Catch's composite command line parser
		| Opt( cfg, "cfg" ) // bind variable to a new option, with a hint string
		["-g"]["--cfg"]
		("config file");        // description string for the help output

	// Now pass the new composite back to Catch so it uses that
	session.cli( cli ); 

	// Let Catch (using Clara) parse the command line
	int returnCode = session.applyCommandLine( argc, argv );
	if( returnCode != 0 ) // Indicates a command line error
		return returnCode;

	if( cfg != "" ) {
		std::cout << "cfg file " << cfg << std::endl;

	}
	pcief_init();

	session.run();

	pcief_uninit();
}
