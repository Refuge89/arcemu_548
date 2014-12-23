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
// Class WorldSocket - Main network code functions, handles
// reading/writing of all packets.

//!!! todo: cleanup in packets, remove useless comments

#include "StdAfx.h"
#include "AuthCodes.h"


#define ECHO_PACKET_LOG_TO_CONSOLE 1

#pragma pack(push, 1)

struct AuthPacketHeader
{
	AuthPacketHeader(uint32 size, uint32 opcode) : len(0), server(true)
	{
		if (size > 0x7FFF)
			raw[len++] = 0x80 | (0xFF & (size >> 16));
			raw[len++] = 0xFF & size;
			raw[len++] = 0xFF & (size >> 8);
			raw[len++] = 0xFF & opcode;
			raw[len++] = 0xFF & (opcode >> 8);
	}

	uint32 getOpcode()
	{
		uint32 opcode = 0;
		if (server)
		{
			uint8 length = len;
			opcode = uint32(raw[--length]) << 8;
			opcode |= uint32(raw[--length]);
		}
		else
		{
			opcode |= uint32(raw[2] & 0xFF) << 24;
			opcode |= uint32(raw[3] & 0xFF) << 16;
			opcode |= uint32(raw[4] & 0xFF) << 8;
			opcode |= uint32(raw[5] & 0xFF);
		}
		return opcode;
	}

	uint32 getSize()
	{
		uint32 size = 0;
		if (server)
		{
			uint8 length = 0;
			if (raw[length] & 0x80)
				size |= uint32(raw[length++] & 0x7F) << 16;
			size |= uint32(raw[length++] & 0xFF) << 8;
			size |= uint32(raw[length] & 0xFF);
		}
		else
		{
			size |= uint32(raw[0] & 0xFF) << 8;
			size |= uint32(raw[1] & 0xFF);
		}
		return size;
	}

	bool server;
	uint8 len;
	uint8 raw[6];
};

struct WorldPacketHeader
{
	WorldPacketHeader(uint32 size, uint32 opcode) : raw(((size) << 13) | (opcode & 0x1FFF)) {}
	uint16 getOpcode() { return uint16(raw & 0x1FFF); };
	uint32 getSize() { return (raw >> 13); };

	uint8 header[4];
	uint32 raw;
};

struct ClientPktHeader
{
	uint16 size;
	uint32 cmd;
};

#pragma pack(pop)

WorldSocket::WorldSocket(SOCKET fd)
	:
	Socket(fd, sWorld.SocketSendBufSize, sWorld.SocketRecvBufSize),
	Authed(false),
	mOpcode(0),
	mRemaining(0),
	mSize(0),
	mSeed(RandomUInt()),
	mRequestID(0),
	mSession(NULL),
	pAuthenticationPacket(NULL),
	_latency(0),
	mQueued(false),
	m_nagleEanbled(false),
	m_fullAccountName(NULL)
{

}

WorldSocket::~WorldSocket()
{
	WorldPacket* pck;
	queueLock.Acquire();
	while ((pck = _queue.Pop()) != NULL)
	{
		delete pck;
	}
	queueLock.Release();

	if (pAuthenticationPacket)
		delete pAuthenticationPacket;

	if (mSession)
	{
		mSession->SetSocket(NULL);
		mSession = NULL;
	}

	if (m_fullAccountName != NULL)
	{
		delete m_fullAccountName;
		m_fullAccountName = NULL;
	}
}

void WorldSocket::OnDisconnect()
{
	if (mSession)
	{
		mSession->SetSocket(0);
		mSession = NULL;
	}

	if (mRequestID != 0)
	{
		sLogonCommHandler.UnauthedSocketClose(mRequestID);
		mRequestID = 0;
	}

	if (mQueued)
	{
		sWorld.RemoveQueuedSocket(this);	// Remove from queued sockets.
		mQueued = false;
	}
}

void WorldSocket::OutPacket(uint32 opcode, size_t len, const void* data)
{
	OUTPACKET_RESULT res;
	if ((len + 10) > WORLDSOCKET_SENDBUF_SIZE)
	{
		LOG_ERROR("WARNING: Tried to send a packet of %u bytes (which is too large) to a socket. Opcode was: %u (0x%03X)", (unsigned int)len, (unsigned int)opcode, (unsigned int)opcode);
		return;
	}

	res = _OutPacket(opcode, len, data);
	if (res == OUTPACKET_RESULT_SUCCESS)
		return;

	if (res == OUTPACKET_RESULT_NO_ROOM_IN_BUFFER)
	{
		/* queue the packet */
		queueLock.Acquire();
		WorldPacket* pck = new WorldPacket(opcode, len);
		if (len) pck->append((const uint8*)data, len);
		_queue.Push(pck);
		queueLock.Release();
	}
}

void WorldSocket::UpdateQueuedPackets()
{
	queueLock.Acquire();
	if (!_queue.HasItems())
	{
		queueLock.Release();
		return;
	}

	WorldPacket* pck;
	while ((pck = _queue.front()) != NULL)
	{
		/* try to push out as many as you can */
		switch (_OutPacket(pck->GetOpcode(), pck->size(), pck->size() ? pck->contents() : NULL))
		{
		case OUTPACKET_RESULT_SUCCESS:
		{
			delete pck;
			_queue.pop_front();
		}
		break;

		case OUTPACKET_RESULT_NO_ROOM_IN_BUFFER:
		{
			/* still connected */
			queueLock.Release();
			return;
		}
		break;

		default:
		{
			/* kill everything in the buffer */
			while ((pck == _queue.Pop()) != 0)
			{
				delete pck;
			}
			queueLock.Release();
			return;
		}
		break;
		}
	}
	queueLock.Release();
}

OUTPACKET_RESULT WorldSocket::_OutPacket(uint32 opcode, size_t len, const void* data)
{	
	bool rv;
	if (!IsConnected())
		return OUTPACKET_RESULT_NOT_CONNECTED;

	BurstBegin();
	//if((m_writeByteCount + len + 4) >= m_writeBufferSize)
	if (writeBuffer.GetSpace() < (len + 4))
	{
		BurstEnd();
		return OUTPACKET_RESULT_NO_ROOM_IN_BUFFER;
	}

	// Packet logger :)
	sWorldLog.LogPacket((uint32)len, opcode, (const uint8*)data, 1, (mSession ? mSession->GetAccountId() : 0));

	if (_crypt.IsInitialized())
		 {
			 WorldPacketHeader header(len, opcode);
			_crypt.EncryptSend(((uint8*)&header.raw), 4);
			rv = BurstSend((const uint8*)&header.raw, 4);
		}
	else
		 {
			AuthPacketHeader header(len + 2, opcode);
			rv = BurstSend((const uint8*)&header.raw, header.len);
		}

	// Pass the rest of the packet to our send buffer (if there is any)
	if (len > 0 && rv)
	{
		rv = BurstSend((const uint8*)data, (uint32)len);
	}

	if (rv) BurstPush();
	BurstEnd();
	return rv ? OUTPACKET_RESULT_SUCCESS : OUTPACKET_RESULT_SOCKET_ERROR;
}

void WorldSocket::OnConnect()
{
	sWorld.mAcceptedConnections++;
	_latency = getMSTime();

	WorldPacket packet(MSG_WOW_CONNECTION, 46);
	packet << "RLD OF WARCRAFT CONNECTION - SERVER TO CLIENT";
	SendPacket(&packet);
}

void WorldSocket::OnConnectTwo()
{
	WorldPacket packet(SMSG_AUTH_CHALLENGE, 37);

	packet << uint16(0);

	for (int i = 0; i < 8; i++)
		packet << uint32(0);

	packet << uint8(1);
	packet << uint32(mSeed);

	SendPacket(&packet);
}

void WorldSocket::_HandleAuthSession(WorldPacket* recvPacket)
{
    WorldPacket addonsData;

    recvPacket->read<uint32>();
    recvPacket->read<uint32>();
	*recvPacket >> AuthDigest[18];
	*recvPacket >> AuthDigest[14];
	*recvPacket >> AuthDigest[3];
	*recvPacket >> AuthDigest[4];
	*recvPacket >> AuthDigest[0];
    recvPacket->read<uint32>();
	*recvPacket >> AuthDigest[11];
    *recvPacket >> mClientSeed;
	*recvPacket >> AuthDigest[19];
    recvPacket->read<uint8>();
    recvPacket->read<uint8>();
	*recvPacket >> AuthDigest[2];
	*recvPacket >> AuthDigest[9];
	*recvPacket >> AuthDigest[12];
    recvPacket->read<uint64>();
    recvPacket->read<uint32>();
	*recvPacket >> AuthDigest[16];
	*recvPacket >> AuthDigest[5];
	*recvPacket >> AuthDigest[6];
	*recvPacket >> AuthDigest[8];
	mClientBuild = recvPacket->read<uint16>();
	*recvPacket >> AuthDigest[17];
	*recvPacket >> AuthDigest[7];
	*recvPacket >> AuthDigest[13];
	*recvPacket >> AuthDigest[15];
	*recvPacket >> AuthDigest[1];
	*recvPacket >> AuthDigest[10];
    *recvPacket >> addonSize;

	addonsData.resize(addonSize);
	recvPacket->read((uint8*)addonsData.contents(), addonSize);

	recvPacket->ReadBit();
	uint32 accountNameLength = recvPacket->ReadBits(11);

	account = recvPacket->ReadString(accountNameLength);

	// Send out a request for this account.
	mRequestID = sLogonCommHandler.ClientConnected(account, this);

	if (mRequestID == 0xFFFFFFFF)
	{
		Disconnect();
		return;
	}

	// shitty hash !
	m_fullAccountName = new string(account);

	// Set the authentication packet
	pAuthenticationPacket = recvPacket;
}

void WorldSocket::InformationRetreiveCallback(WorldPacket & recvData, uint32 requestid)
{
	if (requestid != mRequestID)
		return;

	uint32 error;
	recvData >> error;

	if (error != 0 || pAuthenticationPacket == NULL)
	{
		LOG_ERROR("Something happened wrong @ the logon server\n");
		// something happened wrong @ the logon server
		//OutPacket(SMSG_AUTH_RESPONSE, 1, "\x0D");
		SendAuthResponseError(AUTH_FAILED);
		return;
	}

	// Extract account information from the packet.
	string AccountName;
	const string* ForcedPermissions;
	uint32 AccountID;
	string GMFlags;
	uint8 AccountFlags;
	string lang = "enUS";
	uint32 i;

	recvData >> AccountID >> AccountName >> GMFlags >> AccountFlags;

	ForcedPermissions = sLogonCommHandler.GetForcedPermissions(AccountName);
	if (ForcedPermissions != NULL)
		GMFlags.assign(ForcedPermissions->c_str());

	mRequestID = 0;

	// Pull the sessionkey we generated during the logon - client handshake
	uint8 K[40];
	recvData.read(K, 40);
	_crypt.Init(K);

	BigNumber BNK;
	BNK.SetBinary(K, 40);

	//checking if player is already connected
	//disconnect current player and login this one(blizzlike)

	if(recvData.rpos() != recvData.wpos())
		recvData.read((uint8*)lang.data(), 4);

	WorldSession* session = sWorld.FindSession(AccountID);
	if (session)
	{
		// AUTH_FAILED = 0x0D
		session->Disconnect();

		// clear the logout timer so he times out straight away
		session->SetLogoutTimer(1);

		// we must send authentication failed here.
		// the stupid newb can relog his client.
		// otherwise accounts dupe up and disasters happen.
		//OutPacket(SMSG_AUTH_RESPONSE, 1, "\x15");
		//printf("Auth failed\n");
		SendAuthResponseError(AUTH_UNKNOWN_ACCOUNT); // auth failed
		return;
	}

	Sha1Hash sha;

	uint32 t = 0;
	if (m_fullAccountName == NULL)
	{
		sha.UpdateData(AccountName);
	}
	else
	{
		sha.UpdateData(*m_fullAccountName);

		// this is unused now. we may as well free up the memory.
		delete m_fullAccountName;
		m_fullAccountName = NULL;
	}

	sha.UpdateData((uint8*)&t, 4);
	sha.UpdateData((uint8*)&mClientSeed, 4);
	sha.UpdateData((uint8*)&mSeed, 4);
	sha.UpdateData((uint8*)&K, 40);
	sha.Finalize();
		

	if (memcmp(sha.GetDigest(), AuthDigest, 20))
	{	
		SendAuthResponseError(AUTH_UNKNOWN_ACCOUNT);
		return;
	}

	// Allocate session
	WorldSession* pSession = new WorldSession(AccountID, AccountName, this);
	mSession = pSession;
	ARCEMU_ASSERT(mSession != NULL);
	// aquire delete mutex
	pSession->deleteMutex.Acquire();

	// Set session properties
	pSession->SetClientBuild(mClientBuild);
	pSession->LoadSecurity(GMFlags);
	pSession->SetAccountFlags(AccountFlags);
	pSession->m_lastPing = (uint32)UNIXTIME;
	pSession->language = sLocalizationMgr.GetLanguageId(lang);

	//if(recvData.rpos() != recvData.wpos())
	//recvData >> pSession->m_muted;

	for (i = 0; i < 8; ++i)
		pSession->SetAccountData(i, NULL, true, 0);

	if (sWorld.m_useAccountData)
	{
		QueryResult* pResult = CharacterDatabase.Query("SELECT * FROM account_data WHERE acct = %u", AccountID);
		if (pResult == NULL)
			CharacterDatabase.Execute("INSERT INTO account_data VALUES(%u, '', '', '', '', '', '', '', '', '')", AccountID);
		else
		{
			size_t len;
			const char* data;
			char* d;
			for (i = 0; i < 8; ++i)
			{
				data = pResult->Fetch()[1 + i].GetString();
				len = data ? strlen(data) : 0;
				if (len > 1)
				{
					d = new char[len + 1];
					memcpy(d, data, len + 1);
					pSession->SetAccountData(i, d, true, (uint32)len);
				}
			}

			delete pResult;
		}
	}
	Log.Debug("Auth", "%s from %s:%u [%ums]", AccountName.c_str(), GetRemoteIP().c_str(), GetRemotePort(), _latency);

#ifdef SESSION_CAP
	if (sWorld.GetSessionCount() >= SESSION_CAP)
	{
		//OutPacket(SMSG_AUTH_RESPONSE, 1, "\x0D");
		SendAuthResponseError(AUTH_FAILED);
		Disconnect();
		return;
	}
#endif

	// Check for queue.
	uint32 playerLimit = sWorld.GetPlayerLimit();
	if ((sWorld.GetSessionCount() < playerLimit) || pSession->HasGMPermissions())
	{
		Authenticate();
	}
	else if (playerLimit > 0)
	{
		// Queued, sucker.
		uint32 Position = sWorld.AddQueuedSocket(this);
		mQueued = true;
		Log.Debug("Queue", "%s added to queue in position %u", AccountName.c_str(), Position);

		// Send packet so we know what we're doing
		SendAuthResponse(AUTH_WAIT_QUEUE, true, Position);
	}
	else
	{
		SendAuthResponseError(AUTH_REJECT);
		//OutPacket(SMSG_AUTH_RESPONSE, 1, "\x0E"); // AUTH_REJECT = 14
		Disconnect();
	}

	// release delete mutex
	pSession->deleteMutex.Release();

}

void WorldSocket::Authenticate()
{
	ARCEMU_ASSERT(pAuthenticationPacket != NULL);
	mQueued = false;

	if (mSession == NULL)
		return;

	SendAuthResponse(AUTH_OK, false, NULL);

	WorldPacket cdata(SMSG_CLIENTCACHE_VERSION, 4);
	cdata << uint32(18414);
	SendPacket(&cdata);

	//sAddonMgr.SendAddonInfoPacket(pAuthenticationPacket, static_cast< uint32 >(pAuthenticationPacket->rpos()), mSession);
	mSession->_latency = _latency;

	delete pAuthenticationPacket;
	pAuthenticationPacket = NULL;

	sWorld.AddSession(mSession);
	sWorld.AddGlobalSession(mSession);

	if (mSession->HasGMPermissions())
		sWorld.gmList.insert(mSession);
}

void WorldSocket::SendAuthResponse(uint8 code, bool queued, uint32 queuePos)
{
	WorldPacket data(SMSG_AUTH_RESPONSE, 80);

	QueryResult* classResult = CharacterDatabase.Query("SELECT class, expansion FROM realm_classes WHERE realmId = %u", 1);
	QueryResult* raceResult = CharacterDatabase.Query("SELECT race, expansion FROM realm_races WHERE realmId = %u", 1);

	if (!classResult || !raceResult)
	{
		LOG_ERROR("Unable to retrieve class or race data. Make sure you applied the Auth_Update SQL");
		return;
	}

	data.WriteBit(code == AUTH_OK);

	if (code == AUTH_OK)
	{
		data.WriteBits(0, 21);
		
		data.WriteBits(classResult->GetRowCount(), 23);
		data.WriteBits(0, 21);
		data.WriteBit(0);
		data.WriteBit(0);
		data.WriteBit(0);
		data.WriteBit(0);
		data.WriteBits(raceResult->GetRowCount(), 23);
		data.WriteBit(0);
	}

	data.WriteBit(queued);

	if (queued)
		data.WriteBit(1);

	data.FlushBits();

	if (queued)
		data << uint32(0);

	if (code == AUTH_OK)
	{
		do
		{
			Field* fields = raceResult->Fetch();

			data << fields[1].GetUInt8();
			data << fields[0].GetUInt8();
		} while (raceResult->NextRow());

		do
		{
			Field* fields = classResult->Fetch();

			data << fields[1].GetUInt8();
			data << fields[0].GetUInt8();
		} while (classResult->NextRow());

		data << uint32(0);
		data << uint8(4);
		data << uint32(4);
		data << uint32(0);
		data << uint8(4);
		data << uint32(0);
		data << uint32(0);
		data << uint32(0);
	}

	data << uint8(code);

	SendPacket(&data);
}

void WorldSocket::_HandlePing(WorldPacket* recvPacket)
{
	uint32 ping;
	if (recvPacket->size() < 4)
	{
		LOG_ERROR("Socket closed due to incomplete ping packet.");
		Disconnect();
		return;
	}

	*recvPacket >> _latency;
	*recvPacket >> ping;

	if (mSession)
	{
		mSession->_latency = _latency;
		mSession->m_lastPing = (uint32)UNIXTIME;

		// reset the move time diff calculator, don't worry it will be re-calculated next movement packet.
		mSession->m_clientTimeDelay = 0;
	}

	OutPacket(SMSG_PONG, 4, &ping);

#ifdef WIN32
	// Dynamically change nagle buffering status based on latency.
	//if(_latency >= 250)
	// I think 350 is better, in a MMO 350 latency isn't that big that we need to worry about reducing the number of packets being sent.
	if (_latency >= 350)
	{
		if (!m_nagleEanbled)
		{
			u_long arg = 0;
			setsockopt(GetFd(), 0x6, 0x1, (const char*)&arg, sizeof(arg));
			m_nagleEanbled = true;
		}
	}
	else
	{
		if (m_nagleEanbled)
		{
			u_long arg = 1;
			setsockopt(GetFd(), 0x6, 0x1, (const char*)&arg, sizeof(arg));
			m_nagleEanbled = false;
		}
	}
#endif
}

void WorldSocket::OnRead()
{
	for (;;)
	{
		if (mRemaining == 0)
		{
			if (_crypt.IsInitialized())
			{
				if (readBuffer.GetSize() < 4)
				{
					return;
				}

				WorldPacketHeader Header(0, 0);
				readBuffer.Read((uint8*)&Header.raw, 4);
				_crypt.DecryptRecv((uint8*)&Header.raw, 4);

				mRemaining = mSize = Header.getSize();
				mOpcode = Header.getOpcode();
			}
			else
			{
				if (readBuffer.GetSize() < 6)
				{
					return;
				}

				ClientPktHeader Header;
				readBuffer.Read((uint8*)&Header, 6);
				_crypt.DecryptRecv((uint8*)&Header, sizeof(ClientPktHeader));
				
				mRemaining = mSize = Header.size -= 4;
				mOpcode = Header.cmd;
			}
		}

		WorldPacket* Packet;

		if (mRemaining > 0)
		{
			if (readBuffer.GetSize() < mRemaining)
			{
				return;
			}
		}

		Packet = new WorldPacket(static_cast<uint16>(mOpcode), mSize);
		Packet->resize(mSize);

		if (mRemaining > 0)
		{
			// Copy from packet buffer into our actual buffer.
			///Read(mRemaining, (uint8*)Packet->contents());
			readBuffer.Read((uint8*)Packet->contents(), mRemaining);
		}

		sWorldLog.LogPacket(mSize, static_cast<uint16>(mOpcode), mSize ? Packet->contents() : NULL, 0, (mSession ? mSession->GetAccountId() : 0));
		mRemaining = mSize = mOpcode = 0;
		// Check for packets that we handle
		switch (Packet->GetOpcode())
		{
		case CMSG_PING:
		{
			_HandlePing(Packet);
			delete Packet;
		}
		break;
		case MSG_WOW_CONNECTION:
		{
			//printf("Client to server\n");
			HandleWoWConnection(Packet);      // new 4.x
		}
		break;
		case CMSG_AUTH_SESSION:
		{
			_HandleAuthSession(Packet);
		}
		break;
		default:
		{
			if (mSession) mSession->QueuePacket(Packet);
			else delete Packet;
		}
		break;
		}
	}
}


void WorldSocket::HandleWoWConnection(WorldPacket* recvPacket)
{
	string ClientToServerMsg;

	*recvPacket >> ClientToServerMsg;

	if (ClientToServerMsg != "D OF WARCRAFT CONNECTION - CLIENT TO SERVER")
	{
		LOG_ERROR("Wasn't as expected, returning... \n");
		return;
	}
	else
	{
		// cause we love having a hundred functions
		OnConnectTwo();
	}
}

void WorldSocket::SendAuthResponseError(uint8 code)
{
	WorldPacket packet(SMSG_AUTH_RESPONSE, 1);
	packet.WriteBit(0);      // has queue info
	packet.WriteBit(0);      // has account info
	packet << uint8(code);   // the error code

	SendPacket(&packet);
}


void WorldLog::LogPacket(uint32 len, uint32 opcode, const uint8* data, uint8 direction, uint32 accountid)
{
#ifdef ECHO_PACKET_LOG_TO_CONSOLE
	sLog.outString("[%s]: %s %s (0x%03X) of %u bytes.", direction ? "SERVER" : "CLIENT", direction ? "sent" : "received",
		LookupName(opcode, g_worldOpcodeNames), opcode, len);
#endif

	if (bEnabled)
	{
		mutex.Acquire();
		unsigned int line = 1;
		unsigned int countpos = 0;
		uint16 lenght = static_cast<uint16>(len);
		unsigned int count = 0;

		fprintf(m_file, "{%s} Packet: (0x%04X) %s PacketSize = %u stamp = %u accountid = %u\n", (direction ? "SERVER" : "CLIENT"), opcode,
			LookupName(opcode, g_worldOpcodeNames), lenght, getMSTime(), accountid);
		fprintf(m_file, "|------------------------------------------------|----------------|\n");
		fprintf(m_file, "|00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F |0123456789ABCDEF|\n");
		fprintf(m_file, "|------------------------------------------------|----------------|\n");

		if (lenght > 0)
		{
			fprintf(m_file, "|");
			for (count = 0; count < lenght; count++)
			{
				if (countpos == 16)
				{
					countpos = 0;

					fprintf(m_file, "|");

					for (unsigned int a = count - 16; a < count; a++)
					{
						if ((data[a] < 32) || (data[a] > 126))
							fprintf(m_file, ".");
						else
							fprintf(m_file, "%c", data[a]);
					}

					fprintf(m_file, "|\n");

					line++;
					fprintf(m_file, "|");
				}

				fprintf(m_file, "%02X ", data[count]);

				//FIX TO PARSE PACKETS WITH LENGTH < OR = TO 16 BYTES.
				if (count + 1 == lenght && lenght <= 16)
				{
					for (unsigned int b = countpos + 1; b < 16; b++)
						fprintf(m_file, "   ");

					fprintf(m_file, "|");

					for (unsigned int a = 0; a < lenght; a++)
					{
						if ((data[a] < 32) || (data[a] > 126))
							fprintf(m_file, ".");
						else
							fprintf(m_file, "%c", data[a]);
					}

					for (unsigned int c = count; c < 15; c++)
						fprintf(m_file, " ");

					fprintf(m_file, "|\n");
				}

				//FIX TO PARSE THE LAST LINE OF THE PACKETS WHEN THE LENGTH IS > 16 AND ITS IN THE LAST LINE.
				if (count + 1 == lenght && lenght > 16)
				{
					for (unsigned int b = countpos + 1; b < 16; b++)
						fprintf(m_file, "   ");

					fprintf(m_file, "|");

					unsigned short print = 0;

					for (unsigned int a = line * 16 - 16; a < lenght; a++)
					{
						if ((data[a] < 32) || (data[a] > 126))
							fprintf(m_file, ".");
						else
							fprintf(m_file, "%c", data[a]);

						print++;
					}

					for (unsigned int c = print; c < 16; c++)
						fprintf(m_file, " ");

					fprintf(m_file, "|\n");
				}

				countpos++;
			}
		}
		fprintf(m_file, "-------------------------------------------------------------------\n\n");
		fflush(m_file);
		mutex.Release();
	}
}