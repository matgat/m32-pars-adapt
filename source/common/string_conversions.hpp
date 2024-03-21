﻿#pragma once
//  ---------------------------------------------
//  Functions for string conversions
//  ---------------------------------------------
//  #include "string_conversions.hpp" // str::to_num_or<>()
//  ---------------------------------------------
#include <string_view>
#include <expected>
#include <charconv> // std::from_chars
#include <stdexcept> // std::runtime_error
#include <format>


//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace str //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

//---------------------------------------------------------------------------
// Convert a string_view to number (non throwing)
template<typename T>
[[nodiscard]] constexpr std::expected<T, std::string> to_num_or(const std::string_view sv) noexcept
{
    T result;
    const auto it_end = sv.data() + sv.size();
    const auto [it, ec] = std::from_chars(sv.data(), it_end, result);
    if( ec!=std::errc() or it!=it_end )
       {
        return std::unexpected( std::format("\"{}\" is not a valid number", sv) );
       }
    return result;
}


//---------------------------------------------------------------------------
// Convert a string_view to number (throwing)
template<typename T>
[[nodiscard]] constexpr T to_num(const std::string_view sv)
{
    T result;
    const auto it_end = sv.data() + sv.size();
    const auto [it, ec] = std::from_chars(sv.data(), it_end, result);
    if( ec!=std::errc() or it!=it_end )
       {
        throw std::runtime_error{ std::format("\"{}\" is not a valid number", sv) };
       }
    return result;
}

}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::




/////////////////////////////////////////////////////////////////////////////
#ifdef TEST_UNITS ///////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static ut::suite<"string_conversions"> string_conversions_tests = []
{////////////////////////////////////////////////////////////////////////////

ut::test("str::to_num_or<>") = []
   {
    if( const auto num = str::to_num_or<int>("-123") )
       {
        ut::expect( ut::that % num.value()==-123 );
       }
    else
       {
        ut::log << num.error() << '\n';
        ut::expect( false ) << "-123 should be a valid int";
       }

    if( const auto num = str::to_num_or<int>("123a") )
       {
        ut::log << "got value " << num.value() << '\n';
        ut::expect( false ) << "123a shouldn't be a valid int";
       }

    if( const auto num = str::to_num_or<unsigned short>("42") )
       {
        ut::expect( ut::that % num.value()==42u );
       }
    else
       {
        ut::log << num.error() << '\n';
        ut::expect( false ) << "42 should be a valid unsigned short";
       }

    if( const auto num = str::to_num_or<unsigned short>("-42") )
       {
        ut::log << "got value " << num.value() << '\n';
        ut::expect( false ) << "-42 shouldn't be a valid unsigned short";
       }
   };

ut::test("str::to_num<>") = []
   {
    ut::expect( ut::that % str::to_num<int>("42")==42 );
    ut::expect( ut::that % str::to_num<double>("42.1")==42.1 );

    ut::expect( ut::throws([]{ [[maybe_unused]] const auto n = str::to_num<int>("42.1"); }) ) << "should throw\n";
    ut::expect( ut::throws([]{ [[maybe_unused]] const auto n = str::to_num<int>("42a"); }) ) << "should throw\n";
    ut::expect( ut::throws([]{ [[maybe_unused]] const auto n = str::to_num<int>(""); }) ) << "should throw\n";
   };

};///////////////////////////////////////////////////////////////////////////
#endif // TEST_UNITS ////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
