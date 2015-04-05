gem 'Mxx_ru', '>=1.6.4'

require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {
	required_prj 'build.rb'

	required_prj 'test/so_5/build_tests.rb'

	required_prj 'sample/so_5/build_samples.rb'
}
