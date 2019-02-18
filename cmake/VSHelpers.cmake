# Add Visual Studio filters to better organize the code
function(vs_set_filters)
	cmake_parse_arguments(VS_SET_FILTERS "" "FILTER_ROOT;BASE_DIR" "SOURCES" ${ARGN})
	if(MSVC)
		foreach(FILE IN ITEMS ${VS_SET_FILTERS_SOURCES}) 
		    # Get the directory of the source file
		    get_filename_component(PARENT_DIR "${FILE}" DIRECTORY)

		    # Remove common directory prefix to make the group
		    if(BASE_DIR STREQUAL "")
		    	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}" "" GROUP "${PARENT_DIR}")
		    else()
		    	string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/${VS_SET_FILTERS_BASE_DIR}" "" GROUP "${PARENT_DIR}")
		    endif()

		    # Use Windows path separators
		    string(REPLACE "/" "\\" GROUP "${GROUP}")

		    # Add to filter
		    source_group("${VS_SET_FILTERS_FILTER_ROOT}${GROUP}" FILES "${FILE}")
		endforeach()
	endif()
endfunction()

# Make the Debug and RelWithDebInfo targets use Program Database for Edit and Continue for easier debugging
function(vs_use_edit_and_continue)
	if(MSVC)
		string(REPLACE "/Zi" "/ZI" CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}")
		string(REPLACE "/Zi" "/ZI" CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
		set(CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG} PARENT_SCOPE)
		set(CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_CXX_FLAGS_RELWITHDEBINFO} PARENT_SCOPE)
	endif()
endfunction()
