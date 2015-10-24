require 'mxx_ru/binary_unittest'

path = 'test/so_5/msg_tracing/overlimit_abort_app'

MxxRu::setup_target(
	MxxRu::NegativeBinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
