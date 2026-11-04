# The integration tests are a pytest suite in test/integration (Python 3,
# requires pytest, pytest-xdist and pysam). They run against the freshly built executable.
add_test(
    NAME RunBamReadcount
    COMMAND python3 -m pytest -p no:cacheprovider -n auto ${CMAKE_SOURCE_DIR}/test/integration
        --brc $<TARGET_FILE:bam-readcount>
)
set_tests_properties(RunBamReadcount PROPERTIES LABELS integration)
