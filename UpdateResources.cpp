#include "UpdateResources.h"

// defines to make deferencing easier
#define OffsetToString ss.ulOffsetToString
#define cbData         ss.cbD
#define cbsz           ss.cb
#define szStr          ss.sz

#define FREE_RES_ID( _Res_ )                             \
    if (IS_ID == (_Res_)->discriminant)                  \
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)(_Res_)) \

void FreeOne(PRESNAME pRes)
{
    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pRes->OffsetToDataEntry);
    FREE_RES_ID(pRes->Name);
    FREE_RES_ID(pRes->Type);
    RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pRes);
}


void FreeStrings(PUPDATEDATA pUpd)
{
    PSDATA      pstring, pStringTmp;

    pstring = pUpd->StringHead;
    while (pstring != NULL) {
        pStringTmp = pstring->uu.ss.pnext;
        if (pstring->discriminant == IS_STRING)
            RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pstring->szStr);
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pstring);
        pstring = pStringTmp;
    }

    return;
}

void FreeData(PUPDATEDATA pUpd)
{
    PRESTYPE    pType;
    PRESNAME    pRes;

    for (pType = pUpd->ResTypeHeadID; pUpd->ResTypeHeadID; pType = pUpd->ResTypeHeadID) {
        pUpd->ResTypeHeadID = pUpd->ResTypeHeadID->pnext;

        for (pRes = pType->NameHeadID; pType->NameHeadID; pRes = pType->NameHeadID) {
            pType->NameHeadID = pType->NameHeadID->pnext;
            FreeOne(pRes);
        }

        for (pRes = pType->NameHeadName; pType->NameHeadName; pRes = pType->NameHeadName) {
            pType->NameHeadName = pType->NameHeadName->pnext;
            FreeOne(pRes);
        }

        FREE_RES_ID(pType->Type);
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pType);
    }

    for (pType = pUpd->ResTypeHeadName; pUpd->ResTypeHeadName; pType = pUpd->ResTypeHeadName) {
        pUpd->ResTypeHeadName = pUpd->ResTypeHeadName->pnext;

        for (pRes = pType->NameHeadID; pType->NameHeadID; pRes = pType->NameHeadID) {
            pType->NameHeadID = pType->NameHeadID->pnext;
            FreeOne(pRes);
        }

        for (pRes = pType->NameHeadName; pType->NameHeadName; pRes = pType->NameHeadName) {
            pType->NameHeadName = pType->NameHeadName->pnext;
            FreeOne(pRes);
        }

        // no FREE_RES_ID needed here.
        RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)pType);
    }

    FreeStrings(pUpd);

    return;
}

ULONG
MuMoveFilePos(INT fh, ULONG pos)
{
    return _llseek(fh, pos, SEEK_SET);
}



ULONG
MuWrite(INT fh, PVOID p, ULONG n)
{
    ULONG       n1;

    if ((n1 = _lwrite(fh, (const char *)p, n)) != n) {
        return n1;
    }
    else
        return 0;
}



ULONG
MuRead(INT fh, UCHAR *p, ULONG n)
{
    ULONG       n1;

    if ((n1 = _lread(fh, p, n)) != n) {
        return n1;
    }
    else
        return 0;
}

LONG
PEWriteResFile(
    INT inpfh,
    INT outfh,
    ULONG cbOldexe,
    PUPDATEDATA pUpdate
)

{

    IMAGE_NT_HEADERS32 Old;

    //
    // Position file to start of NT header and read the image header.
    //

    MuMoveFilePos(inpfh, cbOldexe);
    MuRead(inpfh, (PUCHAR)&Old, sizeof(IMAGE_NT_HEADERS32));

    //
    // If the file is not an NT image, then return an error.
    //

    if (Old.Signature != IMAGE_NT_SIGNATURE) {
        return ERROR_INVALID_EXE_SIGNATURE;
    }

    //
    // If the file is not an executable or a dll, then return an error.
    //

    if ((Old.FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0 &&
        (Old.FileHeader.Characteristics & IMAGE_FILE_DLL) == 0) {
        return ERROR_EXE_MARKED_INVALID;
    }

    //
    // Call the proper function dependent on the machine type.
    //

    if (Old.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC) {
        return PEWriteResource(inpfh, outfh, cbOldexe, pUpdate, (IMAGE_NT_HEADERS64 *)&Old);
    }
    else if (Old.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR32_MAGIC) {
        return PEWriteResource(inpfh, outfh, cbOldexe, pUpdate, (IMAGE_NT_HEADERS32 *)&Old);
    }
    else {
        return ERROR_BAD_EXE_FORMAT;
    }
}

LONG WriteResFile(HANDLE hUpdate, WCHAR *pDstname)
{
    INT         inpfh;
    INT         outfh;
    ULONG       onewexe;
    IMAGE_DOS_HEADER    oldexe;
    PUPDATEDATA pUpdate;
    INT         rc;
    WCHAR *pFilename;

    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    if (pUpdate == NULL) {
        return GetLastError();
    }
    pFilename = (WCHAR *)GlobalLock(pUpdate->hFileName);
    if (pFilename == NULL) {
        GlobalUnlock(hUpdate);
        return GetLastError();
    }

    /* open the original exe file */
    inpfh = HandleToUlong(CreateFileW(pFilename, GENERIC_READ,
        0 /*exclusive access*/, NULL /* security attr */,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL));
    GlobalUnlock(pUpdate->hFileName);
    if (inpfh == -1) {
        GlobalUnlock(hUpdate);
        return ERROR_OPEN_FAILED;
    }

    /* read the old format EXE header */
    rc = _lread(inpfh, (char *)&oldexe, sizeof(oldexe));
    if (rc != sizeof(oldexe)) {
        _lclose(inpfh);
        GlobalUnlock(hUpdate);
        return ERROR_READ_FAULT;
    }

    /* make sure its really an EXE file */
    if (oldexe.e_magic != IMAGE_DOS_SIGNATURE) {
        _lclose(inpfh);
        GlobalUnlock(hUpdate);
        return ERROR_INVALID_EXE_SIGNATURE;
    }

    /* make sure theres a new EXE header floating around somewhere */
    if (!(onewexe = oldexe.e_lfanew)) {
        _lclose(inpfh);
        GlobalUnlock(hUpdate);
        return ERROR_BAD_EXE_FORMAT;
    }

    outfh = HandleToUlong(CreateFileW(pDstname, GENERIC_READ | GENERIC_WRITE,
        0 /*exclusive access*/, NULL /* security attr */,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL));

    if (outfh != -1) {
        rc = PEWriteResFile(inpfh, outfh, onewexe, pUpdate);
        _lclose(outfh);
    }
    _lclose(inpfh);
    GlobalUnlock(hUpdate);
    return rc;
}

HANDLE NTMU_BeginUpdateResource(LPCWSTR pszPath)
{
    HMODULE     hModule;
    PUPDATEDATA pUpdate;
    HANDLE      hUpdate;
    LPWSTR      pFileName;
    DWORD       attr;
    size_t      cchFileNameLen;
    HRESULT     hr;

    SetLastError(NO_ERROR);
    if (pszPath == NULL) {
        SetLastError(ERROR_INVALID_PARAMETER);
        goto cleanup;
    }

    hUpdate = GlobalAlloc(GHND, sizeof(UPDATEDATA));
    if (hUpdate == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    if (pUpdate == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup_hupdate;
    }

    hr = StringCchLengthW(pszPath, STRSAFE_MAX_CCH, &cchFileNameLen);
    if (FAILED(hr)) {
        SetLastError(HRESULT_CODE(hr));
        goto cleanup_pupdate;
    }

    pUpdate->Status = NO_ERROR;
    pUpdate->hFileName = GlobalAlloc(GHND, (cchFileNameLen + 1) * sizeof(WCHAR));
    if (pUpdate->hFileName == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup_pupdate;
    }
    pFileName = (LPWSTR)GlobalLock(pUpdate->hFileName);
    if (pFileName == NULL) {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup_pfilename;
    }
    hr = StringCchCopyW(pFileName,
        cchFileNameLen + 1,
        pszPath);

    GlobalUnlock(pUpdate->hFileName);

    if (FAILED(hr)) {
        SetLastError(HRESULT_CODE(hr));
        goto cleanup_pfilename;
    }

    attr = GetFileAttributesW(pFileName);
    if (attr == 0xffffffff) {
        goto cleanup_pfilename;
    }
    else if (attr & (FILE_ATTRIBUTE_READONLY |
        FILE_ATTRIBUTE_SYSTEM |
        FILE_ATTRIBUTE_HIDDEN |
        FILE_ATTRIBUTE_DIRECTORY)) {
        SetLastError(ERROR_WRITE_PROTECT);
        goto cleanup_pfilename;
    }

    hModule = LoadLibraryExW(pszPath, NULL, LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES);
    if (hModule == NULL) {
        if (GetLastError() == NO_ERROR)
            SetLastError(ERROR_BAD_EXE_FORMAT);
        goto cleanup_pfilename;
    }
    FreeLibrary(hModule);

    if (pUpdate->Status != NO_ERROR) {
        // return code set by enum functions
        goto cleanup_pfilename;
    }
    GlobalUnlock(hUpdate);
    return hUpdate;

cleanup_pfilename:
    GlobalFree(pUpdate->hFileName);

cleanup_pupdate:
    GlobalUnlock(hUpdate);

cleanup_hupdate:
    GlobalFree(hUpdate);

cleanup:
    return NULL;
}

BOOL NTMU_EndUpdateResource(HANDLE hUpdate, BOOL fDiscard)
{
    LPWSTR      pFileName;
    PUPDATEDATA pUpdate;
    WCHAR       pTempFileName[MAX_PATH];
    LPWSTR      p;
    LONG        rc;
    DWORD       LastError = 0;
    HRESULT     hr;

    SetLastError(0);
    pUpdate = (PUPDATEDATA)GlobalLock(hUpdate);
    if (fDiscard) {
        rc = NO_ERROR;
    }
    else {
        if (pUpdate == NULL) {
            return FALSE;
        }
        pFileName = (LPWSTR)GlobalLock(pUpdate->hFileName);
        if (pFileName != NULL) {
            hr = StringCchCopyExW(pTempFileName,
                RTL_NUMBER_OF(pTempFileName),
                pFileName,
                &p,
                NULL,
                0);
            if (FAILED(hr)) {
                rc = LastError = HRESULT_CODE(hr);
            }
            else {
                do {
                    p--;
                } while (*p != L'\\' && p >= pTempFileName);
                *(p + 1) = 0;
                rc = GetTempFileNameW(pTempFileName, L"RCX", 0, pTempFileName);
                if (rc == 0) {
                    rc = GetTempPathW(MAX_PATH, pTempFileName);
                    if (rc == 0) {
                        pTempFileName[0] = L'.';
                        pTempFileName[1] = L'\\';
                        pTempFileName[2] = 0;
                    }
                    rc = GetTempFileNameW(pTempFileName, L"RCX", 0, pTempFileName);
                    if (rc == 0) {
                        rc = GetLastError();
                    }
                    else {
                        rc = WriteResFile(hUpdate, pTempFileName);
                        if (rc == NO_ERROR) {
                            DeleteFileW(pFileName);
                            MoveFileW(pTempFileName, pFileName);
                        }
                        else {
                            LastError = rc;
                            DeleteFileW(pTempFileName);
                        }
                    }
                }
                else {
                    rc = WriteResFile(hUpdate, pTempFileName);
                    if (rc == NO_ERROR) {
                        DeleteFileW(pFileName);
                        MoveFileW(pTempFileName, pFileName);
                    }
                    else {
                        LastError = rc;
                        DeleteFileW(pTempFileName);
                    }
                }
            }
            GlobalUnlock(pUpdate->hFileName);
        }
        GlobalFree(pUpdate->hFileName);
    }

    if (pUpdate != NULL) {
        FreeData(pUpdate);
        GlobalUnlock(hUpdate);
    }
    GlobalFree(hUpdate);

    SetLastError(LastError);
    return rc ? FALSE : TRUE;
}