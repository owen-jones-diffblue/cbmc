import os

from regression.executable_runner import ExecutableRunner
from regression.regex_expectation import RegexExpectation


class CbmcExpectation(RegexExpectation):
    """Encapsulate the output of cbmc"""

    def __init__(self, goto_functions, return_code, cmdline):
        """Get the state at the end of test_function"""
        RegexExpectation.__init__(self, goto_functions, return_code, compatibility_mode=False, cmdline=cmdline)


class CbmcDriver:
    """Run cbmc"""

    def __init__(self, folder_name):
        """Set the class path and temporary directory"""
        self.folder_name = folder_name
        self.test_file = ''
        self.extra_args = []

    def with_test_file(self, test_file):
        self.test_file = test_file
        return self

    def with_extra_args(self, string):
        self.extra_args = string.split(" ")
        return self

    def run(self):
        """Run cbmc and parse the output before returning it"""
        executable_path = os.path.join(os.environ['CBMC_CMAKE_HOME'], 'bin', 'cbmc')
        cmd = [executable_path, "--validate-goto-model", *self.extra_args, self.test_file]

        executable_runner = ExecutableRunner(cmd, cwd=self.folder_name)
        (stdout, _, return_code) = executable_runner.run(pipe_stderr_to_stdout=True)

        return CbmcExpectation(stdout, return_code, " ".join(cmd))
