# Threaded MemCpy - The Fastest MemCpy you have ever seen!
When you see Threaded, you usually think Latency. But in this case, the latency is really low - so low that the benefit you get from this is so good that you never want to let it go again.

# Results

## Intel i5-4690, DDR3-1333 16-16-16-24
Size: 33177600 byte (Width: 3840, Height: 2160, Depth: 4)
Iterations: 100

| Name             | Calls    | Valid    | Average Time | Minimum Time | Maximum Time | Bandwidth            |
|:-----------------|---------:|---------:|-------------:|-------------:|-------------:|---------------------:|
| C                |      100 |      100 |   7710292 ns |   5608893 ns |  10303792 ns |       4303027392 B/s |
| C++              |      100 |      100 |   6793001 ns |   5618548 ns |   7975213 ns |       4884085754 B/s |
| MOVSB            |      100 |      100 |   6760911 ns |   5262781 ns |   8189376 ns |       4907267101 B/s |
| MT: 2 Threads    |      100 |      100 |   5335434 ns |   4586063 ns |   6651911 ns |       6218349651 B/s |
| MT: 3 Threads    |      100 |      100 |   4966139 ns |   4601861 ns |   5631714 ns |       6680763156 B/s |
| MT: 4 Threads    |      100 |      100 |   4763404 ns |   4057093 ns |   5348505 ns |       6965102361 B/s |
| MT: 5 Threads    |      100 |      100 |   4949661 ns |   4567922 ns |   5359622 ns |       6703003791 B/s |
| MT: 6 Threads    |      100 |      100 |   4790888 ns |   4299342 ns |   5247860 ns |       6925145224 B/s |
| MT: 7 Threads    |      100 |      100 |   4788071 ns |   4449139 ns |   5967294 ns |       6929220252 B/s |
| MT: 8 Threads    |      100 |      100 |   4730010 ns |   4366341 ns |   5297597 ns |       7014276228 B/s |
