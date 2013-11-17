configure_file(testdata.py.in testdata.py @ONLY)
set(BC_PYTHONPATH_EXTRA ${CMAKE_CURRENT_BUILD_DIR})
def_integration_test(bam-readcount RunBamReadcount bam-readcount_test.py)
