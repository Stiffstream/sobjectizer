require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/nef_thread_pool/custom_work_thread'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
