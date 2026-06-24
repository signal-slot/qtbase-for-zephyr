# Qt "Books" SQL example on RT1170-EVKB with file-based SQLite on SD card.
#
# Build:
#   west build -b mimxrt1170_evk@B/mimxrt1176/cm7 -p always \
#       qt5/qtbase/examples/zephyr-rt1170-books -- -DSHIELD=rk055hdmipi4ma0

cmake_minimum_required(VERSION 3.20.0)

set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--push-state,--whole-archive"
    "<LINK_ITEM>"
    "LINKER:--pop-state")
set(CMAKE_CXX_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(qt_books LANGUAGES CXX C)

list(APPEND CMAKE_PREFIX_PATH "$ENV{QT_ZEPHYR_PREFIX}")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Sql)

set(BOOKS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../sql/books")

target_sources(app PRIVATE
    src/main.cpp
    src/bookwindow.cpp
    src/bookdelegate.cpp
)

target_include_directories(app PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${BOOKS_DIR}"
)

target_link_libraries(app PRIVATE
    Qt6::Sql
    Qt6::Widgets
    Qt6::Gui
    Qt6::Core
)

# SVG images (star.svg, star-filled.svg) used by BookDelegate
qt_add_resources(app "books"
    PREFIX "/"
    BASE "${BOOKS_DIR}"
    FILES
        "${BOOKS_DIR}/images/star.svg"
        "${BOOKS_DIR}/images/star-filled.svg"
)
