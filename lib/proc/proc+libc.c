/* pk_proc.c — Process management using libc (POSIX) */

proc_CreateResult pk_proc_create(pk_str path, pk_slice_str argv) {
	proc_CreateResult r;
	char path_buf[4096];
	pk_str_to_cstr(path, path_buf, sizeof(path_buf));

	size_t argc = argv.len;
	if (argc > 64) argc = 64;

	char* c_argv[66];
	char arg_bufs[64][256];

	c_argv[0] = path_buf;
	for (size_t i = 0; i < argc; i++) {
		pk_str_to_cstr(argv.data[i], arg_bufs[i], sizeof(arg_bufs[i]));
		c_argv[i + 1] = arg_bufs[i];
	}
	c_argv[argc + 1] = (char*)0;

	char* c_envp[1];
	c_envp[0] = (char*)0;

	pid_t pid = fork();
	if (pid < 0) { r.tag = 1; r.val._1 = 0; return r; }

	if (pid == 0) {
#ifdef __linux__
		prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif
		execve(path_buf, c_argv, c_envp);
		_exit(127);
	}

	r.tag = 0; r.val._0 = (int32_t)pid;
	return r;
}

proc_Status pk_proc_check(int32_t pid) {
	proc_Status r;
	int status = 0;
	pid_t ret = waitpid((pid_t)pid, &status, WNOHANG);
	if (ret > 0) {
		if (WIFEXITED(status)) { r.tag = 0; r.val._0 = WEXITSTATUS(status); }
		else if (WIFSIGNALED(status)) { r.tag = 0; r.val._0 = -(int32_t)WTERMSIG(status); }
		else { r.tag = 0; r.val._0 = -1; }
	} else {
		r.tag = 1; r.val._1 = 0;
	}
	return r;
}

int32_t pk_proc_wait(int32_t pid, int32_t timeout) {
	if (timeout < 0) {
		int status = 0;
		waitpid((pid_t)pid, &status, 0);
		if (WIFEXITED(status)) return WEXITSTATUS(status);
		if (WIFSIGNALED(status)) return -(int32_t)WTERMSIG(status);
		return -1;
	}

	int32_t elapsed_ms = 0;
	int32_t sleep_ms = 1;
	while (elapsed_ms < timeout) {
		int status = 0;
		pid_t ret = waitpid((pid_t)pid, &status, WNOHANG);
		if (ret > 0) {
			if (WIFEXITED(status)) return WEXITSTATUS(status);
			if (WIFSIGNALED(status)) return -(int32_t)WTERMSIG(status);
			return -1;
		}
		usleep((useconds_t)sleep_ms * 1000u);
		elapsed_ms += sleep_ms;
		if (sleep_ms < 100) sleep_ms *= 2;
	}

	kill((pid_t)pid, SIGKILL);
	int status = 0;
	waitpid((pid_t)pid, &status, 0);
	return -SIGKILL;
}

proc_VoidResult pk_proc_kill(int32_t pid, _Bool hard) {
	proc_VoidResult r;
	int sig = hard ? SIGKILL : SIGTERM;
	int ret = kill((pid_t)pid, sig);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

__attribute__((noreturn))
void pk_proc_exit(int32_t code) {
	_exit(code);
}

int32_t pk_proc_getpid(void) {
	return (int32_t)getpid();
}

void pk_proc_sleep(int32_t ms) {
	if (ms <= 0) return;
	struct timespec ts;
	ts.tv_sec = ms / 1000;
	ts.tv_nsec = (long)(ms % 1000) * 1000000L;
	nanosleep(&ts, NULL);
}

_Bool pk_proc_isroot(void) {
	return geteuid() == 0;
}

proc_VoidResult pk_proc_setprio(int32_t pid, int32_t level) {
	proc_VoidResult r;
	int nice_val = -level;
	int ret = setpriority(PRIO_PROCESS, (id_t)pid, nice_val);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

int32_t pk_proc_getprio(int32_t pid) {
	errno = 0;
	int ret = getpriority(PRIO_PROCESS, (id_t)pid);
	/* getpriority returns nice value (nice 0 = prio 0 in our mapping, nice -20 = prio 20) */
	return (int32_t)(-ret);
}
