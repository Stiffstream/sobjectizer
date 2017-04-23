require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj.rb'

	target '_unit.test.timer_thread.resend_periodic_signal_via_mhood'

	cpp_source 'main.cpp'
}

