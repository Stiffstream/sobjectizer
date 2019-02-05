/*
 * SObjectizer-5
 */

/*!
 * \file
 * \since
 * v.5.5.4
 *
 * \brief Interfaces of data source and data sources repository.
 */

#pragma once

#include <so_5/declspec.hpp>

#include <so_5/rt/mbox.hpp>

#include <so_5/outliving.hpp>

#if defined( SO_5_MSVC )
	#pragma warning(push)
	#pragma warning(disable: 4251)
#endif

namespace so_5
{

namespace stats
{

/*!
 * \since
 * v.5.5.4
 *
 * \brief An interface of data source.
 */
class SO_5_TYPE source_t
	{
		friend class repository_t;

	protected :
		// Note: clang-3.9 requires this on Windows platform.
		source_t( const source_t & ) = delete;
		source_t( source_t && ) = delete;
		source_t & operator=( const source_t & ) = delete;
		source_t & operator=( source_t && ) = delete;
		source_t() = default;
		virtual ~source_t() SO_5_NOEXCEPT = default;

	public :
		//! Send appropriate notification about the current value.
		virtual void
		distribute(
			//! Target mbox for the appropriate message.
			const mbox_t & distribution_mbox ) = 0;

	private :
		//! Previous item in the data sources list.
		source_t * m_prev{};
		//! Next item in the data sources list.
		source_t * m_next{};
	};

/*!
 * \since
 * v.5.5.4
 *
 * \brief An interface of data sources repository.
 */
class SO_5_TYPE repository_t
	{
	protected :
		// Note: clang-3.9 requires this on Windows platform.
		repository_t( const repository_t & ) = delete;
		repository_t( repository_t && ) = delete;
		repository_t & operator=( const repository_t & ) = delete;
		repository_t & operator=( repository_t && ) = delete;

		repository_t() = default;
		virtual ~repository_t() SO_5_NOEXCEPT = default;

	public :
		//! Registration of new data source.
		/*!
		 * Caller must guarantee that the data source will live until
		 * it is registered in the repository.
		 */
		virtual void
		add( source_t & what ) = 0;

		//! Deregistration of previously registered data source.
		virtual void
		remove( source_t & what ) = 0;

	protected :
		//! Helper method for adding data source to existing list.
		static void
		source_list_add(
			//! A new data source to be added to the list.
			source_t & what,
			//! Marker of the list head.
			//! Will be modified if the list is empty.
			source_t *& head,
			//! Marker of the list tail.
			//! Will be modified.
			source_t *& tail );

		//! Helper method for removing data source from existing list.
		static void
		source_list_remove(
			//! An item to be removed.
			source_t & what,
			//! Marker of the list head.
			//! Will be modified if the list becomes empty.
			source_t *& head,
			//! Marker of the list tail.
			//! Will be modified if the item at the end of the list.
			source_t *& tail );

		//! Helper method for accessing next data source in the list.
		static source_t *
		source_list_next(
			//! The current item.
			const source_t & what );
	};

//
// auto_registered_source_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Version of data source with ability of automatic registration
 * and deregistration of data source in the repository.
 */
class SO_5_TYPE auto_registered_source_t : public source_t
	{
	protected :
		auto_registered_source_t( outliving_reference_t< repository_t > repo );
		~auto_registered_source_t() SO_5_NOEXCEPT override;

	private :
		outliving_reference_t< repository_t > m_repo;
	};

//
// manually_registered_source_t
//
/*!
 * \since
 * v.5.5.4
 *
 * \brief Version of data source for the case when the registration
 * and deregistration of data source in the repository must be performed
 * manually.
 *
 * \note Destructor automatically calls stop() if start() was called.
 */
class SO_5_TYPE manually_registered_source_t : public source_t
	{
	protected :
		manually_registered_source_t();
		~manually_registered_source_t() SO_5_NOEXCEPT override;

	public :
		void
		start( outliving_reference_t< repository_t > repo );

		void
		stop();

	private :
		//! Receives actual value only after successful start.
		repository_t * m_repo;
	};

} /* namespace stats */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

