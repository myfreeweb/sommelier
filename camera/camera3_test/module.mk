# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

include common.mk
include pc_utils.mk

### Rules to generate the arc_camera3_test binary.

camera3_test_PC_DEPS := gbm libcamera_client libcamera_common \
	libcamera_metadata libcbm libchrome-$(BASE_VER) libdrm libexif libsync libyuv
camera3_test_CPPFLAGS := $(call get_pc_cflags,$(camera3_test_PC_DEPS))
camera3_test_LDLIBS := $(call get_pc_libs,$(camera3_test_PC_DEPS)) -ldl \
	-Wl,-Bstatic -lgtest -Wl,-Bdynamic -ljpeg -lpthread -pthread

CXX_BINARY(camera3_test/arc_camera3_test): CPPFLAGS += $(camera3_test_CPPFLAGS)
CXX_BINARY(camera3_test/arc_camera3_test): LDLIBS += $(camera3_test_LDLIBS)
CXX_BINARY(camera3_test/arc_camera3_test): \
	$(camera3_test_CXX_OBJECTS) \
	common/utils/camera_hal_enumerator.o

camera3_test/arc_camera3_test: CXX_BINARY(camera3_test/arc_camera3_test)

clean: CLEAN(camera3_test/arc_camera3_test)

.PHONY: camera3_test/arc_camera3_test

CXX_BINARY(camera3_test/arc_camera3_fuzzer): CPPFLAGS += \
	$(camera3_test_CPPFLAGS) -g -D FUZZER \
	-fsanitize=address -fsanitize-coverage=trace-pc-guard
CXX_BINARY(camera3_test/arc_camera3_fuzzer): LDLIBS += $(camera3_test_LDLIBS) \
	-lFuzzer -fsanitize=address -fsanitize-coverage=trace-pc-guard
CXX_BINARY(camera3_test/arc_camera3_fuzzer): \
	$(camera3_test_CXX_OBJECTS)

camera3_test/arc_camera3_fuzzer: CXX_BINARY(camera3_test/arc_camera3_fuzzer)

clean: CLEAN(camera3_test/arc_camera3_fuzzer)

.PHONY: camera3_test/arc_camera3_fuzzer
