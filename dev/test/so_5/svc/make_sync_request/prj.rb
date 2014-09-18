require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.so_5.svc.make_sync_request'

	cpp_source 'main.cpp'
}

