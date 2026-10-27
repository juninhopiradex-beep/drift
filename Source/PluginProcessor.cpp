#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace ids
{
    static const juce::String groove  = "groove";
    static const juce::String attack  = "attack";
    static const juce::String air     = "air";
    static const juce::String grit    = "grit";
    static const juce::String amount  = "amount";
    static const juce::String mode    = "mode";
}

DriftProcessor::DriftProcessor()
    : AudioProcessor(BusesProperties()
                        .withInput("Input", juce::AudioChannelSet::stereo(), true)
                        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMS", createLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout DriftProcessor::createLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ids::groove, 1 }, "Groove", 0.0f, 100.0f, 42.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ids::attack, 1 }, "Attack", 0.0f, 100.0f, 60.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ids::air, 1 }, "Air", 0.0f, 100.0f, 18.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ids::grit, 1 }, "Grit", 0.0f, 100.0f, 25.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { ids::amount, 1 }, "Amount", 0.0f, 100.0f, 60.0f));
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { ids::mode, 1 }, "Mode",
        juce::StringArray { "Livre", "Sync", "Adaptativo" }, 0));

    return { params.begin(), params.end() };
}

bool DriftProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo();
}

void DriftProcessor::prepareToPlay(double sr, int)
{
    sampleRate = sr;
    delayLength = juce::jmax(1, juce::roundToInt(sr * 0.008)); // 8 ms máx.
    delayBuffer.setSize(2, delayLength + 1, false, true, false);
    delayBuffer.clear();
    delayWritePos = 0;
    airDrift = 0.0f;
    airTarget = 0.0f;
    attackGain = 1.0f;
}

void DriftProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto amount = apvts.getRawParameterValue(ids::amount)->load() / 100.0f;
    const auto groove = apvts.getRawParameterValue(ids::groove)->load() / 100.0f;
    const auto attack = apvts.getRawParameterValue(ids::attack)->load() / 100.0f;
    const auto air    = apvts.getRawParameterValue(ids::air)->load() / 100.0f;
    const auto grit   = apvts.getRawParameterValue(ids::grit)->load() / 100.0f;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = juce::jmin(2, buffer.getNumChannels());

    // Groove: atraso variável de 0 a 8 ms, escalado por Groove x Amount.
    const float maxDelaySamples = (float) delayLength * groove * amount;

    // Air: deriva lenta de ganho em agudos (simplificada: tilt de 1 polo).
    // Grit: ruído "velvet" de baixa amplitude.
    const float gritAmp = grit * amount * 0.01f;

    float timingAccumMs = 0.0f;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* data = buffer.getWritePointer(ch);
        auto* delay = delayBuffer.getWritePointer(ch);
        float state = 0.0f;

        for (int i = 0; i < numSamples; ++i)
        {
            // Escreve no buffer circular.
            delay[delayWritePos] = data[i];

            // Lê com deslocamento variável (interpolação linear simples).
            float offset = maxDelaySamples * (0.5f + 0.5f * uni(rng));
            float readPos = (float) delayWritePos - offset;
            if (readPos < 0.0f) readPos += (float) (delayLength + 1);
            int i0 = (int) readPos;
            int i1 = (i0 + 1) % (delayLength + 1);
            float frac = readPos - (float) i0;
            float delayed = delay[i0] + frac * (delay[i1] - delay[i0]);

            // Attack: variação de ganho por golpe, suavizada.
            attackGain += (1.0f + attack * 0.2f * uni(rng) - attackGain) * 0.001f;

            // Air: deriva lenta de tilt (um polo, suavização de agudos).
            if (ch == 0 && (i % 512) == 0)
                airTarget = uni(rng) * air;
            airDrift += (airTarget - airDrift) * 0.0005f;
            float cutoff = 0.5f + 0.5f * airDrift;
            state += cutoff * (delayed - state);
            float highs = delayed - state;
            float airOut = delayed - highs * (air * 0.5f);

            // Grit: ruído subtil.
            float noise = gritAmp * uni(rng);

            data[i] = (delayed * (1.0f - amount) + airOut * amount) * attackGain + noise;

            ++delayWritePos;
            if (delayWritePos > delayLength) delayWritePos = 0;
        }

        timingAccumMs += maxDelaySamples / (float) sampleRate * 1000.0f;
    }

    meterTimingMs.store(timingAccumMs / (float) juce::jmax(1, numChannels));
}

juce::AudioProcessorEditor* DriftProcessor::createEditor()
{
    return new DriftEditor(*this);
}

void DriftProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void DriftProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DriftProcessor();
}
