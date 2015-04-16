require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.ad_hoc_agents.default_exception_reaction'

	cpp_source 'main.cpp'
}

