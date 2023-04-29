cmake_minimum_required(VERSION 3.25)
project(app C CXX)

FILE(GLOB_RECURSE APP_SRC
        ${CMAKE_CURRENT_LIST_DIR}/app/src/*.*
        )

add_executable(app ${APP_SRC})
