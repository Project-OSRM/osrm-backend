set(oldfile ${CMAKE_SOURCE_DIR}/../Util/UUID.cpp)
if (EXISTS ${oldfile})
	file(REMOVE_RECURSE ${oldfile})
endif()

file(MD5 ${CMAKE_SOURCE_DIR}/../createHierarchy.cpp MD5PREPARE)
file(MD5 ${CMAKE_SOURCE_DIR}/../DataStructures/StaticRTree.h MD5RTREE)
file(MD5 ${CMAKE_SOURCE_DIR}/../DataStructures/NodeInformationHelpDesk.h MD5NODEINFO)
file(MD5 ${CMAKE_SOURCE_DIR}/../Util/GraphLoader.h MD5GRAPH)
file(MD5 ${CMAKE_SOURCE_DIR}/../Server/DataStructures/QueryObjectsStorage.cpp MD5OBJECTS)

CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/../Util/UUID.cpp.in ${CMAKE_SOURCE_DIR}/../Util/UUID.cpp )
