// JUCE wrapper for Cream Grit. Compiled twice: DELAY edition (default) and
// MASTER edition (CREAM_GRIT_MASTER defined), mirroring the drumlogue builds.
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include "../dsp/cream_dsp_sr.h"

namespace {
using namespace creamvst;

enum class Kind { percent, semi, hertz, plain, db, note, scope, tenthHz, tenthSec, wave, pmode };
struct ParamInfo { const char* id; const char* name; Kind kind; };

const ParamInfo kInfo[] = {
  {"grit","GRIT",Kind::percent},{"clock","CLOCK",Kind::semi},{"body","BODY",Kind::percent},
  {"tone","TONE",Kind::percent},{"split","SPLIT",Kind::hertz},{"bits","BITS",Kind::plain},
  {"drive","DRIVE",Kind::percent},{"level","LEVEL",Kind::db},{"radio","RADIO",Kind::percent},
  {"echo","ECHO",Kind::percent},{"note","NOTE",Kind::note},{"feedback","FEEDBACK",Kind::percent},
  {"pitch","PITCH",Kind::semi},{"pmix","P-MIX",Kind::percent},{"pscope","P-SCOPE",Kind::scope},
  {"pan","PAN",Kind::percent},{"chorus","CHORUS",Kind::percent},{"crate","C-RATE",Kind::tenthHz},
  {"cdepth","C-DEPTH",Kind::percent},{"reverb","REVERB",Kind::percent},{"rdecay","R-DECAY",Kind::tenthSec},
  {"rdamp","R-DAMP",Kind::percent},{"prate","P-RATE",Kind::tenthHz},{"pwave","P-WAVE",Kind::wave},
  {"comp","COMP",Kind::percent},{"pmode","P-MODE",Kind::pmode},
};
#ifndef CREAM_GRIT_MASTER
#error "The VST build is MASTER edition only"
#endif
static_assert(sizeof(kInfo)/sizeof(kInfo[0]) == kCount, "parameter table mismatch");

juce::String formatValue(Kind k, int v) {
  switch (k) {
    case Kind::percent: return juce::String(v) + "%";
    case Kind::semi:    return (v > 0 ? "+" : "") + juce::String(v) + " st";
    case Kind::hertz:   return juce::String(v) + " Hz";
    case Kind::db:      return (v > 0 ? "+" : "") + juce::String(v) + " dB";
    case Kind::note:    return SyncEcho::noteName(v);
    case Kind::scope:   return v ? "FULL" : "HIGH";
    case Kind::tenthHz: return juce::String(v * .1, 1) + " Hz";
    case Kind::tenthSec:return juce::String(v * .1, 1) + " s";
    case Kind::wave:    return v ? "SQUARE" : "SINE";
    case Kind::pmode:   return v == 2 ? "DRUMLOGUE" : v ? "SAMPLER" : "CLEAN";
    default:            return juce::String(v);
  }
}

int parseValue(Kind k, const juce::String& text) {
  const auto t = text.trim().toUpperCase();
  if (k == Kind::note) { for (int i = 0; i < 9; ++i) if (t == SyncEcho::noteName(i)) return i; }
  if (k == Kind::scope) return t.startsWith("F") ? 1 : t.startsWith("H") ? 0 : t.getIntValue();
  if (k == Kind::pmode) return t.startsWith("D") ? 2 : t.startsWith("S") ? 1 : t.startsWith("C") ? 0 : t.getIntValue();
  if (k == Kind::wave) return t.startsWith("SQ") ? 1 : t.startsWith("SI") ? 0 : t.getIntValue();
  if (k == Kind::tenthHz || k == Kind::tenthSec) return juce::roundToInt(t.getDoubleValue() * 10.0);
  return t.getIntValue();
}

constexpr int kHostBpm = 29;

juce::AudioProcessorValueTreeState::ParameterLayout makeLayout() {
  juce::AudioProcessorValueTreeState::ParameterLayout layout;
  for (int i = 0; i < kCount; ++i) {
    const auto& p = kInfo[i];
    const Kind kind = p.kind;
    layout.add(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID{p.id, 1}, p.name, kMin[i], kMax[i], kDefault[i],
        juce::AudioParameterIntAttributes()
            .withStringFromValueFunction([kind](int v, int) { return formatValue(kind, v); })
            .withValueFromStringFunction([kind](const juce::String& s) { return parseValue(kind, s); })));
  }
  // MBPM: plugin-only tempo override. kHostBpm (29) = follow the host, 30-300 = manual.
  layout.add(std::make_unique<juce::AudioParameterInt>(
      juce::ParameterID{"mbpm", 1}, "MBPM", kHostBpm, 300, kHostBpm,
      juce::AudioParameterIntAttributes()
          .withStringFromValueFunction([](int v, int) { return v <= kHostBpm ? juce::String("HOST") : juce::String(v) + " BPM"; })
          .withValueFromStringFunction([](const juce::String& s) {
            return s.trim().toUpperCase().startsWith("H") ? kHostBpm : juce::jlimit(kHostBpm, 300, s.getIntValue()); })));
  return layout;
}
}  // namespace

class CreamGritProcessor : public juce::AudioProcessor {
 public:
  CreamGritProcessor()
      : AudioProcessor(BusesProperties()
                           .withInput("Input", juce::AudioChannelSet::stereo(), true)
                           .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
        state_(*this, nullptr, "CreamGrit", makeLayout()) {
    for (int i = 0; i < kCount; ++i) params_[i] = state_.getRawParameterValue(kInfo[i].id);
    mbpm_ = state_.getRawParameterValue("mbpm");
  }

  const juce::String getName() const override { return JucePlugin_Name; }
  bool acceptsMidi() const override { return false; }
  bool producesMidi() const override { return false; }
  double getTailLengthSeconds() const override { return 10.0; }

  int getNumPrograms() override { return kPresets; }
  int getCurrentProgram() override { return program_; }
  const juce::String getProgramName(int i) override { return presetName(i); }
  void changeProgramName(int, const juce::String&) override {}
  void setCurrentProgram(int i) override {
    if (i < 0 || i >= kPresets) return;
    program_ = i;
    const int* v = presetValues(i);
    for (int k = 0; k < kCount; ++k)
      if (auto* p = state_.getParameter(kInfo[k].id))
        p->setValueNotifyingHost(p->convertTo0to1(static_cast<float>(v[k])));
  }

  bool isBusesLayoutSupported(const BusesLayout& l) const override {
    const auto out = l.getMainOutputChannelSet(), in = l.getMainInputChannelSet();
    return out == juce::AudioChannelSet::stereo() &&
           (in == juce::AudioChannelSet::stereo() || in == juce::AudioChannelSet::mono());
  }

  void prepareToPlay(double sampleRate, int maxBlock) override {
    scratch_.assign(static_cast<size_t>(std::max(maxBlock, 32)) * 2, 0.f);
    pushParams(true);
    dsp_.prepare(static_cast<float>(sampleRate));
  }
  void releaseResources() override {}
  void reset() override { dsp_.reset(); }

  void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
    juce::ScopedNoDenormals noDenormals;
    pushParams(false);
    const int manual = juce::roundToInt(mbpm_->load(std::memory_order_relaxed));
    double bpm = 120.0;  // used when the host provides no tempo
    if (manual > kHostBpm) bpm = manual;
    else if (auto* ph = getPlayHead())
      if (auto pos = ph->getPosition())
        if (auto hostBpm = pos->getBpm()) bpm = *hostBpm;
    dsp_.setTempo(static_cast<float>(bpm));

    const int inCh = getTotalNumInputChannels();
    const int frames = buffer.getNumSamples();
    auto* l = buffer.getWritePointer(0);
    auto* r = buffer.getWritePointer(1);
    const float* inR = inCh > 1 ? buffer.getReadPointer(1) : l;
    const int chunk = static_cast<int>(scratch_.size() / 2);
    for (int pos = 0; pos < frames; pos += chunk) {
      const int n = std::min(chunk, frames - pos);
      for (int i = 0; i < n; ++i) { scratch_[2*i] = l[pos+i]; scratch_[2*i+1] = inR[pos+i]; }
      dsp_.process(scratch_.data(), scratch_.data(), static_cast<size_t>(n));
      for (int i = 0; i < n; ++i) { l[pos+i] = scratch_[2*i]; r[pos+i] = scratch_[2*i+1]; }
    }
  }

  bool hasEditor() const override { return true; }
  juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor(*this); }

  void getStateInformation(juce::MemoryBlock& dest) override {
    auto s = state_.copyState();
    s.setProperty("program", program_, nullptr);
    if (auto xml = s.createXml()) copyXmlToBinary(*xml, dest);
  }
  void setStateInformation(const void* data, int size) override {
    if (auto xml = getXmlFromBinary(data, size))
      if (xml->hasTagName(state_.state.getType())) {
        auto s = juce::ValueTree::fromXml(*xml);
        program_ = s.getProperty("program", 0);
        state_.replaceState(s);
      }
  }

 private:
  void pushParams(bool force) {
    int v[kCount]; bool changed = force;
    for (int i = 0; i < kCount; ++i) {
      v[i] = juce::roundToInt(params_[i]->load(std::memory_order_relaxed));
      changed |= v[i] != cached_[i];
      cached_[i] = v[i];
    }
    if (changed) dsp_.set(v);
  }

  juce::AudioProcessorValueTreeState state_;
  std::array<std::atomic<float>*, kCount> params_{};
  std::atomic<float>* mbpm_ = nullptr;
  int cached_[kCount] = {};
  int program_ = 0;
  std::vector<float> scratch_;
  DSP dsp_;
  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CreamGritProcessor)
};

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new CreamGritProcessor(); }
