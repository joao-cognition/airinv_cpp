#ifndef __AIRINV_AIRINV_TYPES_HPP
#define __AIRINV_AIRINV_TYPES_HPP

// //////////////////////////////////////////////////////////////////////
// Import section
// //////////////////////////////////////////////////////////////////////
// STL
#include <map>
#include <memory>
#include <utility>
// Boost
#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
// StdAir
#include <stdair/stdair_exceptions.hpp>
#include <stdair/stdair_file.hpp>
#include <stdair/stdair_inventory_types.hpp>

namespace AIRINV {

  // Forward declarations
  class AIRINV_Service;
  class AIRINV_Master_Service;


  // ///////// Exceptions ///////////

  /**
   * The inventory input file can not be parsed.
   */
  class InventoryFileParsingFailedException
    : public stdair::ParsingFileFailedException {
  public:
    /**
     * Constructor.
     */
    InventoryFileParsingFailedException (const std::string& iWhat)
      : stdair::ParsingFileFailedException (iWhat) {}
  };

  /**
   * The schedule input file can not be parsed.
   */
  class ScheduleFileParsingFailedException
    : public stdair::ParsingFileFailedException {
  public:
    /**
     * Constructor.
     */
    ScheduleFileParsingFailedException (const std::string& iWhat)
      : stdair::ParsingFileFailedException (iWhat) {}
  };

  /**
   * Missing partner flight date within the schedule file.
   */
  class MissingPartnerFlightDateWithinScheduleFile
    : public ScheduleFileParsingFailedException {
  public:
    /**
     * Constructor.
     */
    MissingPartnerFlightDateWithinScheduleFile (const std::string& iWhat)
      : ScheduleFileParsingFailedException (iWhat) {}
  };

  /**
   * The FRAT5 input file can not be parsed.
   */
  class FRAT5FileParsingFailedException
    : public stdair::ParsingFileFailedException {
  public:
    /**
     * Constructor.
     */
    FRAT5FileParsingFailedException (const std::string& iWhat)
      : stdair::ParsingFileFailedException (iWhat) {}
  };

  /**
   * The FFDisutility input file can not be parsed.
   */
  class FFDisutilityFileParsingFailedException
    : public stdair::ParsingFileFailedException {
  public:
    /**
     * Constructor.
     */
    FFDisutilityFileParsingFailedException (const std::string& iWhat)
      : stdair::ParsingFileFailedException (iWhat) {}
  };
  
  /**
   * Specific exception when some BOM objects can not be found within
   * the inventory.
   */
  class SegmentDateNotFoundException : public stdair::ParserException {
  public:
    /**
     * Constructor.
     */
    SegmentDateNotFoundException (const std::string& iWhat)
      : stdair::ParserException (iWhat) {}
  };

  /**
   * The inventory input file can not be found or opened.
   */
  class InventoryInputFileNotFoundException : public stdair::FileNotFoundException {
  public:
    /**
     * Constructor.
     */
    InventoryInputFileNotFoundException (const std::string& iWhat)
      : stdair::FileNotFoundException (iWhat) {}
  };

  /**
   * The schedule input file can not be found or opened.
   */
  class ScheduleInputFileNotFoundException : public stdair::FileNotFoundException {
  public:
    /**
     * Constructor.
     */
    ScheduleInputFileNotFoundException (const std::string& iWhat)
      : stdair::FileNotFoundException (iWhat) {}
  };

  /**
   * The FRAT5 input file can not be found or opened.
   */
  class FRAT5InputFileNotFoundException : public stdair::FileNotFoundException {
  public:
    /**
     * Constructor.
     */
    FRAT5InputFileNotFoundException (const std::string& iWhat)
      : stdair::FileNotFoundException (iWhat) {}
  };

  /**
   * The FF disutility input file can not be found or opened.
   */
  class FFDisutilityInputFileNotFoundException : public stdair::FileNotFoundException {
  public:
    /**
     * Constructor.
     */
    FFDisutilityInputFileNotFoundException (const std::string& iWhat)
      : stdair::FileNotFoundException (iWhat) {}
  };

  /**
   * Duplicated flight date object.
   */
  class FlightDateDuplicationException : public stdair::ObjectCreationgDuplicationException {
  public:
    /**
     * Constructor.
     */
    FlightDateDuplicationException (const std::string& iWhat) 
      : stdair::ObjectCreationgDuplicationException (iWhat) {} 
  };

  /**
   * Specific exception related to bookings made against the inventory.
   */
  class BookingException : public stdair::RootException {
  };

  /**
   * Inventory not found.
   */
  class InventoryNotFoundException : public stdair::ObjectNotFoundException {
  public:
    /**
     * Constructor.
     */
    InventoryNotFoundException (const std::string& iWhat)
      : stdair::ObjectNotFoundException (iWhat) {}
  };

  /**
   * Flight Date not found
   */
  class FlightDateNotFoundException : public stdair::ObjectNotFoundException {
  public:
    /**
     * Constructor.
     */
    FlightDateNotFoundException (const std::string& iWhat)
      : stdair::ObjectNotFoundException (iWhat) {}
  };


  // //////// Type definitions /////////
  /**
   * Inventory input file.
   */
  class InventoryFilePath : public stdair::InputFilePath {
  public:
    /**
     * Constructor.
     */
    explicit InventoryFilePath (const stdair::Filename_T& iFilename)
      : stdair::InputFilePath (iFilename) {}
  };
  
  /**
   * Smart-pointer compatibility layer.
   *
   * By default (legacy mode), the public service pointers are aliases on
   * boost::shared_ptr, so that downstream repositories (e.g. simcrs_cpp,
   * tvlsim_cpp) that store an AIRINV_Master_ServicePtr_T in their own service
   * context keep building unchanged.
   *
   * Downstream consumers can migrate at their own pace to std::shared_ptr by
   * compiling with -DAIRINV_USE_STD_SMART_PTR.
   */
#ifndef AIRINV_USE_STD_SMART_PTR
  // Legacy mode --- boost::shared_ptr (default for backward compatibility)
  /**
   * (Smart) Pointer on the AirInv (slave) service handler.
   */
  typedef boost::shared_ptr<AIRINV_Service> AIRINV_ServicePtr_T;

  /**
   * (Smart) Pointer on the AirInv master service handler.
   */
  typedef boost::shared_ptr<AIRINV_Master_Service> AIRINV_Master_ServicePtr_T;
#else
  // Modern mode --- std::shared_ptr
  /**
   * (Smart) Pointer on the AirInv (slave) service handler.
   */
  typedef std::shared_ptr<AIRINV_Service> AIRINV_ServicePtr_T;

  /**
   * (Smart) Pointer on the AirInv master service handler.
   */
  typedef std::shared_ptr<AIRINV_Master_Service> AIRINV_Master_ServicePtr_T;
#endif // AIRINV_USE_STD_SMART_PTR

  /**
   * Construct an AIRINV_ServicePtr_T using whichever smart-pointer
   * implementation is selected above. Internal factory code uses this helper
   * so it stays correct under both modes -- in particular, it avoids mixing
   * boost::make_shared with the std::shared_ptr typedef (or vice versa), which
   * would otherwise fail to compile when AIRINV_USE_STD_SMART_PTR is defined.
   *
   * Defined as a template so AIRINV_Service only needs to be complete at the
   * call site, not here.
   */
  template<typename... Args>
  inline AIRINV_ServicePtr_T makeAirinvServicePtr (Args&&... iArgs) {
#ifndef AIRINV_USE_STD_SMART_PTR
    return boost::make_shared<AIRINV_Service> (std::forward<Args> (iArgs)...);
#else
    return std::make_shared<AIRINV_Service> (std::forward<Args> (iArgs)...);
#endif
  }
  
  /**
   * Type defining a map of airline codes and the corresponding
   * airline inventories.
   */
  typedef std::map<const stdair::AirlineCode_T,
                   AIRINV_ServicePtr_T> AIRINV_ServicePtr_Map_T;

  /**
   * Define the FRAT5 curve.
   */
  typedef std::map<const stdair::DTD_T, double> FRAT5Curve_T;

}
#endif // __AIRINV_AIRINV_TYPES_HPP
