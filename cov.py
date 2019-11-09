import os
import time
import struct
import signal
import glob
import argparse

QEMU_PATH = os.path.dirname(os.path.realpath(__file__))+"/afl-qemu-trace"


INPUT_FILE = "/dev/shm/coverage_input" #each input is stored here


parser = argparse.ArgumentParser(description='Fast Forkserver Base Binary Coverage')
 
parser.add_argument('-i', metavar='input_folder',  help='Where to look for input files', required=True)
parser.add_argument('-o', metavar='output_folder', help='Where to store the trace files', required=True)
parser.add_argument('-m', metavar='wordize',choices=["32","64"], help='Architecture to use for QEMU', required=True)
parser.add_argument('-t', nargs='?', metavar="timeout", const=1.0, default=1.0, type=float, help='Timeout in seconds')
parser.add_argument('cmd', metavar="...", nargs=argparse.REMAINDER, help = 'Command to run, @@ is replaced by input file')

def replace_input(f, file):
    if f == "@@":
        return file
    else:
        return f
args = parser.parse_args()
ARGS = [replace_input(f,INPUT_FILE) for f in args.cmd]
INDIR = args.i
OUTDIR = args.o
QEMU_PATH += "_"+args.m
TIMEOUT = args.t
print("Running %s with timeout %f on %d inputs\n"%(" ".join(ARGS), TIMEOUT, len(glob.glob(INDIR+"/*"))))
HIDE_OUTPUT = True

def handle_timeout(pid):
    os.kill(pid, signal.SIGKILL)

class Forkserver:
    def __init__(self):
        self.ctl_out, self.ctl_in = os.pipe()
        self.st_out, self.st_in = os.pipe()
        self.in_file = open(INPUT_FILE, "w+")

        fork_pid = os.fork()
        if fork_pid == 0:
            self.child()
        else:
            self.parent()

    def child(self):
        FORKSRV_FD = 198 # from AFL config.h
        os.dup2(self.ctl_out, FORKSRV_FD)
        os.dup2(self.st_in, FORKSRV_FD+1)

        if HIDE_OUTPUT:
            null = open("/dev/null","w")
            null_fd = null.fileno()
            os.dup2(null_fd, 1)
            os.dup2(null_fd, 2)
            null.close()

        os.dup2(self.in_file.fileno(),0)
        os.close(self.in_file.fileno())

        os.close(self.ctl_in);
        os.close(self.ctl_out);
        os.close(self.st_in);
        os.close(self.st_out);
        env = {"TRACE_OUT_DIR": OUTDIR,
               "QEMU_LOG": "nochain",
               } 
        os.execve(QEMU_PATH,["afl-qemu-trace"]+ARGS, env)
        print("child failed")

    def parent(self):

            os.close(self.ctl_out);
            os.close(self.st_in);
            os.read(self.st_out, 4);

    def run(self, testcase):
        self.in_file.truncate(0)
        self.in_file.seek(0)
        self.in_file.write(testcase)
        self.in_file.seek(0)

        os.write(self.ctl_in, "\0\0\0\0")

        pid = struct.unpack("I",os.read(self.st_out, 4))[0]

        signal.signal(signal.SIGALRM, lambda signum,sigfr : handle_timeout(pid))
        signal.setitimer(signal.ITIMER_REAL, TIMEOUT, 0)
        while True:
            try:
                status = os.read(self.st_out, 4)
            except OSError:
                continue
            break
        signal.setitimer(signal.ITIMER_REAL, 0, 0) # disable timer for timeout
        if status:
            status = struct.unpack("I",status)[0]
        return status

frk = Forkserver()
if not os.path.isdir(OUTDIR):
    os.mkdir(OUTDIR)

for p in glob.glob(INDIR+"/*"):
    with open(p, 'r') as f:
        print("run %s"%p)
        frk.run(f.read())
        for trace in glob.glob(OUTDIR+"/trace_thread_*.qemu"):
            #print("mv", trace, OUTDIR+"/input_%s_thread_%s"%(os.path.basename(p), trace.split("_")[-1]))
            os.rename(trace, OUTDIR+"/input_%s_thread_%s"%(os.path.basename(p), trace.split("_")[-1]))
