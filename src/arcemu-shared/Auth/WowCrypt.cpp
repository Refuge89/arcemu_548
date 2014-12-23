/*
* ArcEmu MMORPG Server
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

#include "WowCrypt.h"

#include <algorithm>

#include <openssl/hmac.h>


WowCrypt::WowCrypt()
{
	m_initialized = false;
}


void WowCrypt::Init(uint8* K)
{
	uint8 r[16] = { 0x08, 0xF1, 0x95, 0x9F, 0x47, 0xE5, 0xD2, 0xDB, 0xA1, 0x3D, 0x77, 0x8F, 0x3F, 0x3E, 0xE7, 0x00 };
	uint8 s[16] = { 0x40, 0xAA, 0xD3, 0x92, 0x26, 0x71, 0x43, 0x47, 0x3A, 0x31, 0x08, 0xA6, 0xE7, 0xDC, 0x98, 0x2A };
	uint8 encryptHash[SHA_DIGEST_LENGTH];
	uint8 decryptHash[SHA_DIGEST_LENGTH];
	uint8 pass[1024];
	uint32 md_len;

	// generate c->s key
	HMAC(EVP_sha1(), s, 16, K, 40, decryptHash, &md_len);
	assert(md_len == SHA_DIGEST_LENGTH);

	// generate s->c key
	HMAC(EVP_sha1(), r, 16, K, 40, encryptHash, &md_len);
	assert(md_len == SHA_DIGEST_LENGTH);

	// initialize rc4 structs
	RC4_set_key(&m_clientDecrypt, SHA_DIGEST_LENGTH, decryptHash);
	RC4_set_key(&m_serverEncrypt, SHA_DIGEST_LENGTH, encryptHash);

	// initial encryption pass -- this is just to get key position,
	// the data doesn't actually have to be initialized as discovered
	// by client debugging.
	RC4(&m_serverEncrypt, 1024, pass, pass);
	RC4(&m_clientDecrypt, 1024, pass, pass);
	m_initialized = true;
}

WowCrypt::~WowCrypt()
{

}