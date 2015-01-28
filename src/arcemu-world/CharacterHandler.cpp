/*
* ArcEmu MMORPG Server
* Copyright (C) 2005-2007 Ascent Team <http://www.ascentemu.com/>
* Copyright (C) 2008-2012 <http://www.ArcEmu.org/>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include "StdAfx.h"
#include "git_version.h"

inline uint64 MAKE_NEW_GUID(uint32 l, uint32 e, uint32 h);

uint64 MAKE_NEW_GUID(uint32 l, uint32 e, uint32 h)
{
	return uint64(uint64(l) | (uint64(e) << 32) | (uint64(h) << ((h == 0xF0C0 || h == 0xF102) ? 48 : 52)));
}

LoginErrorCode VerifyName(const char* name, size_t nlen)
{
	const char* p;
	size_t i;

	static const char* bannedCharacters = "\t\v\b\f\a\n\r\\\"\'\? <>[](){}_=+-|/!@#$%^&*~`.,0123456789\0";
	static const char* allowedCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

	if (sWorld.m_limitedNames)
	{
		if (nlen == 0)
			return E_CHAR_NAME_NO_NAME;
		else if (nlen < 2)
			return E_CHAR_NAME_TOO_SHORT;
		else if (nlen > 12)
			return E_CHAR_NAME_TOO_LONG;

		for (i = 0; i < nlen; ++i)
		{
			p = allowedCharacters;
			for (; *p != 0; ++p)
			{
				if (name[i] == *p)
					goto cont;
			}
			return E_CHAR_NAME_INVALID_CHARACTER;
		cont:
			continue;
		}
	}
	else
	{
		for (i = 0; i < nlen; ++i)
		{
			p = bannedCharacters;
			while (*p != 0 && name[i] != *p && name[i] != 0)
				++p;

			if (*p != 0)
				return E_CHAR_NAME_INVALID_CHARACTER;
		}
	}

	return E_CHAR_NAME_SUCCESS;
}

bool ChatHandler::HandleRenameAllCharacter(const char* args, WorldSession* m_session)
{
	uint32 uCount = 0;
	uint32 ts = getMSTime();
	QueryResult* result = CharacterDatabase.Query("SELECT guid, name FROM characters");
	if (result)
	{
		do
		{
			uint32 uGuid = result->Fetch()[0].GetUInt32();
			const char* pName = result->Fetch()[1].GetString();
			size_t szLen = strlen(pName);

			if (VerifyName(pName, szLen) != E_CHAR_NAME_SUCCESS)
			{
				LOG_DEBUG("renaming character %s, %u", pName, uGuid);
				Player* pPlayer = objmgr.GetPlayer(uGuid);
				if (pPlayer != NULL)
				{
					pPlayer->rename_pending = true;
					pPlayer->GetSession()->SystemMessage("Your character has had a force rename set, you will be prompted to rename your character at next login in conformance with server rules.");
				}

				CharacterDatabase.WaitExecute("UPDATE characters SET forced_rename_pending = 1 WHERE guid = %u", uGuid);
				++uCount;
			}

		} while (result->NextRow());
		delete result;
	}

	SystemMessage(m_session, "Procedure completed in %u ms. %u character(s) forced to rename.", getMSTime() - ts, uCount);
	return true;
}

void CapitalizeString(string & arg)
{
	if (arg.length() == 0) return;
	arg[0] = static_cast<char>(toupper(arg[0]));
	for (uint32 x = 1; x < arg.size(); ++x)
		arg[x] = static_cast<char>(tolower(arg[x]));
}

void WorldSession::CharacterEnumProc(QueryResult* result)
{
	uint32 charCount = 0;
	ByteBuffer bitBuffer;
	ByteBuffer dataBuffer;

	bitBuffer.WriteBits(0, 21);
	bitBuffer.WriteBits(result ? result->GetRowCount() : 0, 16);

	if (result)
	{
		charCount = uint32(result->GetRowCount());
		bitBuffer.reserve(24 * charCount / 8);
		dataBuffer.reserve(charCount * 381);

		do
		{
			Player::BuildEnumData(result, &dataBuffer, &bitBuffer);

		} while (result->NextRow());
	}
		bitBuffer.WriteBit(1);
		bitBuffer.FlushBits();
	

	WorldPacket data(SMSG_CHAR_ENUM, 7 + bitBuffer.size() + dataBuffer.size());

	data.append(bitBuffer);

	if (charCount)
		data.append(dataBuffer);

	SendPacket(&data);
}

void WorldSession::HandleCharEnumOpcode(WorldPacket & recv_data)
{
	AsyncQuery * q = new AsyncQuery(new SQLClassCallbackP1<World, uint32>(World::getSingletonPtr(), &World::CharacterEnumProc, GetAccountId()));
	q->AddQuery("SELECT guid, level, race, class, gender, bytes, bytes2, name, positionX, positionY, positionZ, mapId, zoneId, banned, restState, deathstate, forced_rename_pending, player_flags, guild_data.guildid FROM characters LEFT JOIN guild_data ON characters.guid = guild_data.playerid WHERE acct=%u ORDER BY guid LIMIT 10", GetAccountId());
	CharacterDatabase.QueueAsyncQuery(q);
}

void WorldSession::LoadAccountDataProc(QueryResult* result)
{
	size_t len;
	const char* data;
	char* d;

	if (!result)
	{
		CharacterDatabase.Execute("INSERT INTO account_data VALUES(%u, '', '', '', '', '', '', '', '', '')", _accountId);
		return;
	}

	for (uint32 i = 0; i < 7; ++i)
	{
		data = result->Fetch()[1 + i].GetString();
		len = data ? strlen(data) : 0;
		if (len > 1)
		{
			d = new char[len + 1];
			memcpy(d, data, len + 1);
			SetAccountData(i, d, true, (uint32)len);
		}
	}
}

void WorldSession::HandleCharCreateOpcode(WorldPacket & recv_data)
{
	CHECK_PACKET_SIZE(recv_data, 10);

	uint8 hairStyle;
	uint8 face;
	uint8 facialHair;
	uint8 hairColor;
	uint8 race_;
	uint8 class_;
	uint8 skin;
	uint8 gender;
	uint8 outfitId;

	recv_data 
			>> outfitId 
			>> hairStyle 
			>> class_ 
			>> skin
			>> face 
			>> race_ 
			>> facialHair
			>> gender 
			>> hairColor;	

	uint32 nameLength = recv_data.ReadBits(6);
	uint8 unk = recv_data.ReadBit();
	std::string name = recv_data.ReadString(nameLength);
	
	if (unk)
		recv_data.read<uint32>();

	recv_data.rpos(0);

	LoginErrorCode res = VerifyName(name.c_str(), name.length());
	if (res != E_CHAR_NAME_SUCCESS)
	{
		OutPacket(SMSG_CHAR_CREATE, 1, &res);
		return;
	}

	res = g_characterNameFilter->Parse(name, false) ? E_CHAR_NAME_PROFANE : E_CHAR_NAME_SUCCESS;
	if (res != E_CHAR_NAME_SUCCESS)
	{
		OutPacket(SMSG_CHAR_CREATE, 1, &res);
		return;
	}

	res = objmgr.GetPlayerInfoByName(name.c_str()) == NULL ? E_CHAR_CREATE_SUCCESS : E_CHAR_CREATE_NAME_IN_USE;
	if (res != E_CHAR_CREATE_SUCCESS)
	{
		OutPacket(SMSG_CHAR_CREATE, 1, &res);
		return;
	}

	res = sHookInterface.OnNewCharacter(race_, class_, this, name.c_str()) ? E_CHAR_CREATE_SUCCESS : E_CHAR_CREATE_ERROR;
	if (res != E_CHAR_CREATE_SUCCESS)
	{
		OutPacket(SMSG_CHAR_CREATE, 1, &res);
		return;
	}

	QueryResult* result = CharacterDatabase.Query("SELECT COUNT(*) FROM banned_names WHERE name = '%s'", CharacterDatabase.EscapeString(name).c_str());
	if (result)
	{
		if (result->Fetch()[0].GetUInt32() > 0)
		{
			OutPacket(SMSG_CHAR_CREATE, 1, "\x60");
			
			delete result;
			return;
		}
		delete result;
	}

	// Check if player got Death Knight already on this realm.
	if (Config.OptionalConfig.GetBoolDefault("ClassOptions", "DeathKnightLimit", true) && has_dk
		&& (class_ == DEATHKNIGHT))
	{
		WorldPacket data(SMSG_CHAR_CREATE);
		data << uint8(CHAR_CREATE_UNIQUE_CLASS_LIMIT);
		SendPacket(&data);
		return;
	}

	// loading characters

	// Check the number of characters, so we can't make over 10.
	// They're able to manage to create >10 sometimes, not exactly sure how ..

	result = CharacterDatabase.Query("SELECT COUNT(*) FROM characters WHERE acct = %u", GetAccountId());
	if (result)
	{
		if (result->Fetch()[0].GetUInt32() >= 10)
		{
			// We can't make any more characters.
			WorldPacket data(SMSG_CHAR_CREATE);
			data << uint8(CHAR_CREATE_SERVER_LIMIT);
			SendPacket(&data);
			delete result;
			return;
		}
		delete result;
	}

	Player* pNewChar = objmgr.CreatePlayer(class_);
	pNewChar->SetSession(this);
	if (!pNewChar->Create(recv_data))
	{
		// failed.
		pNewChar->ok_to_remove = true;
		delete pNewChar;
		WorldPacket data(SMSG_CHAR_CREATE);
		data << uint8(CHAR_CREATE_FAILED);
		SendPacket(&data);
		return;
	}

	//Same Faction limitation only applies to PVP and RPPVP realms :)
	uint32 realmType = sLogonCommHandler.GetRealmType();
	if (!HasGMPermissions() && realmType == REALMTYPE_PVP && _side >= 0 && !sWorld.crossover_chars)  // ceberwow fixed bug
	{
		if ((pNewChar->IsTeamAlliance() && (_side == 1)) || (pNewChar->IsTeamHorde() && (_side == 0)))
		{
			pNewChar->ok_to_remove = true;
			delete pNewChar;
			WorldPacket data(SMSG_CHAR_CREATE);
			data << uint8(CHAR_CREATE_PVP_TEAMS_VIOLATION);
			SendPacket(&data);
			return;
		}
	}

	//Check if player has a level 55 or higher character on this realm and allow him to create DK.
	//This check can be turned off in optional.conf
	if (Config.OptionalConfig.GetBoolDefault("ClassOptions", "DeathKnightPreReq", false) && !has_level_55_char
		&& (class_ == DEATHKNIGHT))
	{
		pNewChar->ok_to_remove = true;
		delete pNewChar;

		WorldPacket data(SMSG_CHAR_CREATE);
		data << uint8(CHAR_CREATE_LEVEL_REQUIREMENT);
		SendPacket(&data);
		return;
	}

	pNewChar->UnSetBanned();
	pNewChar->addSpell(22027);	  // Remove Insignia

	if (pNewChar->getClass() == WARLOCK)
	{
		pNewChar->AddSummonSpell(416, 3110);		// imp fireball
		pNewChar->AddSummonSpell(417, 19505);
		pNewChar->AddSummonSpell(1860, 3716);
		pNewChar->AddSummonSpell(1863, 7814);
	}

	pNewChar->SaveToDB(true);

	PlayerInfo* pn = new PlayerInfo;
	pn->guid = pNewChar->GetLowGUID();
	pn->name = strdup(pNewChar->GetName());
	pn->cl = pNewChar->getClass();
	pn->race = pNewChar->getRace();
	pn->gender = pNewChar->getGender();
	pn->acct = GetAccountId();
	pn->m_Group = 0;
	pn->subGroup = 0;
	pn->m_loggedInPlayer = NULL;
	pn->team = pNewChar->GetTeam();
	pn->guild = NULL;
	pn->guildRank = NULL;
	pn->guildMember = NULL;
	pn->lastOnline = UNIXTIME;
	objmgr.AddPlayerInfo(pn);

	pNewChar->ok_to_remove = true;
	delete  pNewChar;
	
	OutPacket(SMSG_CHAR_CREATE, 1, "\x2F");

	sLogonCommHandler.UpdateAccountCount(GetAccountId(), 1);
}

void WorldSession::HandleCharDeleteOpcode(WorldPacket & recv_data)
{
	//CHECK_PACKET_SIZE(recv_data, 8);
	std::string fail = "\x48";
	
	ObjectGuid guid;
	
	guid[1] = recv_data.ReadBit();
	guid[3] = recv_data.ReadBit();
	guid[2] = recv_data.ReadBit();
	guid[7] = recv_data.ReadBit();
	guid[4] = recv_data.ReadBit();
	guid[6] = recv_data.ReadBit();
	guid[0] = recv_data.ReadBit();
	guid[5] = recv_data.ReadBit();
	
	recv_data.ReadByteSeq(guid[7]);
	recv_data.ReadByteSeq(guid[1]);
	recv_data.ReadByteSeq(guid[6]);
	recv_data.ReadByteSeq(guid[0]);
	recv_data.ReadByteSeq(guid[3]);
	recv_data.ReadByteSeq(guid[4]);
	recv_data.ReadByteSeq(guid[2]);
	recv_data.ReadByteSeq(guid[5]);

	if (objmgr.GetPlayer(guid) != NULL)
	{
		OutPacket(SMSG_CHAR_DELETE, 1, "\x49");
	}
	else
	{
		fail = DeleteCharacter(guid);
		OutPacket(SMSG_CHAR_DELETE, 1, "\x48");
	}
	
}

uint8 WorldSession::DeleteCharacter(uint32 guid)
{
	PlayerInfo* inf = objmgr.GetPlayerInfo(guid);
	if (inf != NULL && inf->m_loggedInPlayer == NULL)
	{
		QueryResult* result = CharacterDatabase.Query("SELECT name FROM characters WHERE guid = %u AND acct = %u", (uint32)guid, _accountId);
		if (!result)
			return E_CHAR_DELETE_FAILED;

		string name = result->Fetch()[0].GetString();
		delete result;

		if (inf->guild)
		{
			if (inf->guild->GetGuildLeader() == inf->guid)
				return E_CHAR_DELETE_FAILED_GUILD_LEADER;
			else
				inf->guild->RemoveGuildMember(inf, NULL);
		}

		for (int i = 0; i < NUM_CHARTER_TYPES; ++i)
		{
			Charter* c = objmgr.GetCharterByGuid(guid, (CharterTypes)i);
			if (c != NULL)
				c->RemoveSignature((uint32)guid);
		}


		for (int i = 0; i < NUM_ARENA_TEAM_TYPES; ++i)
		{
			ArenaTeam* t = objmgr.GetArenaTeamByGuid((uint32)guid, i);
			if (t != NULL && t->m_leader == guid)
				return E_CHAR_DELETE_FAILED_ARENA_CAPTAIN;
			if (t != NULL)
				t->RemoveMember(inf);
		}

		/*if( _socket != NULL )
		sPlrLog.write("Account: %s | IP: %s >> Deleted player %s", GetAccountName().c_str(), GetSocket()->GetRemoteIP().c_str(), name.c_str());*/

		sPlrLog.writefromsession(this, "deleted character %s (GUID: %u)", name.c_str(), (uint32)guid);

		CharacterDatabase.WaitExecute("DELETE FROM characters WHERE guid = %u", (uint32)guid);

		Corpse* c = objmgr.GetCorpseByOwner((uint32)guid);
		if (c)
			CharacterDatabase.Execute("DELETE FROM corpses WHERE guid = %u", c->GetLowGUID());

		CharacterDatabase.Execute("DELETE FROM playeritems WHERE ownerguid=%u", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM gm_tickets WHERE playerguid = %u", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM playerpets WHERE ownerguid = %u", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM playerpetspells WHERE ownerguid = %u", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM tutorials WHERE playerId = %u", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM questlog WHERE player_guid = %u", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM playercooldowns WHERE player_guid = %u", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM mailbox WHERE player_guid = %u", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM social_friends WHERE character_guid = %u OR friend_guid = %u", (uint32)guid, (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM social_ignores WHERE character_guid = %u OR ignore_guid = %u", (uint32)guid, (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM character_achievement WHERE guid = '%u' AND achievement NOT IN (457, 467, 466, 465, 464, 463, 462, 461, 460, 459, 458, 1404, 1405, 1406, 1407, 1408, 1409, 1410, 1411, 1412, 1413, 1415, 1414, 1416, 1417, 1418, 1419, 1420, 1421, 1422, 1423, 1424, 1425, 1426, 1427, 1463, 1400, 456, 1402)", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM character_achievement_progress WHERE guid = '%u'", (uint32)guid);
		CharacterDatabase.Execute("DELETE FROM playerspells WHERE GUID = '%u'", guid);
		CharacterDatabase.Execute("DELETE FROM playerdeletedspells WHERE GUID = '%u'", guid);
		CharacterDatabase.Execute("DELETE FROM playerreputations WHERE guid = '%u'", guid);
		CharacterDatabase.Execute("DELETE FROM playerskills WHERE GUID = '%u'", guid);

		/* remove player info */
		objmgr.DeletePlayerInfo((uint32)guid);
		return E_CHAR_DELETE_SUCCESS;
	}
	return E_CHAR_DELETE_FAILED;
}

void WorldSession::HandleCharRenameOpcode(WorldPacket & recv_data)
{
	WorldPacket data(SMSG_CHAR_RENAME, recv_data.size() + 1);

	uint64 guid;
	string name;
	recv_data >> guid >> name;

	PlayerInfo* pi = objmgr.GetPlayerInfo((uint32)guid);
	if (pi == 0) return;

	QueryResult* result = CharacterDatabase.Query("SELECT forced_rename_pending FROM characters WHERE guid = %u AND acct = %u",
		(uint32)guid, _accountId);
	if (result == 0)
	{
		delete result;
		return;
	}
	delete result;

	// Check name for rule violation.

	LoginErrorCode err = VerifyName(name.c_str(), name.length());
	if (err != E_CHAR_NAME_SUCCESS)
	{
		data << uint8(err);
		data << guid << name;
		SendPacket(&data);
		return;
	}

	QueryResult* result2 = CharacterDatabase.Query("SELECT COUNT(*) FROM banned_names WHERE name = '%s'", CharacterDatabase.EscapeString(name).c_str());
	if (result2)
	{
		if (result2->Fetch()[0].GetUInt32() > 0)
		{
			// That name is banned!
			data << uint8(E_CHAR_NAME_PROFANE);
			data << guid << name;
			SendPacket(&data);
		}
		delete result2;
	}

	// Check if name is in use.
	if (objmgr.GetPlayerInfoByName(name.c_str()) != NULL)
	{
		data << uint8(E_CHAR_CREATE_NAME_IN_USE);
		data << guid << name;
		SendPacket(&data);
		return;
	}

	// correct capitalization
	CapitalizeString(name);
	objmgr.RenamePlayerInfo(pi, pi->name, name.c_str());

	sPlrLog.writefromsession(this, "a rename was pending. renamed character %s (GUID: %u) to %s.", pi->name, pi->guid, name.c_str());

	// If we're here, the name is okay.
	free(pi->name);
	pi->name = strdup(name.c_str());
	CharacterDatabase.WaitExecute("UPDATE characters SET name = '%s' WHERE guid = %u", name.c_str(), (uint32)guid);
	CharacterDatabase.WaitExecute("UPDATE characters SET forced_rename_pending = 0 WHERE guid = %u", (uint32)guid);

	data << uint8(E_RESPONSE_SUCCESS) << guid << name;
	SendPacket(&data);
}


void WorldSession::HandlePlayerLoginOpcode(WorldPacket & recv_data)
{
	ObjectGuid playerGuid;

	float farClip = 0.0f;
	recv_data >> farClip;

	playerGuid[1] = recv_data.ReadBit();
	playerGuid[4] = recv_data.ReadBit();
	playerGuid[7] = recv_data.ReadBit();
	playerGuid[3] = recv_data.ReadBit();
	playerGuid[2] = recv_data.ReadBit();
	playerGuid[6] = recv_data.ReadBit();
	playerGuid[5] = recv_data.ReadBit();
	playerGuid[0] = recv_data.ReadBit();

	recv_data.ReadByteSeq(playerGuid[5]);
	recv_data.ReadByteSeq(playerGuid[1]);
	recv_data.ReadByteSeq(playerGuid[0]);
	recv_data.ReadByteSeq(playerGuid[6]);
	recv_data.ReadByteSeq(playerGuid[2]);
	recv_data.ReadByteSeq(playerGuid[4]);
	recv_data.ReadByteSeq(playerGuid[7]);
	recv_data.ReadByteSeq(playerGuid[3]);
	

	if (objmgr.GetPlayer((uint32)playerGuid) != NULL || m_loggingInPlayer || _player)
	{
		// A character with that name already exists 0x3E
		uint8 respons = E_CHAR_LOGIN_DUPLICATE_CHARACTER;
		OutPacket(SMSG_CHARACTER_LOGIN_FAILED, 1, &respons);
		return;
	}

	// we make sure the guid is the correct one
	//printf("guid: %d\n", Arcemu::Util::GUID_LOPART(pGuid));
	LOG_ERROR("lOGGING IN GUID : %u", playerGuid);

	AsyncQuery* q = new AsyncQuery(new SQLClassCallbackP0<WorldSession>(this, &WorldSession::LoadPlayerFromDBProc));
	q->AddQuery("SELECT guid,class FROM characters WHERE guid = %u AND forced_rename_pending = 0", playerGuid); // 0
	CharacterDatabase.QueueAsyncQuery(q);
}

void WorldSession::LoadPlayerFromDBProc(QueryResultVector & results)
{
	if (results.size() < 1)
	{
		uint8 respons = E_CHAR_LOGIN_NO_CHARACTER;
		OutPacket(SMSG_CHARACTER_LOGIN_FAILED, 1, &respons);
		return;
	}

	QueryResult* result = results[0].result;
	if (!result)
	{
		Log.Error("WorldSession::LoadPlayerFromDBProc", "Player login query failed!");
		uint8 respons = E_CHAR_LOGIN_NO_CHARACTER;
		OutPacket(SMSG_CHARACTER_LOGIN_FAILED, 1, &respons);
		return;
	}

	Field* fields = result->Fetch();

	uint64 playerGuid = fields[0].GetUInt64();
	uint8 _class = fields[1].GetUInt8();

	Player* plr = NULL;

	switch (_class)
	{
	case WARRIOR:
		plr = new Warrior(playerGuid);
		break;
	case PALADIN:
		plr = new Paladin(playerGuid);
		break;
	case HUNTER:
		plr = new Hunter(playerGuid);
		break;
	case ROGUE:
		plr = new Rogue(playerGuid);
		break;
	case PRIEST:
		plr = new Priest(playerGuid);
		break;
	case DEATHKNIGHT:
		plr = new DeathKnight(playerGuid);
		break;
	case SHAMAN:
		plr = new Shaman(playerGuid);
		break;
	case MAGE:
		plr = new Mage(playerGuid);
		break;
	case WARLOCK:
		plr = new Warlock(playerGuid);
		break;
	case DRUID:
		plr = new Druid(playerGuid);
		break;
	}

	if (plr == NULL)
	{
		Log.Error("WorldSession::LoadPlayerFromDBProc", "Class %u unknown!", _class);
		uint8 respons = E_CHAR_LOGIN_NO_CHARACTER;
		OutPacket(SMSG_CHARACTER_LOGIN_FAILED, 1, &respons);
		return;
	}

	plr->SetSession(this);

	m_bIsWLevelSet = false;

	Log.Debug("WorldSession", "Async loading player %u", (uint32)playerGuid);
	m_loggingInPlayer = plr;
	plr->LoadFromDB((uint32)playerGuid);
}

void WorldSession::FullLogin(Player* plr)
{
	Log.Debug("WorldSession", "Fully loading player %u", plr->GetLowGUID());

	SetPlayer(plr);
	m_MoverWoWGuid.Init(plr->GetGUID());

	MapMgr* mgr = sInstanceMgr.GetInstance(plr);
	if (mgr && mgr->m_battleground)
	{
		// Don't allow player to login into a bg that has ended or is full
		if (mgr->m_battleground->HasEnded() == true ||
			mgr->m_battleground->HasFreeSlots(plr->GetTeamInitial(), mgr->m_battleground->GetType() == false))
		{
			mgr = NULL;
		}
	}

	// Trying to log to an instance that doesn't exist anymore?
	if (!mgr)
	{
		if (!IS_INSTANCE(plr->m_bgEntryPointMap))
		{
			plr->m_position.x = plr->m_bgEntryPointX;
			plr->m_position.y = plr->m_bgEntryPointY;
			plr->m_position.z = plr->m_bgEntryPointZ;
			plr->m_position.o = plr->m_bgEntryPointO;
			plr->m_mapId = plr->m_bgEntryPointMap;
		}
		else
		{
			plr->m_position.x = plr->GetBindPositionX();
			plr->m_position.y = plr->GetBindPositionY();
			plr->m_position.z = plr->GetBindPositionZ();
			plr->m_position.o = 0;
			plr->m_mapId = plr->GetBindMapId();
		}
	}

	//printf("COPYING TO MOVEMENT ARRAY\n");

	// copy to movement array
	movement_packet[0] = m_MoverWoWGuid.GetNewGuidMask();
	memcpy(&movement_packet[1], m_MoverWoWGuid.GetNewGuid(), m_MoverWoWGuid.GetNewGuidLen());

	//printf("STARTING WORLD PRELOAD\n");

	// world preload
	uint32 VMapId;
	float VO;
	float VX;
	float VY;
	float VZ;

	// GMs should start on GM Island and be bound there
	if (HasGMPermissions() && plr->m_FirstLogin && sWorld.gamemaster_startonGMIsland)
	{
		VMapId = 1;
		VO = 0;
		VX = 16222.6f;
		VY = 16265.9f;
		VZ = 14.2085f;

		plr->m_position.x = VX;
		plr->m_position.y = VY;
		plr->m_position.z = VZ;
		plr->m_position.o = VO;
		plr->m_mapId = VMapId;

		plr->SetBindPoint(plr->GetPositionX(), plr->GetPositionY(), plr->GetPositionZ(), plr->GetMapId(), plr->GetZoneId());
	}
	else
	{
		VMapId = plr->GetMapId();
		VO = plr->GetOrientation();
		VX = plr->GetPositionX();
		VY = plr->GetPositionY();
		VZ = plr->GetPositionZ();
	}

	WorldPacket data(SMSG_LOGIN_VERIFY_WORLD, 20); // 5.4.8
	data << VX;
	data << VO;
	data << VY;
	data << VMapId;	
	data << VZ;
	
	SendPacket(&data);

	//printf("SMSG_LOGIN_VERIFY_WORLD\n");

	//StackWorldPacket<20> datab(SMSG_FEATURE_SYSTEM_STATUS);


	//datab.Initialize(SMSG_FEATURE_SYSTEM_STATUS);

	WorldPacket datax(SMSG_FEATURE_SYSTEM_STATUS, 4 + 4 + 4 + 1 + 4 + 2 + 4 + 4 + 4 + 4 + 4 + 4 + 4); // 5.4.8

	bool feedbackSystem = true;
	bool excessiveWarning = false;

	datax << uint32(0);                  // Scroll of Resurrection per day?
	datax << uint32(0);                  // Scroll of Resurrection current
	datax << uint32(0);
	datax << uint8(2);
	datax << uint32(0);

	datax.WriteBit(1);
	datax.WriteBit(1);                   // ingame shop status (0 - "The Shop is temporarily unavailable.")
	datax.WriteBit(1);
	datax.WriteBit(0);                   // Recruit a Friend button
	datax.WriteBit(0);                   // server supports voice chat
	datax.WriteBit(1);                   // show ingame shop icon
	datax.WriteBit(0);                   // Scroll of Resurrection button
	datax.WriteBit(excessiveWarning);    // excessive play time warning
	datax.WriteBit(0);                   // ingame shop parental control (1 - "Feature has been disabled by Parental Controls.")
	datax.WriteBit(feedbackSystem);      // feedback system (bug, suggestion and report systems)
	datax.FlushBits();

	if (excessiveWarning)
	{
		datax << uint32(0);              // excessive play time warning after period(in seconds)
		datax << uint32(0);
		datax << uint32(0);
	}

	if (feedbackSystem)
	{
		datax << uint32(0);
		datax << uint32(1);
		datax << uint32(10);
		datax << uint32(60000);
	}

	SendPacket(&datax);

	//printf("SMSG_FEATURE_SYSTEM_STATUS\n");


	WorldPacket dataldm(SMSG_LEARNED_DANCE_MOVES, 4 + 4); // trinitycore 4.3.4
	dataldm << uint64(0);
	SendPacket(&dataldm);
	//printf("SMSG_LEARNED_MOVES\n");

	plr->UpdateAttackSpeed();

	// Make sure our name exists (for premade system)
	PlayerInfo* info = objmgr.GetPlayerInfo(plr->GetLowGUID());

	if (info == NULL)
	{
		info = new PlayerInfo;
		info->cl = plr->getClass();
		info->gender = plr->getGender();
		info->guid = plr->GetLowGUID();
		info->name = strdup(plr->GetName());
		info->lastLevel = plr->getLevel();
		info->lastOnline = UNIXTIME;
		info->lastZone = plr->GetZoneId();
		info->race = plr->getRace();
		info->team = plr->GetTeam();
		info->guild = NULL;
		info->guildRank = NULL;
		info->guildMember = NULL;
		info->m_Group = NULL;
		info->subGroup = 0;
		objmgr.AddPlayerInfo(info);
	}
	plr->m_playerInfo = info;
	if (plr->m_playerInfo->guild)
	{
		plr->SetGuildId(plr->m_playerInfo->guild->GetGuildId());
		plr->SetGuildRank(plr->m_playerInfo->guildRank->iId);
	}

	info->m_loggedInPlayer = plr;

	// account data == UI config
	SendAccountDataTimes(PER_CHARACTER_CACHE_MASK);

	// Set TIME OF LOGIN
	CharacterDatabase.Execute("UPDATE characters SET online = 1 WHERE guid = %u", plr->GetLowGUID());

	bool enter_world = true;

	// Find our transporter and add us if we're on one.
	if (plr->transporter_info.guid != 0)
	{
		Transporter* pTrans = objmgr.GetTransporter(Arcemu::Util::GUID_LOPART(plr->transporter_info.guid));
		if (pTrans)
		{
			if (plr->IsDead())
			{
				plr->ResurrectPlayer();
				plr->SetHealth(plr->GetMaxHealth());
				plr->SetPower(POWER_TYPE_MANA, plr->GetMaxPower(POWER_TYPE_MANA));
			}

			float c_tposx = pTrans->GetPositionX() + plr->transporter_info.x;
			float c_tposy = pTrans->GetPositionY() + plr->transporter_info.y;
			float c_tposz = pTrans->GetPositionZ() + plr->transporter_info.z;

			if (plr->GetMapId() != pTrans->GetMapId())	   // loaded wrong map
			{
				plr->SetMapId(pTrans->GetMapId());

				StackWorldPacket<20> dataw(SMSG_NEW_WORLD); // 15595
				dataw << c_tposx;
				dataw << plr->GetOrientation();
				dataw << c_tposz;
				dataw << pTrans->GetMapId();
				dataw << c_tposy;

				SendPacket(&dataw);

				// shit is sent in worldport ack.
				enter_world = false;
			}

			plr->SetPosition(c_tposx, c_tposy, c_tposz, plr->GetOrientation(), false);
			plr->m_CurrentTransporter = pTrans;
			pTrans->AddPlayer(plr);
		}
	}

	Log.Debug("Login", "Player %s logged in.", plr->GetName());

	sWorld.incrementPlayerCount(plr->GetTeam());

	if (plr->m_FirstLogin)
	{
		uint32 introid = plr->info->introid;

		OutPacket(SMSG_TRIGGER_CINEMATIC, 4, &introid); // 4.3.4

		// what the fuck is this anyway?
		/*if(sWorld.m_AdditionalFun)    //cebernic: tells people who 's newbie :D
		{
		const int classtext[] = {0, 5, 6, 8, 9, 11, 0, 4, 3, 7, 0, 10};
		sWorld.SendLocalizedWorldText(true, "{65}", classtext[(uint32)plr->getClass() ] , plr->GetName() , (plr->IsTeamHorde() ? "{63}" : "{64}"));
		}*/

	}


	LOG_DETAIL("WORLD: Created new player for existing players (%s)", plr->GetName());

	// Login time, will be used for played time calc
	plr->m_playedtime[2] = uint32(UNIXTIME);

	// disabled
	//Issue a message telling all guild members that this player has signed on
	/*if(plr->IsInGuild())
	{
	Guild* pGuild = plr->m_playerInfo->guild;
	if(pGuild)
	{
	WorldPacket data(SMSG_GUILD_EVENT, 50); // do we need that much? // shoud work on 4.3.4

	data << uint8(GUILD_EVENT_MOTD);
	data << uint8(1);

	if(pGuild->GetMOTD())
	data << pGuild->GetMOTD();
	else
	data << uint8(0);

	SendPacket(&data);

	pGuild->LogGuildEvent(GUILD_EVENT_HASCOMEONLINE, 1, plr->GetName());
	}
	}*/

	// !!!! UNCOMMENT THESE ONCE THEY WORK PROPERLY !!!!
	// don't send this, it's a sandbox, they're not updated
	// Send online status to people having this char in friendlist
	//_player->Social_TellFriendsOnline(); // this should work on 4.3.4
	// send friend list (for ignores)
	//_player->Social_SendFriendList(7); // this should work on 4.3.4

	plr->SendDungeonDifficulty(); // 15595
	plr->SendRaidDifficulty();    // 15595

	//plr->SendEquipmentSetList(); // not sure 4.3.4

#ifndef GM_TICKET_MY_MASTER_COMPATIBLE
	GM_Ticket* ticket = objmgr.GetGMTicketByPlayer(_player->GetGUID());
	if (ticket != NULL)
	{
		//Send status change to gm_sync_channel
		Channel* chn = channelmgr.GetChannel(sWorld.getGmClientChannel().c_str(), _player);
		if (chn)
		{
			std::stringstream ss;
			ss << "GmTicket:" << GM_TICKET_CHAT_OPCODE_ONLINESTATE;
			ss << ":" << ticket->guid;
			ss << ":1";
			chn->Say(_player, ss.str().c_str(), NULL, true);
		}
	}
#endif

	// disabled useless messages
	//#ifdef WIN32
	//_player->BroadcastMessage("Server: %sArcEmu %s - %s-Windows-%s", MSG_COLOR_WHITE, BUILD_TAG, CONFIG, ARCH);
	//#else
	//_player->BroadcastMessage("Server: %sArcEmu %s - %s-%s", MSG_COLOR_WHITE, BUILD_TAG, PLATFORM_TEXT, ARCH);
	//#endif

	// Revision
	//_player->BroadcastMessage("Build hash: %s%s", MSG_COLOR_CYAN, BUILD_HASH_STR);
	// Bugs
	//_player->BroadcastMessage("Bugs: %s%s", MSG_COLOR_SEXHOTPINK, BUGTRACKER);
	// Recruiting message
	//_player->BroadcastMessage(RECRUITING);
	// Shows Online players, and connection peak
	//_player->BroadcastMessage("Online Players: %s%u |rPeak: %s%u|r Accepted Connections: %s%u",
	//                          MSG_COLOR_SEXGREEN, sWorld.GetSessionCount(), MSG_COLOR_SEXBLUE, sWorld.PeakSessionCount, MSG_COLOR_SEXBLUE, sWorld.mAcceptedConnections);

	// Shows Server uptime
	//_player->BroadcastMessage("Server Uptime: |r%s", sWorld.GetUptimeString().c_str());

	// server Message Of The Day
	SendMOTD();

	// disabled for now
	//Set current RestState
	//if(plr->m_isResting)
	// We are resting at an inn , turn on Zzz
	//plr->ApplyPlayerRestState(true);

	// disable everything that is not useful in a sandbox :D
	//Calculate rest bonus if there is time between lastlogoff and now
	/*if(plr->m_timeLogoff > 0 && plr->getLevel() < plr->GetMaxLevel())	// if timelogoff = 0 then it's the first login
	{
	uint32 currenttime = uint32(UNIXTIME);
	uint32 timediff = currenttime - plr->m_timeLogoff;

	//Calculate rest bonus
	if(timediff > 0)
	plr->AddCalculatedRestXP(timediff);
	}

	if(info->m_Group)
	info->m_Group->Update();*/

	if (enter_world && !_player->GetMapMgr())
		plr->AddToWorld();

	sHookInterface.OnFullLogin(_player);

	objmgr.AddPlayer(_player);
}

bool ChatHandler::HandleRenameCommand(const char* args, WorldSession* m_session)
{
	// prevent buffer overflow
	if (strlen(args) > 100)
		return false;

	char name1[100];
	char name2[100];

	if (sscanf(args, "%s %s", name1, name2) != 2)
		return false;

	if (VerifyName(name2, strlen(name2)) != E_CHAR_NAME_SUCCESS)
	{
		RedSystemMessage(m_session, "That name is invalid or contains invalid characters.");
		return true;
	}

	string new_name = name2;
	PlayerInfo* pi = objmgr.GetPlayerInfoByName(name1);
	if (pi == 0)
	{
		RedSystemMessage(m_session, "Player not found with this name.");
		return true;
	}

	if (objmgr.GetPlayerInfoByName(new_name.c_str()) != NULL)
	{
		RedSystemMessage(m_session, "Player found with this name in use already.");
		return true;
	}

	objmgr.RenamePlayerInfo(pi, pi->name, new_name.c_str());

	free(pi->name);
	pi->name = strdup(new_name.c_str());

	// look in world for him
	Player* plr = objmgr.GetPlayer(pi->guid);
	if (plr != 0)
	{
		plr->SetName(new_name);
		BlueSystemMessageToPlr(plr, "%s changed your name to '%s'.", m_session->GetPlayer()->GetName(), new_name.c_str());
		plr->SaveToDB(false);
	}
	else
	{
		CharacterDatabase.WaitExecute("UPDATE characters SET name = '%s' WHERE guid = %u", CharacterDatabase.EscapeString(new_name).c_str(), (uint32)pi->guid);
	}

	GreenSystemMessage(m_session, "Changed name of '%s' to '%s'.", name1, name2);
	sGMLog.writefromsession(m_session, "renamed character %s (GUID: %u) to %s", name1, pi->guid, name2);
	sPlrLog.writefromsession(m_session, "GM renamed character %s (GUID: %u) to %s", name1, pi->guid, name2);
	return true;
}

// We do not do anything with this opcode...
void WorldSession::HandleLoadScreenOpcode(WorldPacket & recv_data)
{
	// empty opcode
	// printf("LOAD SCREEN OPCODE\n");
	uint32 mapId;

	recv_data >> mapId;
	recv_data.ReadBit();
}

void WorldSession::HandleRandomizeCharNameOpcode(WorldPacket & recv_data)
{
	uint8 gender, race;

	recv_data >> race;
	recv_data >> gender;	

	WorldPacket data(SMSG_RANDOMIZE_CHAR_NAME, 10);
	data.WriteBit(0);     //////  This isn't correct
	SendPacket(&data);    //////  but she works! :)
}