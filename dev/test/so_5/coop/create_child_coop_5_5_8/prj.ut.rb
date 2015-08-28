require 'mxx_ru/binary_unittest'

path = 'test/so_5/coop/create_child_coop_5_5_8'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
