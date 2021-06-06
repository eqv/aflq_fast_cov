#CPU_TARGET=i386 sh build_qemu_support.sh
#mv afl-qemu-trace afl-qemu-trace_32
CPU_TARGET=x86_64 sh build_qemu_support.sh
mv afl-qemu-trace afl-qemu-trace_64
