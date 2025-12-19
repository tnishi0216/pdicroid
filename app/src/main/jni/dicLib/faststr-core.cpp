#if !defined(max)
#define	max(i,j)	((i)>(j)?(i):(j))
#endif

#define	USE_QS		1	// Sunday Quick Search

#if defined(FS_WIDE)
#define	s2_single	s2
#define	skip_single	skip
#endif

// word access macro
#ifdef SMALL
#define	GET_HWORD(p)		(((ushort)(((uchar*)(p))[0])) | (((ushort)(((uchar*)(p))[0]))<<8))	// for little-endian
#else
#define	GET_HWORD(p)		(*(ushort*)(p))
#endif

// 大文字・小文字区別の場合は、SingleByteも同じだが、
// 同一視の場合は異なってしまう
// patternが""(空)である場合の動作は不定！
FS_NAME(FindStr)::FS_NAME(FindStr)( const _fchar *pattern, bool fIgnoreCase )
{
	int i;

	n2 = _fcslen(pattern);
	if ( n2 == 0 ){
		s2 = NULL;
#if !defined(FS_WIDE)
		s2_single = s2;
		size = NULL;
#endif
		return;
	}
#if USE_QS
	if ( n2 > MAX_FINDSTR-1 ) n2 = MAX_FINDSTR-1;
#else
	if ( n2 > MAX_FINDSTR ) n2 = MAX_FINDSTR;
#endif
	s2 = new _fuchar[ n2 + 1 ];
#if USE_QS
	memset( skip, n2+1, MAX_FINDSTR );
#else
	memset( skip, n2, MAX_FINDSTR );
#endif
#if !defined(FS_WIDE)
	size = new uchar[ n2 + 1 ];
	memset( skip_single, n2, MAX_FINDSTR );
#endif

	if ( fIgnoreCase ){
		_fuchar *dp;
		dp = s2;
		const _fchar *p = pattern;
#if !defined(FS_WIDE)
		unsigned char *s = size;
#endif
		for ( i=0;i<n2;i++ ){
			_fchar *np = _fnext( p );
#if !defined(FS_WIDE)
			if ( np == p+1 )
#endif
			{
#if USE_QS
				skip[A_lower(*p)] = n2 - i;
#else
				if ( i < n2 - 1 )
					skip[A_lower(*p)] = (n2 - i - 1);
#endif
				*dp++ = A_lower(*p);
#if !defined(FS_WIDE)
				*s++ = 1;
#endif
			}
#if !defined(FS_WIDE)
			else {
				skip[(uchar)*p] = (uchar)(n2 - i - 1);
				i++;
				if ( i < n2 - 1 ){
					skip[(uchar)*(p+1)] = (uchar)(n2 - i - 1);
				}
				*(*(WORD**)&dp)++ = *(WORD*)p;
				*(*(WORD**)&s)++ = 0;
			}
#endif
			p = np;
		}
		*dp = '\0';
		dp = s2_single
#if !defined(FS_WIDE)
			= new unsigned char[ n2 + 1 ]
#endif
		;
		for ( i=0;i<n2-1;i++ ){
#if !defined(FS_WIDE)
			skip_single[(_fuchar)A_lower(pattern[i])] = (short)(n2 - i - 1);
#endif
			*dp++ = A_lower(pattern[i]);
		}
		*dp++ = A_lower(pattern[n2-1]);
		*dp = '\0';
	} else {
#if USE_QS
		for ( i=0;i<n2;i++ ){
			skip[pattern[i]] = n2 - i;
		}
#else
		for ( i=0;i<n2-1;i++ ){
			skip[pattern[i]] = (n2 - i - 1);
#if !defined(FS_WIDE)
			skip_single[(_fchar)pattern[i]] = (short)(n2 - i - 1);
#endif
		}
#endif
		memcpy( s2, pattern, _flentobyte(n2 + 1) );
#if !defined(FS_WIDE)
		s2_single = s2;
#endif
	}

	e2 = s2 + n2 - 1;
#if !defined(FS_WIDE)
	size_e = size + n2 - 1;
#endif
}

FS_NAME(FindStr)::~FS_NAME(FindStr)()
{
#if !defined(FS_WIDE)
	if ( s2 != s2_single ) delete[] s2_single;
#endif
	if ( s2 ) delete[] s2;
#if !defined(FS_WIDE)
	if ( size ) delete[] size;
#endif
}

#ifndef FS_WIDE
_fchar *jfstrichr( const _fchar *s1, _fuchar c )
{
	while ( *s1 ){
		if ( (_fuchar)_flower(*s1) == c ) return (_fchar*)s1;
		s1 = _fnext( s1 );
	}
	return NULL;
}
#if 0
_fchar *jfstristr( const _fchar *s1, int n1, FS_NAME(FindStr) &fs )
{
	if ( fs.n2 == 1 ) return jfstrichr( s1, fs.s2[0] );

#if USE_QS
#error	// not implemented.
	const _fchar *e1 = s1 + n1 - fs.n2;

	if ( s1 <= e1 ){
		do {
			_fuchar c;
			const _fuchar *s2 = fs.s2;
			const _fuchar *size = fs.size_e;	//TODO: これがよくわからない。。そもそも分ける必要あり？
			for (int j=0;;){
				if ( *size )
					c = _flower(*s1);
				else
					c = *s1;
				j++;
				if ( c != *s2 )
					break;
				if (j==fs.n2)
					return (_fchar*)s1;
				s2++;
				size++;
			}
			if (s1 >= e1) break;
			c = s1[fs.n2];
			s1 += fs.skip[ *size ? _flower(c) : c ];	//TODO: *size
		} while(1);
	}
#else
	const _fchar *e1 = s1 + n1;
	s1 += fs.n2 - 1;
	const _fuchar *e2 = fs.e2;

	while ( s1 < e1 ){
		_fuchar c;
		const _fuchar *s2 = e2;
		const _fuchar *size = fs.size_e;
		while ( 1 ){
			if ( *size )
				c = _flower(*s1);
			else
				c = *s1;
			if ( c != *s2 )
				break;
			if ( s2 == fs.s2 )
				return (_fchar*)s1;
			s1--;
			s2--;
			size--;
		}
		s1 += max(fs.skip[c], _fdiff(e2,s2)+1);
	}
#endif
	return NULL;
}
#endif
_fchar *jfstrchr( const _fchar *s1, _fuchar c )
{
	while ( *s1 ){
		if ( (_fuchar)*s1 == c ) return (_fchar*)s1;
		s1 = _fnext( s1 );
	}
	return NULL;
}
// 日本語１文字の検索
_fchar *jfstrmulti( const _fchar *s1, ushort s )
{
	while ( *s1 ){
		if ( GET_HWORD(s1) == s ) return (_fchar*)s1;
		s1 = _fnext( s1 );
	}
	return NULL;
}
#if 0
_fchar *jfstrstr( const _fchar *s1, int n1, FS_NAME(FindStr) &fs )
{
	if ( fs.n2 == 1 ) return jfstrchr( s1, fs.s2[0] );
	if ( fs.n2 == 2 ) return jfstrmulti( s1, GET_HWORD(&fs.s2[0]) );

#if USE_QS
#error	// not implemented
	const _fchar *e1 = s1 + n1 - fs.n2;

	if ( s1 <= e1 ){
		do {
			_fuchar c;
			const _fuchar *s2 = fs.s2;
			for (int j=0;;){
				c = s1[j++];
				if ( c != *s2 )
					break;
				if (j==fs.n2)
					return (_fchar*)s1;
				s2++;
			}
			if (s1 >= e1) break;
			s1 += fs.skip[s1[fs.n2]];
		} while(1);
	}
#else
	const _fchar *e1 = s1 + n1;
	s1 += fs.n2 - 1;
	const _fuchar *e2 = fs.e2;

	while ( s1 < e1 ){
		const _fuchar *s2 = e2;
		while ( (_fuchar)*s1 == (_fuchar)*s2 ){
			if ( s2 == fs.s2 )
				return (_fchar*)s1;
			s1--;
			s2--;
		}
		s1 += max(fs.skip[(_fuchar)*s1], _fdiff(e2,s2) + 1);
	}
#endif
	return NULL;
}
#endif
#endif	// !defined(FS_WIDE)
#if !defined(WINCE)
_fchar *jfstrstr( const _fchar *s1, const _fchar *s2, int n2 )
{
#if 1	// さらに高速化
	size_t n1 = _fcslen( s1 );
	if ( n1 == 0 || (size_t)n2 > n1)
		return NULL;
	if ( n2 == 0 ){
		// error!!!!
		return (_fchar*)s1;	// NULLのほうがよい？？？
	}

	_fuchar c1 = (_fuchar)*s2;
#ifndef FS_WIDE
	_fuchar c2 = (_fuchar)*(s2+1);
	bool fAlpha = A_isalpha(c1);
#endif

	const _fuchar *p;
	const _fuchar *q;
	const _fchar *e = s1+n1-n2;
	while ( 1 ){
#ifndef FS_WIDE
		if ( fAlpha )
#endif
		{
			while ( 1 ){
				if ( s1 > e ){
					return NULL;
				}
				if ( (_fuchar)*s1 == c1 ){
					p = (const _fuchar *)s1+1;
					q = (const _fuchar *)s2+1;
					break;
				}
				s1 = _fnext(s1);
			}
		}
#ifndef FS_WIDE
		else {
			while ( 1 ){
				if ( s1 > e ){
					return NULL;
				}
				if ( (_fuchar)*s1 == c1 ){
					if ( (_fuchar)*(s1+1) == c2 ){
						p = (const _fuchar *)s1+2;
						q = (const _fuchar *)s2+2;
						break;
					}
				}
				s1 = _fnext(s1);
			}
		}
#endif
		while ( *p ){
			if ( *p != *q ){
				break;
			}
			// 高速化の余地あり?
			p++;
			q++;
#ifndef FS_WIDE
			if ( p != (const _fuchar*)_fnext( (const _fchar *)(p-1) ) ){
				if ( *p != *q )
					break;
				p++;
				q++;
			}
#endif
		}
		if ( !*q )
			return (_fchar *)s1;
		s1 = _fnext(s1);
	}
#else	// 0
	size_t n1;
	const _fchar *e;

	n1 = _fcslen(s1);
	if (n1 == 0 || n2 > n1)
		return NULL;

	for (e = s1 + n1 - n2; s1 <= e; s1 = A_next( s1 ) ) {
		const _fuchar *p = (const _fuchar*)s1;
		const _fuchar *q = (const _fuchar*)s2;
		while ( *p && (_fuchar)*p == (_fuchar)*q )
			p++, q++;
		if (*q == '\0')
			return (_fchar *)s1;
	}
	return NULL;
#endif	// 0
}
#endif	// !WINCE

_fchar *fstrichr( const _fchar *s1, FS_NAME(FindStr) &fs )
{
	while ( *s1 ){
		if ( (_fuchar)A_lower(*s1) == (_fuchar)fs.s2[0] ) return (_fchar*)s1;
		s1++;
	}
	return NULL;
}

_fchar *fstristr( const _fchar *s1, int n1, FS_NAME(FindStr) &fs )
{
	if ( fs.n2 == 1 ) return fstrichr( s1, fs );
#if 0	// should be deleted.
	if ( n1 == 0 || fs.n2 > n1 )
		return NULL;

	__assert(fs.n2);

	_fuchar c1 = (_fuchar)*fs.s2;

	const _fuchar *p;
	const _fuchar *q;
	const _fchar *e = s1+n1-fs.n2;
	while ( 1 ){
		while ( 1 ){
			if ( s1 > e ){
				return NULL;
			}
			if ( (_fuchar)A_lower(*s1) == c1 ){
				p = (const _fuchar *)s1+1;
				q = (const _fuchar *)fs.s2+1;
				break;
			}
			s1 = _fnext(s1);
		}
		while ( *p ){
			if ( *p != *q ){
				if ( A_lower( *p ) != A_lower( *q ) ){
					break;
				}
				p++, q++;
				continue;
			}
			// 高速化の余地あり
			// bug fix 2.12
			_fuchar *np = _fnext( p );
			p++;
			q++;
			if ( np != p ){
				if ( *p != *q ) break;
				p++;
				q++;
			}
		}
		if ( !*q )
			return (_fchar *)s1;
		s1 = _fnext(s1);
	}
#else
#if USE_QS
	const _fchar *e1 = s1 + n1 - fs.n2;

	if ( s1 <= e1 ){
		do {
			_fuchar c;
			const _fuchar *s2 = fs.s2_single;
			for (int j=0;;){
				c = A_lower(s1[j++]);
				if ( c != *s2 )
					break;
				if (j==fs.n2)
					return (_fchar*)s1;
				s2++;
			}
			if (s1 >= e1) break;
			s1 += fs.skip_single[A_lower(s1[fs.n2])];
		} while(1);
	}
#else
	const _fchar *e1 = s1 + n1;
	s1 += fs.n2 - 1;
	const _fuchar *e2 = fs.s2_single + fs.n2 - 1;

	while ( s1 < e1 ){
		_fuchar c;
		const _fuchar *s2 = e2;
		while ( 1 ){
			c = A_lower(*s1);
			if ( c != *s2 )
				break;
			if ( s2 == fs.s2_single )
				return (_fchar*)s1;
			s1--;
			s2--;
		}
		s1 += max(fs.skip_single[c], _fdiff(e2,s2)+1);
	}
#endif
	return NULL;
#endif
}

// single byte用strstr
_fchar *fstrstr( const _fchar *s1, int n1, FS_NAME(FindStr) &fs )
{
	if ( fs.n2 == 1 ) return _fcschr( (_fchar*)s1, fs.s2[0] );
#if 0	// should be deleted.
	if ( n1 == 0 || fs.n2 > n1 || fs.n2 == 0 )
		return NULL;

	_fuchar c1 = (_fuchar)*fs.s2;

	const _fuchar *p;
	const _fuchar *q;
	const _fchar *e = s1+n1-fs.n2;
	while ( 1 ){
		while ( 1 ){
			if ( s1 > e ){
				return NULL;
			}
			if ( (_fuchar)*s1 == c1 ){
				p = (const _fuchar *)s1+1;
				q = (const _fuchar *)fs.s2+1;
				break;
			}
			s1 = _fnext(s1);
		}
		while ( *p ){
			if ( *p != *q ){
				break;
			}
			// 高速化の余地あり?
			p++;
			q++;
			if ( p != (const _fuchar*)_fnext( (const _fchar *)p ) ){
				if ( *p != *q )
					break;
				p++;
				q++;
			}
		}
		if ( !*q )
			return (_fchar *)s1;
		s1 = _fnext(s1);
	}
#else
#if USE_QS
	const _fchar *e1 = s1 + n1 - fs.n2;

	if ( s1 <= e1 ){
		do {
			_fuchar c;
			const _fuchar *s2 = fs.s2_single;
			for (int j=0;;){
				c = s1[j++];
				if ( c != *s2 )
					break;
				if (j==fs.n2)
					return (_fchar*)s1;
				s2++;
			}
			if (s1 >= e1) break;
			s1 += fs.skip_single[s1[fs.n2]];
		} while (1);
	}
#else
	const _fchar *e1 = s1 + n1;
	s1 += fs.n2 - 1;
	const _fuchar *e2 = fs.e2;

	while ( s1 < e1 ){
		const _fuchar *s2 = e2;
		while ( (_fuchar)*s1 == (_fuchar)*s2 ){
			if ( s2 == fs.s2 )
				return (_fchar*)s1;
			s1--;
			s2--;
		}
		s1 += max(fs.skip[*(_fuchar*)s1], _fdiff(e2,s2)+1);
	}
#endif
	return NULL;
#endif
}

#ifndef __NO_STRICMP
#if !defined(_UNICODE) && defined(_WINDOWS)
int jfstricmp( const char *s1, const char *s2 )
{
	return CompareString( GetSystemDefaultLCID(), NORM_IGNORECASE, s1, -1, s2, -1 ) - 2;
}
#else	// defined(FS_WIDE) || !defined(_WINDOWS)
int jfstricmp( const char *s1, const char *s2 )
{
	int l1 = strlen(s1);
	int l2 = strlen(s2);
	if ( l1 < l2 ) return -1;
	int r;
	while(l2){
		r = *(unsigned char*)s1 - *(unsigned char*)s2;
		if ( r != 0 ) return r;
		l2--;
	}
	return 0;
}
#endif	// defined(FS_WIDE) || !defined(_WINDOWS)
#endif	// __NO_STRICMP

// DOS,Windows
_fchar *jfstrrchr(const _fchar *s, unsigned short c)
{
	const _fchar *p = NULL;
	do {
#ifndef FS_WIDE
		if (_ismbblead(*(unsigned char *)s))
		{
			if (s[1] == '\0')
			{
				s++;
				if (c == 0)
					p = s;
				break;
			}
			else
			{
				if (((((unsigned int)*(unsigned char *)s) << 8) | s[1]) == c)
					p = s;
				s++;
			}
		}
		else
#endif
		{
			if ((_fuchar)*s == c)
				p = s;
		}
	} while (*s++) ;
	return (_fchar *)p;
}
#if 0
#ifndef __NO_STRRCHR
#if MIXDIC
wchar_t *jfstrrchr(const wchar_t *s, wchar_t c)
{
	const wchar_t *p = NULL;
	do {
		if (*s == c)
			p = s;
	} while (*s++) ;
	return (wchar_t*)p;
}
#endif
#endif	// __NO_STRRCHR
#endif

#if 0

/*======================================================================*/
/* Windows用 FastStr関数												*/
/*======================================================================*/

char *jfstrstr( const char *s1, const char *s2, int n2 )
{
#if 1	// さらに高速化
	size_t n1 = strlen( s1 );
	if ( n1 == 0 || n2 > n1)
		return NULL;
	if ( n2 == 0 ){
		// error!!!!
		return (char*)s1;	// NULLのほうがよい？？？
	}

	_fuchar c1 = (_fuchar)*s2;
	_fuchar c2 = (_fuchar)*(s2+1);
	bool fAlpha = A_isalpha(c1);

	const _fuchar *p;
	const _fuchar *q;
	const char *e = s1+n1-n2;
	while ( 1 ){
		if ( fAlpha ){
			while ( 1 ){
				if ( s1 > e ){
					return NULL;
				}
				if ( (uint8_t)*s1 == c1 ){
					p = (const uchar *)s1+1;
					q = (const uchar *)s2+1;
					break;
				}
				s1 = M_next(s1);
			}
		} else {
			while ( 1 ){
				if ( s1 > e ){
					return NULL;
				}
				if ( (uchar)*s1 == c1 ){
					if ( (uchar)*(s1+1) == c2 ){
						p = (const uchar *)s1+2;
						q = (const uchar *)s2+2;
						break;
					}
				}
				s1 = M_next(s1);
			}
		}
		while ( *p ){
			if ( *p != *q ){
				break;
			}
			// 高速化の余地あり?
			p++;
			q++;
			if ( p != (const uchar*)M_next( (const char *)p ) ){
				if ( *p != *q )
					break;
				p++;
				q++;
			}
		}
		if ( !*q )
			return (char *)s1;
		s1 = M_next(s1);
	}
#else
    size_t n1;
	const char *e;

	n1 = strlen(s1);
	if (n1 == 0 || n2 > n1)
		return NULL;

	for (e = s1 + n1 - n2; s1 <= e; s1 = M_next( s1 ) ) {
		const unsigned char *p = (const uchar*)s1;
		const unsigned char *q = (const uchar*)s2;
		while ( *p && *p == *q )
			p++, q++;
		if (*q == '\0')
			return (char *)s1;
	}
	return NULL;
#endif
}

// シングルバイト版
char *fstrstr( const char *s1, const char *s2, int n2 )
{
	size_t n1;
	const char *e;

	n1 = strlen(s1);
	if (n1 == 0 || n2 > n1)
		return NULL;

	for (e = s1 + n1 - n2; s1 <= e; s1++ ) {
		const unsigned char *p = (const uchar*)s1;
		const unsigned char *q = (const uchar*)s2;
		while ( *p && *p == *q )
			p++, q++;
		if (*q == '\0')
			return (char *)s1;
	}
	return NULL;
}

#ifndef FS_WIDE
char *jfstristr( const char *s1, const char *s2, int n2 )
{
#if 1	// さらに高速化
	size_t n1 = _fcslen( s1 );
	if ( n1 == 0 || n2 > n1)
		return NULL;

	uchar c1 = (uchar)*s2;
	uchar c2 = (uchar)*(s2+1);
	bool fAlpha = A_isalpha(c1);
	if ( fAlpha )
		c1 = A_lower(c1);

	const uchar *p;
	const uchar *q;
	const char *e = s1+n1-n2;
	while ( 1 ){
		if ( fAlpha ){
			while ( 1 ){
				if ( s1 > e ){
					return NULL;
				}
				if ( (uint8_t)A_lower(*s1) == c1 ){
					p = (const uchar *)s1+1;
					q = (const uchar *)s2+1;
					break;
				}
				s1 = M_next(s1);
			}
		} else {
			while ( 1 ){
				if ( s1 > e ){
					return NULL;
				}
				if ( (uint8_t)*s1 == c1 ){
					if ( (uchar)*(s1+1) == c2 ){
						p = (const uchar *)s1+2;
						q = (const uchar *)s2+2;
						break;
					}
				}
				s1 = M_next(s1);
			}
		}
		while ( *p ){
			if ( *p != *q ){
				if ( A_isalpha( *p ) ){
					if ( A_lower( *p ) != A_lower( *q ) ){
						break;
					}
					p++, q++;
					continue;
				} else {
					break;
				}
			}
			// 高速化の余地あり
			// bug fix 2.12
			char *np = M_next( p );
			p++;
			q++;
			if ( np != p ){
				if ( *p != *q ) break;
				p++;
				q++;
			}
		}
		if ( !*q )
			return (char *)s1;
		s1 = M_next(s1);
	}
#else
	size_t n1;
	const char *e;

	n1 = strlen( s1 );
	if (n1 == 0 || n2 > n1)
		return NULL;

	for (e = s1 + n1 - n2; s1 <= e; s1 = M_next( s1 ) ) {
		const uchar *p = (const uchar*)s1;
		const uchar *q = (const uchar*)s2;
		while ( *p ){
			if ( *p != *q ){
				if ( A_isalpha( *p ) ){
					if ( A_lower( *p ) != A_lower( *q ) ){
						break;
					}
					p++, q++;
					continue;
				} else {
					break;
				}
			}
			// 高速化の余地あり
			const uchar *np = (const uchar*)M_next( (const char*)p );
//			const uchar *nq = (const uchar*)M_next( (const char*)q );
			p++;
			q++;
			while ( p != np ){
				if ( *p != *q )
					break;
				p++;
				q++;
			}
		}
		if (*q == '\0')
			return (char *)s1;
	}
	return NULL;
#endif
}
#endif

// シングルバイト版
char *fstristr( const char *s1, const char *s2, int n2 )
{
	size_t n1;
	const char *e;

	n1 = strlen( s1 );
	if (n1 == 0 || n2 > n1)
		return NULL;

	for (e = s1 + n1 - n2; s1 <= e; s1++ ) {
		const uchar *p = (const uchar*)s1;
		const uchar *q = (const uchar*)s2;
		while ( *p ){
			if ( *p != *q ){
				if ( A_isalpha( *p ) ){
					if ( A_lower( *p ) != A_lower( *q ) ){
						break;
					}
					p++, q++;
					continue;
				} else {
					break;
				}
			}
			p++;
			q++;
		}
		if (*q == '\0')
			return (char *)s1;
	}
	return NULL;
}

int jcharcmp( const tchar *s1, const tchar *s2 )
{
	tuchar c1 = *(tuchar*)s1;
	tuchar c2 = *(tuchar*)s2;
	if ( c1 < c2 ){
		return -1;
	} else if ( c1 > c2 ){
		return 1;
	}
	const tchar *np1 = GetNextPtr( s1 );
	if ( _fdiff( np1, s1 ) == 1 ){
		return 0;
	} else {
		c1 = *(tuchar*)(s1+1);
		c2 = *(tuchar*)(s2+1);
		if ( c1 < c2 ){
			return -1;
		} else if ( c1 > c2 ){
			return 1;
		} else {
			return 0;
		}
	}
}

#ifndef FS_WIDE
// DOS,Windows
// ただ単に、BC++4.5のバグに対処するため
char * jfstrchr(const char *s, unsigned short c)
{
	for ( ; ; s++)
	{
		if (iskanji(*(unsigned char *)s))
		{
			if (s[1] == '\0')
			{
				if (c == 0)
				{
					s++;
					return (char *)s;
				}
				break;
			}
			if (((((unsigned int)*(unsigned char *)s) << 8) | s[1]) == c)
				return (char *)s;
			s++;
		}
		else
		{
			if (*(unsigned char *)s == c)
				return (char *)s;
			if (*s == '\0')
				break;
		}
	}
	return NULL;
}
#endif

#endif

