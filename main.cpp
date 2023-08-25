// The slang folder is a slang release downloaded from https://github.com/shader-slang/slang/releases
// API user guide: https://github.com/shader-slang/slang/blob/master/docs/api-users-guide.md

#include <stdio.h>
#include <vector>

#include "slang/slang.h"

static const char*              c_fileNameSource        = "test.slang";
static const char*              c_fileNameOut           = "out_compiled.hlsl";
static const char*              c_fileNameReflection    = "out_reflection.txt";
static const char*              c_entryPointName        = "csmain";
static const SlangStage         c_stage                 = SlangStage::SLANG_STAGE_COMPUTE;
static const SlangCompileTarget c_compileTarget         = SlangCompileTarget::SLANG_HLSL;
static const char*              c_compileProfile        = "cs_5_1";
static const bool               c_loadFromMemory        = false;

int main(int argc, char** argv)
{
    int ret = 0;

    // Create a session and request
    SlangSession* session = spCreateSession(NULL);
    SlangCompileRequest* request = spCreateCompileRequest(session);

    // Set what type of thing we want to come out of the slang compiler
    spSetCodeGenTarget(request, c_compileTarget);

    //spAddSearchPath(request, "some/path/");

    int translationUnitIndex = spAddTranslationUnit(request, SLANG_SOURCE_LANGUAGE_SLANG, "");

    // read the file in and add it as source code
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
        spAddTranslationUnitSourceString(request, translationUnitIndex, c_fileNameSource, source.data());
    }
    else
    {
        spAddTranslationUnitSourceFile(request, translationUnitIndex, c_fileNameSource);
    }

    spSetTargetProfile(request, 0, spFindProfile(session, c_compileProfile));

    // Add an entry point
    int entryPointIndex = spAddEntryPoint(
        request,
        translationUnitIndex,
        c_entryPointName,
        c_stage);

    int anyErrors = spCompile(request);

    if (anyErrors != 0)
    {
        printf("spCompile: ERROR %i\n", anyErrors);
        ret = 1;
    }
    else
        printf("spCompile: OK!\n");

    // Output diagnostics if there were problems
    char const* diagnostics = spGetDiagnosticOutput(request);
    if (diagnostics && diagnostics[0])
        printf("diagnostics:\n%s\n", diagnostics);

    // write the compiled output
    {
        FILE* file = nullptr;
        fopen_s(&file, c_fileNameOut, "wb");
        if (!file)
        {
            printf("Could not open %s for writing.\n", c_fileNameOut);
            ret = 1;
        }
        else
        {
            size_t dataSize = 0;
            void const* data = spGetEntryPointCode(request, entryPointIndex, &dataSize);
            fwrite(data, 1, dataSize, file);
            fclose(file);
        }
    }

    // write out reflection information
    {
        slang::ShaderReflection* shaderReflection = slang::ShaderReflection::get(request);

        FILE* file = nullptr;
        fopen_s(&file, c_fileNameReflection, "wb");
        if (!file)
        {
            printf("Could not open %s for writing.\n", c_fileNameReflection);
            ret = 1;
        }
        else
        {
            // Entry Points
            fprintf(file, "Entry Points:\n");
            SlangUInt entryPointCount = shaderReflection->getEntryPointCount();
            for (SlangUInt ee = 0; ee < entryPointCount; ee++)
            {
                slang::EntryPointReflection* entryPoint = shaderReflection->getEntryPointByIndex(ee);
                fprintf(file, "    %s\n", entryPoint->getName());
            }

            // Parameters
            fprintf(file, "\nParameters:\n");
            unsigned parameterCount = shaderReflection->getParameterCount();
            for (unsigned pp = 0; pp < parameterCount; pp++)
            {
                slang::VariableLayoutReflection* parameter =shaderReflection->getParameterByIndex(pp);
                fprintf(file, "    %s\n", parameter->getName());
            }

            fprintf(file, "\nNote: Other reflection information is available from the shaderReflection object!\n");

            fclose(file);
        }
    }

    // Clean up
    spDestroyCompileRequest(request);
    spDestroySession(session);

    return ret;
}
