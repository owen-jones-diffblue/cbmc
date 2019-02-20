import os

from regression.cbmc.cbmc_driver import CbmcDriver


folder_name = os.path.dirname(os.path.abspath(__file__))


def test___func__1():
    cbmc_driver = CbmcDriver(folder_name) \
                   .with_test_file('main.c')
    with cbmc_driver.run() as cbmc_expectation:
        cbmc_expectation.check_successful_run()
        cbmc_expectation.check_verification_successful()
