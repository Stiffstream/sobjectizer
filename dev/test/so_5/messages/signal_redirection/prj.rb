require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.messages.signal_redirection" )

	cpp_source( "main.cpp" )
}

