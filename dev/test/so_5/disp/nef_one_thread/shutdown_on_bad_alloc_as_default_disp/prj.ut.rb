require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/nef_one_thread/shutdown_on_bad_alloc_as_default_disp'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
