require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj( "so_5/prj.rb" )

	target( "_unit.test.messages.store_and_resend_later" )

	cpp_source( "main.cpp" )
}

