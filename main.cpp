#include <stdio.h>
#include "slang/slang-gfx.h"

using namespace Slang;
using namespace slang;

static const char*              c_fileName          = "test.slang";
static const char*              c_entryPointName    = "csmain";
static const SlangCompileTarget c_compileTarget     = SlangCompileTarget::SLANG_HLSL;
static const char*              c_compileProfile    = "cs_5_1";

void diagnoseIfNeeded(slang::IBlob* diagnosticsBlob)
{
    if (diagnosticsBlob != nullptr)
        printf("%s", (const char*)diagnosticsBlob->getBufferPointer());
}

Result loadShaderProgram(
    ComPtr<slang::ISession>& slangSession,
    slang::ProgramLayout*& slangReflection)
{
    // We need to obtain a compilation session (`slang::ISession`) that will provide
    // a scope to all the compilation and loading of code we do.
    //
    // Our example application uses the `gfx` graphics API abstraction layer, which already
    // creates a Slang compilation session for us, so we just grab and use it here.
    //ComPtr<slang::ISession> slangSession;
    //SLANG_RETURN_ON_FAIL(device->getSlangSession(slangSession.writeRef()));

    // Once the session has been obtained, we can start loading code into it.
    //
    // The simplest way to load code is by calling `loadModule` with the name of a Slang
    // module. A call to `loadModule("MyStuff")` will behave more or less as if you
    // wrote:
    //
    //      import MyStuff;
    //
    // In a Slang shader file. The compiler will use its search paths to try to locate
    // `MyModule.slang`, then compile and load that file. If a matching module had
    // already been loaded previously, that would be used directly.
    //
    // Note: The only interesting wrinkle here is that our file is named `shader-object` with
    // a hyphen in it, so the name is not directly usable as an identifier in Slang code.
    // Instead, when trying to import this module in the context of Slang code, a user
    // needs to replace the hyphens with underscores:
    //
    //      import shader_object;
    //
    ComPtr<slang::IBlob> diagnosticsBlob;
    slang::IModule* module = slangSession->loadModule(c_fileName, diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);
    if (!module)
        return SLANG_FAIL;

    // Loading the `shader-object` module will compile and check all the shader code in it,
    // including the shader entry points we want to use. Now that the module is loaded
    // we can look up those entry points by name.
    //
    // Note: If you are using this `loadModule` approach to load your shader code it is
    // important to tag your entry point functions with the `[shader("...")]` attribute
    // (e.g., `[shader("vertex")] void vertexMain(...)`). Without that information there
    // is no umambiguous way for the compiler to know which functions represent entry
    // points when it parses your code via `loadModule()`.
    //
    ComPtr<slang::IEntryPoint> computeEntryPoint;
    SLANG_RETURN_ON_FAIL(
        module->findEntryPointByName(c_entryPointName, computeEntryPoint.writeRef()));

    // At this point we have a few different Slang API objects that represent
    // pieces of our code: `module`, `vertexEntryPoint`, and `fragmentEntryPoint`.   
    //
    // A single Slang module could contain many different entry points (e.g.,
    // four vertex entry points, three fragment entry points, and two compute
    // shaders), and before we try to generate output code for our target API
    // we need to identify which entry points we plan to use together.
    //
    // Modules and entry points are both examples of *component types* in the
    // Slang API. The API also provides a way to build a *composite* out of
    // other pieces, and that is what we are going to do with our module
    // and entry points.
    //
    slang::IComponentType* componentTypes[] = { module , computeEntryPoint };
    //Slang::List<slang::IComponentType*> componentTypes;
    //componentTypes.add(module);
    //componentTypes.add(computeEntryPoint);

    // Actually creating the composite component type is a single operation
    // on the Slang session, but the operation could potentially fail if
    // something about the composite was invalid (e.g., you are trying to
    // combine multiple copies of the same module), so we need to deal
    // with the possibility of diagnostic output.
    //
    ComPtr<slang::IComponentType> composedProgram;
    SlangResult result = slangSession->createCompositeComponentType(
        componentTypes,
        2,
        composedProgram.writeRef(),
        diagnosticsBlob.writeRef());
    diagnoseIfNeeded(diagnosticsBlob);
    SLANG_RETURN_ON_FAIL(result);
    slangReflection = composedProgram->getLayout();

    // At this point, `composedProgram` represents the shader program
    // we want to run, and the compute shader there have been checked.
    // We can create a `gfx::IShaderProgram` object from `composedProgram`
    // so it may be used by the graphics layer.
    gfx::IShaderProgram::Desc programDesc = {};
    programDesc.slangGlobalScope = composedProgram.get();

    //auto shaderProgram = device->createProgram(programDesc);
    //outShaderProgram = shaderProgram;

    ComPtr<IBlob> kernelBlob;
    composedProgram->getEntryPointCode(
        0,
        0,
        kernelBlob.writeRef(),
        diagnosticsBlob.writeRef()
    );
    diagnoseIfNeeded(diagnosticsBlob);

    const char* source = (const char*)kernelBlob->getBufferPointer();

    return SLANG_OK;
}

int main(int argc, char** argv)
{
	// Create the global session
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

    slang::ProgramLayout* programLayout = nullptr;
    auto r = loadShaderProgram(session, programLayout);

    


	return 0;
}

// TODO: load this from memory before calling it G2G
// TODO: write out a file for the output, and another for the reflection?