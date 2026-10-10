#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HD_ZONE 16
#define HD_LO_BYTE 0xAA
#define HD_HI_BYTE 0xBB
#define HD_MAGIC 0xC0FFEE11BEEFCAFEULL
#define HD_TABLE_CAP (1u << 21)
#define HD_TABLE_MASK (HD_TABLE_CAP - 1u)
#define HD_TOMBSTONE ((void *)(uintptr_t)1)

typedef struct {
    uint64_t magic;
    size_t size;
    size_t seq;
    void *ra;
    const char *tag;
} hd_hdr;

typedef struct {
    void *user;
    void *base;
    size_t size;
    size_t seq;
    void *ra;
    const char *tag;
} hd_blk;

static hd_blk hd_tab[HD_TABLE_CAP];
static size_t hd_seq;
static int hd_ready;
static int hd_reporting;
static void *(*hd_real_malloc)(size_t);
static void (*hd_real_free)(void *);
static void *(*hd_real_realloc)(void *, size_t);

const char *hd_cur_suite;

static void hd_die(const char *op, void *user, size_t size, size_t seq, void *ra,
                   const char *alloc_suite, const char *zone, size_t off, unsigned got,
                   unsigned want) {
    HMODULE img = GetModuleHandleW(NULL);
    fprintf(stderr,
            "HEAPGUARD op=%s suite=%s user=0x%" PRIxPTR " size=%zu seq=%zu alloc_suite=%s "
            "alloc_rva=0x%" PRIxPTR " zone=%s off=%zu got=0x%02x want=0x%02x\n",
            op, hd_cur_suite ? hd_cur_suite : "(pre-main)", (uintptr_t)user, size, seq,
            alloc_suite ? alloc_suite : "?", (uintptr_t)ra - (uintptr_t)img, zone, off, got, want);
    fflush(stderr);
    abort();
}

static void hd_load_real(void) {
    if (hd_ready) {
        return;
    }
    HMODULE m = GetModuleHandleW(L"ucrtbase.dll");
    if (m == NULL) {
        m = GetModuleHandleW(L"msvcrt.dll");
    }
    if (m != NULL) {
        hd_real_malloc = (void *(*)(size_t))(void *)GetProcAddress(m, "malloc");
        hd_real_free = (void (*)(void *))(void *)GetProcAddress(m, "free");
        hd_real_realloc = (void *(*)(void *, size_t))(void *)GetProcAddress(m, "realloc");
    }
    hd_ready = 1;
}

static size_t hd_hash(void *p) {
    uint64_t x = (uint64_t)(uintptr_t)p >> 4;
    x *= 11400714819323198485ULL;
    return (size_t)(x >> 24) & HD_TABLE_MASK;
}

static void hd_insert(void *user, void *base, size_t size, void *ra, const char *tag) {
    size_t i = hd_hash(user);
    while (hd_tab[i].user != NULL && hd_tab[i].user != HD_TOMBSTONE) {
        i = (i + 1u) & HD_TABLE_MASK;
    }
    hd_tab[i].user = user;
    hd_tab[i].base = base;
    hd_tab[i].size = size;
    hd_tab[i].seq = ++hd_seq;
    hd_tab[i].ra = ra;
    hd_tab[i].tag = tag;
}

static hd_blk *hd_find(void *user) {
    size_t i = hd_hash(user);
    while (hd_tab[i].user != NULL) {
        if (hd_tab[i].user == user) {
            return &hd_tab[i];
        }
        i = (i + 1u) & HD_TABLE_MASK;
    }
    return NULL;
}

static void hd_erase(hd_blk *b) {
    b->user = HD_TOMBSTONE;
    b->base = NULL;
    b->size = 0;
    b->seq = 0;
    b->ra = NULL;
    b->tag = NULL;
}

static void hd_check(const char *op, hd_blk *b) {
    hd_hdr *h = (hd_hdr *)b->base;
    if (h->magic != HD_MAGIC || h->size != b->size || h->seq != b->seq) {
        hd_die(op, b->user, b->size, b->seq, b->ra, b->tag, "hdr", 0,
               (unsigned)(h->magic & 0xffu), (unsigned)(HD_MAGIC & 0xffu));
    }
    unsigned char *lo = (unsigned char *)b->base + sizeof(hd_hdr);
    for (size_t i = 0; i < HD_ZONE; i++) {
        if (lo[i] != HD_LO_BYTE) {
            hd_die(op, b->user, b->size, b->seq, b->ra, b->tag, "lo", i, lo[i], HD_LO_BYTE);
        }
    }
    unsigned char *hi = (unsigned char *)b->user + b->size;
    for (size_t i = 0; i < HD_ZONE; i++) {
        if (hi[i] != HD_HI_BYTE) {
            hd_die(op, b->user, b->size, b->seq, b->ra, b->tag, "hi", i, hi[i], HD_HI_BYTE);
        }
    }
}

void *malloc(size_t size) {
    hd_load_real();
    if (hd_real_malloc == NULL) {
        return NULL;
    }
    if (size == 0) {
        size = 1;
    }
    size_t pre = (sizeof(hd_hdr) + HD_ZONE + 15u) & ~(size_t)15u;
    size_t total = pre + size + HD_ZONE;
    void *base = hd_real_malloc(total);
    if (base == NULL) {
        return NULL;
    }
    hd_hdr *h = (hd_hdr *)base;
    h->magic = HD_MAGIC;
    h->size = size;
    h->seq = 0;
    h->ra = __builtin_return_address(0);
    h->tag = hd_cur_suite;
    unsigned char *p = (unsigned char *)base;
    memset(p + sizeof(hd_hdr), HD_LO_BYTE, pre - sizeof(hd_hdr));
    memset(p + pre, 0, size);
    memset(p + pre + size, HD_HI_BYTE, HD_ZONE);
    void *user = p + pre;
    hd_insert(user, base, size, h->ra, h->tag);
    hd_blk *slot = hd_find(user);
    h->seq = slot ? slot->seq : 0;
    return user;
}

static int hd_ptr_in_some_heap(void *p) {
    MEMORY_BASIC_INFORMATION mbi;
    if (VirtualQuery(p, &mbi, sizeof mbi) == 0 || mbi.State != MEM_COMMIT ||
        (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) != 0) {
        return 0;
    }
    HANDLE heaps[64];
    DWORD n = GetProcessHeaps(64, heaps);
    for (DWORD i = 0; i < n; i++) {
        if (HeapValidate(heaps[i], 0, p)) {
            return 1;
        }
    }
    return 0;
}

void free(void *p) {
    hd_load_real();
    if (p == NULL) {
        return;
    }
    hd_blk *b = hd_find(p);
    if (b == NULL) {
        HMODULE img = GetModuleHandleW(NULL);
        fprintf(stderr, "HDDBG free-unknown suite=%s ptr=0x%" PRIxPTR " caller_rva=0x%" PRIxPTR
                        " in_heap=%d\n",
                hd_cur_suite ? hd_cur_suite : "(pre-main)", (uintptr_t)p,
                (uintptr_t)__builtin_return_address(0) - (uintptr_t)img, hd_ptr_in_some_heap(p));
        fflush(stderr);
        if (!hd_ptr_in_some_heap(p)) {
            hd_die("free-invalid", p, 0, 0, __builtin_return_address(0), hd_cur_suite, "wild", 0, 0,
                   0);
        }
        hd_real_free(p);
        return;
    }
    hd_check("free", b);
    void *base = b->base;
    hd_erase(b);
    hd_real_free(base);
}

void *calloc(size_t n, size_t sz) {
    if (n != 0 && sz > (size_t)-1 / n) {
        return NULL;
    }
    return malloc(n * sz);
}

void *realloc(void *p, size_t size) {
    hd_load_real();
    if (p == NULL) {
        return malloc(size);
    }
    hd_blk *b = hd_find(p);
    if (b == NULL) {
        return hd_real_realloc(p, size);
    }
    hd_check("realloc", b);
    if (size == 0) {
        size = 1;
    }
    void *nu = malloc(size);
    if (nu == NULL) {
        return NULL;
    }
    size_t copy = b->size < size ? b->size : size;
    memcpy(nu, p, copy);
    free(p);
    return nu;
}

void hd_heap_audit(void) {
    if (hd_reporting) {
        return;
    }
    for (size_t i = 0; i < HD_TABLE_CAP; i++) {
        if (hd_tab[i].user != NULL && hd_tab[i].user != HD_TOMBSTONE) {
            hd_check("audit", &hd_tab[i]);
        }
    }
}
