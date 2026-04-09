cmake_minimum_required(VERSION 3.27)
project(dns_proxy_cache C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(dns_proxy_cache main.c)
set_target_properties(dns_proxy_cache PROPERTIES OUTPUT_NAME "dns-proxy-cache")
