import os

from regression.cbmc.cbmc_driver import CbmcDriver


folder_name = os.path.dirname(os.path.abspath(__file__))


def test_array_function_parameters():
    cbmc_driver = CbmcDriver(folder_name) \
        .with_test_file("test.c") \
        .with_extra_args("--function test --min-null-tree-depth 2 --max-nondet-tree-depth 2 --bounds-check")
    with cbmc_driver.run() as cbmc_expectation:
        cbmc_expectation.check_matches_regex("\[test.assertion.1\] line \d+ assertion Test.lists\[0\]->next: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.2\] line \d+ assertion Test.lists\[1\]->next: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.3\] line \d+ assertion Test.lists\[2\]->next: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.4\] line \d+ assertion Test.lists\[3\]->next: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.5\] line \d+ assertion !Test.lists\[0\]->next->next: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.6\] line \d+ assertion !Test.lists\[1\]->next->next: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.7\] line \d+ assertion !Test.lists\[2\]->next->next: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.8\] line \d+ assertion !Test.lists\[3\]->next->next: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.9\] line \d+ assertion Test.lists\[0\] != Test.lists\[1\]: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.10\] line \d+ assertion Test.lists\[1\] != Test.lists\[2\]: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.assertion.11\] line \d+ assertion Test.lists\[2\] != Test.lists\[3\]: SUCCESS")
        cbmc_expectation.check_matches_regex("\[test.array_bounds.1\] line \d+ array `Test'.lists upper bound in Test.lists\[\(signed long( long)? int\)4\]: FAILURE")
        cbmc_expectation.check_matches_regex("\[test.assertion.12\] line \d+ assertion !Test.lists\[4\]: FAILURE")
