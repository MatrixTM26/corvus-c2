#!/usr/bin/env bash
set -euo pipefail

CERTS_DIR="$(cd "$(dirname "$0")/.." && pwd)/certs"
DAYS=825
BITS=4096

usage() {
    echo "Usage: $0 [--agents <n>] [--days <days>] [--bits <bits>] [--out <dir>]"
    echo ""
    echo "  --agents <n>   Number of agent cert pairs to generate (default: 1)"
    echo "  --days   <n>   Certificate validity in days              (default: 825)"
    echo "  --bits   <n>   RSA key size                              (default: 4096)"
    echo "  --out    <dir> Output base directory                     (default: <root>/certs)"
    echo ""
    echo "Output layout:"
    echo "  <out>/ca/          ca.key  ca.crt"
    echo "  <out>/server/      server.key  server.crt  ca.crt"
    echo "  <out>/agent-N/     agent.key   agent.crt   ca.crt  (per agent)"
    exit 0
}

AGENTS=1
while [[ $# -gt 0 ]]; do
    case "$1" in
        --agents) AGENTS="$2"; shift 2 ;;
        --days)   DAYS="$2";   shift 2 ;;
        --bits)   BITS="$2";   shift 2 ;;
        --out)    CERTS_DIR="$2"; shift 2 ;;
        --help|-h) usage ;;
        *) echo "[!] Unknown option: $1"; usage ;;
    esac
done

echo "[*] Output directory : $CERTS_DIR"
echo "[*] Validity         : $DAYS days"
echo "[*] Key size         : $BITS bits"
echo "[*] Agent count      : $AGENTS"
echo ""

CA_DIR="$CERTS_DIR/ca"
SERVER_DIR="$CERTS_DIR/server"

mkdir -p "$CA_DIR" "$SERVER_DIR"

echo "[*] Generating CA key and certificate..."
openssl genrsa -out "$CA_DIR/ca.key" "$BITS" 2>/dev/null
openssl req -new -x509 -days "$DAYS" \
    -key "$CA_DIR/ca.key" \
    -out "$CA_DIR/ca.crt" \
    -subj "/CN=C2FrameworkCA/O=RedTeamLab/C=ID" 2>/dev/null
echo "    $CA_DIR/ca.key"
echo "    $CA_DIR/ca.crt"

echo ""
echo "[*] Generating server certificate..."
openssl genrsa -out "$SERVER_DIR/server.key" "$BITS" 2>/dev/null
openssl req -new \
    -key "$SERVER_DIR/server.key" \
    -out "$SERVER_DIR/server.csr" \
    -subj "/CN=C2Server/O=RedTeamLab/C=ID" 2>/dev/null
openssl x509 -req -days "$DAYS" \
    -in  "$SERVER_DIR/server.csr" \
    -CA  "$CA_DIR/ca.crt" \
    -CAkey "$CA_DIR/ca.key" \
    -CAcreateserial \
    -out "$SERVER_DIR/server.crt" 2>/dev/null
cp "$CA_DIR/ca.crt" "$SERVER_DIR/ca.crt"
rm -f "$SERVER_DIR/server.csr"
echo "    $SERVER_DIR/server.key"
echo "    $SERVER_DIR/server.crt"
echo "    $SERVER_DIR/ca.crt"

for I in $(seq 1 "$AGENTS"); do
    AGENT_DIR="$CERTS_DIR/agent-$I"
    mkdir -p "$AGENT_DIR"
    echo ""
    echo "[*] Generating agent-$I certificate..."
    openssl genrsa -out "$AGENT_DIR/agent.key" "$BITS" 2>/dev/null
    openssl req -new \
        -key "$AGENT_DIR/agent.key" \
        -out "$AGENT_DIR/agent.csr" \
        -subj "/CN=C2Agent$I/O=RedTeamLab/C=ID" 2>/dev/null
    openssl x509 -req -days "$DAYS" \
        -in  "$AGENT_DIR/agent.csr" \
        -CA  "$CA_DIR/ca.crt" \
        -CAkey "$CA_DIR/ca.key" \
        -CAcreateserial \
        -out "$AGENT_DIR/agent.crt" 2>/dev/null
    cp "$CA_DIR/ca.crt" "$AGENT_DIR/ca.crt"
    rm -f "$AGENT_DIR/agent.csr"
    echo "    $AGENT_DIR/agent.key"
    echo "    $AGENT_DIR/agent.crt"
    echo "    $AGENT_DIR/ca.crt"
done

echo ""
echo "[+] Done."
echo ""
echo "    Run server (mTLS):"
echo "    ./build/server/c2server -s 0.0.0.0 -p 4444 -m mtls \\"
echo "      --cert $SERVER_DIR/server.crt \\"
echo "      --key  $SERVER_DIR/server.key \\"
echo "      --ca   $SERVER_DIR/ca.crt"
echo ""
for I in $(seq 1 "$AGENTS"); do
    AGENT_DIR="$CERTS_DIR/agent-$I"
    echo "    Run agent-$I (mTLS):"
    echo "    ./build/agent/agent -s <SERVER_IP> -p 4444 -m mtls \\"
    echo "      --cert $AGENT_DIR/agent.crt \\"
    echo "      --key  $AGENT_DIR/agent.key \\"
    echo "      --ca   $AGENT_DIR/ca.crt"
    echo ""
done
