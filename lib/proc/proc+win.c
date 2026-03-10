/* pk_proc.c — Process management for Windows (MinGW) */

/* Build a command line string from path + argv.
 * Windows CreateProcessA takes a single command line string.
 * Format: "path" "arg1" "arg2" ... */
static size_t pk_build_cmdline(pk_str path, pk_slice_str argv, char* out, size_t outsize) {
	size_t pos = 0;

	/* Quote the executable path */
	if (pos < outsize) out[pos++] = '"';
	for (size_t i = 0; i < path.len && pos < outsize - 1; i++) {
		out[pos++] = (char)path.data[i];
	}
	if (pos < outsize) out[pos++] = '"';

	/* Append each argument, quoted */
	for (size_t a = 0; a < argv.len; a++) {
		if (pos < outsize) out[pos++] = ' ';
		if (pos < outsize) out[pos++] = '"';
		pk_str arg = argv.data[a];
		for (size_t i = 0; i < arg.len && pos < outsize - 1; i++) {
			/* Escape embedded quotes */
			if (arg.data[i] == '"' && pos + 1 < outsize) {
				out[pos++] = '\\';
			}
			out[pos++] = (char)arg.data[i];
		}
		if (pos < outsize) out[pos++] = '"';
	}

	if (pos < outsize) out[pos] = '\0';
	else out[outsize - 1] = '\0';
	return pos;
}

static HANDLE pk_dup_inheritable(HANDLE h) {
	HANDLE out = NULL;
	if (!DuplicateHandle(GetCurrentProcess(), h, GetCurrentProcess(),
		&out, 0, TRUE, DUPLICATE_SAME_ACCESS)) {
		return INVALID_HANDLE_VALUE;
	}
	return out;
}

proc_CreateResult pk_proc_create(pk_str path, pk_slice_str argv) {
	proc_CreateResult r;

	/* Find a free slot in the process table */
	int slot = -1;
	for (int i = 0; i < PK_MAX_PROCS; i++) {
		if (!pk_proc_table[i].active) { slot = i; break; }
	}
	if (slot < 0) { r.tag = 1; r.val._1 = 0; return r; }

	/* Ensure job object is ready */
	pk__ensure_job_object();

	/* Build command line */
	char cmdline[32768];
	pk_build_cmdline(path, argv, cmdline, sizeof(cmdline));

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	for (size_t i = 0; i < sizeof(si); i++) ((uint8_t*)&si)[i] = 0;
	for (size_t i = 0; i < sizeof(pi); i++) ((uint8_t*)&pi)[i] = 0;
	si.cb = sizeof(si);

	/* CREATE_SUSPENDED so we can assign to job before it runs */
	DWORD flags = CREATE_SUSPENDED;

	BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, flags, NULL, NULL, &si, &pi);
	if (!ok) {
		r.tag = 1; r.val._1 = 0;
		return r;
	}

	/* Assign child to the job object so it dies when we die */
	if (pk__job_object) {
		AssignProcessToJobObject(pk__job_object, pi.hProcess);
	}

	/* Resume the child */
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);

	/* Store handle in our table */
	pk_proc_table[slot].hProcess = pi.hProcess;
	pk_proc_table[slot].dwPid = pi.dwProcessId;
	pk_proc_table[slot].active = 1;

	r.tag = 0; r.val._0 = (int32_t)pi.dwProcessId;
	return r;
}

proc_CreateResult pk_proc_create_redirect(pk_str path, pk_slice_str argv,
	pk_str stdin_path,
	pk_str stdout_path, _Bool stdout_append,
	pk_str stderr_path, _Bool stderr_append) {
	proc_CreateResult r;

	if (stdin_path.len == 0 && stdout_path.len == 0 && stderr_path.len == 0) {
		return pk_proc_create(path, argv);
	}

	/* Find a free slot in the process table */
	int slot = -1;
	for (int i = 0; i < PK_MAX_PROCS; i++) {
		if (!pk_proc_table[i].active) { slot = i; break; }
	}
	if (slot < 0) { r.tag = 1; r.val._1 = 0; return r; }

	/* Ensure job object is ready */
	pk__ensure_job_object();

	/* Build command line */
	char cmdline[32768];
	pk_build_cmdline(path, argv, cmdline, sizeof(cmdline));

	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE hIn = INVALID_HANDLE_VALUE;
	HANDLE hOut = INVALID_HANDLE_VALUE;
	HANDLE hErr = INVALID_HANDLE_VALUE;
	HANDLE close_list[3];
	int close_len = 0;

	if (stdin_path.len > 0) {
		char in_path[4096];
		pk_str_to_cstr(stdin_path, in_path, sizeof(in_path));
		hIn = CreateFileA(in_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hIn == INVALID_HANDLE_VALUE) { r.tag = 1; r.val._1 = 0; return r; }
		close_list[close_len++] = hIn;
	} else {
		HANDLE src = GetStdHandle(STD_INPUT_HANDLE);
		if (src == INVALID_HANDLE_VALUE || src == NULL) { r.tag = 1; r.val._1 = 0; return r; }
		hIn = pk_dup_inheritable(src);
		if (hIn == INVALID_HANDLE_VALUE) { r.tag = 1; r.val._1 = 0; return r; }
		close_list[close_len++] = hIn;
	}

	if (stdout_path.len > 0) {
		char out_path[4096];
		DWORD access = stdout_append ? FILE_APPEND_DATA : GENERIC_WRITE;
		DWORD create_disp = stdout_append ? OPEN_ALWAYS : CREATE_ALWAYS;
		pk_str_to_cstr(stdout_path, out_path, sizeof(out_path));
		hOut = CreateFileA(out_path, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa, create_disp, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hOut == INVALID_HANDLE_VALUE) { r.tag = 1; r.val._1 = 0; goto cleanup; }
		close_list[close_len++] = hOut;
	} else {
		HANDLE src = GetStdHandle(STD_OUTPUT_HANDLE);
		if (src == INVALID_HANDLE_VALUE || src == NULL) { r.tag = 1; r.val._1 = 0; goto cleanup; }
		hOut = pk_dup_inheritable(src);
		if (hOut == INVALID_HANDLE_VALUE) { r.tag = 1; r.val._1 = 0; goto cleanup; }
		close_list[close_len++] = hOut;
	}

	if (stderr_path.len > 0 && stdout_path.len > 0 &&
		stdout_append == stderr_append && pk_str_eq(stdout_path, stderr_path)) {
		hErr = hOut;
	} else if (stderr_path.len > 0) {
		char err_path[4096];
		DWORD access = stderr_append ? FILE_APPEND_DATA : GENERIC_WRITE;
		DWORD create_disp = stderr_append ? OPEN_ALWAYS : CREATE_ALWAYS;
		pk_str_to_cstr(stderr_path, err_path, sizeof(err_path));
		hErr = CreateFileA(err_path, access, FILE_SHARE_READ | FILE_SHARE_WRITE,
			&sa, create_disp, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hErr == INVALID_HANDLE_VALUE) { r.tag = 1; r.val._1 = 0; goto cleanup; }
		close_list[close_len++] = hErr;
	} else {
		HANDLE src = GetStdHandle(STD_ERROR_HANDLE);
		if (src == INVALID_HANDLE_VALUE || src == NULL) { r.tag = 1; r.val._1 = 0; goto cleanup; }
		hErr = pk_dup_inheritable(src);
		if (hErr == INVALID_HANDLE_VALUE) { r.tag = 1; r.val._1 = 0; goto cleanup; }
		close_list[close_len++] = hErr;
	}

	STARTUPINFOA si;
	PROCESS_INFORMATION pi;
	for (size_t i = 0; i < sizeof(si); i++) ((uint8_t*)&si)[i] = 0;
	for (size_t i = 0; i < sizeof(pi); i++) ((uint8_t*)&pi)[i] = 0;
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = hIn;
	si.hStdOutput = hOut;
	si.hStdError = hErr;

	/* CREATE_SUSPENDED so we can assign to job before it runs */
	DWORD flags = CREATE_SUSPENDED;

	BOOL ok = CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, flags, NULL, NULL, &si, &pi);
	if (!ok) {
		r.tag = 1; r.val._1 = 0;
		goto cleanup;
	}

	/* Assign child to the job object so it dies when we die */
	if (pk__job_object) {
		AssignProcessToJobObject(pk__job_object, pi.hProcess);
	}

	/* Resume the child */
	ResumeThread(pi.hThread);
	CloseHandle(pi.hThread);

	/* Store handle in our table */
	pk_proc_table[slot].hProcess = pi.hProcess;
	pk_proc_table[slot].dwPid = pi.dwProcessId;
	pk_proc_table[slot].active = 1;

	r.tag = 0; r.val._0 = (int32_t)pi.dwProcessId;

cleanup:
	for (int i = 0; i < close_len; i++) {
		if (close_list[i] != INVALID_HANDLE_VALUE && close_list[i] != NULL) {
			CloseHandle(close_list[i]);
		}
	}
	return r;
}

proc_Status pk_proc_check(int32_t pid) {
	proc_Status r;
	int slot = pk_proc_find_by_pid(pid);
	if (slot < 0) {
		/* Unknown pid — try to open it */
		HANDLE h = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, (DWORD)pid);
		if (!h) { r.tag = 0; r.val._0 = -1; return r; }
		DWORD exitCode;
		if (GetExitCodeProcess(h, &exitCode) && exitCode != STILL_ACTIVE) {
			CloseHandle(h);
			r.tag = 0; r.val._0 = (int32_t)exitCode;
		} else {
			CloseHandle(h);
			r.tag = 1; r.val._1 = 0; /* alive */
		}
		return r;
	}

	DWORD exitCode;
	DWORD waitRet = WaitForSingleObject(pk_proc_table[slot].hProcess, 0);
	if (waitRet == WAIT_OBJECT_0) {
		GetExitCodeProcess(pk_proc_table[slot].hProcess, &exitCode);
		CloseHandle(pk_proc_table[slot].hProcess);
		pk_proc_table[slot].active = 0;
		r.tag = 0; r.val._0 = (int32_t)exitCode;
	} else {
		r.tag = 1; r.val._1 = 0; /* still alive */
	}
	return r;
}

int32_t pk_proc_wait(int32_t pid, int32_t timeout) {
	int slot = pk_proc_find_by_pid(pid);
	HANDLE hProc = NULL;

	if (slot >= 0) {
		hProc = pk_proc_table[slot].hProcess;
	} else {
		hProc = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE | PROCESS_TERMINATE,
			FALSE, (DWORD)pid);
		if (!hProc) return -1;
	}

	DWORD waitMs = (timeout < 0) ? INFINITE : (DWORD)timeout;
	DWORD waitRet = WaitForSingleObject(hProc, waitMs);

	if (waitRet == WAIT_OBJECT_0) {
		DWORD exitCode = 0;
		GetExitCodeProcess(hProc, &exitCode);
		if (slot >= 0) {
			CloseHandle(pk_proc_table[slot].hProcess);
			pk_proc_table[slot].active = 0;
		} else {
			CloseHandle(hProc);
		}
		return (int32_t)exitCode;
	}

	/* Timeout expired — kill the process */
	TerminateProcess(hProc, 1);
	WaitForSingleObject(hProc, INFINITE);
	DWORD exitCode = 0;
	GetExitCodeProcess(hProc, &exitCode);

	if (slot >= 0) {
		CloseHandle(pk_proc_table[slot].hProcess);
		pk_proc_table[slot].active = 0;
	} else {
		CloseHandle(hProc);
	}
	return -9; /* Killed, analogous to -SIGKILL */
}

proc_VoidResult pk_proc_kill(int32_t pid, _Bool hard) {
	proc_VoidResult r;
	int slot = pk_proc_find_by_pid(pid);
	HANDLE hProc = NULL;

	if (slot >= 0) {
		hProc = pk_proc_table[slot].hProcess;
	} else {
		hProc = OpenProcess(PROCESS_TERMINATE, FALSE, (DWORD)pid);
		if (!hProc) { r.tag = 1; r.val._1 = 0; return r; }
	}

	/* Windows has no signals. TerminateProcess is always hard kill.
	 * For soft kill we still terminate but with exit code 15 (SIGTERM). */
	UINT exitCode = hard ? 9 : 15;
	if (TerminateProcess(hProc, exitCode)) {
		if (slot >= 0) {
			WaitForSingleObject(hProc, 5000);
			CloseHandle(pk_proc_table[slot].hProcess);
			pk_proc_table[slot].active = 0;
		} else {
			CloseHandle(hProc);
		}
		r.tag = 0; r.val._0 = 0;
	} else {
		if (slot < 0) CloseHandle(hProc);
		r.tag = 1; r.val._1 = 0;
	}
	return r;
}

__attribute__((noreturn))
void pk_proc_exit(int32_t code) {
	_exit(code);
	__builtin_trap();
}

int32_t pk_proc_getpid(void) {
	return (int32_t)GetCurrentProcessId();
}

void pk_proc_sleep(int32_t ms) {
	if (ms <= 0) return;
	Sleep((DWORD)ms);
}

_Bool pk_proc_isroot(void) {
	/* Simplified: always return false on Windows */
	return 0;
}

proc_VoidResult pk_proc_setprio(int32_t pid, int32_t level) {
	proc_VoidResult r;
	int slot = pk_proc_find_by_pid(pid);
	HANDLE hProc = NULL;

	if (slot >= 0) {
		hProc = pk_proc_table[slot].hProcess;
	} else if (pid == 0 || pid == (int32_t)GetCurrentProcessId()) {
		hProc = GetCurrentProcess();
	} else {
		hProc = OpenProcess(PROCESS_SET_INFORMATION, FALSE, (DWORD)pid);
		if (!hProc) { r.tag = 1; r.val._1 = 0; return r; }
	}

	/* Map Pika levels -2..+2 to Windows priority classes */
	DWORD priClass;
	if (level <= -2)      priClass = IDLE_PRIORITY_CLASS;
	else if (level == -1) priClass = BELOW_NORMAL_PRIORITY_CLASS;
	else if (level == 0)  priClass = NORMAL_PRIORITY_CLASS;
	else if (level == 1)  priClass = ABOVE_NORMAL_PRIORITY_CLASS;
	else                  priClass = HIGH_PRIORITY_CLASS;

	if (SetPriorityClass(hProc, priClass)) {
		r.tag = 0; r.val._0 = 0;
	} else {
		r.tag = 1; r.val._1 = 0;
	}

	/* Close handle if we opened it ourselves */
	if (slot < 0 && hProc != GetCurrentProcess()) {
		CloseHandle(hProc);
	}
	return r;
}

int32_t pk_proc_getprio(int32_t pid) {
	HANDLE hProc = NULL;
	int slot = pk_proc_find_by_pid(pid);

	if (slot >= 0) {
		hProc = pk_proc_table[slot].hProcess;
	} else if (pid == 0 || pid == (int32_t)GetCurrentProcessId()) {
		hProc = GetCurrentProcess();
	} else {
		hProc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, (DWORD)pid);
		if (!hProc) return 0;
	}

	DWORD priClass = GetPriorityClass(hProc);

	/* Close handle if we opened it */
	if (slot < 0 && hProc != GetCurrentProcess()) {
		CloseHandle(hProc);
	}

	/* Map Windows priority classes back to Pika levels -2..+2 */
	if (priClass == IDLE_PRIORITY_CLASS)           return -2;
	if (priClass == BELOW_NORMAL_PRIORITY_CLASS)   return -1;
	if (priClass == NORMAL_PRIORITY_CLASS)          return 0;
	if (priClass == ABOVE_NORMAL_PRIORITY_CLASS)    return 1;
	if (priClass == HIGH_PRIORITY_CLASS)            return 2;
	if (priClass == REALTIME_PRIORITY_CLASS)        return 2;
	return 0;
}
