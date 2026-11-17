#include <cstdint>
#include <cstdio>
#include <fstream>

#include "sha3.h"

uint64_t rotl64(uint64_t x, int n)
{
    if (n == 0)
        return x;
    uint64_t left = x << n;
    uint64_t right = x >> (64 - n);
    uint64_t res = left | right;
    return res;
}

void theta(uint64_t A[5][5])
{
    uint64_t C[5];
    uint64_t D[5];
    for (int x = 0; x < 5; x++)
    {
        uint64_t parite = 0;
        for (int y = 0; y < 5; y++)
            parite ^= A[x][y];
        C[x] = parite;
    }
    for (int x = 0; x < 5; x++)
    {
        uint64_t prec_col = C[(x + 4) % 5];
        uint64_t suiv_col = C[(x + 1) % 5];
        D[x] = prec_col ^ rotl64(suiv_col, 1);
    }
    for (int x = 0; x < 5; x++)
    {
        for (int y = 0; y < 5; y++)
        {
            A[x][y] = A[x][y] ^ D[x];
        }
    }
}

void rho_pi(uint64_t A[5][5])
{
    uint64_t B[5][5];
    for (int x = 0; x < 5; x++)
    {
        for (int y = 0; y < 5; y++)
        {
            int nouveau_x = y;
            int nouveau_y = (2 * x + 3 * y) % 5;
            B[nouveau_x][nouveau_y] = rotl64(A[x][y], R[x][y]);
        }
    }
    for (int x = 0; x < 5; x++)
    {
        for (int y = 0; y < 5; y++)
            A[x][y] = B[x][y];
    }
}

void chi(uint64_t A[5][5])
{
    uint64_t B[5][5];
    for (int x = 0; x < 5; x++)
    {
        for (int y = 0; y < 5; y++)
            B[x][y] = A[x][y];
    }
    for (int x = 0; x < 5; x++)
    {
        for (int y = 0; y < 5; y++)
        {
            uint64_t c = B[x][y];
            uint64_t s = B[(x + 1) % 5][y];
            uint64_t ns = B[(x + 2) % 5][y];
            A[x][y] = c ^ (~s & ns);
        }
    }
}

void iota(uint64_t A[5][5], int ronde)
{
    A[0][0] = A[0][0] ^ RC[ronde];
}

void keccak_f(uint64_t A[5][5])
{
    for (int ronde = 0; ronde < 24; ronde++)
    {
        theta(A);
        rho_pi(A);
        chi(A);
        iota(A, ronde);
    }
}

uint64_t octets_vers_uint64(const unsigned char *octets)
{
    uint64_t resultat = 0;
    for (int i = 0; i < 8; i++)
    {
        uint64_t octet = octets[i];
        int dec = 8 * i;
        resultat |= octet << dec;
    }
    return resultat;
}

void absorber(uint64_t A[5][5], const unsigned char bloc[RATE])
{
    for (int y = 0; y < 5; y++)
    {
        for (int x = 0; x < 5; x++)
        {
            int tmp = x + 5 * y;
            int debut = 8 * tmp;
            if (debut < RATE)
                A[x][y] = A[x][y] ^ octets_vers_uint64(bloc + debut);
        }
    }
    keccak_f(A);
}

bool sha3_256_fichier(const char *nom, unsigned char hash[TAILLE_HASH])
{
    std::ifstream fichier(nom, std::ios::binary);
    if (!fichier)
    {
        return false;
    }
    uint64_t A[5][5] = {};
    unsigned char bloc[RATE];
    int n;
    while (true)
    {
        fichier.read((char *)bloc, RATE);
        n = fichier.gcount();
        if (n < RATE)
        {
            break;
        }
        absorber(A, bloc);
    }
    if (fichier.bad())
        return false;

    for (int i = n; i < RATE; i++)
        bloc[i] = 0;
    bloc[n] = 0x06;
    uint8_t PAD_END = 0x80;
    int lastByte = RATE - 1;
    bloc[lastByte] |= PAD_END;
    absorber(A, bloc);
    for (int x = 0; x < 4; x++)
    {
        for (int i = 0; i < 8; i++)
        {
            uint64_t lane = A[x][0];
            int shift = 8 * i;
            int pos = 8 * x + i;
            hash[pos] = (lane >> shift) & 0xFF;
        }
    }
    return true;
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("mettre un fichier en parametre\n");
        return 1;
    }
    unsigned char hash[TAILLE_HASH];
    if (!sha3_256_fichier(argv[1], hash))
    {
        printf("impossible de lire le fichier\n");
        return 1;
    }
    for (int i = 0; i < TAILLE_HASH; i++)
        printf("%02x", hash[i]);
    printf("\n");
    return 0;
}
