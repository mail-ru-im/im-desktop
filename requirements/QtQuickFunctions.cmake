function(im_find_qt_quick_libraries_to_copy outlibraries folder_path lib_name)
    list(APPEND ${outlibraries} ${ICQ_QT_QML_DIR}/${folder_path}/${lib_name})
    list(APPEND ${outlibraries} ${ICQ_QT_QML_DIR}/${folder_path}/qmldir)
    list(APPEND ${outlibraries} ${ICQ_QT_QML_DIR}/${folder_path}/plugins.qmltypes)
    set(${outlibraries} ${${outlibraries}} PARENT_SCOPE)
endfunction()

function(im_copy_file_to_folder file folder)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory ${folder})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${file} ${folder})
endfunction()

function(im_copy_qt_quick_libraries)
    set(options)
    set(oneValueArgs FOLDER)
    set(multiValueArgs LIBS)
    cmake_parse_arguments(_PARSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(LIBRARY ${_PARSED_LIBS})
        im_copy_file_to_folder(${LIBRARY} ${_PARSED_FOLDER})
    endforeach()
endfunction()

function(im_copy_qt_quick_libraries_apple)
    set(options)
    set(oneValueArgs FOLDER PLUGINS_FOLDER RELATIVE_PLUGINS_FOLDER)
    set(multiValueArgs LIBS)
    cmake_parse_arguments(_PARSED "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    foreach(LIBRARY ${_PARSED_LIBS})
        get_filename_component(FILE_EXT ${LIBRARY} EXT)
        if("${FILE_EXT}" STREQUAL ".${QT_LIB_EXT}")
            im_copy_file_to_folder(${LIBRARY} ${_PARSED_PLUGINS_FOLDER})
            get_filename_component(FILE_NAME ${LIBRARY} NAME)
            set(SYMLINK_PATH "${_PARSED_FOLDER}/${FILE_NAME}")
            set(RELATIVE_LIBRARY_PATH "${_PARSED_RELATIVE_PLUGINS_FOLDER}/${FILE_NAME}")
            add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory ${_PARSED_FOLDER})
            if(NOT EXISTS ${SYMLINK_PATH})
                add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E chdir ${_PARSED_FOLDER} ln -s -f ${RELATIVE_LIBRARY_PATH} ${FILE_NAME})
            endif()
        else()
            im_copy_file_to_folder(${LIBRARY} ${_PARSED_FOLDER})
        endif()
    endforeach()
endfunction()
