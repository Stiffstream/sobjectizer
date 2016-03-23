require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/thread_pool/threshold'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
