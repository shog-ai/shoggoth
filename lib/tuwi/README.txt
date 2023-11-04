===============================================================================
TUWI - TERMINAL UI FRAMEWORK
===============================================================================

Tuwi is a terminal user interface framework written in the C programming language, developed by Shoggoth Systems for use in the Shoggoth project (https://shoggoth.network).
Tuwi is the framework used for implementing the terminal user interface of Shoggoth, Camel, and other Shoggoth projects.

**WARNING**
This library is still early in development and should be considered unstable and experimental.



===============================================================================
EXAMPLES
===============================================================================

Examples can be found in /examples

You can run examples with:

$ E=<example_name> make run-example 

where <example_name> is the name of the example you want to run. For example:

$ E=simple make run-example



===============================================================================
BUILDING TUWI
===============================================================================

Supported platforms
================================================

Tuwi currently supports only Linux and macOS operating systems.

Requirements
================================================

* GNU Make


Building
================================================

Clone the repository
============================

$ git clone https://github.com/shoggoth-systems/tuwi

cd into the cloned directory
============================

$ cd tuwi


Build with make
============================

$ make build

The above command will build a static library into ./target/libtuwi.a which you can link with your project.



===============================================================================
USING TUWI
===============================================================================

To use Tuwi, include the header ./tuwi.h in your project, then link with the static library in ./target/libtuwi.a 

You can check the /examples directory for examples of how to use Tuwi or read the documentation below



===============================================================================
DOCUMENTATION
===============================================================================

Detailed documentation on how to use Tuwi can be found at https://shoggoth.network/explorer/docs/tuwi



===============================================================================
CONTRIBUTING
===============================================================================

Please follow the Shoggoth contribution guidelines at https://shoggoth.network/explorer/docs#contributing