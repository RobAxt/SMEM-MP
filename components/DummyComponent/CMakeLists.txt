include($ENV{IDF_PATH}/tools/cmake/version.cmake)

set(source "source/DummyComponent.c")
set(public_requires)

# list(APPEND sources "src/anotherDummy.c")

idf_component_register(SRCS "${source}"
                    INCLUDE_DIRS "include"
                    REQUIRES ${public_requires})
