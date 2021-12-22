require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/adv_thread_pool/exception_from_safe_handler'

MxxRu::setup_target(
	MxxRu::NegativeBinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
