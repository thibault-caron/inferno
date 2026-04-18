#ifndef LPTF_EXCEPTION
#define LPTF_EXCEPTION

#include <exception>
#include <string>

class InfernoException : public std::exception {
 private:
  std::string fullMessage;

 public:
  InfernoException(const std::string& msg, const std::string& value) {
    fullMessage = msg + " : " + value;
  }

  const char* what() const noexcept override { return fullMessage.c_str(); }
};

class InvalidIdentifier : public InfernoException {
 public:
  InvalidIdentifier(const std::string& value)
      : InfernoException("Wrong identifier", value) {}
};

class UnsupportedVersion : public InfernoException {
 public:
  UnsupportedVersion(const std::string& value,
                     const std::string& msg = "Unsupported version")
      : InfernoException(msg, value) {}
};

class InvalidType : public InfernoException {
 public:
  InvalidType(const std::string& value)
      : InfernoException("Invalid message type", value) {}
};

class InvalidSize : public InfernoException {
 public:
  // source = header, payload...
  InvalidSize(const std::string& source, const std::string& value)
      : InfernoException("Invalid size for " + source, value) {}
};

class InvalidFieldValue : public InfernoException {
 public:
  InvalidFieldValue(const std::string& field, const std::string& value)
      : InfernoException("Unsupported value for " + field, value) {}
};

#endif