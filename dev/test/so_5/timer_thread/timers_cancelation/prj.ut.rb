require 'mxx_ru/binary_unittest'

MxxRu::setup_target(
	MxxRu::Binary_unittest_target.new(
		"test/so_5/timer_thread/timers_cancelation/prj.ut.rb",
		"test/so_5/timer_thread/timers_cancelation/prj.rb" )
)
