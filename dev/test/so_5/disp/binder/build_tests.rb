#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {
	add_test = lambda { |name| required_prj "test/so_5/disp/binder/#{name}" }

	add_test[ 'bind_to_disp_1/prj.ut.rb' ]
	add_test[ 'bind_to_disp_2/prj.ut.rb' ]
	add_test[ 'bind_to_disp_3/prj.ut.rb' ]
	add_test[ 'bind_to_disp_error_no_disp/prj.ut.rb' ]
	add_test[ 'bind_to_disp_error_disp_type_mismatch/prj.ut.rb' ]
	add_test[ 'correct_unbind_after_throw_on_bind/prj.ut.rb' ]
}

