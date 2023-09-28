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

struct msg_full
	{
		msg_part_one m_one;
		msg_part_two m_two;

		msg_full( int x, int y ) : m_one{ x }, m_two{ y } {}
	};

class a_sender_t final : public so_5::agent_t
	{
		const so_5::mbox_t m_dest_1;
		const so_5::mbox_t m_dest_2;

	public:
		a_sender_t(
			context_t ctx,
			so_5::mbox_t dest_1,
			so_5::mbox_t dest_2 )
			:	so_5::agent_t{ std::move(ctx) }
			,	m_dest_1{ std::move(dest_1) }
			,	m_dest_2{ std::move(dest_2) }
			{}

		void
		so_evt_start() override
			{
				so_5::send< so_5::mutable_msg<msg_full> >( m_dest_1, 0, 0 );
				so_5::send< so_5::mutable_msg<msg_full> >( m_dest_2, 1, 1 );
			}
	};

class a_part_one_consumer_t final : public so_5::agent_t
	{
	public:
		a_part_one_consumer_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self()
					.event( []( const msg_part_one & msg ) {
							std::cout << "part_one: " << msg.m_x << std::endl;
						} )
					;
			}
	};

class a_part_two_consumer_t final : public so_5::agent_t
	{
	public:
		a_part_two_consumer_t( context_t ctx )
			:	so_5::agent_t{ std::move(ctx) }
			{}

		void
		so_define_agent() override
			{
				so_subscribe_self()
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
				auto dest_1 = so_5::make_unique_subscribers_mbox( coop.environment() );
				auto dest_2 = so_5::make_unique_subscribers_mbox( coop.environment() );

				auto * part_one = coop.make_agent< a_part_one_consumer_t >();
				auto * part_two = coop.make_agent< a_part_two_consumer_t >();

				auto * binding = coop.take_under_control(
						std::make_unique< so_5::multi_sink_binding_t<> >() );

				so_5::bind_then_transform< so_5::mutable_msg<msg_full> >(
						*binding,
						dest_1,
						[d = part_one->so_direct_mbox()]( const msg_full & msg ) {
							return so_5::make_transformed< msg_part_one >(
									d,
									msg.m_one );
						} );

				so_5::bind_then_transform< so_5::mutable_msg<msg_full> >(
						*binding,
						dest_2,
						[d = part_two->so_direct_mbox()]( auto & msg ) {
							msg.m_two = msg_part_two{ 3 };
							return so_5::make_transformed< msg_part_two >(
									d,
									msg.m_two );
						} );

				coop.make_agent< a_sender_t >( dest_1, dest_2 );
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

