// //////////////////////////////////////////////////////////////////////
// Import section
// //////////////////////////////////////////////////////////////////////
// STL
#include <cassert>
#include <sstream>
#include <version>
#if defined(__cpp_lib_format)
#include <format>
#endif
// AirInv
#include <airinv/basic/BasConst_AIRINV_Service.hpp>
#include <airinv/service/AIRINV_ServiceContext.hpp>

namespace AIRINV {

  // //////////////////////////////////////////////////////////////////////
  AIRINV_ServiceContext::AIRINV_ServiceContext()
    : _ownStdairService (false), _ownSEVMGRService (true), 
      _airlineCode (DEFAULT_AIRLINE_CODE) {
  }

  // //////////////////////////////////////////////////////////////////////
  AIRINV_ServiceContext::
  AIRINV_ServiceContext (const stdair::AirlineCode_T& iAirlineCode)
    : _ownStdairService (false), _ownSEVMGRService (true), 
      _airlineCode (iAirlineCode) {
  }

  // //////////////////////////////////////////////////////////////////////
  AIRINV_ServiceContext::AIRINV_ServiceContext (const AIRINV_ServiceContext&)
    : _ownStdairService (false), _ownSEVMGRService (true), 
      _airlineCode (DEFAULT_AIRLINE_CODE) {
  }

  // //////////////////////////////////////////////////////////////////////
  AIRINV_ServiceContext::~AIRINV_ServiceContext() {
  }
  
  // //////////////////////////////////////////////////////////////////////
  const std::string AIRINV_ServiceContext::shortDisplay() const {
#if defined(__cpp_lib_format)
    // Use {:d} so the bool formats as 0/1, matching the ostringstream
    // fallback below (operator<< without std::boolalpha prints 1/0).
    return std::format("AIRINV_ServiceContext[{}] -- Owns StdAir service: {:d}",
                       _airlineCode, _ownStdairService);
#else
    std::ostringstream oStr;
    oStr << "AIRINV_ServiceContext[" << _airlineCode
         << "] -- Owns StdAir service: " << _ownStdairService;
    return oStr.str();
#endif
  }

  // //////////////////////////////////////////////////////////////////////
  const std::string AIRINV_ServiceContext::display() const {
    std::ostringstream oStr;
    oStr << shortDisplay();
    return oStr.str();
  }

  // //////////////////////////////////////////////////////////////////////
  const std::string AIRINV_ServiceContext::describe() const {
    return shortDisplay();
  }

  // //////////////////////////////////////////////////////////////////////
  void AIRINV_ServiceContext::reset() {

    // The shared_ptr<>::reset() method drops the refcount by one.
    // If the count result is dropping to zero, the resource pointed to
    // by the shared_ptr<> will be freed.
    
    // Reset the stdair shared pointer
    _stdairService.reset();

    // Reset the rmol shared pointer
    _rmolService.reset();

    // Reset the airrac shared pointer
    _airracService.reset();

    // Reset the sevmgr shared pointer
    _sevmgrService.reset();
  }

}
