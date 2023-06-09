require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/nef_thread_pool/unique_thread_id'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
