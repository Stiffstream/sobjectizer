require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/nef_thread_pool/max_demands_at_once'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
