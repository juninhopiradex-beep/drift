#pragma once

#include <JuceHeader.h>
#include <random>

// DRIFT: micro-variações naturais para música gerada por IA.
// Módulos: Groove (timing), Attack (transientes), Air (agudos), Grit (textura),
// Amount (quantidade global). Modos: Livre, Sync, Adaptativo.
class DriftProcessor : public juce::AudioProcessor
{
public:
    DriftProcessor();
    ~DriftProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Drift"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    // Medidor para a GUI: desvio atual de timing em ms.
    std::atomic<float> meterTimingMs { 0.0f };

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();

    double sampleRate = 44100.0;
    std::mt19937 rng { 12345u };
    std::uniform_real_distribution<float> uni { -1.0f, 1.0f };

    // Estado de deriva (lenta) para o módulo Air.
    float airDrift = 0.0f;
    float airTarget = 0.0f;

    // Pequeno buffer de atraso para o Groove (até 8 ms).
    juce::AudioBuffer<float> delayBuffer;
    int delayWritePos = 0;
    int delayLength = 0;

    // Suavização do ganho de transientes.
    float attackGain = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DriftProcessor)
};
