require 'rubygems'

gem 'Mxx_ru', '>= 1.3.0'

require 'mxx_ru/cpp'

MxxRu::Cpp::dll_target {

  target '_test.so_5.api.several_dlls.second'

  implib_path 'lib'

  required_prj 'so_5/prj.rb'

  cpp_source 'second.cpp'

  define 'SECOND_PRJ'

}

