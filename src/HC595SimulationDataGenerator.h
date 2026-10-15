#ifndef HC595_SIMULATION_DATA_GENERATOR
#define HC595_SIMULATION_DATA_GENERATOR

#include <AnalyzerHelpers.h>
#include <string>

class HC595AnalyzerSettings;

class HC595SimulationDataGenerator
{
public:
    HC595SimulationDataGenerator();
    ~HC595SimulationDataGenerator();

    void Initialize( U32 simulation_sample_rate, HC595AnalyzerSettings* settings );
    U32 GenerateSimulationData( U64 newest_sample_requested, U32 sample_rate, SimulationChannelDescriptor** simulation_channel );

protected:
    HC595AnalyzerSettings* mSettings;
    U32 mSimulationSampleRateHz;

    // Clock out one word (mBitsPerWord bits) then pulse the latch.
    void ShiftOutWord( U32 value );

    U32 mValue; // running test pattern value

    SimulationChannelDescriptorGroup mSimulationChannels;
    SimulationChannelDescriptor* mDataSim;
    SimulationChannelDescriptor* mShiftSim;
    SimulationChannelDescriptor* mLatchSim;
};

#endif // HC595_SIMULATION_DATA_GENERATOR
