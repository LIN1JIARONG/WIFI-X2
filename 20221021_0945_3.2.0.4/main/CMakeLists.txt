idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ".")

target_link_libraries(${COMPONENT_TARGET} "-u custom_app_desc")
                    
                    
                    