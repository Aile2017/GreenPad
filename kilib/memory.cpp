#include "stdafx.h"
#include "memory.h"
using namespace ki;

#ifdef _DEBUG
#include "log.h"
#include "kstring.h"
#endif


//=========================================================================

#ifdef __GNUC__
// Operator new/delete for -nostdlib gcc builds (call malloc/free from msvcrt)
void* __cdecl operator new(size_t siz)     { return malloc(siz); }
void  __cdecl operator delete(void* ptr)   { if (ptr) free(ptr); }
void  operator delete(void* ptr, size_t)   { ::operator delete(ptr); }
void  operator delete[](void* ptr)         { ::operator delete(ptr); }
void  operator delete[](void* ptr, size_t) { ::operator delete(ptr); }
void* operator new[](size_t sz)            { return ::operator new(sz); }
#endif // __GNUC__


#ifdef USE_ORIGINAL_MEMMAN
//=========================================================================
//	効率的なメモリ管理を目指すアロケータ
//=========================================================================

//
// メモリブロック
// 「sizバイト * num個」分の領域を一括確保するのが仕事
//
// 空きブロックには、先頭バイトに [次の空きブロックのindex] を格納。
// これを用いて、先頭への出し入れのみが可能な単方向リストとして扱う。
//

struct ki::MemBlock
{
public:
	bool  Construct( ushort siz, ushort num )
	{
		// 確保
		buf_   = (byte *)malloc( siz*num );
		if( !buf_ )
			return false;
		first_ = 0;
		avail_ = num;

		// 連結リスト初期化
		ushort i=0;
		for( byte *p=buf_; i<num; p+=siz )
			*((ushort*)p) = ++i;
		return true;
	}

	inline void  Destruct()
		{ ::free( buf_ ); /* 解放 */ }

	void* Alloc( ushort siz )
	{
		// メモリ切り出し
		//   ( avail==0 等のチェックは上位層に任せる )
		byte* blk = buf_ + siz*first_;
		first_    = *(ushort*)blk;
		--avail_;
		return blk;
	}

	void  DeAlloc( void* ptr, ushort siz )
	{
		// メモリ戻す
		//   ( 変なポインタ渡されたらだ～め～ )
		byte* blk = static_cast<byte*>(ptr);
		*(ushort*)blk      = first_;
		first_    = static_cast<ushort>((blk-buf_)/siz);
		++avail_;
	}

	inline bool  isAvail()
		{ return (avail_ != 0); } // 空きがある？
	inline bool  isEmpty( ushort num )
		{ return (avail_ == num); } // 完全に空？
	bool hasThisPtr( const void* ptr, size_t len ) // このブロックのポインタ？
		{ return ( buf_<=ptr && ptr<buf_+len ); }

private:
	byte* buf_;
	ushort first_, avail_;
};

//-------------------------------------------------------------------------

//
// 固定サイズ確保人
// 「sizバイト」の領域を毎回確保するのが仕事
//
// メモリブロックのリストを保持し、空いているブロックを使って
// メモリ要求に応えていく。空きがなくなったら新しくMemBlockを
// 作ってリストに加える。
//
// 最後にメモリ割り当て/解放を行ったBlockをそれぞれ記憶しておき、
// 最初にそこを調べることで高速化を図る。
//

bool MemoryManager::FixedSizeMemBlockPool::Construct( ushort siz )
{
	// メモリブロック情報域をちょこっと確保
	blocks_ = (MemBlock *)malloc( sizeof(MemBlock) * 4 );
	if( !blocks_ ) return false;

	// ブロックサイズ等計算
	int npb = BLOCK_SIZ/siz;
	numPerBlock_ = static_cast<ushort>( Min( npb, 65535 ) );
	fixedSize_   = siz;

	// 一個だけブロック作成
	bool ok = blocks_[0].Construct( fixedSize_, numPerBlock_ );
	if( !ok )
	{
		free( blocks_ ); // free on failure
		return false;
	}
	// All good.
	lastA_            = 0;
	lastDA_           = 0;
	blockNum_         = 1;
	blockNumReserved_ = 4;
	return true;
}

void MemoryManager::FixedSizeMemBlockPool::Destruct()
{
	// 各ブロックを解放
	for( int i=0; i<blockNum_; ++i )
		blocks_[i].Destruct();

	// ブロック情報保持領域のメモリも解放
	free( blocks_ );
	blockNum_ = 0;
}

void* MemoryManager::FixedSizeMemBlockPool::Alloc()
{
	// ここでlastA_がValidかどうかチェックしないとまずい。
	// DeAllocされてなくなってるかもしらないので。

	// 前回メモリを切り出したブロックに
	// まだ空きがあるかどうかチェック
	if( !blocks_[lastA_].isAvail() )
	{
		// 無かった場合、リストの末尾から順に線形探索
		for( int i=blockNum_;; )
		{
			if( blocks_[--i].isAvail() )
			{
				// 空きブロック発見～！
				lastA_ = i;
				break;
			}
			if( i == 0 )
			{
				// 全部埋まってた...
				if( blockNum_ == blockNumReserved_ )
				{
					// しかも作業領域も満杯なので拡張
					MemBlock* nb = (MemBlock *)malloc( sizeof(MemBlock) * blockNum_*2 );
					if( !nb )
						return NULL;
					memmove( nb, blocks_, sizeof(MemBlock)*(blockNum_) );
					free( blocks_ );
					blocks_ = nb;
					blockNumReserved_ *= 2;
				}

				// 新しくブロック構築
				bool ok = blocks_[ blockNum_ ].Construct( fixedSize_, numPerBlock_ );
				if( !ok ) return NULL;
				lastA_ = blockNum_++;
				break;
			}
		}
	}
	void *ret = blocks_[lastA_].Alloc( fixedSize_ );
	// ブロックから切り出し割り当て
	return ret;
}

void MemoryManager::FixedSizeMemBlockPool::DeAlloc( void* ptr )
{
	// 該当ブロックを探索
	const INT_PTR mx=blockNum_-1, ln=fixedSize_*numPerBlock_;
	for( INT_PTR u=lastDA_, d=lastDA_-1;; )
	{
		if( u>=0 )
			if( blocks_[u].hasThisPtr(ptr,ln) )
			{
				lastDA_ = u;
				break;
			}
			else if( u==mx )
			{
				u = -1;
			}
			else
			{
				++u;
			}
		if( d>=0 )
			if( blocks_[d].hasThisPtr(ptr,ln) )
			{
				lastDA_ = d;
				break;
			}
			else
			{
				--d;
			}
	}

	// 解放を実行
	blocks_[lastDA_].DeAlloc( ptr, fixedSize_ );

	// この削除でブロックが完全に空になった場合
	if( blocks_[lastDA_].isEmpty( numPerBlock_ ) )
	{
		// しかも一番後ろのブロックでなかったら
		INT_PTR end = blockNum_-1;
		if( lastDA_ != end )
		{
			// 一番後ろが空だったら解放
			if( blocks_[end].isEmpty( numPerBlock_ ) )
			{
				blocks_[end].Destruct();
				--blockNum_;
				if( lastA_ > --end )
					lastA_ = end;
			}

			// 後ろと交換
			MemBlock tmp( blocks_[lastDA_] );
			blocks_[lastDA_] = blocks_[end];
			blocks_[end]     = tmp;
		}

		if( blockNum_ > 4 && blockNum_ <= blockNumReserved_ >> 2 )
		{
			// Reduce size of mem pool if less than a quarter of what is needed.
			//LOGGERF( TEXT("Reducing from %d to %d"), blockNumReserved_, blockNum_ );
			MemBlock* nb = (MemBlock *)malloc( sizeof(MemBlock) * blockNum_ );
			if( !nb )
				return;
			memmove( nb, blocks_, sizeof(MemBlock)*(blockNum_) );
			free( blocks_ );
			blocks_ = nb;
			blockNumReserved_ = blockNum_;
		}
	}
}

inline bool MemoryManager::FixedSizeMemBlockPool::isValid()
{
	// 既に使用開始されているか？
	return (blockNum_ != 0);
}

//-------------------------------------------------------------------------

//
// 最上位層
// 指定サイズにあった FixedSizeMemBlockPool に処理をまわす
//
// lokiの実装では固定サイズアロケータも、必要に応じて
// 動的確保していたが、それは面倒なのでやめました。(^^;
// 最初に64個確保したからと言って、そんなにメモリも喰わないし…。
//
MemoryManager* MemoryManager::pUniqueInstance_;
MemoryManager::MemoryManager()
{
	// メモリプールをZEROクリア
	#ifndef STACK_MEM_POOLS
	static MemoryManager::FixedSizeMemBlockPool staticpools[ SMALL_MAX/2 ];
	pools_ = staticpools;
	#endif
	#ifdef STACK_MEM_POOLS
	mem00( pools_, /*sizeof(pools_)*/ sizeof(FixedSizeMemBlockPool) * (SMALL_MAX/2) );
	#endif

	// 唯一のインスタンスは私です
	pUniqueInstance_ = this;
}

MemoryManager::~MemoryManager()
{
	// 構築済みメモリプールを全て解放, Release all built memory pools
	for( int i=0; i<SMALL_MAX/2; ++i )
		if( pools_[i].isValid() )
			pools_[i].Destruct();

//	delete [] pools_;
}

void* A_HOT MemoryManager::Alloc( size_t siz )
{
	siz = siz + (siz&1);

	// サイズが零か大きすぎるなら
	// デフォルトの new 演算子に任せる
	uint i = static_cast<uint>( (siz-1)/2 );
	if( i >= SMALL_MAX/2 )
		return malloc( siz );

	// マルチスレッド対応
	AutoLock al(this);

	// このサイズのメモリ確保が初めてなら
	// ここでメモリプールを作成する。
	if( !pools_[i].isValid() )
	{
		bool ok = pools_[i].Construct( static_cast<ushort>(siz) );
		if( !ok ) return NULL;
	}

	// ここで割り当て
	return pools_[i].Alloc();
}

void A_HOT MemoryManager::DeAlloc( void* ptr, size_t siz )
{
	siz = siz + (siz&1);

	// サイズが零か大きすぎるなら
	// デフォルトの delete 演算子に任せる
	uint i = static_cast<uint>( (siz-1)/2 );
	if( i >= SMALL_MAX/2 )
	{
		::free( ptr );
		return; // VCで return void が出来ないとは…
	}

	// マルチスレッド対応
	AutoLock al(this);

	// ここで解放
	pools_[i].DeAlloc( ptr );
}

#else // USE_ORIGINAL_MEMMAN



MemoryManager* MemoryManager::pUniqueInstance_;

MemoryManager::MemoryManager()
{
	// 唯一のインスタンスは私です
	pUniqueInstance_ = this;

}

MemoryManager::~MemoryManager()
{
}

void* MemoryManager::Alloc( size_t siz )
{
	return ::malloc(siz);
}
void MemoryManager::DeAlloc( void* ptr, size_t siz )
{
	::free(ptr);
}

#endif
