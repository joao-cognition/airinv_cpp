/*!
 * \page AirInvRestServer.cpp   Modern REST API Server for AirInv
 *
 * Uses Boost.Beast for HTTP/1.1 and delegates to RestApiHandler which
 * wraps AIRINV_Master_Service methods as REST endpoints.
 *
 * Usage:
 *   AirInvRestServer -b -p 8080          # built-in sample BOM on port 8080
 *   AirInvRestServer -i invdump01.csv    # from inventory dump
 * \code
 */
// //////////////////////////////////////////////////////////////////////
// Import section
// //////////////////////////////////////////////////////////////////////
// STL
#include <cassert>
#include <sstream>
#include <fstream>
#include <string>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>
// Boost
#include <boost/program_options.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/signal_set.hpp>
// StdAir
#include <stdair/basic/BasLogParams.hpp>
#include <stdair/basic/BasDBParams.hpp>
#include <stdair/service/Logger.hpp>
#include <stdair/stdair_json.hpp>
// AirInv
#include <airinv/config/airinv-paths.hpp>
#include <airinv/AIRINV_Master_Service.hpp>
#include <airinv/server/RestApiHandler.hpp>

namespace beast = boost::beast;
namespace http  = beast::http;
namespace net   = boost::asio;
using tcp       = net::ip::tcp;

// //////// Constants //////
static const std::string K_DEFAULT_LOG ("airinvRestServer.log");
static const unsigned short K_DEFAULT_PORT = 8080;
static const std::string K_DEFAULT_ADDR = "0.0.0.0";

static const std::string K_DEFAULT_INVENTORY (STDAIR_SAMPLE_DIR "/invdump01.csv");
static const std::string K_DEFAULT_SCHEDULE  (STDAIR_SAMPLE_DIR "/schedule01.csv");
static const std::string K_DEFAULT_OND       (STDAIR_SAMPLE_DIR "/ond01.csv");
static const std::string K_DEFAULT_FRAT5     (STDAIR_SAMPLE_DIR "/frat5.csv");
static const std::string K_DEFAULT_FF_DISUTILITY (STDAIR_SAMPLE_DIR "/ffDisutility.csv");
static const std::string K_DEFAULT_YIELD     (STDAIR_SAMPLE_DIR "/yield01.csv");
static const int K_EARLY_RETURN = 99;

// //////// CLI parsing ////////
int readConfiguration (int argc, char* argv[],
                       std::string& ioAddr, unsigned short& ioPort,
                       bool& ioIsBuiltin, bool& ioIsForSchedule,
                       std::string& ioInvFile, std::string& ioSchedFile,
                       std::string& ioONDFile, std::string& ioFRAT5File,
                       std::string& ioFFDisFile, std::string& ioYieldFile,
                       std::string& ioLogFile) {
  namespace po = boost::program_options;
  ioIsBuiltin = false;
  ioIsForSchedule = false;

  po::options_description desc ("AirInvRestServer options");
  desc.add_options()
    ("help,h", "Show help")
    ("version,v", "Print version")
    ("builtin,b", "Use built-in sample BOM")
    ("for_schedule,f", "Build BOM from schedule file")
    ("inventory,i", po::value<std::string>(&ioInvFile)->default_value (K_DEFAULT_INVENTORY), "Inventory dump CSV")
    ("schedule,s",  po::value<std::string>(&ioSchedFile)->default_value (K_DEFAULT_SCHEDULE), "Schedule CSV")
    ("ond,o",       po::value<std::string>(&ioONDFile)->default_value (K_DEFAULT_OND), "O&D CSV")
    ("frat5,r",     po::value<std::string>(&ioFRAT5File)->default_value (K_DEFAULT_FRAT5), "FRAT5 CSV")
    ("ff_disutility,d", po::value<std::string>(&ioFFDisFile)->default_value (K_DEFAULT_FF_DISUTILITY), "FF disutility CSV")
    ("yield,y",     po::value<std::string>(&ioYieldFile)->default_value (K_DEFAULT_YIELD), "Yield CSV")
    ("address,a",   po::value<std::string>(&ioAddr)->default_value (K_DEFAULT_ADDR), "Bind address")
    ("port,p",      po::value<unsigned short>(&ioPort)->default_value (K_DEFAULT_PORT), "Bind port")
    ("log,l",       po::value<std::string>(&ioLogFile)->default_value (K_DEFAULT_LOG), "Log file")
    ;
  po::variables_map vm;
  po::store (po::parse_command_line (argc, argv, desc), vm);
  po::notify (vm);

  if (vm.count ("help")) { std::cout << desc << "\n"; return K_EARLY_RETURN; }
  if (vm.count ("version")) { std::cout << PACKAGE_NAME << " " << PACKAGE_VERSION << "\n"; return K_EARLY_RETURN; }
  if (vm.count ("builtin")) ioIsBuiltin = true;
  if (vm.count ("for_schedule")) ioIsForSchedule = true;
  return 0;
}

// //////// Single-request Beast session (synchronous, one-shot) ////////
static void doSession (tcp::socket sock,
                       const AIRINV::RestApiHandler& handler) {
  try {
    beast::flat_buffer buf;
    http::request<http::string_body> req;
    http::read (sock, buf, req);

    const std::string method = req.method_string().to_string();
    const std::string target = req.target().to_string();
    const std::string& body  = req.body();

    // CORS preflight: browsers send an OPTIONS request before a cross-origin
    // POST with a JSON content type. Answer it directly with 204 + the allowed
    // methods/headers so browser/SPA clients are not blocked. The actual
    // requests still carry Access-Control-Allow-Origin (set below).
    if (method == "OPTIONS") {
      http::response<http::empty_body> res;
      res.version (req.version());
      res.result (http::status::no_content);
      res.set (http::field::server, "AirInvRestServer/" PACKAGE_VERSION);
      res.set (http::field::access_control_allow_origin, "*");
      res.set (http::field::access_control_allow_methods, "GET, POST, OPTIONS");
      res.set (http::field::access_control_allow_headers, "Content-Type");
      res.set (http::field::access_control_max_age, "86400");
      res.prepare_payload();
      http::write (sock, res);
      sock.shutdown (tcp::socket::shutdown_send);
      return;
    }

    const auto [statusCode, contentType, respBody] =
      handler.handle (method, target, body);

    http::response<http::string_body> res;
    res.version (req.version());
    res.result (static_cast<http::status>(statusCode));
    res.set (http::field::server, "AirInvRestServer/" PACKAGE_VERSION);
    res.set (http::field::content_type, contentType);
    res.set (http::field::access_control_allow_origin, "*");
    res.set (http::field::access_control_allow_methods, "GET, POST, OPTIONS");
    res.set (http::field::access_control_allow_headers, "Content-Type");
    res.body() = respBody;
    res.prepare_payload();

    http::write (sock, res);
    sock.shutdown (tcp::socket::shutdown_send);

  } catch (const beast::system_error& e) {
    if (e.code() != http::error::end_of_stream)
      std::cerr << "Session error: " << e.what() << "\n";
  } catch (const std::exception& e) {
    std::cerr << "Session error: " << e.what() << "\n";
  }
}

// ////////////////////// M A I N ////////////////////////
int main (int argc, char* argv[]) {

  std::string addr;
  unsigned short port;
  bool isBuiltin, isForSchedule;
  std::string invFile, schedFile, ondFile, frat5File, ffDisFile, yieldFile, logFile;

  const int rc = readConfiguration (argc, argv, addr, port,
                                    isBuiltin, isForSchedule,
                                    invFile, schedFile, ondFile,
                                    frat5File, ffDisFile, yieldFile,
                                    logFile);
  if (rc == K_EARLY_RETURN) return 0;

  // Logger
  std::ofstream logOut (logFile);
  logOut.clear();
  const stdair::BasLogParams lLogParams (stdair::LOG::DEBUG, logOut);

  // Inventory service
  AIRINV::AIRINV_Master_Service airinvService (lLogParams);

  if (isBuiltin) {
    airinvService.buildSampleBom();
    std::cout << "Built sample BOM (built-in)\n";
  } else if (isForSchedule) {
    stdair::ScheduleFilePath lSched (schedFile);
    stdair::ODFilePath lOND (ondFile);
    stdair::FRAT5FilePath lFRAT5 (frat5File);
    stdair::FFDisutilityFilePath lFFDis (ffDisFile);
    AIRRAC::YieldFilePath lYield (yieldFile);
    airinvService.parseAndLoad (lSched, lOND, lFRAT5, lFFDis, lYield);
    std::cout << "Loaded schedule from " << schedFile << "\n";
  } else {
    AIRINV::InventoryFilePath lInv (invFile);
    airinvService.parseAndLoad (lInv);
    std::cout << "Loaded inventory from " << invFile << "\n";
  }

  // REST handler
  AIRINV::RestApiHandler handler (airinvService);

  // Start Beast acceptor.
  //
  // The acceptor and the signal_set are both driven exclusively by this single
  // io_context, which is run on the main thread (ioc.run() below). Keeping every
  // operation on the acceptor (async_accept) and the shutdown (acceptor.close()
  // from the signal handler) on the same single-threaded io_context avoids the
  // data race that a synchronous accept() on one thread + close() on another
  // would otherwise create (Boost.Asio: a socket/acceptor is not safe for
  // concurrent use).
  net::io_context ioc {1};
  tcp::acceptor acceptor {ioc, {net::ip::make_address (addr), port}};

  // Count in-flight sessions so we can wait for them to finish before the
  // service objects (handler, airinvService) go out of scope at the end of
  // main(); otherwise a still-running detached session would dereference
  // freed memory.
  std::atomic<int> activeSessions {0};

  // Graceful shutdown on SIGINT/SIGTERM: close the acceptor, which completes the
  // pending async_accept with operation_aborted, so the io_context drains and
  // ioc.run() returns. This runs on the io_context thread, same as the acceptor.
  net::signal_set signals (ioc, SIGINT, SIGTERM);
  signals.async_wait ([&](const beast::error_code&, int) {
    beast::error_code ec;
    acceptor.close (ec);
  });

  // Recursive asynchronous accept loop. Each accepted connection is handed off
  // to its own detached worker thread running the synchronous doSession(); the
  // loop then re-arms itself. All acceptor access stays on the io_context thread.
  std::function<void()> doAccept;
  doAccept = [&]() {
    acceptor.async_accept ([&](const beast::error_code& ec, tcp::socket sock) {
      if (ec) {
        // operation_aborted on shutdown (acceptor closed), or a genuine error.
        return;
      }
      ++activeSessions;
      std::thread ([s = std::move (sock), &handler, &activeSessions]() mutable {
        doSession (std::move (s), handler);
        --activeSessions;
      }).detach();
      doAccept();
    });
  };
  doAccept();

  std::cout << "AirInvRestServer listening on http://"
            << addr << ":" << port << "/api/v1/health\n";

  // Run the io_context on the main thread until the acceptor is closed.
  ioc.run();

  // Wait for any in-flight sessions to drain before destroying the service.
  while (activeSessions.load() > 0) {
    std::this_thread::sleep_for (std::chrono::milliseconds (10));
  }

  std::cout << "Server stopped.\n";
  return 0;
}
