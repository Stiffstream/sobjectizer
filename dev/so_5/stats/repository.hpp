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

#include <so_5/mbox.hpp>

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
		virtual ~source_t() noexcept = default;

	public :
		//! Send appropriate notification about the current value.
		virtual void
		distribute(
			//! Target mbox for the appropriate message.
			const mbox_t & /*distribution_mbox*/ ) = 0;

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
		virtual ~repository_t() noexcept = default;

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
		remove( source_t & what ) noexcept = 0;

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
			source_t *& tail ) noexcept;

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
			source_t *& tail ) noexcept;

		//! Helper method for accessing next data source in the list.
		static source_t *
		source_list_next(
			//! The current item.
			const source_t & what ) noexcept;
	};

/*!
 * \brief A holder for data-souce that should be automatically
 * registered and deregistered in registry.
 *
 * This class is necessary because data-source can't
 * register and deregister itself in the constructor/destructor.
 * It can lead to errors when `distribute()` method is called
 * during object's destruction.
 *
 * To avoid that problem data-source is created inside that
 * holder. It means that data-source is still live and fully
 * constructed when the destructor of holder starts its work.
 * It allows to deregister data-source before the destructor
 * of data source will be called.
 *
 * \since
 * v.5.6.0
 */
template< typename Data_Source >
class auto_registered_source_holder_t
	{
	public :
		// This class isn't Copyable nor Moveable.
		auto_registered_source_holder_t(
			const auto_registered_source_holder_t & ) = delete;
		auto_registered_source_holder_t(
			auto_registered_source_holder_t && ) = delete;

		//! Initializing constructor.
		template< typename... Args >
		auto_registered_source_holder_t(
			outliving_reference_t< repository_t > repo,
			Args && ...args )
			:	m_repo{ repo }
			,	m_ds{ std::forward<Args>(args)... }
			{
				m_repo.get().add( m_ds );
			}

		~auto_registered_source_holder_t() noexcept
			{
				m_repo.get().remove( m_ds );
			}

		Data_Source &
		get() noexcept { return m_ds; }

		const Data_Source &
		get() const noexcept { return m_ds; }

	private :
		//! Repository for data source.
		outliving_reference_t< repository_t > m_repo;

		//! Data source itself.
		Data_Source m_ds;
	};

/*!
 * \brief An addition to auto_registered_source_holder for the
 * cases where manual registration of data_source should be used
 * instead of automatic one.
 *
 * \since
 * v.5.6.0
 */
template< typename Data_Source >
class manually_registered_source_holder_t
	{
	public :
		// This class isn't Copyable nor Moveable.
		manually_registered_source_holder_t(
			const manually_registered_source_holder_t & ) = delete;
		manually_registered_source_holder_t(
			manually_registered_source_holder_t && ) = delete;

		//! Initializing constructor.
		template< typename... Args >
		manually_registered_source_holder_t(
			Args && ...args )
			:	m_ds{ std::forward<Args>(args)... }
			{}

		~manually_registered_source_holder_t() noexcept
			{
				if( m_repo )
					stop();
			}

		void
		start( outliving_reference_t< repository_t > repo )
			{
				repo.get().add( m_ds );
				m_repo = &(repo.get());
			}

		void
		stop() noexcept
			{
				m_repo->remove( m_ds );
				m_repo = nullptr;
			}

		Data_Source &
		get() noexcept { return m_ds; }

		const Data_Source &
		get() const noexcept { return m_ds; }

	private :
		//! Repository for data source.
		repository_t * m_repo{ nullptr };

		//! Data source itself.
		Data_Source m_ds;
	};

} /* namespace stats */

} /* namespace so_5 */

#if defined( SO_5_MSVC )
	#pragma warning(pop)
#endif

