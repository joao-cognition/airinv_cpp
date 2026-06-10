// //////////////////////////////////////////////////////////////////////
// Import section
// //////////////////////////////////////////////////////////////////////
// STL
#include <sstream>
#include <string>
#include <tuple>
#include <algorithm>
#include <cctype>
// Boost
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/lexical_cast.hpp>
// StdAir
#include <stdair/stdair_json.hpp>
#include <stdair/stdair_basic_types.hpp>
#include <stdair/bom/TravelSolutionStruct.hpp>
#include <stdair/bom/ClassAvailabilityStruct.hpp>
// AirInv
#include <airinv/AIRINV_Master_Service.hpp>
#include <airinv/server/RestApiHandler.hpp>

namespace {

  // Parse query string into a map
  std::map<std::string, std::string>
  parseQueryString (const std::string& query) {
    std::map<std::string, std::string> params;
    std::istringstream iss (query);
    std::string pair;
    while (std::getline (iss, pair, '&')) {
      const auto eq = pair.find ('=');
      if (eq != std::string::npos) {
        params[pair.substr (0, eq)] = pair.substr (eq + 1);
      }
    }
    return params;
  }

  // Split a path by '/' into segments, skipping empty segments
  std::vector<std::string> splitPath (const std::string& path) {
    std::vector<std::string> segs;
    std::istringstream iss (path);
    std::string seg;
    while (std::getline (iss, seg, '/')) {
      if (!seg.empty()) segs.push_back (seg);
    }
    return segs;
  }

  // URL-decode a percent-encoded string
  std::string urlDecode (const std::string& src) {
    std::string ret;
    ret.reserve (src.size());
    for (std::size_t i = 0; i < src.size(); ++i) {
      if (src[i] == '%' && i + 2 < src.size()) {
        const auto hi = src[i + 1];
        const auto lo = src[i + 2];
        if (std::isxdigit (hi) && std::isxdigit (lo)) {
          ret += static_cast<char>(std::stoi (src.substr (i + 1, 2), nullptr, 16));
          i += 2;
          continue;
        }
      }
      ret += (src[i] == '+') ? ' ' : src[i];
    }
    return ret;
  }

  std::string jsonError (const std::string& msg) {
    return "{\"error\":\"" + msg + "\"}";
  }

}

namespace AIRINV {

  // //////////////////////////////////////////////////////////////////////
  RestApiHandler::RestApiHandler (AIRINV_Master_Service& iService)
    : _service (iService) {
  }

  // //////////////////////////////////////////////////////////////////////
  std::tuple<unsigned, std::string, std::string>
  RestApiHandler::handle (const std::string& method,
                          const std::string& target,
                          const std::string& body) const {

    const std::string kJson = "application/json";
    const std::string kText = "text/plain";

    // Split target into path and query
    std::string path = target;
    std::string queryStr;
    const auto qpos = target.find ('?');
    if (qpos != std::string::npos) {
      path = target.substr (0, qpos);
      queryStr = target.substr (qpos + 1);
    }
    const auto params = parseQueryString (queryStr);
    const auto segs = splitPath (path);

    try {

      // --- GET /api/v1/health ---
      if (method == "GET" && path == "/api/v1/health") {
        return {200, kJson, "{\"status\":\"ok\"}"};
      }

      // --- GET /api/v1/flight-dates ---
      // Optional query params: airline, flight
      if (method == "GET" && segs.size() == 3
          && segs[0] == "api" && segs[1] == "v1" && segs[2] == "flight-dates") {
        stdair::AirlineCode_T airline = "all";
        stdair::FlightNumber_T flight = 0;
        if (auto it = params.find ("airline"); it != params.end()) {
          airline = urlDecode (it->second);
        }
        if (auto it = params.find ("flight"); it != params.end()) {
          flight = boost::lexical_cast<stdair::FlightNumber_T>(it->second);
        }
        const auto result = _service.jsonExportFlightDateList (airline, flight);
        return {200, kJson, result};
      }

      // --- GET /api/v1/flight-dates/{airline}/{flight}/{date} ---
      if (method == "GET" && segs.size() == 6
          && segs[0] == "api" && segs[1] == "v1" && segs[2] == "flight-dates") {
        const stdair::AirlineCode_T airline = urlDecode (segs[3]);
        const auto flight =
          boost::lexical_cast<stdair::FlightNumber_T>(segs[4]);
        const stdair::Date_T depDate =
          boost::gregorian::from_simple_string (segs[5]);
        const auto result =
          _service.jsonExportFlightDateObjects (airline, flight, depDate);
        return {200, kJson, result};
      }

      // --- GET /api/v1/inventory/display ---
      if (method == "GET" && segs.size() >= 4
          && segs[0] == "api" && segs[1] == "v1" && segs[2] == "inventory"
          && segs[3] == "display") {
        std::string csvResult;
        if (params.count ("airline") && params.count ("flight")
            && params.count ("date")) {
          const stdair::AirlineCode_T airline =
            urlDecode (params.at ("airline"));
          const auto flight =
            boost::lexical_cast<stdair::FlightNumber_T>(
              params.at ("flight"));
          const stdair::Date_T depDate =
            boost::gregorian::from_simple_string (params.at ("date"));
          csvResult = _service.csvDisplay (airline, flight, depDate);
        } else {
          csvResult = _service.csvDisplay();
        }
        return {200, kText, csvResult};
      }

      // --- POST /api/v1/sell ---
      if (method == "POST" && path == "/api/v1/sell") {
        std::istringstream iss (body);
        boost::property_tree::ptree pt;
        boost::property_tree::read_json (iss, pt);
        const std::string segment = pt.get<std::string>("segment");
        const stdair::ClassCode_T classCode = pt.get<std::string>("class_code");
        const stdair::PartySize_T partySize = pt.get<int>("party_size");
        const bool ok = _service.sell (segment, classCode, partySize);
        const std::string resp = ok
          ? "{\"result\":\"sold\",\"segment\":\"" + segment + "\"}"
          : "{\"result\":\"rejected\",\"segment\":\"" + segment + "\"}";
        return {200, kJson, resp};
      }

      // --- POST /api/v1/cancel ---
      if (method == "POST" && path == "/api/v1/cancel") {
        std::istringstream iss (body);
        boost::property_tree::ptree pt;
        boost::property_tree::read_json (iss, pt);
        const std::string segment = pt.get<std::string>("segment");
        const stdair::ClassCode_T classCode = pt.get<std::string>("class_code");
        const stdair::PartySize_T partySize = pt.get<int>("party_size");
        const bool ok = _service.cancel (segment, classCode, partySize);
        const std::string resp = ok
          ? "{\"result\":\"cancelled\",\"segment\":\"" + segment + "\"}"
          : "{\"result\":\"rejected\",\"segment\":\"" + segment + "\"}";
        return {200, kJson, resp};
      }

      // --- POST /api/v1/availability ---
      // JSON body: {"segment":"BA,9,2011-06-10,LHR,SYD"}
      if (method == "POST" && path == "/api/v1/availability") {
        std::istringstream iss (body);
        boost::property_tree::ptree pt;
        boost::property_tree::read_json (iss, pt);
        const std::string segment = pt.get<std::string>("segment");

        // Build a travel solution from the segment key and let the
        // inventory service compute the per-class availabilities.
        stdair::TravelSolutionStruct lTravelSolution;
        lTravelSolution.addSegment (segment);
        _service.calculateAvailability (lTravelSolution);

        // Serialise the resulting class-availability maps.
        boost::property_tree::ptree root;
        root.put ("segment", segment);
        boost::property_tree::ptree availArray;
        const stdair::ClassAvailabilityMapHolder_T& lHolder =
          lTravelSolution.getClassAvailabilityMapHolder();
        for (const stdair::ClassAvailabilityStruct& lClassAvlStruct : lHolder) {
          boost::property_tree::ptree classMap;
          const stdair::ClassAvailabilityMap_T& lClassAvlMap =
            lClassAvlStruct.getClassAvailabilityMap();
          for (const auto& [lClassCode, lAvl] : lClassAvlMap) {
            classMap.put (lClassCode, lAvl);
          }
          availArray.push_back (std::make_pair ("", classMap));
        }
        root.add_child ("availabilities", availArray);
        std::ostringstream oss;
        boost::property_tree::write_json (oss, root);
        return {200, kJson, oss.str()};
      }

      // --- POST /api/v1/json-command ---
      if (method == "POST" && path == "/api/v1/json-command") {
        const stdair::JSONString lJSONCommandString (body);
        const auto result = _service.jsonHandler (lJSONCommandString);
        return {200, kJson, result};
      }

      // --- 404 ---
      return {404, kJson, jsonError ("Not found: " + path)};

    } catch (const std::exception& e) {
      return {500, kJson, jsonError (e.what())};
    }
  }

}
