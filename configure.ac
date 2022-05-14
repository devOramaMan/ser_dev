# initialize the process
AC_PREREQ([2.69])
AC_INIT([ser_ftdi], [0.01])
# make config headers
AC_CONFIG_HEADERS([./inc/config.h])
#Auxiliary files go here
AC_CONFIG_AUX_DIR([build-aux])
# init automake
AM_INIT_AUTOMAKE([1.11])


#configure and create "Makefile"
#find and probe C compiler
AC_PROG_CC

AC_CANONICAL_HOST
HOST_OS_FLAG=""
AS_CASE([$host_cpu],
        [x86_64|aarch64*|mips64*|ppc64*|sparc64],
        [
            HOST_OS_FLAG="64"
        ],
        [i?86|arm*|mips*|ppc*|sparc],
        [
            HOST_OS_FLAG=""
        ])

AC_SUBST([HOST_OS_FLAG])

EXTRA_CFLAGS=""
EXTRA_LDFLAGS=""

AC_MSG_CHECKING([operating system])
case $host in
*-darwin*)
	AC_MSG_RESULT([Darwin/Mac OS X])
	backend=darwin
	platform=posix
	;;
*-haiku*)
	AC_MSG_RESULT([Haiku])
	backend=haiku
	platform=posix
	;;
*-linux* | *-uclinux*)
	dnl on Android Linux, some functions are in different places
	case $host in
	*-linux-android*)
		AC_MSG_RESULT([Android Linux])
		is_android_linux=yes
		;;
	*)
		AC_MSG_RESULT([Linux])
		;;
	esac
	backend=linux
	platform=posix
	;;
*-netbsd*)
	AC_MSG_RESULT([NetBSD])
	backend=netbsd
	platform=posix
	;;
*-openbsd*)
	AC_MSG_RESULT([OpenBSD])
	backend=openbsd
	platform=posix
	;;
*-solaris*)
	AC_MSG_RESULT([SunOS])
	backend=sunos
	platform=posix
	;;
*-cygwin*)
	AC_MSG_RESULT([Windows (using Cygwin)])
	backend=windows
	platform=windows
	EXTRA_CFLAGS="-Wall -Werror -pthread"
	EXTRA_LDFLAGS="-lpthread"
	;;
*-mingw* | *msys*)
	AC_MSG_RESULT([Windows])
	backend=windows
	platform=windows
	test "x$enable_shared" = xyes && create_import_lib=yes
	EXTRA_CFLAGS="-mwin32 -fno-omit-frame-pointer"
	EXTRA_LDFLAGS="-static-libgcc"
	;;
*)
	AC_MSG_RESULT([Null])
	#AC_MSG_WARN([The host being compiled for is not supported.])
	#AC_MSG_WARN([The library may compile but will not function in any useful manner.])
    AC_MSG_ERROR(["Missing OS/platform configuraiton"])
	backend=null
	platform=posix
	;;
esac

if test "x$platform" = xposix; then
	AC_DEFINE([PLATFORM_POSIX], [1], [Define to 1 if compiling for a POSIX platform.])
	AC_CHECK_TYPES([nfds_t], [], [], [[#include <poll.h>]])
	AC_CHECK_FUNCS([pipe2])
	dnl Some compilers do not support the '-pthread' option so check for it here
	saved_CFLAGS="${CFLAGS}"
	CFLAGS="-Wall -Werror -pthread"
	AC_MSG_CHECKING([if $CC recognizes -pthread])
	AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [])],
		[AC_MSG_RESULT([yes])
		 AC_SUBST(THREAD_CFLAGS, [-pthread])],
		[AC_MSG_RESULT([no])])
	CFLAGS="${saved_CFLAGS}"
	dnl Android Linux and Darwin provide pthread functions directly in libc
	dnl glibc also provides some pthread functions directly, so search for a thread-specific function
	AC_SEARCH_LIBS([pthread_create], [pthread],
		[test "x$ac_cv_search_pthread_create" != "xnone required" && AC_SUBST(THREAD_LIBS, [-lpthread])],
		[], [])
	AC_SEARCH_LIBS([__atomic_fetch_add_4], [atomic])
elif test "x$platform" = xwindows; then
	AC_DEFINE([PLATFORM_WINDOWS], [1], [Define to 1 if compiling for a Windows platform.])
    AC_DEFINE([_WIN32], [1], [Define to 1 if compiling for a Windows platform.])
else
	AC_MSG_ERROR([Unknown platform])
fi

case $backend in
darwin)
	AC_CHECK_FUNCS([pthread_threadid_np])
	LIBS="${LIBS} -lobjc -Wl,-framework,IOKit -Wl,-framework,CoreFoundation -Wl,-framework,Security"
	AC_CHECK_HEADERS([IOKit/usb/IOUSBHostFamilyDefinitions.h])
	;;
haiku)
	LIBS="${LIBS} -lbe"
	;;
linux)
	AC_SEARCH_LIBS([clock_gettime], [rt], [], [], [])
	AC_CHECK_FUNCS([pthread_setname_np])
	AC_ARG_ENABLE([udev],
		[AS_HELP_STRING([--enable-udev], [use udev for device enumeration and hotplug support (recommended) [default=yes]])],
		[use_udev=$enableval], [use_udev=yes])
	if test "x$use_udev" = xyes; then
		dnl system has udev. use it or fail!
		AC_CHECK_HEADER([libudev.h], [], [AC_MSG_ERROR([udev support requested but libudev header not installed])])
		AC_CHECK_LIB([udev], [udev_new], [], [AC_MSG_ERROR([udev support requested but libudev not installed])])

		# We can build umockdev tests (if available)
		m4_ifdef([PKG_PROG_PKG_CONFIG],[
			PKG_PROG_PKG_CONFIG
		   	PKG_CHECK_MODULES([UMOCKDEV], [umockdev-1.0 >= 0.16.0], [ac_have_umockdev=yes], [ac_have_umockdev=no])
		   	PKG_CHECK_MODULES([UMOCKDEV_HOTPLUG], [umockdev-1.0 >= 0.17.7], [ac_umockdev_hotplug=yes], [ac_umockdev_hotplug=no])
			if test $ac_umockdev_hotplug = yes; then
			   AC_DEFINE([UMOCKDEV_HOTPLUG], [1], [UMockdev hotplug code is not racy])
			fi
		], [])
	else
		AC_CHECK_HEADERS([asm/types.h])
		AC_CHECK_HEADER([linux/netlink.h], [], [AC_MSG_ERROR([Linux netlink header not found])])
		AC_CHECK_HEADER([sys/socket.h], [], [AC_MSG_ERROR([Linux socket header not found])])
	fi
	;;
sunos)
	LIBS="${LIBS} -ldevinfo"
	;;
windows)
	AC_CHECK_TYPES([struct timespec], [], [], [[#include <time.h>]])
	AC_DEFINE([_WIN32], [1], [Define.])
	LT_LDFLAGS="${LT_LDFLAGS} -avoid-version -Wl,--add-stdcall-alias"
	;;
*)
	dnl no special handling required
	;;
esac

dnl headers not available on all platforms but required on others
AC_CHECK_HEADERS([sys/time.h])

if test "x$platform" = xposix; then
	dnl check availability of clock_gettime()
	if test "x$backend" = xdarwin; then
		dnl need to verify that OS X target is 10.12 or later for clock_gettime()
		AC_MSG_CHECKING([whether OS X target version is 10.12 or later])
		AC_COMPILE_IFELSE([AC_LANG_PROGRAM([
				#include <AvailabilityMacros.h>
				#if MAC_OS_X_VERSION_MIN_REQUIRED < 101200
				# error "Target OS X version is too old"
				#endif
			], [])],
			[AC_MSG_RESULT([yes])
			 osx_10_12_or_later=yes],
			[AC_MSG_RESULT([no])
			 osx_10_12_or_later=])
		if test "x$osx_10_12_or_later" = xyes; then
			AC_CHECK_FUNCS([clock_gettime], [have_clock_gettime=yes], [have_clock_gettime=])
		else
			AC_MSG_NOTICE([clock_gettime() is not available on target OS X version])
		fi
	else
		AC_CHECK_FUNCS([clock_gettime], [have_clock_gettime=yes], [AC_MSG_ERROR([clock_gettime() is required on this platform])])
	fi

	if test "x$have_clock_gettime" = xyes; then
		dnl the clock_gettime() function needs certain clock IDs defined
		AC_CHECK_DECL([CLOCK_MONOTONIC], [], [AC_MSG_ERROR([C library headers missing definition for CLOCK_MONOTONIC])], [[#include <time.h>]])
		dnl use the monotonic clock for condition variable timed waits if possible
		AC_CHECK_FUNCS([pthread_condattr_setclock], [need_clock_realtime=], [need_clock_realtime=yes])
		if test "x$need_clock_realtime" = xyes; then
			AC_CHECK_DECL([CLOCK_REALTIME], [], [AC_MSG_ERROR([C library headers missing definition for CLOCK_REALTIME])], [[#include <time.h>]])
		fi
	fi
fi

dnl eventfd support
if test "x$backend" = xlinux || test "x$backend" = xsunos; then
	AC_ARG_ENABLE([eventfd],
		[AS_HELP_STRING([--enable-eventfd], [use eventfd for signalling [default=auto]])],
		[use_eventfd=$enableval],
		[use_eventfd=auto])
	if test "x$use_eventfd" != xno; then
		AC_CHECK_HEADER([sys/eventfd.h], [eventfd_h=yes], [eventfd_h=])
		if test "x$eventfd_h" = xyes; then
			AC_CHECK_DECLS([EFD_NONBLOCK, EFD_CLOEXEC], [eventfd_h_ok=yes], [eventfd_h_ok=], [[#include <sys/eventfd.h>]])
			if test "x$eventfd_h_ok" = xyes; then
				AC_CHECK_FUNC([eventfd], [eventfd_ok=yes], [eventfd_ok=])
				if test "x$eventfd_ok" = xyes; then
					AC_DEFINE([HAVE_EVENTFD], [1], [Define to 1 if the system has eventfd functionality.])
				elif test "x$use_eventfd" = xyes; then
					AC_MSG_ERROR([eventfd() function not found; glibc 2.9+ required])
				fi
			elif test "x$use_eventfd" = xyes; then
				AC_MSG_ERROR([eventfd header not usable; glibc 2.9+ required])
			fi
		elif test "x$use_eventfd" = xyes; then
			AC_MSG_ERROR([eventfd header not available; glibc 2.9+ required])
		fi
	fi
	AC_MSG_CHECKING([whether to use eventfd for signalling])
	if test "x$use_eventfd" = xno; then
		AC_MSG_RESULT([no (disabled by user)])
	elif test "x$eventfd_h" != xyes; then
		AC_MSG_RESULT([no (header not available)])
	elif test "x$eventfd_h_ok" != xyes; then
		AC_MSG_RESULT([no (header not usable)])
	elif test "x$eventfd_ok" != xyes; then
		AC_MSG_RESULT([no (functions not available)])
	else
		AC_MSG_RESULT([yes])
	fi
fi

dnl timerfd support
if test "x$backend" = xlinux || test "x$backend" = xsunos; then
	AC_ARG_ENABLE([timerfd],
		[AS_HELP_STRING([--enable-timerfd], [use timerfd for timing [default=auto]])],
		[use_timerfd=$enableval],
		[use_timerfd=auto])
	if test "x$use_timerfd" != xno; then
		AC_CHECK_HEADER([sys/timerfd.h], [timerfd_h=yes], [timerfd_h=])
		if test "x$timerfd_h" = xyes; then
			AC_CHECK_DECLS([TFD_NONBLOCK, TFD_CLOEXEC], [timerfd_h_ok=yes], [timerfd_h_ok=], [[#include <sys/timerfd.h>]])
			if test "x$timerfd_h_ok" = xyes; then
				AC_CHECK_FUNC([timerfd_create], [timerfd_ok=yes], [timerfd_ok=])
				if test "x$timerfd_ok" = xyes; then
					AC_DEFINE([HAVE_TIMERFD], [1], [Define to 1 if the system has timerfd functionality.])
				elif test "x$use_timerfd" = xyes; then
					AC_MSG_ERROR([timerfd_create() function not found; glibc 2.9+ required])
				fi
			elif test "x$use_timerfd" = xyes; then
				AC_MSG_ERROR([timerfd header not usable; glibc 2.9+ required])
			fi
		elif test "x$use_timerfd" = xyes; then
			AC_MSG_ERROR([timerfd header not available; glibc 2.9+ required])
		fi
	fi
	AC_MSG_CHECKING([whether to use timerfd for timing])
	if test "x$use_timerfd" = xno; then
		AC_MSG_RESULT([no (disabled by user)])
	elif test "x$timerfd_h" != xyes; then
		AC_MSG_RESULT([no (header not available)])
	elif test "x$timerfd_h_ok" != xyes; then
		AC_MSG_RESULT([no (header not usable)])
	elif test "x$timerfd_ok" != xyes; then
		AC_MSG_RESULT([no (functions not available)])
	else
		AC_MSG_RESULT([yes])
	fi
fi

dnl Message logging
AC_ARG_ENABLE([log],
	[AS_HELP_STRING([--disable-log], [disable all logging])],
	[log_enabled=$enableval],
	[log_enabled=yes])
if test "x$log_enabled" != xno; then
	AC_DEFINE([ENABLE_LOGGING], [1], [Define to 1 to enable message logging.])
fi

AC_ARG_ENABLE([debug-log],
	[AS_HELP_STRING([--enable-debug-log], [start with debug message logging enabled [default=no]])],
	[debug_log_enabled=$enableval],
	[debug_log_enabled=no])
if test "x$debug_log_enabled" != xno; then
	AC_DEFINE([ENABLE_DEBUG_LOGGING], [1], [Define to 1 to start with debug message logging enabled.])
fi

AC_ARG_ENABLE([system-log],
	[AS_HELP_STRING([--enable-system-log], [output logging messages to the systemwide log, if supported by the OS [default=no]])],
	[system_log_enabled=$enableval],
	[system_log_enabled=no])
if test "x$system_log_enabled" != xno; then
	AC_DEFINE([USE_SYSTEM_LOGGING_FACILITY], [1], [Define to 1 to output logging messages to the systemwide log.])
	if test "x$backend" != xwindows && test "x$is_android_linux" != xyes; then
		dnl Check if syslog is available in standard C library
		AC_CHECK_HEADER([syslog.h], [syslog_h=yes], [syslog_h=])
		if test "x$syslog_h" = xyes; then
			AC_CHECK_FUNCS([syslog])
		fi
	fi
fi

AC_SUBST([EXTRA_CFLAGS])
AC_SUBST([EXTRA_LDFLAGS])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([ser_ftdi/Makefile])


#End
AC_OUTPUT