#include "Data/OnsetSessionTokens.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOnsetSessionTokenTest, "Onset.Auth.SessionTokens",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOnsetSessionTokenTest::RunTest(const FString& Parameters)
{
    using namespace Onset::SessionTokens;
    const FMHSessionTokenConfig Config{TEXT("test-secret-for-mh-framework"), 300};
    const FString Fixture = TEXT("NzY1NjExOTgwMDAwMDAwMDB8U3RlYW18MnwxNzAwMDAwMzAw.Qcyv/PLZNvAA86kSY2Tmh+WI3as8+9WW8rugGPbLLbY=");
    const auto Issued = Issue(Config, TEXT("Steam"), TEXT("76561198000000000"), 2, 1700000000);
    if (!TestTrue(TEXT("Onset issue succeeds"), Issued.IsSuccess())) return false;
    TestEqual(TEXT("Onset wire format matches independent legacy HMAC fixture"), *Issued.GetValue(), Fixture);
    const auto Claims = Validate(Config, Fixture, 1700000300);
    if (!TestTrue(TEXT("Legacy token accepted at expiry boundary"), Claims.IsSuccess())) return false;
    TestEqual(TEXT("Onset platform mapping"), Claims.GetValue()->Platform, FString(TEXT("Steam")));
    TestEqual(TEXT("Onset account mapping"), Claims.GetValue()->PlatformID, FString(TEXT("76561198000000000")));
    TestEqual(TEXT("Onset character slot mapping"), Claims.GetValue()->SlotIndex, 2);
    TestFalse(TEXT("Expired legacy token rejected"), Validate(Config, Fixture, 1700000301).IsSuccess());
    TestFalse(TEXT("Wrong server key rejected"), Validate({TEXT("different-key"), 300}, Fixture, 1700000000).IsSuccess());
    for (int32 Slot = -1; Slot <= 2; ++Slot)
    {
        const auto Token = Issue(Config, TEXT("Steam"), TEXT("user"), Slot, 0);
        if (TestTrue(TEXT("Supported slot or unselected sentinel can be issued"), Token.IsSuccess()))
        {
            const auto RoundTrip = Validate(Config, *Token.GetValue(), 1);
            TestTrue(TEXT("Supported slot is retained"), RoundTrip.IsSuccess() && RoundTrip.GetValue()->SlotIndex == Slot);
        }
    }
    TestFalse(TEXT("Slot below sentinel cannot be issued"), Issue(Config, TEXT("Steam"), TEXT("user"), -2, 0).IsSuccess());
    TestFalse(TEXT("Fourth slot cannot be issued"), Issue(Config, TEXT("Steam"), TEXT("user"), 3, 0).IsSuccess());
    const FMHSessionTokenService Generic(Config);
    for (const TCHAR* Context : {TEXT("3"), TEXT("-2"), TEXT("00"), TEXT("+1"), TEXT("1junk"), TEXT("2147483648")})
    {
        const auto Token = Generic.Issue(TEXT("user"), TEXT("Steam"), Context, 0);
        if (TestTrue(TEXT("Generic service permits opaque context"), Token.IsSuccess()))
            TestFalse(TEXT("Valid signature does not authorize an invalid Onset slot"), Validate(Config, *Token.GetValue(), 1).IsSuccess());
    }
    TestFalse(TEXT("Missing signing key fails closed"), Issue({}, TEXT("Steam"), TEXT("user"), 0, 0).IsSuccess());
    TestFalse(TEXT("Missing verification key fails closed"), Validate({}, Fixture, 0).IsSuccess());
    TestFalse(TEXT("Delimiter injection fails closed"), Issue(Config, TEXT("Steam"), TEXT("user|other"), 0, 0).IsSuccess());
    return true;
}
#endif
