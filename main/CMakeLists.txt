idf_component_register(SRCS "SMEM-MP.c"
                    INCLUDE_DIRS ".")

set(EXTRA_COMPONENT_DIRS ../components/net_driver)
set(EXTRA_COMPONENT_DIRS ../components/sntp_driver)
# Agrega manualmente componentes oficiales si no se están incluyendo
#set(EXTRA_COMPONENT_DIRS $ENV{IDF_PATH}/components/)

