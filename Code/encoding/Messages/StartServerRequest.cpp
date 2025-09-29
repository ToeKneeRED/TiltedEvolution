#include "StartServerRequest.h"

void StartServerRequest::SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept
{
    Serialization::WriteString(aWriter, Password);
}

void StartServerRequest::DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept
{
    Password = Serialization::ReadString(aReader);
}
