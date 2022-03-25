nohup build/Base_MESI/gem5.opt \
              --debug-flags=ProtocolTrace\
              --outdir=results configs/example/ruby_random_test.py    \
              --num-cpus=4 --num-dirs=4 --maxloads=10000000  \
              --l1d_assoc=8 --l2_assoc=16 --l1i_assoc=4 \
              | rotatelogs -n 2 base_mesi_1kw.log 100M
#
