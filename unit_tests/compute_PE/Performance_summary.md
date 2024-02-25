# Compute PEs

We use various compute PEs and evaluate their performance.

## V1: compute everything in a single loop

49.1165 ms, 140 MHz

Cycles = 49.1165 / 1000 * 140 * 1e6 = 6,876,310

```
int query_num = 1000;
int num_fetched_vectors_per_query = 64;
int d = 128;
```

In the best case:
1000 * 64 * (128/16) = 512,000

There is a 10x difference to the theoretical performance!