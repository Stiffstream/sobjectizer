require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.environment.default_subscr_storage" )

	cpp_source( "main.cpp" )
}
