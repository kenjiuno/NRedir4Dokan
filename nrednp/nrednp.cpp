
#pragma warning(disable: 4530)

#include <windows.h>
#include <winnetwk.h>
#include <winsvc.h>
#include <stdio.h>
#include <npapi.h>
#include <strsafe.h>
#include <shlwapi.h>

#include "nrednp.h"

static VOID
DokanDbgPrintW(LPCWSTR format, ...)
{
	WCHAR buffer[512];
	va_list argp;
	va_start(argp, format);
	StringCchVPrintfW(buffer, 127, format, argp);
	va_end(argp);
	OutputDebugStringW(buffer);
}

#define DbgPrintW(format, ...) \
	DokanDbgPrintW(format, __VA_ARGS__)

DWORD APIENTRY
NPGetCaps(
	DWORD Index)
{
	DWORD rc = 0;
	DbgPrintW(L"NPGetCaps %d\n", Index);

	switch (Index) {
	case WNNC_SPEC_VERSION:
		DbgPrintW(L"  WNNC_SPEC_VERSION\n");
		rc = WNNC_SPEC_VERSION51;
		break;
 
	case WNNC_NET_TYPE:
		DbgPrintW(L"  WNNC_NET_TYPE\n");
		rc = WNNC_NET_RDR2SAMPLE;
		break;
  
	case WNNC_DRIVER_VERSION:
		DbgPrintW(L"  WNNC_DRIVER_VERSION\n");
		rc = 1;
		break;

	case WNNC_CONNECTION:
		DbgPrintW(L"  WNC_CONNECTION\n");
		rc = WNNC_CON_GETCONNECTIONS|WNNC_CON_GETPERFORMANCE;
		break;

	case WNNC_ENUMERATION:
		DbgPrintW(L"  WNNC_ENUMERATION\n");
		rc = WNNC_ENUM_LOCAL|WNNC_ENUM_GLOBAL|WNNC_ENUM_CONTEXT;
		break;
		
	case WNNC_START:
		DbgPrintW(L"  WNNC_START\n");
		rc = 1;
		break;

	case WNNC_USER:
		DbgPrintW(L"  WNNC_USER\n");
		rc = 0;
		break;
	case WNNC_DIALOG:
		DbgPrintW(L"  WNNC_DIALOG\n");
		rc = WNNC_DLG_GETRESOURCEINFORMATION|WNNC_DLG_PROPERTYDIALOG|WNNC_DLG_FORMATNETWORKNAME;
		break;
	case WNNC_ADMIN:
		DbgPrintW(L"  WNNC_ADMIN\n");
		rc = 0;
		break;
	default:
		DbgPrintW(L"  default\n");
		rc = 0;
		break;
	}

	return rc;
}

DWORD APIENTRY
NPLogonNotify(
	__in PLUID		LogonId,
	__in PCWSTR		AuthentInfoType,
	__in PVOID		AuthentInfo,
	__in PCWSTR		PreviousAuthentInfoType,
	__in PVOID		PreviousAuthentInfo,
	__in PWSTR		StationName,
	__in PVOID		StationHandle,
	__out PWSTR		*LogonScript)
{
	DbgPrintW(L"NPLogonNotify\n");
	*LogonScript = NULL;
	return WN_SUCCESS;
}

DWORD APIENTRY
NPPasswordChangeNotify(
	__in LPCWSTR AuthentInfoType,
	__in LPVOID AuthentInfo,
	__in LPCWSTR PreviousAuthentInfoType,
	__in LPVOID RreviousAuthentInfo,
	__in LPWSTR StationName,
	__in PVOID StationHandle,
	__in DWORD ChangeInfo)
{
	DbgPrintW(L"NPPasswordChangeNotify\n");
	SetLastError(WN_NOT_SUPPORTED);
	return WN_NOT_SUPPORTED;
}


DWORD APIENTRY
NPAddConnection(
	__in LPNETRESOURCE NetResource,
	__in LPWSTR Password,
	__in LPWSTR UserName)
{
	DbgPrintW(L"NPAddConnection\n");
	return  NPAddConnection3(NULL, NetResource, Password, UserName, 0);
}

DWORD APIENTRY
NPAddConnection3(
	__in HWND WndOwner,
	__in LPNETRESOURCE NetResource,
	__in LPWSTR Password,
	__in LPWSTR UserName,
	__in DWORD Flags)
{
	DWORD status;
	WCHAR temp[128];
	WCHAR local[3];

	DbgPrintW(L"NPAddConnection3\n");
	DbgPrintW(L"  LocalName: %s\n", NetResource->lpLocalName);
	DbgPrintW(L"  RemoteName: %s\n", NetResource->lpRemoteName);
 
	ZeroMemory(local, sizeof(local));

	if (lstrlen(NetResource->lpLocalName) > 1 &&
		NetResource->lpLocalName[1] == L':') {
		local[0] = (WCHAR)toupper(NetResource->lpLocalName[0]);
		local[1] = L':';
		local[2] = L'\0';
	}

	if (QueryDosDevice(local, temp, 128)) {
		DbgPrintW(L"  WN_ALREADY_CONNECTED");
		status = WN_ALREADY_CONNECTED;
	} else {
		DbgPrintW(L"  WN_BAD_NETNAME");
		status = WN_BAD_NETNAME;
	}

	return status;
}

DWORD APIENTRY
NPCancelConnection(
	__in LPWSTR Name,
	__in BOOL Force)
{
	DbgPrintW(L"NpCancelConnection %s %d\n", Name, Force);
	return WN_SUCCESS;
}

typedef struct EnumIt {
	NetReses reses;
	size_t y;
	
	EnumIt() {
		y = 0;
	}
}	EnumIt;

bool GetShares(Shares &shares) {
	LONG r;
	HKEY hk = NULL;
	bool any = false;
	r = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE, 
		L"SYSTEM\\CurrentControlSet\\Services\\NRedir4Dokan\\Map",
		0,
		KEY_READ,
		&hk
		);
	if (ERROR_SUCCESS == r) {
		DWORD y = 0;
		for (; ; y++) {
			WCHAR wcName[256] = {0};
			DWORD cch = 256;
			DWORD ty = 0;
			BYTE buff[256] = {0};
			DWORD cb = 256;
			r = RegEnumValueW(
				hk,
				y,
				wcName,
				&cch,
				NULL,
				&ty,
				buff,
				&cb
				);
			if (ERROR_SUCCESS != r)
				break;
			if (ty == REG_SZ) {
				LPCWSTR pcw = reinterpret_cast<LPCWSTR>(buff);
				if (StrStrIW(pcw, L"Dokan") != NULL) {
					Share s1;
					if (s1.parse(wcName)) {
						s1.ntPath = pcw;
						shares.push_back(s1);
					}
				}
			}
		}
		r = RegCloseKey(hk);
		any |= true;
	}
	r = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE, 
		L"SYSTEM\\CurrentControlSet\\Services\\NRedir4Dokan\\MapFull",
		0,
		KEY_READ,
		&hk
		);
	if (ERROR_SUCCESS == r) {
		DWORD y = 0;
		for (; ; y++) {
			WCHAR wcName[256] = {0};
			DWORD cch = 256;
			DWORD ty = 0;
			BYTE buff[256] = {0};
			DWORD cb = 256;
			r = RegEnumValueW(
				hk,
				y,
				wcName,
				&cch,
				NULL,
				&ty,
				buff,
				&cb
				);
			if (ERROR_SUCCESS != r)
				break;
			if (ty == REG_SZ) {
				LPCWSTR pcw = reinterpret_cast<LPCWSTR>(buff);
				if (StrStrIW(pcw, L"Dokan") != NULL) {
					Share s1;
					if (s1.parse(wcName)) {
						s1.ntPath = pcw;
						shares.push_back(s1);
					}
				}
			}
		}
		r = RegCloseKey(hk);
		any |= true;
	}
	return any;
}

DWORD APIENTRY
NPGetConnection(
	__in LPWSTR LocalName,
	__out LPWSTR RemoteName,
	__inout LPDWORD BufferSize)
{
	DbgPrintW(L"NPGetConnection %s %d\n", LocalName, *BufferSize);

	if (iswalpha(LocalName[0]) && LocalName[1] == L':') {
		WCHAR wcLocal[10] = {LocalName[0], ':'};
		WCHAR wcTarget[256] = {0};
		
		if (QueryDosDevice(wcLocal, wcTarget, 256) != 0) {
			Shares shares;
			if (GetShares(shares)) {
				Shares::iterator iter = shares.begin();
				for (; iter != shares.end(); iter++) {
					if (StrCmpIW(iter->ntPath.c_str(), wcTarget) == 0) {
						DWORD cchNeed = 2 + iter->server.size() + 1 + iter->share.size() + 1;
						if (cchNeed > *BufferSize) {
							*BufferSize = cchNeed;
							return WN_MORE_DATA;
						}
						
						LPWSTR term = RemoteName + *BufferSize;
						StringCchCopyW(RemoteName, term -RemoteName, L"\\\\");
						StringCchCatW(RemoteName, term -RemoteName, iter->server.c_str());
						StringCchCatW(RemoteName, term -RemoteName, L"\\");
						StringCchCatW(RemoteName, term -RemoteName, iter->share.c_str());
						return WN_SUCCESS;
					}
				}
			}
		}
	}

	return WN_BAD_NETNAME;
}

DWORD APIENTRY
NPOpenEnum(
	__in DWORD Scope,
	__in DWORD Type,
	__in DWORD Usage,
	__in LPNETRESOURCE NetResource,
	__in LPHANDLE Enum)
{
	DWORD status;
	DbgPrintW(L"NPOpenEnum\n");
	if (Enum == NULL)
		return WN_BAD_VALUE;
	switch (Scope){
	case RESOURCE_CONTEXT:
	case RESOURCE_GLOBALNET:
	{
		Shares shares;
		if (GetShares(shares)) {
			if (Type == 0 || Type == RESOURCEUSAGE_ATTACHED || 0 != (Type & RESOURCETYPE_DISK)) {
				if (NetResource == NULL || NetResource->lpRemoteName == NULL) {
					// \\servers
					ServerSet sset;
					{
						Shares::iterator iter = shares.begin();
						for (; iter != shares.end(); iter++) {
							sset.insert(std::wstring()
								.append(L"\\\\")
								.append(iter->server)
							);
						}
					}
					EnumIt *pIt = new EnumIt();
					{
						ServerSet::iterator iter = sset.begin();
						for (; iter != sset.end(); iter++) {
							NetRes r1;
							r1.Scope = RESOURCE_GLOBALNET;
							r1.Type = RESOURCETYPE_DISK;
							r1.DisplayType = RESOURCEDISPLAYTYPE_SERVER;
							r1.Usage = RESOURCEUSAGE_CONTAINER;
							r1.RemoteName = *iter;
							r1.Provider = L"NRedir4Dokan";
							pIt->reses.push_back(r1);
						}
					}
					*Enum = pIt;
				}
				else if (NetResource != NULL || NetResource->lpRemoteName != NULL) {
					// \\server\shares
					ServerSet sset;
					{
						Shares::iterator iter = shares.begin();
						for (; iter != shares.end(); iter++) {
							if (iter->test(NetResource->lpRemoteName)) {
								sset.insert(std::wstring()
									.append(L"\\\\")
									.append(iter->server)
									.append(L"\\")
									.append(iter->share)
								);
							}
						}
					}
					EnumIt *pIt = new EnumIt();
					{
						ServerSet::iterator iter = sset.begin();
						for (; iter != sset.end(); iter++) {
							NetRes r1;
							r1.Scope = RESOURCE_GLOBALNET;
							r1.Type = RESOURCETYPE_DISK;
							r1.DisplayType = RESOURCEDISPLAYTYPE_SHARE;
							r1.Usage = RESOURCEUSAGE_CONNECTABLE;
							r1.RemoteName = *iter;
							r1.Provider = L"NRedir4Dokan";
							pIt->reses.push_back(r1);
						}
					}
					*Enum = pIt;
				}
			}
		}
		status = WN_SUCCESS;
		break;
	}
	default:
		status  = WN_NOT_SUPPORTED;
		break;
	}
	return status;
}


DWORD APIENTRY
NPCloseEnum(
	__in HANDLE Enum)
{
	DbgPrintW(L"NpCloseEnum\n");
	if (Enum != NULL)
		delete reinterpret_cast<EnumIt *>(Enum);
	return WN_SUCCESS;
}


DWORD APIENTRY
NPGetResourceParent(
	__in LPNETRESOURCE NetResource,
	__in LPVOID Buffer,
	__in LPDWORD BufferSize)
{
	DbgPrintW(L"NPGetResourceParent\n");
	return WN_NOT_SUPPORTED;
}


DWORD APIENTRY
NPEnumResource(
	__in HANDLE Enum,
	__in LPDWORD Count,
	__in LPVOID Buffer,
	__in LPDWORD BufferSize)
{
	DbgPrintW(L"NPEnumResource\n");
	if (Enum == NULL || Count == NULL || Buffer == NULL || BufferSize == NULL)
		return WN_BAD_HANDLE;
	EnumIt &rIt = *reinterpret_cast<EnumIt *>(Enum);
	if (rIt.y >= rIt.reses.size())
		return WN_NO_MORE_ENTRIES;
	DWORD n = 0;
	DWORD cbNeed = 0;
	for (; n < *Count && rIt.y +n < rIt.reses.size(); n++) {
		cbNeed += rIt.reses[rIt.y +n].Bytes();
		if (*BufferSize < cbNeed) {
			if (n == 0) {
				*BufferSize = cbNeed;
				return WN_MORE_DATA;
			}
			break;
		}
	}
	DWORD i = 0;
	PBYTE pLo = reinterpret_cast<PBYTE>(Buffer);
	PBYTE pHi = pLo + *BufferSize;
	for (; i < n; i++) {
		rIt.reses[rIt.y +i].WriteTo(pLo, pHi);
	}
	rIt.y += n;
	*Count = n;
	return WN_SUCCESS;
}

typedef struct RemoteNameInfo  { // REMOTE_NAME_INFO structure
	std::wstring UniversalName, ConnectionName, RemainingPath;
	
	typedef REMOTE_NAME_INFO TOutput;
	
	DWORD Bytes() const {
		return sizeof(TOutput)
			+(UniversalName.empty() ? 0 : 2 * (UniversalName.size() +1))
			+(ConnectionName.empty() ? 0 : 2 * (ConnectionName.size() +1))
			+(RemainingPath.empty() ? 0 : 2 * (RemainingPath.size() +1))
			;
	}
	
	bool WriteTo(PBYTE &pLo, PBYTE &pHi) const {
		if (pLo +sizeof(TOutput) > pHi)
			return false;
		TOutput *pNR = reinterpret_cast<TOutput *>(pLo);
		pLo += sizeof(TOutput);
		if (!Ut::Wr(pLo, pHi, UniversalName, pNR->lpUniversalName))
			return false;
		if (!Ut::Wr(pLo, pHi, ConnectionName, pNR->lpConnectionName))
			return false;
		if (!Ut::Wr(pLo, pHi, RemainingPath, pNR->lpRemainingPath))
			return false;

		return true;
	}
	
	struct Ut {
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
}	RemoteNameInfo;


DWORD APIENTRY
NPGetUniversalName(
	__in LPCWSTR LocalPath,
	__in DWORD InfoLevel,
	__in LPVOID Buffer,
	__in LPDWORD BufferSize)
{
	DbgPrintW(L"NPGetUniversalName %s %u \n", LocalPath, InfoLevel);
	if (LocalPath == NULL)
		return ERROR_INVALID_PARAMETER;
	if (BufferSize == NULL)
		return ERROR_INVALID_PARAMETER;
	if (iswalpha(LocalPath[0]) && LocalPath[1] == L':') {
		WCHAR wcLocal[10] = {LocalPath[0], ':'};
		WCHAR wcTarget[256] = {0};
		
		if (QueryDosDevice(wcLocal, wcTarget, 256) != 0) {
			Shares shares;
			if (GetShares(shares)) {
				Shares::iterator iter = shares.begin();
				for (; iter != shares.end(); iter++) {
					if (StrCmpIW(iter->ntPath.c_str(), wcTarget) == 0) {
						if (LocalPath[2] != 0 && LocalPath[2] != L'\\')
							return ERROR_INVALID_PARAMETER;
						
						if (false) { }
						else if (InfoLevel == UNIVERSAL_NAME_INFO_LEVEL) {
							UniversalNameInfo oo;
							oo.UniversalName.assign(L"")
								.append(L"\\\\")
								.append(iter->server)
								.append(L"\\")
								.append(iter->share)
								.append(LocalPath +2)
								;
							
							if (oo.Bytes() > *BufferSize) {
								*BufferSize = oo.Bytes();
								return WN_MORE_DATA;
							}

							PBYTE pLo = reinterpret_cast<PBYTE>(Buffer);
							PBYTE pHi = pLo + *BufferSize;
							oo.WriteTo(pLo, pHi);
							return WN_SUCCESS;
						}
						else if (InfoLevel == REMOTE_NAME_INFO_LEVEL) {
							RemoteNameInfo oo;
							oo.UniversalName.assign(L"")
								.append(L"\\\\")
								.append(iter->server)
								.append(L"\\")
								.append(iter->share)
								.append(LocalPath +2)
								;
							oo.ConnectionName.assign(L"")
								.append(L"\\\\")
								.append(iter->server)
								.append(L"\\")
								.append(iter->share)
								;
							oo.RemainingPath.assign(LocalPath +2)
								;
							
							if (oo.Bytes() > *BufferSize) {
								*BufferSize = oo.Bytes();
								return WN_MORE_DATA;
							}

							PBYTE pLo = reinterpret_cast<PBYTE>(Buffer);
							PBYTE pHi = pLo + *BufferSize;
							oo.WriteTo(pLo, pHi);
							return WN_SUCCESS;
						}
						else return ERROR_INVALID_PARAMETER;
					}
				}
			}
		}
	}
	return WN_NOT_CONNECTED;
}


DWORD APIENTRY
NPGetResourceInformation(
	__in LPNETRESOURCE NetResource,
	__out LPVOID Buffer,
	__out LPDWORD BufferSize,
	__out LPWSTR *System)
{
	DbgPrintW(L"NPGetResourceInformation %u \n", *BufferSize);
	if (NetResource == NULL)
		return ERROR_INVALID_PARAMETER;
	if (BufferSize == NULL)
		return ERROR_INVALID_PARAMETER;
	
	UPath unc;
	if (unc.parse(NetResource->lpRemoteName)) {
		Shares shares;
		if (GetShares(shares)) {
			Shares::iterator iter = shares.begin();
			for (; iter != shares.end(); iter++) {
				if (true
					&& StrCmpIW(iter->server.c_str(), unc.server.c_str()) == 0
					&& StrCmpIW(iter->share.c_str(), unc.share.c_str()) == 0
				) {
					NetRes r1 = {0};
					r1.RemoteName
						.append(L"\\\\")
						.append(iter->server)
						.append(L"\\")
						.append(iter->share)
						;
					r1.Provider = L"NRedir4Dokan";
					r1.Type = RESOURCETYPE_DISK;
					r1.DisplayType = RESOURCEDISPLAYTYPE_SHARE;
					r1.Usage = RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_NOLOCALDEVICE;
					
					bool useSystem = (System != NULL && !unc.local.empty());
					WriteRemainingPath r2;
					if (useSystem) {
						r2.RemainingPath
							.append(L"\\")
							.append(unc.local)
							;
					}
					
					DWORD cbWrite = r1.Bytes() + r2.Bytes();
					if (cbWrite > *BufferSize) {
						*BufferSize = cbWrite;
						return WN_MORE_DATA;
					}

					PBYTE pLo = reinterpret_cast<PBYTE>(Buffer);
					PBYTE pHi = pLo + *BufferSize;
					r1.WriteTo(pLo, pHi);
					if (useSystem) {
						r2.WriteTo(pLo, pHi, *System);
					}
					return WN_SUCCESS;
				}
			}
		}
	}
	return WN_BAD_NETNAME;
}

DWORD APIENTRY
NPGetConnectionPerformance(
	__in LPTSTR                 lpRemoteName,
	__out LPNETCONNECTINFOSTRUCT lpNetConnectInfo)
{
	DbgPrintW(L"NPGetResourceInformation %s \n", lpRemoteName);

	if (lpNetConnectInfo == NULL)
		return ERROR_INVALID_PARAMETER;
	if (lpNetConnectInfo->cbStructure < sizeof(NETCONNECTINFOSTRUCT))
		return ERROR_INVALID_PARAMETER;

	UPath unc;
	if (unc.parse(lpRemoteName)) {
		Shares shares;
		if (GetShares(shares)) {
			Shares::iterator iter = shares.begin();
			for (; iter != shares.end(); iter++) {
				if (true
					&& StrCmpIW(iter->server.c_str(), unc.server.c_str()) == 0
					&& StrCmpIW(iter->share.c_str(), unc.share.c_str()) == 0
				) {
					lpNetConnectInfo->dwFlags = WNCON_SLOWLINK;
					lpNetConnectInfo->dwSpeed = 1000000 / 100;
					lpNetConnectInfo->dwDelay = 50;
					lpNetConnectInfo->dwOptDataSize = 65536;
					return WN_SUCCESS;
				}
			}
		}
	}
	return WN_NO_NETWORK;
}

DWORD NPGetPropertyText(
	__in    DWORD  iButton,
	__in    DWORD  nPropSel,
	__in    LPTSTR lpName,
	__out   LPTSTR lpButtonName,
	__inout DWORD  nButtonNameLen,
	__in    DWORD  nType)
{
	DbgPrintW(L"NPGetPropertyText %s \n", lpName);
	return WN_NOT_SUPPORTED;
}

DWORD NPFormatNetworkName(
	__in    LPTSTR  lpRemoteName,
	__out   LPTSTR  lpFormattedName,
	__inout LPDWORD lpnLength,
	__in    DWORD   dwFlags,
	__in    DWORD   dwAveCharPerLine)
{
	DbgPrintW(L"NPFormatNetworkName %s %u %u \n", lpRemoteName, dwFlags, dwAveCharPerLine);
	if (lpnLength == NULL)
		return ERROR_INVALID_PARAMETER;
	
	if (0 != (dwFlags & WNFMT_ABBREVIATED)) {
		UPath unc;
		if (unc.parse(lpRemoteName)) {
			 return Utsw::WriteTo(lpFormattedName, lpnLength, unc.share.c_str());
		}
	}
	
	return WN_BAD_NETNAME;
}

DWORD APIENTRY
NPGetConnection3(
	__in    LPCWSTR lpLocalName,
	__in    DWORD   dwLevel,
	__out   LPVOID  lpBuffer,
	__inout LPDWORD lpBufferSize)
{
	DbgPrintW(L"NPGetConnection3 %s %u %u \n", lpLocalName, dwLevel, *lpBufferSize);

	if (dwLevel == 1) {
		if (lpBufferSize == NULL)
			return ERROR_INVALID_PARAMETER;
		if (*lpBufferSize < 4) {
			*lpBufferSize = 4;
			return WN_MORE_DATA;
		}
		*reinterpret_cast<DWORD *>(lpBuffer) = WNGETCON_CONNECTED;
		*lpBufferSize = 4;
		return WN_SUCCESS;
	}
	return ERROR_INVALID_PARAMETER;
}
