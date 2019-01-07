#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5'

	if 'cygwin' != toolset.tag( 'gcc_port', 'NOTGCC' )
		if !ENV.has_key?( 'SO_5_NO_SPINLOCKS_TEST' )
			required_prj "test/so_5/spinlocks/llvm_inspired_test/prj.ut.rb"
		end
	end

	required_prj "#{path}/details/build_tests.rb"

	required_prj "#{path}/environment/build_tests.rb"

	required_prj "#{path}/wrapped_env/build_tests.rb"

	required_prj "#{path}/execution_hint/basic_checks/prj.ut.rb"

	required_prj "#{path}/timer_thread/build_tests.rb" 

	required_prj "#{path}/mpsc_queue_traits/build_tests.rb"

	required_prj "#{path}/disp/build_tests.rb" 

	required_prj "#{path}/event_handler/build_tests.rb" 

	required_prj "#{path}/messages/build_tests.rb" 
	required_prj "#{path}/enveloped_msg/build_tests.rb" 

	required_prj "#{path}/state/build_tests.rb" 

	required_prj "#{path}/coop/build_tests.rb" 

	required_prj "#{path}/mbox/build_tests.rb" 

	required_prj "#{path}/mchain/build_tests.rb" 

	required_prj "#{path}/msg_tracing/build_tests.rb" 

	required_prj "#{path}/ad_hoc_agents/build_tests.rb" 

	required_prj "#{path}/message_limits/build_tests.rb" 

	required_prj "#{path}/layer/build_tests.rb" 

	required_prj "#{path}/api/run_so_environment/prj.ut.rb" 
	required_prj "#{path}/api/several_dlls/prj.ut.rb" 

	required_prj "#{path}/svc/build_tests.rb" 
	required_prj "#{path}/mutable_msg/build_tests.rb" 

	required_prj "#{path}/internal_stats/build_tests.rb" 

	required_prj "#{path}/env_infrastructure/build_tests.rb" 

	required_prj "#{path}/event_queue_hook/build_tests.rb" 

	required_prj "#{path}/testing/build_tests.rb" 

	required_prj "#{path}/bench/build_benchmarks.rb" 

	required_prj "#{path}/samples_as_unit_tests/build_tests.rb" 
}
