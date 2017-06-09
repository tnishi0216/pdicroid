//---------------------------------------------------------------------------

#include "tnlib.h"
#pragma hdrstop

#include "LangProcDef.h"
#include "LangProcStd.h"
#include "ktable.h"
#include "pdprof.h"
#include "pdtypes.h"
#include "filestr.h"
#ifdef _Windows
#include "WebSrchTh.h"
#endif
#include "wordcount.h"

//---------------------------------------------------------------------------

#pragma package(smart_init)

#define	NGRAM		2

#define	DEF_LANGPROC_FILE	_T("DefLangProc.ctt")
#define	DEF_SRCHPAT_FILE	_T("SrchPat.ctt")

//TODO: move to LangProcDef.cpp
inline bool IsPhaseDelim(tchar c)
{
	return c=='.' || c==',' || c=='"' || c=='(' || c==')';
}

//TODO: move to common
void trimright(tnstr &s);

//---------------------------------------------------------------------------
//	TLangProcStd
//---------------------------------------------------------------------------
TLangProcStd::TLangProcStd()
{
}
TLangProcStd::~TLangProcStd()
{
}

bool TLangProcStd::Open()
{
	SrchPatFile = MakePath(prof.GetCTTPath(), DEF_SRCHPAT_FILE);
	tnstr path = MakePath(prof.GetCTTPath(), DEF_LANGPROC_FILE);
	return super::Open(path);
}

bool TLangProcStd::OpenIrreg()
{
	AddIrregFile(prof.GetDefIrregDicName());
	AddIrregFile(prof.GetAdditionalIrregDicName());
	return super::OpenIrreg();
}

//TODO: 辞書側のkeyword機能によりある程度絞り込まれる
// ここでの検索で不要な処理を高速化のため削除する必要がある（ハイフン処理など）

#include "windic.h"

#define	OLDCASE	0	// using old type dictionary without ignoring cases.

#if 0
#define	D	DBW		// for debug
#else
#define	D	(void)
#endif

// Prerequisite : Call cs.dic->OpenIrregDic().
// return : true : 続ける
//          false : エラー＝中止
bool TLangProcStd::CompareStd( COMPARE_STRUCT &cs, const int _flags )
{
	tuchar c;
	int convf = 0;
	tchar *dp, *tdp;
	tchar dp1, dp3, dp4;
	dl_def(dicflag);
	const tchar *sp;
	const tchar *ndp = NULL;	// next delim pointer
	if ( _flags & SLW_REPLACEANY ){
		if ( _flags != SLW_REPLACEANY )
			return true;
		// ~による置換
		dp = cs.dp;
		if ( *(dp-2) == '~' )
			return true;
		if (cs.str == dp)	// no string
			return true;
		*dp++ = '~';
		*dp = '\0';
		convf |= SLW_REPLACEANY;
		goto jsrch;
	}

	if ( _flags & SLW_REPLACEANY3 ){
		if ( _flags != SLW_REPLACEANY3 )
			return true;
		// ~による置換
		dp = cs.dp;
		if ( *(dp-2) == '_' )
			return true;
		if (cs.str == dp)	// no string
			return true;
		*dp++ = '_';
		*dp++ = '_';
		*dp = '\0';
		convf |= SLW_REPLACEANY3;
		goto jsrch;
	}

	dp = cs.dp;
	sp = cs.sp;

	if ( _flags & SLW_REPLACEANY2 ){
//		continue;
		// ~の追加
		if ( *(dp-2) == '~' )
			return true;
		*dp++ = '~';
		*dp++ = ' ';
		convf |= SLW_REPLACEANY2;
	}

#if OLDCASE
	// 先頭case反転
	if ( _flags & SLW_CASEIGNORE1 ){
		if ( (tuchar)*sp >= 'A' && (tuchar)*sp <= 'Z' ){
			*dp++ = (tchar)_totlower(*sp++);
			convf |= SLW_CASEIGNORE1;
		} else
		if ( (tuchar)*sp >= 'a' && (tuchar)*sp <= 'z' ){
			*dp++ = (tchar)__totupper(*sp++);
			convf |= SLW_CASEIGNORE1;
		} else
			return true;
	}
#endif

	tdp = dp;
	for(;;){
		c = *sp;
		if (!(cs.fComplete && c)){
			if (!ndp){
				ndp = NextDelim(sp);
			}
#if 1	// new, 日本語対応にはSearchLongestWord()も変更必要有り
			if (sp==ndp)	// 区切り文字 or 文字種が変わった
#else
			if (!IsWordChar(c))
#endif
			{
				ndp = NULL;
				if (c=='!' || c=='?' || c =='.'){
					if ( _flags & SLW_SYMBOLS ){
						// 省略する
						convf |= SLW_SYMBOLS;
						sp++;
						continue;
					} else {
						// 含めて考える
						*dp++ = c;
						sp++;
						continue;
					}
				}
				break;	// loop終了
			}
		}
		if ( c == '-' ){
			cs.FoundHyphen = true;
			if ( _flags & SLW_ELIMHYPHEN1 ){
				// ハイフン除去
				convf |= SLW_ELIMHYPHEN1;
				sp++;
				continue;
			} else
			if ( _flags & SLW_ELIMHYPHEN2 ){
				convf |= SLW_ELIMHYPHEN2;
				sp++;
				break;
			} else
			if ( _flags & SLW_ELIMHYPHEN3 ){
				convf |= SLW_ELIMHYPHEN3;
				dp = tdp;
				sp++;
				continue;
			} else
			if ( _flags & SLW_ELIMHYPHEN4 ){
				convf |= SLW_ELIMHYPHEN4;
				*dp++ = ' ';
				sp++;
				continue;
			} else
			if ( _flags & SLW_ELIMHYPHEN5 ){
				convf |= SLW_ELIMHYPHEN5;
				dp = tdp;
				*dp++ = '-';
				sp++;
				while (*sp=='-') sp++;
				continue;
			}
		}
		if ( _flags & SLW_APOSTROPHE ){
			if ( c == '\'' ){
				convf |= SLW_APOSTROPHE;
				break;
			}
		}
#if 0	//TODO: 未対応
		if ( _flags & SLW_UMLAUT1 ){
			// Umlaut -> ae, ....
			switch ( c ){
				case (byte)'\xC4': *dp++ = 'A'; *dp++ = 'E'; break;
				case (byte)'\xD6': *dp++ = 'O'; *dp++ = 'E'; break;
				case (byte)'\xDC': *dp++ = 'U'; *dp++ = 'E'; break;
				case (byte)'\xDF': *dp++ = 's'; *dp++ = 's'; break;
				case (byte)'\xE4': *dp++ = 'a'; *dp++ = 'e'; break;
				case (byte)'\xF6': *dp++ = 'o'; *dp++ = 'e'; break;
				case (byte)'\xFC': *dp++ = 'u'; *dp++ = 'e'; break;
				default:
					goto j1;
			}
			sp++;
			convf |= SLW_UMLAUT1;
			continue;
	j1:;
		} else
		if ( _flags & SLW_UMLAUT2 ){	// UMLAUT1とUMLAUT2は共存しない
			switch ( *(ushort*)sp ){
				case 0x6561:	// ae
				case 0x4561:	// aE
					*dp++ = '\xE4'; break;
				case 0x6541:	// Ae
				case 0x4541:	// AE
					*dp++ = '\xC4'; break;
				case 0x656F:	// oe
				case 0x456F:	// oE
					*dp++ = '\xF6'; break;
				case 0x654F:	// Oe
				case 0x454F:	// OE
					*dp++ = '\xD6'; break;
				case 0x6575:	// ue
				case 0x4575:	// uE
					*dp++ = '\xFC'; break;
				case 0x6555:	// Ue
				case 0x4555:	// UE
					*dp++ = '\xDC'; break;
				case 0x5353:	// SS
				case 0x5373:	// sS
				case 0x7353:	// Ss
				case 0x7373:	// ss
					*dp++ = '\xDF'; break;
				default:
					goto j2;
			}
			sp+=2;
			convf |= SLW_UMLAUT2;
			continue;
	j2:;
		}
		if ( _flags & SLW_UMLAUT3 ){
			// Umlaut -> a,u,e,o,i
			switch ( c ){
				case (byte)'\xC4': *dp++ = 'A'; break;
				case (byte)'\xD6': *dp++ = 'O'; break;
				case (byte)'\xDC': *dp++ = 'U'; break;
				case (byte)'\xDF': *dp++ = 's'; *dp++ = 's'; break;
				case (byte)'\xE4': *dp++ = 'a'; break;
				//case '\xEB': *dp++ = 'E'; break;
				//case '\xEF': *dp++ = 'I'; break;
				case (byte)'\xF6': *dp++ = 'o'; break;
				case (byte)'\xFC': *dp++ = 'u'; break;
				default:
					goto j3;
			}
			sp++;
			convf |= SLW_UMLAUT3;
			continue;
	j3:;
		} else
		if ( _flags & SLW_UMLAUT4 ){	// UMLAUT3とUMLAUT4は共存しない
			// a,e,o,u,i-->umlaut
			switch ( c ){
				case 'a':
					sp++;
					*dp++ = '\xE4'; break;
				case 'A':
					sp++;
					*dp++ = '\xC4'; break;
				case 'o':
					sp++;
					*dp++ = '\xF6'; break;
				case 'O':
					sp++;
					*dp++ = '\xD6'; break;
				case 'u':	// ue
					sp++;
					*dp++ = '\xFC'; break;
				case 'U':	// Ue
					sp++;
					*dp++ = '\xDC'; break;
				case 0x5353:	// SS
				case 0x7373:	// ss
					sp+=2;
					*dp++ = '\xDF'; break;
				default:
					goto j4;
			}
			convf |= SLW_UMLAUT4;
			continue;
	j4:;
		}
#endif	// 0
#if OLDCASE
		if ( _flags & SLW_CASEIGNORE2 ){
			// 全小文字
			sp++;
			if ( c >= 'A' && c <= 'Z' ){
				*dp++ = (tchar)_totlower(c);
				convf |= SLW_CASEIGNORE2;
			} else {
				*dp++ = c;
			}
		} else
		// 先頭以外全小文字
		if ( (_flags & SLW_CASEIGNORE3) && (dp!=cs.dp) ){
			sp++;
			if ( c >= 'A' && c <= 'Z' ){
				*dp++ = (tchar)_totlower(c);
				convf |= SLW_CASEIGNORE3;
			} else {
				*dp++ = c;
			}
		} else
		// 全大文字
		if ( _flags & SLW_CASEIGNORE4 ){
			sp++;
			if ( c >= 'a' && c <= 'z' ){
				*dp++ = (tchar)_totupper(c);
				convf |= SLW_CASEIGNORE4;
			} else {
				*dp++ = c;
			}
		} else
#endif
		{
			*dp++ = c; sp++;
		}
	}
	if (sp>cs.nextsp)
		cs.nextsp = sp;
	*dp = '\0';
	if ( _flags & SLW_REPLACEIRREG ){
		// 置換処理
		tnstr trsword;
		if ( cs.dic->SearchIrreg( cs.dp, trsword ) ){
			dp = strcpy2( cs.dp, trsword );
			convf |= SLW_REPLACEIRREG;
		}
#ifdef LIGHT
		#define	SLW_REPLACEONES SLW_REPLACEIRREG
		if ( !(convf & SLW_REPLACEIRREG) ){
			// 追加するとき、最後のスペースを忘れないこと！！
			int i = GetStrIndex( cs.dp,
				"am is are was were been "	// 0-5
				"my your our his her their "	// 6-11
				"I me you we us he him she they them "	// 12-21
				"went "			// 22
	//						"took taken "	// 23-24
				);
			if ( i != -1 ){
				if ( i <= 5 ){
					convf |= SLW_REPLACEONES;
					memcpy( cs.dp, _T("be"), (UINT)LENTOBYTE(2) );
					dp = cs.dp + 2;
				} else
				if ( i <= 11 ){
					convf |= SLW_REPLACEONES;
					memcpy( cs.dp, _T("one's"), (UINT)LENTOBYTE(5) );
					dp = cs.dp + 5;
				} else
				if ( i <= 21 )
				{
					convf |= SLW_REPLACEONES;
					memcpy( cs.dp, _T("one"), (UINT)LENTOBYTE(3) );
					dp = cs.dp + 3;
				}
				else if ( i == 22 ){
					convf |= SLW_REPLACEONES;
					memcpy( cs.dp, _T("go"), (UINT)LENTOBYTE(2) );
					dp = cs.dp + 2;
				} else
				if ( i <= 24 ){
					convf |= SLW_REPLACEONES;
					memcpy( cs.dp, _T("take"), (UINT)LENTOBYTE(4) );
					dp = cs.dp + 4;
				}
				*dp = '\0';
			}
		}
#endif
	}
	if ( _flags & SLW_UK ){
		// UK->US
		if ( STR_DIFF(dp,cs.dp) <= 4 )
			return true;
		// 語尾
		switch ( dp[-1] ){
			case 'e':
				switch ( dp[-2] ){
					case 's':
						if ( dp[-3] == 'i' ){
							// ise -> ize
							dp[-2] = 'z';	// ise -> ize
							convf |= SLW_UK;
						} else
							return true;
						break;
					case 'r':
						if ( dp[-3] == 't' ){
							// tre -> ter
							dp[-2] = 'e';
							dp[-1] = 'r';
							convf |= SLW_UK;
						} else
							return true;
						break;
					case 'c':
						// ce -> se
						dp[-2] = 's';
						convf |= SLW_UK;
						break;
					default:
						return true;
				}
				break;
			case 'n':
				if ( dp[-2] == 'o'
					&& dp[-3] == 'i'
					&& dp[-4] == 't'
					&& dp[-5] == 'a'
					&& dp[-6] == 's'
					&& dp[-7] == 'i' ){
					// isation -> ization
					dp[-6] = 'z';
					convf |= SLW_UK;
				} else
					return true;
				break;
			case 'r':
				switch ( dp[-2] ){
					case 'e':
						if ( dp[-3] == 's'
							&& dp[-4] == 'i' ){
							// iser -> izer
							dp[-3] = 'z';
							convf |= SLW_UK;
						} else
							return true;
						break;
					case 'u':
						if ( dp[-3] == 'o' ){
							// our -> or
							dp[-2] = 'r';
							dp--;
							*dp = '\0';
							convf |= SLW_UK;
						} else
							return true;
						break;
					case 'o':
						if ( dp[-3] == 'l'
							&& dp[-4] == 'l' ){
							// llor -> lor
							dp[-3] = 'o';
							dp[-2] = 'r';
							dp--;
							*dp = '\0';
							convf |= SLW_UK;
						} else
							return true;
						break;
					default:
						return true;
				}
				break;
			case 'g':
				if ( dp[-2] == 'n'
					&& dp[-3] == 'i'
					&& dp[-4] == dp[-5] ){
					switch ( dp[-4] ){
						case 'l':
						case 'p':
						case 't':
							// lling -> ling
							// pping -> ping
							// tting -> ting
							dp[-4] = 'i';
							dp[-3] = 'n';
							dp[-2] = 'g';
							dp--;
							*dp = '\0';
							convf |= SLW_UK;
							break;
						default:
							return true;
					}
				} else
					return true;
				break;
			case 'd':
				switch ( dp[-2] ){
					case 'e':
						if ( dp[-3] == dp[-4] ){
							switch ( dp[-3] ){
								// lled -> led
								// pped -> ped
								// tted -> ted
								case 'l':
								case 'p':
								case 't':
									dp[-3] = 'e';
									dp[-2] = 'd';
									dp--;
									*dp = '\0';
									convf |= SLW_UK;
									break;
								default:
									return true;
							}
						} else
							return true;
						break;
					case 'l':
						if ( dp[-3] == 'u'
							&& dp[-4] == 'o' ){
							// ould -> old
							dp[-3] = 'l';
							dp[-2] = 'd';
							dp--;
							*dp = '\0';
							convf |= SLW_UK;
						} else
							return true;
						break;
					default:
						return true;
				}
				break;
			default:
				return true;
		}
	}
	if ( _flags & SLW_DESINENCE1 ){
		if ( STR_DIFF(dp,cs.dp) <= 2 )
			return true;
		// 語尾
		dp1 = dp[-1];
		if ( dp1=='s' ){	// -s, -ies
			if ( (*(dp-2) == 'e') && (*(dp-3) == 'i') ){
				dp -= 2;
				*(dp-1) = 'y';
			} else {
				dp--;
			}
			*dp = '\0';
			convf |= SLW_DESINENCE1;
		} else
		if ( dp1=='g' && *(dp-2) == 'n' && *(dp-3) == 'i' ){	// -ing
			dp -= 3;
			*dp = '\0';
			convf |= SLW_DESINENCE1;
		} else
		if ( dp1=='d' ){	// -d
			dp--;
			*dp = '\0';
			convf |= SLW_DESINENCE1;
		} else
		if ( dp1=='n' ){	// -n
			dp--;
			*dp = '\0';
			convf |= SLW_DESINENCE1;
		}
	}
	if ( _flags & SLW_DESINENCE2 ){
		if ( STR_DIFF(dp,cs.dp) <= 2 )
			return true;
		dp1 = dp[-1];
		if ( (dp1=='s') && (*(dp-2) == 'e') ){	// -es
			dp -= 2;
			*dp = '\0';
			convf |= SLW_DESINENCE2;
		} else
		if ( dp1=='d' ){		// -ied, -ed
			if ( *(dp-2) == 'e' ){
				dp3 = dp[-3];
				if ( dp3 == 'i' ){
					dp -= 2;
					*(dp-1) = 'y';
				} else
				if ( (dp3 == dp[-4]) &&
					( dp3=='b' || dp3=='d' || dp3=='f' || dp3=='g' || dp3=='l' || dp3=='m' || dp3=='n' || dp3=='p' || /*dp3=='s' ||*/ dp3=='r' || dp3=='t' || dp3=='z' ) ){
						// -gged, -pped, -nned, -rred, -tted, -mmed
						// 2012.9.5 -ssedを除外した。discussedなど多くの単語が discus になってしまうため。
					dp-=3;
				} else {
					dp -= 2;
				}
				*dp = '\0';
				convf |= SLW_DESINENCE2;
			}
		} else
		if ( dp1=='g' && *(dp-2) == 'n' && *(dp-3) == 'i' ){	// -ing->-e
			dp4 = dp[-4];
			if ( (dp4 == dp[-5]) &&
				(dp4=='b' || dp4=='d' || dp4=='f' || dp4=='g' || dp4=='k' || dp4=='l' || dp4=='m' || dp4=='n' || dp4=='p' || dp4=='r' || dp4=='s' || dp4=='t' || dp4=='z') ){
					// -gging, -pping, -nning, -rring, -bbing, -dding, -lling, -ffing, -kking, -mming, -ssing, -tting, -zzing
				dp-=4;
			} else {
				// ing --> e置換
				dp -= 2;
				*(dp-1) = 'e';
			}
			*dp = '\0';
			convf |= SLW_DESINENCE2;
		} else
		if ( dp1=='r' && *(dp-2) == 'e' ){	// -er
			dp -= 2;
			*dp = '\0';
			convf |= SLW_DESINENCE2;
		} else
		if ( dp1=='t' && *(dp-2) == 's' && *(dp-3) == 'e' ){
			dp -= 3;
			*dp = '\0';
			convf |= SLW_DESINENCE2;
		}
	}
	if ( _flags & SLW_DESINENCE3 ){
		if ( STR_DIFF(dp,cs.dp) <= 2 )
			return true;
		if ( *(dp-1) == 'd' ){
			if ( *(dp-2) == 'e' ){
				if ( *(dp-3) == 'r' ){	// -red
					dp -= 3;
					*dp = '\0';
					convf |= SLW_DESINENCE3;
				} else
				if ( *(dp-3) == 'l' ){	// -led
					dp -= 3;
					*dp = '\0';
					convf |= SLW_DESINENCE3;
				}
			}
		}

	}
	if ( _flags & SLW_DEUTCH1 ){
		if ( *cs.dp == 'g' && *(cs.dp+1) == 'e' ){	// -eg
			memmove( cs.dp, cs.dp+2, LENTOBYTE(_tcslen(cs.dp+2)+1) );
			convf |= SLW_DEUTCH1;
		}
	}
	if ( _flags & SLW_DEUTCH2 ){
		if ( *(dp-1) == 'e' ){
			if ( *(dp-2) == 's' ){
				dp -= 2;
			} else
			if ( *(dp-2) == 't' ){
				*(dp-2) = 'e';
				*(dp-1) = 'n';
			} else {
				dp--;
			}
			*dp = '\0';
			convf |= SLW_DEUTCH2;
		} else
		if ( (*(dp-1) == 'n' && *(dp-2) == 'e')
			|| (*(dp-1) == 'r' && *(dp-2) == 'e')
			|| (*(dp-1) == 'm' && *(dp-2) == 'e')
			){
			dp -= 2;
			*dp = '\0';
			convf |= SLW_DEUTCH2;
		} else
		if ( *(dp-1) == 't' ){
			*(dp-1) = 'e';
			*dp = 'n';
			dp++;
			*dp = '\0';
			convf |= SLW_DEUTCH2;
		}
	}
	if ( _flags & SLW_DEUTCH3 ){
		if ( (*(dp-1) == 'n')
			|| (*(dp-1) == 'e')
			|| (*(dp-1) == 's') ){
			dp--;    	
			*dp = '\0';
			convf |= SLW_DEUTCH3;
		} else
		if ( (*(dp-1) == 't')	// -t --> -n
			){
			*(dp-1) = 'n';
			*dp = '\0';
			convf |= SLW_DEUTCH3;
		}
	}
	if ( _flags & SLW_REPLACEDEL ){
		// 一般的な単語の削除
		static const tchar *wordlist[] = {
			_T("a"),
			_T("an"),
			_T("the"),
			_T("one"),
			_T("one's"),
			_T("someone"),
			_T("someone's"),
			_T("his"),
			_T("her"),
			_T("him"),
			_T("their"),
			_T("them"),
			_T("very"),
			NULL
		};
		for ( int i=0;wordlist[i];i++ ){
			if (_tcscmp(cs.dp,wordlist[i])==0){
				// 削除対象単語→完全一致と同じ扱い
				convf |= SLW_REPLACEDEL;
				c = cs.dp[-1];	// save
				cs.dp[-1] = '\0';
				cs.dstpart->Add( cs.str, convf|cs.srcflags, cs.numword );
				cs.dp[-1] = c;
				return true;
			}
		}
	}

	if ( _flags != 0 ){
		if ( _flags != convf )
			return true;
	}
jsrch:
	if ( STR_DIFF(dp,cs.str) > LWORD ){
		return true;
	}
	D("str=%ws %04X",cs.str, convf);
	switch ( cs.dic->FindPart( cs.str, dicflag ) )
	{
		case -1:	// error
			return false;
		case 0:	// なし
		case 1:	// 部分一致
			D("Hit-01:%ws",cs.str);
			break;
		case 2:	// 部分一致
			//TODO: SearchPart() for multiple wordに対応必要がある
//			foundflag |= 1;
			if ( cs.fWordDelim ){
				// 2010.10.8 SearchPart()をmulti wordに変更し、
				// dic3.cppにc==BOCU_SHARPを加え、ABC#1対応を追加した
				// もし根本的な不具合がある場合はこのブロックをごっそり削除して構わない
				// 実は今までここのコードは実質意味がなかった
				tnstr_vec fwords;
				if ( cs.dic->SearchPart( cs.str, fwords, dicflag ) ){
					// 完全一致と同じ扱い
					//** これは優先順位を低くしたいなぁ
					//Note: ...,ABC#2, ABC#1と数値逆順にするためにreversed adding.
					for (int i=fwords.size()-1;i>=0;i--){
						cs.dstcomp2->Add( fwords[i], convf|cs.srcflags, cs.numword );
					}
					//** 順位記録
					D("Hit-2-1:%ws",cs.str);
					return true;
				}
			}
			D("Hit-2-2:%ws",cs.str);
			if ( _flags == 0 ){
				cs.notrans_part->Add( cs.str, convf|cs.srcflags, cs.numword );
			}
			cs.dstpart->Add( cs.str, convf|cs.srcflags, convf & SLW_REPLACEANYx ? cs.orgnumword : cs.numword );
			// 2017.2.22 SLW_REPLACEANYxでヒットした場合、numwordは増えないため、orgnumwordを使用する
			break;
		case 3: // 完全一致
			{
				D("Hit-3:%ws %08X",cs.str,convf);
				tnstr_vec words;
				tnstr kword = KWord(cs.str);
				cs.dic->EnumKeyWords(kword, words, dicflag);
				// wordsの中でcwordも含めて完全一致の単語のみ最後へ移動（表示優先順位を上げる）
				int compword_index = -1;
				tnstr cword = join_word(cs.str, kword);
				for (int i=words.get_num()-1;i>=0;i--){
					if (compword_index==-1 && !_tcscmp(cword, words[i])){
						// 完全一致→一番最後にadd
						compword_index = i;
						continue;
					}
					cs.dstcomp->AddCompLast(words[i], convf|cs.srcflags, cs.numword);
				}
				if (compword_index!=-1){
					cs.dstcomp->AddCompLast(words[compword_index], convf|cs.srcflags, cs.numword);
				}
				//** 順位記憶
			}
			return true;
		default:
			break;
	}
	// 完全一致でない場合で、
	// IRREGによる置換処理がある場合は、そこから調べる
	if ( (cs.flags & SLW_REPLACEIRREG) && !dl_is_all(dicflag) ){
		tnstr trsword;
		if ( cs.dic->SearchIrreg( _kwstr(cs.str, cs.dic->GetKCodeTrans()), trsword) ){
			if ( cs.dic->Find( trsword, NULL, dicflag ) == 1 ){
				cs.dstcomp->Add( trsword, convf, cs.numword );
			}
		}
	}
	return true;
}

#define	MAX_COMP	20	// 最大保存ヒット語数
// words : 検索対象語
// str   : 前置単語を含んだ作業用バッファ
int TLangProcStd::SearchStd( COMPARE_STRUCT &cs, const tchar *words, tchar *str, MatchArray *HitWords )
{
	MatchArray *srccomp, *dstcomp, *dstpart;	// 現在の完全一致リスト・スロット

#ifdef __ANDROID__
	// C++BuilderXE7では引数のあるconstructorを直に宣言できない？
	MatchArray c[4];
	MatchArray *comp[2] = { &c[0], &c[1] };	// 完全一致リスト
	MatchArray *part[2] = { &c[2], &c[3] };	// 部分一致リスト
#else
	autoptr<MatchArray> c[4];
	MatchArray *comp[2] = { c[0].get(), c[1].get() };	// 完全一致リスト
	MatchArray *part[2] = { c[2].get(), c[3].get() };	// 部分一致リスト
#endif

	MatchArray comp2( MAX_COMP );
	int compindex = 0;
	srccomp = comp[0];
	comp[0]->Add(_T(""),0,0);

	const tchar *sp = words;
	cs.dstcomp2 = &comp2;

	int skip_retry = -1;	// 単語を飛ばして再検索の連続回数
							// -1は一番先頭の単語という意味
	const int max_skip_retry = 4;	// 単語を飛ばす最大数 // 2009.12.4 3->4に変更

	cs.dic->OpenIrregDic();
	//TLangProc &langproc = *cs.dic->GetLangProc();

	cs.numword = 0;		// 検索語の単語数

	bool preword_exist = str[0]!='\0';
	uint maxlen = 0;
	int r = 0;
	bool inc_pdelim = false;	// include phase delimitor
	bool clicked_passed = false;	// clicked wordを跨いだ時だけtrue
	while ( r != -1 ){
//		if ( STR_DIFF(dp,str) > LWORD ) break;
		// １語コピー //
#if 0	// 意味が無い？ 1998.1.13
		if ( dp != str )
			*dp++ = ' ';
#endif
		if ( !*sp ){
			// 終了
			break;
		}
		if ( !cs.fComplete ){
			// Skip delimitor
			// ここで phase delimitor をcheckし、それらが含まれる単語がreplace any/部分一致の場合はhit対象外にする
			tchar c;
			for (;;){
				c = *sp;
				if (!c || IsWordChar(c))
					break;
				if (IsPhaseDelim(c))
					inc_pdelim = true;
				sp++;
			}
			if ( !c ){
				// 終了
				break;
			}
		}

		int i;
		const tchar *__sp = sp;
		MatchArray notrans_part;	// 変換なしで部分一致の単語リスト

		compindex ^= 1;		// destinationのindex
		dstcomp = comp[compindex];
		dstpart = part[compindex];
		dstcomp->clear();	// 格納先クリア
		dstpart->clear();
		comp2.clear();
		tchar *dp;
		cs.numword++;
		for ( int ci=0;ci<srccomp->get_num();ci++ )
		{
			cs.srcflags = (*srccomp)[ci].flag;

			if (cs.srcflags & SLW_PENALTY2){
				// 2017.2.21 clicked wordが含まれず、かつ５語以上後続単語の追加がない単語は対象外
				if (cs.numword - (*srccomp)[ci].numword > 4){
					continue;
				}
			}

			if (preword_exist && clicked_passed){
				// clicked wordの次の単語以降はpenalty解除
				cs.srcflags &= ~SLW_PENALTY;
			}

			// 前置単語有りで、(*srccomp)[].wordが前置単語であり、
			// cs.spに検索対象単語を含まない場合はpenalty
			// しかし長い単語にはpenaltyが小さすぎて効かない
			if (preword_exist && sp!=words && !clicked_passed){
				cs.srcflags |= SLW_PENALTY;
			}

			if ( (*srccomp)[ci].word[0] ){
				//DBW("srccomp:%d %ws", ci, find_cword_pos((*srccomp)[ci].word.c_str()));
				_tcscpy( str, find_cword_pos((*srccomp)[ci].word) );
				// ヒット語の区切り文字以降を削除 //
				if ( cs.flags & SLW_WORDDELIM ){
					dp = str;
					tchar *last_spc = NULL;
					while ( *dp ){
						tchar c = *dp;
						if (IsWordChar(c)){
							last_spc = NULL;
						} else {
							if (c == ' '){
								last_spc = dp;
							} else {
								if (last_spc)
									*last_spc = '\0';
								else
									*dp = '\0';
								break;
							}
						}
						dp++;
					}
					// 区切り文字移行を削除するとすでにヒットしている文字と同じ場合が有り、無駄な検索となるため対象外とする
					for (int j=0;j<srccomp->get_num();j++){
						if (j==ci) continue;
						if (_tcscmp(str, find_cword_pos((*srccomp)[j].word)) != 0) continue;
						goto jnext;
					}
				}
			}
			if ( str[0] ){
				dp = str + _tcslen(str);
				if (dp[-1]!=' ')
					*dp++ = ' ';
				*dp = '\0';
			} else {
				// 2000.1.7 added
				// ２語目以降なのに、前置単語がない場合は
				if ( cs.numword > 1 ){
					goto jloop;
				}
				dp = str;
			}

			{
				tchar *_dp = dp;

				sp = __sp;

				cs.dp = _dp;
				cs.sp = __sp;
				cs.nextsp = sp;
				cs.notrans_part = &notrans_part;
				cs.dstcomp = dstcomp;
				cs.dstpart = dstpart;
				cs.orgnumword = (*srccomp)[ci].numword;

				//DBW("s:%ws src=%ws", cs.str, cs.sp);
				r = this->FindLoop(cs);
				if (r>0){
					maxlen = STR_DIFF(cs.nextsp,words);
					if ( HitWords ){
						HitWords->add( dstcomp->discard(0) );
					}
					return maxlen;	// found the whole matched word.
				}
			}
		jloop:;
			sp = cs.nextsp;
			if ( cs.fComplete ) break;
jnext:;
		}

		clicked_passed = false;
		if (__sp <= words && sp > words){
			// clicked wordを跨いだ
			clicked_passed = true;

			// 2017.2.21 "on ~"などのように、onが前置単語で、clicked wordが含まれずにhitした場合はpenalty2
			for (i=0;i<dstpart->get_num();i++){
				MATCHINFO &mi = (*dstpart)[i];
				if (mi.flag & (SLW_REPLACEANY|SLW_REPLACEANY2|SLW_REPLACEANY3)){
					mi.flag |= SLW_PENALTY2;
				}
			}
		}

		// 語句区切り文字以降で完全一致がない場合→終了
		if (/*!(cs.flags & SLW_WORDDELIM) && */
			inc_pdelim && dstcomp->get_num()==0
			)
			break;
		
		// 格納先に完全一致はあったか？
		if ( comp2.get_num() ){
			for ( ;comp2.get_num(); ){
				dstcomp->insert( 0, comp2.discard(comp2.get_num()-1) );
			}
			skip_retry = 0;
		}
		if ( dstcomp->get_num() ){
			// 完全一致 or comp2あり
//			if ( maxlen < STR_DIFF(sp, words) )
			{
				maxlen = STR_DIFF(sp,words);
				if ( HitWords ){
					for ( i=0;i<dstcomp->get_num();i++ ){
						MATCHINFO &mi = (*dstcomp)[i];
						HitWords->AddComp( new MATCHINFO( mi ) );
					}
				}
			}
			srccomp = dstcomp;
			if ( notrans_part.get_num() )
				for ( ;notrans_part.get_num(); )
					srccomp->add( notrans_part.discard(0) );
#if 1		// 2000.9.7 追加 単語単位の部分一致であれば、後続もヒットする可能性があるため
			// 例：Academy Awards Presentation
			// 2009.3.13 insert indexを間違っていた
			for(int i=0;dstpart->get_num();i++){
				srccomp->insert( i, dstpart->discard(0) );
			}
#endif
			skip_retry = 0;
		} else {
			//DBW("skip_retry=%d", skip_retry);
			if (skip_retry!=-1 && ++skip_retry > max_skip_retry){
				// スキップが多いものは終了
				break;
			}

			bool to_continue = false;

			// ない場合は前回の完全一致リストで最も変換が少ないものを選ぶ
			// 前回に１つもない場合は、なし
			// ただし、部分一致が２回以下であれば続ける
			// その場合、srccompは、partになる。
			// partにもひとつもなければ終了！！
			if ( dstpart->get_num() ){
				srccomp = dstpart;
				if (skip_retry==-1) skip_retry = 0;
				// 完全一致でなければ返さないので、maxlenの設定は不要！！
			} else {
				// まったく一致が無い場合、最後に追加した単語が悪さを
				// したということで、その単語を飛ばしたもので再度検索を行う
				compindex ^= 1;	// もう一度同じsrccompを使用するために
				if ( skip_retry == -1 /*|| skip_retry > 3 */){
					// スキップが多いものや、
					// 最初の単語の場合は終了
					break;
				}
				to_continue = true;
			}

			// 前置有り検索で、clicked wordを追加したときにヒットがない場合はpenalty pointを追加
			if (preword_exist){
				if (clicked_passed){
					// add penalty point
					for (int i=0;i<srccomp->get_num();i++){
						(*srccomp)[i].flag |= SLW_PENALTY;
					}
				}
			}
			if (to_continue)
				continue;
		}
		// 重複しているものは削除
		// bitcountが少ないほうを優先
		for(i=0;i<srccomp->get_num()-1;i++ ){
			if ( (*srccomp)[i].word == (*srccomp)[i+1].word ){
				if (BitCount((*srccomp)[i].flag) > BitCount((*srccomp)[i+1].flag) ){
					// (i) > (i+1)
					srccomp->del(i+1);
				} else {
					srccomp->del(i);
				}
				i--;
				continue;
			}
		}
	}
//jend:;
	return r == -1 ? -1 : maxlen;
}

// return value:
// -1 : break the loop (error, stop the search)
//	0 : continue
//	1 : found the complete matched word.
int TLangProcStd::FindLoop(COMPARE_STRUCT &cs)
{
	if ( cs.flags & SLW_REPLACEANY ){
		if ( !CompareStd( cs, SLW_REPLACEANY ) ){
			return -1;
		}
	}
	if ( cs.flags & SLW_REPLACEANY3 ){
		if ( !CompareStd( cs, SLW_REPLACEANY3 ) ){
			return -1;
		}
	}

	// SLW_DEUTCH loop //
	static int tbl_deutch[4] = { SLW_DEUTCH3, SLW_DEUTCH2, SLW_DEUTCH1, 0 };
	for ( int fm=0;fm<4;fm++ ){
		if ( (cs.flags & tbl_deutch[fm]) != tbl_deutch[fm] )
			continue;
		// SLW_UMLAUT loop //
		static int tbl2[] = {
#if 0	//TODO: 未対応
			SLW_UMLAUT4|SLW_UMLAUT2,
			SLW_UMLAUT3|SLW_UMLAUT1,
			SLW_UMLAUT4,
			SLW_UMLAUT3,
			SLW_UMLAUT2,
			SLW_UMLAUT1,
#endif
			0 };
		for ( int fl=0;fl<sizeof(tbl2)/sizeof(int);fl++ ){
			if ( (cs.flags & tbl2[fl]) != tbl2[fl] )
				continue;
			// SLW_DESINENCE loop //
			// Desinenceはすべて同時に両立しない
			static int tbl_desinence[5]
				= { SLW_DESINENCE3, SLW_DESINENCE2, SLW_DESINENCE1, SLW_APOSTROPHE, 0 };
			for ( int fk=0;fk<sizeof(tbl_desinence)/sizeof(int);fk++ ){
				if ( (cs.flags & tbl_desinence[fk]) != tbl_desinence[fk] )
					continue;
				// SLW_UK loop //
				static int tbl_uk[2] = { SLW_UK, 0 };
				for ( int ukl=0;ukl<2;ukl++ ){
					if ( (cs.flags & tbl_uk[ukl]) != tbl_uk[ukl] )
						continue;
					// SLW_CASEIGNORE loop //
					// CaseIgnoreはすべて同時に両立しない
					static int tbl_caseignore[] = {
#if OLDCASE
						SLW_CASEIGNORE4, SLW_CASEIGNORE3, SLW_CASEIGNORE2, SLW_CASEIGNORE1,
#endif
						0 };
					for ( int fj=0;fj<sizeof(tbl_caseignore)/sizeof(int);fj++ ){
						if ( (cs.flags & tbl_caseignore[fj]) != tbl_caseignore[fj])
							continue;
						// SLW_REPLACE loop //
						static int tbl1[6]
							= {
								SLW_REPLACEIRREG|SLW_REPLACEDEL,
								SLW_REPLACEIRREG|SLW_REPLACEANY2,
								SLW_REPLACEIRREG,
								SLW_REPLACEANY2,
								SLW_REPLACEDEL,
								0};
						for ( int fi=0;fi<6;fi++ ){
							if ( (cs.flags & tbl1[fi]) != tbl1[fi] )
								continue;
							// SLW_SYMBOLS loop //
							static int tbl_sym[] = { SLW_SYMBOLS, 0 };
							for ( int fsym=0;fsym<2;fsym++ ){
								if ( (cs.flags & tbl_sym[fsym]) != tbl_sym[fsym] )
									continue;
								int _flags = tbl_deutch[fm] | tbl2[fl] | tbl_desinence[fk] | tbl_caseignore[fj] | tbl1[fi] | tbl_uk[ukl] | tbl_sym[fsym];
								if ((_flags & SLW_REPLACEIRREG)
									&& (_flags & (SLW_ENGLISH|SLW_UMLAUT|SLW_DEUTCH))
									){
									// 不規則変化とその他の変化形は同時使用しない
									continue;
								}
								// hyphen //
								if ( cs.flags & SLW_ELIMHYPHEN ){
									cs.FoundHyphen = false;
									if ( !Compare( cs, _flags | SLW_ELIMHYPHEN3 ) ){
										return -1;
									}
									if ( cs.FoundHyphen ){
										if ( !Compare( cs, _flags | SLW_ELIMHYPHEN2 ) ){
											return -1;
										}
										if ( !Compare( cs, _flags | SLW_ELIMHYPHEN4 ) ){
											return -1;
										}
										if ( !Compare( cs, _flags | SLW_ELIMHYPHEN1 ) ){
											return -1;
										}
										if ( !Compare( cs, _flags | SLW_ELIMHYPHEN5 ) ){
											return -1;
										}
									}
								}
								// normal //
								if ( !Compare( cs, _flags ) ){
									return -1;
								}
							}
							if ( cs.flags & SLW_SPELLCHECK ){
								if ( cs.dstcomp->get_num() ){
									// 完全一致あり！
									return 1;
								}
							}
						}
					}
				}
			}
		}
	}
	return 0;	// not found the completed word.
}

int TLangProcStd::SearchLongestWord( class MultiPdic *dic, const tchar *words, const tchar *prevwords, int curpos, int flags, class MatchArray *HitWords )
{
#if 0	// new, 弊害：現状日本語の検索が遅くなる
	return SearchLongestWordOptional( *dic, words, prevwords, flags, HitWords );
#else
	if ( !_tiskanji(words[0]) && (curpos==0 || !_tiskanji(words[curpos]))){
		// 英語
		return SearchLongestWordOptional( *dic, words, prevwords, flags, HitWords );
	} else {
		// 日本語
		return mbSearchLongestWordOptional( *dic, words, curpos, flags, HitWords );
	}
#endif
}

int TLangProcStd::SearchLongestWordExt(class MultiPdic *dic, const tchar *words, const tchar *prevwords, class MatchArray *hitwords, int &thread_key, FNLPSLWExtCallback callback, int user)
{
#if USE_WEBSRCH
	return searchLongestWordExt.SearchLongestWordExt(dic, words, prevwords, hitwords, thread_key, callback, user);
#else
	return 0;
#endif
}

int TLangProcStd::SearchLongestWordExtCmd(int cmd, int thread_key)
{
#if USE_WEBSRCH
	return searchLongestWordExt.SearchLongestWordExtCmd(cmd, thread_key);
#else
	return 0;
#endif
}

tnstr TLangProcStd::MakeMorphKeyword(const tchar *word)
{
	return tnstr(word);
}
bool TLangProcStd::BuildMorphWords(const tchar *word, tnstr_vec &items, tnstr_vec *features)
{
	// N-gram
	if (!word[0])
		return false;

	if (word[1]){
		do {
			items.push(new tnstr(word, NGRAM));
			word++;
		} while (word[0] && word[1]);
	} else {
		items.push(new tnstr(word));
	}
	return true;
}

//
// SLW specific search
//
// 今までとはちょっと異なる手法
// 高速化のために１語ずつ増やしていく
// 単語の区切り：[^A-Za-z0-9-']
// HitWords(O) : ヒットした単語のリスト
// Return value:
// -1 : エラー発生
// 0  : 一致しない
// 1以上 : 先頭からの一致文字数(words上で)
//
// さらに高速化のために、flags!=0のときにflagsに該当する処理がなされなかった場合は無視
// さらにさらに高速化のために、該当後が見つからないオプションについてはそれ
// 以降の検索は行わない(はまだ実現していない)
// wordsの前に変換を加えるような場合には対応していない。(foundflag参照)
// １種類も見つからない場合は検索を中止する(foundflag参照)
//
int TLangProcStd::SearchLongestWord( MultiPdic &dic, const tchar *words, const tchar *prevwords, int flags, MatchArray *HitWords )
{
#if 0
	if ( dic.find2( words ) == 3 ){
		if ( HitWords ) HitWords->add( words );
		return _tcslen(words);
	}
#endif

//	byte HitLevel[2][MAX_COMP];

	tchar *_str = new tchar[ _tcslen( words ) + LWORD ];
	_str[0] = '\0';	// 不正アクセス防止
	_str[1] = '\0';
	_str[2] = '\0';
	_str[3] = '\0';
	tchar *str = _str + 4;

	uint maxlen = 0;

	COMPARE_STRUCT cs;
	cs.flags = flags;
	cs.str = str;
	cs.fComplete = flags & SLW_COMPLETE;
	cs.fWordDelim = flags & SLW_WORDDELIM;
	cs.dic = &dic;

	// 前置単語なしの場合
	*str = '\0';
	int r = Search( cs, words, str, HitWords );
	if ( r == -1 ){
		delete _str;
		return -1;
	}
	if ( maxlen < r )
		maxlen = r;
	if (!(cs.flags & SLW_COMPLETE)){
		// 完全一致の単語のみを調べる
		// すでにHitWordsにあるものは検索しない
		//Note: wordsの最後に空白があるとFindWord()が誤動作するため、trimrightが必要
		// ex. "control " で検索すると "control-"がヒットしてしまう
		tnstr _words(words);
		trimright(_words);
		tnstr cword = dic.create_composit_word(_words);
		if (HitWords->FindWord(cword)==-1){
			cs.fComplete = true;
			int cs_flags = cs.flags;
			cs.flags = SLW_COMPLETE;
			str[0] = '\0';
			r = Search(cs, _words, str, HitWords);
			if ( maxlen < r )
				maxlen = r;
			cs.fComplete = false;
			cs.flags = cs_flags;
		}
	}

	if ( prevwords ){
		for (;;){
			if ( prevwords == words )
				break;
			// 前置単語処理(複数語に対応) //
			tchar *dp = str;
			const tchar *nextword = NULL;
			while ( prevwords < words ){
				if ( !IsWordChar( *prevwords ) ){
					while ( !IsWordChar( *prevwords ) && (prevwords < words) ){
						prevwords++;
					}
					if (!nextword)
						nextword = prevwords;
					if (prevwords == words)
						break;
					*dp++ = ' ';
					continue;
				}
				*dp++ = *prevwords++;
			}
			*dp = '\0';

			MatchArray hits;
			r = Search( cs, words, str, &hits );
			if ( r == -1 )
				break;
			if ( maxlen < r )
				maxlen = r;
			// with abandon でabandonをclick
			InsertHitWords2( *HitWords, hits );
			if (nextword)
				prevwords = nextword;
		}
	}
	SortHitWords( *HitWords );

	delete[] _str;

#if 0
	DBW("----------");
	for ( int i=0;i<HitWords->get_num();i++ ){
		MATCHINFO &m = (*HitWords)[i];
		DBW("%d %08X %s",m.numword,m.flag,m.word.c_str());
	}
#endif
	
	return r == -1 ? -1 : maxlen;
}

// 日本語専用検索(optionなし)
// 高速化のために１語ずつ増やしていく
// 単語の区切り：isdelim
// Return value:
// -1 : エラー発生
// 0  : 一致しない
// 1以上 : 先頭からの一致文字数(words上で)
//
// 日本語は、区切り文字までの文字列を検索対象とする
// それ以降は無視される
//
// ヒットした単語の最後がすべてcurposより前であった場合は、
// wordsの１つ右へずらして再度検索。以降curposを含む or curposが先頭になるまで繰り返す。
// curposを含む語がヒットした場合はそこで終了。
int TLangProcStd::mbSearchLongestWord( MultiPdic &dic, const tchar *words, int curpos, int flags, MatchArray *found )
{
	const tchar *sp = words;
	const tchar *sp_top = sp;
	tchar *str = new tchar[ _tcslen( words ) + 60 ];
	tchar *dp = str;
	int offset = 0;
	int r = 0;
//	if ( found ) found->clear();
	uint maxlen = 0;
	dl_def(dicflag);

	int numword = 0;

	for (;;){
	
	while ( *sp ){
		if (
#ifndef _UNICODE
			!IsDBCSLeadByte( *sp ) &&
#endif
			!IsWordChar( *sp ) ) break;
		// １語コピー //
		int foundflag = 0;
		numword++;

		// すべてのオプションについて試す
//		for ( int i=0;i<=SLW_MASK1;i++ )
		{
#if 0
			if ( i != 0 && (~flags & SLW_MASK1 & i) ){
				continue;
			}
#endif
#if 0
			dp = __dp;
			sp = __sp;
#endif

//			int _flags = i;

//			int convf = 0;

#if 0	//未対応
			// 先頭case反転
			if ( _flags & SLW_CASEIGNORE1 ){
				if ( (tuchar)*sp >= 'A' && (tuchar)*sp <= 'Z' ){
					*dp++ = *sp++;
					convf |= SLW_CASEIGNORE1;
				}
			}
#endif
//			tuchar c;
//			while ( ((flags & SLW_COMPLETE) && *sp) || IsWordChar( *sp ) || IsDBCSLeadByte(*sp) )
			{
				{
#ifndef _UNICODE
					if ( IsDBCSLeadByte(*sp) ){
						*dp++ = *sp++;
					}
#endif
					*dp++ = *sp++;
				}
			}
			*dp = '\0';
#if 0
			if ( i != 0 ){
				if ( i != convf ) continue;
			}
#endif
			if ( STR_DIFF(dp,str) > LWORD ){
				continue;
			}
			switch ( dic.FindPart( str ) ){
				case -1:	// error
					r = -1;
					break;
				case 1:	// 部分一致
				case 2:
#if 1
					if ( flags & SLW_WORDDELIM ){
						tnstr_vec fwords;
						if ( dic.SearchPart( str, fwords, dicflag ) ){
							foundflag = 1;
							// 完全一致と同じ扱い
							//** これは優先順位を低くしたいなぁ
							//Note: ...,ABC#2, ABC#1と数値逆順にするためにreversed adding.
							int len = STR_DIFF(sp, sp_top);
							if (len>curpos-offset){
								if ( maxlen < len ){
									maxlen = len;
									if (found){
										for (int i=fwords.size()-1;i>=0;i--){
											MATCHINFO *mi = new MATCHINFO( fwords[i], 0 /*convf*/, numword, NULL, offset );
											found->AddComp(mi);
										}
									}
								}
							}
							//** 順位記録
							//D("Hit-2-1:%ws",cs.str);
							//return true;
						}
					}
#if 0	//TODO:
					D("Hit-2-2:%ws",cs.str);
					if ( _flags == 0 ){
						cs.notrans_part->Add( cs.str, convf|cs.srcflags, cs.numword );
					}
					cs.dstpart->Add( cs.str, convf|cs.srcflags, cs.numword );
#endif
#else
#if 0	//未対応
					foundflag = 1;
					if ( flags & SLW_WORDDELIM ){
						tnstr fword;
						dic.Search( str, fword, NULL );
						tchar c = fword[_tcslen(str)];
						if ( !IsWordChar(c) && c != ' ' ){
							// 完全一致と同じ扱い
							goto j10;
						}
					}
					LastFound = str;
#endif
#endif
					continue;
				case 3: // 完全一致
//				j10:
					{
						// ヒット候補から完全一致を探す
						tnstr_vec fwords;
						tnstr kword = KWord(str);
						dic.EnumKeyWords(kword, fwords);
						foreach_tnstr_vec(fwords, fw){
							if (strequ(find_cword_pos(fw->c_str()),str)){
								foundflag = 2;
								int len = STR_DIFF(sp, sp_top);
								if (len>curpos-offset){
									if ( maxlen < len ){
										maxlen = len;
										if (found){
											MATCHINFO *mi = new MATCHINFO( str, 0 /* convf */, numword, NULL, offset );
											found->AddComp(mi);
										}
									}
								}
								break;
							}
						}
						if (foundflag==0){
							// 部分一致と同じ扱い
							continue;
						}
					}
					break;
				default:
					continue;
			}
		}
		if ( !foundflag ) break;	// １種類も見つからない
#if 0
		if ( flags & SLW_COMPLETE ) break;
		if ( foundflag == 1 ){
			_tcscpy( __dp, LastFound );
		}
#endif
	}	// while(*sp)

	if (maxlen>curpos-offset)
		break;	// 現在位置を含む語が見つかった

	// 隣の文字から検索やり直し
	offset++;
	if (offset>curpos)
		break;
	sp = words + offset;
	sp_top = sp;
	dp = str;
	maxlen = 0;
	numword = 0;
	
	} // for(;;)

	delete[] str;

	return r == -1 ? -1 : maxlen;
}


#if 0
int TLangProcStd::_SearchLongestWordOptional( MultiPdic &dic, const tchar *words, int flags, tnstr *found )
{
	int r;
	int i;
	int maxlen = 0;
	tnstr *fstr;
	if ( found ){
		fstr = new tnstr;
	} else {
		fstr = NULL;
	}

	// オリジナルで検索
	r = SearchLongestWord( dic, words, 0, fstr );
	if ( r == -1 ){
		goto exit;
	}

	maxlen = r;
	if ( found ) found->set( *fstr );

	// すべてのフラグについて試す
	for ( i=0;i<=(SLW_REPLACEONES|SLW_ELIMHYPHEN|SLW_DESINENCE|SLW_DESINENCE2);i++ ){
		if ( !(flags & (SLW_REPLACEONES|SLW_ELIMHYPHEN|SLW_DESINENCE|SLW_DESINENCE2) & i) ){
			continue;
		}
		r = SearchLongestWord( dic, words, (flags & ~(SLW_REPLACEONES|SLW_ELIMHYPHEN|SLW_DESINENCE|SLW_DESINENCE2)) | i , fstr );
		if ( r == -1 ) break;
		if ( r > maxlen ){
			maxlen = r;
			if ( found ) found->set( *fstr );
		}
	}

exit:
	if ( fstr ) delete fstr;

	return maxlen;
}
#endif	// 0

// prevwords : 前置検索単語 prevwords <= words or prevwords = NULL
int TLangProcStd::SearchLongestWordOptional( MultiPdic &dic, const tchar *words, const tchar *prevwords, int flags, MatchArray *HitWords )
{
	int r;
	int maxlen = 0;
#if 0
	tnstr *fstr;
	if ( HitWords ){
		fstr = new tnstr;
	} else {
		fstr = NULL;
	}
#endif
	tchar *temp = NULL;

	r = SearchLongestWord( dic, words, prevwords, flags, HitWords );
	if ( r == -1 ) goto exit;

	maxlen = r;
//	if ( found ) found->set( *fstr );

#if 0
	if ( flags & SLW_CASEIGNORE ){
		temp = new tchar[ _tcslen(words) + 20 ];
		_tcscpy( temp, words );
		if ( temp[0] >= 'A' && temp[0] <= 'Z' ){
			temp[0] += (tchar)0x20;
		} else
		if ( temp[0] >= 'a' && temp[0] <= 'z' ){
			temp[0] -= (tchar)0x20;
		} else {
			goto next1;
		}
		r = SearchLongestWordOptional( dic, temp, flags, fstr );
		if ( r == -1 ){
			maxlen = r;
			if ( found ) found->set( *fstr );
		}
	}
#endif

//next1:

	if ( flags & SLW_CONJUGATE ){
		if ( !temp )
			temp = new tchar[ _tcslen(words) + 20 ];
		const tchar *sp = dic.GetLangProc()->GetConjugateWords();
		if (sp && sp[0]){
			const tchar *ssp = sp;
			while ( 1 ){
				if ( *sp == ';' || *sp == '\0' ){
					if ( STR_DIFF( sp, ssp ) ){
						_tcsncpy( temp, ssp, STR_DIFF(sp,ssp) );
						_tcscpy( temp + STR_DIFF(sp,ssp), _T(" ") );
						_tcscat( temp, words );
						r = SearchLongestWord( dic, temp, NULL, flags, HitWords );
						if ( r == -1 ){
							maxlen = r;
	//						if ( HitWords ) found->set( *fstr );
						}
					}
					if ( !*sp )
						break;
					sp++;
					ssp = sp;
					continue;
				}
				sp = CharNext( sp );
			}
		}
	}

exit:
//	if ( fstr ) delete fstr;
	if ( temp ) delete temp;

	return maxlen;
}
// 日本語対応版
// curpos : 現在のcursor位置(wordsからのoffset, If not provided, must be 0.)
int TLangProcStd::mbSearchLongestWordOptional( MultiPdic &dic, const tchar *words, int curpos, int flags, MatchArray *found )
{
	int r;
	int maxlen = 0;
#if 0
	tnstr *fstr;
	if ( found ){
		fstr = new tnstr;
	} else {
		fstr = NULL;
	}
#endif
	tchar *temp = NULL;

	r = mbSearchLongestWord( dic, words, curpos, flags, found );
	if ( r == -1 ) goto exit;

	maxlen = r;
//	if ( found ) found->set( *fstr );

exit:
//	if ( fstr ) delete fstr;
	if ( temp ) delete temp;

	return maxlen;
}

//---------------------------------------------------------------------------
//	TLangProcStd0
//---------------------------------------------------------------------------
TLangProcStd0::TLangProcStd0()
{
	KCodeTranslate.encodeKT = ::encodeKT;
	KCodeTranslate.decodeKT = ::decodeKT;
}

