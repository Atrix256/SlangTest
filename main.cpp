#include "slang/slang-gfx.h"
#include "slang/slang.h"
#include <stdio.h>

using namespace Slang;
using namespace slang;

static SlangResult innerMain(int argc, char **argv)
{
	// Create the global session
	ComPtr<IGlobalSession> globalSession;
	globalSession.attach(spCreateSession(nullptr));

	// Create a compile request
	ComPtr<ICompileRequest> compileRequest;
	SLANG_RETURN_ON_FAIL(globalSession->createCompileRequest(compileRequest.writeRef()));

	// We would like to request a CPU code that can be executed directly on the host -
	// which is the 'SLANG_HOST_CALLABLE' target.
	// If we wanted a just a shared library/dll, we could have used SLANG_SHARED_LIBRARY.
	int targetIndex = compileRequest->addCodeGenTarget(SLANG_SHADER_HOST_CALLABLE);
	printf("Target index: %d\n", targetIndex);

	// Set the target flag to indicate that we want to compile all the entrypoints in the
	// slang shader file into a library.
	compileRequest->setTargetFlags(targetIndex, SLANG_TARGET_FLAG_GENERATE_WHOLE_PROGRAM);

	// A compile request can include one or more "translation units," which more or
	// less amount to individual source files (think `.c` files, not the `.h` files they
	// might include).
	//
	// For this example, our code will all be in the Slang language. The user may
	// also specify HLSL input here, but that currently doesn't affect the compiler's
	// behavior much.
	//
	int translationUnitIndex = compileRequest->addTranslationUnit(SLANG_SOURCE_LANGUAGE_SLANG, nullptr);

	// We will load source code for our translation unit from file.
	// There are also variations of this API for adding source code from application-provided buffers.
	//
	compileRequest->addTranslationUnitSourceFile(translationUnitIndex, "test.slang");

	// Once all of the input options for the compiler have been specified,
	// we can invoke `spCompile` to run the compiler and see if any errors
	// were detected.
	//
	const SlangResult compileRes = compileRequest->compile();

	// Even if there were no errors that forced compilation to fail, the
	// compiler may have produced "diagnostic" output such as warnings.
	// We will go ahead and print that output here.
	//
	if (auto diagnostics = compileRequest->getDiagnosticOutput())
	{
		printf("%s", diagnostics);
	}

	if (SLANG_FAILED(compileRes))
	{
		printf("Compilation failed!\n");
		return compileRes;
	}

	// Get the 'shared library' (note that this doesn't necessarily have to be implemented as a shared library
	// it's just an interface to executable code).
	ComPtr<ISlangSharedLibrary> sharedLibrary;
	const SlangResult sharedLibraryResult = compileRequest->getTargetHostCallable(targetIndex, sharedLibrary.writeRef());
	if (SLANG_FAILED(sharedLibraryResult))
	{
		printf("Failed to get shared library\n");
		return sharedLibraryResult;
	}

	// Get the code out
	ComPtr<ISlangBlob> kernelBlob;
	const SlangResult kernelBlobResult = compileRequest->getTargetCodeBlob(targetIndex, kernelBlob.writeRef());
	if (SLANG_FAILED(kernelBlobResult))
	{
		printf("Failed to get kernel blob\n");
		return kernelBlobResult;
	}
	printf("Code:\n%s\n", (const char *)kernelBlob->getBufferPointer());

	return SLANG_OK;
}

int main(int argc, char **argv)
{
	return SLANG_SUCCEEDED(innerMain(argc, argv)) ? 0 : -1;
}
