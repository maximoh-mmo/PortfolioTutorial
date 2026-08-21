#include "FPgSQLStore.h"
#include "OnsetDataStoreModule.h"

#ifndef ONSETDATASTORE_CLIENT_ONLY

#include "libpq-fe.h"

static void LogPGError(struct pg_conn* InConn, const char* Context)
{
	UE_LOG(LogOnsetDataStore, Error, TEXT("PostgreSQL %s: %hs"), ANSI_TO_TCHAR(Context), PQerrorMessage(InConn));
}

FPgSQLStore::~FPgSQLStore()
{
	SaveAll();
	if (Conn)
	{
		PQfinish(Conn);
		Conn = nullptr;
	}
}

bool FPgSQLStore::Exec(const char* SQL)
{
	PGresult* Res = PQexec(Conn, SQL);
	if (PQresultStatus(Res) != PGRES_COMMAND_OK && PQresultStatus(Res) != PGRES_TUPLES_OK)
	{
		LogPGError(Conn, SQL);
		PQclear(Res);
		return false;
	}
	PQclear(Res);
	return true;
}

bool FPgSQLStore::Initialize(const FString& ConnectionString)
{
	ConnStr = ConnectionString.IsEmpty()
		? TEXT("host=localhost dbname=onset user=onset password=onset")
		: ConnectionString;

	Conn = PQconnectdb(TCHAR_TO_UTF8(*ConnStr));
	if (PQstatus(Conn) != CONNECTION_OK)
	{
		LogPGError(Conn, "PQconnectdb");
		PQfinish(Conn);
		Conn = nullptr;
		return false;
	}

	if (!EnsureSchema())
	{
		UE_LOG(LogOnsetDataStore, Error, TEXT("FPgSQLStore: schema migration failed"));
		PQfinish(Conn);
		Conn = nullptr;
		return false;
	}

	UE_LOG(LogOnsetDataStore, Log, TEXT("FPgSQLStore: connected to PostgreSQL"));
	return true;
}

bool FPgSQLStore::EnsureSchema()
{
	if (!Exec(
		"CREATE TABLE IF NOT EXISTS _schema_version ("
		"  version INTEGER PRIMARY KEY,"
		"  applied_at TIMESTAMP NOT NULL DEFAULT NOW()"
		");"))
		return false;

	int32 Version = GetSchemaVersion();
	if (Version < 0)
		return false;

	const int32 LatestVersion = 5;
	while (Version < LatestVersion)
	{
		RunMigration(Version);
		Version = GetSchemaVersion();
	}
	return true;
}

int32 FPgSQLStore::GetSchemaVersion()
{
	PGresult* Res = PQexec(Conn, "SELECT COALESCE(MAX(version), 0) FROM _schema_version;");
	if (PQresultStatus(Res) != PGRES_TUPLES_OK)
	{
		LogPGError(Conn, "GetSchemaVersion");
		PQclear(Res);
		return -1;
	}

	int32 Result = -1;
	if (PQntuples(Res) > 0)
		Result = atoi(PQgetvalue(Res, 0, 0));

	PQclear(Res);
	return Result;
}

void FPgSQLStore::RunMigration(int32 FromVersion)
{
	if (FromVersion < 0) return;

	if (FromVersion == 0)
	{
		if (!Exec(
			"CREATE TABLE IF NOT EXISTS accounts ("
			"  platform TEXT NOT NULL,"
			"  platform_id TEXT NOT NULL,"
			"  created_at TIMESTAMP NOT NULL DEFAULT NOW(),"
			"  last_login TIMESTAMP NOT NULL DEFAULT NOW(),"
			"  PRIMARY KEY (platform, platform_id)"
			");"))
		{
			UE_LOG(LogOnsetDataStore, Error, TEXT("FPgSQLStore: migration 1 failed (accounts table)"));
			return;
		}

		if (!Exec(
			"CREATE TABLE IF NOT EXISTS characters ("
			"  platform TEXT NOT NULL,"
			"  platform_id TEXT NOT NULL,"
			"  slot_index INTEGER NOT NULL CHECK(slot_index >= 0 AND slot_index <= 2),"
			"  character_name TEXT NOT NULL DEFAULT '',"
			"  level INTEGER NOT NULL DEFAULT 1,"
			"  experience INTEGER NOT NULL DEFAULT 0,"
			"  saved_max_health REAL NOT NULL DEFAULT 100.0,"
			"  saved_position_x REAL NOT NULL DEFAULT 0.0,"
			"  saved_position_y REAL NOT NULL DEFAULT 0.0,"
			"  saved_position_z REAL NOT NULL DEFAULT 0.0,"
			"  saved_rotation_yaw REAL NOT NULL DEFAULT 0.0,"
			"  inventory_json TEXT NOT NULL DEFAULT '{}',"
			"  equipment_json TEXT NOT NULL DEFAULT '{}',"
			"  quests_json TEXT NOT NULL DEFAULT '{}',"
			"  created_at TIMESTAMP NOT NULL DEFAULT NOW(),"
			"  updated_at TIMESTAMP NOT NULL DEFAULT NOW(),"
			"  PRIMARY KEY (platform, platform_id, slot_index),"
			"  FOREIGN KEY (platform, platform_id) REFERENCES accounts(platform, platform_id) ON DELETE CASCADE"
			");"))
		{
			UE_LOG(LogOnsetDataStore, Error, TEXT("FPgSQLStore: migration 1 failed (characters table)"));
			return;
		}

		Exec("INSERT INTO _schema_version (version) VALUES (1);");
		UE_LOG(LogOnsetDataStore, Log, TEXT("FPgSQLStore: migration 1 applied (accounts + characters)"));
	}

	if (FromVersion <= 1)
	{
		if (!Exec("ALTER TABLE characters ADD COLUMN current_zone TEXT NOT NULL DEFAULT '';"))
		{
			UE_LOG(LogOnsetDataStore, Error, TEXT("FPgSQLStore: migration 2 failed (current_zone column)"));
			return;
		}

		Exec("INSERT INTO _schema_version (version) VALUES (2);");
		UE_LOG(LogOnsetDataStore, Log, TEXT("FPgSQLStore: migration 2 applied (current_zone column)"));
	}

	if (FromVersion <= 2)
	{
		if (!Exec("ALTER TABLE characters ADD COLUMN character_class INTEGER NOT NULL DEFAULT 0;"))
		{
			UE_LOG(LogOnsetDataStore, Error, TEXT("FPgSQLStore: migration 3 failed (character_class column)"));
			return;
		}

		if (!Exec("ALTER TABLE characters ADD COLUMN appearance_json TEXT NOT NULL DEFAULT '{}';"))
		{
			UE_LOG(LogOnsetDataStore, Error, TEXT("FPgSQLStore: migration 3 failed (appearance_json column)"));
			return;
		}

		Exec("INSERT INTO _schema_version (version) VALUES (3);");
		UE_LOG(LogOnsetDataStore, Log, TEXT("FPgSQLStore: migration 3 applied (character_class + appearance_json)"));
	}

	if (FromVersion <= 3)
	{
		if (!Exec("ALTER TABLE characters ADD COLUMN unspent_stat_points INTEGER NOT NULL DEFAULT 0;"))
		{
			UE_LOG(LogOnsetDataStore, Error, TEXT("FPgSQLStore: migration 4 failed (unspent_stat_points column)"));
			return;
		}

		Exec("INSERT INTO _schema_version (version) VALUES (4);");
		UE_LOG(LogOnsetDataStore, Log, TEXT("FPgSQLStore: migration 4 applied (unspent_stat_points column)"));
	}

	if (FromVersion <= 4)
	{
		if (!Exec("ALTER TABLE characters ADD COLUMN prestige_level INTEGER NOT NULL DEFAULT 0;"))
		{
			UE_LOG(LogOnsetDataStore, Error, TEXT("FPgSQLStore: migration 5 failed (prestige_level column)"));
			return;
		}

		Exec("INSERT INTO _schema_version (version) VALUES (5);");
		UE_LOG(LogOnsetDataStore, Log, TEXT("FPgSQLStore: migration 5 applied (prestige_level column)"));
	}
}

bool FPgSQLStore::LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount)
{
	if (!Conn) return false;

	const char* SQL = "SELECT platform, platform_id FROM accounts WHERE platform = $1 AND platform_id = $2;";
	const char* Params[2] = { TCHAR_TO_UTF8(*Platform), TCHAR_TO_UTF8(*PlatformID) };
	PGresult* Res = PQexecParams(Conn, SQL, 2, NULL, Params, NULL, NULL, 0);

	if (PQresultStatus(Res) != PGRES_TUPLES_OK)
	{
		LogPGError(Conn, "LoadAccount");
		PQclear(Res);
		return false;
	}

	if (PQntuples(Res) == 0)
	{
		PQclear(Res);
		return false;
	}

	OutAccount.Platform = UTF8_TO_TCHAR(PQgetvalue(Res, 0, 0));
	OutAccount.PlatformID = UTF8_TO_TCHAR(PQgetvalue(Res, 0, 1));
	PQclear(Res);

	const char* SlotSQL = "SELECT slot_index, character_name, level, character_class FROM characters WHERE platform = $1 AND platform_id = $2 ORDER BY slot_index;";
	Res = PQexecParams(Conn, SlotSQL, 2, NULL, Params, NULL, NULL, 0);

	if (PQresultStatus(Res) != PGRES_TUPLES_OK)
	{
		LogPGError(Conn, "LoadAccount (slots)");
		PQclear(Res);
		return false;
	}

	OutAccount.Slots.Empty();
	int32 ExpectedSlot = 0;
	int32 NumRows = PQntuples(Res);
	for (int32 Row = 0; Row < NumRows; Row++)
	{
		int32 SlotIdx = atoi(PQgetvalue(Res, Row, 0));
		while (ExpectedSlot < SlotIdx)
		{
			FOnsetCharacterSlotData& EmptySlot = OutAccount.Slots.AddDefaulted_GetRef();
			EmptySlot.SlotIndex = ExpectedSlot;
			EmptySlot.bOccupied = false;
			ExpectedSlot++;
		}

		FOnsetCharacterSlotData& Slot = OutAccount.Slots.AddDefaulted_GetRef();
		Slot.SlotIndex = SlotIdx;
		Slot.CharacterName = UTF8_TO_TCHAR(PQgetvalue(Res, Row, 1));
		Slot.Level = atoi(PQgetvalue(Res, Row, 2));
		Slot.CharacterClass = static_cast<EOnsetCharacterClass>(atoi(PQgetvalue(Res, Row, 3)));
		Slot.bOccupied = true;
		ExpectedSlot = SlotIdx + 1;
	}

	while (ExpectedSlot < 3)
	{
		FOnsetCharacterSlotData& EmptySlot = OutAccount.Slots.AddDefaulted_GetRef();
		EmptySlot.SlotIndex = ExpectedSlot;
		EmptySlot.bOccupied = false;
		ExpectedSlot++;
	}

	PQclear(Res);
	return true;
}

bool FPgSQLStore::CreateAccount(const FString& Platform, const FString& PlatformID)
{
	if (!Conn) return false;

	const char* SQL = "INSERT INTO accounts (platform, platform_id) VALUES ($1, $2) ON CONFLICT DO NOTHING;";
	const char* Params[2] = { TCHAR_TO_UTF8(*Platform), TCHAR_TO_UTF8(*PlatformID) };
	PGresult* Res = PQexecParams(Conn, SQL, 2, NULL, Params, NULL, NULL, 0);

	bool bSuccess = (PQresultStatus(Res) == PGRES_COMMAND_OK);
	if (!bSuccess)
		LogPGError(Conn, "CreateAccount");

	PQclear(Res);
	return bSuccess;
}

bool FPgSQLStore::LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData)
{
	if (!Conn) return false;

	const char* SQL = "SELECT slot_index, character_name, level, experience,"
		" saved_max_health, saved_position_x, saved_position_y, saved_position_z, saved_rotation_yaw,"
		" inventory_json, equipment_json, quests_json, current_zone, character_class, appearance_json, unspent_stat_points, prestige_level"
		" FROM characters WHERE platform = $1 AND platform_id = $2 AND slot_index = $3;";

	const char* Params[3] = { TCHAR_TO_UTF8(*Platform), TCHAR_TO_UTF8(*PlatformID), TCHAR_TO_UTF8(*FString::Printf(TEXT("%d"), SlotIndex)) };
	PGresult* Res = PQexecParams(Conn, SQL, 3, NULL, Params, NULL, NULL, 0);

	if (PQresultStatus(Res) != PGRES_TUPLES_OK)
	{
		LogPGError(Conn, "LoadCharacter");
		PQclear(Res);
		return false;
	}

	if (PQntuples(Res) == 0)
	{
		PQclear(Res);
		return false;
	}

	OutData.SlotIndex = atoi(PQgetvalue(Res, 0, 0));
	OutData.CharacterName = UTF8_TO_TCHAR(PQgetvalue(Res, 0, 1));
	OutData.Level = atoi(PQgetvalue(Res, 0, 2));
	OutData.Experience = atoi(PQgetvalue(Res, 0, 3));
	OutData.SavedMaxHealth = static_cast<float>(atof(PQgetvalue(Res, 0, 4)));
	OutData.SavedPosition.X = static_cast<float>(atof(PQgetvalue(Res, 0, 5)));
	OutData.SavedPosition.Y = static_cast<float>(atof(PQgetvalue(Res, 0, 6)));
	OutData.SavedPosition.Z = static_cast<float>(atof(PQgetvalue(Res, 0, 7)));
	OutData.SavedRotationYaw = static_cast<float>(atof(PQgetvalue(Res, 0, 8)));
	OutData.InventoryJSON = UTF8_TO_TCHAR(PQgetvalue(Res, 0, 9));
	OutData.EquipmentJSON = UTF8_TO_TCHAR(PQgetvalue(Res, 0, 10));
	OutData.QuestsJSON = UTF8_TO_TCHAR(PQgetvalue(Res, 0, 11));
	OutData.CurrentZone = UTF8_TO_TCHAR(PQgetvalue(Res, 0, 12));
	OutData.CharacterClass = static_cast<EOnsetCharacterClass>(atoi(PQgetvalue(Res, 0, 13)));
	OutData.AppearanceJSON = UTF8_TO_TCHAR(PQgetvalue(Res, 0, 14));
	OutData.UnspentStatPoints = atoi(PQgetvalue(Res, 0, 15));
	OutData.PrestigeLevel = atoi(PQgetvalue(Res, 0, 16));

	PQclear(Res);
	return true;
}

bool FPgSQLStore::SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data)
{
	if (!Conn) return false;

	FString SlotIdxStr = FString::Printf(TEXT("%d"), Data.SlotIndex);
	FString LevelStr = FString::Printf(TEXT("%d"), Data.Level);
	FString ExpStr = FString::Printf(TEXT("%d"), Data.Experience);
	FString HealthStr = FString::SanitizeFloat(Data.SavedMaxHealth);
	FString PosXStr = FString::SanitizeFloat(Data.SavedPosition.X);
	FString PosYStr = FString::SanitizeFloat(Data.SavedPosition.Y);
	FString PosZStr = FString::SanitizeFloat(Data.SavedPosition.Z);
	FString YawStr = FString::SanitizeFloat(Data.SavedRotationYaw);

	FString ClassStr = FString::Printf(TEXT("%d"), static_cast<int32>(Data.CharacterClass));
	FString StatPointsStr = FString::Printf(TEXT("%d"), Data.UnspentStatPoints);
	FString PrestigeStr = FString::Printf(TEXT("%d"), Data.PrestigeLevel);

	const char* SQL =
		"INSERT INTO characters"
		" (platform, platform_id, slot_index, character_name, level, experience,"
		"  saved_max_health, saved_position_x, saved_position_y, saved_position_z, saved_rotation_yaw,"
		"  inventory_json, equipment_json, quests_json, current_zone, character_class, appearance_json, unspent_stat_points, prestige_level, updated_at)"
		" VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, NOW())"
		" ON CONFLICT (platform, platform_id, slot_index) DO UPDATE SET"
		"  character_name = EXCLUDED.character_name,"
		"  level = EXCLUDED.level,"
		"  experience = EXCLUDED.experience,"
		"  saved_max_health = EXCLUDED.saved_max_health,"
		"  saved_position_x = EXCLUDED.saved_position_x,"
		"  saved_position_y = EXCLUDED.saved_position_y,"
		"  saved_position_z = EXCLUDED.saved_position_z,"
		"  saved_rotation_yaw = EXCLUDED.saved_rotation_yaw,"
		"  inventory_json = EXCLUDED.inventory_json,"
		"  equipment_json = EXCLUDED.equipment_json,"
		"  quests_json = EXCLUDED.quests_json,"
		"  current_zone = EXCLUDED.current_zone,"
		"  character_class = EXCLUDED.character_class,"
		"  appearance_json = EXCLUDED.appearance_json,"
		"  unspent_stat_points = EXCLUDED.unspent_stat_points,"
		"  prestige_level = EXCLUDED.prestige_level,"
		"  updated_at = NOW();";

	const char* Params[19] = {
		TCHAR_TO_UTF8(*Platform),
		TCHAR_TO_UTF8(*PlatformID),
		TCHAR_TO_UTF8(*SlotIdxStr),
		TCHAR_TO_UTF8(*Data.CharacterName),
		TCHAR_TO_UTF8(*LevelStr),
		TCHAR_TO_UTF8(*ExpStr),
		TCHAR_TO_UTF8(*HealthStr),
		TCHAR_TO_UTF8(*PosXStr),
		TCHAR_TO_UTF8(*PosYStr),
		TCHAR_TO_UTF8(*PosZStr),
		TCHAR_TO_UTF8(*YawStr),
		TCHAR_TO_UTF8(*Data.InventoryJSON),
		TCHAR_TO_UTF8(*Data.EquipmentJSON),
		TCHAR_TO_UTF8(*Data.QuestsJSON),
		TCHAR_TO_UTF8(*Data.CurrentZone),
		TCHAR_TO_UTF8(*ClassStr),
		TCHAR_TO_UTF8(*Data.AppearanceJSON),
		TCHAR_TO_UTF8(*StatPointsStr),
		TCHAR_TO_UTF8(*PrestigeStr)
	};

	PGresult* Res = PQexecParams(Conn, SQL, 19, NULL, Params, NULL, NULL, 0);
	bool bSuccess = (PQresultStatus(Res) == PGRES_COMMAND_OK);
	if (!bSuccess)
		LogPGError(Conn, "SaveCharacter");

	PQclear(Res);
	return bSuccess;
}

bool FPgSQLStore::DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
	if (!Conn) return false;

	FString SlotIdxStr = FString::Printf(TEXT("%d"), SlotIndex);
	const char* SQL = "DELETE FROM characters WHERE platform = $1 AND platform_id = $2 AND slot_index = $3;";
	const char* Params[3] = { TCHAR_TO_UTF8(*Platform), TCHAR_TO_UTF8(*PlatformID), TCHAR_TO_UTF8(*SlotIdxStr) };

	PGresult* Res = PQexecParams(Conn, SQL, 3, NULL, Params, NULL, NULL, 0);
	bool bSuccess = (PQresultStatus(Res) == PGRES_COMMAND_OK);
	if (!bSuccess)
		LogPGError(Conn, "DeleteCharacter");

	PQclear(Res);
	return bSuccess;
}

void FPgSQLStore::SaveAll()
{
	if (Conn)
	{
		Exec("CHECKPOINT;");
	}
}

#endif
