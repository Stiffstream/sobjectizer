require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj "so_5/prj.rb"

	target "_unit.test.mbox.mpsc_mbox_illegal_subscriber"

	cpp_source "main.cpp"
}

