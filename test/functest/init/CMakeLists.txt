# Copyright (c) 2026 Huawei Technologies Co., Ltd. All Rights Reserved.
#
# KUPL is licensed under Mulan PSL v2.
# You can use this software according to the terms and conditions of the Mulan PSL v2.
# You may obtain a copy of Mulan PSL v2 at:
#        http://license.coscl.org.cn/MulanPSL2
# THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER EXPRESS OR IMPLIED,
# INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
# See the Mulan PSL v2 for more details.

cmake_minimum_required(VERSION 3.14)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fprofile-arcs -ftest-coverage")
set(CMAKE_EXE_LINKER_FLAGS "-fprofile-arcs -ftest-coverage")

add_executable(test_init_main test_core_init.cpp test_init_main.cpp)
target_link_libraries(test_init_main
    -Wl,--start-group
    ${GTEST_LIB} kupl dl pthread ${COV_LIB}
    -Wl,--end-group
)

# Separate binary: the failure test must run in a process which never initialized the runtime before.
add_executable(test_init_fail_main test_init_fail.cpp test_init_main.cpp)
target_link_libraries(test_init_fail_main
    -Wl,--start-group
    ${GTEST_LIB} kupl dl pthread ${COV_LIB}
    -Wl,--end-group
)

install(TARGETS test_init_main test_init_fail_main DESTINATION ${KUPL_INSTALL_BIN_PATH})
