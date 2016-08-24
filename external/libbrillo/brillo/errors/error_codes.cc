// Copyright 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <brillo/errors/error_codes.h>

#include <base/posix/safe_strerror.h>

namespace brillo {
namespace errors {

namespace dbus {
const char kDomain[] = "dbus";
}  // namespace dbus

namespace json {
const char kDomain[] = "json_parser";
const char kParseError[] = "json_parse_error";
const char kObjectExpected[] = "json_object_expected";
}  // namespace json

namespace http {
const char kDomain[] = "http";
}  // namespace http

namespace system {
const char kDomain[] = "system";

namespace {
const struct ErrorMapEntry {
  const char* error_code;
  int errnum;
} error_map[] = {
#define ERROR_ENTRY(err) { #err, err }
  ERROR_ENTRY(EPERM),            // Operation not permitted
  ERROR_ENTRY(ENOENT),           // No such file or directory
  ERROR_ENTRY(ESRCH),            // No such process
  ERROR_ENTRY(EINTR),            // Interrupted system call
  ERROR_ENTRY(EIO),              // I/O error
  ERROR_ENTRY(ENXIO),            // No such device or address
  ERROR_ENTRY(E2BIG),            // Argument list too long
  ERROR_ENTRY(ENOEXEC),          // Exec format error
  ERROR_ENTRY(EBADF),            // Bad file number
  ERROR_ENTRY(ECHILD),           // No child processes
  ERROR_ENTRY(EAGAIN),           // Try again
  ERROR_ENTRY(ENOMEM),           // Out of memory
  ERROR_ENTRY(EACCES),           // Permission denied
  ERROR_ENTRY(EFAULT),           // Bad address
  ERROR_ENTRY(ENOTBLK),          // Block device required
  ERROR_ENTRY(EBUSY),            // Device or resource busy
  ERROR_ENTRY(EEXIST),           // File exists
  ERROR_ENTRY(EXDEV),            // Cross-device link
  ERROR_ENTRY(ENODEV),           // No such device
  ERROR_ENTRY(ENOTDIR),          // Not a directory
  ERROR_ENTRY(EISDIR),           // Is a directory
  ERROR_ENTRY(EINVAL),           // Invalid argument
  ERROR_ENTRY(ENFILE),           // File table overflow
  ERROR_ENTRY(EMFILE),           // Too many open files
  ERROR_ENTRY(ENOTTY),           // Not a typewriter
  ERROR_ENTRY(ETXTBSY),          // Text file busy
  ERROR_ENTRY(EFBIG),            // File too large
  ERROR_ENTRY(ENOSPC),           // No space left on device
  ERROR_ENTRY(ESPIPE),           // Illegal seek
  ERROR_ENTRY(EROFS),            // Read-only file system
  ERROR_ENTRY(EMLINK),           // Too many links
  ERROR_ENTRY(EPIPE),            // Broken pipe
  ERROR_ENTRY(EDOM),             // Math argument out of domain of func
  ERROR_ENTRY(ERANGE),           // Math result not representable
  ERROR_ENTRY(EDEADLK),          // Resource deadlock would occur
  ERROR_ENTRY(ENAMETOOLONG),     // File name too long
  ERROR_ENTRY(ENOLCK),           // No record locks available
  ERROR_ENTRY(ENOSYS),           // Function not implemented
  ERROR_ENTRY(ENOTEMPTY),        // Directory not empty
  ERROR_ENTRY(ELOOP),            // Too many symbolic links encountered
  ERROR_ENTRY(ENOMSG),           // No message of desired type
  ERROR_ENTRY(EIDRM),            // Identifier removed
#ifdef __linux__
  ERROR_ENTRY(ECHRNG),           // Channel number out of range
  ERROR_ENTRY(EL2NSYNC),         // Level 2 not synchronized
  ERROR_ENTRY(EL3HLT),           // Level 3 halted
  ERROR_ENTRY(EL3RST),           // Level 3 reset
  ERROR_ENTRY(ELNRNG),           // Link number out of range
  ERROR_ENTRY(EUNATCH),          // Protocol driver not attached
  ERROR_ENTRY(ENOCSI),           // No CSI structure available
  ERROR_ENTRY(EL2HLT),           // Level 2 halted
  ERROR_ENTRY(EBADE),            // Invalid exchange
  ERROR_ENTRY(EBADR),            // Invalid request descriptor
  ERROR_ENTRY(EXFULL),           // Exchange full
  ERROR_ENTRY(ENOANO),           // No anode
  ERROR_ENTRY(EBADRQC),          // Invalid request code
  ERROR_ENTRY(EBADSLT),          // Invalid slot
  ERROR_ENTRY(EBFONT),           // Bad font file format
#endif  // __linux__
  ERROR_ENTRY(ENOSTR),           // Device not a stream
  ERROR_ENTRY(ENODATA),          // No data available
  ERROR_ENTRY(ETIME),            // Timer expired
  ERROR_ENTRY(ENOSR),            // Out of streams resources
#ifdef __linux__
  ERROR_ENTRY(ENONET),           // Machine is not on the network
  ERROR_ENTRY(ENOPKG),           // Package not installed
#endif  // __linux__
  ERROR_ENTRY(EREMOTE),          // Object is remote
  ERROR_ENTRY(ENOLINK),          // Link has been severed
#ifdef __linux__
  ERROR_ENTRY(EADV),             // Advertise error
  ERROR_ENTRY(ESRMNT),           // Srmount error
  ERROR_ENTRY(ECOMM),            // Communication error on send
#endif  // __linux__
  ERROR_ENTRY(EPROTO),           // Protocol error
  ERROR_ENTRY(EMULTIHOP),        // Multihop attempted
#ifdef __linux__
  ERROR_ENTRY(EDOTDOT),          // RFS specific error
#endif  // __linux__
  ERROR_ENTRY(EBADMSG),          // Not a data message
  ERROR_ENTRY(EOVERFLOW),        // Value too large for defined data type
#ifdef __linux__
  ERROR_ENTRY(ENOTUNIQ),         // Name not unique on network
  ERROR_ENTRY(EBADFD),           // File descriptor in bad state
  ERROR_ENTRY(EREMCHG),          // Remote address changed
  ERROR_ENTRY(ELIBACC),          // Can not access a needed shared library
  ERROR_ENTRY(ELIBBAD),          // Accessing a corrupted shared library
  ERROR_ENTRY(ELIBSCN),          // .lib section in a.out corrupted
  ERROR_ENTRY(ELIBMAX),          // Attempting to link in too many shared libs.
  ERROR_ENTRY(ELIBEXEC),         // Cannot exec a shared library directly
#endif  // __linux__
  ERROR_ENTRY(EILSEQ),           // Illegal byte sequence
#ifdef __linux__
  ERROR_ENTRY(ERESTART),         // Interrupted system call should be restarted
  ERROR_ENTRY(ESTRPIPE),         // Streams pipe error
#endif  // __linux__
  ERROR_ENTRY(EUSERS),           // Too many users
  ERROR_ENTRY(ENOTSOCK),         // Socket operation on non-socket
  ERROR_ENTRY(EDESTADDRREQ),     // Destination address required
  ERROR_ENTRY(EMSGSIZE),         // Message too long
  ERROR_ENTRY(EPROTOTYPE),       // Protocol wrong type for socket
  ERROR_ENTRY(ENOPROTOOPT),      // Protocol not available
  ERROR_ENTRY(EPROTONOSUPPORT),  // Protocol not supported
  ERROR_ENTRY(ESOCKTNOSUPPORT),  // Socket type not supported
  ERROR_ENTRY(EOPNOTSUPP),       // Operation not supported o/transport endpoint
  ERROR_ENTRY(EPFNOSUPPORT),     // Protocol family not supported
  ERROR_ENTRY(EAFNOSUPPORT),     // Address family not supported by protocol
  ERROR_ENTRY(EADDRINUSE),       // Address already in use
  ERROR_ENTRY(EADDRNOTAVAIL),    // Cannot assign requested address
  ERROR_ENTRY(ENETDOWN),         // Network is down
  ERROR_ENTRY(ENETUNREACH),      // Network is unreachable
  ERROR_ENTRY(ENETRESET),        // Network dropped connection because of reset
  ERROR_ENTRY(ECONNABORTED),     // Software caused connection abort
  ERROR_ENTRY(ECONNRESET),       // Connection reset by peer
  ERROR_ENTRY(ENOBUFS),          // No buffer space available
  ERROR_ENTRY(EISCONN),          // Transport endpoint is already connected
  ERROR_ENTRY(ENOTCONN),         // Transport endpoint is not connected
  ERROR_ENTRY(ESHUTDOWN),        // Cannot send after transp. endpoint shutdown
  ERROR_ENTRY(ETOOMANYREFS),     // Too many references: cannot splice
  ERROR_ENTRY(ETIMEDOUT),        // Connection timed out
  ERROR_ENTRY(ECONNREFUSED),     // Connection refused
  ERROR_ENTRY(EHOSTDOWN),        // Host is down
  ERROR_ENTRY(EHOSTUNREACH),     // No route to host
  ERROR_ENTRY(EALREADY),         // Operation already in progress
  ERROR_ENTRY(EINPROGRESS),      // Operation now in progress
  ERROR_ENTRY(ESTALE),           // Stale file handle
#ifdef __linux__
  ERROR_ENTRY(EUCLEAN),          // Structure needs cleaning
  ERROR_ENTRY(ENOTNAM),          // Not a XENIX named type file
  ERROR_ENTRY(ENAVAIL),          // No XENIX semaphores available
  ERROR_ENTRY(EISNAM),           // Is a named type file
  ERROR_ENTRY(EREMOTEIO),        // Remote I/O error
#endif  // __linux__
  ERROR_ENTRY(EDQUOT),           // Quota exceeded
#ifdef __linux__
  ERROR_ENTRY(ENOMEDIUM),        // No medium found
  ERROR_ENTRY(EMEDIUMTYPE),      // Wrong medium type
#endif  // __linux__
  ERROR_ENTRY(ECANCELED),        // Operation Canceled
#ifdef __linux__
  ERROR_ENTRY(ENOKEY),           // Required key not available
  ERROR_ENTRY(EKEYEXPIRED),      // Key has expired
  ERROR_ENTRY(EKEYREVOKED),      // Key has been revoked
  ERROR_ENTRY(EKEYREJECTED),     // Key was rejected by service
#endif  // __linux__
  ERROR_ENTRY(EOWNERDEAD),       // Owner died
  ERROR_ENTRY(ENOTRECOVERABLE),  // State not recoverable
#ifdef __linux__
  ERROR_ENTRY(ERFKILL),          // Operation not possible due to RF-kill
  ERROR_ENTRY(EHWPOISON),        // Memory page has hardware error
#endif  // __linux__
#undef ERROR_ENTRY
  // This list comes from <errno.h> system header. The elements are ordered
  // by increasing errnum values which is the same order used in the header
  // file. So, when new error codes are added to glibc, it should be relatively
  // easy to identify them and add them to this list.
};

// Gets the error code string from system error code. If unknown system error
// number is provided, returns an empty string.
std::string ErrorCodeFromSystemError(int errnum) {
  std::string error_code;
  for (const ErrorMapEntry& entry : error_map) {
    if (entry.errnum == errnum) {
      error_code = entry.error_code;
      break;
    }
  }
  return error_code;
}

}  // anonymous namespace

void AddSystemError(ErrorPtr* error,
                    const tracked_objects::Location& location,
                    int errnum) {
  std::string message = base::safe_strerror(errnum);
  std::string code = ErrorCodeFromSystemError(errnum);
  if (message.empty())
    message = "Unknown error " + std::to_string(errnum);

  if (code.empty())
    code = "error_" + std::to_string(errnum);

  Error::AddTo(error, location, kDomain, code, message);
}

}  // namespace system

}  // namespace errors
}  // namespace brillo
