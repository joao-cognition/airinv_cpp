#ifndef __AIRINV_SVR_RESTAPIHANDLER_HPP
#define __AIRINV_SVR_RESTAPIHANDLER_HPP

// //////////////////////////////////////////////////////////////////////
// Import section
// //////////////////////////////////////////////////////////////////////
// STL
#include <string>
#include <tuple>
#include <mutex>

namespace AIRINV {

  // Forward declarations
  class AIRINV_Master_Service;

  /**
   * @brief REST API handler mapping HTTP requests to AIRINV_Master_Service
   *        method calls.
   *
   * Endpoints:
   *   GET  /api/v1/health
   *   GET  /api/v1/flight-dates?airline=XX&flight=NNN
   *   GET  /api/v1/flight-dates/{airline}/{flight}/{date}
   *   GET  /api/v1/inventory/display?airline=XX&flight=NNN&date=YYYY-MM-DD
   *   POST /api/v1/sell           (JSON body)
   *   POST /api/v1/cancel         (JSON body)
   *   POST /api/v1/json-command   (JSON body)
   */
  class RestApiHandler {
  public:
    RestApiHandler (const RestApiHandler&) = delete;
    RestApiHandler& operator= (const RestApiHandler&) = delete;

    explicit RestApiHandler (AIRINV_Master_Service& iService);

    /**
     * Handle an HTTP request and produce a response.
     *
     * @param method  HTTP method (GET, POST, …)
     * @param target  Full request target including query string
     * @param body    Request body (for POST)
     * @return tuple of (HTTP status code, content-type, response body)
     */
    [[nodiscard]] std::tuple<unsigned, std::string, std::string>
    handle (const std::string& method,
            const std::string& target,
            const std::string& body) const;

  private:
    AIRINV_Master_Service& _service;

    /**
     * Serialises access to the (non-thread-safe) AIRINV_Master_Service.
     * The REST server dispatches each connection to its own thread, so every
     * call into _service must hold this lock to avoid racing on the BOM tree.
     * Mutable so it can be locked from the const handle() method.
     */
    mutable std::mutex _serviceMutex;
  };

}
#endif // __AIRINV_SVR_RESTAPIHANDLER_HPP
