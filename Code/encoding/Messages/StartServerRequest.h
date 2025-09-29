#pragma once
#include "Message.h"

struct StartServerRequest final : ClientMessage
{
    static constexpr ClientOpcode Opcode = kStartServerRequest;

    void SerializeRaw(TiltedPhoques::Buffer::Writer& aWriter) const noexcept override;
    void DeserializeRaw(TiltedPhoques::Buffer::Reader& aReader) noexcept override;

    bool operator==(const StartServerRequest& acRhs) const noexcept { return GetOpcode() == acRhs.GetOpcode() && Password == acRhs.Password; }

    String Password;
};
