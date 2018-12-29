#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	path = 'test/so_5/testing/v1'

	required_prj "#{path}/testenv_empty_scenario/prj.ut.rb"
	required_prj "#{path}/testenv_ping_pong/prj.ut.rb"
	required_prj "#{path}/testenv_simple_one_step/prj.ut.rb"
	required_prj "#{path}/testenv_simple_two_steps/prj.ut.rb"
	required_prj "#{path}/testenv_wait_completion_simple/prj.ut.rb"
	required_prj "#{path}/testenv_stored_state_name/prj.ut.rb"
	required_prj "#{path}/testenv_simple_when_any/prj.ut.rb"
	required_prj "#{path}/testenv_simple_when_all/prj.ut.rb"
	required_prj "#{path}/testenv_simple_when_all_2/prj.ut.rb"
	required_prj "#{path}/testenv_simple_when_all_3/prj.ut.rb"

	required_prj "#{path}/testenv_mpmc_mbox_ignore/prj.ut.rb"

	required_prj "#{path}/testenv_constraints/prj.ut.rb"

	required_prj "#{path}/testenv_constructors/prj.ut.rb"

	required_prj "#{path}/impact_as_message/prj.ut.rb"

	required_prj "#{path}/inline_ns/prj.ut.rb"
}

