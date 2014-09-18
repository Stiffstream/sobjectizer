require 'mxx_ru/binary_unittest'

MxxRu::setup_target(
	MxxRu::Binary_unittest_target.new(
		"test/so_5/messages/resend_message/prj.ut.rb",
		"test/so_5/messages/resend_message/prj.rb" )
)
