AC_INIT([SmoothUV], [1], [https://github.com/dubhater/vapoursynth-smoothuv/issues], [SmoothUV], [https://github.com/dubhater/vapoursynth-smoothuv])

: ${CFLAGS=""}
: ${CXXFLAGS=""}

AM_INIT_AUTOMAKE([foreign no-dist-gzip dist-xz subdir-objects no-define])
AM_SILENT_RULES([yes])

LT_INIT([disable-static win32-dll])

AC_PROG_CXX

AC_CANONICAL_HOST


X86="false"

AS_CASE(
  [$host_cpu],
  [i?86],     [BITS="32" X86="true"],
  [x86_64],   [BITS="64" X86="true"],
  [AC_MSG_ERROR([Currently only x86 systems are supported.])]
)

AS_CASE(
   [$host_os],
   [cygwin*|mingw*],
   [
      AS_IF(
         [test "x$BITS" = "x32"],
         [
          AC_SUBST([PLUGINLDFLAGS], ["-Wl,--kill-at"])
         ]
      )
   ]
)


AS_IF(
      [test "x$X86" = "xtrue"],
      [
       AC_SUBST([MFLAGS], ["-mfpmath=sse -msse2"])
      ]
)

PKG_CHECK_MODULES([VapourSynth], [vapoursynth])


dnl Workaround for a bug in libtool
dnl The windows libtool uses a file magic checking method that only accepts
dnl dynamic libraries. Change it for libtool's alternative checking method.

dnl Workaround found in configure.ac from ffms2.

dnl Reproducing the error message below, for search engines and people
dnl looking for a solution...

dnl *** Warning: linker path does not have real file for library -lfftw3f.
dnl *** I have the capability to make that library automatically link in when
dnl *** you link to this library.  But I can only do this if you have a
dnl *** shared version of the library, which you do not appear to have
dnl *** because I did check the linker path looking for a file starting
dnl *** with libfftw3f and none of the candidates passed a file format test
dnl *** using a file magic. Last file checked: /home/asdf/mingw-w64/i686/mingw/lib/libfftw3f.a
dnl *** The inter-library dependencies that have been dropped here will be
dnl *** automatically added whenever a program is linked with this library
dnl *** or is declared to -dlopen it.
dnl *
dnl *** Since this library must not contain undefined symbols,
dnl *** because either the platform does not support them or
dnl *** it was explicitly requested with -no-undefined,
dnl *** libtool will only create a static version of it.

if test "$lt_cv_file_magic_cmd" = "func_win32_libid" ; then
    deplibs_check_method='file_magic file format pei*-(i386|x86-64)|(.*architecture: i386)?'
    file_magic_cmd='$OBJDUMP -f'
fi


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
