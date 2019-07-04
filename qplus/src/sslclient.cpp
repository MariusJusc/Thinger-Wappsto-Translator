#include "sslclient.hpp"
#include "error.hpp"

#include <sys/socket.h>
#include <resolv.h>
#include <netdb.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>

#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/x509_vfy.h>
#include <fcntl.h>

#include <linux/tcp.h>
#include <cassert> 

const int BUF_SIZE = 8192;       

int verify_callback(int preverify, X509_STORE_CTX* x509_ctx)
{
    /* For error codes, see http://www.openssl.org/docs/apps/verify.html  */
    int depth = X509_STORE_CTX_get_error_depth(x509_ctx);
    int err = X509_STORE_CTX_get_error(x509_ctx);
    
    std::cout << "verify_callback (depth=" << depth << ")(preverify=" << preverify << ")\n";
        
    if(preverify == 0)
    {
        if(err == X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY)
            std::cerr << "  Error = X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY\n";
        else if(err == X509_V_ERR_CERT_UNTRUSTED)
            std::cerr << "  Error = X509_V_ERR_CERT_UNTRUSTED\n";
        else if(err == X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN)
            std::cerr << "  Error = X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN\n";
        else if(err == X509_V_ERR_CERT_NOT_YET_VALID)
            std::cerr << "  Error = X509_V_ERR_CERT_NOT_YET_VALID\n";
        else if(err == X509_V_ERR_CERT_HAS_EXPIRED)
            std::cerr << "  Error = X509_V_ERR_CERT_HAS_EXPIRED\n";
        else if(err == X509_V_OK)
            std::cerr << "  Error = X509_V_OK\n";
        else
            std::cerr << "  Error = " <<  err << "\n";
    }

    return preverify;
}

namespace qplus {
        
    void SSLClient::hostname_and_port(const std::string& url)
    {
        hostname_ = url;
       
        // remove protocol -
        std::string::size_type pos = hostname_.find("://");
        if (pos != std::string::npos)
            hostname_ = hostname_.substr(pos+3);            

        if (hostname_.back() == '/')           
            hostname_.pop_back();
        
        pos = hostname_.find(":");
        std::string portnum = hostname_.substr(pos+1);
        hostname_ = hostname_.substr(0,pos);

        if (portnum.back() == '/')
            portnum.pop_back();

        port_ = std::stoi(portnum);
    }

    /* ---------------------------------------------------------- *
     * create_socket() creates the socket & TCP-connect to server *
     * ---------------------------------------------------------- */
    int SSLClient::create_socket(const std::string& url_str) 
    {
        int sockfd;
        char* tmp_ptr = NULL;
        struct hostent *host;
        struct sockaddr_in dest_addr;

        // Extract hostname and port from url 'https://example.com:12345'
        hostname_and_port(url_str);

        if ( (host = gethostbyname(hostname_.c_str())) == NULL ) {
            std::cerr << "Error: Cannot resolve hostname " <<  hostname_ << "\n";
            return 0;
        }

        // create the basic TCP socket        
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        dest_addr.sin_family=AF_INET;
        dest_addr.sin_port=htons(port_);
        dest_addr.sin_addr.s_addr = *(long*)(host->h_addr);

        /* ---------------------------------------------------------- *
        * Zeroing the rest of the struct                             *
        * ---------------------------------------------------------- */
        memset(&(dest_addr.sin_zero), '\0', 8);
        tmp_ptr = inet_ntoa(dest_addr.sin_addr);

        /* ---------------------------------------------------------- *
        * Try to make the host connect here                          *
        * ---------------------------------------------------------- */
        if ( ::connect(sockfd, (struct sockaddr *) &dest_addr, sizeof(struct sockaddr)) == -1 ) {
            std::cerr << "Error: Cannot connect to host " << hostname_ << "  on port " <<  port_ << "\n";
            return 0;
        }

        return sockfd;
    }
        
    SSLClient::SSLClient() : SSLClient("", "", "") 
    {
    }

    SSLClient::SSLClient(const std::string& cert, const std::string& key) : SSLClient(cert, key, "") 
    {
    }

    SSLClient::SSLClient(const std::string& client_cert, 
                         const std::string& client_key, 
                         const std::string& ca_cert) : client_cert_(client_cert), client_key_(client_key), ca_cert_(ca_cert)
    {
        init();

    }
    
    void SSLClient::init() 
    {
        /* ---------------------------------------------------------- *
        * These function calls initialize openssl for correct work.  *
        * ---------------------------------------------------------- */
        OpenSSL_add_all_algorithms();
        ERR_load_BIO_strings();
        ERR_load_crypto_strings();
        SSL_load_error_strings();

        /* ---------------------------------------------------------- *
        * Create the Input/Output BIO's.                             *
        * ---------------------------------------------------------- */
        certbio = BIO_new(BIO_s_file());
        outbio  = BIO_new_fp(stdout, BIO_NOCLOSE);

        /* ---------------------------------------------------------- *
        * initialize SSL library and register algorithms             *
        * ---------------------------------------------------------- */
        if(SSL_library_init() < 0) {
            throw error_t("Could not initialize the OpenSSL library !");
        }

        method = TLS_client_method();
        
        /* ---------------------------------------------------------- *
        * Try to create a new SSL context                            *
        * ---------------------------------------------------------- */
        if ( (ctx = SSL_CTX_new(method)) == NULL) {
            throw error_t("Unable to create a new SSL context structure.");
        }

        // -------- private key if exist --------------
        if (!client_key_.empty()) {

            privkey = EVP_PKEY_new();
            fp = fopen(client_key_.c_str(), "r");
            if (fp == 0) {
                throw error_t("Error: Could not find file 'client.key'");
            }

            PEM_read_PrivateKey( fp, &privkey, NULL, NULL);  

            if (SSL_CTX_use_PrivateKey(ctx, privkey) != 1) {
                fclose(fp);
                throw error_t("Error: Could not validate private key");
            }

            fclose(fp);
        }

        // ------------------- client certificate -----------
        if (!client_cert_.empty()) {

            fp = fopen(client_cert_.c_str(), "r");
            if (fp == 0) {
                throw error_t("Error: Could not find file 'client.crt'");
            }

            if (! (pcert = PEM_read_X509(fp, NULL, NULL, NULL))) {
                fclose(fp);
                throw error_t("Error: could not read client certificate from file.");
            }
            if (SSL_CTX_use_certificate(ctx, pcert) != 1) {
                fclose(fp);
                throw error_t("Error: Could not find/validate certificate");
            }

            fclose(fp);
        }

        if (!ca_cert_.empty())  {

            SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, verify_callback);
            SSL_CTX_set_verify_depth(ctx, 3);

            const long flags = SSL_OP_ALL | SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION;
            SSL_CTX_set_options(ctx, flags);

            if (!SSL_CTX_load_verify_locations(ctx, ca_cert_.c_str(), NULL)) { 
                throw error_t("Could not load CA cert");
            }
        }

        ssl = SSL_new(ctx);
     } // constructor
     
     SSLClient::~SSLClient() {

        /* ---------------------------------------------------------- *
        * Free the structures we don't need anymore                  *
        * -----------------------------------------------------------*/
        SSL_free(ssl);
        close(fdescriptor_);
        X509_free(cert);
        SSL_CTX_free(ctx);
        BIO_printf(outbio, "Finished SSL/TLS connection with server: %s, port %d\n", hostname_.c_str(), port_);
     }

     void SSLClient::setClientKey(const std::string& key) 
     {
         client_key_ = key; 
     }

     void SSLClient::setCACertificate(const std::string& ca)
     {
        ca_cert_ = ca;
     }

     void SSLClient::setClientCertificate(const std::string& cert)
     {
        client_cert_ = cert;
     }

     bool SSLClient::connect(const std::string& url)
     {
        /* ---------------------------------------------------------- *
        * Make the underlying TCP socket connection                  *
        * ---------------------------------------------------------- */
        fdescriptor_ = create_socket(url);
        if(fdescriptor_ == 0)
            return false;

        std::cout << "Successfully made the TCP connection to: " <<  url << "\n";

        /* ---------------------------------------------------------- *
        * Attach the SSL session to the socket descriptor            *
        * ---------------------------------------------------------- */
        SSL_set_fd(ssl, fdescriptor_);

        int result_long = SSL_get_verify_result(ssl);
        std::cout << "Certificate Check Result: " << result_long << "\n";

        /* ---------------------------------------------------------- *
        e Try to SSL-connect here, returns 1 for success             *
        * ---------------------------------------------------------- */
        if ( SSL_connect(ssl) != 1 ) {
            std::cerr << "Error: Could not build a SSL session to: " << url << "\n";
            return false; 
        }
		
        /*
		 *int rcvbuf, mss;
		 *socklen_t len = sizeof(rcvbuf);
		 *getsockopt(fdescriptor_, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &len);
		 *
		 *len = sizeof(mss);
		 *getsockopt(fdescriptor_, IPPROTO_TCP, TCP_MAXSEG, &mss, &len);
		 *printf("defaults: SO_RCVBUF = %d, MSS = %d\n", rcvbuf, mss);
         */
		
		//assert(false);

        std::cout << "Successfully enabled SSL/TLS session to: " <<  url << "\n";

        /* ---------------------------------------------------------- *
        * Get the remote certificate into the X509 structure         *
        * ---------------------------------------------------------- */

        SSL_accept(ssl);

        cert = SSL_get_peer_certificate(ssl);
        if (cert == NULL) {
            std::cerr << "Error: Could not get a certificate from: " << url << "\n";
            return false;
        }

        std::cout << "Retrieved the server's certificate from: " << url << "\n";

        if (SSL_get_verify_result(ssl) == X509_V_OK) {
            /* The client sent a certificate which verified OK */
            std::cout << "Certificate is valid!! from: " << url << "\n";
        }

        /* ---------------------------------------------------------- *
        * extract various certificate information                    *
        * -----------------------------------------------------------*/
        certname = X509_NAME_new();
        certname = X509_get_subject_name(cert);

        /* ---------------------------------------------------------- *
        * display the cert subject here                              *
        * -----------------------------------------------------------*/
        std::cout << "Displaying the certificate subject data:\n";
        X509_NAME_print_ex(outbio, certname, 0, 0);
        BIO_printf(outbio, "\n");

        return true;
     }

     int SSLClient::descriptor() const
     {
         return fdescriptor_;
     }

    int SSLClient::write(const std::string& data) const
    {
        return SSL_write(ssl, data.c_str(), data.length());
    }

    std::string SSLClient::read() const
    {
        unsigned char data[BUF_SIZE];
        memset(data, 0, BUF_SIZE);

        int n = SSL_read(ssl, data, BUF_SIZE);
        if (n < 0) {
            BIO_printf(outbio, "SSL socket read failed \n");
            return "";
        }
		
		return std::string(data, data + n);		
    }
     
} // namespace qplus


