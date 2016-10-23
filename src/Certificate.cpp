/*
 *  Copyright 2016 Bjango Pty Ltd. All rights reserved.
 *  Copyright 2010 William Tisäter. All rights reserved.
 * 
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *    1.  Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *    2.  Redistributions in binary form must reproduce the above copyright
 *        notice, this list of conditions and the following disclaimer in the
 *        documentation and/or other materials provided with the distribution.
 *
 *    3.  The name of the copyright holder may not be used to endorse or promote
 *        products derived from this software without specific prior written
 *        permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER ``AS IS'' AND ANY
 *  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *  DISCLAIMED. IN NO EVENT SHALL WILLIAM TISÄTER BE LIABLE FOR ANY
 *  DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *  ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 *  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "Certificate.h"
#include <cstdio>
#include <iostream>

using namespace std;

/* Generates a 2048-bit RSA key. */
EVP_PKEY * generate_key()
{
    /* Allocate memory for the EVP_PKEY structure. */
    EVP_PKEY * pkey = EVP_PKEY_new();
    if(!pkey)
    {
        std::cerr << "Unable to create EVP_PKEY structure." << std::endl;
        return NULL;
    }
    
    /* Generate the RSA key and assign it to pkey. */
    RSA * rsa = RSA_generate_key(2048, RSA_F4, NULL, NULL);
    if(!EVP_PKEY_assign_RSA(pkey, rsa))
    {
        std::cerr << "Unable to generate 2048-bit RSA key." << std::endl;
        EVP_PKEY_free(pkey);
        return NULL;
    }
    
    /* The key has been generated, return it. */
    return pkey;
}

/* Generates a self-signed x509 certificate. */
X509 * generate_x509(EVP_PKEY * pkey)
{
    /* Allocate memory for the X509 structure. */
    X509 * x509 = X509_new();
    if(!x509)
    {
        std::cerr << "Unable to create X509 structure." << std::endl;
        return NULL;
    }
    
    /* Set the serial number. */
    ASN1_INTEGER_set(X509_get_serialNumber(x509), 1);
    
    /* This certificate is valid from now until exactly one year from now. */
    X509_gmtime_adj(X509_get_notBefore(x509), 0);
    X509_gmtime_adj(X509_get_notAfter(x509), 31536000L);
    
    /* Set the public key for our certificate. */
    X509_set_pubkey(x509, pkey);
    
    /* We want to copy the subject name to the issuer name. */
    X509_NAME * name = X509_get_subject_name(x509);
    
    /* Set the country code and common name. */
    X509_NAME_add_entry_by_txt(name, "C",  MBSTRING_ASC, (unsigned char *)"CA",        -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "O",  MBSTRING_ASC, (unsigned char *)"istatd", -1, -1, 0);
    X509_NAME_add_entry_by_txt(name, "CN", MBSTRING_ASC, (unsigned char *)"localhost", -1, -1, 0);
    
    /* Now set the issuer name. */
    X509_set_issuer_name(x509, name);
    
    /* Actually sign the certificate with our key. */
    if(!X509_sign(x509, pkey, EVP_sha1()))
    {
        std::cerr << "Error signing certificate." << std::endl;
        X509_free(x509);
        return NULL;
    }
    
    return x509;
}

bool write_to_disk(EVP_PKEY * pkey, X509 * x509)
{
    /* Open the PEM file for writing the key to disk. */
    string privateKeyPath = string(CONFIG_PATH) + "key.pem";
    FILE * pkey_file = fopen(privateKeyPath.c_str(), "wb");
    if(!pkey_file)
    {
        std::cerr << "Unable to open \"key.pem\" for writing." << std::endl;
        return false;
    }
    
    /* Write the key to disk. */
    bool ret = PEM_write_PrivateKey(pkey_file, pkey, NULL, NULL, 0, NULL, NULL);
    fclose(pkey_file);
    
    if(!ret)
    {
        std::cerr << "Unable to write private key to disk." << std::endl;
        return false;
    }
    
    /* Open the PEM file for writing the certificate to disk. */
    string certPath = string(CONFIG_PATH) + "cert.pem";
    FILE * x509_file = fopen(certPath.c_str(), "wb");
    if(!x509_file)
    {
        std::cerr << "Unable to open \"cert.pem\" for writing." << std::endl;
        return false;
    }
    
    /* Write the certificate to disk. */
    ret = PEM_write_X509(x509_file, x509);
    fclose(x509_file);
    
    if(!ret)
    {
        std::cerr << "Unable to write certificate to disk." << std::endl;
        return false;
    }
    
    return true;
}

int createSSLCertificate()
{
    /* Generate the key. */
    std::cout << "Generating RSA key..." << std::endl;
    
    EVP_PKEY * pkey = generate_key();
    if(!pkey)
        return 1;
    
    /* Generate the certificate. */
    std::cout << "Generating x509 certificate..." << std::endl;
    
    X509 * x509 = generate_x509(pkey);
    if(!x509)
    {
        EVP_PKEY_free(pkey);
        return 1;
    }
    
    /* Write the private key and certificate out to disk. */
    std::cout << "Writing key and certificate to disk..." << std::endl;
    
    bool ret = write_to_disk(pkey, x509);
    EVP_PKEY_free(pkey);
    X509_free(x509);
    
    if(ret)
    {
        std::cout << "Success!" << std::endl;
        return 0;
    }

    return 1;
}
