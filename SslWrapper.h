/*
 * SslWrapper.h
 *
 *  Created on: 11 aug 2015
 *      Author: markus
 */

#ifndef SSLWRAPPER_H_
#define SSLWRAPPER_H_

#include <stdint.h>
#include <string>

#include <openssl/ssl.h>

class SslWrapper
{
public:
    SslWrapper(const std::string& host, uint16_t port = 0);    // host may be "host.domain" or "host.domain:port"
    ~SslWrapper();

    int read( uint8_t* buffer, size_t bufferSize );
    int write( uint8_t* buffer, size_t bufferSize );
    void closeConnection();

private:
    SSL_CTX* mSslCtx;
    BIO *mSslBio;
    SSL *mSsl;

    std::string mHostPort;
};



#endif /* SSLWRAPPER_H_ */
