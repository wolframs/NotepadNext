find_package(Qt6 REQUIRED COMPONENTS Test)
qt_add_executable(editor_feature_tests
    EditorFeatureTests.cpp
    ../src/widgets/MarkdownPreviewBrowser.cpp
    ../src/widgets/MarkdownPreviewBrowser.h
    ../src/ApplicationSettings.cpp
    ../src/ApplicationSettings.h
)
target_include_directories(editor_feature_tests PRIVATE ../src ../src/widgets ../thirdparty/lua/src)
target_compile_definitions(editor_feature_tests PRIVATE NOTEPADNEXT_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_link_libraries(editor_feature_tests PRIVATE Qt6::Widgets Qt6::Test lua)
set_target_properties(editor_feature_tests PROPERTIES WIN32_EXECUTABLE OFF)
add_test(NAME editor_features COMMAND editor_feature_tests)
set_tests_properties(editor_features PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
