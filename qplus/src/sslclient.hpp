#pragma once

#include <string>
#include <functional>

#include <openssl/x509.h>
#include <openssl/ssl.h>
#include <ev++.h>

int verify_callback(int preverify, X509_STORE_CTX* x509_ctx);

namespace qplus {

    class SSLClient
    {
        private:
            std::string hostname_;
            int port_;

            BIO              *certbio = NULL;
            BIO               *outbio = NULL;
            X509                *cert = NULL;
            X509_NAME       *certname = NULL;
            const SSL_METHOD *method;
            SSL_CTX *ctx;
            SSL *ssl;
            int fdescriptor_ = 0;
            int ret;
            unsigned long ssl_err = 0;

            EVP_PKEY* privkey;
            X509* pcert;
            FILE* fp;

            std::string client_cert_;
            std::string client_key_;
            std::string ca_cert_;
            
            const char* const PREFERRED_CIPHERS = "HIGH:!aNULL:!kRSA:!SRP:!PSK:!CAMELLIA:!RC4:!MD5:!DSS";

            // private function ------------
            void init();
            void hostname_and_port(const std::string& url);

            /** ---------------------------------------------------------- *
             * create_socket() creates the socket & TCP-connect to server *
             * ---------------------------------------------------------- */
            int create_socket(const std::string& url_str);

        public: 
            
            SSLClient();
            SSLClient(const std::string& cert, const std::string& key); 
            SSLClient(const std::string& client_cert, const std::string& client_key, const std::string& ca_cert);
            ~SSLClient(); 
            
            bool connect(const std::string& url);
            int descriptor() const;

            void setClientKey(const std::string& key);
            void setCACertificate(const std::string& ca);
            void setClientCertificate(const std::string& cert);
            
            int write(const std::string& data) const;
            std::string read() const;
    };

} // namespace qplus
