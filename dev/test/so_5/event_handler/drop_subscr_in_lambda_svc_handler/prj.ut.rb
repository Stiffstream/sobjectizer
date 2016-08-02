require 'mxx_ru/binary_unittest'

path = 'test/so_5/event_handler/drop_subscr_in_lambda_svc_handler'

MxxRu::setup_target(
	MxxRu::BinaryUnittestTarget.new(
		"#{path}/prj.ut.rb",
		"#{path}/prj.rb" )
)
