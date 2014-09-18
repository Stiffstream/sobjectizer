require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.so_5.svc.sync_request_and_wait_for'

	cpp_source 'main.cpp'
}

