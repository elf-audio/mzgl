if (ANDROID)
  include_directories (${ANDROID_NDK}/sources/android/native_app_glue/)

  set (${CMAKE_C_FLAGS}, "${CMAKE_C_FLAGS}")

  add_library (native_app_glue STATIC ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c)

  set (OBOE_DIR ../../android/oboe)
  add_subdirectory (${OBOE_DIR} ./oboe)
  include_directories (${OBOE_DIR}/include)

  set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -O3 -fno-stack-protector")

  # Refer to: https://github.com/android-ndk/ndk/issues/381.
  set (CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -u ANativeActivity_onCreate")

  set_source_files_properties (lib/mzgl/util/FloatBuffer.cpp
                               PROPERTIES COMPILE_FLAGS "-ffast-math -O3  -mfpu=neon -mfloat-abi=softfp")
endif ()
