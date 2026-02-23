find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(WEB_SRC_DIR ${COMPONENT_DIR}/web)
set(WEB_DIST_DIR ${COMPONENT_DIR}/web_dist)
set(WEB_MINIFY_SCRIPT ${COMPONENT_DIR}/../tools/minify_web_assets.py)
set(ENV_LOCAL_FILE ${CMAKE_SOURCE_DIR}/.env.local)

set(APP_WIFI_SSID "unexplained_error")
set(APP_WIFI_PASSWORD "unexpected-bacon")

if(EXISTS ${ENV_LOCAL_FILE})
    file(STRINGS ${ENV_LOCAL_FILE} WIFI_ENV_LINES REGEX "^(WIFI_SSID|WIFI_PASSWORD)=")
    foreach(WIFI_ENV_LINE IN LISTS WIFI_ENV_LINES)
        if(WIFI_ENV_LINE MATCHES "^WIFI_SSID=(.*)$")
            if(NOT "${CMAKE_MATCH_1}" STREQUAL "")
                set(APP_WIFI_SSID "${CMAKE_MATCH_1}")
            endif()
        endif()
        if(WIFI_ENV_LINE MATCHES "^WIFI_PASSWORD=(.*)$")
            if(NOT "${CMAKE_MATCH_1}" STREQUAL "")
                set(APP_WIFI_PASSWORD "${CMAKE_MATCH_1}")
            endif()
        endif()
    endforeach()
endif()

string(REPLACE "\\" "\\\\" APP_WIFI_SSID_ESCAPED "${APP_WIFI_SSID}")
string(REPLACE "\"" "\\\"" APP_WIFI_SSID_ESCAPED "${APP_WIFI_SSID_ESCAPED}")
string(REPLACE "\\" "\\\\" APP_WIFI_PASSWORD_ESCAPED "${APP_WIFI_PASSWORD}")
string(REPLACE "\"" "\\\"" APP_WIFI_PASSWORD_ESCAPED "${APP_WIFI_PASSWORD_ESCAPED}")

set(GENERATED_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_INCLUDE_DIR})
configure_file(
    ${COMPONENT_DIR}/app_wifi_config.h.in
    ${GENERATED_INCLUDE_DIR}/app_wifi_config.h
    @ONLY
)

execute_process(
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WEB_DIST_DIR}
)

execute_process(
    COMMAND ${Python3_EXECUTABLE} ${WEB_MINIFY_SCRIPT} --src ${WEB_SRC_DIR} --dst ${WEB_DIST_DIR}
    RESULT_VARIABLE WEB_MINIFY_RESULT
)

if(NOT WEB_MINIFY_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to minify web assets")
endif()

idf_component_register(
    SRCS
        "main.cpp"
        "internal/board_profile.cpp"
        "internal/gpio_runtime.cpp"
    INCLUDE_DIRS "." ${GENERATED_INCLUDE_DIR}
    EMBED_TXTFILES
        "web_dist/index.html"
        "web_dist/styles.css"
        "web_dist/app.js"
)
