/*
 * cxloop: asocia un dispositivo loop a un archivo o partición a partir de un
 * desplazamiento. El busybox del updater trae mount pero no losetup, y su
 * «mount -o loop» siempre empieza en el byte 0; la partición de PlayMemories
 * (nflasha12) tiene su FAT16 a partir del sector 3, igual que la monta el init
 * de la cámara en modo normal (/log, por /dev/loop5).
 *
 *   cxloop attach /dev/loop1 /dev/nflasha12 1536 ro|rw
 *   cxloop status /dev/loop1
 *   cxloop detach /dev/loop1
 *
 * Sale con 0 si todo fue bien. No monta nada ni escribe en el archivo: eso lo
 * hace después el mount del busybox sobre el loop. Sin libc; se compila con
 * tools/build-cxloop.sh y corre desde /ramdsk (memoria) en modo updater.
 */
typedef unsigned long long u64;
typedef unsigned int u32;
typedef unsigned char u8;

struct loop_info64 {
    u64 lo_device, lo_inode, lo_rdevice, lo_offset, lo_sizelimit;
    u32 lo_number, lo_encrypt_type, lo_encrypt_key_size, lo_flags;
    u8 lo_file_name[64], lo_crypt_name[64], lo_encrypt_key[32];
    u64 lo_init[2];
};

#define LOOP_SET_FD       0x4C00
#define LOOP_CLR_FD       0x4C01
#define LOOP_SET_STATUS64 0x4C04
#define LOOP_GET_STATUS64 0x4C05

static long sys(long n, long a, long b, long c)
{
    register long r7 __asm__("r7") = n;
    register long r0 __asm__("r0") = a;
    register long r1 __asm__("r1") = b;
    register long r2 __asm__("r2") = c;
    __asm__ volatile("svc 0" : "+r"(r0) : "r"(r7), "r"(r1), "r"(r2) : "memory");
    return r0;
}

static u32 len(const char *s) { u32 n = 0; while (s[n]) n++; return n; }
static void say(const char *s) { sys(4, 1, (long)s, len(s)); }
static void num(u32 v)
{
    char b[12];
    int i = 11;
    b[i] = 0;
    do {
        u32 q = 0, r = v;                 /* v / 10 sin libgcc */
        while (r >= 10) { u32 k = 10, m = 1; while (r >= k * 2 && k < 0x40000000u) { k *= 2; m *= 2; } r -= k; q += m; }
        b[--i] = (char)('0' + r);
        v = q;
    } while (v);
    say(b + i);
}
static int is(const char *a, const char *b) { while (*a && *a == *b) { a++; b++; } return *a == *b; }

static int fail(const char *step, long err)
{
    say("error ");
    say(step);
    say(" ");
    num((u32)-err);
    say("\n");
    return 1;
}

int main_c(int argc, char **argv)
{
    struct loop_info64 info;
    long lfd, ffd, r;
    u32 i, off = 0;
    int rw;
    if (argc < 3) { say("uso: cxloop attach LOOP ARCHIVO DESPLAZAMIENTO ro|rw | status LOOP | detach LOOP\n"); return 2; }
    for (i = 0; i < sizeof(info); i++) ((u8 *)&info)[i] = 0;
    if (is(argv[1], "detach")) {
        lfd = sys(5, (long)argv[2], 0, 0);
        if (lfd < 0) return fail("abrir el loop", lfd);
        r = sys(54, lfd, LOOP_CLR_FD, 0);
        if (r < 0) return fail("soltar", r);
        say("ok suelto\n");
        return 0;
    }
    if (is(argv[1], "status")) {
        lfd = sys(5, (long)argv[2], 0, 0);
        if (lfd < 0) return fail("abrir el loop", lfd);
        r = sys(54, lfd, LOOP_GET_STATUS64, (long)&info);
        if (r < 0) return fail("estado", r);
        say("ok desplazamiento ");
        num((u32)info.lo_offset);
        say(" solo lectura ");
        num(info.lo_flags & 1);
        say("\n");
        return 0;
    }
    if (!is(argv[1], "attach") || argc < 6) { say("uso: cxloop attach LOOP ARCHIVO DESPLAZAMIENTO ro|rw\n"); return 2; }
    for (i = 0; argv[4][i]; i++) {
        if (argv[4][i] < '0' || argv[4][i] > '9') { say("desplazamiento malo\n"); return 2; }
        off = off * 10 + (u32)(argv[4][i] - '0');
    }
    rw = is(argv[5], "rw");
    if (!rw && !is(argv[5], "ro")) { say("ro o rw\n"); return 2; }
    lfd = sys(5, (long)argv[2], rw ? 2 : 0, 0);
    if (lfd < 0) return fail("abrir el loop", lfd);
    ffd = sys(5, (long)argv[3], rw ? 2 : 0, 0);
    if (ffd < 0) return fail("abrir el archivo", ffd);
    r = sys(54, lfd, LOOP_SET_FD, ffd);
    if (r < 0) return fail("asociar", r);
    info.lo_offset = off;
    r = sys(54, lfd, LOOP_SET_STATUS64, (long)&info);
    if (r < 0) {
        sys(54, lfd, LOOP_CLR_FD, 0);
        return fail("desplazamiento", r);
    }
    for (i = 0; i < sizeof(info); i++) ((u8 *)&info)[i] = 0;
    r = sys(54, lfd, LOOP_GET_STATUS64, (long)&info);
    if (r < 0 || (u32)info.lo_offset != off) {
        sys(54, lfd, LOOP_CLR_FD, 0);
        return fail("comprobar", r < 0 ? r : -1);
    }
    say("ok asociado, desplazamiento ");
    num(off);
    say(rw ? " rw\n" : " ro\n");
    return 0;
}

__asm__(".text\n.arm\n.global _start\n_start:\n"
        "    ldr r0, [sp]\n    add r1, sp, #4\n    bl main_c\n    mov r7, #1\n    svc 0\n");
