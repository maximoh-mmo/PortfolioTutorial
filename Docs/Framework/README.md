# MH framework

Reusable Unreal Engine 5.8 plugins developed inside PortfolioTutorial. Start with
[architecture](Architecture.md), [dependency rules](DependencyRules.md), and
[async conventions](AsyncConventions.md). See the [roadmap](../../Planning/Framework/Roadmap.md)
and [active checklist](../../TODO/Framework/README.md).

Current delivery: three enabled plugins, nine runtime modules, [MHCore primitives](Core.md),
a [backend session-token service](SessionTokens.md), [async HTTP](Http.md), and
[persistence contracts with memory and SQLite providers](Persistence.md), plus an optional
[PostgreSQL provider](PostgreSQL.md) and [HTTP record provider](HttpPersistence.md). Onset production consumers
have not been migrated. Onset remains the compatibility baseline.
The foundation compile gate must pass before extracting production behavior.

See [server testing](ServerTesting.md) for the cooked dedicated-server validation script.
