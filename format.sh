#! /bin/sh
find src -name '*.?pp' | xargs clang-format -i

