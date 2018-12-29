require 'mxx_ru/binary_unittest'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"test/so_5/event_queue_hook/simple/prj.ut.rb",
		"test/so_5/event_queue_hook/simple/prj.rb" )
)
