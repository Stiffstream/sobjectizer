/*
	SObjectizer 5.
*/

/*!
	\file
	\brief The base class for the object with a reference counting definition.
*/

#pragma once

#include <so_5/h/declspec.hpp>
#include <so_5/h/types.hpp>
#include <so_5/h/compiler_features.hpp>

#include <type_traits>
#include <utility>
#include <memory>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

//! The base class for the object with a reference counting.
/*!
 * Should be used as a base class. The smart reference for such objects
 * should be defined for derived classes (for example agent_ref_t).
*/
class SO_5_TYPE atomic_refcounted_t
{
	public:
		/*! Disabled. */
		atomic_refcounted_t(
			const atomic_refcounted_t & ) = delete;

		/*! Disabled. */
		atomic_refcounted_t &
		operator = (
			const atomic_refcounted_t & ) = delete;

		//! Default constructor.
		/*!
		 * Sets reference counter to 0.
		 */
#if defined(SO_5_STD_ATOMIC_HAS_ONLY_DEFAULT_CTOR)
		atomic_refcounted_t() SO_5_NOEXCEPT
		{
			m_ref_counter = 0;
		}
#else
		atomic_refcounted_t() SO_5_NOEXCEPT : m_ref_counter(0)
		{}
#endif

		//! Destructor.
		/*!
		 * Do nothing.
		 */
		~atomic_refcounted_t() SO_5_NOEXCEPT = default;

		//! Increments reference count.
		inline void
		inc_ref_count() SO_5_NOEXCEPT
		{
			++m_ref_counter;
		}

		//! Decrement reference count.
		/*!
		 * \return Value of reference counter *after* decrement.
		*/
		inline unsigned long
		dec_ref_count() SO_5_NOEXCEPT
		{
			return --m_ref_counter;
		}

	private:
		//! Object reference count.
		atomic_counter_t m_ref_counter;
};

//
// intrusive_ptr_t
//
/*!
 * \since
 * v.5.2.0
 *
 * \brief Template class for smart reference wrapper on the atomic_refcounted_t.
 *
 * \tparam T class which must be derived from the atomic_refcounted_t.
 */
template< class T >
class intrusive_ptr_t
{
		static void ensure_right_T()
		{
			static_assert(
					std::is_base_of< atomic_refcounted_t, T >::value,
					"T must be derived from atomic_refcounted_t" );
		}

	public :
		//! Default constructor.
		/*!
		 * Constructs a null reference.
		 */
		intrusive_ptr_t() SO_5_NOEXCEPT
			:	m_obj( nullptr )
		{
			ensure_right_T();
		}
		//! Constructor for a raw pointer.
		intrusive_ptr_t( T * obj ) SO_5_NOEXCEPT
			:	m_obj( obj )
		{
			ensure_right_T();
			take_object();
		}
		//! Copy constructor.
		intrusive_ptr_t( const intrusive_ptr_t & o ) SO_5_NOEXCEPT
			:	m_obj( o.m_obj )
		{
			ensure_right_T();
			take_object();
		}
		//! Move constructor.
		intrusive_ptr_t( intrusive_ptr_t && o ) SO_5_NOEXCEPT
			:	m_obj( o.m_obj )
		{
			ensure_right_T();
			o.m_obj = nullptr;
		}

		/*!
		 * \since
		 * v.5.2.2
		 *
		 * \brief Constructor from another smart reference.
		 */
		template< class Y >
		intrusive_ptr_t(
			const intrusive_ptr_t< Y > & o ) SO_5_NOEXCEPT
			:	m_obj( o.get() )
		{
			ensure_right_T();
			take_object();
		}

		/*!
		 * \since
		 * v.5.5.23
		 *
		 * \brief Constructor from unique_ptr instance.
		 */
		template< class Y >
		intrusive_ptr_t(
			std::unique_ptr< Y > o ) SO_5_NOEXCEPT
			:	m_obj( o.release() )
		{
			ensure_right_T();
			take_object();
		}

		//! Destructor.
		~intrusive_ptr_t() SO_5_NOEXCEPT
		{
			dismiss_object();
		}

		//! Copy operator.
		intrusive_ptr_t &
		operator=( const intrusive_ptr_t & o ) SO_5_NOEXCEPT
		{
			intrusive_ptr_t( o ).swap( *this );
			return *this;
		}

		//! Move operator.
		intrusive_ptr_t &
		operator=( intrusive_ptr_t && o ) SO_5_NOEXCEPT
		{
			intrusive_ptr_t( std::move(o) ).swap( *this );
			return *this;
		}

		//! Swap values.
		void
		swap( intrusive_ptr_t & o ) SO_5_NOEXCEPT
		{
			std::swap( m_obj, o.m_obj );
		}

		/*!
		 * \since
		 * v.5.2.2
		 *
		 * \brief Drop controlled object.
		 */
		void
		reset() SO_5_NOEXCEPT
		{
			dismiss_object();
		}

		/*!
		 * \since
		 * v.5.2.2
		 *
		 * \brief Make reference with casing to different type.
		 */
		template< class Y >
		intrusive_ptr_t< Y >
		make_reference() const SO_5_NOEXCEPT
		{
			return intrusive_ptr_t< Y >( *this );
		}

		//! Is this a null reference?
		/*!
			i.e. whether get() != 0.

			\retval true if *this manages an object. 
			\retval false otherwise.
		*/
		operator bool() const SO_5_NOEXCEPT
		{
			return nullptr != m_obj;
		}

		/*!
		 * \name Access to object.
		 * \{
		 */
		T *
		get() const SO_5_NOEXCEPT
		{
			return m_obj;
		}

		T *
		operator->() const SO_5_NOEXCEPT
		{
			return m_obj;
		}

		T &
		operator*() const SO_5_NOEXCEPT
		{
			return *m_obj;
		}
		/*!
		 * \}
		 */

		/*!
		 * \name Comparision
		 * \{
		 */
		bool operator==( const intrusive_ptr_t & o ) const
		{
			T * p1 = get();
			T * p2 = o.get();
			if( p1 != nullptr && p2 != nullptr )
				return (*p1) == (*p2);
			else
				return p1 == p2;
		}

		bool operator<( const intrusive_ptr_t & o ) const
		{
			T * p1 = get();
			T * p2 = o.get();
			if( p1 != nullptr && p2 != nullptr )
				return (*p1) < (*p2);
			else
				return p1 < p2;
		}
		/*!
		 * \}
		 */

	private :
		//! Object controlled by a smart reference.
		T * m_obj;

		//! Increment reference count to object if it's not null.
		void
		take_object() SO_5_NOEXCEPT
		{
			if( m_obj )
				m_obj->inc_ref_count();
		}

		//! Decrement reference count to object and delete it if needed.
		void
		dismiss_object() SO_5_NOEXCEPT
		{
			if( m_obj )
			{
				if( 0 == m_obj->dec_ref_count() )
				{
					delete m_obj;
				}
				m_obj = nullptr;
			}
		}
};

namespace rt
{

// For compatibility with previous versions.
/*!
 * \deprecated Obsolete in v.5.5.1. Use so_5::intrusive_ptr_t instead.
 */
template< class T >
using smart_atomic_reference_t = intrusive_ptr_t< T >;

} /* namespace rt */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif
