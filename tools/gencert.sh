#!/usr/bin/env bash
set -e

OUTDIR="${1:-certs}"
DAYS=825
BITS=4096
CURVE=prime256v1

mkdir -p "$OUTDIR"
cd "$OUTDIR"

echo "[*] Generating CA..."
openssl ecparam -genkey -name $CURVE -noout -out ca.key
openssl req -new -x509 -days $DAYS -key ca.key -out ca.crt \
    -subj "/C=US/O=Internal/CN=C2-CA"

echo "[*] Generating server cert..."
openssl ecparam -genkey -name $CURVE -noout -out server.key
openssl req -new -key server.key -out server.csr \
    -subj "/C=US/O=Internal/CN=c2server"
cat > server_ext.cnf << 'EOF'
[v3_req]
subjectAltName = @alt_names
keyUsage = digitalSignature, keyEncipherment
extendedKeyUsage = serverAuth
[alt_names]
DNS.1 = localhost
IP.1  = 127.0.0.1
EOF
openssl x509 -req -days $DAYS -in server.csr -CA ca.crt -CAkey ca.key \
    -CAcreateserial -out server.crt -extensions v3_req -extfile server_ext.cnf
rm -f server.csr server_ext.cnf

echo "[*] Generating agent cert (for mTLS)..."
openssl ecparam -genkey -name $CURVE -noout -out agent.key
openssl req -new -key agent.key -out agent.csr \
    -subj "/C=US/O=Internal/CN=c2agent"
openssl x509 -req -days $DAYS -in agent.csr -CA ca.crt -CAkey ca.key \
    -CAcreateserial -out agent.crt \
    -extensions v3_req -extfile <(printf '[v3_req]\nextendedKeyUsage=clientAuth\n')
rm -f agent.csr

echo "[*] Verifying chain..."
openssl verify -CAfile ca.crt server.crt
openssl verify -CAfile ca.crt agent.crt

chmod 600 *.key

echo ""
echo "[+] Certs written to: $(pwd)"
echo "    ca.crt      — CA certificate"
echo "    server.crt  — server certificate"
echo "    server.key  — server private key"
echo "    agent.crt   — agent certificate (mTLS)"
echo "    agent.key   — agent private key (mTLS)"
echo ""
echo "Usage (server):"
echo "  ./c2server -s 0.0.0.0 -p 4444 -m tls"
echo "  ./c2server -s 0.0.0.0 -p 4444 -m https"
echo "  ./c2server -s 0.0.0.0 -p 4444 -m mtls"
echo ""
echo "Usage (agent mTLS):"
echo "  ./agent -s <host> -p 4444 -m mtls --cert certs/agent.crt --key certs/agent.key --ca certs/ca.crt"
