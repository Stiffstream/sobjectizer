require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/nef_one_thread/custom_work_thread'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
