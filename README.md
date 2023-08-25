# SlangTest

Slang is essentially a superset of HLSL which adds some very nice features.

It can output plain hlsl which flattens all those nice features into standard shader code.
It can also output other types of shader languages, as well as compiled shader binaries.

This code is the example that I wish existed, which was a bit painful to put together from the sparse documentation.

StringBlob.h is here because of a quirk in the API that processes slang source from memory. It wants an object type that is not exposed in the public headers.
Maybe a version could be added that takes a const char* instead.  That would be nice!

Slang is at: 
https://github.com/shader-slang/slang

Slang in this repo is a release, downloaded from:
https://github.com/shader-slang/slang/releases
