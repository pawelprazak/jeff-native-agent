#include <iostream>

#include "StdSender.hpp"

StdSender::~StdSender() {
    // Empty
}

void StdSender::send(std::string message) {
    std::cout << message << std::endl;
}

void StdSender::start() {
    // Empty
}

void StdSender::stop() {
    // Empty
}

void StdSender::flush() {
    // Empty
}
