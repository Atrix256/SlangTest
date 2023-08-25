#pragma once

// This header is needed to be able to pass source code to slang from memory.
// It roughly duplicates objects that are in the slang source code.
// It'd be nice if loadModuleFromSource() could take a const char* for the source code instead, so none of this was needed!
// Or, alternately, if StringBlob was exposed in the public headers, that you get when downloading the prebuilt binaries.

/// A base class for COM interfaces that require atomic ref counting 
/// and are *NOT* derived from RefObject
class ComBaseObject
{
public:

    /// If assigned the the ref count is *NOT* copied
    ComBaseObject& operator=(const ComBaseObject&) { return *this; }

    /// Copy Ctor, does not copy ref count
    ComBaseObject(const ComBaseObject&) :
        m_refCount(0)
    {}

    /// Default Ctor sets with no refs
    ComBaseObject()
        : m_refCount(0)
    {}

    /// Dtor needs to be virtual to avoid needing to 
    /// Implement release for all derived types.
    virtual ~ComBaseObject()
    {}

protected:
    inline uint32_t _releaseImpl()
    {
        // Check there is a ref count to avoid underflow
        //SLANG_ASSERT(m_refCount != 0);
        const uint32_t count = --m_refCount;
        if (count == 0)
        {
            delete this;
        }
        return count;
    }

    std::atomic<uint32_t> m_refCount;
};

/** Base class for simple blobs.
*/
class BlobBase : public ISlangBlob, public ISlangCastable, public ComBaseObject
{
public:
    // ISlangUnknown
    SLANG_NO_THROW SlangResult SLANG_MCALL queryInterface(SlangUUID const& uuid, void** outObject) SLANG_OVERRIDE
    {
        void* intf = getInterface(uuid);
        if (intf)
        {
            ++m_refCount;
            *outObject = intf;
            return SLANG_OK;
        }
        return SLANG_E_NO_INTERFACE;
    }

    SLANG_NO_THROW uint32_t SLANG_MCALL addRef() SLANG_OVERRIDE { return ++m_refCount; }

    SLANG_NO_THROW uint32_t SLANG_MCALL release() SLANG_OVERRIDE { return _releaseImpl(); }

    // ICastable
    virtual SLANG_NO_THROW void* SLANG_MCALL castAs(const SlangUUID& guid) SLANG_OVERRIDE;

protected:
    ISlangUnknown* getInterface(const Guid& guid);
    void* getObject(const Guid& guid);
};

ISlangUnknown* BlobBase::getInterface(const Guid& guid)
{
    if (guid == ISlangUnknown::getTypeGuid() ||
        guid == ISlangBlob::getTypeGuid())
    {
        return static_cast<ISlangBlob*>(this);
    }
    if (guid == ISlangCastable::getTypeGuid())
    {
        return static_cast<ISlangCastable*>(this);
    }
    return nullptr;
}

void* BlobBase::getObject(const Guid& guid)
{
    SLANG_UNUSED(guid);
    return nullptr;
}

void* BlobBase::castAs(const SlangUUID& guid)
{
    if (auto intf = getInterface(guid))
    {
        return intf;
    }
    return getObject(guid);
}

/** A blob that uses a `StringRepresentation` for its storage.

By design the StringBlob owns a unique reference to the StringRepresentation.
This is because StringBlob, implements an interface which should work across threads.
*/
class StringBlob : public BlobBase
{
public:
    SLANG_CLASS_GUID(0xf7e0e93c, 0xde70, 0x4531, { 0x9c, 0x9f, 0xdd, 0xa3, 0xf6, 0xc6, 0xc0, 0xdd });

    // ICastable
    virtual SLANG_NO_THROW void* SLANG_MCALL castAs(const SlangUUID& guid) SLANG_OVERRIDE
    {
        if (auto intf = getInterface(guid))
        {
            return intf;
        }
        return getObject(guid);
    }

    // ISlangBlob
    SLANG_NO_THROW void const* SLANG_MCALL getBufferPointer() SLANG_OVERRIDE { return string; }
    SLANG_NO_THROW size_t SLANG_MCALL getBufferSize() SLANG_OVERRIDE { return strlen(string); }

    static ComPtr<ISlangBlob> create(const char* in)
    {
        auto blob = new StringBlob;
        blob->string = in;
        return ComPtr<ISlangBlob>(blob);
    }

protected:

    const char* string = nullptr;
};
