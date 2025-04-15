#include <boost/system/error_code.hpp>
#include <iostream>
#include <string>

/*
Look for template here:
https://github.com/boostorg/outcome/blob/master/doc/src/snippets/boost-only/error_code_registration.cpp#L32
*/

enum class torrent_errc
{
    success = 0, // 0 should not represent an error
    block_request_timeout = 1,
    opening_file_error = 2,
    seeking_position_error = 3,
    writing_error = 4
};

namespace boost
{
    namespace system
    {
        // Tell the C++ 11 STL metaprogramming that enum torrent_errc
        // is registered with the standard error code system
        template <>
        struct is_error_code_enum<torrent_errc> : std::true_type
        {
        };

        // inline error_code make_error_code(utils::torrent_errc e)
        // {
        // return {static_cast<int>(e), torrent_error()};
        //  return error_code(static_cast<int>(e), generic_category());
        //}
    } // namespace system
} // namespace boost

namespace utils
{

    class torrent_error : public boost::system::error_category
    {
    public:
        // Return a short descriptive name for the category
        virtual const char *name() const noexcept override final { return "Torrent Error"; }
        // Return what each enum means in text
        virtual std::string message(int c) const override final
        {
            switch (static_cast<torrent_errc>(c))
            {
            case torrent_errc::success:
                return "conversion successful";
            case torrent_errc::block_request_timeout:
                return "block was not received in set time";
            case torrent_errc::opening_file_error:
                return "file can not be opened";
            case torrent_errc::seeking_position_error:
                return "could not seek to position in file";
            case torrent_errc::writing_error:
                return "failed to write data to file";
            default:
                return "unknown";
            }
        }
        // OPTIONAL: Allow generic error conditions to be compared to me
        virtual boost::system::error_condition default_error_condition(int c) const noexcept override final
        {
            switch (static_cast<torrent_errc>(c))
            {
            case torrent_errc::block_request_timeout:
                return make_error_condition(boost::system::errc::bad_message);
            default:
                // I have no mapping for this code
                return boost::system::error_condition(c, *this);
            }
        }
    };
} // namespace utils



// Define the linkage for this function to be used by external code.
// This would be the usual __declspec(dllexport) or __declspec(dllimport)
// if we were in a Windows DLL etc. But for this example use a global
// instance but with inline linkage so multiple definitions do not collide.
#define THIS_MODULE_API_DECL extern inline

// Declare a global function returning a static instance of the custom category
THIS_MODULE_API_DECL const utils::torrent_error &torrent_error()
{
    static utils::torrent_error c;
    return c;
}

// Overload the global make_error_code() free function with our
// custom enum. It will be found via ADL by the compiler if needed.
inline boost::system::error_code make_error_code(torrent_errc e)
{
    return {static_cast<int>(e), torrent_error()};
}