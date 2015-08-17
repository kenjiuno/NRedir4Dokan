
#ifndef _NREDNP_H_
#define _NREDNP_H_

#include <string>
#include <vector>
#include <set>

//! 共有
typedef struct Share {
	std::wstring server; //! サーバー名
	std::wstring share; //! 共有名
	std::wstring ntPath; //! \Device\DokanRedirector{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	
	//! 解析
	bool parse(LPCWSTR pcw) {
		if (pcw != NULL && *pcw == L'\\') {
			LPCWSTR pcw1 = StrChrW(pcw +1, L'\\');
			if (pcw1 != NULL) {
				LPCWSTR pcw2 = StrChrW(pcw1 +1, L'\\');
				if (pcw2 == NULL) {
					server.assign(pcw +1, pcw1 -pcw -1);
					share.assign(pcw1 +1);
					return true;
				}
			}
		}
		return false;
	}

	//! サーバー名が等価かどうかを検査
	bool test(LPCWSTR pcw) {
		if (pcw != NULL && pcw[0] == L'\\' && pcw[1] == L'\\') {
			if (StrCmpI(server.c_str(), pcw +2) == 0) {
				return true;
			}
		}
		return false;
	}
}	Share;

//! Share の vector
typedef std::vector<Share> Shares;

//! UNC パスの解析サポート
typedef struct UPath {
	std::wstring server; //! サーバー名
	std::wstring share; //! 共有名
	std::wstring local; //! ローカルパス
	
	bool parse(LPCWSTR pcw) {
		if (pcw != NULL && pcw[0] == L'\\' && pcw[1] == L'\\') {
			LPCWSTR pcw0 = pcw +2;
			LPCWSTR pcw1 = StrChrW(pcw0, L'\\');
			if (pcw1 != NULL) {
				server.assign(pcw0, pcw1 -pcw0);
				
				LPCWSTR pcw2 = StrChrW(pcw1 +1, L'\\');
				if (pcw2 == NULL) {
					share.assign(pcw1 +1);
					local.clear();
				}
				else {
					share.assign(pcw1 +1, pcw2 -pcw1 -1);
					local.assign(pcw2 +1);
				}
				return true;
			}
		}
		server.clear();
		share.clear();
		local.clear();
		return false;
	}
}	UPath;

//! 大文字小文字を区別しない比較演算子
typedef struct {
	bool operator()(
		const std::wstring &lv, 
		const std::wstring &rv
	) const {
		return StrCmpIW(lv.c_str(), rv.c_str()) < 0;
	}
}	CompareNoCase;

//! サーバー名のセット
typedef std::set<std::wstring, CompareNoCase> ServerSet;

//! NETRESOURCE ラッパー
typedef struct NetRes {
	DWORD Scope; //! RESOURCE_ 定数
	DWORD Type; //! RESOURCETYPE_ 定数
	DWORD DisplayType; //! RESOURCEDISPLAYTYPE_ 定数
	DWORD Usage; //! RESOURCEUSAGE_ 定数
	std::wstring LocalName, RemoteName, Comment, Provider;
	
	//! 必要なバイト数
	DWORD Bytes() const {
		return sizeof(NETRESOURCE)
			+(LocalName.empty() ? 0 : 2 * (LocalName.size() +1))
			+(RemoteName.empty() ? 0 : 2 * (RemoteName.size() +1))
			+(Comment.empty() ? 0 : 2 * (Comment.size() +1))
			+(Provider.empty() ? 0 : 2 * (Provider.size() +1))
			;
	}
	
	//! バッファに書き出す
	bool WriteTo(PBYTE &pLo, PBYTE &pHi) const {
		if (pLo +sizeof(NETRESOURCE) > pHi)
			return false;
		NETRESOURCE *pNR = reinterpret_cast<NETRESOURCE *>(pLo);
		pNR->dwScope = Scope;
		pNR->dwType = Type;
		pNR->dwDisplayType = DisplayType;
		pNR->dwUsage = Usage;
		pLo += sizeof(NETRESOURCE);
		if (!Ut::Wr(pLo, pHi, LocalName, pNR->lpLocalName))
			return false;
		if (!Ut::Wr(pLo, pHi, RemoteName, pNR->lpRemoteName))
			return false;
		if (!Ut::Wr(pLo, pHi, Comment, pNR->lpComment))
			return false;
		if (!Ut::Wr(pLo, pHi, Provider, pNR->lpProvider))
			return false;

		return true;
	}
	
	//! 補助
	struct Ut {
		//! 書き出す
		static bool Wr(PBYTE &pLo, PBYTE &pHi, const std::wstring &ws, LPWSTR &ppcw) {
			if (ws.empty()) {
				ppcw = NULL;
				return true;
			}
			int cb = 2 * (ws.size() +1);
			if (pHi -pLo < cb)
				return false;
			pHi -= cb;
			ppcw = reinterpret_cast<LPWSTR>(pHi);
			memcpy(pHi, ws.c_str(), cb);
			return true;
		}
	};
}	NetRes;

//! NetRes の vector
typedef std::vector<NetRes> NetReses;

//! UNIVERSAL_NAME_INFO ラッパー
typedef struct UniversalNameInfo {
	std::wstring UniversalName; //! UNC パス
	
	typedef UNIVERSAL_NAME_INFO TOutput; //! 出力型
	
	//! 必要なバイト数
	DWORD Bytes() const {
		return sizeof(TOutput)
			+(UniversalName.empty() ? 0 : 2 * (UniversalName.size() +1))
			;
	}
	
	//! 書き出す
	bool WriteTo(PBYTE &pLo, PBYTE &pHi) const {
		if (pLo +sizeof(TOutput) > pHi)
			return false;
		TOutput *pNR = reinterpret_cast<TOutput *>(pLo);
		pLo += sizeof(TOutput);
		if (!Ut::Wr(pLo, pHi, UniversalName, pNR->lpUniversalName))
			return false;

		return true;
	}
	
	//! 補助
	struct Ut {
		//! 書き出す
		static bool Wr(PBYTE &pLo, PBYTE &pHi, const std::wstring &ws, LPWSTR &ppcw) {
			if (ws.empty()) {
				ppcw = NULL;
				return true;
			}
			int cb = 2 * (ws.size() +1);
			if (pHi -pLo < cb)
				return false;
			pHi -= cb;
			ppcw = reinterpret_cast<LPWSTR>(pHi);
			memcpy(pHi, ws.c_str(), cb);
			return true;
		}
	};
}	UniversalNameInfo;

//! NPGetResourceInformation を補助
typedef struct WriteRemainingPath {
	std::wstring RemainingPath; //! 残りのローカルパス
	
	//! 必要なバイト数
	DWORD Bytes() const {
		return 
			+(RemainingPath.empty() ? 0 : 2 * (RemainingPath.size() +1))
			;
	}
	
	//! 書き出す
	bool WriteTo(PBYTE &pLo, PBYTE &pHi, LPWSTR &ppcw) const {
		if (pLo > pHi)
			return false;
		if (!Ut::Wr(pLo, pHi, RemainingPath, ppcw))
			return false;

		return true;
	}
	
	//! 補助
	struct Ut {
		//! 書き出す
		static bool Wr(PBYTE &pLo, PBYTE &pHi, const std::wstring &ws, LPWSTR &ppcw) {
			if (ws.empty()) {
				ppcw = NULL;
				return true;
			}
			int cb = 2 * (ws.size() +1);
			if (pHi -pLo < cb)
				return false;
			pHi -= cb;
			ppcw = reinterpret_cast<LPWSTR>(pHi);
			memcpy(pHi, ws.c_str(), cb);
			return true;
		}
	};
}	WriteRemainingPath;

//! __out LPTSTR 文字列を作成補助
class Utsw {
public:
	//! 書き出す
	static DWORD WriteTo(LPWSTR pwDest, DWORD *lpnLength, LPCWSTR pcwNew) {
		if (pwDest == NULL)
			return ERROR_INVALID_PARAMETER;
		if (lpnLength == NULL)
			return ERROR_INVALID_PARAMETER;
		
		DWORD &cchDest = *lpnLength;
		if (pcwNew == NULL) {
			if (cchDest < 2) {
				cchDest = 2;
				return WN_MORE_DATA;
			}
			*pwDest = 0;
			return WN_SUCCESS;
		}
		else {
			DWORD cchSrc = lstrlen(pcwNew) +1;
			if (cchDest < cchSrc) {
				cchDest = cchSrc;
				return false;
			}
			StringCchCopyW(pwDest, cchDest, pcwNew);
			return WN_SUCCESS;
		}
	}
};

#endif // _NREDNP_H_
