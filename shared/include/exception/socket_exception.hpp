#ifndef SOCKET_EXCEPTION
#define SOCKET_EXCEPTION
#include "lptf_exception.hpp"
#include <exception>
#include <string>

class SendFailure : public InfernoException {
 public:
  SendFailure(const std::string& messageType)
      : InfernoException("Failed to send message", messageType) {}
};



#endif