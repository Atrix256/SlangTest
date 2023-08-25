#include <stdio.h>
#include "slang/slang-gfx.h"
#include <vector>

using namespace Slang;
using namespace slang;

static const char*              c_fileNameSource        = "test.slang";
static const char*              c_fileNameOut           = "out_compiled.hlsl";
static const char*              c_fileNameReflection    = "out_reflection.txt";
static const char*              c_entryPointName        = "csmain";
static const SlangCompileTarget c_compileTarget         = SlangCompileTarget::SLANG_HLSL;
static const char*              c_compileProfile        = "cs_5_1";

// Needed to store the source code in
class Blob : public ISlangBlob
{
public:
    Blob(void* data, size_t size)
        : m_data(data)
        , m_size(size)
    {
    }

    virtual SLANG_NO_THROW void const* SLANG_MCALL getBufferPointer()
    {
        return m_data;
    }

    virtual SLANG_NO_THROW size_t SLANG_MCALL getBufferSize()
    {
        return m_size;
    }

    virtual SLANG_NO_THROW uint32_t SLANG_MCALL addRef()
    {
        return 1;
    }

    virtual SLANG_NO_THROW uint32_t SLANG_MCALL release()
    {
        return 1;
    }

    SlangResult queryInterface(SlangUUID const& guid, void** outObject)
    {
        if (auto intf = getInterface(guid))
        {
            *outObject = intf;
            return SLANG_OK;
        }
        return SLANG_E_NO_INTERFACE;
    }

    void* castAs(const SlangUUID& guid)
    {
        if (auto intf = getInterface(guid))
        {
            return intf;
        }
        return getObject(guid);
    }

    ISlangUnknown* getInterface(const Guid& guid)
    {
        return static_cast<ISlangBlob*>(this);
    }

    void* getObject(const Guid& guid)
    {
        SLANG_UNUSED(guid);
        return nullptr;
    }

protected:
    void* m_data = nullptr;
    size_t m_size = 0;
};

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

    // The first block has slang load the source from disk.
    // The second block (in the else) has slang load the source from memory, but is crashing for some reason.
#if 1
    IModule* module = session->loadModule(c_fileNameSource, diagnosticsBlob.writeRef());
#else
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
    Blob sourceBlob(source.data(), source.size() - 1);
    ComPtr<Blob> sourceBlobPtr(&sourceBlob);

    IModule* module = session->loadModuleFromSource(c_fileNameSource, c_fileNameSource, sourceBlobPtr, diagnosticsBlob.writeRef());
#endif

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

        fprintf(file, "\nNote: Other reflection information is available!\n");

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
