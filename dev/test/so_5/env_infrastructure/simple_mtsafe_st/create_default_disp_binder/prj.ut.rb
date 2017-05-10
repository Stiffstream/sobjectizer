require 'mxx_ru/binary_unittest'

path = 'test/so_5/env_infrastructure/simple_mtsafe_st/create_default_disp_binder'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
