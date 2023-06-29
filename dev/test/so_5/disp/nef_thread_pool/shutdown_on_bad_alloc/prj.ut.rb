require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/nef_thread_pool/shutdown_on_bad_alloc'

Mxx_ru::setup_target(
	Mxx_ru::Binary_unittest_target.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
