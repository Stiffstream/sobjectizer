require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/adv_thread_pool/exception_from_safe_handler_2'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
