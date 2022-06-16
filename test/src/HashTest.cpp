///////////////////////////////////////////////////////////////////////////////
// MuldeR's Utilities for Qt
// Copyright (C) 2004-2022 LoRd_MuldeR <MuldeR2@GMX.de>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
//
// http://www.gnu.org/licenses/lgpl-2.1.txt
//////////////////////////////////////////////////////////////////////////////////

#include "MUtilsTest.h"

//MUtils
#include <MUtils/Hash.h>

//Qt
#include <QSet>

//===========================================================================
// HELPER METHODS
//===========================================================================

static _inline void fisherYatesShuffle(QByteArray &arr)
{
	char *const ptr = arr.data();
	for (int k = arr.size() - 1; k > 0; k--)
	{
		qSwap(ptr[k], ptr[qrand() % (k + 1)]);
	}
}

//===========================================================================
// TESTBED CLASS
//===========================================================================

class HashTest : public Testbed
{
protected:
	virtual void SetUp()
	{
	}

	virtual void TearDown()
	{
	}
};

//===========================================================================
// TEST METHODS
//===========================================================================

static const char *const TEST_MESSAGE_ORG = "The quick brown fox jumps over the lazy dog";
static const char *const TEST_MESSAGE_ALT = "The quick brown fox jumps over the lazy fog";

static const char* const SEED_KEY = "S73_iT6BTdgNc?kL";

#define TEST_HASH_DIRECT(ID, INPUT, DIGEST) do \
{ \
	QScopedPointer<MUtils::Hash::Hash> test_1(MUtils::Hash::create(MUtils::Hash::HASH_##ID)); \
	QScopedPointer<MUtils::Hash::Hash> test_2(MUtils::Hash::create(MUtils::Hash::HASH_##ID, SEED_KEY)); \
	ASSERT_TRUE(test_1->update(QByteArray(INPUT))); \
	ASSERT_TRUE(test_2->update(QByteArray(INPUT))); \
	const QByteArray result_1 = test_1->digest(); \
	const QByteArray result_2 = test_2->digest(); \
	ASSERT_STRCASENE(result_1.constData(), result_2.constData()); \
	ASSERT_STRCASEEQ(result_1.constData(), (DIGEST)); \
} \
while(0)

#define TEST_HASH_FILEIO(ID, INPUT, DIGEST) do \
{ \
	const QString workDir = makeTempFolder(__FUNCTION__); \
	ASSERT_FALSE(workDir.isEmpty()); \
	const QString fileName = QString("%1/message.txt").arg(workDir); \
	{ \
		QFile file(fileName); \
		ASSERT_TRUE(file.open(QIODevice::WriteOnly | QIODevice::Truncate)); \
		file.write((INPUT), qstrlen((INPUT))); \
		file.close(); \
	} \
	{ \
		QScopedPointer<MUtils::Hash::Hash> test(MUtils::Hash::create(MUtils::Hash::HASH_##ID)); \
		QFile file(fileName); \
		ASSERT_TRUE(file.open(QIODevice::ReadOnly)); \
		ASSERT_TRUE(test->update(file)); \
		file.close(); \
		ASSERT_STRCASEEQ(test->digest(), (DIGEST)); \
	} \
	QFile::remove(fileName); \
} \
while(0)

#define TEST_HASH_STRESS(ID, HASHMAP, INPUT) do \
{ \
	QByteArray message((INPUT)); \
	const int initialSize = (HASHMAP).size(); \
	qsrand(time(NULL)); \
	for (size_t i = 0; i < 1299827; i++) \
	{ \
		fisherYatesShuffle(message); \
		QScopedPointer<MUtils::Hash::Hash> hash(MUtils::Hash::create(MUtils::Hash::HASH_##ID)); \
		ASSERT_TRUE(hash->update(message)); \
		(HASHMAP).insert(hash->digest(false)); \
		ASSERT_GT(((size_t)(HASHMAP).count()), initialSize + i); \
	} \
} \
while (0)

//-----------------------------------------------------------------
// Keccak
//-----------------------------------------------------------------

TEST_F(HashTest, TestKeccak224Direct)
{
	TEST_HASH_DIRECT(KECCAK_224, "",               "f71837502ba8e10837bdd8d365adb85591895602fc552b48b7390abd");
	TEST_HASH_DIRECT(KECCAK_224, TEST_MESSAGE_ORG, "310aee6b30c47350576ac2873fa89fd190cdc488442f3ef654cf23fe");
	TEST_HASH_DIRECT(KECCAK_224, TEST_MESSAGE_ALT, "6db4cb22fe56606394880043ce4917a1803ad73b2e2b782768ea713b");
}

TEST_F(HashTest, TestKeccak224FileIO)
{
	TEST_HASH_FILEIO(KECCAK_224, TEST_MESSAGE_ORG, "310aee6b30c47350576ac2873fa89fd190cdc488442f3ef654cf23fe");
	TEST_HASH_FILEIO(KECCAK_224, TEST_MESSAGE_ALT, "6db4cb22fe56606394880043ce4917a1803ad73b2e2b782768ea713b");
}

TEST_F(HashTest, TestKeccak256Direct)
{
	TEST_HASH_DIRECT(KECCAK_256, "",               "c5d2460186f7233c927e7db2dcc703c0e500b653ca82273b7bfad8045d85a470");
	TEST_HASH_DIRECT(KECCAK_256, TEST_MESSAGE_ORG, "4d741b6f1eb29cb2a9b9911c82f56fa8d73b04959d3d9d222895df6c0b28aa15");
	TEST_HASH_DIRECT(KECCAK_256, TEST_MESSAGE_ALT, "4cc2957d93a4a88251a40c48a42225364157567bc81d4aec9389ee7065c042d4");
}

TEST_F(HashTest, TestKeccak256FileIO)
{
	TEST_HASH_FILEIO(KECCAK_256, TEST_MESSAGE_ORG, "4d741b6f1eb29cb2a9b9911c82f56fa8d73b04959d3d9d222895df6c0b28aa15");
	TEST_HASH_FILEIO(KECCAK_256, TEST_MESSAGE_ALT, "4cc2957d93a4a88251a40c48a42225364157567bc81d4aec9389ee7065c042d4");
}

TEST_F(HashTest, TestKeccak384Direct)
{
	TEST_HASH_DIRECT(KECCAK_384, "",               "2c23146a63a29acf99e73b88f8c24eaa7dc60aa771780ccc006afbfa8fe2479b2dd2b21362337441ac12b515911957ff");
	TEST_HASH_DIRECT(KECCAK_384, TEST_MESSAGE_ORG, "283990fa9d5fb731d786c5bbee94ea4db4910f18c62c03d173fc0a5e494422e8a0b3da7574dae7fa0baf005e504063b3");
	TEST_HASH_DIRECT(KECCAK_384, TEST_MESSAGE_ALT, "4e16433a5e0cdc2d88a21714a040c92999230a1c3d165a4e9670702b2df26f3587b4d721b6faaf348228f616dd921578");
}

TEST_F(HashTest, TestKeccak384FileIO)
{
	TEST_HASH_FILEIO(KECCAK_384, TEST_MESSAGE_ORG, "283990fa9d5fb731d786c5bbee94ea4db4910f18c62c03d173fc0a5e494422e8a0b3da7574dae7fa0baf005e504063b3");
	TEST_HASH_FILEIO(KECCAK_384, TEST_MESSAGE_ALT, "4e16433a5e0cdc2d88a21714a040c92999230a1c3d165a4e9670702b2df26f3587b4d721b6faaf348228f616dd921578");
}

TEST_F(HashTest, TestKeccak512Direct)
{
	TEST_HASH_DIRECT(KECCAK_512, "",               "0eab42de4c3ceb9235fc91acffe746b29c29a8c366b7c60e4e67c466f36a4304c00fa9caf9d87976ba469bcbe06713b435f091ef2769fb160cdab33d3670680e");
	TEST_HASH_DIRECT(KECCAK_512, TEST_MESSAGE_ORG, "d135bb84d0439dbac432247ee573a23ea7d3c9deb2a968eb31d47c4fb45f1ef4422d6c531b5b9bd6f449ebcc449ea94d0a8f05f62130fda612da53c79659f609");
	TEST_HASH_DIRECT(KECCAK_512, TEST_MESSAGE_ALT, "0b46f421465ec602262e0a1044e59b36fbdb5f63f84e712963d2bc61bcb46ab0ebf86e59c14c253717ea558929c251695663226ffa5660ff7a29a5acbdaea901");
}

TEST_F(HashTest, TestKeccak512FileIO)
{
	TEST_HASH_FILEIO(KECCAK_512, TEST_MESSAGE_ORG, "d135bb84d0439dbac432247ee573a23ea7d3c9deb2a968eb31d47c4fb45f1ef4422d6c531b5b9bd6f449ebcc449ea94d0a8f05f62130fda612da53c79659f609");
	TEST_HASH_FILEIO(KECCAK_512, TEST_MESSAGE_ALT, "0b46f421465ec602262e0a1044e59b36fbdb5f63f84e712963d2bc61bcb46ab0ebf86e59c14c253717ea558929c251695663226ffa5660ff7a29a5acbdaea901");
}

TEST_F(HashTest, TestKeccak512Stress)
{
	QSet<QByteArray> test;
	TEST_HASH_STRESS(KECCAK_512, test, TEST_MESSAGE_ORG);
	TEST_HASH_STRESS(KECCAK_512, test, TEST_MESSAGE_ALT);
}

//-----------------------------------------------------------------
// BLAKE2
//-----------------------------------------------------------------

TEST_F(HashTest, TestBlake2Direct)
{
	TEST_HASH_DIRECT(BLAKE2_512, "",               "786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419d25e1031afee585313896444934eb04b903a685b1448b755d56f701afe9be2ce");
	TEST_HASH_DIRECT(BLAKE2_512, TEST_MESSAGE_ORG, "a8add4bdddfd93e4877d2746e62817b116364a1fa7bc148d95090bc7333b3673f82401cf7aa2e4cb1ecd90296e3f14cb5413f8ed77be73045b13914cdcd6a918");
	TEST_HASH_DIRECT(BLAKE2_512, TEST_MESSAGE_ALT, "a5b8a16391f8e34e16901fc2fd5754523b0c95354c2f22d3efc327c53070504ea062e219c502561f77a4933c18d36633e5f3ecf1f11506159f4b1875abb767c1");
}

TEST_F(HashTest, TestBlake2FileIO)
{
	TEST_HASH_FILEIO(BLAKE2_512, TEST_MESSAGE_ORG, "a8add4bdddfd93e4877d2746e62817b116364a1fa7bc148d95090bc7333b3673f82401cf7aa2e4cb1ecd90296e3f14cb5413f8ed77be73045b13914cdcd6a918");
	TEST_HASH_FILEIO(BLAKE2_512, TEST_MESSAGE_ALT, "a5b8a16391f8e34e16901fc2fd5754523b0c95354c2f22d3efc327c53070504ea062e219c502561f77a4933c18d36633e5f3ecf1f11506159f4b1875abb767c1");
}

TEST_F(HashTest, TestBlake2Stress)
{
	QSet<QByteArray> test;
	TEST_HASH_STRESS(BLAKE2_512, test, TEST_MESSAGE_ORG);
	TEST_HASH_STRESS(BLAKE2_512, test, TEST_MESSAGE_ALT);
}

#undef TEST_HASH_DIRECT
#undef TEST_HASH_FILEIO
