# Qt Serialization Benchmarks

<div align="center">
  <img src="data_layout.png" alt="Data Layout" width="60%">
</div>

<br>

Benchmarks of Qt serialization formats using a `TaskManager` with **10,000 tasks**
(Qt 6.11.0 dynamic release build, Apple LLVM 17.0 runnin on Apple M4, macos 26.0,1)

| Format        | Serialization Time | Deserialization Time | Serialized Size |
|---------------|--------------------|----------------------|-----------------|
| QDataStream   | 0.50 ms            | 1 ms                 | 1127 KB         |
| XML           | 7 ms               | 7.2 ms               | 1757 KB         |
| JSON          | 14 ms              | 5.2 ms               | 2005 KB         |
| CBOR          | 10 ms              | 9.2 ms               | 1511 KB         |
| CBORStream    | 4 ms               | 3.5 ms               | 1690 KB         |
| QtProtobuf    | 10 ms              | 7.7 ms               | 890 KB          |

