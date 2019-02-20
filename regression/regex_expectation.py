import re
import sys


class RegexExpectation:
    """Encapsulate searching text output for particular strings"""

    def __init__(self, text, return_code, compatibility_mode=True, cmdline=""):
        """Get the state at the end of test_function"""
        if return_code < 0:
            self.exit_code = 0
            self.signal = -return_code
        else:
            self.exit_code = return_code
            self.signal = 0
        if compatibility_mode:
            # This makes is easier to use this code with test.desc files written for test.pl
            text += "\nEXIT={}\nSIGNAL={}\n".format(self.exit_code, self.signal)
        self.text = text
        self.split_text = text.splitlines()
        self.cmdline = cmdline

    def check_successful_run(self):
        self.check_does_not_match_regex("^warning: ignoring")
        assert self.exit_code == 0
        assert self.signal == 0

    def check_verification_successful(self):
        self.check_matches_regex("^VERIFICATION SUCCESSFUL$")

    def check_matches(self, x):
        assert any(line.find(x) != -1 for line in self.split_text)

    def check_does_not_match(self, x):
        assert all(line.find(x) == -1 for line in self.split_text)

    def check_matches_regex(self, x):
        assert any(re.search(x, line) for line in self.split_text)

    def check_does_not_match_regex(self, x):
        assert all(not re.search(x, line) for line in self.split_text)

    def check_matches_multiline_regex(self, x):
        assert re.search(x, self.text) is not None

    def check_does_not_match_multiline_regex(self, x):
        assert re.search(x, self.text) is None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_value, traceback):
        if exc_value is not None:
            print("Failure may relate to command: ", self.cmdline, file=sys.stderr)
