
#ifndef _NREDNP_H_
#define _NREDNP_H_

#include <string>
#include <vector>
#include <set>

//! ���L
typedef struct Share {
	std::wstring server; //! �T�[�o�[��
	std::wstring share; //! ���L��
	std::wstring ntPath; //! \Device\DokanRedirector{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}
	
	//! ���
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

	//! �T�[�o�[�����������ǂ���������
	bool test(LPCWSTR pcw) {
		if (pcw != NULL && pcw[0] == L'\\' && pcw[1] == L'\\') {
			if (StrCmpI(server.c_str(), pcw +2) == 0) {
				return true;
			}
		}
		return false;
	}
}	Share;

//! Share �� vector
typedef std::vector<Share> Shares;

//! UNC �p�X�̉�̓T�|�[�g
typedef struct UPath {
	std::wstring server; //! �T�[�o�[��
	std::wstring share; //! ���L��
	std::wstring local; //! ���[�J���p�X
	
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

//! �啶������������ʂ��Ȃ���r���Z�q
typedef struct {
	bool operator()(
		const std::wstring &lv, 
		const std::wstring &rv
	) const {
		return StrCmpIW(lv.c_str(), rv.c_str()) < 0;
	}
}	CompareNoCase;

//! �T�[�o�[���̃Z�b�g
typedef std::set<std::wstring, CompareNoCase> ServerSet;

//! NETRESOURCE ���b�p�[
typedef struct NetRes {
	DWORD Scope; //! RESOURCE_ �萔
	DWORD Type; //! RESOURCETYPE_ �萔
	DWORD DisplayType; //! RESOURCEDISPLAYTYPE_ �萔
	DWORD Usage; //! RESOURCEUSAGE_ �萔
	std::wstring LocalName, RemoteName, Comment, Provider;
	
	//! �K�v�ȃo�C�g��
	DWORD Bytes() const {
		return sizeof(NETRESOURCE)
			+(LocalName.empty() ? 0 : 2 * (LocalName.size() +1))
			+(RemoteName.empty() ? 0 : 2 * (RemoteName.size() +1))
			+(Comment.empty() ? 0 : 2 * (Comment.size() +1))
			+(Provider.empty() ? 0 : 2 * (Provider.size() +1))
			;
	}
	
	//! �o�b�t�@�ɏ����o��
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
	
	//! �⏕
	struct Ut {
		//! �����o��
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

//! NetRes �� vector
typedef std::vector<NetRes> NetReses;

//! UNIVERSAL_NAME_INFO ���b�p�[
typedef struct UniversalNameInfo {
	std::wstring UniversalName; //! UNC �p�X
	
	typedef UNIVERSAL_NAME_INFO TOutput; //! �o�͌^
	
	//! �K�v�ȃo�C�g��
	DWORD Bytes() const {
		return sizeof(TOutput)
			+(UniversalName.empty() ? 0 : 2 * (UniversalName.size() +1))
			;
	}
	
	//! �����o��
	bool WriteTo(PBYTE &pLo, PBYTE &pHi) const {
		if (pLo +sizeof(TOutput) > pHi)
			return false;
		TOutput *pNR = reinterpret_cast<TOutput *>(pLo);
		pLo += sizeof(TOutput);
		if (!Ut::Wr(pLo, pHi, UniversalName, pNR->lpUniversalName))
			return false;

		return true;
	}
	
	//! �⏕
	struct Ut {
		//! �����o��
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

//! NPGetResourceInformation ��⏕
typedef struct WriteRemainingPath {
	std::wstring RemainingPath; //! �c��̃��[�J���p�X
	
	//! �K�v�ȃo�C�g��
	DWORD Bytes() const {
		return 
			+(RemainingPath.empty() ? 0 : 2 * (RemainingPath.size() +1))
			;
	}
	
	//! �����o��
	bool WriteTo(PBYTE &pLo, PBYTE &pHi, LPWSTR &ppcw) const {
		if (pLo > pHi)
			return false;
		if (!Ut::Wr(pLo, pHi, RemainingPath, ppcw))
			return false;

		return true;
	}
	
	//! �⏕
	struct Ut {
		//! �����o��
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

//! __out LPTSTR ��������쐬�⏕
class Utsw {
public:
	//! �����o��
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
