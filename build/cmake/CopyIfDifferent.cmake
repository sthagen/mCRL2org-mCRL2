file(GLOB TO_COPY "${COPY_SRC}/*")
file(COPY ${TO_COPY} DESTINATION ${COPY_DEST})
