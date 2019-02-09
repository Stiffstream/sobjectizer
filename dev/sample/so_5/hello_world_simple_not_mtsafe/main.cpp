/*
 * A sample of "Hello, World" for simple_not_mtsafe environment infrastructure.
 */

// Main SObjectizer header files.
#include <so_5/all.hpp>

using namespace std;
using namespace so_5;

class hello_actor final : public agent_t
{
public :
	using agent_t::agent_t;

	void so_evt_start() override
	{
		cout << "Hello, world!" << endl;
	}
};

int main()
{
	so_5::launch( []( environment_t & env )
		{
			// Creating and registering a cooperation.
			env.introduce_coop( []( coop_t & coop ) {
				// Adding agent to the cooperation.
				coop.make_agent<hello_actor>();
			});
		},
		[]( environment_params_t & params ) {
			params.infrastructure_factory(
					so_5::env_infrastructures::simple_not_mtsafe::factory() );
		} );

	return 0;
}

