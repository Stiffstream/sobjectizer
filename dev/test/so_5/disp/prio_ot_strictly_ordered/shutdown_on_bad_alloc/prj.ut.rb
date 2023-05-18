require 'mxx_ru/binary_unittest'

path = 'test/so_5/disp/prio_ot_strictly_ordered/shutdown_on_bad_alloc'

Mxx_ru::setup_target(
	MxxRu::NegativeBinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
