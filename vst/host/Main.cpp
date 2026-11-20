// Loopback FX Host: captures what Windows plays to one output (WASAPI loopback),
// runs it through a VST3 effect and plays the result on another output.
#include <juce_audio_utils/juce_audio_utils.h>
#include "LoopbackCapture.h"

namespace {
const char* kDefaultPlugin = "C:\\Program Files\\Common Files\\VST3\\Cream Grit M.vst3";

juce::String toJuce(const std::wstring& s) { return juce::String(s.c_str()); }

class FreePlayHead : public juce::AudioPlayHead {
 public:
  std::atomic<double> bpm{120.0};
  int64_t samples = 0; double rate = 48000;
  juce::Optional<PositionInfo> getPosition() const override {
    PositionInfo p;
    p.setBpm(bpm.load()); p.setIsPlaying(true);
    p.setTimeInSamples(samples); p.setTimeInSeconds(samples / rate);
    p.setPpqPosition(samples / rate * bpm.load() / 60.0);
    p.setTimeSignature(juce::AudioPlayHead::TimeSignature{});
    return p;
  }
};
}  // namespace

class HostComponent : public juce::Component, public juce::AudioIODeviceCallback, private juce::Timer {
 public:
  HostComponent() {
    juce::PropertiesFile::Options o;
    o.applicationName = "LoopbackFxHost"; o.filenameSuffix = ".settings"; o.osxLibrarySubFolder = "Application Support";
    props_ = std::make_unique<juce::PropertiesFile>(o);

    formats_.addFormat(std::make_unique<juce::VST3PluginFormat>());

    addAndMakeVisible(sourceLabel_); sourceLabel_.setText(juce::String::fromUTF8("取り込み元（Windowsの再生先）"), juce::dontSendNotification);
    addAndMakeVisible(source_);
    source_.onChange = [this] { startCapture(); };
    addAndMakeVisible(refresh_); refresh_.setButtonText(juce::String::fromUTF8("更新")); refresh_.onClick = [this] { fillSources(); };
    addAndMakeVisible(outBtn_); outBtn_.setButtonText(juce::String::fromUTF8("出力デバイス…")); outBtn_.onClick = [this] { showDeviceDialog(); };
    addAndMakeVisible(loadBtn_); loadBtn_.setButtonText(juce::String::fromUTF8("プラグイン読込…")); loadBtn_.onClick = [this] { choosePlugin(); };
    addAndMakeVisible(bypass_); bypass_.setButtonText("Bypass");
    addAndMakeVisible(bpmLabel_); bpmLabel_.setText("BPM", juce::dontSendNotification);
    addAndMakeVisible(bpm_);
    bpm_.setRange(30, 300, 1); bpm_.setSliderStyle(juce::Slider::IncDecButtons);
    bpm_.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 24);
    bpm_.setValue(props_->getDoubleValue("bpm", 120));
    bpm_.onValueChange = [this] { playHead_.bpm = bpm_.getValue(); props_->setValue("bpm", bpm_.getValue()); };
    playHead_.bpm = bpm_.getValue();
    addAndMakeVisible(status_); status_.setFont(juce::FontOptions(13.f));

    std::unique_ptr<juce::XmlElement> devXml = props_->getXmlValue("audio");
    devices_.initialise(0, 2, devXml.get(), true);
    devices_.addAudioCallback(this);
    devices_.addChangeListener(&saver_);
    saver_.fn = [this] { if (auto x = devices_.createStateXml()) props_->setValue("audio", x.get()); };

    fillSources();
    loadPlugin(props_->getValue("plugin", kDefaultPlugin));
    setSize(720, 200);
    startTimerHz(4);
  }
  ~HostComponent() override {
    stopTimer();
    devices_.removeAudioCallback(this);
    capture_.stop();
    editor_.reset();
    const juce::SpinLock::ScopedLockType sl(lock_);
    plugin_.reset();
  }

  void resized() override {
    auto r = getLocalBounds().reduced(10);
    auto row = r.removeFromTop(28);
    sourceLabel_.setBounds(row.removeFromLeft(200));
    refresh_.setBounds(row.removeFromRight(60)); row.removeFromRight(6);
    source_.setBounds(row);
    r.removeFromTop(6);
    row = r.removeFromTop(28);
    outBtn_.setBounds(row.removeFromLeft(130)); row.removeFromLeft(6);
    loadBtn_.setBounds(row.removeFromLeft(130)); row.removeFromLeft(6);
    bypass_.setBounds(row.removeFromLeft(90)); row.removeFromLeft(6);
    bpmLabel_.setBounds(row.removeFromLeft(40));
    bpm_.setBounds(row.removeFromLeft(140));
    r.removeFromTop(4);
    status_.setBounds(r.removeFromTop(40));
    r.removeFromTop(4);
    if (editor_) editor_->setBounds(r.withSize(editor_->getWidth(), editor_->getHeight()));
  }

  // ---------------------------------------------------------------- audio
  void audioDeviceAboutToStart(juce::AudioIODevice* d) override {
    outRate_ = d->getCurrentSampleRate(); block_ = d->getCurrentBufferSizeSamples();
    tmp_.setSize(2, std::max(block_, 4096));
    prepareAll();
  }
  void audioDeviceStopped() override {}
  void audioDeviceIOCallbackWithContext(const float* const*, int, float* const* outs, int numOut, int n,
                                        const juce::AudioIODeviceCallbackContext&) override {
    juce::ScopedNoDenormals nd;
    const juce::SpinLock::ScopedTryLockType sl(lock_);
    if (!sl.isLocked() || n > tmp_.getNumSamples()) { for (int c = 0; c < numOut; ++c) if (outs[c]) juce::FloatVectorOperations::clear(outs[c], n); return; }
    const bool ok = reader_.read(capture_.fifo, tmp_.getWritePointer(0), tmp_.getWritePointer(1), n);
    if (!ok) ++underruns_;
    juce::AudioBuffer<float> view(tmp_.getArrayOfWritePointers(), 2, n);
    if (plugin_ && !bypass_.getToggleState()) {
      juce::MidiBuffer midi;
      plugin_->processBlock(view, midi);
    }
    playHead_.samples += n;
    float pk = 0;
    for (int c = 0; c < numOut; ++c) {
      if (!outs[c]) continue;
      if (c < 2) { juce::FloatVectorOperations::copy(outs[c], view.getReadPointer(c), n); pk = std::max(pk, view.getMagnitude(c, 0, n)); }
      else juce::FloatVectorOperations::clear(outs[c], n);
    }
    peak_ = std::max(peak_.load(), pk);
  }

 private:
  struct Saver : juce::ChangeListener { std::function<void()> fn; void changeListenerCallback(juce::ChangeBroadcaster*) override { if (fn) fn(); } } saver_;

  void prepareAll() {
    const juce::SpinLock::ScopedLockType sl(lock_);
    const double inRate = capture_.sampleRate();
    reader_.prepare(inRate, outRate_, size_t(std::max(2.0 * block_, 0.015 * inRate)));
    capture_.fifo.clear();
    playHead_.rate = outRate_;
    if (plugin_) {
      plugin_->releaseResources();
      plugin_->setPlayConfigDetails(2, 2, outRate_, block_);
      plugin_->prepareToPlay(outRate_, std::max(block_, 4096));
    }
  }

  void fillSources() {
    endpoints_ = listRenderEndpoints();
    const auto saved = props_->getValue("source");
    source_.clear(juce::dontSendNotification);
    int sel = 0;
    for (size_t i = 0; i < endpoints_.size(); ++i) {
      source_.addItem(toJuce(endpoints_[i].name), int(i) + 1);
      if (toJuce(endpoints_[i].id) == saved) sel = int(i) + 1;
    }
    if (sel) source_.setSelectedId(sel, juce::sendNotificationSync);
  }

  void startCapture() {
    const int i = source_.getSelectedId() - 1;
    if (i < 0 || i >= int(endpoints_.size())) return;
    props_->setValue("source", toJuce(endpoints_[i].id));
    devices_.removeAudioCallback(this);
    if (!capture_.start(endpoints_[i].id))
      juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Loopback",
          juce::String::fromUTF8("取り込みを開始できません: ") + capture_.error());
    devices_.addAudioCallback(this);  // re-runs audioDeviceAboutToStart -> prepareAll
  }

  void loadPlugin(const juce::String& path) {
    juce::OwnedArray<juce::PluginDescription> found;
    for (auto* f : formats_.getFormats()) f->findAllTypesForFile(found, path);
    if (found.isEmpty()) { status_.setText(juce::String::fromUTF8("プラグインが見つかりません: ") + path, juce::dontSendNotification); return; }
    juce::String err;
    auto inst = formats_.createPluginInstance(*found[0], outRate_ > 0 ? outRate_ : 48000, std::max(block_, 512), err);
    if (!inst) { status_.setText(err, juce::dontSendNotification); return; }
    inst->setPlayHead(&playHead_);
    editor_.reset();
    devices_.removeAudioCallback(this);
    { const juce::SpinLock::ScopedLockType sl(lock_); plugin_ = std::move(inst); }
    devices_.addAudioCallback(this);
    props_->setValue("plugin", path);
    pluginName_ = plugin_->getName();
    if (auto* e = plugin_->createEditorIfNeeded()) {
      editor_.reset(e);
      addAndMakeVisible(*editor_);
      setSize(std::max(720, editor_->getWidth() + 20), 120 + editor_->getHeight());
    }
    resized();
  }

  void choosePlugin() {
    chooser_ = std::make_unique<juce::FileChooser>("VST3", juce::File("C:\\Program Files\\Common Files\\VST3"), "*.vst3");
    chooser_->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles | juce::FileBrowserComponent::canSelectDirectories,
                          [this](const juce::FileChooser& fc) { if (fc.getResult() != juce::File()) loadPlugin(fc.getResult().getFullPathName()); });
  }

  void showDeviceDialog() {
    auto* sel = new juce::AudioDeviceSelectorComponent(devices_, 0, 0, 2, 2, false, false, true, false);
    sel->setSize(500, 360);
    juce::DialogWindow::LaunchOptions o;
    o.content.setOwned(sel); o.dialogTitle = juce::String::fromUTF8("出力デバイス");
    o.useNativeTitleBar = true; o.resizable = false;
    o.launchAsync();
  }

  void timerCallback() override {
    auto* d = devices_.getCurrentAudioDevice();
    const juce::String outName = d ? d->getName() : "-";
    const int si = source_.getSelectedId() - 1;
    const juce::String inName = si >= 0 && si < int(endpoints_.size()) ? toJuce(endpoints_[si].name) : "-";
    juce::String s;
    s << pluginName_ << "   in " << inName << " @" << int(capture_.sampleRate()) << "  ->  out " << outName << " @" << int(outRate_)
      << "\nbuffer " << int(capture_.fifo.available()) << " fr, ratio " << juce::String(reader_.ratio(), 5)
      << ", underruns " << int(underruns_.load()) << ", peak " << juce::String(juce::Decibels::gainToDecibels(peak_.exchange(0.f)), 1) << " dB";
    const bool loop = d && inName.isNotEmpty() && inName == outName;
    if (loop) s << juce::String::fromUTF8("   ⚠ 取り込み元と出力が同じ（ハウリング）");
    status_.setColour(juce::Label::textColourId, loop ? juce::Colours::orangered : juce::Colours::lightgrey);
    status_.setText(s, juce::dontSendNotification);
  }

  std::unique_ptr<juce::PropertiesFile> props_;
  juce::AudioPluginFormatManager formats_;
  juce::AudioDeviceManager devices_;
  std::unique_ptr<juce::AudioPluginInstance> plugin_;
  std::unique_ptr<juce::AudioProcessorEditor> editor_;
  std::unique_ptr<juce::FileChooser> chooser_;
  juce::String pluginName_;
  juce::SpinLock lock_;
  FreePlayHead playHead_;
  LoopbackCapture capture_;
  DriftReader reader_;
  std::vector<RenderEndpoint> endpoints_;
  juce::AudioBuffer<float> tmp_;
  double outRate_ = 48000; int block_ = 512;
  std::atomic<int> underruns_{0};
  std::atomic<float> peak_{0.f};
  juce::Label sourceLabel_, bpmLabel_, status_;
  juce::ComboBox source_;
  juce::TextButton refresh_, outBtn_, loadBtn_;
  juce::ToggleButton bypass_;
  juce::Slider bpm_;
};

class HostApp : public juce::JUCEApplication {
 public:
  const juce::String getApplicationName() override { return "Loopback FX Host"; }
  const juce::String getApplicationVersion() override { return "0.1.0"; }
  void initialise(const juce::String&) override { window_ = std::make_unique<Window>(); }
  void shutdown() override { window_.reset(); }
 private:
  struct Window : juce::DocumentWindow {
    Window() : DocumentWindow("Loopback FX Host", juce::Colour(0xff202428), allButtons) {
      setUsingNativeTitleBar(true); setContentOwned(new HostComponent(), true);
      setResizable(true, false); centreWithSize(getWidth(), getHeight()); setVisible(true);
    }
    void closeButtonPressed() override { juce::JUCEApplication::quit(); }
  };
  std::unique_ptr<Window> window_;
};

START_JUCE_APPLICATION(HostApp)
