#!/bin/zsh
# Re-sign a development AYAstorm.app with a stable local identity.
#
#   scripts/bench/sign_dev_app.sh <AYAstorm.app>
#
# A build-tree app is signed ad hoc, so its designated requirement is its
# cdhash and changes with every rebuild. macOS privacy permissions (TCC) are
# bound to that requirement, so every rebuild asks again for access to the
# Desktop folder (a build-tree app reads its skins from the checkout) and
# holds the viewer at startup. Signed with the same certificate, the
# requirement stays the same and a permission granted once keeps applying.
#
# The first run creates a self-signed code signing certificate in a keychain
# of its own, $AYASTORM_DEV_SIGNING_DIR (default
# $OPENSIM_BENCH_DIR/signing, i.e. ~/OpenSimBench/signing). The login
# keychain and trust settings are not changed, and the certificate is not
# trusted by the system: it is only for local development builds, never for
# distribution (see docs/build/building_ayastorm_macos.md for release signing).
#
# codesign finds the private key only in keychains on the user search list.
# Adding this keychain there is a change to the user's keychain settings, so
# the script does not do it; it stops and prints the command to run once.

set -eu
APP=${1:-}
[[ -d $APP ]] || { echo "usage: $0 <AYAstorm.app>" >&2; exit 2; }
APP=${APP:A}
[[ $APP == */build-darwin-* ]] || { echo "only for build-tree apps: $APP" >&2; exit 2; }

DIR=${AYASTORM_DEV_SIGNING_DIR:-${OPENSIM_BENCH_DIR:-$HOME/OpenSimBench}/signing}
NAME="AYAstorm Dev Local Signing"
KC=$DIR/ayastorm-dev-signing.keychain-db
PW_FILE=$DIR/keychain-password

if [[ ! -f $KC ]]; then
  mkdir -p $DIR
  chmod 700 $DIR
  umask 077
  openssl rand -hex 24 > $PW_FILE
  tmp=$(mktemp -d)
  trap 'rm -rf $tmp' EXIT
  cat > $tmp/cert.cnf <<EOF
[req]
distinguished_name = dn
x509_extensions = ext
prompt = no
[dn]
CN = $NAME
[ext]
basicConstraints = critical,CA:false
keyUsage = critical,digitalSignature
extendedKeyUsage = critical,codeSigning
EOF
  openssl req -x509 -newkey rsa:2048 -nodes -days 3650 -config $tmp/cert.cnf \
    -keyout $tmp/key.pem -out $tmp/cert.pem 2>/dev/null
  # -legacy: macOS security(1) cannot read OpenSSL 3's default PKCS#12 ciphers
  p12pw=$(openssl rand -hex 16)
  legacy=()
  openssl pkcs12 -help 2>&1 | grep -q -- -legacy && legacy=(-legacy)
  openssl pkcs12 -export $legacy -inkey $tmp/key.pem -in $tmp/cert.pem \
    -name "$NAME" -passout pass:$p12pw -out $tmp/id.p12
  security create-keychain -p "$(cat $PW_FILE)" $KC
  security set-keychain-settings $KC   # no auto-lock timeout
  security unlock-keychain -p "$(cat $PW_FILE)" $KC
  security import $tmp/id.p12 -k $KC -P $p12pw -T /usr/bin/codesign >/dev/null
  # let codesign use the key without a GUI prompt
  security set-key-partition-list -S apple-tool:,apple: -s -k "$(cat $PW_FILE)" $KC >/dev/null
  echo "created $NAME in $KC"
fi

security unlock-keychain -p "$(cat $PW_FILE)" $KC
if ! security list-keychains -d user | grep -qF "$KC"; then
  echo "add the signing keychain to your keychain search list once, then run this again:" >&2
  echo "  security list-keychains -d user -s $(security list-keychains -d user | sed -E 's/^ *"//; s/"$//' | tr '\n' ' ')$KC" >&2
  exit 1
fi
# by SHA-1: codesign does not look up an untrusted identity by name
id=$(security find-identity -p codesigning $KC | awk -v n="\"$NAME\"" 'index($0, n) {print $2; exit}')
[[ -n $id ]] || { echo "no $NAME identity in $KC" >&2; exit 1; }
codesign --force --deep --keychain $KC --sign $id "$APP"
codesign --verify --deep --strict "$APP"
codesign -dr - "$APP" 2>&1 | sed -n 's/^designated => /designated requirement: /p'
