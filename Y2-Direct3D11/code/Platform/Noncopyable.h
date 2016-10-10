
/*
	Taken from Boost 1.57, modified to compile independently in Visual Studio.
	Uses C++11 if supported (see _MSV_VER checks).
*/

//  Boost noncopyable.hpp header file  --------------------------------------//

//  (C) Copyright Beman Dawes 1999-2003. Distributed under the Boost
//  Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/utility for documentation.

#ifndef BOOST_CORE_NONCOPYABLE_HPP
#define BOOST_CORE_NONCOPYABLE_HPP

namespace boost {

//  Private copy constructor and copy assignment ensure classes derived from
//  class noncopyable cannot be copied.

//  Contributed by Dave Abrahams

	class noncopyable
	{
	protected:
#if _MSC_VER > 1700
		constexpr noncopyable() = default;
		~noncopyable() = default;
#else
		noncopyable() {}
		~noncopyable() {}
#endif

#if _MSC_VER > 1700
		noncopyable( const noncopyable& ) = delete;
		noncopyable& operator=( const noncopyable& ) = delete;
#else
	private:  // emphasize the following members are private
		noncopyable( const noncopyable& );
		noncopyable& operator=( const noncopyable& );
#endif
  };

} // namespace boost

#endif  // BOOST_CORE_NONCOPYABLE_HPP
