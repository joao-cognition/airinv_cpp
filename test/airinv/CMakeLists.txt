#
# AirInv Tests
#

##
# Specify each test suite configuration

# * InventoryTestSuite Test Suite
module_test_add_suite (airinv InventoryTestSuite InventoryTestSuite.cpp)

# * RestApiTestSuite Test Suite (exercises the REST API handler in-process).
#   The handler implementation lives with the server binary, so it is compiled
#   directly into the test executable.
module_test_add_suite (airinv RestApiTestSuite
  "RestApiTestSuite.cpp;${CMAKE_SOURCE_DIR}/airinv/server/RestApiHandler.cpp")


##
# Register all the test suites to be built and performed
module_test_build_all ()

