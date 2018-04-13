require 'mxx_ru/binary_unittest'

path = 'test/so_5/state/enter_exit_current_state_ptr'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
