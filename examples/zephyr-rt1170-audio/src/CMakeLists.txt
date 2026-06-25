cmake_minimum_required(VERSION 3.20)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Multimedia)
qt_add_executable(audio_test main.cpp)
target_link_libraries(audio_test PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Multimedia)

qt_add_resources(audio_test "audio_resources"
    PREFIX "/"
    FILES tone_pi.wav tone_po.wav tone_pa.wav
)
