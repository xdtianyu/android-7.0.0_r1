#
# @Author Ralf Habacker
# 
# extracts version information from autoconf config file
# and set related cmake variables
# 
# returns  
#   ${prefix}_VERSION
#   ${prefix}_VERSION_STRING
#   ${prefix}_MAJOR_VERSION
#   ${prefix}_MINOR_VERSION
#   ${prefix}_MICRO_VERSION
# 
macro(autoversion config prefix)
	file (READ ${config} _configure_ac)
	string(TOUPPER ${prefix} prefix_upper)
	string (REGEX REPLACE ".*${prefix}_major_version], .([0-9]+).*" "\\1" ${prefix_upper}_MAJOR_VERSION ${_configure_ac})
	string (REGEX REPLACE ".*${prefix}_minor_version], .([0-9]+).*" "\\1" ${prefix_upper}_MINOR_VERSION ${_configure_ac})
	string (REGEX REPLACE ".*${prefix}_micro_version], .([0-9]+).*" "\\1" ${prefix_upper}_MICRO_VERSION ${_configure_ac})
	set (${prefix_upper}_VERSION ${${prefix_upper}_MAJOR_VERSION}.${${prefix_upper}_MINOR_VERSION}.${${prefix_upper}_MICRO_VERSION})
	set (${prefix_upper}_VERSION_STRING "${${prefix_upper}_VERSION}")

endmacro()

#
# parses config.h template and create cmake equivalent 
# not implemented yet
# 
macro(autoconfig template output)
	file(READ ${template} contents)
	# Convert file contents into a CMake list (where each element in the list
	# is one line of the file)
	STRING(REGEX REPLACE ";" "\\\\;" contents "${contents}")
	STRING(REGEX REPLACE "\n" ";" contents "${contents}")
	foreach(line contents)
		message(STATUS ${line})
		# find #undef lines
		# append to config.h #define <variable-name> <variable-content>
	endforeach()
endmacro()
