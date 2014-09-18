require 'mxx_ru/binary_unittest'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		"test/so_5/disp/binder/correct_unbind_after_throw_on_bind/prj.ut.rb",
		"test/so_5/disp/binder/correct_unbind_after_throw_on_bind/prj.rb" )
)
