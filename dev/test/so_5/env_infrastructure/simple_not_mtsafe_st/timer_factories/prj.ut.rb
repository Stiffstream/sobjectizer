require 'mxx_ru/binary_unittest'

path = 'test/so_5/env_infrastructure/simple_not_mtsafe_st/timer_factories'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
