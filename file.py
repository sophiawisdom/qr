from torch.utils.cpp_extension import load
import torch
import random
qr = load(name="qr", sources=["qr_entry.cpp", "qr.cu"], extra_cuda_cflags=["--keep", "--keep-dir", "/workspace/qr/temp", "--extended-lambda"], verbose=True)

import time
output = torch.zeros((1_000_000, 19), device="cuda", dtype=torch.uint8)
lock = torch.zeros((128,), device="cuda", dtype=torch.int32)
lock[64] = random.randint(0, 2**31)

torch.cuda.synchronize()
t0 = time.time()
qr.qr(lock, output)
torch.cuda.synchronize()
t1 = time.time()
print(f"Took {t1-t0:.1f}s")

f = f"output_{int(time.time())}.txt"
output_file = open(f, 'w')
print(f"writing to {f}")
def pr(string):
    output_file.write(string + "\n")
    output_file.flush()
    print(string)

n_results = int(lock[0])
pr(f"Got {n_results} values!")

outputs = output.tolist()
results = outputs[:n_results]
results.sort(key=lambda a:a[-1], reverse=False)
for result in results:
    mask = result[17]
    count = result[18]
    result_str = ''.join([chr(a) for a in result[:17]])
    pr(f"Got {count} for {result_str} on mask {chr(mask)}")

output_file.close()