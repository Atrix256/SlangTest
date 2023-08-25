#include <stdio.h>
#include <vector>

#include "slang/slang-gfx.h"

using namespace Slang;
using namespace slang;

#include "StringBlob.h"

static const char*              c_fileNameSource        = "test.slang";
static const char*              c_fileNameOut           = "out_compiled.hlsl";
static const char*              c_fileNameReflection    = "out_reflection.txt";
static const char*              c_entryPointName        = "csmain";
static const SlangCompileTarget c_compileTarget         = SlangCompileTarget::SLANG_HLSL;
static const char*              c_compileProfile        = "cs_5_1";
static const bool               c_loadFromMemory        = true;

int main(int argc, char** argv)
{
	// Create the global session. This is costly so should only be done once per application launch if possible.
	ComPtr<IGlobalSession> globalSession;
	createGlobalSession(globalSession.writeRef());

	// Create the session
	ComPtr<ISession> session;
	TargetDesc targetDesc;
	targetDesc.format = c_compileTarget;
	targetDesc.profile = globalSession->findProfile(c_compileProfile);
	SessionDesc sessionDesc;
	sessionDesc.targets = &targetDesc;
	sessionDesc.targetCount = 1;
	globalSession->createSession(sessionDesc, session.writeRef());

    ComPtr<IBlob> diagnosticsBlob;

    IModule* module = nullptr;
    if (c_loadFromMemory)
    {
        std::vector<char> source;
        {
            FILE* file = nullptr;
            fopen_s(&file, c_fileNameSource, "rb");
            fseek(file, 0, SEEK_END);
            source.resize(ftell(file) + 1, 0); // an extra byte for a null terminator
            fseek(file, 0, SEEK_SET);
            fread(source.data(), 1, source.size() - 1, file);
            fclose(file);
        }
        ComPtr<IBlob> sourceBlob = StringBlob::create(source.data());

        module = session->loadModuleFromSource(c_fileNameSource, c_fileNameSource, sourceBlob, diagnosticsBlob.writeRef());
    }
    else
    {
        module = session->loadModule(c_fileNameSource, diagnosticsBlob.writeRef());
    }

    if (diagnosticsBlob)
        printf("%s", (const char*)diagnosticsBlob->getBufferPointer());
    if (!module)
    {
        printf("Could not load module %s\n", c_fileNameSource);
        return 1;
    }

    // Get the entry point
    ComPtr<IEntryPoint> computeEntryPoint;
    SLANG_RETURN_ON_FAIL(
        module->findEntryPointByName(c_entryPointName, computeEntryPoint.writeRef()));

    // make a composed program
    IComponentType* componentTypes[] = { module , computeEntryPoint };
    ComPtr<IComponentType> composedProgram;
    SlangResult result = session->createCompositeComponentType(
        componentTypes,
        2,
        composedProgram.writeRef(),
        diagnosticsBlob.writeRef());
    if (diagnosticsBlob)
        printf("%s", (const char*)diagnosticsBlob->getBufferPointer());
    if (SLANG_FAILED(result))
    {
        printf("Could not compose program for filename %s entry point %s\n", c_fileNameSource, c_entryPointName);
        return 2;
    }

    // Get the reflection information
    ProgramLayout* slangReflection = composedProgram->getLayout();

    // write the reflection information
    {
        FILE* file = nullptr;
        fopen_s(&file, c_fileNameReflection, "wb");
        if (!file)
        {
            printf("Could not open %s for writing.\n", c_fileNameReflection);
            return 3;
        }

        // Entry Points
        SlangUInt entryPointCount = slangReflection->getEntryPointCount();
        fprintf(file, "%i Entry Points:\n", (int)entryPointCount);
        for (SlangUInt index = 0; index < entryPointCount; ++index)
        {
            EntryPointReflection* refl = slangReflection->getEntryPointByIndex(index);
            fprintf(file, "  %s\n", refl->getName());
        }

        fprintf(file, "\nNote: Other reflection information is available from slangReflection object!\n");

        fclose(file);
    }

    // Get the compiled output
    ComPtr<IBlob> kernelBlob;
    composedProgram->getEntryPointCode(
        0, // Entry Point Index.  Could process multiple entry points at once.
        0, // Target Index.  Could process multiple targets at once.
        kernelBlob.writeRef(),
        diagnosticsBlob.writeRef()
    );
    if (diagnosticsBlob)
        printf("%s", (const char*)diagnosticsBlob->getBufferPointer());

    // write the compiled output
    {
        FILE* file = nullptr;
        fopen_s(&file, c_fileNameOut, "wb");
        if (!file)
        {
            printf("Could not open %s for writing.\n", c_fileNameOut);
            return 4;
        }

        fwrite(kernelBlob->getBufferPointer(), 1, kernelBlob->getBufferSize(), file);
        fclose(file);
    }

	return 0;
}
