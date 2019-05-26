# Prime Number Calculation
*for CS OS course at University of Vienna*

Copyright (C) 2019 Bernhard C. Schrenk

Searches prime numbers using the method of trying to divide by all smaller numbers (less then square-root).
Various ways of multi-threading.

* **simple:** single-threaded (C++17)
* **threaded:** using pthreads, dividing search number field equally to threads (C++17)
* **threadpool:** using pthreads, but assigning work packages dynamically to pool of threads (C++17)
* **threadpool-cpp:** like threadpool, but with C++11 threads (C++17 or MS VS2019)
* **directcompute:** using GPU shader (VS2019 & Windows 10 SDK 10.0.17763.0)
* **rust-vulkan:** using GPU vulkan shader (Rust Language Edition 2018)
