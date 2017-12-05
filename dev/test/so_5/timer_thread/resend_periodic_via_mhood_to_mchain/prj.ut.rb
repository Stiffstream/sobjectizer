require 'mxx_ru/binary_unittest'

path = 'test/so_5/timer_thread/resend_periodic_via_mhood_to_mchain'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
