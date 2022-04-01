build/Base_MESI/gem5.opt \
              --debug-flags=ProtocolTrace\
              --outdir=results configs/example/se.py    \
              --num-cpus=4 --mem-size=4GB \
              --l1d_assoc=8 --l2_assoc=16 --l1i_assoc=4 \
              --cpu-type=TimingSimpleCPU \
              --num-dirs=4 --ruby  \
                 -c ./code/thread | tee > testflush.log



