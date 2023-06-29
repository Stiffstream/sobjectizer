#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/bench'

	required_prj "#{path}/ping_pong/prj.rb"
	required_prj "#{path}/same_msg_in_different_states/prj.rb"
	required_prj "#{path}/parallel_send_to_same_mbox/prj.rb"
	required_prj "#{path}/change_state/prj.rb"
	required_prj "#{path}/many_mboxes/prj.rb"
	required_prj "#{path}/thread_pool_disp/prj.rb"
	required_prj "#{path}/no_workload/prj.rb"
	required_prj "#{path}/agent_ring/prj.rb"
	required_prj "#{path}/coop_dereg/prj.rb"
	required_prj "#{path}/parallel_parent_child/prj.rb"
	required_prj "#{path}/skynet1m/prj.rb"
	required_prj "#{path}/prepared_receive/prj.rb"
	required_prj "#{path}/prepared_select/prj.rb"
	required_prj "#{path}/named_mboxes/prj.rb"
}
