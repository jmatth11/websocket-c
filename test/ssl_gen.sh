# 1. Generate a 2048-bit RSA private key
openssl genrsa -out server.key 2048

# 2. Create a self-signed certificate valid for 365 days
openssl req -new -x509 -key server.key -out server.crt \
    -days 365 -subj "/C=US/ST=Test/L=Local/O=DevOrg/OU=Dev/CN=localhost"
