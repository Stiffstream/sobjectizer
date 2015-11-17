require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj "so_5/prj.rb"

	target "_test.bench.so_5.coop_dereg"

	cpp_source "main.cpp"
}

