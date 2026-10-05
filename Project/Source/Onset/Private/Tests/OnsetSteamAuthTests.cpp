#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Subsystem/OnsetSteamAuthResultCache.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOnsetSteamAuthEarlyResultTest, "Onset.Auth.SteamEarlyResult",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FOnsetSteamAuthEarlyResultTest::RunTest(const FString& Parameters)
{
	FOnsetSteamAuthResultCache Results;
	bool bSuccess = false;
	Results.Record(TEXT("player-a"), true);
	TestFalse(TEXT("An early callback must not authenticate a different user"), Results.Consume(TEXT("player-b"), bSuccess));
	TestTrue(TEXT("PostLogin can consume an earlier successful callback"), Results.Consume(TEXT("player-a"), bSuccess));
	TestTrue(TEXT("Successful validation is retained"), bSuccess);
	TestFalse(TEXT("The result is consumed exactly once"), Results.Consume(TEXT("player-a"), bSuccess));

	Results.Record(TEXT("player-a"), false);
	bSuccess = true;
	TestTrue(TEXT("Early rejection is retained until PostLogin"), Results.Consume(TEXT("player-a"), bSuccess));
	TestFalse(TEXT("A failed validation cannot authorize account work"), bSuccess);

	Results.Record(TEXT("player-a"), true);
	Results.Record(TEXT("player-a"), false);
	TestTrue(TEXT("A revocation supersedes earlier success"), Results.Consume(TEXT("player-a"), bSuccess));
	TestFalse(TEXT("The latest failure remains a rejection"), bSuccess);

	Results.Record(TEXT("player-a"), true);
	Results.Forget(TEXT("player-a"));
	TestFalse(TEXT("Logout removes results before reconnect"), Results.Consume(TEXT("player-a"), bSuccess));
	Results.Record(TEXT("player-a"), true);
	Results.Record(TEXT("player-b"), true);
	Results.Reset();
	TestFalse(TEXT("World teardown clears player-a's result"), Results.Consume(TEXT("player-a"), bSuccess));
	TestFalse(TEXT("World teardown clears player-b's result"), Results.Consume(TEXT("player-b"), bSuccess));
	return true;
}

#endif
