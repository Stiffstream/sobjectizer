require 'mxx_ru/binary_unittest'

MxxRu::setup_target(
	MxxRu::Binary_unittest_target.new(
		"test/so_5/messages/store_and_resend_later/prj.ut.rb",
		"test/so_5/messages/store_and_resend_later/prj.rb" )
)
