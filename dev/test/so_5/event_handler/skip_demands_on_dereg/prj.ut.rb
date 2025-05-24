require 'mxx_ru/binary_unittest'

path = 'test/so_5/event_handler/skip_demands_on_dereg'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
