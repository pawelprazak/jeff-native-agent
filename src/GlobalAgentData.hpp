#ifndef JEFF_NATIVE_AGENT_GLOBALAGENTDATA_HPP
#define JEFF_NATIVE_AGENT_GLOBALAGENTDATA_HPP

#include <jni.h>
#include <jvmti.h>
#include "Sender.h"

namespace jeff {

    typedef struct GlobalAgentData {
        JavaVM *jvm;
        /* JVMTI Environment */
        jvmtiEnv *jvmti;
        jboolean vm_is_dead;
        jboolean vm_is_initialized;
        jboolean vm_is_started;
        /* Data access Lock */
        jrawMonitorID lock;
        /* Networking */
        std::unique_ptr<Sender> sender;
    } GlobalAgentData;

    extern GlobalAgentData gdata;
}
#endif //JEFF_NATIVE_AGENT_GLOBALAGENTDATA_HPP
