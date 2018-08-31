require 'rubygems'

gem 'Mxx_ru', '>= 1.3.0'

require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

  target '_unit.test.so_5.api.several_dlls'

  required_prj 'test/so_5/api/several_dlls/first.rb'
  required_prj 'test/so_5/api/several_dlls/second.rb'
  required_prj 'test/so_5/api/several_dlls/third.rb'

  cpp_source 'main.cpp'
}

