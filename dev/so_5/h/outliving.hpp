/*!
 * \file
 * \brief outliving_reference_t and related stuff.
 *
 * \note
 * This code is borrowed from cpp_util_3 project
 * (https://bitbucket.org/sobjectizerteam/cpp_util-3.0)
 *
 * \since
 * v.5.5.19
 */

#pragma once

#include <so_5/h/compiler_features.hpp>

#include <memory>

namespace so_5 {

/*!
 * \brief Helper class for indication of long-lived reference via its type.
 *
 * Sometimes it is necessary to store a reference to an object that lives
 * longer that a reference holder. For example:
 * \code
	class config {...};
	class config_consumer {
		config & cfg_;
	public :
		config_consumer(config & cfg) : cfg_(cfg) {...}
		...
	};

	void f() {
		config cfg = load_config(); // cfg outlives all other objects in f().
		config_consumer consumer(cfg); // It is safe to store a reference
			// for all lifetime of consumer object.
		...
	}
 * \endcode
 *
 * The problem there is: when we see <tt>consumer::consumer(cfg)</tt> we can
 * say is it safe to pass a reference to short-lived object to it or not.
 * Helper class outliving_reference_t can be used as indicator that
 * <tt>consumer::consumer</tt> expects a reference to long-lived object: 
 * \code
	class config_consumer {
		so_5::outliving_reference_t<config> cfg_;
	public :
		config_consumer(so_5::outliving_reference_t<config> cfg)
			: cfg_(cfg)
		{...}
		...
	};

	void f() {
		config cfg = load_config();
		// An explicit sign that cfg will outlive consumer object.
		config_consumer consumer(so_5::outliving_mutable(cfg));
		...
	}
 * \endcode
 *
 * If it is necessary to store a const reference to long-lived object
 * then outliving_reference_t<const T> and outliving_const() should be used:
 * \code
	class data_processor {
		so_5::outliving_reference_t<const config> cfg_;
	public :
		data_processor(so_5::outliving_reference_t<const config> cfg)
			: cfg_(cfg)
		{...}
		...
	};

	void f() {
		config cfg = load_config();
		config_consumer consumer(so_5::outliving_mutable(cfg));
		data_processor processor(so_5::outliving_const(cfg));
		...
	}
 * \endcode
 *
 * \attention
 * outliving_reference_t has no copy operator! It is CopyConstructible,
 * but not CopyAssingable class.
 *
 * \tparam T type for reference.
 *
 * \since
 * v.5.5.19
 */
template< typename T >
class outliving_reference_t
{
	T * m_ptr;

public :
	using type = T;

	explicit outliving_reference_t(T & r) SO_5_NOEXCEPT : m_ptr(std::addressof(r)) {}

	outliving_reference_t(T &&) = delete;
	outliving_reference_t(outliving_reference_t const & o) SO_5_NOEXCEPT
		: m_ptr(std::addressof(o.get()))
		{}

	template<typename U>
	outliving_reference_t(outliving_reference_t<U> const & o) SO_5_NOEXCEPT
		: m_ptr(std::addressof(o.get()))
		{}

	outliving_reference_t & operator=(outliving_reference_t const &o) = delete;

	operator T&() const SO_5_NOEXCEPT { return this->get(); }

	T & get() const SO_5_NOEXCEPT { return *m_ptr; }
};

/*!
 * \brief Make outliving_reference wrapper for mutable reference.
 *
 * \sa outliving_reference_t
 *
 * \since
 * v.5.5.19
 */
template< typename T >
outliving_reference_t< T >
outliving_mutable( T & r ) { return outliving_reference_t<T>(r); }

/*!
 * \brief Make outliving_reference wrapper for const reference.
 *
 * \sa outliving_reference_t
 *
 * \since
 * v.5.5.19
 */
template< typename T >
outliving_reference_t< const T >
outliving_const( T const & r ) { return outliving_reference_t<const T>(r); }

/*!
 * \brief Make outliving_reference wrapper for const reference.
 *
 * \sa outliving_reference_t
 *
 * \since
 * v.5.5.22
 */
template< typename T >
outliving_reference_t< const T >
outliving_const( outliving_reference_t<T> r ) { return outliving_reference_t<const T>(r.get()); }

} /* namespace so_5 */

