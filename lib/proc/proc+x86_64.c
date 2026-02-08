/* pk_proc.c — Process management for x86_64 Linux raw syscalls */

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

	int64_t pid = pk_syscall0(__PK_SYS_fork);
	if (pid < 0) { r.tag = 1; r.val._1 = 0; return r; }

	if (pid == 0) {
		pk_syscall5(__PK_SYS_prctl, PK_PR_SET_PDEATHSIG, PK_SIGTERM, 0, 0, 0);
		pk_syscall3(__PK_SYS_execve, (int64_t)path_buf, (int64_t)c_argv, (int64_t)c_envp);
		pk_syscall1(__PK_SYS_exit, 127);
		__builtin_unreachable();
	}

	r.tag = 0; r.val._0 = (int32_t)pid;
	return r;
}

proc_Status pk_proc_check(int32_t pid) {
	proc_Status r;
	int32_t status = 0;
	int64_t ret = pk_syscall4(__PK_SYS_wait4, pid, (int64_t)&status, PK_WNOHANG, 0);
	if (ret > 0) {
		if ((status & 0x7f) == 0) { r.tag = 0; r.val._0 = (status >> 8) & 0xff; }
		else { r.tag = 0; r.val._0 = -(status & 0x7f); }
	} else {
		r.tag = 1; r.val._1 = 0;
	}
	return r;
}

int32_t pk_proc_wait(int32_t pid, int32_t timeout) {
	if (timeout < 0) {
		int32_t status = 0;
		pk_syscall4(__PK_SYS_wait4, pid, (int64_t)&status, 0, 0);
		if ((status & 0x7f) == 0) return (status >> 8) & 0xff;
		return -(status & 0x7f);
	}

	int32_t elapsed_ms = 0;
	int32_t sleep_ms = 1;
	while (elapsed_ms < timeout) {
		int32_t status = 0;
		int64_t ret = pk_syscall4(__PK_SYS_wait4, pid, (int64_t)&status, PK_WNOHANG, 0);
		if (ret > 0) {
			if ((status & 0x7f) == 0) return (status >> 8) & 0xff;
			return -(status & 0x7f);
		}
		struct pk_timespec ts;
		ts.sec = sleep_ms / 1000;
		ts.nsec = (int64_t)(sleep_ms % 1000) * 1000000LL;
		pk_syscall2(__PK_SYS_nanosleep, (int64_t)&ts, 0);
		elapsed_ms += sleep_ms;
		if (sleep_ms < 100) sleep_ms *= 2;
	}

	pk_syscall2(__PK_SYS_kill, pid, PK_SIGKILL);
	int32_t status = 0;
	pk_syscall4(__PK_SYS_wait4, pid, (int64_t)&status, 0, 0);
	return -PK_SIGKILL;
}

proc_VoidResult pk_proc_kill(int32_t pid, _Bool hard) {
	proc_VoidResult r;
	int32_t sig = hard ? PK_SIGKILL : PK_SIGTERM;
	int64_t ret = pk_syscall2(__PK_SYS_kill, pid, sig);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

__attribute__((noreturn))
void pk_proc_exit(int32_t code) {
	pk_syscall1(__PK_SYS_exit, code);
	__builtin_trap();
}

int32_t pk_proc_getpid(void) {
	return (int32_t)pk_syscall0(__PK_SYS_getpid);
}

void pk_proc_sleep(int32_t ms) {
	if (ms <= 0) return;
	struct pk_timespec ts;
	ts.sec = ms / 1000;
	ts.nsec = (int64_t)(ms % 1000) * 1000000LL;
	pk_syscall2(__PK_SYS_nanosleep, (int64_t)&ts, 0);
}

_Bool pk_proc_isroot(void) {
	return pk_syscall0(__PK_SYS_geteuid) == 0;
}

int32_t pk_proc_minprio(void) { return -20; }

int32_t pk_proc_maxprio(void) { return 20; }

proc_VoidResult pk_proc_setprio(int32_t pid, int32_t level) {
	proc_VoidResult r;
	int32_t nice_val = -level;
	int64_t ret = pk_syscall3(__PK_SYS_setpriority, PK_PRIO_PROCESS, pid, nice_val);
	if (ret < 0) { r.tag = 1; r.val._1 = 0; }
	else { r.tag = 0; r.val._0 = 0; }
	return r;
}

int32_t pk_proc_getprio(int32_t pid) {
	int64_t ret = pk_syscall2(__PK_SYS_getpriority, PK_PRIO_PROCESS, pid);
	return (int32_t)(ret - 20);
}
