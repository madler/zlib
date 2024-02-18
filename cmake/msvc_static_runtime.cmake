# CMAKE_CONFIGURATION_TYPES is empty on non-IDE generators (Ninja, NMake)
# and that's why we also use CMAKE_BUILD_TYPE to cover for those generators.
# For IDE generators, CMAKE_BUILD_TYPE is usually empty
foreach(config_type ${CMAKE_CONFIGURATION_TYPES} ${CMAKE_BUILD_TYPE})
    string(TOUPPER ${config_type} upper_config_type)
    set(flag_var "CMAKE_C_FLAGS_${upper_config_type}")
    if(${flag_var} MATCHES "/MD")
        string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endif()
endforeach()

# clean up
set(upper_config_type)
set(config_type)
set(flag_var)
