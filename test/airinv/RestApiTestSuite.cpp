/*!
 * \page RestApiTestSuite_cpp Command-Line Test to Demonstrate the AirInv REST API
 * \code
 */
// //////////////////////////////////////////////////////////////////////
// Import section
// //////////////////////////////////////////////////////////////////////
// STL
#include <sstream>
#include <fstream>
#include <string>
#include <tuple>
// Boost Unit Test Framework (UTF)
#define BOOST_TEST_DYN_LINK
#define BOOST_TEST_MAIN
#define BOOST_TEST_MODULE RestApiTestSuite
#include <boost/test/unit_test.hpp>
// StdAir
#include <stdair/basic/BasLogParams.hpp>
#include <stdair/service/Logger.hpp>
// Airinv
#include <airinv/AIRINV_Master_Service.hpp>
#include <airinv/server/RestApiHandler.hpp>

namespace boost_utf = boost::unit_test;

// (Boost) Unit Test XML Report
std::ofstream utfReportStream ("RestApiTestSuite_utfresults.xml");

/**
 * Configuration for the Boost Unit Test Framework (UTF)
 */
struct UnitTestConfig {
  /** Constructor. */
  UnitTestConfig() {
    boost_utf::unit_test_log.set_stream (utfReportStream);
#if BOOST_VERSION_MACRO >= 105900
    boost_utf::unit_test_log.set_format (boost_utf::OF_XML);
#else // BOOST_VERSION_MACRO
    boost_utf::unit_test_log.set_format (boost_utf::XML);
#endif // BOOST_VERSION_MACRO
    boost_utf::unit_test_log.set_threshold_level (boost_utf::log_test_units);
  }

  /** Destructor. */
  ~UnitTestConfig() {
  }
};

// //////////////////////////////////////////////////////////////////////
/**
 * Build an AIRINV master service with the default sample BOM tree and wrap
 * it into a REST API handler. The handler is exercised in-process (no socket
 * is opened), which keeps the test deterministic and fast while still
 * covering the full request-dispatch / JSON-serialisation path.
 */
struct RestApiFixture {
  RestApiFixture()
    : _logOutputFile ("RestApiTestSuite.log"),
      _logParams (stdair::LOG::DEBUG, _logOutputFile),
      _airinvService (_logParams),
      _handler (_airinvService) {
    _airinvService.buildSampleBom();
  }

  std::ofstream _logOutputFile;
  stdair::BasLogParams _logParams;
  AIRINV::AIRINV_Master_Service _airinvService;
  AIRINV::RestApiHandler _handler;
};

// /////////////// Main: Unit Test Suite //////////////

// Set the UTF configuration (re-direct the output to a specific file)
BOOST_GLOBAL_FIXTURE (UnitTestConfig);

// Start the test suite
BOOST_AUTO_TEST_SUITE (master_test_suite)

/**
 * Test the health-check endpoint.
 */
BOOST_AUTO_TEST_CASE (rest_api_health) {
  RestApiFixture lFixture;
  const auto [status, mime, body] =
    lFixture._handler.handle ("GET", "/api/v1/health", "");
  BOOST_CHECK_EQUAL (status, 200u);
  BOOST_CHECK_EQUAL (mime, "application/json");
  BOOST_CHECK (body.find ("ok") != std::string::npos);
}

/**
 * Test the flight-date listing endpoint.
 */
BOOST_AUTO_TEST_CASE (rest_api_flight_date_list) {
  RestApiFixture lFixture;
  const auto [status, mime, body] =
    lFixture._handler.handle ("GET", "/api/v1/flight-dates?airline=BA&flight=9",
                              "");
  BOOST_CHECK_EQUAL (status, 200u);
  BOOST_CHECK (body.find ("BA") != std::string::npos);
}

/**
 * Test the per-flight-date object export endpoint.
 */
BOOST_AUTO_TEST_CASE (rest_api_flight_date_object) {
  RestApiFixture lFixture;
  const auto [status, mime, body] =
    lFixture._handler.handle ("GET",
                              "/api/v1/flight-dates/BA/9/2011-06-10", "");
  BOOST_CHECK_EQUAL (status, 200u);
  BOOST_CHECK (body.find ("flight_date") != std::string::npos);
}

/**
 * Test the sell endpoint (POST with a JSON body).
 */
BOOST_AUTO_TEST_CASE (rest_api_sell) {
  RestApiFixture lFixture;
  const std::string lBody =
    "{\"segment\":\"BA,9,2011-06-10,LHR,SYD\","
    "\"class_code\":\"Q\",\"party_size\":2}";
  const auto [status, mime, body] =
    lFixture._handler.handle ("POST", "/api/v1/sell", lBody);
  BOOST_CHECK_EQUAL (status, 200u);
  BOOST_CHECK (body.find ("sold") != std::string::npos);
}

/**
 * Test the availability endpoint (POST with a JSON body).
 */
BOOST_AUTO_TEST_CASE (rest_api_availability) {
  RestApiFixture lFixture;
  const std::string lBody = "{\"segment\":\"BA,9,2011-06-10,LHR,SYD\"}";
  const auto [status, mime, body] =
    lFixture._handler.handle ("POST", "/api/v1/availability", lBody);
  BOOST_CHECK_EQUAL (status, 200u);
  BOOST_CHECK (body.find ("availabilities") != std::string::npos);
}

/**
 * Test the backward-compatible JSON command endpoint.
 */
BOOST_AUTO_TEST_CASE (rest_api_json_command) {
  RestApiFixture lFixture;
  const std::string lBody =
    "{\"list\":{\"airline_code\":\"BA\", \"flight_number\":9}}";
  const auto [status, mime, body] =
    lFixture._handler.handle ("POST", "/api/v1/json-command", lBody);
  BOOST_CHECK_EQUAL (status, 200u);
  BOOST_CHECK (body.find ("inventories") != std::string::npos);
}

/**
 * Test that an unknown route returns a 404.
 */
BOOST_AUTO_TEST_CASE (rest_api_not_found) {
  RestApiFixture lFixture;
  const auto [status, mime, body] =
    lFixture._handler.handle ("GET", "/api/v1/does-not-exist", "");
  BOOST_CHECK_EQUAL (status, 404u);
}

// End the test suite
BOOST_AUTO_TEST_SUITE_END()

/*!
 * \endcode
 */
