nohup ~/gem5/build/Base_MESI/gem5.opt \
              --debug-flags=ProtocolTrace\
              --outdir=results  ~/gem5/configs/example/ruby_random_test.py    \
              --num-cpus=4 --num-dirs=4 --maxloads=10  \
              --l1d_assoc=8 --l2_assoc=16 --l1i_assoc=4 \
              | tee > test.log
#