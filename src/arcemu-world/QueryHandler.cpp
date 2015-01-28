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

//////////////////////////////////////////////////////////////
/// This function handles CMSG_NAME_QUERY:
//////////////////////////////////////////////////////////////
void WorldSession::HandleNameQueryOpcode(WorldPacket & recv_data)
{
	ObjectGuid guid;

	uint8 bit14, bit1C;
	uint32 unk, unk1;

	guid[4] = recv_data.ReadBit();
	bit14 = recv_data.ReadBit();
	guid[6] = recv_data.ReadBit();
	guid[0] = recv_data.ReadBit();
	guid[7] = recv_data.ReadBit();
	guid[1] = recv_data.ReadBit();
	bit1C = recv_data.ReadBit();
	guid[5] = recv_data.ReadBit();
	guid[2] = recv_data.ReadBit();
	guid[3] = recv_data.ReadBit();

	recv_data.ReadByteSeq(guid[7]);
	recv_data.ReadByteSeq(guid[5]);
	recv_data.ReadByteSeq(guid[1]);
	recv_data.ReadByteSeq(guid[2]);
	recv_data.ReadByteSeq(guid[6]);
	recv_data.ReadByteSeq(guid[3]);
	recv_data.ReadByteSeq(guid[0]);
	recv_data.ReadByteSeq(guid[4]);

	// virtual and native realm Addresses

	if (bit14)
		recv_data >> unk;

	if (bit1C)
		recv_data >> unk1;

	SendNameQueryOpcode(guid);
}

void WorldSession::SendNameQueryOpcode(ObjectGuid guid)
{
	PlayerInfo* pn = objmgr.GetPlayerInfo(guid);

	if (!pn)
		return;	

	WorldPacket data(SMSG_NAME_QUERY_RESPONSE, 500);
		
	ObjectGuid guid2 = 0;
	ObjectGuid guid3 = guid;
	
	data.WriteBit(guid[3]);
	data.WriteBit(guid[6]);
	data.WriteBit(guid[7]);
	data.WriteBit(guid[2]);
	data.WriteBit(guid[5]);
	data.WriteBit(guid[4]);
	data.WriteBit(guid[0]);
	data.WriteBit(guid[1]);

	data.WriteByteSeq(guid[5]);
	data.WriteByteSeq(guid[4]);
	data.WriteByteSeq(guid[7]);
	data.WriteByteSeq(guid[6]);
	data.WriteByteSeq(guid[1]);
	data.WriteByteSeq(guid[2]);

	data << uint8(!pn);

	if (pn)
	{
		data << uint32(0); // realmIdSecond
		data << uint32(1); // AccID
		data << uint8(pn->cl);
		data << uint8(pn->race);
		data << uint8(pn->lastLevel);
		data << uint8(pn->gender);
	}

	data.WriteByteSeq(guid[0]);
	data.WriteByteSeq(guid[3]);

	if (!pn)
	{
		SendPacket(&data);
		return;
	}
		data.WriteBit(guid2[2]);
		data.WriteBit(guid2[7]);
		data.WriteBit(guid3[7]);
		data.WriteBit(guid3[2]);
		data.WriteBit(guid3[0]);
		data.WriteBit(0);
		data.WriteBit(guid2[4]);
		data.WriteBit(guid3[5]);
		data.WriteBit(guid2[1]);
		data.WriteBit(guid2[3]);
		data.WriteBit(guid2[0]);

		for (uint8 i = 0; i < 5; ++i)
			data.WriteBits(0, 7);

		data.WriteBit(guid3[6]);
		data.WriteBit(guid3[3]);
		data.WriteBit(guid2[5]);
		data.WriteBit(guid3[1]);
		data.WriteBit(guid3[4]);
		data.WriteBits(strlen(pn->name), 6);
		data.WriteBit(guid2[6]);

		data.FlushBits();

		data.WriteByteSeq(guid3[6]);
		data.WriteByteSeq(guid3[0]);
		data.WriteString((pn->name));
		data.WriteByteSeq(guid2[5]);
		data.WriteByteSeq(guid2[2]);
		data.WriteByteSeq(guid3[3]);
		data.WriteByteSeq(guid2[4]);
		data.WriteByteSeq(guid2[3]);
		data.WriteByteSeq(guid3[4]);
		data.WriteByteSeq(guid3[2]);
		data.WriteByteSeq(guid2[7]);

		data.WriteByteSeq(guid2[6]);
		data.WriteByteSeq(guid3[7]);
		data.WriteByteSeq(guid3[1]);
		data.WriteByteSeq(guid2[1]);
		data.WriteByteSeq(guid3[5]);
		data.WriteByteSeq(guid2[0]);

		SendPacket(&data);
}

//////////////////////////////////////////////////////////////
/// This function handles CMSG_QUERY_TIME:
//////////////////////////////////////////////////////////////
void WorldSession::HandleQueryTimeOpcode(WorldPacket & recv_data)
{

	WorldPacket data(SMSG_QUERY_TIME_RESPONSE, 4 + 4);
	data << (uint32)UNIXTIME;
	data << (uint32)0; //VLack: 3.1; thanks Stewart for reminding me to have the correct structure even if it seems the old one still works.
	SendPacket(&data);

}

//////////////////////////////////////////////////////////////
/// This function handles CMSG_CREATURE_QUERY:
//////////////////////////////////////////////////////////////
void WorldSession::HandleCreatureQueryOpcode(WorldPacket & recv_data)
{
	CHECK_PACKET_SIZE(recv_data, 12);
	//sStackWorldPacket( data, SMSG_CREATURE_QUERY_RESPONSE, sizeof(CreatureInfo) + 250*4 );
	WorldPacket data (SMSG_CREATURE_QUERY_RESPONSE, sizeof(CreatureInfo) + 250*4);
	uint32 entry;
	uint64 guid;
	CreatureInfo *ci;

	recv_data >> entry;
	recv_data >> guid;

	if(entry == 300000)
	{
		data << (uint32)entry;
		data << "WayPoint";
		data << uint8(0) << uint8(0) << uint8(0);
		data << "Level is WayPoint ID";
		for(uint32 i = 0; i < 8;i++)
		{
			data << uint32(0);
		}
		data << uint8(0);  
	}
	else
	{
		ci = CreatureNameStorage.LookupEntry(entry);
		if(ci == NULL)
			return;

		LocalizedCreatureName * lcn = (language>0) ? sLocalizationMgr.GetLocalizedCreatureName(entry, language) : NULL;

		if(lcn == NULL)
		{
			sLog.outDetail("WORLD: CMSG_CREATURE_QUERY '%s'", ci->Name);
			data << (uint32)entry;
			data << ci->Name;
			data << uint16(0);
			data << uint16(0);
			data << uint16(0);
			data << uint8(0); //some str, never seen it non empty atm
			data << ci->SubName;
		}
		else
		{
			sLog.outDetail("WORLD: CMSG_CREATURE_QUERY '%s' (localized to %s)", ci->Name, lcn->Name);
			data << (uint32)entry;
			data << lcn->Name;
			data << uint16(0);
			data << uint16(0);
			data << uint16(0);
			data << uint8(0); //some str, never seen it non empty atm
			data << lcn->SubName;
		}
		data << ci->info_str; //!!! this is a string in 2.3.0 Example: stormwind guard has : "Direction"
		data << ci->Flags1;  
		data << uint32(0);
		data << ci->Type;
		data << ci->Family;
		data << ci->Rank;
		data << ci->killcredit[0];
		data << ci->killcredit[1];
		data << ci->Male_DisplayID;
		data << ci->Female_DisplayID;
		data << ci->Male_DisplayID2;
		data << ci->Female_DisplayID2;
		data << ci->unkfloat1;
		data << ci->unkfloat2;
		data << ci->Leader;
		data << ci->QuestItems[0];
		data << ci->QuestItems[1];
		data << ci->QuestItems[2];
		data << ci->QuestItems[3];
		data << ci->QuestItems[4];
		data << ci->QuestItems[5];
		data << ci->waypointid;
		data << uint32(0);
	}

	SendPacket( &data );
}

//////////////////////////////////////////////////////////////
/// This function handles CMSG_GAMEOBJECT_QUERY:
//////////////////////////////////////////////////////////////
void WorldSession::HandleGameObjectQueryOpcode(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	CHECK_PACKET_SIZE(recv_data, 12);
	WorldPacket data(SMSG_GAMEOBJECT_QUERY_RESPONSE, sizeof(GameObjectInfo) + 250 * 6); // not sure

	uint32 entryID;
	uint64 guid;
	GameObjectInfo* goinfo;


	recv_data >> entryID;
	recv_data >> guid;

	LOG_DETAIL("WORLD: CMSG_GAMEOBJECT_QUERY '%u'", entryID);

	goinfo = GameObjectNameStorage.LookupEntry(entryID);
	if(goinfo == NULL)
		return;

	LocalizedGameObjectName* lgn = (language > 0) ? sLocalizationMgr.GetLocalizedGameObjectName(entryID, language) : NULL;

	data << entryID;                // unique identifier of the GO template
	data << goinfo->Type;           // type of the gameobject
	data << goinfo->DisplayID;      // displayid/modelid of the gameobject

	// Name of the gameobject
	if(lgn)
		data << lgn->Name;
	else
		data << goinfo->Name;

	data << uint8(0);              // name2, always seems to be empty
	data << uint8(0);              // name3, always seems to be empty
	data << uint8(0);              // name4, always seems to be empty
	data << goinfo->Category;       // Category string of the GO, like "attack", "pvp", "point", etc
	data << goinfo->Castbartext;    // text displayed when using the go, like "collecting", "summoning" etc
	data << goinfo->Unkstr;
	data << goinfo->SpellFocus;     // spellfocus id, ex.: spell casted when interacting with the GO
	data << goinfo->sound1;
	data << goinfo->sound2;
	data << goinfo->sound3;
	data << goinfo->sound4;
	data << goinfo->sound5;
	data << goinfo->sound6;
	data << goinfo->sound7;
	data << goinfo->sound8;
	data << goinfo->sound9;
	data << goinfo->Unknown1;
	data << goinfo->Unknown2;
	data << goinfo->Unknown3;
	data << goinfo->Unknown4;
	data << goinfo->Unknown5;
	data << goinfo->Unknown6;
	data << goinfo->Unknown7;
	data << goinfo->Unknown8;
	data << goinfo->Unknown9;
	data << goinfo->Unknown10;
	data << goinfo->Unknown11;
	data << goinfo->Unknown12;
	data << goinfo->Unknown13;
	data << goinfo->Unknown14;
	data << uint32(0);         // 15
	data << uint32(0);         // 16
	data << uint32(0);         // 17
	data << uint32(0);         // 18
	data << uint32(0);         // 19
	data << uint32(0);         // 20
	data << uint32(0);         // 21
	data << uint32(0);         // 22

	data << float(goinfo->Size);       // scaling of the GO

	// questitems that the go can contain
	for(uint32 i = 0; i < 6; ++i)
	{
		data << uint32(goinfo->QuestItems[i]);
	}

	data << uint32(0);

	SendPacket(&data);
}

//////////////////////////////////////////////////////////////
/// This function handles MSG_CORPSE_QUERY:
//////////////////////////////////////////////////////////////
void WorldSession::HandleCorpseQueryOpcode(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	LOG_DETAIL("WORLD: Received MSG_CORPSE_QUERY");

	Corpse* pCorpse;
	WorldPacket data(MSG_CORPSE_QUERY, 25);
	MapInfo* pMapinfo;

	pCorpse = objmgr.GetCorpseByOwner(GetPlayer()->GetLowGUID());
	if(pCorpse)
	{
		pMapinfo = WorldMapInfoStorage.LookupEntry(pCorpse->GetMapId());
		if(pMapinfo)
		{
			if(pMapinfo->type == INSTANCE_NULL || pMapinfo->type == INSTANCE_BATTLEGROUND)
			{
				data << uint8(0x01); //show ?
				data << pCorpse->GetMapId(); // mapid (that tombstones shown on)
				data << pCorpse->GetPositionX();
				data << pCorpse->GetPositionY();
				data << pCorpse->GetPositionZ();
				data << pCorpse->GetMapId(); //instance mapid (needs to be same as mapid to be able to recover corpse)
				data << uint32(0);
				SendPacket(&data);
			}
			else
			{
				data << uint8(0x01); //show ?
				data << pMapinfo->repopmapid; // mapid (that tombstones shown on)
				data << pMapinfo->repopx;
				data << pMapinfo->repopy;
				data << pMapinfo->repopz;
				data << pCorpse->GetMapId(); //instance mapid (needs to be same as mapid to be able to recover corpse)
				data << uint32(0);
				SendPacket(&data);
			}
		}
		else
		{

			data << uint8(0x01); //show ?
			data << pCorpse->GetMapId(); // mapid (that tombstones shown on)
			data << pCorpse->GetPositionX();
			data << pCorpse->GetPositionY();
			data << pCorpse->GetPositionZ();
			data << pCorpse->GetMapId(); //instance mapid (needs to be same as mapid to be able to recover corpse)
			data << uint32(0);
			SendPacket(&data);

		}
	}
}

void WorldSession::HandlePageTextQueryOpcode(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	CHECK_PACKET_SIZE(recv_data, 4);
	uint32 pageid = 0;
	recv_data >> pageid;

	while(pageid)
	{
		ItemPage* page = ItemPageStorage.LookupEntry(pageid);
		if(!page)
			return;

		LocalizedItemPage* lpi = (language > 0) ? sLocalizationMgr.GetLocalizedItemPage(pageid, language) : NULL;
		WorldPacket data(SMSG_PAGE_TEXT_QUERY_RESPONSE, 1000);
		data << pageid;
		if(lpi)
			data << lpi->Text;
		else
			data << page->text;

		data << page->next_page;
		pageid = page->next_page;
		SendPacket(&data);
	}
}
//////////////////////////////////////////////////////////////
/// This function handles CMSG_ITEM_NAME_QUERY:
//////////////////////////////////////////////////////////////
void WorldSession::HandleItemNameQueryOpcode(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN

	CHECK_PACKET_SIZE(recv_data, 4);
	WorldPacket reply(SMSG_ITEM_NAME_QUERY_RESPONSE, 100);
	uint32 itemid;
	recv_data >> itemid;
	reply << itemid;
	ItemPrototype* proto = ItemPrototypeStorage.LookupEntry(itemid);
	ItemName* MetaName = ItemNameStorage.LookupEntry(itemid);
	if(!proto && !MetaName)
		reply << "Unknown Item";
	else
	{
		if(proto)
		{
			LocalizedItem* li = (language > 0) ? sLocalizationMgr.GetLocalizedItem(itemid, language) : NULL;
			if(li)
				reply << li->Name;
			else
				reply << proto->Name1;

			reply << proto->InventoryType;
		}
		else
		{
			reply << MetaName->name;
			reply << MetaName->slot;
		}

	}

	SendPacket(&reply);
}

void WorldSession::HandleInrangeQuestgiverQuery(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN;

	WorldPacket data(SMSG_QUESTGIVER_STATUS_MULTIPLE, 1000);
	Object::InRangeSet::iterator itr;
	Creature* pCreature;
	uint32 count = 0;
	data << count;

	// 32 count
	// <foreach count>
	//    64 guid
	//    8 status

	for(itr = _player->m_objectsInRange.begin(); itr != _player->m_objectsInRange.end(); ++itr)
	{
		if(!(*itr)->IsCreature())
			continue;

		pCreature = TO_CREATURE(*itr);

		if(pCreature->isQuestGiver())
		{
			data << pCreature->GetGUID();
			data << uint8(sQuestMgr.CalcStatus(pCreature, _player));
			++count;
		}
	}

	*(uint32*)(data.contents()) = count;
	SendPacket(&data);
}

void WorldSession::HandleAchievmentQueryOpcode(WorldPacket & recv_data)
{
	CHECK_INWORLD_RETURN;

	uint64 guid = recv_data.unpackGUID(); // Get the inspectee's GUID
	Player* pTarget = objmgr.GetPlayer((uint32)guid);
	if(!pTarget)
	{
		return;
	}
#ifdef ENABLE_ACHIEVEMENTS
	pTarget->GetAchievementMgr().SendAllAchievementData(GetPlayer());
#endif
}
