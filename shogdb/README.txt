===============================================================================
SHOGDB - Shoggoth Database
===============================================================================

ShogDB is an in-memory, key-value database written in the C programming language, developed by ShogAI for use in the Shoggoth project.
ShogDB is the database used by Shoggoth Nodes to store information like DHTs and pins. ShogDB also persists data on disk by periodically saving data to a file.

ShogDB uses string keys, with values of different types like:
* int
* uint
* float
* string
* bool
* JSON.

Applications can communicate with ShogDB via HTTP requests. ShogDB exposes the HTTP API at http://127.0.0.1:6961 by default.

**WARNING**
ShogDB is still early in development and should be considered unstable and experimental.


===============================================================================
USING SHOGDB
===============================================================================


Using the HTTP API
================================================


Using the C client library
================================================



Configuration
================================================

ShogDB requires a TOML configuration file ./dbconfig.toml located in the current working directory, and stores data on disk in a ./save.sdb file.



===============================================================================
BUILDING SHOGDB
===============================================================================

Supported platforms
================================================

ShogDB currently supports only Linux and macOS operating systems.

Building
================================================

Clone the repository
============================

$ git clone https://github.com/shog-ai/shogdb

cd into the cloned directory
============================

$ cd shogdb


Build with make
============================

$ make

The above command will build a binary into ./target/shogdb


===============================================================================
CONTRIBUTING
===============================================================================

Please follow the Shoggoth contribution guidelines at https://shoggoth.network/explorer/docs#contributing

