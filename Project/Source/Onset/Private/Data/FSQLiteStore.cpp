#include "Data/FSQLiteStore.h"
#include "sqlite/sqlite3.h"
#include "HAL/FileManager.h"
#include "Misc/Paths.h"

static void LogSQLError(struct sqlite3* InDB, const char* Context)
{
	UE_LOG(LogTemp, Error, TEXT("SQLite %s: %hs"), ANSI_TO_TCHAR(Context), sqlite3_errmsg(InDB));
}

static FString NormalizePath(const FString& Path)
{
	return FPaths::ConvertRelativePathToFull(Path);
}

FSQLiteStore::~FSQLiteStore()
{
	SaveAll();
	if (ActiveStmt)
	{
		sqlite3_finalize(ActiveStmt);
		ActiveStmt = nullptr;
	}
	if (DB)
	{
		if (bWALEnabled)
			Exec("PRAGMA wal_checkpoint(TRUNCATE);");
		sqlite3_close(DB);
		DB = nullptr;
	}
}

bool FSQLiteStore::Exec(const char* SQL)
{
	char* ErrMsg = nullptr;
	if (sqlite3_exec(DB, SQL, nullptr, nullptr, &ErrMsg) != SQLITE_OK)
	{
		UE_LOG(LogTemp, Error, TEXT("SQLite Exec: %s"), ANSI_TO_TCHAR(ErrMsg));
		sqlite3_free(ErrMsg);
		return false;
	}
	return true;
}

bool FSQLiteStore::PrepareAndBind(const char* SQL)
{
	if (ActiveStmt)
	{
		sqlite3_finalize(ActiveStmt);
		ActiveStmt = nullptr;
	}
	return sqlite3_prepare_v2(DB, SQL, -1, &ActiveStmt, nullptr) == SQLITE_OK;
}

bool FSQLiteStore::Initialize(const FString& InConnectionString)
{
	DBPath = InConnectionString.IsEmpty()
		? FPaths::ProjectSavedDir() / TEXT("OnsetPlayerData.db")
		: NormalizePath(InConnectionString);

	IFileManager& FileManager = IFileManager::Get();
	bool bFileExisted = FileManager.FileExists(*DBPath);

	if (sqlite3_open(TCHAR_TO_UTF8(*DBPath), &DB) != SQLITE_OK)
	{
		LogSQLError(DB, "sqlite3_open");
		return false;
	}

	if (!Exec("PRAGMA journal_mode=WAL;"))
		return false;
	bWALEnabled = true;

	if (!Exec("PRAGMA synchronous=NORMAL;"))
		return false;

	if (!Exec("PRAGMA foreign_keys=ON;"))
		return false;

	if (!EnsureSchema())
		return false;

	UE_LOG(LogTemp, Log, TEXT("FSQLiteStore: opened %s (existing=%d)"), *DBPath, bFileExisted);
	return true;
}

bool FSQLiteStore::EnsureSchema()
{
	if (!Exec(
		"CREATE TABLE IF NOT EXISTS _schema_version ("
		"  version INTEGER PRIMARY KEY,"
		"  applied_at TEXT NOT NULL DEFAULT (datetime('now'))"
		");"))
		return false;

	int32 Version = GetSchemaVersion();
	if (Version < 0)
		return false;

	const int32 LatestVersion = 2;
	while (Version < LatestVersion)
	{
		RunMigration(Version);
		Version = GetSchemaVersion();
	}
	return true;
}

int32 FSQLiteStore::GetSchemaVersion()
{
	if (!PrepareAndBind("SELECT COALESCE(MAX(version), 0) FROM _schema_version;"))
		return -1;

	int32 Result = -1;
	if (sqlite3_step(ActiveStmt) == SQLITE_ROW)
		Result = sqlite3_column_int(ActiveStmt, 0);

	sqlite3_finalize(ActiveStmt);
	ActiveStmt = nullptr;
	return Result;
}

void FSQLiteStore::RunMigration(int32 FromVersion)
{
	if (FromVersion < 0) return;

	if (FromVersion == 0)
	{
		if (!Exec(
			"CREATE TABLE accounts ("
			"  platform TEXT NOT NULL,"
			"  platform_id TEXT NOT NULL,"
			"  created_at TEXT NOT NULL DEFAULT (datetime('now')),"
			"  last_login TEXT NOT NULL DEFAULT (datetime('now')),"
			"  PRIMARY KEY (platform, platform_id)"
			");"))
		{
			UE_LOG(LogTemp, Error, TEXT("FSQLiteStore: migration 1 failed (accounts table)"));
			return;
		}

		if (!Exec(
			"CREATE TABLE characters ("
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
			"  created_at TEXT NOT NULL DEFAULT (datetime('now')),"
			"  updated_at TEXT NOT NULL DEFAULT (datetime('now')),"
			"  PRIMARY KEY (platform, platform_id, slot_index),"
			"  FOREIGN KEY (platform, platform_id) REFERENCES accounts(platform, platform_id) ON DELETE CASCADE"
			");"))
		{
			UE_LOG(LogTemp, Error, TEXT("FSQLiteStore: migration 1 failed (characters table)"));
			return;
		}

		Exec("INSERT INTO _schema_version (version) VALUES (1);");
		UE_LOG(LogTemp, Log, TEXT("FSQLiteStore: migration 1 applied (accounts + characters)"));
	}

	if (FromVersion <= 1)
	{
		if (!Exec("ALTER TABLE characters ADD COLUMN current_zone TEXT NOT NULL DEFAULT '';"))
		{
			UE_LOG(LogTemp, Error, TEXT("FSQLiteStore: migration 2 failed (current_zone column)"));
			return;
		}

		Exec("INSERT INTO _schema_version (version) VALUES (2);");
		UE_LOG(LogTemp, Log, TEXT("FSQLiteStore: migration 2 applied (current_zone column)"));
	}
}

bool FSQLiteStore::LoadAccount(const FString& Platform, const FString& PlatformID, FOnsetAccountData& OutAccount)
{
	if (!DB) return false;

	const char* SQL = "SELECT platform, platform_id FROM accounts WHERE platform = ?1 AND platform_id = ?2;";
	if (!PrepareAndBind(SQL))
		return false;

	sqlite3_bind_text(ActiveStmt, 1, TCHAR_TO_UTF8(*Platform), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(ActiveStmt, 2, TCHAR_TO_UTF8(*PlatformID), -1, SQLITE_TRANSIENT);

	if (sqlite3_step(ActiveStmt) != SQLITE_ROW)
	{
		sqlite3_finalize(ActiveStmt);
		ActiveStmt = nullptr;
		return false;
	}

	OutAccount.Platform = UTF8_TO_TCHAR(sqlite3_column_text(ActiveStmt, 0));
	OutAccount.PlatformID = UTF8_TO_TCHAR(sqlite3_column_text(ActiveStmt, 1));
	sqlite3_finalize(ActiveStmt);
	ActiveStmt = nullptr;

	const char* SlotSQL = "SELECT slot_index, character_name, level FROM characters WHERE platform = ?1 AND platform_id = ?2 ORDER BY slot_index;";
	if (!PrepareAndBind(SlotSQL))
		return false;

	sqlite3_bind_text(ActiveStmt, 1, TCHAR_TO_UTF8(*Platform), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(ActiveStmt, 2, TCHAR_TO_UTF8(*PlatformID), -1, SQLITE_TRANSIENT);

	OutAccount.Slots.Empty();
	int32 ExpectedSlot = 0;
	while (sqlite3_step(ActiveStmt) == SQLITE_ROW)
	{
		int32 SlotIdx = sqlite3_column_int(ActiveStmt, 0);
		while (ExpectedSlot < SlotIdx)
		{
			FOnsetCharacterSlotData& EmptySlot = OutAccount.Slots.AddDefaulted_GetRef();
			EmptySlot.SlotIndex = ExpectedSlot;
			EmptySlot.bOccupied = false;
			ExpectedSlot++;
		}

		FOnsetCharacterSlotData& Slot = OutAccount.Slots.AddDefaulted_GetRef();
		Slot.SlotIndex = SlotIdx;
		Slot.CharacterName = UTF8_TO_TCHAR(sqlite3_column_text(ActiveStmt, 1));
		Slot.Level = sqlite3_column_int(ActiveStmt, 2);
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

	sqlite3_finalize(ActiveStmt);
	ActiveStmt = nullptr;
	return true;
}

bool FSQLiteStore::CreateAccount(const FString& Platform, const FString& PlatformID)
{
	if (!DB) return false;

	if (!PrepareAndBind("INSERT OR IGNORE INTO accounts (platform, platform_id) VALUES (?1, ?2);"))
		return false;

	sqlite3_bind_text(ActiveStmt, 1, TCHAR_TO_UTF8(*Platform), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(ActiveStmt, 2, TCHAR_TO_UTF8(*PlatformID), -1, SQLITE_TRANSIENT);

	int32 RC = sqlite3_step(ActiveStmt);
	sqlite3_finalize(ActiveStmt);
	ActiveStmt = nullptr;

	if (RC != SQLITE_DONE)
	{
		LogSQLError(DB, "CreateAccount");
		return false;
	}
	return true;
}

bool FSQLiteStore::LoadCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex, FOnsetFullCharacterData& OutData)
{
	if (!DB) return false;

	const char* SQL = "SELECT slot_index, character_name, level, experience,"
		" saved_max_health, saved_position_x, saved_position_y, saved_position_z, saved_rotation_yaw,"
		" inventory_json, equipment_json, quests_json, current_zone"
		" FROM characters WHERE platform = ?1 AND platform_id = ?2 AND slot_index = ?3;";

	if (!PrepareAndBind(SQL))
		return false;

	sqlite3_bind_text(ActiveStmt, 1, TCHAR_TO_UTF8(*Platform), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(ActiveStmt, 2, TCHAR_TO_UTF8(*PlatformID), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(ActiveStmt, 3, SlotIndex);

	if (sqlite3_step(ActiveStmt) != SQLITE_ROW)
	{
		sqlite3_finalize(ActiveStmt);
		ActiveStmt = nullptr;
		return false;
	}

	OutData.SlotIndex = sqlite3_column_int(ActiveStmt, 0);
	OutData.CharacterName = UTF8_TO_TCHAR(sqlite3_column_text(ActiveStmt, 1));
	OutData.Level = sqlite3_column_int(ActiveStmt, 2);
	OutData.Experience = sqlite3_column_int(ActiveStmt, 3);
	OutData.SavedMaxHealth = static_cast<float>(sqlite3_column_double(ActiveStmt, 4));
	OutData.SavedPosition.X = static_cast<float>(sqlite3_column_double(ActiveStmt, 5));
	OutData.SavedPosition.Y = static_cast<float>(sqlite3_column_double(ActiveStmt, 6));
	OutData.SavedPosition.Z = static_cast<float>(sqlite3_column_double(ActiveStmt, 7));
	OutData.SavedRotationYaw = static_cast<float>(sqlite3_column_double(ActiveStmt, 8));
	OutData.InventoryJSON = UTF8_TO_TCHAR(sqlite3_column_text(ActiveStmt, 9));
	OutData.EquipmentJSON = UTF8_TO_TCHAR(sqlite3_column_text(ActiveStmt, 10));
	OutData.QuestsJSON = UTF8_TO_TCHAR(sqlite3_column_text(ActiveStmt, 11));
	OutData.CurrentZone = UTF8_TO_TCHAR(sqlite3_column_text(ActiveStmt, 12));

	sqlite3_finalize(ActiveStmt);
	ActiveStmt = nullptr;
	return true;
}

bool FSQLiteStore::SaveCharacter(const FString& Platform, const FString& PlatformID, const FOnsetFullCharacterData& Data)
{
	if (!DB) return false;

	const char* SQL = "INSERT OR REPLACE INTO characters"
		" (platform, platform_id, slot_index, character_name, level, experience,"
		"  saved_max_health, saved_position_x, saved_position_y, saved_position_z, saved_rotation_yaw,"
		"  inventory_json, equipment_json, quests_json, current_zone, updated_at)"
		" VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9, ?10, ?11, ?12, ?13, ?14, ?15, datetime('now'));";

	if (!PrepareAndBind(SQL))
		return false;

	sqlite3_bind_text(ActiveStmt, 1, TCHAR_TO_UTF8(*Platform), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(ActiveStmt, 2, TCHAR_TO_UTF8(*PlatformID), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(ActiveStmt, 3, Data.SlotIndex);
	sqlite3_bind_text(ActiveStmt, 4, TCHAR_TO_UTF8(*Data.CharacterName), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(ActiveStmt, 5, Data.Level);
	sqlite3_bind_int(ActiveStmt, 6, Data.Experience);
	sqlite3_bind_double(ActiveStmt, 7, Data.SavedMaxHealth);
	sqlite3_bind_double(ActiveStmt, 8, Data.SavedPosition.X);
	sqlite3_bind_double(ActiveStmt, 9, Data.SavedPosition.Y);
	sqlite3_bind_double(ActiveStmt, 10, Data.SavedPosition.Z);
	sqlite3_bind_double(ActiveStmt, 11, Data.SavedRotationYaw);
	sqlite3_bind_text(ActiveStmt, 12, TCHAR_TO_UTF8(*Data.InventoryJSON), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(ActiveStmt, 13, TCHAR_TO_UTF8(*Data.EquipmentJSON), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(ActiveStmt, 14, TCHAR_TO_UTF8(*Data.QuestsJSON), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(ActiveStmt, 15, TCHAR_TO_UTF8(*Data.CurrentZone), -1, SQLITE_TRANSIENT);

	int32 RC = sqlite3_step(ActiveStmt);
	sqlite3_finalize(ActiveStmt);
	ActiveStmt = nullptr;

	if (RC != SQLITE_DONE)
	{
		LogSQLError(DB, "SaveCharacter");
		return false;
	}
	return true;
}

bool FSQLiteStore::DeleteCharacter(const FString& Platform, const FString& PlatformID, int32 SlotIndex)
{
	if (!DB) return false;

	if (!PrepareAndBind("DELETE FROM characters WHERE platform = ?1 AND platform_id = ?2 AND slot_index = ?3;"))
		return false;

	sqlite3_bind_text(ActiveStmt, 1, TCHAR_TO_UTF8(*Platform), -1, SQLITE_TRANSIENT);
	sqlite3_bind_text(ActiveStmt, 2, TCHAR_TO_UTF8(*PlatformID), -1, SQLITE_TRANSIENT);
	sqlite3_bind_int(ActiveStmt, 3, SlotIndex);

	int32 RC = sqlite3_step(ActiveStmt);
	sqlite3_finalize(ActiveStmt);
	ActiveStmt = nullptr;

	return RC == SQLITE_DONE;
}

void FSQLiteStore::SaveAll()
{
	if (DB && bWALEnabled)
	{
		Exec("PRAGMA wal_checkpoint(PASSIVE);");
	}
}
