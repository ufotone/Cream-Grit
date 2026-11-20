// WASAPI loopback capture of a playback (render) endpoint into a lock-free stereo FIFO,
// plus a drift-compensating reader for the audio callback.
#pragma once
#define NOMINMAX
#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <functiondiscoverykeys_devpkey.h>
#include <atomic>
#include <cmath>
#include <string>
#include <thread>
#include <vector>

struct RenderEndpoint { std::wstring id, name; };

template <class T> struct ComPtr {
  T* p = nullptr;
  ~ComPtr() { if (p) p->Release(); }
  T** operator&() { return &p; }
  T* operator->() const { return p; }
  explicit operator bool() const { return p != nullptr; }
};

inline std::vector<RenderEndpoint> listRenderEndpoints() {
  std::vector<RenderEndpoint> out;
  CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
  ComPtr<IMMDeviceEnumerator> en;
  if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&en))) return out;
  ComPtr<IMMDeviceCollection> col;
  if (FAILED(en->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &col))) return out;
  UINT n = 0; col->GetCount(&n);
  for (UINT i = 0; i < n; ++i) {
    ComPtr<IMMDevice> dev; if (FAILED(col->Item(i, &dev))) continue;
    LPWSTR id = nullptr; dev->GetId(&id);
    ComPtr<IPropertyStore> ps; PROPVARIANT v; PropVariantInit(&v);
    std::wstring name = L"?";
    if (SUCCEEDED(dev->OpenPropertyStore(STGM_READ, &ps)) && SUCCEEDED(ps->GetValue(PKEY_Device_FriendlyName, &v)) && v.vt == VT_LPWSTR) name = v.pwszVal;
    PropVariantClear(&v);
    out.push_back({id ? id : L"", name});
    CoTaskMemFree(id);
  }
  return out;
}

// Single-producer / single-consumer ring of stereo frames.
class StereoFifo {
 public:
  explicit StereoFifo(size_t frames = 1 << 16) : buf_(frames * 2), mask_(frames - 1) {}
  size_t available() const { return w_.load(std::memory_order_acquire) - r_.load(std::memory_order_acquire); }
  size_t capacity() const { return mask_ + 1; }
  void push(const float* lr, size_t frames) {
    size_t w = w_.load(std::memory_order_relaxed);
    const size_t r = r_.load(std::memory_order_acquire);
    if (w - r + frames > capacity()) return;  // overflow: drop (reader resyncs)
    for (size_t i = 0; i < frames; ++i, ++w) { buf_[(w & mask_) * 2] = lr[2*i]; buf_[(w & mask_) * 2 + 1] = lr[2*i+1]; }
    w_.store(w, std::memory_order_release);
  }
  float peek(size_t offset, int ch) const { return buf_[((r_.load(std::memory_order_relaxed) + offset) & mask_) * 2 + ch]; }
  void skip(size_t frames) { r_.fetch_add(frames, std::memory_order_release); }
  void clear() { r_.store(w_.load(std::memory_order_acquire), std::memory_order_release); }
 private:
  std::vector<float> buf_;
  size_t mask_;
  std::atomic<size_t> w_{0}, r_{0};
};

class LoopbackCapture {
 public:
  ~LoopbackCapture() { stop(); }
  bool start(const std::wstring& deviceId) {
    stop();
    running_ = true; ok_ = false; error_.clear();
    thread_ = std::thread([this, deviceId] { run(deviceId); });
    for (int i = 0; i < 200 && running_ && !ok_ && error_.empty(); ++i) Sleep(5);
    return ok_;
  }
  void stop() { running_ = false; if (thread_.joinable()) thread_.join(); ok_ = false; }
  double sampleRate() const { return rate_; }
  const std::string& error() const { return error_; }
  StereoFifo fifo;

 private:
  void run(std::wstring id) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    {
      ComPtr<IMMDeviceEnumerator> en; ComPtr<IMMDevice> dev; ComPtr<IAudioClient> client; ComPtr<IAudioCaptureClient> cap;
      WAVEFORMATEX* wf = nullptr;
      auto fail = [&](const char* m) { error_ = m; running_ = false; };
      if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&en))) { fail("enumerator"); goto done; }
      if (FAILED(en->GetDevice(id.c_str(), &dev))) { fail("device not found"); goto done; }
      if (FAILED(dev->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&client))) { fail("activate"); goto done; }
      client->GetMixFormat(&wf);
      if (FAILED(client->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_LOOPBACK, 2000000, 0, wf, nullptr))) { fail("initialize loopback"); goto done; }
      if (FAILED(client->GetService(__uuidof(IAudioCaptureClient), (void**)&cap))) { fail("capture client"); goto done; }
      {
        const int ch = wf->nChannels, bits = wf->wBitsPerSample;
        bool isFloat = wf->wFormatTag == WAVE_FORMAT_IEEE_FLOAT;
        if (wf->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
          static const GUID kFloat = {0x00000003, 0x0000, 0x0010, {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71}};
          isFloat = IsEqualGUID(reinterpret_cast<WAVEFORMATEXTENSIBLE*>(wf)->SubFormat, kFloat) != 0;
        }
        rate_ = wf->nSamplesPerSec;
        client->Start();
        ok_ = true;
        std::vector<float> tmp;
        while (running_) {
          UINT32 packet = 0;
          while (SUCCEEDED(cap->GetNextPacketSize(&packet)) && packet > 0) {
            BYTE* data = nullptr; UINT32 frames = 0; DWORD flags = 0;
            if (FAILED(cap->GetBuffer(&data, &frames, &flags, nullptr, nullptr))) break;
            tmp.assign(frames * 2, 0.f);
            if (!(flags & AUDCLNT_BUFFERFLAGS_SILENT))
              for (UINT32 f = 0; f < frames; ++f)
                for (int c = 0; c < 2; ++c) {
                  const int src = std::min(c, ch - 1), idx = f * ch + src;
                  float v;
                  if (isFloat) v = reinterpret_cast<const float*>(data)[idx];
                  else if (bits == 16) v = reinterpret_cast<const int16_t*>(data)[idx] / 32768.f;
                  else if (bits == 32) v = reinterpret_cast<const int32_t*>(data)[idx] / 2147483648.f;
                  else { const BYTE* b = data + idx * 3; v = int32_t((b[0] << 8) | (b[1] << 16) | (b[2] << 24)) / 2147483648.f; }
                  tmp[2*f + c] = v;
                }
            fifo.push(tmp.data(), frames);
            cap->ReleaseBuffer(frames);
          }
          Sleep(2);
        }
        client->Stop();
      }
    done:
      if (wf) CoTaskMemFree(wf);
    }
    CoUninitialize();
  }
  std::thread thread_;
  std::atomic<bool> running_{false}, ok_{false};
  std::string error_;
  double rate_ = 48000;
};

// Reads the FIFO at outRate with linear-interpolation resampling and a slow control loop
// that keeps the fill level near `targetFrames` (absorbs clock drift between devices).
class DriftReader {
 public:
  void prepare(double inRate, double outRate, size_t targetFrames) {
    base_ = inRate / outRate; target_ = double(targetFrames); frac_ = 0; primed_ = false; ratio_ = base_;
  }
  // Returns false when underrun (output silence).
  bool read(StereoFifo& f, float* l, float* r, int n) {
    const double avail = double(f.available());
    if (!primed_) { if (avail < target_) { fill0(l, r, n); return false; } primed_ = true; }
    if (avail > target_ * 4) { f.skip(size_t(avail - target_)); frac_ = 0; }
    const double err = (double(f.available()) - target_) / target_;
    ratio_ += 0.0005 * ((base_ * (1.0 + 0.002 * std::max(-1.0, std::min(1.0, err)))) - ratio_);
    for (int i = 0; i < n; ++i) {
      if (f.available() < 2) { fill0(l + i, r + i, n - i); primed_ = false; return false; }
      l[i] = f.peek(0, 0) + float(frac_) * (f.peek(1, 0) - f.peek(0, 0));
      r[i] = f.peek(0, 1) + float(frac_) * (f.peek(1, 1) - f.peek(0, 1));
      frac_ += ratio_;
      const size_t step = size_t(frac_);
      if (step) { f.skip(step); frac_ -= double(step); }
    }
    return true;
  }
  double ratio() const { return ratio_; }
 private:
  static void fill0(float* l, float* r, int n) { for (int i = 0; i < n; ++i) l[i] = r[i] = 0.f; }
  double base_ = 1, ratio_ = 1, target_ = 1024, frac_ = 0;
  bool primed_ = false;
};
