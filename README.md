# Lethe — User-Space Memory Allocator

User-space memory allocator in C with free-list management, boundary-tag coalescing, and thread-local caching.

> A drop-in replacement for `malloc`/`free`/`realloc` in C, implementing free-list management, boundary-tag coalescing, and thread-local caching. Injectable into any existing program via `LD_PRELOAD` without recompilation.

## Benchmark Results
Here is a comparison between Lethe and glibc (default) allocators using `bench/bench.c`:

### Single-threaded malloc/free (ns/op)
| Size | Lethe | glibc |
|---|---|---|
| 16 | 53.6 | 19.4 |
| 32 | 53.5 | 18.3 |
| 64 | 53.8 | 17.6 |
| 128 | 54.2 | 18.0 |
| 256 | 52.8 | 16.8 |
| 512 | 106.3 | 16.3 |
| 1024 | 98.6 | 19.9 |

### Multi-threaded scaling (ops/sec)
| Threads | Lethe | glibc |
|---|---|---|
| 1 | 14,237,787 | 50,421,031 |
| 2 | 2,699,567 | 69,411,727 |
| 4 | 3,683,984 | 124,506,960 |
| 8 | 1,858,439 | 242,895,025 |
| 16 | 1,394,220 | 287,615,289 |

*Note: Lethe shows contention at higher thread counts since allocations >= 512 bypass the thread cache and hit the global mutex, whereas glibc uses per-thread arenas.*

## Design Notes
- **Boundary Tags**: O(1) coalescing by storing block size and `is_free` status in both headers and footers.
- **Thread-Local Caching**: Uses `__thread` variables and `pthread_key_create` destructors for lock-free fast paths on small allocations.
- **In-place Realloc**: `realloc` aggressively tries to absorb free adjacent blocks (forward coalescing) to avoid `memcpy`.

All checklist features completed!
