/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright (c) 2006 - 2014 Caustic Graphics, Inc.    All rights reserved.        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#ifndef __OPENRL_H__
#define __OPENRL_H__

#include <OpenRL/rlplatform.h>
#include <OpenRL/rl.h>
#include <stddef.h>

typedef void * OpenRLContext;
typedef RLintptr OpenRLContextAttribute;
typedef void (*OpenRLNotify)(
	RLenum error,
	const void * private_data,
	size_t private_size,
	const char * message,
	void * user_data);

typedef void (* OpenRLDebugCallback) (RLenum type, void *data, size_t len, int hasDifferentials, int tag, float frameX, float frameY, const char *primName, int stringIndex, int line, void *userData); 
	
/* OpenRL context creation attributes */
enum {
	kOpenRL_RequireHardwareAcceleration = 1,
	kOpenRL_ExcludeCPUCores = 2,
	kOpenRL_DisableHyperthreading = 3,
	kOpenRL_DisableStats = 4,
	kOpenRL_DisableProfiling = 5,
	kOpenRL_DisableTokenStream = 6,
	kOpenRL_Reserved = 7,
	kOpenRL_ForceCPU = 8,
	kOpenRL_DisableDifferentials = 9,
	kOpenRL_EnableRayPrefixShaders = 10,
	kOpenRL_WorkerThreadPriority = 11,
	kOpenRL_EnableFilteredAccumulates = 12,

	kOpenRL_ContextAttributeEnd         // Bounds the enum values: must be one more than the value of the largest enum value.
};

#define RL_CONTEXT_STOPPED_CST 0x0580

#ifdef __cplusplus
extern "C" {
#endif
	
RL_APICALL OpenRLContext OpenRLCreateContext(
	const OpenRLContextAttribute * attributes,
	OpenRLNotify notify,
	void * user_data);
RL_APICALL void OpenRLStopContext(OpenRLContext ctx);
RL_APICALL int OpenRLDestroyContext(OpenRLContext ctx);
RL_APICALL int OpenRLSetCurrentContext(OpenRLContext ctx);
RL_APICALL OpenRLContext OpenRLGetCurrentContext();
RL_APICALL void OpenRLSetContextDebugCallback(OpenRLContext ctx, OpenRLDebugCallback cb, void *data);

RL_APICALL unsigned int OpenRLGetHardwareAcceleratorCount();

RL_APICALL void OpenRLLogMessagesToStdout(RLenum, const void *, size_t, const char *, void *);
RL_APICALL void OpenRLLogMessagesToStderr(RLenum, const void *, size_t, const char *, void *);

#ifdef __cplusplus
}
#endif
		
#endif
