#include <stdio.h>
#include "slang/slang-gfx.h"

using namespace Slang;
using namespace slang;

int main(int argc, char** argv)
{
	// Create the global session
	ComPtr<IGlobalSession> globalSession;
	createGlobalSession(globalSession.writeRef());

	// Create the session
	ComPtr<ISession> session;
	TargetDesc targetDesc;
	targetDesc.format = SlangCompileTarget::SLANG_HLSL;
	targetDesc.profile = globalSession->findProfile("cs_5_1");

	SessionDesc sessionDesc;
	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;

	globalSession->createSession(sessionDesc, session.writeRef());

	// Load the module
	ComPtr<IBlob> diagnostics;
	ComPtr<IModule> module(session->loadModule("test", diagnostics.writeRef()));
	if (diagnostics)
	{
		printf("Error 1:\n%s\n", (const char*)diagnostics->getBufferPointer());
		return 1;
	}

	// Make Composite
	ComPtr<IComponentType> program;
	IComponentType* components[] = { module };
	session->createCompositeComponentType(components, 1, program.writeRef());

	// Get the compiled code
	ComPtr<IBlob> kernelBlob;
	program->getEntryPointCode(
		0,
		0,
		kernelBlob.writeRef(),
		diagnostics.writeRef()
	);
	if (diagnostics)
	{
		printf("Error 2:\n%s\n", (const char*)diagnostics->getBufferPointer());
		return 2;
	}

	// get the code out
	printf("Code:\n%s\n", (const char*)kernelBlob->getBufferPointer());

	return 0;
}