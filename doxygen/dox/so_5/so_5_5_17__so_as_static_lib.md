# so-5.5.17: SObjectizer as static library {#so_5_5_17__so_as_static_lib}

Since v.5.5.17, it is possible to build SObjectizer as a shared and as a static library.

When SObjectizer is built via default build scripts (`build.rb` for Mxx_ru or `CMakeLists.txt` for CMake) both versions of the library are built: so.5.5.17.dll and so_s.5.5.17.lib on Windows platform, libso.5.5.17.so and libso_s.5.5.17.a on Unixes.

To use SObjectizer as a static library via Mxx_ru it is necessary to add `so_5/prj_s.rb` into your project file:

~~~~~{.rb}
require 'mxx_ru/cpp'

MxxRu::Cpp::exe_target {

	required_prj 'so_5/prj_s.rb'
	target 'sample.so_5.hello_world_s'

	cpp_source 'main.cpp'
}
~~~~~

To use SObjectizer as a static library via Mxx_ru it is necessary to link your target against `so_s.5.5.17` library:

~~~~~{.txt}
set(SAMPLE_S sample.so_5.hello_world_s)
add_executable(${SAMPLE_S} main.cpp)
target_link_libraries(${SAMPLE_S} so_s.${SO_5_VERSION})
~~~~~

**NOTE** If you are building SObjectizer by different build tool it is necessary to define symbols `SO_5_PRJ` and `SO_5_STATIC_LIB`. Symbol `SO_5_STATIC_LIB` must also be defined if building projects use SObjectizer as a static library.
