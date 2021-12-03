#include <iostream>
#include <Python.h>

#include "windows_signer.h"
#include <stdio.h>
#include <conio.h>
#include <tchar.h>
#include <bcrypt.h>
#include <ncrypt.h>

#include <openssl/ec.h>
#include <openssl/bn.h>
#include <openssl/opensslv.h>

#if OPENSSL_VERSION_NUMBER >= 0x10100000L
typedef struct ECDSA_SIG_st {
    BIGNUM *r;
    BIGNUM *s;
} ECDSA_SIG;
#endif

#define MY_ENCODING_TYPE  (PKCS_7_ASN_ENCODING | X509_ASN_ENCODING)
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#define STATUS_UNSUCCESSFUL ((NTSTATUS)0xC0000001L)

static bool EnableLogging = false;

void LogInfo(const std::string& message) {
  if (EnableLogging) {
    std::cout << "tls_offload.cpp: " << message << "...." << std::endl;
  }
}

void WinCertStoreKey::Cleanup() {
    LogInfo("Cleanup is called\n");
    if(pSignerCert) CertFreeCertificateContext(pSignerCert);
    if(hCertStore) CertCloseStore(hCertStore, CERT_CLOSE_STORE_CHECK_FLAG);
    if(hAlg) BCryptCloseAlgorithmProvider(hAlg,0);
    if (hHash) BCryptDestroyHash(hHash);
    if(pbHashObject) HeapFree(GetProcessHeap(), 0, pbHashObject);
    if(pbHash) HeapFree(GetProcessHeap(), 0, pbHash);
}

WinCertStoreKey::~WinCertStoreKey() {
    Cleanup();
}

void WinCertStoreKey::HandleError(LPTSTR psz) {
    _ftprintf(stderr, TEXT("An error occurred in the program. \n"));
    _ftprintf(stderr, TEXT("%s\n"), psz);
    _ftprintf(stderr, TEXT("Error number %x.\n"), GetLastError());
    Cleanup();
}

void WinCertStoreKey::GetSignerCert() {
    std::cout << "is_rsa_type: " << is_rsa_type << std::endl;
    std::cout << "cert_store_name: " << cert_store_name << std::endl;
    std::cout << "cert_subject: " << cert_subject << std::endl;
    std::wstring w_cert_store_name = std::wstring(cert_store_name.begin(), cert_store_name.end());
    std::wstring w_cert_subject = std::wstring(cert_subject.begin(), cert_subject.end());

    if (!(hCertStore = CertOpenStore(
       CERT_STORE_PROV_SYSTEM,
       0,
       NULL,
       cert_store_provider,
       w_cert_store_name.c_str())))
    {
        HandleError(TEXT("The MY store could not be opened."));
        return;
    }

    // Get a pointer to the signer's certificate.
    // This certificate must have access to the signer's private key.
    if(pSignerCert = CertFindCertificateInStore(
       hCertStore,
       MY_ENCODING_TYPE,
       0,
       CERT_FIND_SUBJECT_STR,
       w_cert_subject.c_str(),
       NULL))
    {
       _tprintf(TEXT("The signer's certificate was found.\n"));
    }
    else
    {
        HandleError( TEXT("Signer certificate not found."));
        return;
    }
}

void WinCertStoreKey::GetPrivateKey() {
    if(!(CryptAcquireCertificatePrivateKey(
        pSignerCert,
        CRYPT_ACQUIRE_ONLY_NCRYPT_KEY_FLAG,
        NULL,
        &hCryptProv,
        &dwKeySpec,
        NULL)))
    {
        HandleError(TEXT("CryptAcquireCertificatePrivateKey.\n"));
    } else {
        LogInfo("Get private key\n");
        LogInfo("key spec is %lu\n", dwKeySpec);
        if (dwKeySpec == CERT_NCRYPT_KEY_SPEC) LogInfo("key spec is ncrypt key\n");
    }
}

void WinCertStoreKey::CreateHash(PBYTE pbToSign, DWORD cbToSign) {
    NTSTATUS                status          = STATUS_UNSUCCESSFUL;
    DWORD                   cbData          = 0,
                            cbHashObject    = 0;

    //open an algorithm handle
    if(!NT_SUCCESS(status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_SHA256_ALGORITHM, NULL, 0)))
    {
        wprintf(L"**** Error 0x%x returned by BCryptOpenAlgorithmProvider\n", status);
        goto Cleanup;
    }

    //calculate the size of the buffer to hold the hash object
    if(!NT_SUCCESS(status = BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0)))
    {
        wprintf(L"**** Error 0x%x returned by BCryptGetProperty\n", status);
        goto Cleanup;
    }

    //allocate the hash object on the heap
    pbHashObject = (PBYTE)HeapAlloc (GetProcessHeap(), 0, cbHashObject);
    if(NULL == pbHashObject)
    {
        wprintf(L"**** memory allocation failed\n");
        goto Cleanup;
    }

   //calculate the length of the hash
    if(!NT_SUCCESS(status = BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0)))
    {
        wprintf(L"**** Error 0x%x returned by BCryptGetProperty\n", status);
        goto Cleanup;
    }

    //allocate the hash buffer on the heap
    pbHash = (PBYTE)HeapAlloc (GetProcessHeap(), 0, cbHash);
    if(NULL == pbHash)
    {
        wprintf(L"**** memory allocation failed\n");
        goto Cleanup;
    }

    //create a hash
    if(!NT_SUCCESS(status = BCryptCreateHash(hAlg, &hHash, pbHashObject, cbHashObject, NULL, 0, 0)))
    {
        wprintf(L"**** Error 0x%x returned by BCryptCreateHash\n", status);
        goto Cleanup;
    }
    

    //hash some data
    if(!NT_SUCCESS(status = BCryptHashData(hHash, pbToSign, cbToSign, 0)))
    {
        wprintf(L"**** Error 0x%x returned by BCryptHashData\n", status);
        goto Cleanup;
    }
    
    //close the hash
    if(!NT_SUCCESS(status = BCryptFinishHash(hHash, pbHash, cbHash, 0)))
    {
        wprintf(L"**** Error 0x%x returned by BCryptFinishHash\n", status);
        goto Cleanup;
    }

    if (EnableLogging) printf("Hash size is: %lu\n", cbHash);

Cleanup:
    return;
}

void WinCertStoreKey::NCryptSign(PBYTE pbSignatureOut, PDWORD cbSignatureOut) {
    // create padding info
    BCRYPT_PSS_PADDING_INFO pss_padding_info = {};
    pss_padding_info.pszAlgId = BCRYPT_SHA256_ALGORITHM;
    pss_padding_info.cbSalt = 32; // 32 bytes for sha256
    void* padding_info = nullptr;
    DWORD dwFlag = 0;
    LogInfo(is_rsa_type? "key is rsa\n": "key is ec\n");
    if (is_rsa_type) {
        padding_info = &pss_padding_info;
        dwFlag = BCRYPT_PAD_PSS;
    }
    
    //sign the hash
    DWORD cbSignature = 0;
    SECURITY_STATUS secStatus = ERROR_SUCCESS;
    if(FAILED(secStatus = NCryptSignHash(hCryptProv,padding_info,pbHash,cbHash,NULL, 0,&cbSignature,dwFlag)))
    {
        wprintf(L"**** Error 0x%x returned by NCryptSignHash\n", secStatus);
        goto Cleanup;
    } else {
        if (EnableLogging) printf("First call to NCryptSignHash succeeded, sig len %lu\n", cbSignature);
    }

    //allocate the signature buffer
    PBYTE pbSignature = NULL;
    pbSignature = (PBYTE)HeapAlloc(GetProcessHeap (), 0, cbSignature);
    if(NULL == pbSignature)
    {
        wprintf(L"**** memory allocation failed\n");
        goto Cleanup;
    }

    if(FAILED(secStatus = NCryptSignHash(hCryptProv,padding_info,pbHash,cbHash,pbSignature,cbSignature,&cbSignature,dwFlag)))
    {
        wprintf(L"**** Error 0x%x returned by NCryptSignHash\n", secStatus);
        goto Cleanup;
    } else {
        if (EnableLogging) {
            printf("Sign succeeded!\n");
            printf("Signature length is: %lu\n", cbSignature);
        }
        std::cout << "Signature is: " << pbSignature << std::endl;
        if (!is_rsa_type) {
            // Convert the RAW ECDSA signature to a DER-encoded ECDSA-Sig-Value.
            LogInfo("converting ECDSA signature\n");
            size_t order_len = cbSignature / 2;
            ECDSA_SIG_st *sig = (ECDSA_SIG_st*)HeapAlloc (GetProcessHeap(), 0, sizeof(ECDSA_SIG_st));
            sig->r = BN_bin2bn(pbSignature, order_len, NULL);
            sig->s = BN_bin2bn(pbSignature + order_len, order_len, NULL);
            if (EnableLogging) std::cout << "sig->r " << sig->r <<std::endl;
            
            if (!sig || !sig->r || !sig->s) {
                LogInfo("calling BN_bin2bn failed\n");
                goto Cleanup;
            }
            if (EnableLogging) {
                std::cout << "r: " << sig->r << std::endl;
                std::cout << "s: " << sig->s << std::endl;
            }
            char * number_str = BN_bn2hex(sig->r);
            int len = i2d_ECDSA_SIG(sig, nullptr);
            if (len <= 0) {
                printf("first call to i2d_ECDSA_SIG failed\n");
                goto Cleanup;
            }
            if (EnableLogging) printf("first call to i2d_ECDSA_SIG returns len %d\n", len);
            PBYTE pbSignatureNew = (PBYTE)HeapAlloc (GetProcessHeap(), 0, len);
            PBYTE pbSig = pbSignatureNew;
            if (EnableLogging) {
                printf("pbSignatureNew is %p\n", pbSignatureNew);
                printf("pbSig is %p\n", pbSig);
                printf("second call to i2d_ECDSA_SIG\n");
            }
            len = i2d_ECDSA_SIG(sig, &pbSig);
            if (len <= 0) {
                HeapFree(GetProcessHeap(), 0, pbSignatureNew);
                printf("second call to i2d_ECDSA_SIG failed\n");
                goto Cleanup;
            }
            if (EnableLogging) {
                printf("conversion is done, sig size is: %d\n", len);
                printf("pbSignatureNew is %p\n", pbSignatureNew);
                printf("pbSignatureNew + len is %p\n", pbSignatureNew + len);
                printf("pbSig is %p\n", pbSig);
            }
            pbSignature = pbSignatureNew;
            cbSignature = len;
        }
        if (pbSignatureOut && cbSignatureOut) {
            //int sig_size = (int)cbSignature;
            for (int i = 0; i < cbSignature; i++) {
                pbSignatureOut[i] = pbSignature[i];
            }
            *cbSignatureOut = cbSignature;
        }
    }

    Cleanup:
        return;
}

bool WinCertStoreKey::Sign(unsigned char *sig, size_t *sig_len, const unsigned char *tbs, size_t tbs_len) {
    GetSignerCert();
    GetPrivateKey();
    CreateHash((PBYTE) tbs, tbs_len);
    DWORD len;
    NCryptSign(sig, &len);
    *sig_len = len;
    return 1;
}

extern "C" {

__declspec(dllexport) WinCertStoreKey* CreateCustomKey(bool is_rsa_type, bool local_machine_store, const char *store_name, const char *subject) {
  // creating custom key
  char * val = getenv("GOOGLE_AUTH_TLS_OFFLOAD_LOGGING");
  EnableLogging = (val == nullptr)? false : true;
  if (EnableLogging) {
    printf("In CreateCustomKey\n");
    std::cout << "is_rsa_type: " << is_rsa_type << std::endl;
    std::cout << "local_machine_store: " << local_machine_store << std::endl;
    std::cout << "store_name: " << store_name << std::endl;
    std::cout << "subject: " << subject << std::endl;
  }
  WinCertStoreKey *key = new WinCertStoreKey(is_rsa_type, local_machine_store, store_name, subject);
  return key;
}

__declspec(dllexport) void DestroyCustomKey(WinCertStoreKey *key) {
  // deleting custom key
  if (EnableLogging) printf("In DestroyCustomKey\n");
  delete key;
}

__declspec(dllexport) bool Sign(WinCertStoreKey *key, unsigned char *sig, size_t *sig_len, const unsigned char *tbs, size_t tbs_len) {
  return key->Sign(sig, sig_len, tbs, tbs_len);
}

}

static PyMethodDef Methods[] = {
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef module = {PyModuleDef_HEAD_INIT, "windows signer", NULL, -1, Methods};

PyMODINIT_FUNC PyInit_windows_signer_ext(void) {
    return PyModule_Create(&module);
}