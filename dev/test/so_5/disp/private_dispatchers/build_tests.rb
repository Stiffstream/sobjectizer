#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {

	required_prj "test/so_5/disp/private_dispatchers/simple/prj.ut.rb"
	required_prj "test/so_5/disp/private_dispatchers/env_stop/prj.ut.rb"
}
