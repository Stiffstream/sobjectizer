require 'mxx_ru/binary_unittest'

path = 'test/so_5/environment/stop_guards/stop_in_progress_nothrow'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
