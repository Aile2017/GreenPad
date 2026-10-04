
#include "kilib/stdafx.h"
#include "RSearch.h"
#include "kilib/ktlaptr.h"
using namespace ki;



//=========================================================================
//@{
//	Character type
//@}
//=========================================================================

enum RegToken
{
	R_Char,  // normal characters
	R_Any,   // '.'
	R_Lcl,   // '['
	R_Rcl,   // ']'
	R_Ncl,   // '^'
	R_Range, // '-'
	R_Lbr,   // '('
	R_Rbr,   // ')'
	R_Bar,   // '|'
	R_Star,  // '*'
	R_Plus,  // '+'
	R_Quest, // '?'
	R_End    // '\0'
};



//=========================================================================
//@{
//	decompose into tokens
//
//	The upper layer will do its best for ^, which represents the beginning of a line, and $, which represents the end of a line.
//@}
//=========================================================================

class RegLexer
{
public:
	RegLexer( const wchar_t* pat, ulong len );
	RegToken GetToken();
	wchar_t  GetChar() const { return chr_; }

private:
	const wchar_t* pat_;
	const wchar_t* end_;
	const wchar_t* sub_;
	wchar_t        chr_;
};



//=========================================================================
//@{
//	Decomposition into tokens: implementation
//@}
//=========================================================================

inline RegLexer::RegLexer( const wchar_t* pat, ulong len )
	: pat_( pat )
	, end_( pat+len )
	, sub_( L"" )
	, chr_( L'\0' )
{
}

RegToken RegLexer::GetToken()
{
	const wchar_t*& x = (*sub_ ? sub_ : pat_);
	if( x == end_ ) return R_End;
	switch( *x++ )
	{
	case L'.': return R_Any;
	case L'[': return R_Lcl;
	case L']': return R_Rcl;
	case L'^': return R_Ncl;
	case L'-': return R_Range;
	case L'(': return R_Lbr;
	case L')': return R_Rbr;
	case L'|': return R_Bar;
	case L'*': return R_Star;
	case L'+': return R_Plus;
	case L'?': return R_Quest;
	case L'\\': if( x==end_ ) return R_End; switch( *x++ ) {
		case L'x': case L'X': // \xXX  : Latin-1 code point
		case L'u':            // \uXXXX: UCS-2 code point
		{
			const int maxDigits = (*(x-1)==L'u' ? 4 : 2);
			int ndigits = 0;
			wchar_t v = 0;
			for( ; ndigits<maxDigits && x<end_; ++ndigits, ++x )
			{
				const wchar_t ch = *x;
				if(      L'0'<=ch && ch<=L'9' ) v = 16*v + (ch-L'0');
				else if( L'A'<=ch && ch<=L'F' ) v = 16*v + (ch-L'A'+10);
				else if( L'a'<=ch && ch<=L'f' ) v = 16*v + (ch-L'a'+10);
				else break;
			}
			chr_ = ndigits ? v : *(x-1); // no hex digit: literal x/u
			return R_Char;
		}
		case L't': chr_=L'\t';            return R_Char;
		case L'n': chr_=L'\n';            return R_Char;
		case L'r': chr_=L'\r';            return R_Char;
		case L'f': chr_=L'\f';            return R_Char;
		case L'v': chr_=L'\v';            return R_Char;
		case L'a': chr_=L'\a';            return R_Char;
		case L'w': sub_=L"[0-9a-zA-Z_]";  return GetToken();
		case L'W': sub_=L"[^0-9a-zA-Z_]"; return GetToken();
		case L'd': sub_=L"[0-9]";         return GetToken();
		case L'D': sub_=L"[^0-9]";        return GetToken();
		case L's': sub_=L"[\t ]";         return GetToken();
		case L'S': sub_=L"[^\t ]";        return GetToken();
		} // fall through...
	default:
		chr_ = *(x-1);
		return R_Char;
	}
}



//=========================================================================
//@{
//	Types of values ​​assigned to syntax tree nodes
//@}
//=========================================================================

enum RegTypeEnum
{
	N_Char,     // Ordinary characters (ch)
	N_Class,    // [...] etc. (cls)
	N_Concat,   // conjunction (left, right)
	N_Or,       // |          (left, right)
	N_Closure,  // *          (left)
	N_Closure1, // +          (left)
	N_01,       // ?          (left)
	N_ClosureNg,  // *? non-greedy (left)
	N_Closure1Ng, // +? non-greedy (left)
	N_01Ng,       // ?? non-greedy (left)
	N_Empty    // empty (--)
};
typedef byte RegType;

struct RegClass: public Object
{
	struct OneRange
	{
		OneRange( wchar_t s, wchar_t e ): stt(s), end(e) {}
		wchar_t stt;
		wchar_t end;
	};
	OneRange      range;
	uptr<RegClass> next;
	RegClass( wchar_t s, wchar_t e, RegClass* n )
		: range( s, e ) , next( n ) {}
};

struct RegNode: public Object
{
	RegNode()
	: type ( N_Char )
	, cmpcls (false)
	, ch   ( L'\0' )
	, cls  ( NULL )
	, left (NULL)
	, right (NULL) { }

	explicit RegNode( RegType t, RegClass *kls, bool cmp )
	: type(t), cmpcls(cmp), ch(L'\0'), cls(kls)
	, left (NULL), right (NULL) { }

	~RegNode()
	{
		if( left )  delete left;
		if( right ) delete right;
	}
	RegType           type; // Type of this node
	bool            cmpcls; // ↑Whether it is a complement set or not
	wchar_t             ch; // character
	uptr<RegClass>     cls; // character set
	RegNode          *left; // the child on the left
	RegNode         *right; // child on the right
};



//=========================================================================
//@{
//	Syntax tree creation
//@}
//=========================================================================

class RegParser
{
public:
	RegParser( const unicode* pat );
	~RegParser() { delete root_; }
	RegNode* root() const { return root_; }
	bool err() { return err_; }
	bool hasLazy() const { return hasLazy_; }
	bool isHeadType() const { return isHeadType_; }
	bool isTailType() const { return isTailType_; }

private:
	RegNode* make_empty_leaf();
	RegNode* make_char_leaf( wchar_t c );
	RegNode* make_node( RegType t, RegNode* lft, RegNode* rht );
	void eat_token();
	RegNode* expr();
	RegNode* term();
	RegNode* factor();
	RegNode* primary();
	RegNode* reclass();

private:
	bool    err_;
	bool    hasLazy_;
	bool    isHeadType_;
	bool    isTailType_;
	RegNode *root_;

	RegLexer lex_;
	RegToken nextToken_;
};



//=========================================================================
//@{
//	Syntax tree creation: implementation
//@}
//=========================================================================

namespace { static int tmp; }

inline RegParser::RegParser( const unicode* pat )
	: err_       ( false )
	, hasLazy_   ( false )
	, isHeadType_( *pat==L'^' )
	, isTailType_( (tmp=my_lstrlenW(pat), tmp && pat[tmp-1]==L'$') )
	, lex_(
		(isHeadType_ ? pat+1 : pat),
		(my_lstrlenW(pat) - (isHeadType_ ? 1 : 0)
		                  - (isTailType_ ? 1 : 0)) )
{
	eat_token();
	root_ = expr();
}

inline void RegParser::eat_token()
{
	nextToken_ = lex_.GetToken();
}

inline RegNode* RegParser::make_empty_leaf()
{
	RegNode* node = new RegNode;
	if( node )
	{
		node->type = N_Empty;
	}
	return node;
}

inline RegNode* RegParser::make_char_leaf( wchar_t c )
{
	RegNode* node = new RegNode;
	if( node )
	{
		node->type = N_Char;
		node->ch   = c;
	}
	return node;
}

RegNode* RegParser::make_node( RegType t, RegNode* lft, RegNode* rht )
{
	RegNode* node = new RegNode;
	if( node )
	{
		node->type = t;
		node->left = lft;
		node->right= rht;
	}
	return node;
}

RegNode* RegParser::reclass()
{
//	CLASS   ::= '^'? CHAR (CHAR | -CHAR)*

	bool neg = false;
	if( nextToken_ == R_Ncl )
		neg=true, eat_token();

	RegClass* cls = NULL;
	while( nextToken_ == R_Char )
	{
		wchar_t ch = lex_.GetChar();
		eat_token();
		if( nextToken_ == R_Range )
		{
			eat_token();
			if( nextToken_ != R_Char )
				err_ = true;
			else
			{
				wchar_t ch2 = lex_.GetChar();
				cls = new RegClass( Min(ch,ch2), Max(ch,ch2), cls );
				eat_token();
			}
		}
		else
		{
			cls = new RegClass( ch, ch, cls );
		}
	}

	RegNode* node = new RegNode( N_Class, cls, neg );
	return node;
}

RegNode* RegParser::primary()
{
//	PRIMARY ::= CHAR
//              '.'
//	            '[' CLASS ']'
//				'(' REGEXP ')'

	RegNode* node;
	switch( nextToken_ )
	{
	case R_Char:
		node = make_char_leaf( lex_.GetChar() );
		eat_token();
		break;
	case R_Any:{
		node         = new RegNode( N_Class, new RegClass( 0, 65535, NULL ), false );
		eat_token();
		}break;
	case R_Lcl:
		eat_token();
		node = reclass();
		if( nextToken_ == R_Rcl )
			eat_token();
		else
			err_ = true;
		break;
	case R_Lbr:
		eat_token();
		node = expr();
		if( nextToken_ == R_Rbr )
			eat_token();
		else
			err_ = true;
		break;
	default:
		node = make_empty_leaf();
		err_ = true;
		break;
	}
	return node;
}

RegNode* RegParser::factor()
{
//	FACTOR  ::= PRIMARY
//	            PRIMARY '*'
//			    PRIMARY '+'
//			    PRIMARY '?'
//	            PRIMARY '*?' / '+?' / '??'  (non-greedy)

	RegNode* node = primary();
	RegType greedy, lazy;
	switch( nextToken_ )
	{
	case R_Star:  greedy=N_Closure;  lazy=N_ClosureNg;  break;
	case R_Plus:  greedy=N_Closure1; lazy=N_Closure1Ng; break;
	case R_Quest: greedy=N_01;       lazy=N_01Ng;       break;
	default: return node;
	}
	eat_token();
	if( nextToken_ == R_Quest )
	{
		hasLazy_ = true;
		node = make_node( lazy, node, NULL );
		eat_token();
	}
	else
	{
		node = make_node( greedy, node, NULL );
	}
	return node;
}

RegNode* RegParser::term()
{
//	TERM    ::= EMPTY
//	            FACTOR TERM

	if( nextToken_ == R_End )
		return make_empty_leaf();

	RegNode* node = factor();
	if( nextToken_==R_Lbr || nextToken_==R_Lcl
	 || nextToken_==R_Char|| nextToken_==R_Any )
		node = make_node( N_Concat, node, term() );
	return node;
}

RegNode* RegParser::expr()
{
//	REGEXP  ::= TERM
//	            TERM '|' REGEXP

	RegNode* node = term();
	if( nextToken_ == R_Bar )
	{
		eat_token();
		node = make_node( N_Or, node, expr() );
	}
	return node;
}



//=========================================================================
//@{
//	state transition
//@}
//=========================================================================

struct RegTrans : public Object
{
	enum TypeEnum { Epsilon, Class, Char };
	typedef byte Type;

	Type           type;
	bool         cmpcls;
	int              to; // Transition to state number to
	uptr<RegClass>  cls; // this character set
	                     // orEpsilon comes
	uptr<RegTrans> next; // linked list

//	RegTrans() {} ;
	explicit RegTrans(Type t, RegClass *rc, bool cmp, int tto, RegTrans *nx)
		: type(t), cmpcls(cmp), to(tto), cls(rc), next(nx) {}

/*
	template<class Cmp>
	bool match_i( wchar_t c, Cmp )
	{
		c = Cmp::map(c);
		RegClass* p = cls.get();
		while( p )
			if( Cmp::map(p->range.stt)<=c && c<=Cmp::map(p->range.end) )
				return true;
			else
				p = p->next.get();
		return false;
	}
*/
	bool match_c( wchar_t c ) const
	{
		for( RegClass* p=cls.get(); p; p=p->next.get() )
			if( p->range.stt<=c && c<=p->range.end )
				return true;
		return false;
	}

	bool match_i( wchar_t c ) const
	{
		c = IgnoreCase::map(c);
		for( RegClass* p=cls.get(); p; p=p->next.get() )
			if( IgnoreCase::map(p->range.stt)<=c
			 && c<=IgnoreCase::map(p->range.end) )
				return true;
		return false;
	}

	bool match( wchar_t c, bool caseS ) const
	{
		bool m = caseS ? match_c( c ) : match_i( c );
		return cmpcls ? !m : m;
	}
};



//=========================================================================
//@{
//	Syntax tree -> NFA conversion
//@}
//=========================================================================

class RegNFA: public Object
{
public:
	RegNFA( const wchar_t* pat );
	~RegNFA();

	int match( const wchar_t* str, int len, bool caseS, bool needEnd );
	bool isHeadType() const { return parser.isHeadType(); }
	bool isTailType() const { return parser.isTailType(); }

private:
	// Matching process
	int dfa_match( const wchar_t* str, int len, bool caseS );

	struct st_ele { int st, ps; };
	void push(storage<st_ele>& stack, int curSt, int pos);
	st_ele pop(storage<st_ele>& stack);

private:
	void add_transition( int from, wchar_t ch, int to );
	void add_transition( int from, RegClass *cls, bool cmp, int to );
	void add_e_transition( int from, int to );
	int gen_state();
	void gen_nfa( int entry, RegNode* t, int exit );

private:
	RegParser      parser;
	storage<RegTrans*> st;
	int      start, final;
};

RegNFA::RegNFA( const wchar_t* pat )
	: parser( pat ), st (16)
{
	start = gen_state();
	final = gen_state();
	gen_nfa( start, parser.root(), final );
}

inline RegNFA::~RegNFA()
{
	for( ulong i=0,e=st.size(); i<e; ++i )
		delete st[i];
}

inline void RegNFA::add_transition
	( int from, RegClass *cls, bool cmp, int to )
{
	st[from] = new RegTrans(RegTrans::Class, cls, cmp, to, st[from]);
}

inline void RegNFA::add_transition( int from, wchar_t ch, int to )
{
	add_transition( from, new RegClass(ch,ch,NULL), false, to );
}

inline void RegNFA::add_e_transition( int from, int to )
{
	st[from] = new RegTrans(RegTrans::Epsilon, NULL, false, to, st[from]);
}

inline int RegNFA::gen_state()
{
	st.Add( NULL );
	return st.size() - 1;
}

void RegNFA::gen_nfa( int entry, RegNode* t, int exit )
{
	switch( t->type )
	{
	case N_Char:
		//         ch
		//  entry ----> exit
		add_transition( entry, t->ch, exit );
		break;
	case N_Class:
		//         cls
		//  entry -----> exit
		add_transition( entry, t->cls.release(), t->cmpcls, exit );
		break;
	case N_Concat: {
		//         left         right
		//  entry ------> step -------> exit
		int step = gen_state();
		gen_nfa( entry, t->left, step );
		gen_nfa( step, t->right, exit );
		} break;
	case N_Or:
		//          left
		//         ------>
		//  entry ------->--> exit
		//          right
		gen_nfa( entry, t->left, exit );
		gen_nfa( entry, t->right, exit );
		break;
	case N_Closure:
		//                       e
		//         e          <------        e
		//  entry ---> before ------> after ---> exit
		//    |                left                ^
		//    >------->------------------->------>-|
		//                      e
	case N_Closure1:
		//                       e
		//         e          <------        e
		//  entry ---> before ------> after ---> exit
		//                     left
	case N_ClosureNg:
	case N_Closure1Ng: {
		// The first transition added from a state is tried first.
		// Greedy prefers looping/consuming, non-greedy prefers exiting.
		const bool lazy = (t->type==N_ClosureNg || t->type==N_Closure1Ng);
		const bool star = (t->type==N_Closure   || t->type==N_ClosureNg);
		int before = gen_state();
		int after = gen_state();
		if( !lazy )
		{
			add_e_transition( entry, before );
			add_e_transition( after, before );
			add_e_transition( after, exit );
			gen_nfa( before, t->left, after );
			if( star )
				add_e_transition( entry, exit );
		}
		else
		{
			if( star )
				add_e_transition( entry, exit );
			add_e_transition( after, exit );
			add_e_transition( after, before );
			gen_nfa( before, t->left, after );
			add_e_transition( entry, before );
		}
		} break;
	case N_01:
		//           e
		//        ------>
		//  entry ------> exit
		//         left
		// greedy: prefer consuming
		gen_nfa( entry, t->left, exit );
		add_e_transition( entry, exit );
		break;
	case N_01Ng:
		// non-greedy: prefer skipping
		add_e_transition( entry, exit );
		gen_nfa( entry, t->left, exit );
		break;
	case N_Empty:
		//         e
		//  entry ---> exit
		add_e_transition( entry, exit );
		break;
	}
}



//=========================================================================
//@{
//	matching
//@}
//=========================================================================

void RegNFA::push(storage<st_ele>& stack, int curSt, int pos)
{
	// ε Infinite loop prevention measures. Don't go back to the same situation...
	for( int i=stack.size()-1; i>=0; --i )
		if( stack[i].ps != pos )
			break;
		else if( stack[i].st == curSt )
			return;

	st_ele nw = {curSt,pos};
	stack.Add( nw );
}

RegNFA::st_ele RegNFA::pop(storage<st_ele>& stack)
{
	st_ele se = stack[stack.size()-1];
	stack.ForceSize( stack.size()-1 );
	return se;
}

// needEnd: only a match that reaches the end of str is acceptable
// (pattern anchored with '$', or a full-string match is required).
int RegNFA::match( const wchar_t* str, int len, bool caseS, bool needEnd )
{
	if( parser.err() )
		return -1; // I can't match it because it's in an error state.
	//if( st.size() <= 31 )
	//	return dfa_match(str,len,caseS); // Maybe use DFA if the number of states is small

	if( parser.hasLazy() )
	{
		// Non-greedy pattern: transitions are explored in preference order,
		// so the first acceptable match found is the right one.
		storage<st_ele> stack(16);
		push(stack, start, 0);
		while( stack.size() > 0 )
		{
			st_ele se = pop(stack);
			if( se.st == final && (!needEnd || se.ps == len) )
				return se.ps;
			for( RegTrans* tr=st[se.st]; tr!=NULL; tr=tr->next.get() )
			{
				if( tr->type == RegTrans::Epsilon )
					push(stack, tr->to, se.ps);
				else if( se.ps<len && tr->match( str[se.ps], caseS ) )
					push(stack, tr->to, se.ps+1);
			}
		}
		return -1;
	}

	// Greedy only: the longest match
	int matchpos = -1;

	storage<st_ele> stack(16);
	push(stack, start, 0);
	while( stack.size() > 0 )
	{
		// pop from stack
		st_ele se = pop(stack);
		int curSt = se.st;
		int   pos = se.ps;

		// Record if the match is successful
		if( curSt == final ) // 1==end state
			if( matchpos < pos )
				matchpos = pos;

		// Explore further transitions
		if( matchpos < len )
		{
			for( RegTrans* tr=st[curSt]; tr!=NULL; tr=tr->next.get() )
			{
				if( tr->type == RegTrans::Epsilon )
					push(stack, tr->to, pos);
				else if( pos<len && tr->match( str[pos], caseS ) )
					push(stack, tr->to, pos+1);
			}
		}
	}

	return matchpos;
}

int RegNFA::dfa_match( const wchar_t* str, int len, bool caseS )
{
	int matchpos = -1;

	unsigned int StateSet = (1<<start);
	for(int pos=0; StateSet; ++pos)
	{
		// ε-closure
		for(uint DifSS=StateSet; DifSS;)
		{
			unsigned int NewSS = 0;
			for(int s=0; (1u<<s)<=DifSS; ++s)
				if( (1u<<s) & DifSS )
					for( RegTrans* tr=st[s]; tr!=NULL; tr=tr->next.get() )
						if( tr->type == RegTrans::Epsilon )
						 NewSS |= 1u << tr->to;
			DifSS = (NewSS|StateSet) ^ StateSet;
			StateSet |= NewSS;
		}

		// Determine whether the acceptance state is included.
		if( StateSet & (1<<final) )
			matchpos = pos;

		// reached the end of the string
		if( pos == len )
			break;

		// transition
		unsigned int NewSS = 0;
		for(int s=0; (1u<<s)<=StateSet; ++s)
			if( (1u<<s) & StateSet )
				for( RegTrans* tr=st[s]; tr!=NULL; tr=tr->next.get() )
					if( tr->type!=RegTrans::Epsilon && tr->match(str[pos], caseS) )
					 NewSS |= 1u << tr->to;
		StateSet = NewSS;
	}

	return matchpos;
}

//////////////////////////////////////////////////////////////////////

bool reg_match( const wchar_t* pat, const wchar_t* str, bool caseS )
{
	int len = my_lstrlenW(str);

	RegNFA re( pat );
	return len == re.match( str, len, caseS, true );
}



//=========================================================================
//@{
//	Search object for GreenPad
//@}
//=========================================================================

RSearch::RSearch( const unicode* key, bool caseS, bool down )
	: re_    ( new RegNFA(key) )
	, caseS_ ( caseS )
	, down_  ( down )
{
}

RSearch::~RSearch()
{
	delete re_;
}

bool RSearch::Search(
	const unicode* str, ulong len, ulong stt, ulong* mbg, ulong* med )
{
	if( down_ && re_->isHeadType() && stt>0 )
		return false;

	const int d = (down_ ? 1 : -1);
	      int s = (!down_ && re_->isHeadType() ? 0 : stt);
	const int e = (down_ ? (re_->isHeadType() ? 1 : (long)len) : -1);

	for( ; s!=e; s+=d )
	{
		const int L = re_->match( str+s, len-s, caseS_, re_->isTailType() );
		if( L >= 0 )
		{
			if( re_->isTailType() && L!=static_cast<int>(len-s) )
				continue;
			*mbg = static_cast<ulong>(s);
			*med = static_cast<ulong>(s+L);
			return true;
		}
	}

	return false;
}
