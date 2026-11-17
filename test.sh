#!/bin/sh
# Tests du programme sha3.
# usage : sh test.sh [chemin_du_binaire]     (par defaut ./sha3)
# Ecrit en sh POSIX pour tourner sous OpenBSD (ksh) comme sous Linux.

BIN=${1:-./sha3}
DIR=tests_tmp
ok=0
ko=0

rm -rf $DIR
mkdir $DIR

# verifier fichier hash_attendu description
verifier() {
    obtenu=$($BIN "$1" | awk '{print $1}')
    if [ "$obtenu" = "$2" ]; then
        echo "OK    $3"
        ok=$((ok + 1))
    else
        echo "ECHEC $3"
        echo "      attendu : $2"
        echo "      obtenu  : $obtenu"
        ko=$((ko + 1))
    fi
}

echo "== 1. Vecteurs de test connus =="
printf '' > $DIR/vide
verifier $DIR/vide a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a "message vide"

printf 'abc' > $DIR/abc
verifier $DIR/abc 3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532 "\"abc\""

printf 'abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq' > $DIR/abc448
verifier $DIR/abc448 41c0dba2a9d6240849100376a8235e2c82e1b9998a999e21db32dd97496d3376 "\"abcdbcdecdef...nopq\" (56 octets)"

dd if=/dev/zero bs=1000 count=1000 2>/dev/null | tr '\0' 'a' > $DIR/million_a
verifier $DIR/million_a 5c8875ae474a3634ba4fd55ec85bffd661f32aca75c6d699d0cdcb6c115891c1 "1 000 000 x 'a'"

echo "== 2. Comparaison avec openssl =="
if printf '' | openssl dgst -sha3-256 > /dev/null 2>&1; then
    # Tailles autour de la frontiere des blocs (RATE = 136 octets)
    for n in 1 134 135 136 137 271 272 273 1000 100000; do
        dd if=/dev/urandom of=$DIR/alea_$n bs=$n count=1 2>/dev/null
        ref=$(openssl dgst -sha3-256 < $DIR/alea_$n | awk '{print $NF}')
        verifier $DIR/alea_$n "$ref" "fichier aleatoire de $n octets"
    done

    # Fichier contenant les 256 valeurs d'octets (\0, \n, \r, 0x1a, 0xff...)
    # pour verifier qu'il n'y a pas de probleme d'encodage
    i=0
    while [ $i -lt 256 ]; do
        printf "\\$(printf %o $i)"
        i=$((i + 1))
    done > $DIR/tous_octets
    ref=$(openssl dgst -sha3-256 < $DIR/tous_octets | awk '{print $NF}')
    verifier $DIR/tous_octets "$ref" "les 256 valeurs d'octets"
else
    echo "openssl ne supporte pas sha3-256 ici, tests sautes"
fi

echo "== 3. Gros fichier (50 Mo) =="
dd if=/dev/urandom of=$DIR/gros bs=1048576 count=50 2>/dev/null
debut=$(date +%s)
$BIN $DIR/gros > $DIR/gros.hash
fin=$(date +%s)
echo "temps : $((fin - debut)) s"
if printf '' | openssl dgst -sha3-256 > /dev/null 2>&1; then
    ref=$(openssl dgst -sha3-256 < $DIR/gros | awk '{print $NF}')
    verifier $DIR/gros "$ref" "fichier aleatoire de 50 Mo"
fi

echo "== 4. Cas d'erreur =="
if $BIN $DIR/nexiste_pas > /dev/null 2>&1; then
    echo "ECHEC fichier inexistant : le programme aurait du renvoyer une erreur"
    ko=$((ko + 1))
else
    echo "OK    fichier inexistant -> code de retour non nul"
    ok=$((ok + 1))
fi

rm -rf $DIR
echo
echo "Resultat : $ok OK, $ko ECHEC"
[ $ko -eq 0 ]
