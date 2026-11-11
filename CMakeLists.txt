cmake_minimum_required(VERSION 3.10)
project(beatbox C)

file(GLOB APP_SOURCES "app/src/*.c")
file(GLOB HAL_SOURCES "hal/src/*.c")

add_executable(beatbox ${APP_SOURCES} ${HAL_SOURCES})

target_include_directories(beatbox PRIVATE hal/include/hal app/include)

target_link_libraries(beatbox asound gpiod)

set(DEPLOY_DIR "$ENV{HOME}/ensc351/public/myApps")

add_custom_command(TARGET beatbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR}
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:beatbox> ${DEPLOY_DIR}/
)

add_custom_command(TARGET beatbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR}/beatbox-wav-files
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/wave-files ${DEPLOY_DIR}/beatbox-wav-files/
)