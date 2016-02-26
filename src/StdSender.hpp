#ifndef JEFF_NATIVE_AGENT_STDSENDER_H
#define JEFF_NATIVE_AGENT_STDSENDER_H

#include <string>
#include <memory>

#include "Sender.hpp"

class StdSender : public Sender {
public:
    virtual ~StdSender();

    virtual void start();

    virtual void stop();

    virtual void flush();

    virtual void send(std::string value);
};


#endif //JEFF_NATIVE_AGENT_STDSENDER_H
