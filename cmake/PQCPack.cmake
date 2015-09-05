INCLUDE(InstallRequiredSystemLibraries)

SET(CPACK_PACKAGE_NAME pquery-${PQUERY_EXT})
SET(CPACK_DEBIAN_PACKAGE_MAINTAINER "Alexey Bychko") 
SET(CPACK_PACKAGE_CONTACT "alexey.bychko@percona.com")

INCLUDE(CPack)

