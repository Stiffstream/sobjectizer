/*
 * A unit-test for testing deregistration of already deregistered
 * coops (reference to environment is being got from direct mbox).
 */

#include <so_5/all.hpp>

#include <test/3rd_party/various_helpers/time_limited_execution.hpp>

#include <cstdlib>

namespace test {

class a_child_t final : public so_5::agent_t
	{
		const std::vector< std::string > m_head;
		std::vector< std::string > m_tail;

	public:
		a_child_t( context_t ctx, std::vector< std::string > head )
			:	so_5::agent_t::agent_t{ std::move(ctx) }
			,	m_head{ std::move(head) }
			{}

		void
		set_tail( std::vector< std::string > tail )
			{
				m_tail = std::move(tail);
			}

		void
		so_evt_start() override
			{
				so_deregister_agent_coop_normally();
			}
	};

struct msg_child_destroyed final : public so_5::signal_t {};

class destruction_notificator_t
	{
		const so_5::mbox_t m_target;

	public:
		destruction_notificator_t( so_5::mbox_t target )
			:	m_target{ std::move(target) }
			{}
		~destruction_notificator_t()
			{
				so_5::send< msg_child_destroyed >( m_target );
			}
	};

class a_test_performer_t final : public so_5::agent_t
	{
		struct info_t
			{
				so_5::mbox_t m_direct_mbox;
				so_5::coop_handle_t m_coop_handle;

				info_t(
					so_5::mbox_t direct_mbox,
					so_5::coop_handle_t coop_handle )
					:	m_direct_mbox{ std::move(direct_mbox) }
					,	m_coop_handle{ std::move(coop_handle) }
					{}
			};

		std::vector< info_t > m_infos;

	public:
		a_test_performer_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( &a_test_performer_t::evt_child_destroyed )
					;
			}

		void
		so_evt_start() override
			{
				register_next_coop();
			}

	private:
		void
		evt_child_destroyed(
			mhood_t< msg_child_destroyed > /*cmd*/ )
			{
				if( m_infos.size() < 100u )
					register_next_coop();
				else
					try_deregister_again();
			}

		void
		register_next_coop()
			{
				// This code is written in hope that reallocation of
				// memory will rewrite the old value of deregistered
				// agents.
				auto c = so_environment().make_coop();
				c->take_under_control(
						std::make_unique< destruction_notificator_t >(
								so_direct_mbox() ) );
				auto * a = c->make_agent< a_child_t >( make_payload() );
				a->set_tail( make_payload() );
				auto direct_mbox = a->so_direct_mbox();
				auto coop_handle = so_environment().register_coop( std::move(c) );

				m_infos.emplace_back(
						std::move(direct_mbox),
						std::move(coop_handle) );
			}

		void
		try_deregister_again()
			{
				for( const auto & info : m_infos )
					{
						info.m_direct_mbox->environment().deregister_coop(
								info.m_coop_handle,
								so_5::dereg_reason::normal );
					}

				so_deregister_agent_coop_normally();
			}

		[[nodiscard]]
		std::vector< std::string >
		make_payload() const
			{
				std::vector< std::string > result;
				auto s = 50 + static_cast< std::size_t >( std::rand() % 200 );
				if( !s ) s = 1u;

				for( std::size_t i = 0; i < s; ++i )
					{
						auto l = 50u + static_cast< std::size_t >( std::rand() % 500 );
						const auto ch = static_cast< char >( std::rand() & 0xff );
						result.push_back( std::string( l, ch ) );
					}

				return result;
			}
	};

void
init( so_5::environment_t & env )
	{
		env.introduce_coop( []( so_5::coop_t & parent ) {
				parent.make_agent< a_test_performer_t >();
			} );
	}

} /* namespace test */

using namespace test;

int
main()
{
	try
	{
		run_with_time_limit( []{ so_5::launch( init ); },
				20 );

		return 0;
	}
	catch( const std::exception & x )
	{
		std::cerr << "Exception: " << x.what() << std::endl;
	}

	return 2;
}

