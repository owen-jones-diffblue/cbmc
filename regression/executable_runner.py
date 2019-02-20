import os
import signal
import subprocess
import time


class ExecutableRunner:
    """Run an executable"""

    def __init__(self, cmd, cwd=None, timeout=600, encoding='utf-8'):
        """Set the command to run, the timeout (optional) and the encoding (optional)

        cmd should be in the format excepted by subprocess.Popen, i.e. [executable_path, arg1, arg2, ...]
        timeout is in seconds
        """
        self.cmd = cmd
        self.cwd = cwd
        self.timeout = timeout
        self.encoding = encoding

    def run(self, pipe_stderr_to_stdout=False, env_dict=None, regular_log_messages=False):
        """Run the executable and capture the output

        If the process exceeds the timeout then we raise a TimeoutError.

        Return the tuple (output to stdout (string), output to stderr (string), exit code (int)).
        """
        # We limit ourselves to the API available in python 3.3 to make life easier on Travis. There are API
        # improvements in python 3.5 and 3.6 which would make this code simpler.

        # Allow the caller to pipe stderr to stdout
        stderr = subprocess.STDOUT if pipe_stderr_to_stdout else subprocess.PIPE

        # Allow the caller to set environment variables in the subprocess
        env = None
        if env_dict:
            env = os.environ.copy()
            for key, value in env_dict.items():
                env[key] = value

        # We must pass start_new_session=True so that we can kill the subprocess and all of its children using the
        # pid of the subprocess, without killing the process that pytest is running in
        proc = subprocess.Popen(self.cmd, cwd=self.cwd, stdout=subprocess.PIPE, stderr=stderr, env=env,
                                                                                             start_new_session=True)
        if regular_log_messages:
            stdout_data, stderr_data = get_proc_output_with_regular_log_messages(proc, self.timeout)
        else:
            stdout_data, stderr_data = get_proc_output(proc, self.timeout)

        return (stdout_data.decode(self.encoding) if stdout_data else None,
                stderr_data.decode(self.encoding) if stderr_data else None,
                proc.returncode)


def get_proc_output(proc, timeout):
    try:
        stdout_data, stderr_data = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        kill_process_and_children(proc.pid)
        print('The executable ran for longer than {} seconds'.format(timeout))
        raise TimeoutError
    return stdout_data, stderr_data


def get_proc_output_with_regular_log_messages(proc, timeout):
    stdout_data = None
    stderr_data = None
    time_run = 0
    print("")
    process_completed = False
    while not process_completed:
        try:
            stdout_data, stderr_data = proc.communicate(timeout=60)
            process_completed = True
        except subprocess.TimeoutExpired:
            time_run += 60
            print("pytest still running... ({} seconds)".format(time_run))
            if time_run >= timeout:
                kill_process_and_children(proc.pid)
                print('The executable ran for longer than {} seconds'.format(timeout))
                raise TimeoutError
    return stdout_data, stderr_data


def kill_process_and_children(pid):
    # We can't just run `proc.kill()` because we have run a process which may have launched child processes and
    # `proc.kill()` would only kill the process we launched
    os.killpg(os.getpgid(pid), signal.SIGTERM)
    time.sleep(5)
    os.killpg(os.getpgid(pid), signal.SIGKILL)
