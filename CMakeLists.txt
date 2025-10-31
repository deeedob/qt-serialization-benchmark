cmake_minimum_required(VERSION 3.27)
project(QtSerializationBenchmark LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test Protobuf ProtobufQtCoreTypes ProtobufQtGuiTypes)
qt_standard_project_setup(REQUIRES 6.10)

qt_add_executable(QtSerializationBenchmark qtserializationbenchmark.cpp)

set_target_properties(QtSerializationBenchmark
    PROPERTIES
        WIN32_EXECUTABLE TRUE
        MACOSX_BUNDLE TRUE
)

qt_add_protobuf(QtSerializationBenchmark
    PROTO_FILES
        "task_manager.proto"
    PROTO_INCLUDES
        $<TARGET_PROPERTY:Qt::ProtobufQtCoreTypes,QT_PROTO_INCLUDES>
        $<TARGET_PROPERTY:Qt::ProtobufQtGuiTypes,QT_PROTO_INCLUDES>
)

target_link_libraries(QtSerializationBenchmark
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Test
        Qt6::Protobuf
        Qt::ProtobufQtCoreTypes
        Qt::ProtobufQtGuiTypes
)
