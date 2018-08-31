require 'rubygems'

gem 'Mxx_ru', '>= 1.3.0'

require 'mxx_ru/cpp'

MxxRu::Cpp::dll_target {

  target '_test.so_5.api.several_dlls.first'

  implib_path 'lib'

  required_prj 'so_5/prj.rb'

  cpp_source 'first.cpp'

  define 'FIRST_PRJ'

}

