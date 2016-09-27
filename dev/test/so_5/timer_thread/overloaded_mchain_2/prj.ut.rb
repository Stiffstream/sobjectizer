require 'mxx_ru/binary_unittest'

path = 'test/so_5/timer_thread/overloaded_mchain_2'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
