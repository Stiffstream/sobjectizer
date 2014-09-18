require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.so_5.svc.simple_svc_adhoc_agents'

	cpp_source 'main.cpp'
}

