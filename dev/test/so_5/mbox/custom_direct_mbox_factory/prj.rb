require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj "so_5/prj.rb"

	target "_unit.test.mbox.custom_direct_mbox_factory"

	cpp_source "main.cpp"
}

