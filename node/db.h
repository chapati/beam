// Copyright 2018 The Beam Team
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "core/common.h"
#include "core/block_crypt.h"
#include "sqlite/sqlite3.h"

namespace beam {

class NodeDBUpgradeException : public std::runtime_error
{
public:
    NodeDBUpgradeException(const char* message)
        : std::runtime_error(message)
    {}
};

class NodeDB
{
public:

	struct StateFlags {
		static const uint32_t Functional	= 0x1;	// has block body
		static const uint32_t Reachable		= 0x2;	// has only functional nodes up to the genesis state
		static const uint32_t Active		= 0x4;	// part of the current blockchain
	};

	struct ParamID {
		enum Enum {
			DbVer,
			CursorRow,
			CursorHeight,
			FossilHeight, // Height starting from which and below original blocks are erased
			CfgChecksum,
			MyID,
			RichContractInfo,
			RichContractParser,
			Treasury,
			EventsOwnerID, // hash of keys used to scan and record events
			HeightTxoLo, // Height starting from which and below Txo info is totally erased.
			HeightTxoHi, // Height starting from which and below Txo infi is compacted, only the commitment is left
			SyncData,
			LastRecoveryHeight,
			MappingStamp,
			Deprecated_3, // ShieldedOutputs
			ShieldedInputs,
			AssetsCount, // Including unused. The last element is guaranteed to be used.
			AssetsCountUsed, // num of 'live' assets
			EventsSerif, // pseudo-random, reset each time the events are rescanned.
			ForbiddenState,
			Flags1, // used for 2-stage migration, where the 2nd stage is performed by the Processor
			CacheState,
		};
	};

	struct Flags1 {
		static const uint64_t PendingRebuildNonStd = 1;
	};

	struct Query
	{
		enum Enum
		{
			Begin,
			Commit,
			Rollback,
			Scheme,
			AutoincrementID,
			ParamGet,
			ParamSet,
			ParamDel,
			StateIns,
			StateDel,
			StateGet,
			StateGetHash,
			StateGetHeightAndPrev,
			StateFind,
			StateFind2,
			StateFindWithFlag,
			StateFindWorkGreater,
			StateUpdPrevRow,
			StateGetNextFCount,
			StateSetNextCount,
			StateSetNextCountF,
			StateGetHeightAndAux,
			StateGetNextFunctional,
			StateSetFlags,
			StateGetFlags0,
			StateGetFlags1,
			StateGetChainWork,
			StateGetNextCount,
			StateSetPeer,
			StateGetPeer,
			StateGetExtra,
			StateSetInputs,
			StateGetInputs,
			StateSetTxosAndExtra,
			StateGetTxos,
			StateFindByTxos,
			TipAdd,
			TipDel,
			TipReachableAdd,
			TipReachableDel,
			EnumTips,
			EnumFunctionalTips,
			EnumAtHeight,
			EnumAncestors,
			StateGetPrev,
			Unactivate,
			Activate,
			StateGetBlock,
			StateSetBlock,
			StateDelBlockPP,
			StateDelBlockPPR,
			StateDelBlockAll,
			EventIns,
			EventDel,
			EventEnum,
			EventFind,
			PeerAdd,
			PeerDel,
			PeerEnum,
			BbsEnumCSeq,
			BbsHistogram,
			BbsEnumAllSeq,
			BbsEnumAll,
			BbsFindRaw,
			BbsFind,
			BbsFindCursor,
			BbsDel,
			BbsIns,
			BbsMaxTime,
			BbsTotals,
			DummyIns,
			DummyFindLowest,
			DummyFind,
			DummyUpdHeight,
			DummyDel,
			KernelIns,
			KernelFind,
			KernelDel,
			TxoAdd,
			TxoDel,
			TxoDelFrom,
			TxoSetSpent,
			TxoEnum,
			TxoEnumBySpentMigrate,
			TxoSetValue,
			TxoGetValue,
			BlockFind,
			FindHeightBelow,
			StreamIns,
			StreamDel,
			EnumSystemStatesBkwd,
			UniqueIns,
			UniqueFind,
			UniqueDel,
			UniqueDelAll,
			CacheIns,
			CacheFind,
			CacheEnumByHit,
			CacheUpdateHit,
			CacheDel,

			AssetFindOwner,
			AssetFindMin,
			AssetAdd,
			AssetDel,
			AssetGet,
			AssetSetVal,
			AssetsDelAll,

			AssetEvtsInsert,
			AssetEvtsEnumBwd,
			AssetEvtsGet,
			AssetEvtsDeleteFrom,

			ContractDataFind,
			ContractDataFindNext,
			ContractDataFindPrev,
			ContractDataInsert,
			ContractDataUpdate,
			ContractDataDel,
			ContractDataEnum,
			ContractDataEnumAll,
			ContractDataDelAll,

			ContractLogInsert,
			ContractLogDel,
			ContractLogEnum,
			ContractLogEnumCid,

			ShieldedStatisticSel,
			ShieldedStatisticIns,
			ShieldedStatisticDel,

			KrnInfoInsert,
			KrnInfoEnumH,
			KrnInfoEnumCid,
			KrnInfoDel,

			Dbg0,
			Dbg1,
			Dbg2,
			Dbg3,
			Dbg4,

			count
		};
	};

	struct StreamType
	{
		enum Enum
		{
			StatesMmr,
			Shielded,
			ShieldedMmr,
			AssetsMmr,
			ShieldedState,

			count
		};

		static uint64_t Key(uint64_t idx, Enum);
	};

	NodeDB();
	virtual ~NodeDB();

	void Close();
	void Open(const char* szPath);
	bool IsOpen() const
	{
		return nullptr != m_pDb;
	}

	void Vacuum();
	void CheckIntegrity();

	virtual void OnModified() {}

	class Recordset
	{
		sqlite3_stmt* m_pStmt;
		NodeDB* m_pDB;

		void InitInternal(NodeDB&, Query::Enum, const char*);

	public:

		Recordset();
		Recordset(NodeDB&, Query::Enum, const char*);
		~Recordset();

		void Reset();
		void Reset(NodeDB&, Query::Enum, const char*);

		// Perform the query step. SELECT only: returns true while there're rows to read
		bool Step();
		void StepStrict(); // must return at least 1 row, applicable for SELECT
		bool StepModifySafe(); // UPDATE/INSERT: returns true if successful, false if constraints are violated (but exc is not raised)

		// in/out
		void put(int col, uint32_t);
		void put(int col, uint64_t);
		void put(int col, const Blob&);
		void put(int col, const char*);
		void put(int col, const Key::ID&, Key::ID::Packed&);
		void get(int col, uint32_t&);
		void get(int col, uint64_t&);
		void get(int col, Blob&);
		void get(int col, ByteBuffer&); // don't over-use
		void get(int col, Key::ID&);

		const void* get_BlobStrict(int col, uint32_t n);

		template <typename T> void put_As(int col, const T& x) { put(col, Blob(&x, sizeof(x))); }
		template <typename T> void get_As(int col, T& out) { out = get_As<T>(col); }
		template <typename T> const T& get_As(int col) { return *(const T*) get_BlobStrict(col, sizeof(T)); }

		void putNull(int col);
		bool IsNull(int col);

		void put(int col, const Merkle::Hash& x) { put_As(col, x); }
		void get(int col, Merkle::Hash& x) { get_As(col, x); }
		void put(int col, const Block::PoW& x) { put_As(col, x); }
		void get(int col, Block::PoW& x) { get_As(col, x); }

		void putZeroBlob(int col, uint32_t nSize);
	};

	int get_RowsChanged() const;
	uint64_t get_LastInsertRowID() const;

	class Transaction {
		NodeDB* m_pDB;
	public:
		Transaction(NodeDB* = NULL);
		Transaction(NodeDB& db) :Transaction(&db) {}
		~Transaction(); // by default - rolls back

		bool IsInProgress() const { return NULL != m_pDB; }

		void Start(NodeDB&);
		void Commit();
		void Rollback();
	};

	// Hi-level functions

	void ParamSet(uint32_t ID, const uint64_t*, const Blob*);
	bool ParamGet(uint32_t ID, uint64_t*, Blob*, ByteBuffer* = NULL);
	bool ParamDelSafe(uint32_t ID);

	uint64_t ParamIntGetDef(uint32_t ID, uint64_t def = 0);
	void ParamIntSet(uint32_t ID, uint64_t val);

	uint64_t InsertState(const Block::SystemState::Full&, const PeerID&); // Fails if state already exists

	uint64_t FindActiveStateStrict(Height);
	uint64_t StateFindSafe(const Block::SystemState::ID&);
	void get_State(uint64_t rowid, Block::SystemState::Full&);
	void get_StateHash(uint64_t rowid, Merkle::Hash&);

	bool DeleteState(uint64_t rowid, uint64_t& rowPrev); // State must exist. Returns false if there are ancestors.

	uint32_t GetStateNextCount(uint64_t rowid);
	uint32_t GetStateFlags(uint64_t rowid);
	void SetFlags(uint64_t rowid, uint32_t);

	void SetStateFunctional(uint64_t rowid);
	void SetStateNotFunctional(uint64_t rowid);

	void set_Peer(uint64_t rowid, const PeerID*);
	bool get_Peer(uint64_t rowid, PeerID&);

	uint32_t get_StateExtra(uint64_t rowid, void*, uint32_t nSize);
	TxoID get_StateTxos(uint64_t rowid);

	void set_StateTxosAndExtra(uint64_t rowid, const TxoID*, const Blob* pExtra, const Blob* pRB);

	void SetStateBlock(uint64_t rowid, const Blob& bodyP, const Blob& bodyE, const PeerID&);
	void GetStateBlock(uint64_t rowid, ByteBuffer* pP, ByteBuffer* pE, ByteBuffer* pRB);
	void DelStateBlockPP(uint64_t rowid); // delete perishable, peer. Keep eternal, extra, txos, rollback
	void DelStateBlockPPR(uint64_t rowid); // delete perishable, rollback, peer. Keep eternal, extra, txos
	void DelStateBlockAll(uint64_t rowid); // delete perishable, peer, eternal, extra, txos, rollback

	struct StateID {
		uint64_t m_Row;
		Height m_Height;
		void SetNull();
	};

	void get_StateID(const StateID&, Block::SystemState::ID&);

	TxoID FindStateByTxoID(StateID&, TxoID); // returns the Txos at state end

	struct WalkerState {
		Recordset m_Rs;
		StateID m_Sid;

		bool MoveNext();
	};

#pragma pack (push, 1)
	struct StateInput
	{
		ECC::uintBig m_CommX;
		TxoID m_Txo_AndY;

		static const TxoID s_Y = TxoID(1) << (sizeof(TxoID) * 8 - 1);

		void Set(TxoID, const ECC::Point&);
		void Set(TxoID, const ECC::uintBig& x, uint8_t y);

		TxoID get_ID() const;
		void Get(ECC::Point&) const;

		static bool IsLess(const StateInput&, const StateInput&);
	};
#pragma pack (pop)

	void set_StateInputs(uint64_t rowid, StateInput*, size_t);
	bool get_StateInputs(uint64_t rowid, std::vector<StateInput>&);

	void EnumTips(WalkerState&); // height lowest to highest
	void EnumFunctionalTips(WalkerState&); // chainwork highest to lowest

	Height get_HeightBelow(Height);
	void EnumStatesAt(WalkerState&, Height);
	void EnumAncestors(WalkerState&, const StateID&);
	bool get_Prev(StateID&);
	bool get_Prev(uint64_t&);

	bool get_Cursor(StateID& sid);

	void get_ChainWork(uint64_t, Difficulty::Raw&);

	// the following functions move the curos, and mark the states with 'Active' flag
	void MoveBack(StateID&);
	void MoveFwd(const StateID&);

	void assert_valid(); // diagnostic, for tests only

	typedef uint32_t EventIndexType;
	void InsertEvent(Height, const Blob&, const Blob& key); // body must start with the uintBigFor<EventIndexType>
	void DeleteEventsFrom(Height);

	struct WalkerEvent {
		Recordset m_Rs;
		Height m_Height;
		uintBigFor<EventIndexType>::Type m_Index;
		Blob m_Body;
		Blob m_Key;

		bool MoveNext();
	};

	void EnumEvents(WalkerEvent&, Height hMin);
	void FindEvents(WalkerEvent&, const Blob& key); // in case of duplication the most recently added comes first

	struct WalkerPeer
	{
		Recordset m_Rs;

		struct Data {
			PeerID m_ID;
			uint32_t m_Rating;
			uint64_t m_Address;
			Timestamp m_LastSeen;
		} m_Data;

		bool MoveNext();
	};

	void EnumPeers(WalkerPeer&); // highest to lowest
	void PeerIns(const WalkerPeer::Data&);
	void PeersDel();

	struct WalkerBbs
	{
		typedef ECC::Hash::Value Key;

		Recordset m_Rs;
		uint64_t m_ID;

		struct Data {
			Key m_Key;
			BbsChannel m_Channel;
			Timestamp m_TimePosted;
			Blob m_Message;
			uint32_t m_Nonce;
		} m_Data;

		bool MoveNext();
	};

	void EnumBbsCSeq(WalkerBbs&); // set channel and ID before invocation
	uint64_t BbsIns(const WalkerBbs::Data&); // must be unique (if not sure - first try to find it). Returns the ID
	bool BbsFind(WalkerBbs&); // set Key
	uint64_t BbsFind(const WalkerBbs::Key&);
	void BbsDel(uint64_t id);
	uint64_t BbsFindCursor(Timestamp);
	Timestamp get_BbsMaxTime();
	uint64_t get_BbsLastID();

	struct BbsTotals {
		uint32_t m_Count;
		uint64_t m_Size;
	};

	void get_BbsTotals(BbsTotals&);

	struct WalkerBbsLite
	{
		typedef WalkerBbs::Key Key;

		Recordset m_Rs;
		uint64_t m_ID;
		Key m_Key;
		uint32_t m_Size;

		bool MoveNext();
	};

	void EnumAllBbsSeq(WalkerBbsLite&); // ordered by m_ID. Must be initialized to specify the lower bound

	struct WalkerBbsTimeLen
	{
		Recordset m_Rs;
		uint64_t m_ID;
		Timestamp m_Time;
		uint32_t m_Size;

		bool MoveNext();
	};

	void EnumAllBbs(WalkerBbsTimeLen&); // ordered by m_ID.

	struct IBbsHistogram {
		virtual bool OnChannel(BbsChannel, uint64_t nCount) = 0;
	};
	bool EnumBbs(IBbsHistogram&);


	void InsertDummy(Height h, const Key::ID&);
	Height GetLowestDummy(Key::ID&);
	void DeleteDummy(const Key::ID& kid);
	void SetDummyHeight(const Key::ID&, Height);
	Height GetDummyHeight(const Key::ID&);

	void InsertKernel(const Blob&, Height h);
	void DeleteKernel(const Blob&, Height h);
	Height FindKernel(const Blob&); // in case of duplicates - returning the one with the largest Height
    Height FindBlock(const Blob&);

	uint64_t FindStateWorkGreater(const Difficulty::Raw&);

	void TxoAdd(TxoID, const Blob&);
	void TxoDel(TxoID);
	void TxoDelFrom(TxoID);
	void TxoSetSpent(TxoID, Height);

	struct WalkerTxo
	{
		Recordset m_Rs;
		TxoID m_ID;
		Blob m_Value;
		Height m_SpendHeight;

		bool MoveNext();
	};

	void EnumTxos(WalkerTxo&, TxoID id0);
	void TxoSetValue(TxoID, const Blob&);
	void TxoGetValue(WalkerTxo&, TxoID);

	void ShieldedResize(uint64_t n, uint64_t n0) {
		StreamResize_T<ECC::Point::Storage>(StreamType::Shielded, n, n0);
	}

	void ShieldedWrite(uint64_t pos, const ECC::Point::Storage* p, uint64_t nCount) {
		StreamIO_T(StreamType::Shielded, pos, Cast::NotConst(p), nCount, true);
	}

	void ShieldedRead(uint64_t pos, ECC::Point::Storage* p, uint64_t nCount) {
		StreamIO_T(StreamType::Shielded, pos, p, nCount, false);
	}

	void ShieldedStateResize(uint64_t n, uint64_t n0) {
		StreamResize_T<ECC::Hash::Value>(StreamType::ShieldedState, n, n0);
	}

	void ShieldedStateWrite(uint64_t pos, const ECC::Hash::Value* p, uint64_t nCount) {
		StreamIO_T(StreamType::ShieldedState, pos, Cast::NotConst(p), nCount, true);
	}

	void ShieldedStateRead(uint64_t pos, ECC::Hash::Value* p, uint64_t nCount) {
		StreamIO_T(StreamType::ShieldedState, pos, p, nCount, false);
	}

	void ShieldedOutpSet(Height h, uint64_t count);
	uint64_t ShieldedOutpGet(Height h);
	void ShieldedOutpDelFrom(Height h);

	struct WalkerSystemState
	{
		Recordset m_Rs;
		uint64_t m_RowTrg;
		Block::SystemState::Full m_State;

		bool MoveNext();
	};

	void EnumSystemStatesBkwd(WalkerSystemState&, const StateID&);

	class StreamMmr
		:public Merkle::FlatMmr
	{
		const uint8_t m_hStoreFrom;
		StreamType::Enum m_eType;

	public:
		NodeDB& m_DB;

		StreamMmr(NodeDB&, StreamType::Enum, bool bStoreH0);

		void Append(const Merkle::Hash&);
		void ShrinkTo(uint64_t nCount);
		void ResizeTo(uint64_t nCount);

	protected:
		// Mmr
		virtual void LoadElement(Merkle::Hash& hv, const Merkle::Position& pos) const override;
		virtual void SaveElement(const Merkle::Hash& hv, const Merkle::Position& pos) override;

		struct CacheEntry
		{
			Merkle::Hash m_Value;
			uint64_t m_X;
		};

		// Simple cache, optimized for sequential add and root calculation
		CacheEntry m_pCache[64];

		// last popped element
		struct
		{
			Merkle::Hash m_Value;
			Merkle::Position m_Pos;
		} m_LastOut;

		bool CacheFind(Merkle::Hash& hv, const Merkle::Position& pos) const;
		void CacheAdd(const Merkle::Hash& hv, const Merkle::Position& pos);
	};

	class StatesMmr
		:public StreamMmr
	{
	public:
		static uint64_t H2I(Height h);

		StatesMmr(NodeDB&);

		void LoadStateHash(Merkle::Hash& hv, Height) const;

	protected:
		// Mmr
		virtual void LoadElement(Merkle::Hash& hv, const Merkle::Position& pos) const override;
		virtual void SaveElement(const Merkle::Hash& hv, const Merkle::Position& pos) override;
	};

	bool UniqueInsertSafe(const Blob& key, const Blob* pVal); // returns false if not unique (and doesn't update the value)
	bool UniqueFind(const Blob& key, Recordset&);
	void UniqueDeleteStrict(const Blob& key);
	void UniqueDeleteAll();

	void CacheInsert(const Blob& key, const Blob& data);
	bool CacheFind(const Blob& key, ByteBuffer&);
	void CacheSetMaxSize(uint64_t);

#pragma pack (push, 1)
	struct CacheState
	{
		uint64_t m_HitCounter;
		uint64_t m_SizeMax;
		uint64_t m_SizeCurrent;
	};
#pragma pack (pop)

	void get_CacheState(CacheState&);

	void AssetAdd(Asset::Full&); // sets ID=0 to auto assign, otherwise - specified ID must be used
	Asset::ID AssetFindByOwner(const PeerID&);
	Asset::ID AssetDelete(Asset::ID); // returns remaining assets count (including the unused)
	bool AssetGetSafe(Asset::Full&); // must set ID before invocation
	void AssetSetValue(Asset::ID, const AmountBig::Type&, Height hLockHeight);
	bool AssetGetNext(Asset::Full&); // for enum
	void AssetsDelAll();

	struct AssetEvt
	{
		Asset::ID m_ID;
		Height m_Height;
		uint64_t m_Index;
		Blob m_Body;
	};

	struct WalkerAssetEvt
		:public AssetEvt
	{
		Recordset m_Rs;

		bool MoveNext();
	};

	void AssetEvtsInsert(const AssetEvt&);
	void AssetEvtsEnumBwd(WalkerAssetEvt&, Asset::ID, Height);
	void AssetEvtsGetStrict(WalkerAssetEvt&, Height, uint64_t);
	void AssetEvtsDeleteFrom(Height);

	bool ContractDataFind(const Blob& key, Blob&, Recordset&);
	bool ContractDataFindNext(Blob& key, Recordset&); // key in-out
	bool ContractDataFindPrev(Blob& key, Recordset&); // key in-out
	void ContractDataInsert(const Blob& key, const Blob&);
	void ContractDataUpdate(const Blob& key, const Blob&);
	void ContractDataDel(const Blob& key);
	void ContractDataDelAll();

	struct WalkerContractData
	{
		Recordset m_Rs;
		Blob m_Key;
		Blob m_Val;
		bool MoveNext();
	};

	void ContractDataEnum(WalkerContractData&, const Blob& keyMin, const Blob& keyMax);
	void ContractDataEnum(WalkerContractData&);

	void StreamsDelAll(StreamType::Enum t0, StreamType::Enum t1);

#pragma pack (push, 1)
	struct HeightPosPacked
	{
		uintBigFor<Height>::Type m_Height;
		uintBigFor<uint32_t>::Type m_Idx;
		void put(NodeDB::Recordset& rs, int iCol, const HeightPos& pos);
		static void get(NodeDB::Recordset& rs, int iCol, HeightPos& pos);
	};
#pragma pack (pop)

	struct ContractLog
	{
		struct Entry
		{
			HeightPos m_Pos;
			Blob m_Key;
			Blob m_Val;
		};

		struct Walker
		{
			HeightPosPacked m_bufMin, m_bufMax;

			Recordset m_Rs;
			Entry m_Entry;
			bool MoveNext();
		};
	};

	void ContractLogInsert(const ContractLog::Entry&);
	void ContractLogDel(const HeightPos& posMin, const HeightPos& posMax);
	void ContractLogEnum(ContractLog::Walker&, const HeightPos& posMin, const HeightPos& posMax);
	void ContractLogEnum(ContractLog::Walker&, const Blob& keyMin, const Blob& keyMax, const HeightPos& posMin, const HeightPos& posMax);

	struct KrnInfo
	{
		typedef ECC::Hash::Value Cid;

		struct Entry
		{
			HeightPos m_Pos;
			Cid m_Cid;
			Blob m_Val;
		};

		struct Walker
		{
			HeightPosPacked m_bufMin, m_bufMax;
			Recordset m_Rs;
			Entry m_Entry;
			bool MoveNext();
		};
	};


	void KrnInfoInsert(const KrnInfo::Entry&);
	void KrnInfoDel(const HeightRange&);
	void KrnInfoEnum(KrnInfo::Walker&, Height);
	void KrnInfoEnum(KrnInfo::Walker&, const HeightPos&, const HeightPos&);
	void KrnInfoEnum(KrnInfo::Walker&, const KrnInfo::Cid&, Height hMax);

	void TestChanged1Row();

private:

	sqlite3* m_pDb;

	struct Statement
	{
		sqlite3_stmt* m_pStmt;
		Statement() :m_pStmt(nullptr) {}
		~Statement() { Close(); }

		void Close();
	};

	Statement m_pPrep[Query::count];

	void Prepare(Statement&, const char*);

	void TestRet(int);
	void ThrowSqliteError(int);
	static void ThrowError(const char*);
	static void ThrowInconsistent();

	void Create();
	void CreateTables20();
	void CreateTables21();
	void CreateTables22();
	void CreateTables23();
	void CreateTables27();
	void CreateTables28();
	void CreateTables30();
	void CreateTables31();
	void ExecQuick(const char*);
	std::string ExecTextOut(const char*);
	bool ExecStep(sqlite3_stmt*);
	int ExecStepRaw(sqlite3_stmt*);
	bool ExecStep(Query::Enum, const char*); // returns true while there's a row

	sqlite3_stmt* get_Statement(Query::Enum, const char*);

	uint64_t get_AutoincrementID(const char* szTable);
	void TipAdd(uint64_t rowid, Height);
	void TipDel(uint64_t rowid, Height);
	void TipReachableAdd(uint64_t rowid);
	void TipReachableDel(uint64_t rowid);
	void SetNextCount(uint64_t rowid, uint32_t);
	void SetNextCountFunctional(uint64_t rowid, uint32_t);
	void OnStateReachable(uint64_t rowid, uint64_t rowPrev, Height, bool);
	void put_Cursor(const StateID& sid); // jump

	void MigrateFrom18();
	void MigrateFrom20();

	static const uint32_t s_StreamBlob;

	void StreamIO(StreamType::Enum, uint64_t pos, uint8_t*, uint64_t nCount, bool bWrite);
	void StreamResize(StreamType::Enum, uint64_t n, uint64_t n0);
	void StreamShrinkInternal(uint64_t k0, uint64_t k1);

	template <typename T>
	void StreamIO_T(StreamType::Enum eType, uint64_t pos, T* p, uint64_t nCount, bool bWrite) {
		StreamIO(eType, pos * sizeof(T), reinterpret_cast<uint8_t*>(p), nCount * sizeof(T), bWrite);
	}

	template <typename T>
	void StreamResize_T(StreamType::Enum eType, uint64_t n, uint64_t n0) {
		StreamResize(eType, n * sizeof(T), n0 * sizeof(T));
	}

	struct BlobGuard;
	void OpenBlob(BlobGuard&, const char* szTable, const char* szColumn, uint64_t rowid, bool bRW);

	static const Asset::ID s_AssetEmpty0;
	void AssetInsertRaw(Asset::ID, const Asset::Full*);
	void AssetDeleteRaw(Asset::ID);
	Asset::ID AssetFindMinFree(Asset::ID nMin);

	void set_CacheState(CacheState&); // auto cleans the cache if necessary
};



} // namespace beam
