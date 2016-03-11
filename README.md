Hoard Memory Allocator
======================
[![Build Status](https://travis-ci.org/skipor/Hoard-Allocator.svg?branch=master)](https://travis-ci.org/skipor/Hoard-Allocator)

Article "Hoard: A Scalable Memory Allocator for Multithreaded Applications"
http://people.cs.umass.edu/~emery/pubs/berger-asplos2000.pdf

run (no trace):       LD_PRELOAD=libmalloc_intercept.so MALLOC_INTERCEPT_NO_TRACE=1 kreversi
run (trace):    LD_PRELOAD=libmalloc_intercept.so kreversi
