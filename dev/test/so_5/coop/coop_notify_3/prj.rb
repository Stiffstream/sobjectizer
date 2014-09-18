require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj "so_5/prj.rb"

	target "_unit.test.coop.coop_notify_3"

	cpp_source "main.cpp"
}

