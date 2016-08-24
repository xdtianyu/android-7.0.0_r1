#define HELP_toybox_musl_nommu_is_broken "When using musl-libc on a nommu system, you'll need to say \"y\" here.\n\nAlthough uclibc lets you detect support for things like fork() and\ndaemon() at compile time, musl intentionally includes broken versions\nthat always return -ENOSYS on nommu systems, and goes out of its way\nto prevent any cross-compile compatible compile-time probes for a\nnommu system.\n\nMusl does this despite the fact that a nommu system can't even run\nstandard ELF binaries, and requires specially packaged executables.\n(You can't even check a #define to see that you're building against\nmusl, due to its maintainer's policy that musl never has bugs that\nrequire workarounds.)\n\nSo our only choice is to manually provide a musl nommu bug workaround\nyou can manually select to enable (larger, slower) nommu support with\nmusl.\n\nYou don't need this for uClibc, we have a compile time probe that\nautodetects nommu support there.\n\n"

#define HELP_toybox_uid_usr "When commands like useradd/groupadd allocate user IDs, start here.\n\n"

#define HELP_toybox_uid_sys "When commands like useradd/groupadd allocate system IDs, start here.\n\n"

#define HELP_toybox_debug "Enable extra checks for debugging purposes. All of them catch\nthings that can only go wrong at development time, not runtime.\n\n"

#define HELP_toybox_norecurse "When one toybox command calls another, usually it just calls the new\ncommand's main() function rather than searching the $PATH and calling\nexec on another file (which is much slower).\n\nThis disables that optimization, so toybox will run external commands\n       even when it has a built-in version of that command. This requires\n       toybox symlinks to be installed in the $PATH, or re-invoking the\n       \"toybox\" multiplexer command by name.\n\n"

#define HELP_toybox_free "When a program exits, the operating system will clean up after it\n(free memory, close files, etc). To save size, toybox usually relies\non this behavior. If you're running toybox under a debugger or\nwithout a real OS (ala newlib+libgloss), enable this to make toybox\nclean up after itself.\n\n"

#define HELP_toybox_i18n "Support for UTF-8 character sets, and some locale support.\n\n"

#define HELP_toybox_help_dashdash "Support --help argument in all commands, even ones with a NULL\noptstring. Produces the same output as \"help command\".\n\n"

#define HELP_toybox_help "Include help text for each command.\n\n"

#define HELP_toybox_float "Include floating point support infrastructure and commands that\nrequire it.\n\n"

#define HELP_toybox_smack "Include SMACK options in commands like ls for systems like Tizen.\n\n\n"

#define HELP_toybox_selinux "Include SELinux options in commands such as ls, and add\nSELinux-specific commands such as chcon to the Android menu.\n\n"

#define HELP_toybox_lsm_none "Don't try to achieve \"watertight\" by plugging the holes in a\ncollander, instead use conventional unix security (and possibly\nLinux Containers) for a simple straightforward system.\n\n"

#define HELP_toybox_suid "Support for the Set User ID bit, to install toybox suid root and drop\npermissions for commands which do not require root access. To use\nthis change ownership of the file to the root user and set the suid\nbit in the file permissions:\n\nchown root:root toybox; chmod +s toybox\n\nprompt \"Security Blanket\"\ndefault TOYBOX_LSM_NONE\nhelp\nSelect a Linux Security Module to complicate your system\nuntil you can't find holes in it.\n\n"

#define HELP_toybox "usage: toybox [--long | --version | [command] [arguments...]]\n\nWith no arguments, shows available commands. First argument is\nname of a command to run, followed by any arguments to that command.\n\n--long	Show path to each command\n--version	Show toybox version\n\nTo install command symlinks, try:\n  for i in $(/bin/toybox --long); do ln -s /bin/toybox $i; done\n\n"

#define HELP_setprop "usage: setprop NAME VALUE\n\nSets an Android system property.\n\n"

#define HELP_setenforce "usage: setenforce [enforcing|permissive|1|0]\n\nSets whether SELinux is enforcing (1) or permissive (0).\n\n"

#define HELP_runcon "usage: runcon CONTEXT COMMAND [ARGS...]\n\nRun a command in a specified security context.\n\n"

#define HELP_restorecon "usage: restorecon [-D] [-F] [-R] [-n] [-v] FILE...\n\nRestores the default security contexts for the given files.\n\n-D	apply to /data/data too\n-F	force reset\n-R	recurse into directories\n-n	don't make any changes; useful with -v to see what would change\n-v	verbose: show any changes\n\n"

#define HELP_load_policy "usage: load_policy FILE\n\nLoad the specified policy file.\n\n"

#define HELP_getprop "usage: getprop [NAME [DEFAULT]]\n\nGets an Android system property, or lists them all.\n\n"

#define HELP_getenforce "usage: getenforce\n\nShows whether SELinux is disabled, enforcing, or permissive.\n\n"

#define HELP_test_scankey "usage: test_scankey\n\nMove a letter around the screen. Hit ESC to exit.\n\n\n"

#define HELP_test_many_options "usage: test_many_options -[a-zA-Z]\n\nPrint the optflags value of the command arguments, in hex.\n\n"

#define HELP_test_human_readable "usage: test_human_readable [-sbi] NUMBER\n\n"

#define HELP_skeleton_alias "usage: skeleton_alias [-dq] [-b NUMBER]\n\nExample of a second command with different arguments in the same source\nfile as the first. This allows shared infrastructure not added to lib/.\n\n"

#define HELP_skeleton "usage: skeleton [-a] [-b STRING] [-c NUMBER] [-d LIST] [-e COUNT] [...]\n\nTemplate for new commands. You don't need this.\n\nWhen creating a new command, copy this file and delete the parts you\ndon't need. Be sure to replace all instances of \"skeleton\" (upper and lower\ncase) with your new command name.\n\nFor simple commands, \"hello.c\" is probably a better starting point.\n\n"

#define HELP_hello "usage: hello [-s]\n\nA hello world program.  You don't need this.\n\nMostly used as a simple template for adding new commands.\nOccasionally nice to smoketest kernel booting via \"init=/usr/bin/hello\".\n\n"

#define HELP_umount "usage: umount [-a [-t TYPE[,TYPE...]]] [-vrfD] [DIR...]\n\nUnmount the listed filesystems.\n\n-a	Unmount all mounts in /proc/mounts instead of command line list\n-D  Don't free loopback device(s).\n-f  Force unmount.\n-l  Lazy unmount (detach from filesystem now, close when last user does).\n-n	Don't use /proc/mounts\n-r  Remount read only if unmounting fails.\n-t	Restrict \"all\" to mounts of TYPE (or use \"noTYPE\" to skip)\n-v	Verbose\n\n\n"

#define HELP_sync "usage: sync\n\nWrite pending cached data to disk (synchronize), blocking until done.\n\n"

#define HELP_su "usage: su [-lmp] [-c CMD] [-s SHELL] [USER [ARGS...]]\n\nSwitch to user (or root) and run shell (with optional command line).\n\n-s	shell to use\n-c	command to pass to shell with -c\n-l	login shell\n-(m|p)	preserve environment\n\n"

#define HELP_seq "usage: seq [-w|-f fmt_str] [-s sep_str] [first] [increment] last\n\nCount from first to last, by increment. Omitted arguments default\nto 1. Two arguments are used as first and last. Arguments can be\nnegative or floating point.\n\n-f	Use fmt_str as a printf-style floating point format string\n-s	Use sep_str as separator, default is a newline character\n-w	Pad to equal width with leading zeroes.\n\n"

#define HELP_pidof "usage: pidof [-s] [-o omitpid[,omitpid...]] [NAME]...\n\nPrint the PIDs of all processes with the given names.\n\n-s	single shot, only return one pid.\n-o	omit PID(s)\n\n"

#define HELP_passwd_sad "Password changes are checked to make sure they don't include the entire\nusername (but not a subset of it), and the entire previous password\n(but changing password1, password2, password3 is fine). This heuristic\naccepts \"aaaaaa\" as a password.\n\n"

#define HELP_passwd "usage: passwd [-a ALGO] [-dlu] <account name>\n\nupdate user's authentication tokens. Default : current user\n\n-a ALGO	Encryption method (des, md5, sha256, sha512) default: des\n-d		Set password to ''\n-l		Lock (disable) account\n-u		Unlock (enable) account\n\n"

#define HELP_mount "usage: mount [-afFrsvw] [-t TYPE] [-o OPTIONS...] [[DEVICE] DIR]\n\nMount new filesystem(s) on directories. With no arguments, display existing\nmounts.\n\n-a	mount all entries in /etc/fstab (with -t, only entries of that TYPE)\n-O	only mount -a entries that have this option\n-f	fake it (don't actually mount)\n-r	read only (same as -o ro)\n-w	read/write (default, same as -o rw)\n-t	specify filesystem type\n-v	verbose\n\nOPTIONS is a comma separated list of options, which can also be supplied\nas --longopts.\n\nThis mount autodetects loopback mounts (a file on a directory) and\nbind mounts (file on file, directory on directory), so you don't need\nto say --bind or --loop. You can also \"mount -a /path\" to mount everything\nin /etc/fstab under /path, even if it's noauto.\n\n\n"

#define HELP_mktemp "usage: mktemp [-dqu] [-p DIR] [TEMPLATE]\n\nSafely create a new file \"DIR/TEMPLATE\" and print its name.\n\n-d	Create directory instead of file (--directory)\n-p	Put new file in DIR (--tmpdir)\n-q	Quiet, no error messages\n-u	Don't create anything, just print what would be created\n\nEach X in TEMPLATE is replaced with a random printable character. The\ndefault TEMPLATE is tmp.XXXXXX, and the default DIR is $TMPDIR if set,\nelse \"/tmp\".\n\n"

#define HELP_mknod_z "usage: mknod [-Z CONTEXT] ...\n\n-Z	Set security context to created file\n\n"

#define HELP_mknod "usage: mknod [-m MODE] NAME TYPE [MAJOR MINOR]\n\nCreate a special file NAME with a given type. TYPE is b for block device,\nc or u for character device, p for named pipe (which ignores MAJOR/MINOR).\n\n-m	Mode (file permissions) of new device, in octal or u+x format\n\n"

#define HELP_sha1sum "usage: sha1sum [FILE]...\n\ncalculate sha1 hash for each input file, reading from stdin if none.\nOutput one hash (20 hex digits) for each input file, followed by\nfilename.\n\n-b	brief (hash only, no filename)\n\n"

#define HELP_md5sum "usage: md5sum [FILE]...\n\nCalculate md5 hash for each input file, reading from stdin if none.\nOutput one hash (16 hex digits) for each input file, followed by\nfilename.\n\n-b	brief (hash only, no filename)\n\n"

#define HELP_killall "usage: killall [-l] [-iqv] [-SIGNAL|-s SIGNAL] PROCESS_NAME...\n\nSend a signal (default: TERM) to all processes with the given names.\n\n-i	ask for confirmation before killing\n-l	print list of all available signals\n-q	don't print any warnings or error messages\n-s	send SIGNAL instead of SIGTERM\n-v	report if the signal was successfully sent\n\n"

#define HELP_hostname "usage: hostname [newname]\n\nGet/Set the current hostname\n\n"

#define HELP_dmesg "usage: dmesg [-c] [-r|-t] [-n LEVEL] [-s SIZE]\n\nPrint or control the kernel ring buffer.\n\n-c	Clear the ring buffer after printing\n-n	Set kernel logging LEVEL (1-9)\n-r	Raw output (with <level markers>)\n-s	Show the last SIZE many bytes\n-t	Don't print kernel's timestamps\n\n"

#define HELP_yes "usage: yes [args...]\n\nRepeatedly output line until killed. If no args, output 'y'.\n\n\n"

#define HELP_xxd "usage: xxd [-c n] [-g n] [-l n] [-p] [-r] [file]\n\nHexdump a file to stdout.  If no file is listed, copy from stdin.\nFilename \"-\" is a synonym for stdin.\n\n-c n	Show n bytes per line (default 16).\n-g n	Group bytes by adding a ' ' every n bytes (default 2).\n-l n	Limit of n bytes before stopping (default is no limit).\n-p	Plain hexdump (30 bytes/line, no grouping).\n-r	Reverse operation: turn a hexdump into a binary file.\n\n"

#define HELP_which "usage: which [-a] filename ...\n\nSearch $PATH for executable files matching filename(s).\n\n-a	Show all matches\n\n"

#define HELP_w "usage: w\n\nShow who is logged on and since how long they logged in.\n\n"

#define HELP_vmstat "usage: vmstat [-n] [DELAY [COUNT]]\n\nPrint virtual memory statistics, repeating each DELAY seconds, COUNT times.\n(With no DELAY, prints one line. With no COUNT, repeats until killed.)\n\nShow processes running and blocked, kilobytes swapped, free, buffered, and\ncached, kilobytes swapped in and out per second, file disk blocks input and\noutput per second, interrupts and context switches per second, percent\nof CPU time spent running user code, system code, idle, and awaiting I/O.\nFirst line is since system started, later lines are since last line.\n\n-n	Display the header only once\n\n"

#define HELP_vconfig "usage: vconfig COMMAND [OPTIONS]\n\nCreate and remove virtual ethernet devices\n\nadd             [interface-name] [vlan_id]\nrem             [vlan-name]\nset_flag        [interface-name] [flag-num]       [0 | 1]\nset_egress_map  [vlan-name]      [skb_priority]   [vlan_qos]\nset_ingress_map [vlan-name]      [skb_priority]   [vlan_qos]\nset_name_type   [name-type]\n\n"

#define HELP_usleep "usage: usleep MICROSECONDS\n\nPause for MICROSECONDS microseconds.\n\n"

#define HELP_uptime "usage: uptime\n\nTell how long the system has been running and the system load\naverages for the past 1, 5 and 15 minutes.\n\n"

#define HELP_truncate "usage: truncate [-c] -s SIZE file...\n\nSet length of file(s), extending sparsely if necessary.\n\n-c	Don't create file if it doesn't exist.\n-s	New size (with optional prefix and suffix)\n\nSIZE prefix: + add, - subtract, < shrink to, > expand to,\n             / multiple rounding down, % multiple rounding up\nSIZE suffix: k=1024, m=1024^2, g=1024^3, t=1024^4, p=1024^5, e=1024^6\n\n"

#define HELP_timeout "usage: timeout [-k LENGTH] [-s SIGNAL] LENGTH COMMAND...\n\nRun command line as a child process, sending child a signal if the\ncommand doesn't exit soon enough.\n\nLength can be a decimal fraction. An optional suffix can be \"m\"\n(minutes), \"h\" (hours), \"d\" (days), or \"s\" (seconds, the default).\n\n-s	Send specified signal (default TERM)\n-k	Send KILL signal if child still running this long after first signal.\n-v	Verbose\n\n"

#define HELP_taskset "usage: taskset [-ap] [mask] [PID | cmd [args...]]\n\nLaunch a new task which may only run on certain processors, or change\nthe processor affinity of an exisitng PID.\n\nMask is a hex string where each bit represents a processor the process\nis allowed to run on. PID without a mask displays existing affinity.\n\n-p	Set/get the affinity of given PID instead of a new command.\n-a	Set/get the affinity of all threads of the PID.\n\n"

#define HELP_nproc "usage: nproc [--all]\n\nPrint number of processors.\n\n--all	Show all processors, not just ones this task can run on.\n\n"

#define HELP_tac "usage: tac [FILE...]\n\nOutput lines in reverse order.\n\n"

#define HELP_sysctl "usage: sysctl [-aAeNnqw] [-p [FILE] | KEY[=VALUE]...]\n\nRead/write system control data (under /proc/sys).\n\n-a,A	Show all values\n-e	Don't warn about unknown keys\n-N	Don't print key values\n-n	Don't print key names\n-p	Read values from FILE (default /etc/sysctl.conf)\n-q	Don't show value after write\n-w	Only write values (object to reading)\n\n"

#define HELP_switch_root "usage: switch_root [-c /dev/console] NEW_ROOT NEW_INIT...\n\nUse from PID 1 under initramfs to free initramfs, chroot to NEW_ROOT,\nand exec NEW_INIT.\n\n-c	Redirect console to device in NEW_ROOT\n-h	Hang instead of exiting on failure (avoids kernel panic)\n\n"

#define HELP_swapon "usage: swapon [-d] [-p priority] filename\n\nEnable swapping on a given device/file.\n\n-d	Discard freed SSD pages\n\n"

#define HELP_swapoff "usage: swapoff swapregion\n\nDisable swapping on a given swapregion.\n\n"

#define HELP_stat "usage: stat [-f] [-c FORMAT] FILE...\n\nDisplay status of files or filesystems.\n\n-f display filesystem status instead of file status\n-c Output specified FORMAT string instead of default\n\nThe valid format escape sequences for files:\n%a  Access bits (octal) |%A  Access bits (flags)|%b  Blocks allocated\n%B  Bytes per block     |%d  Device ID (dec)    |%D  Device ID (hex)\n%f  All mode bits (hex) |%F  File type          |%g  Group ID\n%G  Group name          |%h  Hard links         |%i  Inode\n%n  Filename            |%N  Long filename      |%o  I/O block size\n%s  Size (bytes)        |%u  User ID            |%U  User name\n%x  Access time         |%X  Access unix time   |%y  File write time\n%Y  File write unix time|%z  Dir change time    |%Z  Dir change unix time\n\nThe valid format escape sequences for filesystems:\n%a  Available blocks    |%b  Total blocks       |%c  Total inodes\n%d  Free inodes         |%f  Free blocks        |%i  File system ID\n%l  Max filename length |%n  File name          |%s  Fragment size\n%S  Best transfer size  |%t  Filesystem type    |%T  Filesystem type name\n\n"

#define HELP_shred "usage: shred [-fuz] [-n COUNT] [-s SIZE] FILE...\n\nSecurely delete a file by overwriting its contents with random data.\n\n-f        Force (chmod if necessary)\n-n COUNT  Random overwrite iterations (default 1)\n-o OFFSET Start at OFFSET\n-s SIZE   Use SIZE instead of detecting file size\n-u        unlink (actually delete file when done)\n-x        Use exact size (default without -s rounds up to next 4k)\n-z        zero at end\n\nNote: data journaling filesystems render this command useless, you must\noverwrite all free space (fill up disk) to erase old data on those.\n\n"

#define HELP_setsid "usage: setsid [-t] command [args...]\n\nRun process in a new session.\n\n-t	Grab tty (become foreground process, receiving keyboard signals)\n\n"

#define HELP_rmmod "usage: rmmod [-wf] [MODULE]\n\nUnload the module named MODULE from the Linux kernel.\n-f	Force unload of a module\n-w	Wait until the module is no longer used.\n\n\n"

#define HELP_rfkill "Usage: rfkill COMMAND [DEVICE]\n\nEnable/disable wireless devices.\n\nCommands:\nlist [DEVICE]   List current state\nblock DEVICE    Disable device\nunblock DEVICE  Enable device\n\nDEVICE is an index number, or one of:\nall, wlan(wifi), bluetooth, uwb(ultrawideband), wimax, wwan, gps, fm.\n\n"

#define HELP_rev "usage: rev [FILE...]\n\nOutput each line reversed, when no files are given stdin is used.\n\n"

#define HELP_reset "usage: reset\n\nreset the terminal\n\n"

#define HELP_reboot "usage: reboot/halt/poweroff [-fn]\n\nRestart, halt or powerdown the system.\n\n-f	Don't signal init\n-n	Don't sync before stopping the system.\n\n"

#define HELP_realpath "usage: realpath FILE...\n\nDisplay the canonical absolute pathname\n\n"

#define HELP_readlink "usage: readlink FILE\n\nWith no options, show what symlink points to, return error if not symlink.\n\nOptions for producing cannonical paths (all symlinks/./.. resolved):\n\n-e	cannonical path to existing entry (fail if missing)\n-f	full path (fail if directory missing)\n-n	no trailing newline\n-q	quiet (no output, just error code)\n\n"

#define HELP_readahead "usage: readahead FILE...\n\nPreload files into disk cache.\n\n"

#define HELP_pwdx "usage: pwdx PID...\n\nPrint working directory of processes listed on command line.\n\n"

#define HELP_printenv "usage: printenv [-0] [env_var...]\n\nPrint environment variables.\n\n-0	Use \\0 as delimiter instead of \\n\n\n"

#define HELP_pmap "usage: pmap [-xq] [pids...]\n\nReports the memory map of a process or processes.\n\n-x Show the extended format.\n-q Do not display some header/footer lines.\n\n"

#define HELP_pivot_root "usage: pivot_root OLD NEW\n\nSwap OLD and NEW filesystems (as if by simultaneous mount --move), and\nmove all processes with chdir or chroot under OLD into NEW (including\nkernel threads) so OLD may be unmounted.\n\nThe directory NEW must exist under OLD. This doesn't work on initramfs,\nwhich can't be moved (about the same way PID 1 can't be killed; see\nswitch_root instead).\n\n"

#define HELP_partprobe "usage: partprobe DEVICE...\n\nTell the kernel about partition table changes\n\nAsk the kernel to re-read the partition table on the specified devices.\n\n"

#define HELP_oneit "usage: oneit [-p] [-c /dev/tty0] command [...]\n\nSimple init program that runs a single supplied command line with a\ncontrolling tty (so CTRL-C can kill it).\n\n-c	Which console device to use (/dev/console doesn't do CTRL-C, etc).\n-p	Power off instead of rebooting when command exits.\n-r	Restart child when it exits.\n-3	Write 32 bit PID of each exiting reparented process to fd 3 of child.\n	(Blocking writes, child must read to avoid eventual deadlock.)\n\nSpawns a single child process (because PID 1 has signals blocked)\nin its own session, reaps zombies until the child exits, then\nreboots the system (or powers off with -p, or restarts the child with -r).\n\nResponds to SIGUSR1 by halting the system, SIGUSR2 by powering off,\nand SIGTERM or SIGINT reboot.\n\n"

#define HELP_nsenter "usage: nsenter [-t pid] [-F] [-i] [-m] [-n] [-p] [-u] [-U] COMMAND...\n\nRun COMMAND in an existing (set of) namespace(s).\n\n-t  PID to take namespaces from    (--target)\n-F  don't fork, even if -p is used (--no-fork)\n\nThe namespaces to switch are:\n\n-i	SysV IPC: message queues, semaphores, shared memory (--ipc)\n-m	Mount/unmount tree (--mount)\n-n	Network address, sockets, routing, iptables (--net)\n-p	Process IDs and init, will fork unless -F is used (--pid)\n-u	Host and domain names (--uts)\n-U	UIDs, GIDs, capabilities (--user)\n\nIf -t isn't specified, each namespace argument must provide a path\nto a namespace file, ala \"-i=/proc/$PID/ns/ipc\"\n\n"

#define HELP_unshare "usage: unshare [-imnpuUr] COMMAND...\n\nCreate new container namespace(s) for this process and its children, so\nsome attribute is not shared with the parent process.\n\n-f  Fork command in the background (--fork)\n-i	SysV IPC (message queues, semaphores, shared memory) (--ipc)\n-m	Mount/unmount tree (--mount)\n-n	Network address, sockets, routing, iptables (--net)\n-p	Process IDs and init (--pid)\n-r	Become root (map current euid/egid to 0/0, implies -U) (--map-root-user)\n-u	Host and domain names (--uts)\n-U	UIDs, GIDs, capabilities (--user)\n\nA namespace allows a set of processes to have a different view of the\nsystem than other sets of processes.\n\n"

#define HELP_netcat_listen_tty "usage: netcat [-t]\n\n-t	allocate tty (must come before -l or -L)\n\n"

#define HELP_netcat "usage: netcat [-lL COMMAND...] [-u] [-wpq #] [-s addr] {IPADDR PORTNUM|-f FILENAME}\n\n-L	listen for multiple incoming connections (server mode).\n-f	use FILENAME (ala /dev/ttyS0) instead of network\n-l	listen for one incoming connection.\n-p	local port number\n-q	SECONDS quit this many seconds after EOF on stdin.\n-s	local ipv4 address\n-w	SECONDS timeout for connection\n\nUse \"stty 115200 -F /dev/ttyS0 && stty raw -echo -ctlecho\" with\nnetcat -f to connect to a serial port.\n\nThe command line after -l or -L is executed to handle each incoming\nconnection. If none, the connection is forwarded to stdin/stdout.\n\nFor a quick-and-dirty server, try something like:\nnetcat -s 127.0.0.1 -p 1234 -tL /bin/bash -l\n"

#define HELP_nbd_client "usage: nbd-client [-ns] HOST PORT DEVICE\n\n-n	Do not fork into background\n-s	nbd swap support (lock server into memory)\n\n"

#define HELP_mountpoint "usage: mountpoint [-q] [-d] directory\n       mountpoint [-q] [-x] device\n\n-q	Be quiet, return zero if directory is a mountpoint\n-d	Print major/minor device number of the directory\n-x	Print major/minor device number of the block device\n\n"

#define HELP_modinfo "usage: modinfo [-0] [-b basedir] [-k kernrelease] [-F field] [modulename...]\n\nDisplay module fields for all specified modules, looking in\n<basedir>/lib/modules/<kernrelease>/ (kernrelease defaults to uname -r).\n\n"

#define HELP_mkswap "usage: mkswap [-L LABEL] DEVICE\n\nSets up a Linux swap area on a device or file.\n\n"

#define HELP_mkpasswd "usage: mkpasswd [-P FD] [-m TYPE] [-S SALT] [PASSWORD] [SALT]\n\nCrypt PASSWORD using crypt(3)\n\n-P FD   Read password from file descriptor FD\n-m TYPE Encryption method (des, md5, sha256, or sha512; default is des)\n-S SALT\n\n"

#define HELP_mix "usage: mix [-d DEV] [-c CHANNEL] [-l VOL] [-r RIGHT]\n\nList OSS sound channels (module snd-mixer-oss), or set volume(s).\n\n-c CHANNEL	Set/show volume of CHANNEL (default first channel found)\n-d DEV		Device node (default /dev/mixer)\n-l VOL		Volume level\n-r RIGHT	Volume of right stereo channel (with -r, -l sets left volume)\n\n"

#define HELP_makedevs "usage: makedevs [-d device_table] rootdir\n\nCreate a range of special files as specified in a device table.\n\n-d	file containing device table (default reads from stdin)\n\nEach line of of the device table has the fields:\n<name> <type> <mode> <uid> <gid> <major> <minor> <start> <increment> <count>\nWhere name is the file name, and type is one of the following:\n\nb	Block device\nc	Character device\nd	Directory\nf	Regular file\np	Named pipe (fifo)\n\nOther fields specify permissions, user and group id owning the file,\nand additional fields for device special files. Use '-' for blank entries,\nunspecified fields are treated as '-'.\n\n"

#define HELP_lsusb "usage: lsusb\n\nList USB hosts/devices.\n\n"

#define HELP_lspci_text "usage: lspci [-n] [-i FILE ]\n\n-n	Numeric output (repeat for readable and numeric)\n-i	PCI ID database (default /usr/share/misc/pci.ids)\n\n\n"

#define HELP_lspci "usage: lspci [-ekm]\n\nList PCI devices.\n\n-e	Print all 6 digits in class\n-k	Print kernel driver\n-m	Machine parseable format\n\n"

#define HELP_lsmod "usage: lsmod\n\nDisplay the currently loaded modules, their sizes and their dependencies.\n\n"

#define HELP_chattr "usage: chattr [-R] [-+=AacDdijsStTu] [-v version] [File...]\n\nChange file attributes on a Linux second extended file system.\n\nOperators:\n  '-' Remove attributes.\n  '+' Add attributes.\n  '=' Set attributes.\n\nAttributes:\n  A  Don't track atime.\n  a  Append mode only.\n  c  Enable compress.\n  D  Write dir contents synchronously.\n  d  Don't backup with dump.\n  i  Cannot be modified (immutable).\n  j  Write all data to journal first.\n  s  Zero disk storage when deleted.\n  S  Write file contents synchronously.\n  t  Disable tail-merging of partial blocks with other files.\n  u  Allow file to be undeleted.\n  -R Recurse.\n  -v Set the file's version/generation number.\n\n\n"

#define HELP_lsattr "usage: lsattr [-Radlv] [Files...]\n\nList file attributes on a Linux second extended file system.\n\n-R Recursively list attributes of directories and their contents.\n-a List all files in directories, including files that start with '.'.\n-d List directories like other files, rather than listing their contents.\n-l List long flag names.\n-v List the file's version/generation number.\n\n"

#define HELP_losetup "usage: losetup [-cdrs] [-o OFFSET] [-S SIZE] {-d DEVICE...|-j FILE|-af|{DEVICE FILE}}\n\nAssociate a loopback device with a file, or show current file (if any)\nassociated with a loop device.\n\nInstead of a device:\n-a	Iterate through all loopback devices\n-f	Find first unused loop device (may create one)\n-j	Iterate through all loopback devices associated with FILE\n\nexisting:\n-c	Check capacity (file size changed)\n-d	Detach loopback device\n\nnew:\n-s	Show device name (alias --show)\n-o	Start assocation at OFFSET into FILE\n-r	Read only\n-S	Limit SIZE of loopback association (alias --sizelimit)\n\n"

#define HELP_login "usage: login [-p] [-h host] [-f USERNAME] [USERNAME]\n\nLog in as a user, prompting for username and password if necessary.\n\n-p	Preserve environment\n-h	The name of the remote host for this login\n-f	login as USERNAME without authentication\n\n"

#define HELP_iorenice "usage: iorenice PID [CLASS] [PRIORITY]\n\nDisplay or change I/O priority of existing process. CLASS can be\n\"rt\" for realtime, \"be\" for best effort, \"idle\" for only when idle, or\n\"none\" to leave it alone. PRIORITY can be 0-7 (0 is highest, default 4).\n\n"

#define HELP_ionice "usage: ionice [-t] [-c CLASS] [-n LEVEL] [COMMAND...|-p PID]\n\nChange the I/O scheduling priority of a process. With no arguments\n(or just -p), display process' existing I/O class/priority.\n\n-c	CLASS = 1-3: 1(realtime), 2(best-effort, default), 3(when-idle)\n-n	LEVEL = 0-7: (0 is highest priority, default = 5)\n-p	Affect existing PID instead of spawning new child\n-t	Ignore failure to set I/O priority\n\nSystem default iopriority is generally -c 2 -n 4.\n\n"

#define HELP_insmod "usage: insmod MODULE [MODULE_OPTIONS]\n\nLoad the module named MODULE passing options if given.\n\n"

#define HELP_inotifyd "usage: inotifyd PROG FILE[:MASK] ...\n\nWhen a filesystem event matching MASK occurs to a FILE, run PROG as:\n\n  PROG EVENTS FILE [DIRFILE]\n\nIf PROG is \"-\" events are sent to stdout.\n\nThis file is:\n  a  accessed    c  modified    e  metadata change  w  closed (writable)\n  r  opened      D  deleted     M  moved            0  closed (unwritable)\n  u  unmounted   o  overflow    x  unwatchable\n\nA file in this directory is:\n  m  moved in    y  moved out   n  created          d  deleted\n\nWhen x event happens for all FILEs, inotifyd exits (after waiting for PROG).\n\n"

#define HELP_ifconfig "usage: ifconfig [-a] [INTERFACE [ACTION...]]\n\nDisplay or configure network interface.\n\nWith no arguments, display active interfaces. First argument is interface\nto operate on, one argument by itself displays that interface.\n\n-a	Show all interfaces, not just active ones\n\nAdditional arguments are actions to perform on the interface:\n\nADDRESS[/NETMASK] - set IPv4 address (1.2.3.4/5)\ndefault - unset ipv4 address\nadd|del ADDRESS[/PREFIXLEN] - add/remove IPv6 address (1111::8888/128)\nup - enable interface\ndown - disable interface\n\nnetmask|broadcast|pointopoint ADDRESS - set more IPv4 characteristics\nhw ether|infiniband ADDRESS - set LAN hardware address (AA:BB:CC...)\ntxqueuelen LEN - number of buffered packets before output blocks\nmtu LEN - size of outgoing packets (Maximum Transmission Unit)\n\nFlags you can set on an interface (or -remove by prefixing with -):\narp - don't use Address Resolution Protocol to map LAN routes\npromisc - don't discard packets that aren't to this LAN hardware address\nmulticast - force interface into multicast mode if the driver doesn't\nallmulti - promisc for multicast packets\n\nObsolete fields included for historical purposes:\nirq|io_addr|mem_start ADDR - micromanage obsolete hardware\noutfill|keepalive INTEGER - SLIP analog dialup line quality monitoring\nmetric INTEGER - added to Linux 0.9.10 with comment \"never used\", still true\n\n"

#define HELP_hwclock "usage: hwclock [-rswtluf]\n\n-f FILE Use specified device file instead of /dev/rtc (--rtc)\n-l      Hardware clock uses localtime (--localtime)\n-r      Show hardware clock time (--show)\n-s      Set system time from hardware clock (--hctosys)\n-t      Set the system time based on the current timezone (--systz)\n-u      Hardware clock uses UTC (--utc)\n-w      Set hardware clock from system time (--systohc)\n\n"

#define HELP_hostid "usage: hostid\n\nPrint the numeric identifier for the current host.\n\n"

#define HELP_hexedit "usage: hexedit FILENAME\n\nHexadecimal file editor. All changes are written to disk immediately.\n\n-r	Read only (display but don't edit)\n\nKeys:\nArrows        Move left/right/up/down by one line/column\nPg Up/Pg Dn   Move up/down by one page\n0-9, a-f      Change current half-byte to hexadecimal value\nu             Undo\nq/^c/^d/<esc> Quit\n\n"

#define HELP_help "usage: help [-ah] [command]\n\nShow usage information for toybox commands.\nRun \"toybox\" with no arguments for a list of available commands.\n\n-h	HTML output\n-a	All commands\n"

#define HELP_fsync "usage: fsync [-d] [FILE...]\n\nSynchronize a file's in-core state with storage device.\n\n-d	Avoid syncing metadata.\n\n"

#define HELP_fsfreeze "usage: fsfreeze {-f | -u} MOUNTPOINT\n\nFreeze or unfreeze a filesystem.\n\n-f	freeze\n-u	unfreeze\n\n"

#define HELP_freeramdisk "usage: freeramdisk [RAM device]\n\nFree all memory allocated to specified ramdisk\n\n"

#define HELP_free "usage: free [-bkmgt]\n\nDisplay the total, free and used amount of physical memory and swap space.\n\n-bkmgt	Output units (default is bytes)\n-h	Human readable\n\n"

#define HELP_flock "usage: flock [-sxun] fd\n\nManage advisory file locks.\n\n-s	Shared lock.\n-x	Exclusive lock (default).\n-u	Unlock.\n-n	Non-blocking: fail rather than wait for the lock.\n\n"

#define HELP_fallocate "usage: fallocate [-l size] file\n\nTell the filesystem to allocate space for a file.\n\n"

#define HELP_factor "usage: factor NUMBER...\n\nFactor integers.\n\n"

#define HELP_eject "usage: eject [-stT] [DEVICE]\n\nEject DEVICE or default /dev/cdrom\n\n-s	SCSI device\n-t	Close tray\n-T	Open/close tray (toggle).\n\n"

#define HELP_unix2dos "usage: unix2dos [FILE...]\n\nConvert newline format from unix \"\\n\" to dos \"\\r\\n\".\nIf no files listed copy from stdin, \"-\" is a synonym for stdin.\n\n"

#define HELP_dos2unix "usage: dos2unix [FILE...]\n\nConvert newline format from dos \"\\r\\n\" to unix \"\\n\".\nIf no files listed copy from stdin, \"-\" is a synonym for stdin.\n\n"

#define HELP_count "usage: count\n\nCopy stdin to stdout, displaying simple progress indicator to stderr.\n\n"

#define HELP_clear "Clear the screen.\n\n"

#define HELP_chvt "usage: chvt N\n\nChange to virtual terminal number N. (This only works in text mode.)\n\nVirtual terminals are the Linux VGA text mode displays, ordinarily\nswitched between via alt-F1, alt-F2, etc. Use ctrl-alt-F1 to switch\nfrom X to a virtual terminal, and alt-F6 (or F7, or F8) to get back.\n\n"

#define HELP_chroot "usage: chroot NEWPATH [commandline...]\n\nRun command within a new root directory. If no command, run /bin/sh.\n\n"

#define HELP_chcon "usage: chcon [-hRv] CONTEXT FILE...\n\nChange the SELinux security context of listed file[s].\n\n-h change symlinks instead of what they point to.\n-R recurse into subdirectories.\n-v verbose output.\n\n"

#define HELP_bzcat "usage: bzcat [FILE...]\n\nDecompress listed files to stdout. Use stdin if no files listed.\n\n"

#define HELP_bunzip2 "usage: bunzip2 [-cftkv] [FILE...]\n\nDecompress listed files (file.bz becomes file) deleting archive file(s).\nRead from stdin if no files listed.\n\n-c	force output to stdout\n-f	force decompression. (If FILE doesn't end in .bz, replace original.)\n-k	keep input files (-c and -t imply this)\n-t  test integrity\n-v	verbose\n\n"

#define HELP_blockdev "usage: blockdev --OPTION... BLOCKDEV...\n\nCall ioctl(s) on each listed block device\n\nOPTIONs:\n--setro		Set read only\n--setrw		Set read write\n--getro		Get read only\n--getss		Get sector size\n--getbsz	Get block size\n--setbsz	BYTES	Set block size\n--getsz		Get device size in 512-byte sectors\n--getsize	Get device size in sectors (deprecated)\n--getsize64	Get device size in bytes\n--flushbufs	Flush buffers\n--rereadpt	Reread partition table\n\n"

#define HELP_fstype "usage: fstype DEV...\n\nPrints type of filesystem on a block device or image.\n\n"

#define HELP_blkid "usage: blkid DEV...\n\nPrints type, label and UUID of filesystem on a block device or image.\n\n"

#define HELP_base64 "usage: base64 [-di] [-w COLUMNS] [FILE...]\n\nEncode or decode in base64.\n\n-d	decode\n-i	ignore non-alphabetic characters\n-w	wrap output at COLUMNS (default 76)\n\n"

#define HELP_acpi "usage: acpi [-abctV]\n\nShow status of power sources and thermal devices.\n\n-a	show power adapters\n-b	show batteries\n-c	show cooling device state\n-t	show temperatures\n-V	show everything\n\n"

#define HELP_xzcat "usage: xzcat [filename...]\n\nDecompress listed files to stdout. Use stdin if no files listed.\n\n\n\n"

#define HELP_watch "usage: watch [-n SEC] [-t] PROG ARGS\n\nRun PROG periodically\n\n-n  Loop period in seconds (default 2)\n-t  Don't print header\n-e  Freeze updates on command error, and exit after enter.\n\n"

#define HELP_vi "usage: vi FILE\n\nVisual text editor. Predates the existence of standardized cursor keys,\nso the controls are weird and historical.\n\n"

#define HELP_userdel "usage: userdel [-r] USER\nusage: deluser [-r] USER\n\nOptions:\n-r remove home directory\nDelete USER from the SYSTEM\n\n"

#define HELP_useradd "usage: useradd [-SDH] [-h DIR] [-s SHELL] [-G GRP] [-g NAME] [-u UID] USER [GROUP]\n\nCreate new user, or add USER to GROUP\n\n-D       Don't assign a password\n-g NAME  Real name\n-G GRP   Add user to existing group\n-h DIR   Home directory\n-H       Don't create home directory\n-s SHELL Login shell\n-S       Create a system user\n-u UID   User id\n\n"

#define HELP_tr "usage: tr [-cds] SET1 [SET2]\n\nTranslate, squeeze, or delete characters from stdin, writing to stdout\n\n-c/-C  Take complement of SET1\n-d     Delete input characters coded SET1\n-s     Squeeze multiple output characters of SET2 into one character\n\n"

#define HELP_traceroute "usage: traceroute [-46FUIldnvr] [-f 1ST_TTL] [-m MAXTTL] [-p PORT] [-q PROBES]\n[-s SRC_IP] [-t TOS] [-w WAIT_SEC] [-g GATEWAY] [-i IFACE] [-z PAUSE_MSEC] HOST [BYTES]\n\ntraceroute6 [-dnrv] [-m MAXTTL] [-p PORT] [-q PROBES][-s SRC_IP] [-t TOS] [-w WAIT_SEC]\n  [-i IFACE] HOST [BYTES]\n\nTrace the route to HOST\n\n-4,-6 Force IP or IPv6 name resolution\n-F    Set the don't fragment bit (supports IPV4 only)\n-U    Use UDP datagrams instead of ICMP ECHO (supports IPV4 only)\n-I    Use ICMP ECHO instead of UDP datagrams (supports IPV4 only)\n-l    Display the TTL value of the returned packet (supports IPV4 only)\n-d    Set SO_DEBUG options to socket\n-n    Print numeric addresses\n-v    verbose\n-r    Bypass routing tables, send directly to HOST\n-m    Max time-to-live (max number of hops)(RANGE 1 to 255)\n-p    Base UDP port number used in probes(default 33434)(RANGE 1 to 65535)\n-q    Number of probes per TTL (default 3)(RANGE 1 to 255)\n-s    IP address to use as the source address\n-t    Type-of-service in probe packets (default 0)(RANGE 0 to 255)\n-w    Time in seconds to wait for a response (default 3)(RANGE 0 to 86400)\n-g    Loose source route gateway (8 max) (supports IPV4 only)\n-z    Pause Time in milisec (default 0)(RANGE 0 to 86400) (supports IPV4 only)\n-f    Start from the 1ST_TTL hop (instead from 1)(RANGE 1 to 255) (supports IPV4 only)\n-i    Specify a network interface to operate with\n\n"

#define HELP_tftpd "usage: tftpd [-cr] [-u USER] [DIR]\n\nTransfer file from/to tftp server.\n\n-r	read only\n-c	Allow file creation via upload\n-u	run as USER\n-l	Log to syslog (inetd mode requires this)\n\n"

#define HELP_tftp "usage: tftp [OPTIONS] HOST [PORT]\n\nTransfer file from/to tftp server.\n\n-l FILE Local FILE\n-r FILE Remote FILE\n-g    Get file\n-p    Put file\n-b SIZE Transfer blocks of SIZE octets(8 <= SIZE <= 65464)\n\n"

#define HELP_test "usage: test [-bcdefghLPrSsuwx PATH] [-nz STRING] [-t FD] [X ?? Y]\n\nReturn true or false by performing tests. (With no arguments return false.)\n\n--- Tests with a single argument (after the option):\nPATH is/has:\n  -b  block device   -f  regular file   -p  fifo           -u  setuid bit\n  -c  char device    -g  setgid         -r  read bit       -w  write bit\n  -d  directory      -h  symlink        -S  socket         -x  execute bit\n  -e  exists         -L  symlink        -s  nonzero size\nSTRING is:\n  -n  nonzero size   -z  zero size      (STRING by itself implies -n)\nFD (integer file descriptor) is:\n  -t  a TTY\n\n--- Tests with one argument on each side of an operator:\nTwo strings:\n  =  are identical	 !=  differ\nTwo integers:\n  -eq  equal         -gt  first > second    -lt  first < second\n  -ne  not equal     -ge  first >= second   -le  first <= second\n\n--- Modify or combine tests:\n  ! EXPR     not (swap true/false)   EXPR -a EXPR    and (are both true)\n  ( EXPR )   evaluate this first     EXPR -o EXPR    or (is either true)\n\n"

#define HELP_telnetd "Handle incoming telnet connections\n\n-l LOGIN  Exec LOGIN on connect\n-f ISSUE_FILE Display ISSUE_FILE instead of /etc/issue\n-K Close connection as soon as login exits\n-p PORT   Port to listen on\n-b ADDR[:PORT]  Address to bind to\n-F Run in foreground\n-i Inetd mode\n-w SEC    Inetd 'wait' mode, linger time SEC\n-S Log to syslog (implied by -i or without -F and -w)\n\n"

#define HELP_telnet "usage: telnet HOST [PORT]\n\nConnect to telnet server\n\n"

#define HELP_tcpsvd "usage: tcpsvd [-hEv] [-c N] [-C N[:MSG]] [-b N] [-u User] [-l Name] IP Port Prog\nusage: udpsvd [-hEv] [-c N] [-u User] [-l Name] IP Port Prog\n\nCreate TCP/UDP socket, bind to IP:PORT and listen for incoming connection.\nRun PROG for each connection.\n\nIP            IP to listen on, 0 = all\nPORT          Port to listen on\nPROG ARGS     Program to run\n-l NAME       Local hostname (else looks up local hostname in DNS)\n-u USER[:GRP] Change to user/group after bind\n-c N          Handle up to N (> 0) connections simultaneously\n-b N          (TCP Only) Allow a backlog of approximately N TCP SYNs\n-C N[:MSG]    (TCP Only) Allow only up to N (> 0) connections from the same IP\n              New connections from this IP address are closed\n              immediately. MSG is written to the peer before close\n-h            Look up peer's hostname\n-E            Don't set up environment variables\n-v            Verbose\n\n"

#define HELP_tar "usage: tar -[cxtzhmvO] [-X FILE] [-T FILE] [-f TARFILE] [-C DIR]\n\nCreate, extract, or list files from a tar file\n\nOperation:\nc Create\nf Name of TARFILE ('-' for stdin/out)\nh Follow symlinks\nm Don't restore mtime\nt List\nv Verbose\nx Extract\nz (De)compress using gzip\nC Change to DIR before operation\nO Extract to stdout\nexclude=FILE File to exclude\nX File with names to exclude\nT File with names to include\n\n"

#define HELP_syslogd "usage: syslogd  [-a socket] [-O logfile] [-f config file] [-m interval]\n                [-p socket] [-s SIZE] [-b N] [-R HOST] [-l N] [-nSLKD]\n\nSystem logging utility\n\n-a      Extra unix socket for listen\n-O FILE Default log file <DEFAULT: /var/log/messages>\n-f FILE Config file <DEFAULT: /etc/syslog.conf>\n-p      Alternative unix domain socket <DEFAULT : /dev/log>\n-n      Avoid auto-backgrounding.\n-S      Smaller output\n-m MARK interval <DEFAULT: 20 minutes> (RANGE: 0 to 71582787)\n-R HOST Log to IP or hostname on PORT (default PORT=514/UDP)\"\n-L      Log locally and via network (default is network only if -R)\"\n-s SIZE Max size (KB) before rotation (default:200KB, 0=off)\n-b N    rotated logs to keep (default:1, max=99, 0=purge)\n-K      Log to kernel printk buffer (use dmesg to read it)\n-l N    Log only messages more urgent than prio(default:8 max:8 min:1)\n-D      Drop duplicates\n\n"

#define HELP_sulogin "usage: sulogin [-t time] [tty]\n\nSingle User Login.\n-t	Default Time for Single User Login\n\n"

#define HELP_cd "usage: cd [-PL] [path]\n\nChange current directory.  With no arguments, go $HOME.\n\n-P	Physical path: resolve symlinks in path.\n-L	Local path: .. trims directories off $PWD (default).\n\n"

#define HELP_exit "usage: exit [status]\n\nExit shell.  If no return value supplied on command line, use value\nof most recent command, or 0 if none.\n\n"

#define HELP_sh "usage: sh [-c command] [script]\n\nCommand shell.  Runs a shell script, or reads input interactively\nand responds to it.\n\n-c	command line to execute\n-i	interactive mode (default when STDIN is a tty)\n\n"

#define HELP_route "usage: route [-ne] [-A inet[6]] / [add|del]\n\nDisplay/Edit kernel routing tables.\n\n-n	no name lookups\n-e	display other/more information\n-A	inet{6} Select Address Family\n\nreject mod dyn reinstate metric netmask gw mss window irtt dev\n\n"

#define HELP_ping "usage: ping [OPTIONS] HOST\n\nCheck network connectivity by sending packets to a host and reporting\nits response.\n\nSend ICMP ECHO_REQUEST packets to ipv4 or ipv6 addresses and prints each\necho it receives back, with round trip time.\n\nOptions:\n-4, -6      Force IPv4 or IPv6\n-c CNT      Send CNT many packets\n-I IFACE/IP Source interface or address\n-q          Quiet, only displays output at start and when finished\n-s SIZE     Packet SIZE in bytes (default 56)\n-t TTL      Set Time (number of hops) To Live\n-W SEC      Seconds to wait for response after all packets sent (default 10)\n-w SEC      Exit after this many seconds\n\n"

#define HELP_deallocvt "usage: deallocvt [N]\n\nDeallocate unused virtual terminal /dev/ttyN, or all unused consoles.\n\n"

#define HELP_openvt "usage: openvt [-c N] [-sw] [command [command_options]]\n\nstart a program on a new virtual terminal (VT)\n\n-c N  Use VT N\n-s    Switch to new VT\n-w    Wait for command to exit\n\nif -sw used together, switch back to originating VT when command completes\n\n"

#define HELP_netstat "usage: netstat [-pWrxwutneal]\n\nDisplay networking information.\n\n-r  Display routing table.\n-a  Display all sockets (Default: Connected).\n-l  Display listening server sockets.\n-t  Display TCP sockets.\n-u  Display UDP sockets.\n-w  Display Raw sockets.\n-x  Display Unix sockets.\n-e  Display other/more information.\n-n  Don't resolve names.\n-W  Wide Display.\n-p  Display PID/Program name for sockets.\n\n"

#define HELP_more "usage: more [FILE]...\n\nView FILE (or stdin) one screenful at a time.\n\n"

#define HELP_modprobe "usage: modprobe [-alrqvsDb] MODULE [symbol=value][...]\n\nmodprobe utility - inserts modules and dependencies.\n\n-a  Load multiple MODULEs\n-l  List (MODULE is a pattern)\n-r  Remove MODULE (stacks) or do autoclean\n-q  Quiet\n-v  Verbose\n-s  Log to syslog\n-D  Show dependencies\n-b  Apply blacklist to module names too\n\n"

#define HELP_mke2fs_extended "usage: mke2fs [-E stride=###] [-O option[,option]]\n\n-E stride= Set RAID stripe size (in blocks)\n-O [opts]  Specify fewer ext2 option flags (for old kernels)\n           All of these are on by default (as appropriate)\n   none         Clear default options (all but journaling)\n   dir_index    Use htree indexes for large directories\n   filetype     Store file type info in directory entry\n   has_journal  Set by -j\n   journal_dev  Set by -J device=XXX\n   sparse_super Don't allocate huge numbers of redundant superblocks\n\n"

#define HELP_mke2fs_label "usage: mke2fs [-L label] [-M path] [-o string]\n\n-L         Volume label\n-M         Path to mount point\n-o         Created by\n\n"

#define HELP_mke2fs_gen "usage: gene2fs [options] device filename\n\nThe [options] are the same as mke2fs.\n\n"

#define HELP_mke2fs_journal "usage: mke2fs [-j] [-J size=###,device=XXX]\n\n-j         Create journal (ext3)\n-J         Journal options\n           size: Number of blocks (1024-102400)\n           device: Specify an external journal\n\n"

#define HELP_mke2fs "usage: mke2fs [-Fnq] [-b ###] [-N|i ###] [-m ###] device\n\nCreate an ext2 filesystem on a block device or filesystem image.\n\n-F         Force to run on a mounted device\n-n         Don't write to device\n-q         Quiet (no output)\n-b size    Block size (1024, 2048, or 4096)\n-N inodes  Allocate this many inodes\n-i bytes   Allocate one inode for every XXX bytes of device\n-m percent Reserve this percent of filesystem space for root user\n\n"

#define HELP_mdev_conf "The mdev config file (/etc/mdev.conf) contains lines that look like:\nhd[a-z][0-9]* 0:3 660\n\nEach line must contain three whitespace separated fields. The first\nfield is a regular expression matching one or more device names,\nthe second and third fields are uid:gid and file permissions for\nmatching devies.\n\n"

#define HELP_mdev "usage: mdev [-s]\n\nCreate devices in /dev using information from /sys.\n\n-s	Scan all entries in /sys to populate /dev.\n\n"

#define HELP_lsof "usage: lsof [-lt] [-p PID1,PID2,...] [NAME]...\n\nLists open files. If names are given on the command line, only\nthose files will be shown.\n\n-l	list uids numerically\n-p	for given comma-separated pids only (default all pids)\n-t	terse (pid only) output\n\n"

#define HELP_logger "usage: logger [-s] [-t tag] [-p [facility.]priority] [message]\n\nLog message (or stdin) to syslog.\n\n"

#define HELP_last "usage: last [-W] [-f FILE]\n\nShow listing of last logged in users.\n\n-W      Display the information without host-column truncation.\n-f FILE Read from file FILE instead of /var/log/wtmp.\n\n"

#define HELP_klogd "usage: klogd [-n] [-c N]\n\n-c  N   Print to console messages more urgent than prio N (1-8)\"\n-n    Run in foreground.\n\n"

#define HELP_ipcs "usage: ipcs [[-smq] -i shmid] | [[-asmq] [-tcplu]]\n\n-i Show specific resource\nResource specification:\n-a All (default)\n-m Shared memory segments\n-q Message queues\n-s Semaphore arrays\nOutput format:\n-c Creator\n-l Limits\n-p Pid\n-t Time\n-u Summary\n\n"

#define HELP_ipcrm "usage: ipcrm [ [-q msqid] [-m shmid] [-s semid]\n          [-Q msgkey] [-M shmkey] [-S semkey] ... ]\n\n-mM Remove memory segment after last detach\n-qQ Remove message queue\n-sS Remove semaphore\n\n"

#define HELP_ip "usage: ip [ OPTIONS ] OBJECT { COMMAND }\n\nShow / manipulate routing, devices, policy routing and tunnels.\n\nwhere OBJECT := {address | link | route | rule | tunnel}\nOPTIONS := { -f[amily] { inet | inet6 | link } | -o[neline] }\n\n"

#define HELP_init "usage: init\n\nSystem V style init.\n\nFirst program to run (as PID 1) when the system comes up, reading\n/etc/inittab to determine actions.\n\n"

#define HELP_iconv "usage: iconv [-f FROM] [-t TO] [FILE...]\n\nConvert character encoding of files.\n\n-f  convert from (default utf8)\n-t  convert to   (default utf8)\n\n"

#define HELP_host "usage: host [-av] [-t TYPE] NAME [SERVER]\n\nPerform DNS lookup on NAME, which can be a domain name to lookup,\nor an ipv4 dotted or ipv6 colon seprated address to reverse lookup.\nSERVER (if present) is the DNS server to use.\n\n-a	no idea\n-t	not a clue\n-v	verbose\n\n"

#define HELP_groupdel "usage: groupdel [USER] GROUP\n\nDelete a group or remove a user from a group\n\n"

#define HELP_groupadd "usage: groupadd [-S] [-g GID] [USER] GROUP\n\nAdd a group or add a user to a group\n\n  -g GID Group id\n  -S     Create a system group\n\n"

#define HELP_getty "usage: getty [OPTIONS] BAUD_RATE[,BAUD_RATE]... TTY [TERMTYPE]\n\n-h    Enable hardware RTS/CTS flow control\n-L    Set CLOCAL (ignore Carrier Detect state)\n-m    Get baud rate from modem's CONNECT status message\n-n    Don't prompt for login name\n-w    Wait for CR or LF before sending /etc/issue\n-i    Don't display /etc/issue\n-f ISSUE_FILE  Display ISSUE_FILE instead of /etc/issue\n-l LOGIN  Invoke LOGIN instead of /bin/login\n-t SEC    Terminate after SEC if no login name is read\n-I INITSTR  Send INITSTR before anything else\n-H HOST    Log HOST into the utmp file as the hostname\n\n"

#define HELP_ftpget "usage: ftpget [-cv] [-u USER -p PASSWORD -P PORT] HOST_NAME [LOCAL_FILENAME] REMOTE_FILENAME\nusage: ftpput [-v] [-u USER -p PASSWORD -P PORT] HOST_NAME [REMOTE_FILENAME] LOCAL_FILENAME\n\nftpget - Get a remote file from FTP.\nftpput - Upload a local file on remote machine through FTP.\n\n-c Continue previous transfer.\n-v Verbose.\n-u User name.\n-p Password.\n-P Port Number (default 21).\n\n"

#define HELP_fsck "usage: fsck [-ANPRTV] [-C FD] [-t FSTYPE] [FS_OPTS] [BLOCKDEV]...\n\nCheck and repair filesystems\n\n-A      Walk /etc/fstab and check all filesystems\n-N      Don't execute, just show what would be done\n-P      With -A, check filesystems in parallel\n-R      With -A, skip the root filesystem\n-T      Don't show title on startup\n-V      Verbose\n-C n    Write status information to specified filedescriptor\n-t TYPE List of filesystem types to check\n\n\n"

#define HELP_fold "usage: fold [-bsu] [-w WIDTH] [FILE...]\n\nFolds (wraps) or unfolds ascii text by adding or removing newlines.\nDefault line width is 80 columns for folding and infinite for unfolding.\n\n-b	Fold based on bytes instead of columns\n-s	Fold/unfold at whitespace boundaries if possible\n-u	Unfold text (and refold if -w is given)\n-w	Set lines to WIDTH columns or bytes\n\n"

#define HELP_file "usage: file [file...]\n\nExamine the given files and describe their content types.\n\n"

#define HELP_fdisk "usage: fdisk [-lu] [-C CYLINDERS] [-H HEADS] [-S SECTORS] [-b SECTSZ] DISK\n\nChange partition table\n\n-u            Start and End are in sectors (instead of cylinders)\n-l            Show partition table for each DISK, then exit\n-b size       sector size (512, 1024, 2048 or 4096)\n-C CYLINDERS  Set number of cylinders/heads/sectors\n-H HEADS\n-S SECTORS\n\n"

#define HELP_expr "usage: expr ARG1 OPERATOR ARG2...\n\nEvaluate expression and print result. For example, \"expr 1 + 2\".\n\nThe supported operators are (grouped from highest to lowest priority):\n\n  ( )    :    * / %    + -    != <= < >= > =    &    |\n\nEach constant and operator must be a separate command line argument.\nAll operators are infix, meaning they expect a constant (or expression\nthat resolves to a constant) on each side of the operator. Operators of\nthe same priority (within each group above) are evaluated left to right.\nParentheses may be used (as separate arguments) to elevate the priority\nof expressions.\n\nCalling expr from a command shell requires a lot of \\( or '*' escaping\nto avoid interpreting shell control characters.\n\nThe & and | operators are logical (not bitwise) and may operate on\nstrings (a blank string is \"false\"). Comparison operators may also\noperate on strings (alphabetical sort).\n\nConstants may be strings or integers. Comparison, logical, and regex\noperators may operate on strings (a blank string is \"false\"), other\noperators require integers.\n\n"

#define HELP_dumpleases "usage: dumpleases [-r|-a] [-f LEASEFILE]\n\nDisplay DHCP leases granted by udhcpd\n-f FILE,  Lease file\n-r        Show remaining time\n-a        Show expiration time\n\n"

#define HELP_diff "usage: diff [-abBdiNqrTstw] [-L LABEL] [-S FILE] [-U LINES] FILE1 FILE2\n\n-a  Treat all files as text\n-b  Ignore changes in the amount of whitespace\n-B  Ignore changes whose lines are all blank\n-d  Try hard to find a smaller set of changes\n-i  Ignore case differences\n-L  Use LABEL instead of the filename in the unified header\n-N  Treat absent files as empty\n-q  Output only whether files differ\n-r  Recurse\n-S  Start with FILE when comparing directories\n-T  Make tabs line up by prefixing a tab when necessary\n-s  Report when two files are the same\n-t  Expand tabs to spaces in output\n-U  Output LINES lines of context\n-w  Ignore all whitespace\n\n"

#define HELP_dhcpd "usage: dhcpd [-46fS] [-i IFACE] [-P N] [CONFFILE]\n\n -f    Run in foreground\n -i Interface to use\n -S    Log to syslog too\n -P N  Use port N (default ipv4 67, ipv6 547)\n -4, -6    Run as a DHCPv4 or DHCPv6 server\n\n"

#define HELP_dhcp "usage: dhcp [-fbnqvoCRB] [-i IFACE] [-r IP] [-s PROG] [-p PIDFILE]\n            [-H HOSTNAME] [-V VENDOR] [-x OPT:VAL] [-O OPT]\n\n     Configure network dynamicaly using DHCP.\n\n   -i Interface to use (default eth0)\n   -p Create pidfile\n   -s Run PROG at DHCP events (default /usr/share/dhcp/default.script)\n   -B Request broadcast replies\n   -t Send up to N discover packets\n   -T Pause between packets (default 3 seconds)\n   -A Wait N seconds after failure (default 20)\n   -f Run in foreground\n   -b Background if lease is not obtained\n   -n Exit if lease is not obtained\n   -q Exit after obtaining lease\n   -R Release IP on exit\n   -S Log to syslog too\n   -a Use arping to validate offered address\n   -O Request option OPT from server (cumulative)\n   -o Don't request any options (unless -O is given)\n   -r Request this IP address\n   -x OPT:VAL  Include option OPT in sent packets (cumulative)\n   -F Ask server to update DNS mapping for NAME\n   -H Send NAME as client hostname (default none)\n   -V VENDOR Vendor identifier (default 'toybox VERSION')\n   -C Don't send MAC as client identifier\n   -v Verbose\n\n   Signals:\n   USR1  Renew current lease\n   USR2  Release current lease\n\n\n"

#define HELP_dhcp6 "usage: dhcp6 [-fbnqvR] [-i IFACE] [-r IP] [-s PROG] [-p PIDFILE]\n\n      Configure network dynamicaly using DHCP.\n\n    -i Interface to use (default eth0)\n    -p Create pidfile\n    -s Run PROG at DHCP events\n    -t Send up to N Solicit packets\n    -T Pause between packets (default 3 seconds)\n    -A Wait N seconds after failure (default 20)\n    -f Run in foreground\n    -b Background if lease is not obtained\n    -n Exit if lease is not obtained\n    -q Exit after obtaining lease\n    -R Release IP on exit\n    -S Log to syslog too\n    -r Request this IP address\n    -v Verbose\n\n    Signals:\n    USR1  Renew current lease\n    USR2  Release current lease\n\n"

#define HELP_dd "usage: dd [if=FILE] [of=FILE] [ibs=N] [obs=N] [bs=N] [count=N] [skip=N]\n        [seek=N] [conv=notrunc|noerror|sync|fsync]\n\nOptions:\nif=FILE   Read from FILE instead of stdin\nof=FILE   Write to FILE instead of stdout\nbs=N      Read and write N bytes at a time\nibs=N     Read N bytes at a time\nobs=N     Write N bytes at a time\ncount=N   Copy only N input blocks\nskip=N    Skip N input blocks\nseek=N    Skip N output blocks\nconv=notrunc  Don't truncate output file\nconv=noerror  Continue after read errors\nconv=sync     Pad blocks with zeros\nconv=fsync    Physically write data out before finishing\n\nNumbers may be suffixed by c (x1), w (x2), b (x512), kD (x1000), k (x1024),\nMD (x1000000), M (x1048576), GD (x1000000000) or G (x1073741824)\nCopy a file, converting and formatting according to the operands.\n\n"

#define HELP_crontab "usage: crontab [-u user] FILE\n               [-u user] [-e | -l | -r]\n               [-c dir]\n\nFiles used to schedule the execution of programs.\n\n-c crontab dir\n-e edit user's crontab\n-l list user's crontab\n-r delete user's crontab\n-u user\nFILE Replace crontab by FILE ('-': stdin)\n\n"

#define HELP_crond "usage: crond [-fbS] [-l N] [-d N] [-L LOGFILE] [-c DIR]\n\nA daemon to execute scheduled commands.\n\n-b Background (default)\n-c crontab dir\n-d Set log level, log to stderr\n-f Foreground\n-l Set log level. 0 is the most verbose, default 8\n-S Log to syslog (default)\n-L Log to file\n\n"

#define HELP_gunzip "usage: gunzip [-cflqStv] [FILE...]\n\nDecompess (deflate) file(s). With no files, compress stdin to stdout.\n\nOn successful decompression, compressed files are replaced with the\nuncompressed version. The input file is removed and replaced with\na new file without the .gz extension (with same ownership/permissions).\n\n-c	cat to stdout (act as zcat)\n-f	force (output file exists, input is tty, unrecognized extension)\n-l	list compressed/uncompressed/ratio/name for each input file.\n-q	quiet (no warnings)\n-S	specify exension (default .*)\n-t	test compressed file(s)\n-v	verbose (like -l, but decompress files)\n\n"

#define HELP_zcat "usage: zcat [FILE...]\n\nDecompress deflated file(s) to stdout\n\n"

#define HELP_decompress "usage: compress [-zglrcd9] [FILE]\n\nCompress or decompress file (or stdin) using \"deflate\" algorithm.\n\n-c	compress with -g gzip (default)  -l zlib  -r raw  -z zip\n-d	decompress (autodetects type)\n\n\n"

#define HELP_gzip_d "usage: gzip [-d]\n\n-d	decompress (act as gunzip)\n\n"

#define HELP_gzip "usage: gzip [-19cfqStvzgLR] [FILE...]\n\nCompess (deflate) file(s). With no files, compress stdin to stdout.\n\nOn successful decompression, compressed files are replaced with the\nuncompressed version. The input file is removed and replaced with\na new file without the .gz extension (with same ownership/permissions).\n\n-1	Minimal compression (fastest)\n-9	Max compression (default)\n-c	cat to stdout (act as zcat)\n-f	force (if output file exists, input is tty, unrecognized extension)\n-q	quiet (no warnings)\n-S	specify exension (default .*)\n-t	test compressed file(s)\n-v	verbose (like -l, but compress files)\n\nCompression type:\n-g gzip (default)    -L zlib    -R raw    -z zip\n\n"

#define HELP_compress "usage: compress [-zgLR19] [FILE]\n\nCompress or decompress file (or stdin) using \"deflate\" algorithm.\n\n-1	min compression\n-9	max compression (default)\n-g	gzip (default)\n-L	zlib\n-R	raw\n-z	zip\n\n"

#define HELP_brctl "usage: brctl COMMAND [BRIDGE [INTERFACE]]\n\nManage ethernet bridges\n\nCommands:\nshow                  Show a list of bridges\naddbr BRIDGE          Create BRIDGE\ndelbr BRIDGE          Delete BRIDGE\naddif BRIDGE IFACE    Add IFACE to BRIDGE\ndelif BRIDGE IFACE    Delete IFACE from BRIDGE\nsetageing BRIDGE TIME Set ageing time\nsetfd BRIDGE TIME     Set bridge forward delay\nsethello BRIDGE TIME  Set hello time\nsetmaxage BRIDGE TIME Set max message age\nsetpathcost BRIDGE PORT COST   Set path cost\nsetportprio BRIDGE PORT PRIO   Set port priority\nsetbridgeprio BRIDGE PRIO      Set bridge priority\nstp BRIDGE [1/yes/on|0/no/off] STP on/off\n\n"

#define HELP_bootchartd "usage: bootchartd {start [PROG ARGS]}|stop|init\n\nCreate /var/log/bootlog.tgz with boot chart data\n\nstart: start background logging; with PROG, run PROG,\n       then kill logging with USR1\nstop:  send USR1 to all bootchartd processes\ninit:  start background logging; stop when getty/xdm is seen\n      (for init scripts)\n\nUnder PID 1: as init, then exec $bootchart_init, /init, /sbin/init\n\n"

#define HELP_arping "usage: arping [-fqbDUA] [-c CNT] [-w TIMEOUT] [-I IFACE] [-s SRC_IP] DST_IP\n\nSend ARP requests/replies\n\n-f         Quit on first ARP reply\n-q         Quiet\n-b         Keep broadcasting, don't go unicast\n-D         Duplicated address detection mode\n-U         Unsolicited ARP mode, update your neighbors\n-A         ARP answer mode, update your neighbors\n-c N       Stop after sending N ARP requests\n-w TIMEOUT Time to wait for ARP reply, seconds\n-I IFACE   Interface to use (default eth0)\n-s SRC_IP  Sender IP address\nDST_IP     Target IP address\n\n"

#define HELP_arp "Usage: arp\n[-vn] [-H HWTYPE] [-i IF] -a [HOSTNAME]\n[-v]              [-i IF] -d HOSTNAME [pub]\n[-v]  [-H HWTYPE] [-i IF] -s HOSTNAME HWADDR [temp]\n[-v]  [-H HWTYPE] [-i IF] -s HOSTNAME HWADDR [netmask MASK] pub\n[-v]  [-H HWTYPE] [-i IF] -Ds HOSTNAME IFACE [netmask MASK] pub\n\nManipulate ARP cache\n\n-a    Display (all) hosts\n-s    Set new ARP entry\n-d    Delete a specified entry\n-v    Verbose\n-n    Don't resolve names\n-i IF Network interface\n-D    Read <hwaddr> from given device\n-A,-p AF  Protocol family\n-H    HWTYPE Hardware address type\n\n\n"

#define HELP_xargs_pedantic "This version supports insane posix whitespace handling rendered obsolete\nby -0 mode.\n\n\n"

#define HELP_xargs "usage: xargs [-ptxr0] [-s NUM] [-n NUM] [-L NUM] [-E STR] COMMAND...\n\nRun command line one or more times, appending arguments from stdin.\n\nIf command exits with 255, don't launch another even if arguments remain.\n\n-s	Size in bytes per command line\n-n	Max number of arguments per command\n-0	Each argument is NULL terminated, no whitespace or quote processing\n#-p	Prompt for y/n from tty before running each command\n#-t	Trace, print command line to stderr\n#-x	Exit if can't fit everything in one command\n#-r	Don't run command with empty input\n#-L	Max number of lines of input per command\n-E	stop at line matching string\n\n"

#define HELP_who "usage: who\n\nPrint logged user information on system\n\n"

#define HELP_wc "usage: wc -lwcm [FILE...]\n\nCount lines, words, and characters in input.\n\n-l	show lines\n-w	show words\n-c	show bytes\n-m	show characters\n\nBy default outputs lines, words, bytes, and filename for each\nargument (or from stdin if none). Displays only either bytes\nor characters.\n\n"

#define HELP_uuencode "usage: uuencode [-m] [file] encode-filename\n\nUuencode stdin (or file) to stdout, with encode-filename in the output.\n\n-m	base64-encode\n\n"

#define HELP_uudecode "usage: uudecode [-o OUTFILE] [INFILE]\n\nDecode file from stdin (or INFILE).\n\n-o	write to OUTFILE instead of filename in header\n\n"

#define HELP_unlink "usage: unlink FILE\n\nDeletes one file.\n\n"

#define HELP_uniq "usage: uniq [-cduiz] [-w maxchars] [-f fields] [-s char] [input_file [output_file]]\n\nReport or filter out repeated lines in a file\n\n-c	show counts before each line\n-d	show only lines that are repeated\n-u	show only lines that are unique\n-i	ignore case when comparing lines\n-z	lines end with \\0 not \\n\n-w	compare maximum X chars per line\n-f	ignore first X fields\n-s	ignore first X chars\n\n"

#define HELP_uname "usage: uname [-asnrvm]\n\nPrint system information.\n\n-s	System name\n-n	Network (domain) name\n-r	Kernel Release number\n-v	Kernel Version\n-m	Machine (hardware) name\n-a	All of the above\n\n"

#define HELP_ulimit "usage: ulimit [-P PID] [-SHRacdefilmnpqrstuv] [LIMIT]\n\nPrint or set resource limits for process number PID. If no LIMIT specified\n(or read-only -ap selected) display current value (sizes in bytes).\nDefault is ulimit -P $PPID -Sf\" (show soft filesize of your shell).\n\n-S  Set/show soft limit          -H  Set/show hard (maximum) limit\n-a  Show all limits              -c  Core file size\n-d  Process data segment         -e  Max scheduling priority\n-f  Output file size             -i  Pending signal count\n-l  Locked memory                -m  Resident Set Size\n-n  Number of open files         -p  Pipe buffer\n-q  Posix message queue          -r  Max Real-time priority\n-R  Realtime latency (usec)      -s  Stack size\n-t  Total CPU time (in seconds)  -u  Maximum processes (under this UID)\n-v  Virtual memory size          -P  PID to affect (default $PPID)\n\n"

#define HELP_tty "usage: tty [-s]\n\nShow filename of terminal connected to stdin.\n\nPrints \"not a tty\" and exits with nonzero status if no terminal\nis connected to stdin.\n\n-s	silent, exit code only\n\n"

#define HELP_true "Return zero.\n\n"

#define HELP_touch "usage: touch [-amch] [-d DATE] [-t TIME] [-r FILE] FILE...\n\nUpdate the access and modification times of each FILE to the current time.\n\n-a	change access time\n-m	change modification time\n-c	don't create file\n-h	change symlink\n-d	set time to DATE (in YYYY-MM-DDThh:mm:SS[.frac][tz] format)\n-t	set time to TIME (in [[CC]YY]MMDDhhmm[.ss][frac] format)\n-r	set time same as reference FILE\n\n"

#define HELP_time "usage: time [-p] COMMAND [ARGS...]\n\nRun command line and report real, user, and system time elapsed in seconds.\n(real = clock on the wall, user = cpu used by command's code,\nsystem = cpu used by OS on behalf of command.)\n\n-p	posix mode (ignored)\n\n"

#define HELP_tee "usage: tee [-ai] [file...]\n\nCopy stdin to each listed file, and also to stdout.\nFilename \"-\" is a synonym for stdout.\n\n-a	append to files.\n-i	ignore SIGINT.\n\n"

#define HELP_tail_seek "This version uses lseek, which is faster on large files.\n\n"

#define HELP_tail "usage: tail [-n|c NUMBER] [-f] [FILE...]\n\nCopy last lines from files to stdout. If no files listed, copy from\nstdin. Filename \"-\" is a synonym for stdin.\n\n-n	output the last NUMBER lines (default 10), +X counts from start.\n-c	output the last NUMBER bytes, +NUMBER counts from start\n-f	follow FILE(s), waiting for more data to be appended\n\n"

#define HELP_strings "usage: strings [-fo] [-n LEN] [FILE...]\n\nDisplay printable strings in a binary file\n\n-f	Precede strings with filenames\n-n	At least LEN characters form a string (default 4)\n-o	Precede strings with decimal offsets\n\n"

#define HELP_split "usage: split [-a SUFFIX_LEN] [-b BYTES] [-l LINES] [INPUT [OUTPUT]]\n\nCopy INPUT (or stdin) data to a series of OUTPUT (or \"x\") files with\nalphabetically increasing suffix (aa, ab, ac... az, ba, bb...).\n\n-a	Suffix length (default 2)\n-b	BYTES/file (10, 10k, 10m, 10g...)\n-l	LINES/file (default 1000)\n\n"

#define HELP_sort "usage: sort [-Mbcdfginrsuz] [-k#[,#[x]] [-t X]] [-o FILE] [FILE...]\n\nSort all lines of text from input files (or stdin) to stdout.\n\n-M	month sort (jan, feb, etc).\n-b	ignore leading blanks (or trailing blanks in second part of key)\n-c	check whether input is sorted\n-d	dictionary order (use alphanumeric and whitespace chars only)\n-f	force uppercase (case insensitive sort)\n-g	general numeric sort (double precision with nan and inf)\n-i	ignore nonprinting characters\n-k	sort by \"key\" (see below)\n-n	numeric order (instead of alphabetical)\n-o	output to FILE instead of stdout\n-r	reverse\n-s	skip fallback sort (only sort with keys)\n-t	use a key separator other than whitespace\n-u	unique lines only\n-x	Hexadecimal numerical sort\n-z	zero (null) terminated lines\n\nSorting by key looks at a subset of the words on each line.  -k2\nuses the second word to the end of the line, -k2,2 looks at only\nthe second word, -k2,4 looks from the start of the second to the end\nof the fourth word.  Specifying multiple keys uses the later keys as\ntie breakers, in order.  A type specifier appended to a sort key\n(such as -2,2n) applies only to sorting that key.\n"

#define HELP_sleep_float "Length can be a decimal fraction.\n\n"

#define HELP_sleep "usage: sleep LENGTH\n\nWait before exiting. An optional suffix can be \"m\" (minutes), \"h\" (hours),\n\"d\" (days), or \"s\" (seconds, the default).\n\n\n"

#define HELP_sed "usage: sed [-inrE] [-e SCRIPT]...|SCRIPT [-f SCRIPT_FILE]... [FILE...]\n\nStream editor. Apply one or more editing SCRIPTs to each line of input\n(from FILE or stdin) producing output (by default to stdout).\n\n-e	add SCRIPT to list\n-f	add contents of SCRIPT_FILE to list\n-i	Edit each file in place.\n-n	No default output. (Use the p command to output matched lines.)\n-r	Use extended regular expression syntax.\n-E	Alias for -r.\n-s	Treat input files separately (implied by -i)\n\nA SCRIPT is a series of one or more COMMANDs separated by newlines or\nsemicolons. All -e SCRIPTs are concatenated together as if separated\nby newlines, followed by all lines from -f SCRIPT_FILEs, in order.\nIf no -e or -f SCRIPTs are specified, the first argument is the SCRIPT.\n\nEach COMMAND may be preceded by an address which limits the command to\napply only to the specified line(s). Commands without an address apply to\nevery line. Addresses are of the form:\n\n  [ADDRESS[,ADDRESS]]COMMAND\n\nThe ADDRESS may be a decimal line number (starting at 1), a /regular\nexpression/ within a pair of forward slashes, or the character \"$\" which\nmatches the last line of input. (In -s or -i mode this matches the last\nline of each file, otherwise just the last line of the last file.) A single\naddress matches one line, a pair of comma separated addresses match\neverything from the first address to the second address (inclusive). If\nboth addresses are regular expressions, more than one range of lines in\neach file can match.\n\nREGULAR EXPRESSIONS in sed are started and ended by the same character\n(traditionally / but anything except a backslash or a newline works).\nBackslashes may be used to escape the delimiter if it occurs in the\nregex, and for the usual printf escapes (\\abcefnrtv and octal, hex,\nand unicode). An empty regex repeats the previous one. ADDRESS regexes\n(above) require the first delimeter to be escaped with a backslash when\nit isn't a forward slash (to distinguish it from the COMMANDs below).\n\nSed mostly operates on individual lines one at a time. It reads each line,\nprocesses it, and either writes it to the output or discards it before\nreading the next line. Sed can remember one additional line in a separate\nbuffer (using the h, H, g, G, and x commands), and can read the next line\nof input early (using the n and N command), but other than that command\nscripts operate on individual lines of text.\n\nEach COMMAND starts with a single character. The following commands take\nno arguments:\n\n  {  Start a new command block, continuing until a corresponding \"}\".\n     Command blocks may nest. If the block has an address, commands within\n     the block are only run for lines within the block's address range.\n\n  }  End command block (this command cannot have an address)\n\n  d  Delete this line and move on to the next one\n     (ignores remaining COMMANDs)\n\n  D  Delete one line of input and restart command SCRIPT (same as \"d\"\n     unless you've glued lines together with \"N\" or similar)\n\n  g  Get remembered line (overwriting current line)\n\n  G  Get remembered line (appending to current line)\n\n  h  Remember this line (overwriting remembered line)\n\n  H  Remember this line (appending to remembered line, if any)\n\n  l  Print line, escaping \\abfrtv (but not newline), octal escaping other\n     nonprintable characters, wrapping lines to terminal width with a\n     backslash, and appending $ to actual end of line.\n\n  n  Print default output and read next line, replacing current line\n     (If no next line available, quit processing script)\n\n  N  Append next line of input to this line, separated by a newline\n     (This advances the line counter for address matching and \"=\", if no\n     next line available quit processing script without default output)\n\n  p  Print this line\n\n  P  Print this line up to first newline (from \"N\")\n\n  q  Quit (print default output, no more commands processed or lines read)\n\n  x  Exchange this line with remembered line (overwrite in both directions)\n\n  =  Print the current line number (followed by a newline)\n\nThe following commands (may) take an argument. The \"text\" arguments (to\nthe \"a\", \"b\", and \"c\" commands) may end with an unescaped \"\\\" to append\nthe next line (for which leading whitespace is not skipped), and also\ntreat \";\" as a literal character (use \"\\;\" instead).\n\n  a [text]   Append text to output before attempting to read next line\n\n  b [label]  Branch, jumps to :label (or with no label, to end of SCRIPT)\n\n  c [text]   Delete line, output text at end of matching address range\n             (ignores remaining COMMANDs)\n\n  i [text]   Print text\n\n  r [file]   Append contents of file to output before attempting to read\n             next line.\n\n  s/S/R/F    Search for regex S, replace matched text with R using flags F.\n             The first character after the \"s\" (anything but newline or\n             backslash) is the delimiter, escape with \\ to use normally.\n\n             The replacement text may contain \"&\" to substitute the matched\n             text (escape it with backslash for a literal &), or \\1 through\n             \\9 to substitute a parenthetical subexpression in the regex.\n             You can also use the normal backslash escapes such as \\n and\n             a backslash at the end of the line appends the next line.\n\n             The flags are:\n\n             [0-9]    A number, substitute only that occurrence of pattern\n             g        Global, substitute all occurrences of pattern\n             i        Ignore case when matching\n             p        Print the line if match was found and replaced\n             w [file] Write (append) line to file if match replaced\n\n  t [label]  Test, jump to :label only if an \"s\" command found a match in\n             this line since last test (replacing with same text counts)\n\n  T [label]  Test false, jump only if \"s\" hasn't found a match.\n\n  w [file]   Write (append) line to file\n\n  y/old/new/ Change each character in 'old' to corresponding character\n             in 'new' (with standard backslash escapes, delimiter can be\n             any repeated character except \\ or \\n)\n\n  : [label]  Labeled target for jump commands\n\n  #  Comment, ignore rest of this line of SCRIPT\n\nDeviations from posix: allow extended regular expressions with -r,\nediting in place with -i, separate with -s, printf escapes in text, line\ncontinuations, semicolons after all commands, 2-address anywhere an\naddress is allowed, \"T\" command, multiline continuations for [abc],\n\\; to end [abc] argument before end of line.\n\n"

#define HELP_rmdir "usage: rmdir [-p] [dirname...]\n\nRemove one or more directories.\n\n-p	Remove path.\n\n"

#define HELP_rm "usage: rm [-fiRr] FILE...\n\nRemove each argument from the filesystem.\n\n-f	force: remove without confirmation, no error if it doesn't exist\n-i	interactive: prompt for confirmation\n-rR	recursive: remove directory contents\n\n"

#define HELP_renice "usage: renice [-gpu] -n increment ID ...\n\n"

#define HELP_pwd "usage: pwd [-L|-P]\n\nPrint working (current) directory.\n\n-L  Use shell's path from $PWD (when applicable)\n-P  Print cannonical absolute path\n\n"

#define HELP_pkill "usage: pkill [-l SIGNAL] [PATTERN]\n\n-l	SIGNAL to send\n-V	verbose\n\n"

#define HELP_pgkill_common "usage: pgrep [-fnovx] [-G GID,] [-g PGRP,] [-P PPID,] [-s SID,] [-t TERM,] [-U UID,] [-u EUID,]\n\n-f	Check full command line for PATTERN\n-G	Match real Group ID(s)\n-g	Match Process Group(s) (0 is current user)\n-n	Newest match only\n-o	Oldest match only\n-P	Match Parent Process ID(s)\n-s	Match Session ID(s) (0 for current)\n-t	Match Terminal(s)\n-U	Match real User ID(s)\n-u	Match effective User ID(s)\n-v	Negate the match\n-x	Match whole command (not substring)\n\n"

#define HELP_pgrep "usage: pgrep [-cL] [-d DELIM] [-L SIGNAL] [PATTERN]\n\nSearch for process(es). PATTERN is an extended regular expression checked\nagainst command names.\n\n-c	Show only count of matches\n-d	Use DELIM instead of newline\n-L	Send SIGNAL instead of printing name\n-l	Show command name\n\n"

#define HELP_top_common "usage: COMMON [-bq] [-n NUMBER] [-d SECONDS] [-p PID,] [-u USER,] [-s SORT]\n\n-b	Batch mode (no tty)\n-d	Delay SECONDS between each cycle (default 3)\n-n	Exit after NUMBER iterations\n-p	Show these PIDs\n-u	Show these USERs\n-q	Quiet (no header lines)\n\nCursor LEFT/RIGHT to change sort, UP/DOWN move list, space to force\nupdate, R to reverse sort, Q to exit.\n\n"

#define HELP_iotop "usage: iotop [-AaKO]\n\nRank processes by I/O.\n\n-A	All I/O, not just disk\n-a	Accumulated I/O (not percentage)\n-K	Kilobytes\n-k	Fallback sort FIELDS (default -[D]IO,-ETIME,-PID)\n-O	Only show processes doing I/O\n-o	Show FIELDS (default PID,PR,USER,[D]READ,[D]WRITE,SWAP,[D]IO,COMM)\n-s	Sort by field number (0-X, default 6)\n\n"

#define HELP_top "usage: top [-m] [ -d seconds ] [ -n iterations ]\n\nShow process activity in real time.\n\n-k	Fallback sort FIELDS (default -S,-%CPU,-ETIME,-PID)\n-o	Show FIELDS (def PID,USER,PR,NI,VIRT,RES,SHR,S,%CPU,%MEM,TIME+,CMDLINE)\n-s	Sort by field number (1-X, default 9)\n\n"

#define HELP_ps "usage: ps [-AadeflnwZ] [-gG GROUP,] [-k FIELD,] [-o FIELD,] [-p PID,] [-t TTY,] [-uU USER,]\n\nList processes.\n\nWhich processes to show (selections may be comma separated lists):\n\n-A	All processes\n-a	Processes with terminals that aren't session leaders\n-d	All processes that aren't session leaders\n-e	Same as -A\n-g	Belonging to GROUPs\n-G	Belonging to real GROUPs (before sgid)\n-p	PIDs (--pid)\n-P	Parent PIDs (--ppid)\n-s	In session IDs\n-t	Attached to selected TTYs\n-u	Owned by USERs\n-U	Owned by real USERs (before suid)\n\nOutput modifiers:\n\n-k	Sort FIELDs in +increasing or -decreasting order (--sort)\n-M	Measure field widths (expanding as necessary)\n-n	Show numeric USER and GROUP\n-w	Wide output (don't truncate at terminal width)\n\nWhich FIELDs to show. (Default = -o PID,TTY,TIME,CMD)\n\n-f	Full listing (-o USER:8=UID,PID,PPID,C,STIME,TTY,TIME,CMD)\n-l	Long listing (-o F,S,UID,PID,PPID,C,PRI,NI,ADDR,SZ,WCHAN,TTY,TIME,CMD)\n-o	Output FIELDs instead of defaults, each with optional :size and =title\n-O  Add FIELDS to defaults\n-Z	Include LABEL\n\nAvailable -o FIELDs:\n\n  ADDR  Instruction pointer               ARGS    Command line (argv[] -path)\n  CMD   COMM without -f, ARGS with -f     CMDLINE Command line (argv[])\n  COMM  Original command name             COMMAND Original command path\n  CPU   Which processor running on        ETIME   Elapsed time since PID start\n  F     Flags (1=FORKNOEXEC 4=SUPERPRIV)  GID     Group id\n  GROUP Group name                        LABEL   Security label\n  MAJFL Major page faults                 MINFL   Minor page faults\n  NAME  Command name (argv[0])            NI      Niceness (lower is faster)\n  PCPU  Percentage of CPU time used       PGID    Process Group ID\n  PID   Process ID                        PPID    Parent Process ID\n  PRI   Priority (higher is faster)       PSR     Processor last executed on\n  RGID  Real (before sgid) group ID       RGROUP  Real (before sgid) group name\n  RSS   Resident Set Size (pages in use)  RTPRIO  Realtime priority\n  RUID  Real (before suid) user ID        RUSER   Real (before suid) user name\n  S     Process state:\n        R (running) S (sleeping) D (device I/O) T (stopped)  t (traced)\n        Z (zombie)  X (deader)   x (dead)       K (wakekill) W (waking)\n  SCHED Scheduling policy (0=other, 1=fifo, 2=rr, 3=batch, 4=iso, 5=idle)\n  STAT  Process state (S) plus:\n        < high priority          N low priority L locked memory\n        s session leader         + foreground   l multithreaded\n  STIME Start time of process in hh:mm (size :19 shows yyyy-mm-dd hh:mm:ss)\n  SZ    Memory Size (4k pages needed to completely swap out process)\n  TIME  CPU time consumed                 TTY     Controlling terminal\n  UID   User id                           USER    User name\n  VSZ   Virtual memory size (1k units)    %VSZ    VSZ as % of physical memory\n  WCHAN Waiting in kernel for\n\n"

#define HELP_printf "usage: printf FORMAT [ARGUMENT...]\n\nFormat and print ARGUMENT(s) according to FORMAT, using C printf syntax\n(% escapes for cdeEfgGiosuxX, \\ escapes for abefnrtv0 or \\OCTAL or \\xHEX).\n\n"

#define HELP_patch "usage: patch [-i file] [-p depth] [-Ru]\n\nApply a unified diff to one or more files.\n\n-i	Input file (defaults=stdin)\n-l	Loose match (ignore whitespace)\n-p	Number of '/' to strip from start of file paths (default=all)\n-R	Reverse patch.\n-u	Ignored (only handles \"unified\" diffs)\n\nThis version of patch only handles unified diffs, and only modifies\na file when all all hunks to that file apply.  Patch prints failed\nhunks to stderr, and exits with nonzero status if any hunks fail.\n\nA file compared against /dev/null (or with a date <= the epoch) is\ncreated/deleted as appropriate.\n\n"

#define HELP_paste "usage: paste [-s] [-d list] [file...]\n\nReplace newlines in files.\n\n-d list    list of delimiters to separate lines\n-s         process files sequentially instead of in parallel\n\nBy default print corresponding lines separated by <tab>.\n\n"

#define HELP_od "usage: od [-bcdosxv] [-j #] [-N #] [-A doxn] [-t acdfoux[#]]\n\n-A	Address base (decimal, octal, hexdecimal, none)\n-j	Skip this many bytes of input\n-N	Stop dumping after this many bytes\n-t	output type a(scii) c(har) d(ecimal) f(loat) o(ctal) u(nsigned) (he)x\n	plus optional size in bytes\n	aliases: -b=-t o1, -c=-t c, -d=-t u2, -o=-t o2, -s=-t d2, -x=-t x2\n-v	Don't collapse repeated lines together\n\n"

#define HELP_nohup "usage: nohup COMMAND [ARGS...]\n\nRun a command that survives the end of its terminal.\n\nRedirect tty on stdin to /dev/null, tty on stdout to \"nohup.out\".\n\n"

#define HELP_nl "usage: nl [-E] [-l #] [-b MODE] [-n STYLE] [-s SEPARATOR] [-w WIDTH] [FILE...]\n\nNumber lines of input.\n\n-E	Use extended regex syntax (when doing -b pREGEX)\n-b	which lines to number: a (all) t (non-empty, default) pREGEX (pattern)\n-l	Only count last of this many consecutive blank lines\n-n	number STYLE: ln (left justified) rn (right justified) rz (zero pad)\n-s	Separator to use between number and line (instead of TAB)\n-w	Width of line numbers (default 6)\n\n"

#define HELP_nice "usage: nice [-n PRIORITY] command [args...]\n\nRun a command line at an increased or decreased scheduling priority.\n\nHigher numbers make a program yield more CPU time, from -20 (highest\npriority) to 19 (lowest).  By default processes inherit their parent's\nniceness (usually 0).  By default this command adds 10 to the parent's\npriority.  Only root can set a negative niceness level.\n\n"

#define HELP_mkfifo "usage: mkfifo [-Z CONTEXT] [NAME...]\n\nCreate FIFOs (named pipes).\n\n-Z	Security context\n"

#define HELP_mkdir_z "usage: [-Z context]\n\n-Z	set security context\n\n"

#define HELP_mkdir "usage: mkdir [-vp] [-m mode] [dirname...]\n\nCreate one or more directories.\n\n-m	set permissions of directory to mode.\n-p	make parent directories as needed.\n-v	verbose\n\n"

#define HELP_ls_color "--color  device=yellow  symlink=turquoise/red  dir=blue  socket=purple\n         files: exe=green  suid=red  suidfile=redback  stickydir=greenback\n         =auto means detect if output is a tty.\n\nusage: ls --color[=auto] [-ACFHLRSZacdfhiklmnpqrstux1] [directory...]\n\nlist files\n\nwhat to show:\n-a	all files including .hidden		-c  use ctime for timestamps\n-d	directory, not contents			-i  inode number\n-k	block sizes in kilobytes		-p  put a '/' after dir names\n-q	unprintable chars as '?'		-s  size (in blocks)\n-u	use access time for timestamps		-A  list all files but . and ..\n-H	follow command line symlinks		-L  follow symlinks\n-R	recursively list files in subdirs	-F  append /dir *exe @sym |FIFO\n-Z	security context\n\noutput formats:\n-1	list one file per line			-C  columns (sorted vertically)\n-g	like -l but no owner			-h  human readable sizes\n-l	long (show full details)		-m  comma separated\n-n	like -l but numeric uid/gid		-o  like -l but no group\n-x	columns (horizontal sort)\n\nsorting (default is alphabetical):\n-f	unsorted	-r  reverse	-t  timestamp	-S  size\n"

#define HELP_ls "usage: ls --color[=auto] [-ACFHLRSZacdfhiklmnpqrstux1] [directory...]\n\nlist files\n\nwhat to show:\n-a	all files including .hidden		-c  use ctime for timestamps\n-d	directory, not contents			-i  inode number\n-k	block sizes in kilobytes		-p  put a '/' after dir names\n-q	unprintable chars as '?'		-s  size (in blocks)\n-u	use access time for timestamps		-A  list all files but . and ..\n-H	follow command line symlinks		-L  follow symlinks\n-R	recursively list files in subdirs	-F  append /dir *exe @sym |FIFO\n-Z	security context\n\noutput formats:\n-1	list one file per line			-C  columns (sorted vertically)\n-g	like -l but no owner			-h  human readable sizes\n-l	long (show full details)		-m  comma separated\n-n	like -l but numeric uid/gid		-o  like -l but no group\n-x	columns (horizontal sort)\n\nsorting (default is alphabetical):\n-f	unsorted	-r  reverse	-t  timestamp	-S  size\n--color  device=yellow  symlink=turquoise/red  dir=blue  socket=purple\n         files: exe=green  suid=red  suidfile=redback  stickydir=greenback\n         =auto means detect if output is a tty.\n\n"

#define HELP_ln "usage: ln [-sfnv] [FROM...] TO\n\nCreate a link between FROM and TO.\nWith only one argument, create link in current directory.\n\n-s	Create a symbolic link\n-f	Force the creation of the link, even if TO already exists\n-n	Symlink at destination treated as file\n-v	Verbose\n\n"

#define HELP_link "usage: link FILE NEWLINK\n\nCreate hardlink to a file.\n\n"

#define HELP_killall5 "usage: killall5 [-l [SIGNAL]] [-SIGNAL|-s SIGNAL] [-o PID]...\n\nSend a signal to all processes outside current session.\n\n-l     List signal name(s) and number(s)\n-o PID Omit PID\n-s     send SIGNAL (default SIGTERM)\n\n"

#define HELP_kill "usage: kill [-l [SIGNAL] | -s SIGNAL | -SIGNAL] pid...\n\nSend signal to process(es).\n\n-l	List signal name(s) and number(s)\n-s	Send SIGNAL (default SIGTERM)\n\n"

#define HELP_whoami "usage: whoami\n\nPrint the current user name.\n\n"

#define HELP_logname "usage: logname\n\nPrint the current user name.\n\n"

#define HELP_groups "usage: groups [user]\n\nPrint the groups a user is in.\n\n"

#define HELP_id "usage: id [-GZgnru] \n\nPrint user and group ID.\n-G	Show only the group IDs\n-Z	Show only security context\n-g	Show only the effective group ID\n-n	print names instead of numeric IDs (to be used with -Ggu)\n-r	Show real ID instead of effective ID\n-u	Show only the effective user ID\n"

#define HELP_head "usage: head [-n number] [file...]\n\nCopy first lines from files to stdout. If no files listed, copy from\nstdin. Filename \"-\" is a synonym for stdin.\n\n-n	Number of lines to copy.\n\n"

#define HELP_grep "usage: grep [-EFivwcloqsHbhn] [-A NUM] [-m MAX] [-e REGEX]... [-f REGFILE] [FILE]...\n\nShow lines matching regular expressions. If no -e, first argument is\nregular expression to match. With no files (or \"-\" filename) read stdin.\nReturns 0 if matched, 1 if no match found.\n\n-e  Regex to match. (May be repeated.)\n-f  File containing regular expressions to match.\n\nmatch type:\n-A  Show NUM lines after     -B  Show NUM lines before match\n-C  NUM lines context (A+B)  -E  extended regex syntax\n-F  fixed (literal match)    -i  case insensitive\n-m  match MAX many lines     -r  recursive (on dir)\n-v  invert match             -w  whole word (implies -E)\n-x  whole line               -z  input NUL terminated\n\ndisplay modes: (default: matched line)\n-c  count of matching lines  -l  show matching filenames\n-o  only matching part       -q  quiet (errors only)\n-s  silent (no error msg)    -Z  output NUL terminated\n\noutput prefix (default: filename if checking more than 1 file)\n-H  force filename           -b  byte offset of match\n-h  hide filename            -n  line number of match\n\n"

#define HELP_find "usage: find [-HL] [DIR...] [<options>]\n\nSearch directories for matching files.\nDefault: search \".\" match all -print all matches.\n\n-H  Follow command line symlinks         -L  Follow all symlinks\n\nMatch filters:\n-name  PATTERN  filename with wildcards   -iname      case insensitive -name\n-path  PATTERN  path name with wildcards  -ipath      case insensitive -path\n-user  UNAME    belongs to user UNAME     -nouser     user ID not known\n-group GROUP    belongs to group GROUP    -nogroup    group ID not known\n-perm  [-/]MODE permissions (-=min /=any) -prune      ignore contents of dir\n-size  N[c]     512 byte blocks (c=bytes) -xdev       only this filesystem\n-links N        hardlink count            -atime N    accessed N days ago\n-ctime N        created N days ago        -mtime N    modified N days ago\n-newer FILE     newer mtime than FILE     -mindepth # at least # dirs down\n-depth          ignore contents of dir    -maxdepth # at most # dirs down\n-inum  N        inode number N            -empty      empty files and dirs\n-type [bcdflps] (block, char, dir, file, symlink, pipe, socket)\n\nNumbers N may be prefixed by a - (less than) or + (greater than):\n\nCombine matches with:\n!, -a, -o, ( )    not, and, or, group expressions\n\nActions:\n-print   Print match with newline  -print0    Print match with null\n-exec    Run command with path     -execdir   Run command in file's dir\n-ok      Ask before exec           -okdir     Ask before execdir\n-delete  Remove matching file/dir\n\nCommands substitute \"{}\" with matched file. End with \";\" to run each file,\nor \"+\" (next argument after \"{}\") to collect and run with multiple files.\n\n"

#define HELP_false "Return nonzero.\n\n"

#define HELP_expand "usage: expand [-t TABLIST] [FILE...]\n\nExpand tabs to spaces according to tabstops.\n\n-t	TABLIST\n\nSpecify tab stops, either a single number instead of the default 8,\nor a comma separated list of increasing numbers representing tabstop\npositions (absolute, not increments) with each additional tab beyound\nthat becoming one space.\n\n"

#define HELP_env "usage: env [-i] [-u NAME] [NAME=VALUE...] [command [option...]]\n\nSet the environment for command invocation.\n\n-i	Clear existing environment.\n-u NAME	Remove NAME from the environment\n\n"

#define HELP_echo "usage: echo [-ne] [args...]\n\nWrite each argument to stdout, with one space between each, followed\nby a newline.\n\n-n	No trailing newline.\n-e	Process the following escape sequences:\n	\\\\	backslash\n	\\0NNN	octal values (1 to 3 digits)\n	\\a	alert (beep/flash)\n	\\b	backspace\n	\\c	stop output here (avoids trailing newline)\n	\\f	form feed\n	\\n	newline\n	\\r	carriage return\n	\\t	horizontal tab\n	\\v	vertical tab\n	\\xHH	hexadecimal values (1 to 2 digits)\n\n"

#define HELP_du "usage: du [-d N] [-askxHLlmc] [file...]\n\nShow disk usage, space consumed by files and directories.\n\nSize in:\n-k    1024 byte blocks (default)\n-K    512 byte blocks (posix)\n-m    megabytes\n-h    human readable format (e.g., 1K 243M 2G )\n\nWhat to show:\n-a    all files, not just directories\n-H    follow symlinks on cmdline\n-L    follow all symlinks\n-s    only total size of each argument\n-x    don't leave this filesystem\n-c    cumulative total\n-d N  only depth < N\n-l    disable hardlink filter\n\n"

#define HELP_dirname "usage: dirname PATH\n\nShow directory portion of path.\n\n"

#define HELP_df "usage: df [-HPkh] [-t type] [FILESYSTEM ...]\n\nThe \"disk free\" command shows total/used/available disk space for\neach filesystem listed on the command line, or all currently mounted\nfilesystems.\n\n-P	The SUSv3 \"Pedantic\" option\n-k	Sets units back to 1024 bytes (the default without -P)\n-h	Human readable output (K=1024)\n-H	Human readable output (k=1000)\n-t type	Display only filesystems of this type.\n\nPedantic provides a slightly less useful output format dictated by Posix,\nand sets the units to 512 bytes instead of the default 1024 bytes.\n\n"

#define HELP_date "usage: date [-u] [-r FILE] [-d DATE] [+DISPLAY_FORMAT] [-D SET_FORMAT] [SET]\n\nSet/get the current date/time. With no SET shows the current date.\n\nDefault SET format is \"MMDDhhmm[[CC]YY][.ss]\", that's (2 digits each)\nmonth, day, hour (0-23), and minute. Optionally century, year, and second.\nAlso accepts \"@UNIXTIME[.FRACTION]\" as seconds since midnight Jan 1 1970.\n\n-d	Show DATE instead of current time (convert date format)\n-D	+FORMAT for SET or -d (instead of MMDDhhmm[[CC]YY][.ss])\n-r	Use modification time of FILE instead of current date\n-u	Use UTC instead of current timezone\n\n+FORMAT specifies display format string using these escapes:\n\n%% literal %             %n newline              %t tab\n%S seconds (00-60)       %M minute (00-59)       %m month (01-12)\n%H hour (0-23)           %I hour (01-12)         %p AM/PM\n%y short year (00-99)    %Y year                 %C century\n%a short weekday name    %A weekday name         %u day of week (1-7, 1=mon)\n%b short month name      %B month name           %Z timezone name\n%j day of year (001-366) %d day of month (01-31) %e day of month ( 1-31)\n%s seconds past the Epoch\n\n%U Week of year (0-53 start sunday)   %W Week of year (0-53 start monday)\n%V Week of year (1-53 start monday, week < 4 days not part of this year)\n\n%D = \"%m/%d/%y\"    %r = \"%I : %M : %S %p\"   %T = \"%H:%M:%S\"   %h = \"%b\"\n%x locale date     %X locale time           %c locale date/time\n\n"

#define HELP_cut "usage: cut OPTION... [FILE]...\n\nPrint selected parts of lines from each FILE to standard output.\n\n-b LIST	select only these bytes from LIST.\n-c LIST	select only these characters from LIST.\n-f LIST	select only these fields.\n-d DELIM	use DELIM instead of TAB for field delimiter.\n-s	do not print lines not containing delimiters.\n-n	don't split multibyte characters (Ignored).\n\n"

#define HELP_cpio "usage: cpio -{o|t|i|p DEST} [-v] [--verbose] [-F FILE] [--no-preserve-owner]\n       [ignored: -mdu -H newc]\n\ncopy files into and out of a \"newc\" format cpio archive\n\n-F FILE	use archive FILE instead of stdin/stdout\n-p DEST	copy-pass mode, copy stdin file list to directory DEST\n-i	extract from archive into file system (stdin=archive)\n-o	create archive (stdin=list of files, stdout=archive)\n-t	test files (list only, stdin=archive, stdout=list of files)\n-v	verbose (list files during create/extract)\n--no-preserve-owner (don't set ownership during extract)\n\n"

#define HELP_install "usage: install [-dDpsv] [-o USER] [-g GROUP] [-m MODE] [SOURCE...] DEST\n\nCopy files and set attributes.\n\n-d	Act like mkdir -p\n-D	Create leading directories for DEST\n-g	Make copy belong to GROUP\n-m	Set permissions to MODE\n-o	Make copy belong to USER\n-p	Preserve timestamps\n-s	Call \"strip -p\"\n-v	Verbose\n\n"

#define HELP_mv "usage: mv [-finv] SOURCE... DEST\"\n\n-f	force copy by deleting destination file\n-i	interactive, prompt before overwriting existing DEST\n-n	no clobber (don't overwrite DEST)\n-v	verbose\n"

#define HELP_cp_preserve "-i	interactive, prompt before overwriting existing DEST\n-l	hard link instead of copy\n-n	no clobber (don't overwrite DEST)\n-p	preserve timestamps, ownership, and mode\n-r	synonym for -R\n-s	symlink instead of copy\n-v	verbose\nletter(s) of:\n\n        mode - permissions (ignore umask for rwx, copy suid and sticky bit)\n   ownership - user and group\n  timestamps - file creation, modification, and access times.\n     context - security context\n       xattr - extended attributes\n         all - all of the above\nusage: cp [--preserve=motcxa] [-adlnrsv] [-fipRHLP] SOURCE... DEST\n\nCopy files from SOURCE to DEST.  If more than one SOURCE, DEST must\nbe a directory.\n\n--preserve takes either a comma separated list of attributes, or the first\n-F	delete any existing destination file first (--remove-destination)\n-H	Follow symlinks listed on command line\n-L	Follow all symlinks\n-P	Do not follow symlinks [default]\n-R	recurse into subdirectories (DEST must be a directory)\n-a	same as -dpr\n-d	don't dereference symlinks\n-f	delete destination files we can't write to\n"

#define HELP_cp "usage: cp [--preserve=motcxa] [-adlnrsv] [-fipRHLP] SOURCE... DEST\n\nCopy files from SOURCE to DEST.  If more than one SOURCE, DEST must\nbe a directory.\n\n--preserve takes either a comma separated list of attributes, or the first\n-F	delete any existing destination file first (--remove-destination)\n-H	Follow symlinks listed on command line\n-L	Follow all symlinks\n-P	Do not follow symlinks [default]\n-R	recurse into subdirectories (DEST must be a directory)\n-a	same as -dpr\n-d	don't dereference symlinks\n-f	delete destination files we can't write to\n-i	interactive, prompt before overwriting existing DEST\n-l	hard link instead of copy\n-n	no clobber (don't overwrite DEST)\n-p	preserve timestamps, ownership, and mode\n-r	synonym for -R\n-s	symlink instead of copy\n-v	verbose\nletter(s) of:\n\n        mode - permissions (ignore umask for rwx, copy suid and sticky bit)\n   ownership - user and group\n  timestamps - file creation, modification, and access times.\n     context - security context\n       xattr - extended attributes\n         all - all of the above\n"

#define HELP_comm "usage: comm [-123] FILE1 FILE2\n\nReads FILE1 and FILE2, which should be ordered, and produces three text\ncolumns as output: lines only in FILE1; lines only in FILE2; and lines\nin both files. Filename \"-\" is a synonym for stdin.\n\n-1 suppress the output column of lines unique to FILE1\n-2 suppress the output column of lines unique to FILE2\n-3 suppress the output column of lines duplicated in FILE1 and FILE2\n\n"

#define HELP_cmp "usage: cmp [-l] [-s] FILE1 FILE2\n\nCompare the contents of two files.\n\n-l	show all differing bytes\n-s	silent\n\n"

#define HELP_cksum "usage: cksum [-IPLN] [file...]\n\nFor each file, output crc32 checksum value, length and name of file.\nIf no files listed, copy from stdin.  Filename \"-\" is a synonym for stdin.\n\n-H	Hexadecimal checksum (defaults to decimal)\n-L	Little endian (defaults to big endian)\n-P	Pre-inversion\n-I	Skip post-inversion\n-N	Do not include length in CRC calculation\n\n"

#define HELP_chmod "usage: chmod [-R] MODE FILE...\n\nChange mode of listed file[s] (recursively with -R).\n\nMODE can be (comma-separated) stanzas: [ugoa][+-=][rwxstXugo]\n\nStanzas are applied in order: For each category (u = user,\ng = group, o = other, a = all three, if none specified default is a),\nset (+), clear (-), or copy (=), r = read, w = write, x = execute.\ns = u+s = suid, g+s = sgid, o+s = sticky. (+t is an alias for o+s).\nsuid/sgid: execute as the user/group who owns the file.\nsticky: can't delete files you don't own out of this directory\nX = x for directories or if any category already has x set.\n\nOr MODE can be an octal value up to 7777	ug uuugggooo	top +\nbit 1 = o+x, bit 1<<8 = u+w, 1<<11 = g+1	sstrwxrwxrwx	bottom\n\nExamples:\nchmod u+w file - allow owner of \"file\" to write to it.\nchmod 744 file - user can read/write/execute, everyone else read only\n\n"

#define HELP_chown "see: chgrp\n\n"

#define HELP_chgrp "usage: chgrp/chown [-RHLP] [-fvh] group file...\n\nChange group of one or more files.\n\n-f	suppress most error messages.\n-h	change symlinks instead of what they point to\n-R	recurse into subdirectories (implies -h).\n-H	with -R change target of symlink, follow command line symlinks\n-L	with -R change target of symlink, follow all symlinks\n-P	with -R change symlink, do not follow symlinks (default)\n-v	verbose output.\n\n"

#define HELP_catv "usage: catv [-evt] [filename...]\n\nDisplay nonprinting characters as escape sequences. Use M-x for\nhigh ascii characters (>127), and ^x for other nonprinting chars.\n\n-e  Mark each newline with $\n-t  Show tabs as ^I\n-v  Don't use ^x or M-x escapes.\n\n"

#define HELP_cat "usage: cat [-etuv] [file...]\n\nCopy (concatenate) files to stdout.  If no files listed, copy from stdin.\nFilename \"-\" is a synonym for stdin.\n\n-e	Mark each newline with $\n-t	Show tabs as ^I\n-u	Copy one byte at a time (slow).\n-v	Display nonprinting characters as escape sequences. Use M-x for\n	high ascii characters (>127), and ^x for other nonprinting chars.\n"

#define HELP_cal "usage: cal [[month] year]\n\nPrint a calendar.\n\nWith one argument, prints all months of the specified year.\nWith two arguments, prints calendar for month and year.\n\n"

#define HELP_basename "usage: basename string [suffix]\n\nReturn non-directory portion of a pathname removing suffix\n\n"

