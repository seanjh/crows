idf_component_register(
  SRCS "crow.c"
  INCLUDE_DIRS "."
  REQUIRES joltwallet__littlefs
)

littlefs_create_partition_image(storage ../data FLASH_IN_PROJECT)