require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/prio_ot_strictly_ordered/simple_seq1'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
