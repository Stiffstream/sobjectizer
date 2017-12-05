require 'mxx_ru/binary_unittest'

path = 'test/so_5/state/const_enter_exit_handlers'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
