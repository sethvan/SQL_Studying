add_library(create_db_table_binds
getDBTables.cpp 
createDBTableBinds.cpp
)
target_link_libraries(create_db_table_binds PUBLIC 
binds_array_wrapper
${MYSQLCLIENT_LIBRARY}
)
target_include_directories( create_db_table_binds PUBLIC include )
add_subdirectory(generated_code)
