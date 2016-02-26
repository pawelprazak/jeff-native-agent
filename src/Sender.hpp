#ifndef JEFF_NATIVE_AGENT_SENDER_H
#define JEFF_NATIVE_AGENT_SENDER_H

#include <string>
#include <iostream>
#include <memory>

class Sender {
public:
    virtual ~Sender() { };

    virtual void send(std::string value) = 0;

    virtual void start() = 0;

    virtual void stop() = 0;

    virtual void flush() = 0;

    static std::unique_ptr<Sender> create();

    static std::unique_ptr<Sender> create(std::string host, std::string port);
};

#endif //JEFF_NATIVE_AGENT_SENDER_H
