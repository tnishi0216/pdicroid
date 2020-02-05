#include "tnlib.h"
#pragma	hdrstop
#include "japa.h"
#include "dictext.h"
#include "dic.h"

const tchar *EXTCRCHAR =	_T(" \\ ");
tnstr StrOneLineDelim( _T(" /// ") );
tnstr StrDelimiter( _T(",") );

const tchar *StrCR = _T("\r");
const tchar *StrLF = _T("\n");
const tchar *StrCRLF = _T("\r\n");
//const tchar *StrExpSepa = _T(" / ");	moved to japa1.cpp
#define	L_StrExpSepa	3

#define	WITHPRON	0

// 拡張PDICテキスト形式(改行を許す）
// 最後の改行は付加されないので呼出側で出力すること！！
int PutMultiLine( TOFile &tof, const tchar *p )
{
	while ( *p ){
		if ( *p == CharCR ){
			p++;
			if ( *p == CharLF ){
				p++;
			}
	jmp1:
			tof.put( EXTCRCHAR );
			continue;
		}
		if ( *p == CharLF ){
			p++;
			goto jmp1;
		}
		if ( tof.put( *p ) == -1 ){
			return -1;
		}
		p++;
	}
	return 0;
}

// bufへ複数行対応フォーマットで書き込む
// 最後の改行は付加されないので呼出側で出力すること！！
void PutMultiLine( tchar *buf, const tchar *p )
{
	while ( *p ){
		if ( *p == CharCR ){
			p++;
			if ( *p == CharLF ){
				p++;
			}
	jmp1:
			_tcscpy( buf, EXTCRCHAR );
			buf += sizeof(EXTCRCHAR);
			continue;
		}
		if ( *p == CharLF ){
			p++;
			goto jmp1;
		}
		*buf++ = *p++;
	}
//	*buf++ = CharLF;
	*buf = '\0';
}

int PutText( TOFile*file, Japa &japa, BOOL fExtPdicText )
{
	int r = 0;
#if WITHPRON
	if ( japa.pron[0] ){
		r |= file->put( "[" );
		r |= file->put( japa.pron );
		r |= file->put( "]" );
	}
#endif
	if ( fExtPdicText ){
		// 拡張PDICテキスト形式(改行を許す）
		r |= PutMultiLine( *file, japa.japa );
	} else {
		r |= file->put( japa );
	}
	if ( japa.exp[0] ){
		r |= file->put( StrExpSepa );
		if ( fExtPdicText ){
			// 拡張PDICテキスト形式(改行を許す）
			r |= PutMultiLine( *file, japa.exp );
		} else {
			r |= file->put( japa.exp );
		}
	}
	r |= file->put( CharLF );
	if ( r ){
		return -1;
	}
	return 0;
}

void SetTextCRLF( _jMixChar &text, const tchar *str )
{
	for (;;){
		const tchar *crptr = ::_tcsstr( str, EXTCRCHAR );
		if ( !crptr ){
			if ( *str )
				text.cat( str );
			break;
		}
		text.cat( str, STR_DIFF(crptr,str) );
		text.cat( StrCRLF );
		str = crptr + L_EXTCRCHAR;
	}
}

#ifdef CMD32
#include "dic3.h"
void GetParams( const tchar *text, FlexCharArray &params );		// defined in pdout.cpp
extern MultiPdic dic;
#endif

// fSeparate : 訳/用例の区別
void GetMultiPdicText( tchar *line, Japa &japa, BOOL fSeparate, BOOL fReplaceCRLF )
{
	tchar *expptr = NULL;
	if ( fSeparate ){
		expptr = _tcsstr( line, StrExpSepa );
		if ( expptr ){
			*(tchar*)expptr = '\0';
			expptr += L_StrExpSepa;
		}
	}
#if defined(CMD32)
#ifdef USE_JLINK
	tchar *objtext = _tcsstr( expptr ? expptr : line, _T(" * ") );
	while ( objtext ){
		objtext += 3;
		if ( objtext[1] == '=' ){
			objtext[-3] = '\0';
			tchar *nextobjtext = _tcsstr( objtext, _T(" * ") );
			if ( nextobjtext ){
				if ( nextobjtext[3+1] == '=' ){
					nextobjtext[0] = '\0';
				} else nextobjtext = NULL;
			}
			FlexCharArray params;
			switch ( objtext[0] ){
#ifdef USE_ICONFILE
				case 'C':	// IconFileObject
					// C=title,filename,iconfilename
					GetParams( objtext+2, params );
					JLIconFile *obj = new JLIconFile( &dic, params[1], dic.GetObjectNumber() );
					obj->SetTitle( params[0] );
					japa.jlink.add( obj );
					break;
#endif
				case 'F':	// FileLinkObject
					GetParams( objtext+2, params );
					if ( params.get_num() >= 2 ){
#if UNIVDIC_JLINK
						JLFile *obj = new JLFile( &dic[0], params.get_num() >= 2 ? (const tchar*)params[1] : _T(""), dic[0].GetObjectNumber() );
#else
						JLFile *obj = new JLFile( (Pdic*)&dic[0], params.get_num() >= 2 ? (const tchar*)params[1] : _T(""), ((Pdic*)&dic[0])->GetObjectNumber() );
#endif
						if ( params.get_num() >= 1 )
							obj->SetTitle( params[0] );
						japa.jlinks.add( obj );
					}
					break;
				case 'P':	// Pronuciation
					japa.pron.set( objtext+2 );
					break;
				default:
					// unknwon -> skip
					break;
			}
			objtext = nextobjtext;
		} else {
			objtext = _tcsstr( objtext, _T(" * ") );
		}
	}
#endif
#endif
	if ( fReplaceCRLF ){
		SetTextCRLF( japa.japa, line );
	} else {
		japa.japa.set( line );
	}
	if ( expptr ){
		if ( fReplaceCRLF ){
			SetTextCRLF( japa.exp, expptr );
		} else {
			japa.exp.set( expptr );
		}
	}
#if 0
	const tchar *startp = line;
	bool fExp = false;
	while ( 1 ){
		if ( ( fReplaceCRLF && !strncmp( line, EXTCRCHAR, L_EXTCRCHAR ) ) || !*line ){
			if ( fExp ){
				if ( STR_DIFF( line, startp ) ){
					japa.exp.cat( startp, STR_DIFF( line, startp ) );
				}
				if ( !*line )
					break;
				japa.exp.cat( StrCRLF );
				line += L_EXTCRCHAR;
			} else {
				if ( STR_DIFF( line, startp ) ){
					japa.japa.cat( startp, STR_DIFF( line, startp ) );
				}
				if ( !*line )
					break;
				japa.japa.cat( StrCRLF );
				line += L_EXTCRCHAR;
			}
			startp = line;
		} else if ( fSeparate && !fExp && !strncmp( line, StrExpSepa, L_StrExpSepa ) ){
			if ( STR_DIFF( line, startp ) ){
				japa.japa.cat( startp, STR_DIFF( line, startp ) );
			}
			line += L_StrExpSepa;
			startp = line;
			fExp = true;
		} else {
			line = GetNextPtr( line );
		}
	}
#endif
#if 0
	const tchar *startp = line;
//	bool fExp = false;
	while ( 1 ){
		if ( ( fReplaceCRLF && !strncmp( line, EXTCRCHAR, L_EXTCRCHAR ) ) || !*line ){
			if ( STR_DIFF( line, startp ) ){
				japa.japa.cat( startp, STR_DIFF( line, startp ) );
			}
			if ( !*line )
				break;
			japa.japa.cat( StrCRLF );
			line += L_EXTCRCHAR;
			startp = line;
		} else if ( fSeparate && !strncmp( line, StrExpSepa, L_StrExpSepa ) ){
			if ( STR_DIFF( line, startp ) ){
				japa.japa.cat( startp, STR_DIFF( line, startp ) );
			}
			line += L_StrExpSepa;
			startp = line;
			break;
		} else {
			line = GetNextPtr( line );
		}
	}
	if ( *line ){
		while ( 1 ){
			if ( ( fReplaceCRLF && !strncmp( line, EXTCRCHAR, L_EXTCRCHAR ) ) || !*line ){
				if ( STR_DIFF( line, startp ) ){
					japa.exp.cat( startp, STR_DIFF( line, startp ) );
				}
				if ( !*line )
					break;
				japa.exp.cat( StrCRLF );
				line += L_EXTCRCHAR;
				startp = line;
			} else {
				line = GetNextPtr( line );
			}
		}
	}
#endif
}

