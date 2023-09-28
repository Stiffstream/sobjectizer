#include <so_5/bind_then_transform_helpers.hpp>
#include <so_5/all.hpp>

namespace test
{

struct msg_part_one
	{
		int m_x;

		explicit msg_part_one( int x ) : m_x{x} {}
	};

struct msg_part_two
	{
		int m_y;

		explicit msg_part_two( int y ) : m_y{y} {}
	};

struct msg_signal final : public so_5::signal_t {};

class a_sender_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_dest;

	public:
		a_sender_t( context_t ctx, so_5::mbox_t dest )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_dest{ std::move(dest) }
			{}

		void
		so_evt_start() override
			{
				so_5::send< msg_signal >( m_dest );
			}
	};

class a_part_one_consumer_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_src;

	public:
		a_part_one_consumer_t( context_t ctx, so_5::mbox_t src )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_src{ std::move(src) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe( m_src )
					.event( []( const msg_part_one & msg ) {
							std::cout << "part_one: " << msg.m_x << std::endl;
						} )
					;
			}
	};

class a_part_two_consumer_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_src;

	public:
		a_part_two_consumer_t( context_t ctx, so_5::mbox_t src )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_src{ std::move(src) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe( m_src )
					.event( []( const msg_part_two & msg ) {
							std::cout << "part_two: " << msg.m_y << std::endl;
						} )
					;
			}
	};

void
introduce_coop( so_5::environment_t & env )
	{
		env.introduce_coop( []( so_5::coop_t & coop ) {
				auto dest = coop.environment().create_mbox();
				auto part_one_dest = coop.environment().create_mbox();
				auto part_two_dest = coop.environment().create_mbox();

				auto * binding = coop.take_under_control(
						std::make_unique< so_5::multi_sink_binding_t<> >() );

				so_5::bind_then_transform< msg_signal >(
						*binding,
						dest,
						[part_one_dest]() {
							return so_5::make_transformed< msg_part_one >(
									part_one_dest,
									0 );
						} );

				so_5::bind_then_transform< msg_signal >(
						*binding,
						dest,
						[part_two_dest]() {
							return so_5::make_transformed< msg_part_two >(
									part_two_dest,
									1 );
						} );

				coop.make_agent< a_sender_t >( dest );
				coop.make_agent< a_part_one_consumer_t >( part_one_dest );
				coop.make_agent< a_part_two_consumer_t >( part_two_dest );
			} );
	}

} /* namespace test */

int
main()
{
	using namespace test;

	so_5::launch( []( so_5::environment_t & env ) {
			introduce_coop( env );

			std::this_thread::sleep_for( std::chrono::seconds{1} );
			env.stop();
		} );

	return 0;
}


