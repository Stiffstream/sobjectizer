#!/usr/local/bin/ruby
require 'mxx_ru/cpp'

MxxRu::Cpp::composite_target {
	add_test = lambda { |name| required_prj "test/so_5/disp/#{name}" }

	add_test[ 'binder/build_tests.rb' ]

	add_test[ 'thread_pool/build_tests.rb' ]

	add_test[ 'adv_thread_pool/build_tests.rb' ]

	add_test[ 'private_dispatchers/build_tests.rb' ]

	add_test[ 'prio_ot_strictly_ordered/build_tests.rb' ]
	add_test[ 'prio_ot_quoted_round_robin/build_tests.rb' ]

	add_test[ 'prio_dt_one_per_prio/build_tests.rb' ]
}


