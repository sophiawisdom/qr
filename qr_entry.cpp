#include <torch/extension.h>

#include <vector>

#include<tuple>
using namespace std;

// CUDA forward declarations 

int run_qr(int *lock, unsigned char *output);

#define CHECK_CUDA(x) TORCH_CHECK(x.device().is_cuda(), #x " must be a CUDA tensor")
#define CHECK_CONTIGUOUS(x) TORCH_CHECK(x.is_contiguous(), #x " must be contiguous")
#define CHECK_INPUT(x) CHECK_CUDA(x); CHECK_CONTIGUOUS(x)

int qr_entrypoint(torch::Tensor lock, torch::Tensor output) {
  CHECK_INPUT(lock);
  CHECK_INPUT(output);
  assert(lock.dtype() == torch::kInt32);
  assert(output.dtype() == torch::kUInt8);
  return run_qr((int *)lock.data_ptr(), (unsigned char *)output.data_ptr());
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME, m) {
  m.def("qr", &qr_entrypoint, "cuda qr");
}